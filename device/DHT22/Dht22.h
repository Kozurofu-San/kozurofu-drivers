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

    int16_t getTemperature () override
    {
        uint64_t ret = 0;

        // Send request
        _p.write(1);
        _p.write(0);
        _timer.delay(5000);
        _p.write(1);

        // Wait for data is ready
        _p.setDir(IGpio::Direction::Input);
        while(_p.read() == 1);
        while(_p.read() == 0);
        _timer.delay(50);

        // Get data

        return ret;
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
