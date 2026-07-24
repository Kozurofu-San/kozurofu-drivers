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
int16_t getTemperature() override
{
    uint8_t data[5] = {};
    size_t timeout;

    // Start signal
    _p.setDir(IGpio::Direction::Output);
    _p.write(0);
    _timer.delay(2'000);     // 2 ms
    _p.write(1);

    // Release bus
    _p.setDir(IGpio::Direction::Input);

    // Sensor response
    timeout = 100;
    while (_p.read() && --timeout)
        _timer.delay(1);
    if (!timeout)
        return -1;

    timeout = 100;
    while (!_p.read() && --timeout)
        _timer.delay(1);
    if (!timeout)
        return -1;

    timeout = 100;
    while (_p.read() && --timeout)
        _timer.delay(1);
    if (!timeout)
        return -1;

    // Read 40 bits
    for (size_t i = 0; i < 40; ++i)
    {
        // Wait for LOW (≈50 us)
        timeout = 100;
        while (!_p.read() && --timeout)
            _timer.delay(1);

        if (!timeout)
            return -1;

        // Measure HIGH duration
        size_t t = 0;
        while (_p.read())
        {
            _timer.delay(1);
            if (++t > 100)
                return -1;
        }

        data[i / 8] <<= 1;
        if (t > 40)          // ≈70 us -> 1, ≈26 us -> 0
            data[i / 8] |= 1;
    }

    // Checksum
    if ((uint8_t)(data[0] + data[1] + data[2] + data[3]) != data[4])
        return -1;

    uint16_t raw = (uint16_t(data[2]) << 8) | data[3];

    bool negative = raw & 0x8000;
    raw &= 0x7FFF;

    int16_t temperature = static_cast<int16_t>(raw);
    if (negative)
        temperature = -temperature;

    return temperature;
}
    uint16_t getHumidity () override
    {
        return 0;
    }

    bool isInit()
    {
        return _isInit;
    }
    
    private:

    IGpio &_p;
    ITimer &_timer;

    bool _isInit = false;
};
}
