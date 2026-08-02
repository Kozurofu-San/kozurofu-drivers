#pragma once

#include "Hmc830Const.h"

#include "interface/Generator.h"
#include "interface/Timer.h"
#include "interface/Spi.h"

/* // RF generator

#include "device/Hmc830/Hmc830.h"

    SpiController spi1 {SPI1};
    GpioDriver gpio_rfgen_cs {GPIOA, 4};
    SpiDriver spi_rfgen {spi1};
    Hmc830Driver rfgen {spi_rfgen, timer_us};

    // SPI1
    GpioDriver::remap(AFIO_MAPR_SPI1_REMAP, false);
    GpioDriver::mode(GPIOA, 4, GpioDriver::Mode::AlternatePushpull); // NSS
    GpioDriver::mode(GPIOA, 5, GpioDriver::Mode::AlternatePushpull); // SCK
    GpioDriver::mode(GPIOA, 6, GpioDriver::Mode::Input);             // MISO
    GpioDriver::mode(GPIOA, 7, GpioDriver::Mode::AlternatePushpull); // MOSI
    CHECK(p.spi1.init(1'000'000));

    // Timer
    CHECK(p.timer_us.init({1, ITimer::Units::us}));
    p.timer_us.start();

    // RF generator
    p.gpio_rfgen_cs.init(GpioDriver::Mode::OutputPushpull, GpioDriver::Speed::High);
    p.spi_rfgen.init(&p.gpio_rfgen_cs, 0);
    p.rfgen.init(Hmc830Driver::Mode::Integer);

    // Example
    bool ret = p.rfgen.setFrequency(50'000'000);
    p.rfgen.setPower(9);
*/

namespace driver
{

class Hmc830Driver: public IGenerator
{

public:

    enum Mode: uint8_t
    {
        Fractional = 0,
        Integer    = 1
    };

    Hmc830Driver(ISpi &p, ITimer &timer)
        : _p(p), _timer(timer) {}

    bool init(Mode mode = Mode::Integer, bool singleEnded = true)
    {
        _vco.mode = mode;
        _vco.se   = singleEnded;

        // HMC mode: rise SEN -> rise CLK
        // Open mode: rise CLK -> rise SEN
        _timer.delay(100);
        _p.enable();
        _timer.delay(100);
        _p.disable();

        uint32_t id = read(Hmc830::ID);
        printf("HMC830 ID 0x%lX\n", id);

        // Main registers
        write(Hmc830::OpenMode, Hmc830::SoftReset);         // R0
        _timer.delay(100);
        // Select the SPI enable bit instead of relying on the board CEN pin.
        write(Hmc830::Enables, Hmc830::chipen_from_spi);    // R1 0x000002
        write(Hmc830::REFDIV, 40);                          // R2  → f_PFD = 1 MHz

        // VCO subregister defaults recommended for the wideband HMC830.
        // write(Hmc830::VCOSPI, 0x7FB0);                      // R5 → VCO R6
        write(Hmc830::VCOSPI, 0x1628);                      // R5 → VCO R5
        write(Hmc830::VCOSPI, 0x60A0);                      // R5 → VCO R4
        turn(Turn::On);                                     // R5 → VCO R1
        setVcoParameters();                                 // R5 → VCO R2 and R3

        // Main-register values from the HMC830 reference configuration.
        // The two SDCFG values differ only in fractional bypass/enable.
        write(Hmc830::SDCFG,                    // R6 mode == Mode::Integer ? 0x0307CA : 0x030F4A
            mode << Hmc830::frac_bypass   |     // bypass Σ-Δ
            1 << Hmc830::AutoSeed         |
            1 << Hmc830::clkrq_refdiv_sel |
            !mode << Hmc830::SD_Enable);          // Σ-Δ off
        write(Hmc830::LockDetect,                           // R7 0x00014D
            1 << Hmc830::Enable_Internal_Lock_Detect    |
            1 << Hmc830::Lock_Detect_Window_type        |
            1 << Hmc830::LD_Digital_Window_duration     |
            3 << Hmc830::LD_Digital_Timer_Freq_Control  |
            0 << Hmc830::LD_Timer_Test_Mode             |
            0 << Hmc830::Auto_Relock_One_Try);
        write(Hmc830::AnalogEn, 0xC1BEFF);                  // R8 0xC1BEFF
        write(Hmc830::ChargePump,                           // R9 0x153FFF
            (1000U / 20) << Hmc830::CP_DN_Gain  |
            (1000U / 20) << Hmc830::CP_UP_Gain  |
            (100U  /  5) << Hmc830::Offset_Magnitude);
        write(Hmc830::VCOAutoCal,                           // R10 0x002046
            6 << Hmc830::Vtune_Resolution      |
            1 << Hmc830::Wait_State_Set_Up     |
            1 << Hmc830::FSM_VSPI_Clock_Select);
        write(Hmc830::PD, 0x07C061);                        // R11 0x07C061
        write(Hmc830::FineFrequencyCorrection, 0);          // R12
        write(Hmc830::GPO_SPI_RDIV,                         // R15 0x000081
            1 << Hmc830::LDO_Driver_Always_On |
            1 << Hmc830::gpo_select);

        // Main registers
        write(Hmc830::VCOSPI, 0);
        write(Hmc830::FrequencyInteger,    _vco.intg);
        write(Hmc830::FrequencyFractional, _vco.frac);      // R04

        _timer.delay(1000);

        _isInit = true;
        return true;
    }

    bool setFrequency(uint32_t frequency, size_t channel = 0) override
    {
        // 25 MHz … 3 GHz
        if (frequency < 25'000'000UL || frequency > 3'000'000'000UL)
            return false;

        uint8_t  div   = 1;
        uint8_t  stage = 1;

        // Output divider: VCO must be in [1.5 … 3] GHz
        if (frequency < 1'500'000'000UL)
        {
            div = 2;
            while ((uint64_t)frequency * div < 1'500'000'000ULL && div < 62)
                div += 2;

            if ((uint64_t)frequency * div > 3'000'000'000ULL)
                return false;

            stage = 0;   // divider mode
        }
        // else: fundamental, div=1, stage=1

        const uint32_t f_pfd = XtalFrequency / 40;          // 1 MHz
        const uint64_t f_vco = (uint64_t)frequency * div;

        // N = f_vco / f_pfd
        uint32_t intg = static_cast<uint32_t>(f_vco / f_pfd);
        uint32_t frac = 0;

        if (_vco.mode == Mode::Fractional)
        {
            // FRAC = round( (f_vco % f_pfd) * 2^24 / f_pfd )
            const uint64_t rem = f_vco % f_pfd;
            frac = static_cast<uint32_t>(
                (rem * FracModulus + f_pfd / 2) / f_pfd);

            if (frac >= FracModulus)   // Borrow to integer
            {
                frac = 0;
                intg++;
            }
        }

        // Range of N (fractional ≥ 20, integer ≥ 16…; up to ~524287)
        if (intg < 20 || intg > 524287)
            return false;

        _vco.div   = div;
        _vco.stage = stage;
        _vco.intg  = intg;
        _vco.frac  = frac;

        setVcoParameters();

        uint16_t timeout = Timeout;
        uint32_t status;
        do
        {
            status = read(0x10) & 0xFF;
            if (status != 0x80)
                break;
            _timer.delay(1);
        } while (timeout--);
        if (!timeout) return false;
        
        return true;
    }

    bool setPhase(uint16_t phase, size_t channel = 0) override
    {
        return false;
    }

    bool setPower(int32_t power, size_t channel = 0) override
    {
        // RF buffer gain in 3 dB steps: 0, 3, 6, 9 dB.
        if (power <= 0)      _vco.gain = 0;
        else if (power <= 3) _vco.gain = 1;
        else if (power <= 6) _vco.gain = 2;
        else                 _vco.gain = 3;

        setVcoParameters();
        return checkAutocal();
    }

    bool turn(Turn on) override
    {
        if (on == Turn::On)
        {
            write(Hmc830::VCOSPI,                               // R5 → VCO R1
                0 << Hmc830::VCO_Subsystem_ID |
                Hmc830::VCO_Enables << Hmc830::VCO_Subsystem_register_address |
                (
                    1 << Hmc830::Master_Enable_VCO_Subsystem    |
                    1 << Hmc830::Manual_Mode_PLL_buffer_enable  |
                    1 << Hmc830::Manual_Mode_RF_buffer_enable   |
                    1 << Hmc830::Manual_Mode_Divide_by_1_enable |
                    1 << Hmc830::Manual_Mode_RF_Divider_enable
                ) << Hmc830::VCO_Subsystem_data);
        }
        else
        {
            write(Hmc830::VCOSPI,                               // R5 → VCO R1
                0 << Hmc830::VCO_Subsystem_ID |
                Hmc830::VCO_Enables << Hmc830::VCO_Subsystem_register_address |
                (
                    0 << Hmc830::Master_Enable_VCO_Subsystem    |
                    0 << Hmc830::Manual_Mode_PLL_buffer_enable  |
                    0 << Hmc830::Manual_Mode_RF_buffer_enable   |
                    0 << Hmc830::Manual_Mode_Divide_by_1_enable |
                    0 << Hmc830::Manual_Mode_RF_Divider_enable
                ) << Hmc830::VCO_Subsystem_data);
        }
        return true;
    }

    void reset() override
    {
    }

    bool isInit() override
    {
        return _isInit;
    }

private:
    ISpi   &_p;
    ITimer &_timer;

    bool _isInit = false;

    struct vcoReg_s
    {
        uint32_t frac  : 24 = 0;      // 0 … 0xFFFFFF
        uint32_t div   :  6 = 1;
        uint32_t gain  :  2 = 3;
        uint32_t intg  : 19 = 2000;
        uint32_t stage :  1 = 1;
        uint32_t mode  :  1 = 1;      // 0 - Fractional, 1 - Integer
        uint32_t se    :  1 = 1;      // 1 - Single Ended, 0 - Differential Output
    } _vco;

    static constexpr uint16_t Timeout        = 1000;
    static constexpr uint32_t XtalFrequency  = 40'000'000UL;
    static constexpr uint32_t FracModulus    = 1UL << 24;   // 16777216
    static constexpr uint8_t  Read           = 0x80;

    bool checkAutocal()
    {
        uint16_t timeout = Timeout;
        uint32_t status;
        do
        {
            status = read(Hmc830::VCOTune) & 0xFF;
            if (status != Hmc830::AutoCal_Busy)
                break;
            _timer.delay(1);
        } while (timeout--);

        if (!timeout) return false;
        
        return true;
    }

    void setVcoParameters()
    {
        // VCO R2 — divider / gain / stage
        write(Hmc830::VCOSPI,
            0 << Hmc830::VCO_Subsystem_ID |
            Hmc830::VCO_Biases << Hmc830::VCO_Subsystem_register_address |
            (
                _vco.div   << Hmc830::RF_Divide_ratio               |
                _vco.gain  << Hmc830::RF_output_buffer_gain_control |
                _vco.stage << Hmc830::Divider_output_stage_gain_control
            ) << Hmc830::VCO_Subsystem_data);

        // VCO R3 — RF_N/RF_P selection and recommended RF buffer bias.
        write(Hmc830::VCOSPI,
            0 << Hmc830::VCO_Subsystem_ID |
            Hmc830::VCO_Config << Hmc830::VCO_Subsystem_register_address |
            ( 0x10 |
                _vco.se << Hmc830::RF_buffer_SE_enable |
                2U << Hmc830::RF_buffer_bias |
                2U << 5 |
                0 << Hmc830::Manual_RFO_Mode
            ) << Hmc830::VCO_Subsystem_data);

        write(Hmc830::VCOSPI, 0);   // Address subsystem reset

        write(Hmc830::FrequencyInteger,    _vco.intg);   // R03
        write(Hmc830::FrequencyFractional, _vco.frac);   // R04
    }

    uint32_t read(uint8_t reg)
    {
        _p.enable();
        uint32_t data = _p.transfer((reg << 1) | Read);
        data = (data << 8) | _p.transfer(0);
        data = (data << 8) | _p.transfer(0);
        data = (data << 8) | _p.transfer(0);
        data &= 0xFFFFFF;
        _p.disable();
        return data;
    }

    bool write(uint8_t reg, uint32_t data)
    {
        _p.enable();
        data <<= 1;
        data |= static_cast<uint32_t>(reg) << 25;
        for (int i = 24; i >= 0; i -= 8)
            _p.transfer(static_cast<uint8_t>(data >> i));
        _p.disable();
        return true;
    }
};

}
