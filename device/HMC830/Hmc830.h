#pragma once

#include "Hmc830Const.h"

#include "interface/Generator.h"
#include "interface/Timer.h"
#include "interface/Spi.h"

/* // Clock generator

#include "device/HMC830/Hmc830.h"

    I2cController i2c2 {I2C2};
    I2cDriver i2c_clock {i2c2};
    Si5351Driver clock {i2c_clock, timer_ms};

    // I2C
    GpioDriver::remap(AFIO_MAPR_I2C1_REMAP, false);
    GpioDriver::mode(GPIOB, 11, GpioDriver::Mode::AlternateOpendrain);   // SDA
    GpioDriver::mode(GPIOB, 10, GpioDriver::Mode::AlternateOpendrain);   // SCL
    CHECK(p.i2c2.init(400'000));

    // Timer
    CHECK(p.timer_ms.init({1, ITimer::Units::ms}));
    p.timer_ms.start();

    // Clock generator
    CHECK(p.i2c2.check(II2c::Address::SI5351));
    CHECK(p.i2c_clock.init(II2c::Address::SI5351));
    CHECK(p.clock.init());

    // Example
    p.clock.setFrequency(0, 1'000'000);
    p.clock.setFrequency(1, 1'000'000);
    p.clock.setPhase(0, 0);
    p.clock.setPhase(1, 30);
    p.clock.reset();
    p.clock.setPower(0, 2);
    p.clock.setPower(0, 8);
*/

namespace driver
{

class Hmc830Driver: public IGenerator
{
    public:

    Hmc830Driver(ISpi &p, ITimer &timer)
        : _p(p), _timer(timer) {}

    bool init()
    {
        
        // HMC mode: rise SEN -> rise CLK
        // Open mode: rise CLK -> rise SEN
        _timer.delay(100);
        _p.enable();
        _timer.delay(100);
        _p.disable();

        uint32_t id = read(Hmc830::ID);

        write(Hmc830::OpenMode, 1 << Hmc830::SoftReset);
        write(Hmc830::REFDIV, 40);
        write(Hmc830::SDCFG, 
            1 << Hmc830::frac_bypass |
            1 << Hmc830::AutoSeed |
            1 << Hmc830::clkrq_refdiv_sel |
            0 << Hmc830::SD_Enable);
        write(Hmc830::LockDetect, 
            1 << Hmc830::Enable_Internal_Lock_Detect |
            1 << Hmc830::Lock_Detect_Window_type |
            1 << Hmc830::LD_Digital_Window_duration |
            3 << Hmc830::LD_Digital_Timer_Freq_Control |
            0 << Hmc830::LD_Timer_Test_Mode |
            0 << Hmc830::Auto_Relock_One_Try);
        write(Hmc830::AnalogEn, 0xC1BEFF);
        write(Hmc830::ChargePump, 
            (1000U / 20) << Hmc830::CP_DN_Gain |
            (1000U / 20) << Hmc830::CP_UP_Gain |
            (100U / 5)   << Hmc830::Offset_Magnitude);
        write(Hmc830::ChargePump, 
            (1000U / 20) << Hmc830::CP_DN_Gain |
            (1000U / 20) << Hmc830::CP_UP_Gain |
            (100U / 5)   << Hmc830::Offset_Magnitude);

        _isInit = true;
        return true;
    }
    
    bool setFrequency(uint32_t frequency, size_t channel = 0) override
    {
        uint32_t intg;
        uint8_t div = 0;
        uint8_t stage = 0;

        // Divider calculation
        if ((frequency >= 25) & (frequency < 1500))
        {
            while (frequency * div < 1500)
            {
                div += 2;
            }
        }
        else if ((frequency >= 1500) & (frequency < 3000))
        {
            div = 1;
        }
        else return false;
        
        if (div <= 2)
        {
            stage = 1;
        }

        intg = frequency * div / (XtalFrequency / 40);  // If FPD = 1 MHz
        _vco.div = div;
        _vco.stage = stage;
        _vco.intg = intg;

        return true;
    }

    bool setPhase(uint16_t phase, size_t channel = 0) override
    {
        return false;
    }

    bool setPower(int32_t power, size_t channel = 0) override
    {
        return false;
    }

    bool turn(Turn on) override
    {
        return false;
    }

    void reset() override
    {
    }

    bool isInit() override
    {
        return _isInit;
    }

    private:

    ISpi &_p;
    ITimer& _timer;

    bool _isInit = false;

    // VCO variables
    struct vcoReg_s{
        uint32_t intg: 19 = 2000;
        uint32_t div: 6 = 1;
        uint32_t gain: 2 = 0;
        uint32_t stage: 1 = 1;
    } _vco;

    static constexpr uint16_t Timeout = 1000;
    static constexpr uint32_t XtalFrequency = 40'000'000UL;
    static constexpr uint8_t Read = 0x80;


    uint32_t read(uint8_t reg)
    {
        _p.enable();
        uint8_t data = _p.transfer((reg << 1)|  Read);
        data <<= 8;
        data |= _p.transfer(0);
        data <<= 8;
        data |= _p.transfer(0);
        data <<= 8;
        data |= _p.transfer(0);
        data &= 0xFFFFFF;

        _p.disable();
        return data;
    }

    bool write(uint8_t reg, uint32_t data)
    {
        bool ret;
        _p.enable();
        data <<= 1;
        data |= reg << 25;
        for (size_t i = 24; i >= 0; i -= 8)
        {
            _p.transfer(data >> i);
        }
        _p.disable();
        return ret;
    }
};

}
