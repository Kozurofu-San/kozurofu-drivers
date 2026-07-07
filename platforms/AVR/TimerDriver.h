#pragma once

#include "interface/Timer.h"
#include "interface/Gpio.h"

#include <avr/io.h>

#undef REG
#define REG(p, bias) (*(volatile uint16_t *)((uint16_t)p + (uint16_t)& bias - (uint16_t)& TACTL))

namespace driver
{
    
class TimerDriver : public ITimer
{
    public:

    enum class P: uint16_t
    {
        TimerA0 = 0x0160,
        TimerA1 = 0x0180,
    };

    enum class Mode
    {
        Normal = 0,
        Pwm = 1,
    };

    TimerDriver(P timer, uint32_t frequency)
        : _timer(timer), _frequency(frequency)
    {
    }

    bool init(Mode mode)
    {
        return true;
    }
    
    void start() override
    {
        
    }
    
    void stop() override
    {
        
    }
    
    void reset() override
    {
        
    }
    
    void delay(uint32_t ms) override
    {
        reset();
        start();
        while (_ms < ms) 
        { 
            // __low_power_mode_0();
        }
        stop();
    }

    void pwm(uint16_t pwm)
    {
        
    }

    void callback(void cb(uint32_t))
    {
        _cb = cb;
    }

    uint32_t getSpeed() override
    {
        return _speed;
    }

    bool isInit() override
    {
        return _isInit;
    }

    inline void load(uint32_t ms) { _ms = ms; }
    inline uint32_t now() { return _ms; }
    
    private:

    P _timer;
    uint32_t _frequency;
    uint32_t _ms = 0;

    IGpio *_gpioPwm = nullptr;
    uint32_t _speed = 0;

    bool _isInit = false;

    void (*_cb)(uint32_t) = nullptr;
};
}
