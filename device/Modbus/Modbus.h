#pragma once

#include "interface/Modbus.h"

#include "interface/Uart.h"
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

class ModbusDriver : IModbus
{
    public:

    ModbusDriver(IUart &p, ITimer &timer)
        : _p(p), _timer(timer)
    {
    }

    bool init()
    {
        // Init check
        if (!_p.isInit())
        {
            return false;
        }
        if (!((_p.getSpeed() > (Speed - 10)) && (_p.getSpeed() < (Speed + 10))))
        {
            return false;
        }

        return _isInit;
    }

    bool write(uint8_t *data, uint8_t len) override
    {
        return false;
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
    static constexpr uint32_t Speed = 9'600U;      // Hz

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
