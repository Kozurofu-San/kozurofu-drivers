#pragma once

#include "Ld2410bConst.h"

#include "interface/Presence.h"
#include "interface/Uart.h"
#include "interface/Gpio.h"
#include "interface/Timer.h"

#include <cstdint>
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

class Ld2410bDriver : IPresence
{
    public:

    Ld2410bDriver(IUart &p, ITimer &timer)
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

        // Get FW version
        addHeader();
        addPayload(Ld2410b::ReadFirmwareVersion);
        addTail();
        _p.write(_buffer, _pointer);
        auto length = read();
        

        return _isInit;
    }

    bool isPresent() override
    {
        if (_present)
        {
            return _present->read();
        }
        // TODO: Implement
        return false;
    }
    
    uint16_t getRange () override
    {
        // TODO: Implement
        return 0;
    }

    bool isInit()
    {
        return _isInit;
    }
    
    private:

    IUart &_p;
    IGpio *_present;
    ITimer &_timer;

    void addHeader()
    {
        _pointer = Ld2410b::Header.size();
        for (uint8_t i = 0; i < Ld2410b::Header.size(); ++i)
        {
            _buffer[i] = Ld2410b::Header[i];
        }
    }

    bool checkHeader()
    {
        bool ret = true;
        for (uint8_t i = 0; i < Ld2410b::Header.size(); ++i)
        {
            if (_buffer[i] != Ld2410b::Header[i])
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
        for (uint8_t i = 0; i < Ld2410b::Tail.size(); ++i)
        {
            _buffer[_pointer++] = Ld2410b::Tail[i];
        }
    }

    bool checkTail(uint8_t addr)
    {
        bool ret = true;
        for (uint8_t i = 0; i < Ld2410b::Tail.size(); ++i)
        {
            if (_buffer[addr + i] != Ld2410b::Tail[i])
            {
                ret &= false;
            }
        }
        return ret;
    }

    uint8_t read()
    {
        // Read header
        _p.read(&_buffer[0], Ld2410b::Header.size());
        if (!checkHeader())
        {
            return 0;
        }

        // Payload length
        _p.read(&_buffer[Ld2410b::Header.size()], 1);

        // Tail pointer
        _pointer = _buffer[Ld2410b::Header.size()] + Ld2410b::Header.size() + 2;

        // Read payload
        _p.read(&_buffer[Ld2410b::Header.size() + 1], _buffer[Ld2410b::Header.size()] + 1);

        // Read tail
        _p.read(&_buffer[_pointer], Ld2410b::Tail.size());
        if (!checkTail(_pointer))
        {
            return 0;
        }
        return _pointer - Ld2410b::Header.size() - 2;
    }


    bool _isInit = false;

    uint8_t _buffer[48];
    uint8_t _pointer;

    static constexpr uint8_t Timeout = 10;
    static constexpr uint32_t Speed = 256'000U;      // Hz

};
}
