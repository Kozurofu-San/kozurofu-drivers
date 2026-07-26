#pragma once

#include "interface/Temperature.h"
#include "interface/Humidity.h"
#include "interface/Gpio.h"
#include "interface/Timer.h"

#include <cstdint>

namespace driver
{

class Dht22 : ITemperature, IHumidity
{
    public:

    struct Data
    {
        int16_t temperature;
        uint16_t humidity;
    };
    
    Dht22(IGpio &p, ITimer &timer)
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

        _isInit = true;
        return true;
    }

    bool read(uint8_t *data)
    {
        size_t timeout;
        
        // https://iarduino.ru/lib/8d409ea88290dbd0f509419ad49a982e.pdf

        // Start signal
        _p.setDir(IGpio::Direction::Output);
        _p.write(0);
        _timer.delay(2'000); // 2 ms

        // Release bus
        _p.write(1);
        _p.setDir(IGpio::Direction::Input);
        _timer.delay(40);

        // Sensor response
        timeout = Timeout;
        while (_p.read() && --timeout)
            _timer.delay(1);
        if (!timeout)
            return false;

        timeout = Timeout;
        while (!_p.read() && --timeout)
            _timer.delay(1);
        if (!timeout)
            return false;

        timeout = Timeout;
        while (_p.read() && --timeout)
            _timer.delay(1);
        if (!timeout)
            return false;

        // Read 40 bits
        for (size_t i = 0; i < 40; ++i)
        {
            // Wait for LOW (≈50 us)
            timeout = Timeout;
            while (!_p.read() && --timeout)
                _timer.delay(1);

            if (!timeout)
                return false;

            // Measure HIGH duration
            auto t = _timer.now();
            timeout = Timeout;
            while (_p.read() && --timeout)
            {
                _timer.delay(1);
            }
            if (!timeout)
                return false;

            t = _timer.now() - t;

            data[i / 8] <<= 1;
            if (t > 40) // ≈70 us -> 1, ≈26 us -> 0
                data[i / 8] |= 1;
        }

        // Checksum
        if ((uint8_t)(data[0] + data[1] + data[2] + data[3]) != data[4])
            return false;

        return true;
    }
    
    int16_t getTemperature() override
    {
        read(_data);

        uint16_t raw = (uint16_t(_data[2]) << 8) | _data[3];

        bool negative = raw & 0x8000;
        raw &= 0x7FFF;

        int16_t temperature = static_cast<int16_t>(raw);
        if (negative)
            temperature = -temperature;

        return temperature;
    }
    
    uint16_t getHumidity () override
    {
        read(_data);
        
        uint16_t raw = (uint16_t(_data[0]) << 8) | _data[1];

        raw &= 0x7FFF;

        int16_t humidity = static_cast<int16_t>(raw);
        return humidity;
    }

    bool isInit()
    {
        return _isInit;
    }
    
    private:

    IGpio &_p;
    ITimer &_timer;

    uint8_t _data[5];

    bool _isInit = false;

    static const size_t Timeout = 1000;
};
}
