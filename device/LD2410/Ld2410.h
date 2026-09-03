#pragma once

#include "Ld2410Const.h"

#include "interface/Presence.h"
#include "interface/Uart.h"
#include "interface/Gpio.h"
#include "interface/Timer.h"

#include <cstdint>
#include <cstdio>
#include <span>

/* // Temperature / Humidity sensor

#include "device/AHT20/Aht20.h"

    I2cController i2c2 {I2C2};
    I2cDriver i2c_tempHum {i2c2};
    TimerDriver timer_ms {TIM3};
    Aht20Driver tempHum {i2c_tempHum, timer_ms};
    
    // I2C
    GpioDriver::remap(AFIO_MAPR_I2C1_REMAP, false);
    GpioDriver::mode(GPIOB, 11, GpioDriver::Mode::AlternateOpendrain);   // SDA
    GpioDriver::mode(GPIOB, 10, GpioDriver::Mode::AlternateOpendrain);   // SCL
    CHECK(p.i2c2.init(400'000));

    // Timer
    p.timer_ms.init({1, ITimer::Units::ms});
    p.timer_ms.start();

    // Temperature / Humidity sensor
    p.i2c2.check(II2c::Address::AHT20);
    p.i2c_tempHum.init(II2c::Address::AHT20);
    p.tempHum.init();
*/

namespace driver
{

class Ld2410Driver : IPresence
{
    public:

    Ld2410Driver(IUart &p, ITimer &timer)
        : _p(p), _timer(timer)
    {
    }

    bool init(IGpio *present = nullptr)
    {
        _present = present;

        // Init check
        if (!_p.isInit())
        {
            return false;
        }
        if (!((_p.getSpeed() > (Speed - 300)) && (_p.getSpeed() < (Speed + 300))))
        {
            return false;
        }

        uint8_t length;
        bool ret = true;

        ret &= cmd(Ld2410::Restart) > 0;
        _timer.delay(1000);
        ret &= cmd(Ld2410::EnableConfig) > 0;
        length = cmd(Ld2410::ReadFirmwareVersion);
        printf("LD2410 FW v");
        for (uint8_t i = 0; i < length - 6; ++i)
        {
            printf("%02X", _buffer[12 + i]);
        }
        printf("\n");
        length = cmd(Ld2410::GetMacAddress);
        printf("LD2410 MAC ");
        for (uint8_t i = 0; i < length - 4; ++i)
        {
            printf("%02X:", _buffer[10 + i]);
        }
        printf("\n");
        
        ret &= cmd(Ld2410::EnableEngineeringMode) > 0;
        ret &= cmd(Ld2410::EndConfig) > 0;

        _isInit = ret;

        return _isInit;
    }

    bool isPresent() override
    {
        if (_present)
        {
            return _present->read();
        }

        uint8_t len = readReport();
        if (!len)
        {
            return 0;
        }

        // TODO: Implement
        return _buffer[8] != 0x00;
    }
    
    uint16_t getRange() override
    {
        // Get data
        uint8_t len = readReport();
        if (!len)
        {
            return 0;
        }

        // Parse
        uint16_t range = 0;     // cm
        if (_buffer[6] == 0x01 && _buffer[38] == 0x01)         // Engineering mode
        {
            // Processed range
            range = _buffer[15];
            range |= _buffer[16] << 8;
        }
        else if (_buffer[6] == 0x02)    // Normal mode
        {
            range = _buffer[16];
        }

        return range;
    }

    bool isInit()
    {
        return _isInit;
    }
    
    private:

    IUart &_p;
    IGpio *_present;
    ITimer &_timer;

    bool _isInit = false;

    uint8_t _buffer[48];
    uint8_t _pointer;

    static constexpr uint8_t Timeout = 10;
    static constexpr uint32_t Speed = 256'000U;      // Hz

    void addHeader()
    {
        _pointer = Ld2410::Header.size();
        for (uint8_t i = 0; i < Ld2410::Header.size(); ++i)
        {
            _buffer[i] = Ld2410::Header[i];
        }
    }

    bool check(std::span<const uint8_t> cmd, uint8_t address = 0)
    {
        bool ret = true;
        for (uint8_t i = 0; i < cmd.size(); ++i)
        {
            if (_buffer[address + i] != cmd[i])
            {
                ret &= false;
            }
        }
        return ret;
    }

    void addPayload(std::span<const uint8_t> cmd)
    {
        _buffer[_pointer++] = cmd.size();
        _buffer[_pointer++] = 0x00;
        for (uint8_t i = 0; i < cmd.size(); ++i)
        {
            _buffer[_pointer++] = cmd[i];
        }
    }

    void addTail()
    {
        for (uint8_t i = 0; i < Ld2410::Tail.size(); ++i)
        {
            _buffer[_pointer++] = Ld2410::Tail[i];
        }
    }

    uint8_t read()
    {
        // Read header
        _p.read(&_buffer[0], Ld2410::Header.size());
        if (!check(Ld2410::Header))
        {
            return 0;
        }

        // Payload length
        _p.read(&_buffer[Ld2410::Header.size()], 1);

        // Tail pointer
        _pointer = _buffer[Ld2410::Header.size()] + Ld2410::Header.size() + 2;

        // Read payload
        _p.read(&_buffer[Ld2410::Header.size() + 1], _buffer[Ld2410::Header.size()] + 1);

        // Read tail
        _p.read(&_buffer[_pointer], Ld2410::Tail.size());
        if (!check(Ld2410::Tail, _pointer))
        {
            return 0;
        }

        // Read ACK
        if (_buffer[8] == 0x01)
        {
            return 0;   // NACK is received
        }

        return _buffer[Ld2410::Header.size()];
    }

    uint8_t readReport()
    {
        // Find and read header
        for (uint8_t i = 0; i < Ld2410::HeaderReport.size(); ++i)
        {
            do
            {
                _p.read(&_buffer[i], 1);
            } while (_buffer[i] != Ld2410::HeaderReport[i]);
        }

        // Payload length
        _p.read(&_buffer[Ld2410::HeaderReport.size()], 1);

        // Tail pointer
        _pointer = _buffer[Ld2410::HeaderReport.size()] + Ld2410::Header.size() + 2;

        // Read payload
        _p.read(&_buffer[Ld2410::Header.size() + 1], _buffer[Ld2410::Header.size()] + 1);

        // Read tail
        _p.read(&_buffer[_pointer], Ld2410::TailReport.size());
        if (!check(Ld2410::TailReport, _pointer))
        {
            return 0;
        }

        // Read ACK
        if (_buffer[8] == 0x01)
        {
            return 0;   // NACK is received
        }

        return _buffer[Ld2410::HeaderReport.size()];
    }

    uint8_t cmd(std::span<const uint8_t> cmd)
    {
        addHeader();
        addPayload(cmd);
        addTail();
        _p.write(_buffer, _pointer);
        return read();
    }


};
}
