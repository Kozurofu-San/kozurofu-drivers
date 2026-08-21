#pragma once

#include "Ld2410bConst.h"

#include "interface/Presence.h"
#include "interface/Uart.h"
#include "interface/Gpio.h"
#include "interface/Timer.h"

#include <cstdint>

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

        uint32_t *ptr = reinterpret_cast<uint32_t*>(_buffer);
        ptr[0] = Ld2410b::Header;
        ptr[1] = 0x0200A000;
        ptr[2] = Ld2410b::Tail;

        _p.write(_buffer, 12);
        _p.read(_buffer, 22);
        

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

    bool _isInit = false;
    uint8_t _buffer[48];
    static constexpr uint8_t Timeout = 10;
    static constexpr uint32_t Speed = 256'000U;      // Hz

};
}
