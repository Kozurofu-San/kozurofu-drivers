#pragma once

#include "interface/Temperature.h"
#include "interface/Humidity.h"
#include "interface/Gpio.h"
#include "interface/Timer.h"

#include <cstdint>

/* // Temperature / Humidity sensor

#include "device/DHT22/Dht22.h"

    GpioDriver gpio_tempHum {GPIOB, 8};
    TimerDriver timer_us {TIM3};
    Dht22 tempHum {gpio_tempHum, timer_us};
    
    // Timer
    p.timer_us.init({1, ITimer::Units::us});
    p.timer_us.start();

    // Temperature / Humidity sensor
    p.gpio_tempHum.init(GpioDriver::Mode::OutputPushpull, GpioDriver::Speed::High);
    p.gpio_tempHum.write(1);    // Sleep
    p.tempHum.init();
*/

namespace driver
{

class Aht22 : ITemperature, IHumidity
{
    public:

    struct Data
    {
        int16_t temperature;
        uint16_t humidity;
    };
    
    Aht22(II2c &p)
        : _p(p)
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

    II2c &_p;
    ITimer &_timer;

    uint8_t _data[5];

    bool _isInit = false;

    static const size_t Timeout = 1000;
    
    bool read(uint8_t *data)
    {
        size_t timeout;
        
        return true;
    }
    
};
}
