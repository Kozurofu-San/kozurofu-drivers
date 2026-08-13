#pragma once

#include "Aht20Const.h"

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

class Aht20Driver : ITemperature, IHumidity
{
    public:

    struct Data
    {
        int16_t temperature;
        uint16_t humidity;
    };
    
    Aht20Driver(II2c &p, ITimer &timer)
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

        // Wait for 40 ms after power-on
        _timer.delay(40);

        // Soft reset
        _p.start();
        _p.address(II2c::Write);
        _p.write(Aht20::SoftReset);
        _p.stop();

        // Soft reset pause
        _timer.delay(20);

        // Check if calibration is ready
        if (getStatus() & Aht20::CalEnable)
        {
            _isInit = true;
        }

        // Initialization
        _p.start();
        _p.address(II2c::Write);
        _p.write(Aht20::Initialization);
        _p.write(0x08);
        _p.write(0x00);
        _p.stop();


        return _isInit;
    }

    int16_t getTemperature() override
    {
        read(_data);

        // Parse raw values from the buffer (20-bit values spread across bytes)
        // Buffer layout: [0]=Status, [1][2][half of 3]=Humidity, [half of 3][4][5]=Temperature, [6]=CRC
        uint32_t raw =
            ((uint32_t)(_data[3] & 0x0F) << 16) |
            ((uint32_t)_data[4] << 8) |
            _data[5];

        // Convert to temperature in tenths of a degree Celsius.
        // T = raw * 200 / 1048576 - 50
        // Multiplying by 10 gives tenths of a degree.
        int32_t temperature = ((raw * 2000UL) >> 20) - 500;

        return temperature;
    }
    
    uint16_t getHumidity () override
    {
        read(_data);
        
        // Parse raw values from the buffer (20-bit values spread across bytes)
        // Buffer layout: [0]=Status, [1][2][half of 3]=Humidity, [half of 3][4][5]=Temperature, [6]=CRC
        uint32_t raw =
            ((uint32_t)_data[1] << 12) |
            ((uint32_t)_data[2] << 4) |
            ((_data[3] >> 4) & 0x0F);

        // Convert to tenths of a percent.
        // RH = raw * 100 / 1048576
        // Multiplying by 10 gives tenths of a percent.
        int32_t humidity = ((raw * 1000UL) >> 20);

        return humidity;
    }

    bool isInit()
    {
        return _isInit;
    }
    
    private:

    II2c &_p;
    ITimer &_timer;

    uint8_t _data[7];

    bool _isInit = false;

    static const size_t Timeout = 10;

    uint8_t getStatus()
    {
        _p.start();
        _p.address(II2c::Read);
        uint8_t status = _p.read(true);
        _p.stop();
        return status;
    }
    
    bool read(uint8_t *data)
    {
        size_t timeout;

        // Start measurement
        _p.start();
        _p.address(II2c::Write);
        _p.write(Aht20::TriggerMeasurement);
        _p.write(0x33);
        _p.write(0x00);
        _p.stop();

        // Wait for 75 ms
        _timer.delay(75);

        // Check if busy
        timeout = Timeout;
        do
        {
            if (!(getStatus() & Aht20::Busy))
            {
                break;
            }
        } while (-- timeout);

        if (!timeout)
        {
            return false;
        }

        // Read data
        _p.start();
        _p.address(II2c::Read);
        uint8_t cnt = 7;
        while (cnt--)
        {
            *data++ = _p.read(!cnt);
        }
        _p.stop();
        

        
        return true;
    }
    
};
}
