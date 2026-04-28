#pragma once

#include "interface/Timer.h"

#include "asf.h"
#include <stdint.h>

namespace driver
{

class TimerDriver : public ITimer
{
    public:

    enum class Mode
    {
        Normal = 0,
        Pwm = 1, // If PWM mode is supported/needed for ATSAM
    };

    TimerDriver(uint32_t* timer)
        : _timer(timer) {}

    bool init(Mode mode, uint32_t periodMs)
    {
        return false;
    }

    void clearInterrupt()
    {
        
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

    // Delay implementation depends on whether we use polling or RTOS delay
    void delay(uint32_t ms) override
    {
        
    }

    uint32_t now() override
    {
        return 0;
    }
    
    void callback(void (*cb)(uint32_t)) override
    {
        
    }
    
    void interrupt()
    {

    }

    uint32_t getSpeed() override
    {
        
    }

    bool isInit() override
    {
        return _isInit;
    }

private:

    uint32_t* _timer;
    uint32_t _ms = 0;
    uint32_t _speed = 0;

    bool _isInit = false;
    void (*_cb)(uint32_t) = nullptr;
};

}