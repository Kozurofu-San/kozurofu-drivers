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

        // Init sequence
        do
        {
            status = readByte(Si5351::DeviceStatus);
        } while (!(status & Si5351::SYS_INIT));     // status = 0xF8

        writeByte(Si5351::CrystalInternalLoadCapacitance, Si5351::InternalCL8pF);

        // Disable all outputs during configuration
        writeByte(Si5351::OutputEnableControl, 0xFF);

        // Power down all clock outputs
        writeByte(Si5351::CLKxControl + 0, Si5351::CLK_PDN);
        writeByte(Si5351::CLKxControl + 1, Si5351::CLK_PDN);
        writeByte(Si5351::CLKxControl + 2, Si5351::CLK_PDN);
        
        // Configure PLLA to 900 MHz
        setupPLL(
            Si5351::PLLAParameters,
            36,     // a
            0,      // b
            1);     // c

        // Disable spread spectrum
        writeByte(Si5351::SpreadSpectrumParameters, 0);

        _frequency[0] = 0;
        _frequency[1] = 0;
        _frequency[2] = 0;

        _power[0] = 0;
        _power[1] = 0;
        _power[2] = 0;


        _isInit = true;
        return true;
    }
    
    void setFrequency(size_t channel, uint32_t frequency, uint16_t phase = 0) override
    {
        if (!_isInit)
            return;

        if (channel > 2)
            return;

        if (frequency == 0)
            return;

        constexpr uint32_t PLL_FREQ = 900000000UL;

        uint32_t div = PLL_FREQ / frequency;

        if (div < 4)
            div = 4;

        if (div > 900)
            div = 900;

        setupMultisynth(channel, div);

        writeByte(Si5351::CLKxControl + channel, 
            Si5351::CLK_SRC_XTAL |
            Si5351::CLK_SRC_PLLA |
            Si5351::CLK_IDRV_8mA |
            Si5351::MS_INT);

        // Enable selected output
        uint8_t enable = readByte(Si5351::OutputEnableControl);
        enable &= ~(1 << channel);
        writeByte(Si5351::OutputEnableControl, enable);

        _frequency[channel] = PLL_FREQ / div;
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

    //---------------------------------------------------------------------
    // PLL configuration
    //---------------------------------------------------------------------

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
                         uint32_t div)
    {
        uint32_t P1 = 128 * div - 512;
        uint32_t P2 = 0;
        uint32_t P3 = 1;

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

        write(
            Si5351::MSxParameters + channel * Si5351::MSN,
            data,
            8);
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

    void read(uint8_t reg, uint8_t *data, size_t len)
    {
        _p.start();
        _p.address(II2c::Cmd::Write);
        _p.write(reg);
        _p.start();
        _p.address(II2c::Cmd::Read);
        while (len--)
        {
            *data++ = _p.read(len == 1);
        }
        _p.stop();
    }

    void writeByte(uint8_t reg, uint8_t data)
    {
        _p.start();
        _p.address(II2c::Cmd::Write);
        _p.write(reg);
        _p.write(data);
        _p.stop();
    }

    void write(uint8_t reg, uint8_t *data, size_t len)
    {
        _p.start();
        _p.address(II2c::Cmd::Write);
        _p.write(reg);
        while (len--)
        {
            _p.write(*data++);
        }
        _p.stop();
    }
};

}
