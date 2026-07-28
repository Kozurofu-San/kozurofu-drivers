#pragma once

#include "Si5351Const.h"

#include "interface/Generator.h"
#include "interface/Timer.h"
#include "interface/I2c.h"

namespace driver
{

class Si5351Driver: public IGenerator
{
    public:

    Si5351Driver(II2c &p, ITimer &timer)
        : _p(p), _timer(timer) {}

    bool init()
    {
        // Init check
        if (!_p.isInit() || !_timer.isInit())
        {
            return false;
        }

        // Wait for the chip's internal initialization to complete
        uint8_t status = readByte(Si5351::DeviceStatus);
        while (status & Si5351::SYS_INIT)
        {
            _timer.delay(1);
            status = readByte(Si5351::DeviceStatus);
        }

        _isInit = writeByte(Si5351::CrystalInternalLoadCapacitance, Si5351::InternalCL8pF);

        // Disable all outputs while configuring
        _isInit &= writeByte(Si5351::OutputEnableControl, 0xFF);
        _isInit &= writeByte(Si5351::OEBPinEnableControl, 0xFF);

        // Power down all outputs
        _isInit &= writeByte(Si5351::CLKxControl + 0, Si5351::CLK_PDN);
        _isInit &= writeByte(Si5351::CLKxControl + 1, Si5351::CLK_PDN);
        _isInit &= writeByte(Si5351::CLKxControl + 2, Si5351::CLK_PDN);

        // Initial VCO = 900 MHz (integer)
        // a = 900000000 / 25000000 = 36
        _pllFrequency = 900000000UL;
        setupPLL(Si5351::PLLAParameters, 36, 0, 1);
        _isInit &= writeByte(Si5351::PLLReset, Si5351::PLLA_RST);

        // Spread spectrum disabled
        _isInit &= writeByte(Si5351::SpreadSpectrumParameters, 0);

        // Reset the stored dividers
        for (size_t i = 0; i < 3; ++i)
        {
            _msDiv[i] = 0;
            _rDiv[i]  = 1;
        }

        return _isInit;
    }
    
    void setFrequency(size_t channel, uint32_t frequency) override
    {
        if (!_isInit)
            return;

        if (channel > 2)
            return;

        // Range 8 kHz … 160 MHz
        if (frequency < 8000UL)
            frequency = 8000UL;
        if (frequency > 160000000UL)
            frequency = 160000000UL;

        const uint32_t xtal = XtalFrequency;
        uint32_t rDiv = 1;
        uint32_t a = 0, b = 0, c = 1;
        bool divBy4 = false;
        bool pllChanged = false;

        //--------------------------------------------------------------
        // > 150 MHz → mode DIVBY4 (VCO = 4 × fout)
        //--------------------------------------------------------------
        if (frequency > 150000000UL)
        {
            uint32_t vco = frequency * 4UL;          // 600…640 MHz

            uint32_t pll_a = vco / xtal;
            uint32_t pll_b = 0;
            uint32_t pll_c = 1;

            setupPLL(Si5351::PLLAParameters, pll_a, pll_b, pll_c);
            writeByte(Si5351::PLLReset, Si5351::PLLA_RST);

            _pllFrequency = vco;
            pllChanged = true;

            a = 4;
            rDiv = 1;
            divBy4 = true;
        }
        //--------------------------------------------------------------
        // ≤ 150 MHz
        //--------------------------------------------------------------
        else
        {
            // Max R, when MS ≥ 8
            // (minimal MS → maximal phase range)
            rDiv = 128;
            uint64_t ms = 0;

            while (rDiv >= 1)
            {
                ms = (uint64_t)_pllFrequency / ((uint64_t)frequency * rDiv);
                if (ms >= 8)
                    break;
                rDiv >>= 1;
            }

            if (rDiv == 0)
                rDiv = 1;

            // Too little divider → reconfigure PLL to lower frequency
            if (ms < 8)
            {
                uint32_t targetMs = 8;
                if ((uint64_t)frequency * 8 > 900000000ULL)
                    targetMs = 6;
                if ((uint64_t)frequency * 6 > 900000000ULL)
                    targetMs = 4;

                uint32_t vco = frequency * targetMs;
                if (vco < 600000000UL) vco = 600000000UL;
                if (vco > 900000000UL) vco = 900000000UL;

                uint32_t pll_a = vco / xtal;
                uint32_t pll_b = 0;
                uint32_t pll_c = 1;

                setupPLL(Si5351::PLLAParameters, pll_a, pll_b, pll_c);
                writeByte(Si5351::PLLReset, Si5351::PLLA_RST);

                _pllFrequency = vco;
                pllChanged = true;

                a = targetMs;
                b = 0;
                c = 1;
                rDiv = 1;

                if (targetMs == 4)
                    divBy4 = true;
            }
            else
            {
                // Fixed point calculation of a, b, c for fractional multisynth
                uint64_t num = _pllFrequency;
                uint64_t den = (uint64_t)frequency * rDiv;

                a = static_cast<uint32_t>(num / den);
                uint64_t rem = num % den;

                c = 1000000UL;                       // A good compromise between precision and register size
                // c = 1048575UL;                    // Maximal precision (20 bits) but may cause overflow in b calculation

                // b = round(rem * c / den)
                b = static_cast<uint32_t>((rem * c + den / 2) / den);

                if (b >= c)
                {
                    a += 1;
                    b = 0;
                }
            }
        }

        // Save for setPhase()
        _msDiv[channel] = a;
        _rDiv[channel]  = rDiv;

        // Write MultiSynth
        setupMultisynth(channel, a, b, c, rDiv, divBy4);

        // Control output: source = PLLA, drive strength = 8 mA, MSx_DIVBY4 if needed
        uint8_t ctrl = Si5351::CLK_SRC_MS |
                    Si5351::CLK_SRC_PLLA |
                    Si5351::CLK_IDRV_8mA;

        if (divBy4)
            ctrl |= Si5351::MS_INT;

        writeByte(Si5351::CLKxControl + channel, ctrl);

        // Turn on the output
        uint8_t enable = readByte(Si5351::OutputEnableControl);
        enable &= ~(1 << channel);
        writeByte(Si5351::OutputEnableControl, enable);

        if (pllChanged)
            writeByte(Si5351::PLLReset, Si5351::PLLA_RST);
    }

    void setPhase(size_t channel, uint16_t phase) override
    {
        if (!_isInit || channel > 2 || _msDiv[channel] == 0)
            return;

        uint32_t degrees = phase % 360;

        // ----- Специальный случай 180° — используем аппаратную инверсию -----
        uint8_t ctrl = readByte(Si5351::CLKxControl + channel);

        if (degrees == 180)
        {
            ctrl |= Si5351::CLK_INV;                 // 180°
            writeByte(Si5351::CLKxControl + channel, ctrl);

            // Фазовый регистр обнуляем
            writeByte(Si5351::CLKxPhaseOffset + channel, 0);
            return;
        }
        else
        {
            ctrl &= ~Si5351::CLK_INV;                // обычный режим
            writeByte(Si5351::CLKxControl + channel, ctrl);
        }

        // ----- Обычный фазовый сдвиг через регистр 165+ -----
        uint32_t ms_r = _msDiv[channel] * _rDiv[channel];
        uint32_t ph   = (degrees * ms_r + 45) / 90;   // округление

        if (ph > 127)
            ph = 127;

        writeByte(Si5351::CLKxPhaseOffset + channel, static_cast<uint8_t>(ph));
    }

    void setPower(size_t channel, int32_t power) override
    {
        if (channel > 2)
            return;

        uint8_t reg = readByte(Si5351::CLKxControl + channel);

        reg &= ~Si5351::CLK_IDRV_MASK;

        switch (power)
        {
        default:
        case 2:
            reg |= Si5351::CLK_IDRV_2mA;
            break;

        case 4:
            reg |= Si5351::CLK_IDRV_4mA;
            break;

        case 6:
            reg |= Si5351::CLK_IDRV_6mA;
            break;

        case 8:
            reg |= Si5351::CLK_IDRV_8mA;
            break;
        }

        writeByte(Si5351::CLKxControl + channel, reg);
    }

    void reset() override
    {
        // Reset PLLA and PLLB
        writeByte(Si5351::PLLReset,
              Si5351::PLLA_RST |
              Si5351::PLLB_RST);
    }

    bool isInit() override
    {
        return _isInit;
    }

    private:

    II2c &_p;
    ITimer& _timer;

    bool _isInit = false;

    uint32_t _pllFrequency = 900'000'000UL;
    uint32_t _msDiv[3] = {};
    uint32_t _rDiv[3] = {1, 1, 1};

    static constexpr uint16_t Timeout = 1000;
    static constexpr uint32_t XtalFrequency = 25'000'000UL;

    //---------------------------------------------------------------------
    // PLL configuration
    //---------------------------------------------------------------------

    // f_vco = f_in * (a + b / c)
    void setupPLL(uint8_t reg,
                  uint32_t a,
                  uint32_t b,
                  uint32_t c)
    {
        uint32_t P1 = 128 * a + (128 * b) / c - 512;
        uint32_t P2 = 128 * b - c * ((128 * b) / c);
        uint32_t P3 = c;

        uint8_t data[8];

        data[0] = (P3 >> 8) & 0xFF;
        data[1] = P3 & 0xFF;

        data[2] = (P1 >> 16) & 0x03;

        data[3] = (P1 >> 8) & 0xFF;
        data[4] = P1 & 0xFF;

        data[5] =
            ((P3 >> 12) & 0xF0) |
            ((P2 >> 16) & 0x0F);

        data[6] = (P2 >> 8) & 0xFF;
        data[7] = P2 & 0xFF;

        write(reg, data, 8);
    }

    //---------------------------------------------------------------------
    // Integer multisynth configuration
    //---------------------------------------------------------------------

    void setupMultisynth(uint8_t channel,
                         uint32_t a, uint32_t b, uint32_t c,
                         uint32_t rDiv,
                         bool divBy4 = false)
    {
        uint32_t P1, P2, P3;

        if (divBy4)
        {
            // Special mode 150…200 MHz (AN619 §4.1.3)
            P1 = 0;
            P2 = 0;
            P3 = 1;
        }
        else
        {
            P1 = 128 * a + (128 * b) / c - 512;
            P2 = 128 * b - c * ((128 * b) / c);
            P3 = c;
        }

        uint8_t data[8];

        data[0] = (P3 >> 8) & 0xFF;
        data[1] =  P3       & 0xFF;

        // data[2]:
        //   bits 6:4 — R divider code
        //   bits 3:2 — MSx_DIVBY4 (11b for DIVBY4)
        //   bits 1:0 — P1[17:16]
        uint8_t rCode = rDividerCode(rDiv) & 0x07;
        uint8_t divBy4Bits = divBy4 ? 0x0C : 0x00;   // 0b11 << 2

        data[2] = (rCode << 4) | divBy4Bits | ((P1 >> 16) & 0x03);

        data[3] = (P1 >> 8) & 0xFF;
        data[4] =  P1       & 0xFF;

        data[5] = ((P3 >> 12) & 0xF0) | ((P2 >> 16) & 0x0F);
        data[6] = (P2 >> 8) & 0xFF;
        data[7] =  P2       & 0xFF;

        write(Si5351::MSxParameters + channel * Si5351::MSN, data, 8);
    }

    static uint8_t rDividerCode(uint32_t rDiv)
    {
        uint8_t code = 0;
        while (rDiv > 1)
        {
            rDiv >>= 1;
            ++code;
        }
        return code;
    }

    //---------------------------------------------------------------------
    // Low-level I2C helpers
    //---------------------------------------------------------------------

    uint8_t readByte(uint8_t reg)
    {
        _p.start();
        _p.address(II2c::Cmd::Write);
        _p.write(reg);
        _p.start();
        _p.address(II2c::Cmd::Read);
        uint8_t ret = _p.read(true);
        _p.stop();
        return ret;
    }

    bool read(uint8_t reg, uint8_t *data, size_t len)
    {
        bool ret;
        _p.start();
        ret = _p.address(II2c::Cmd::Write);
        _p.write(reg);
        _p.start();
        ret &= _p.address(II2c::Cmd::Read);
        while (len--)
        {
            *data++ = _p.read(len == 1);
        }
        _p.stop();
        return ret;
    }

    bool writeByte(uint8_t reg, uint8_t data)
    {
        bool ret;
        _p.start();
        ret = _p.address(II2c::Cmd::Write);
        _p.write(reg);
        _p.write(data);
        _p.stop();
        return ret;
    }

    bool write(uint8_t reg, uint8_t *data, size_t len)
    {
        bool ret;
        _p.start();
        ret = _p.address(II2c::Cmd::Write);
        _p.write(reg);
        while (len--)
        {
            _p.write(*data++);
        }
        _p.stop();
        return ret;
    }
};

}
