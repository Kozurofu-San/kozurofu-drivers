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

        // ID
        uint8_t status = readByte(Si5351::DeviceStatus);    // status = 0x11

        // SYS_INIT is set while the device performs its power-up sequence.
        // Do not configure the PLL until it has cleared.
        while (status & Si5351::SYS_INIT)
        {
            _timer.delay(1);
            status = readByte(Si5351::DeviceStatus);
        }

        _isInit = writeByte(Si5351::CrystalInternalLoadCapacitance, Si5351::InternalCL8pF);

        // Disable all outputs during configuration
        _isInit &= writeByte(Si5351::OutputEnableControl, 0xFF);
        // Use register 3 for output enable; do not let the external OE pin
        // accidentally hold the output disabled.
        _isInit &= writeByte(Si5351::OEBPinEnableControl, 0xFF);

        // Power down all clock outputs
        _isInit &= writeByte(Si5351::CLKxControl + 0, Si5351::CLK_PDN);
        _isInit &= writeByte(Si5351::CLKxControl + 1, Si5351::CLK_PDN);
        _isInit &= writeByte(Si5351::CLKxControl + 2, Si5351::CLK_PDN);
        
        setupPLL(
            Si5351::PLLAParameters,
            PllFrequency / XtalFrequency,
            0,
            1);
        _isInit &= writeByte(Si5351::PLLReset, Si5351::PLLA_RST);

        // Disable spread spectrum
        _isInit &= writeByte(Si5351::SpreadSpectrumParameters, 0);

        // reset();

        _frequency[0] = 0;
        _frequency[1] = 0;
        _frequency[2] = 0;

        _power[0] = 0;
        _power[1] = 0;
        _power[2] = 0;

        return _isInit;
    }
    
    void setFrequency(size_t channel, uint32_t frequency, uint16_t phase = 0) override
    {
        if (!_isInit)
            return;

        if (channel > 2)
            return;

        if (frequency == 0)
            return;

        uint32_t rDiv = 1;
        uint32_t div = PllFrequency / frequency;

        // MS0..MS5 accept integer dividers only in the 4..900 range.
        // The R divider extends the low-frequency range by powers of two.
        while (div > 900 && rDiv < 128)
        {
            rDiv <<= 1;
            div = PllFrequency / (frequency * rDiv);
        }

        if (div < 4)
            div = 4;

        if (div > 900)
            div = 900;

        setupMultisynth(channel, div, rDiv);

        writeByte(Si5351::CLKxControl + channel, 
            // Bits 3:2 must select MultiSynth; CLK_SRC_XTAL routes the
            // crystal directly to the pin and ignores the divider settings.
            Si5351::CLK_SRC_MS |
            Si5351::CLK_SRC_PLLA |
            Si5351::CLK_IDRV_8mA |
            Si5351::MS_INT);

        // Enable selected output
        uint8_t enable = readByte(Si5351::OutputEnableControl);
        enable &= ~(1 << channel);
        writeByte(Si5351::OutputEnableControl, enable);

        _frequency[channel] = PllFrequency / (div * rDiv);
    }

    uint32_t getFrequency(size_t channel) override
    {
        if (channel > 2)
            return 0;

        return _frequency[channel];
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

        _power[channel] = power;
    }

    int32_t getPower(size_t channel) override
    {
        if (channel > 2)
            return 0;

        return _power[channel];
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

    uint32_t _frequency[3];
    int32_t _power[3];

    static constexpr uint16_t Timeout = 1000;
    static constexpr uint32_t XtalFrequency = 25'000'000UL;
    static constexpr uint32_t PllFrequency = 900'000'000UL;

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
                         uint32_t div,
                         uint32_t rDiv)
    {
        uint32_t P1 = 128 * div - 512;
        uint32_t P2 = 0;
        uint32_t P3 = 1;

        uint8_t data[8];

        data[0] = (P3 >> 8) & 0xFF;
        data[1] = P3 & 0xFF;

        data[2] = ((rDividerCode(rDiv) & 0x07) << 4) |
                  ((P1 >> 16) & 0x03);

        data[3] = (P1 >> 8) & 0xFF;
        data[4] = P1 & 0xFF;

        data[5] =
            ((P3 >> 12) & 0xF0) |
            ((P2 >> 16) & 0x0F);

        data[6] = (P2 >> 8) & 0xFF;
        data[7] = P2 & 0xFF;

        write(
            Si5351::MSxParameters + channel * Si5351::MSN,
            data,
            8);
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
