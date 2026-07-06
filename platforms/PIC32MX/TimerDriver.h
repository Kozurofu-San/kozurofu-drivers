#pragma once

#include "interface/Timer.h"

#include <stdint.h>

#include <xc.h>
#include <sys/attribs.h>

namespace driver
{

class TimerDriver : public ITimer
{
    public:

    enum class P: uint32_t
    {
        PortB = 0xBF886040,
        PortC = 0xBF886080,
        PortD = 0xBF8860C0,
        PortE = 0xBF886100,
        PortF = 0xBF886140,
        PortG = 0xBF886180,
    };

    enum class Mode
    {
        Normal = 0,
        Pwm = 1, // If PWM mode is supported/needed for ATSAM
    };

    TimerDriver(P timer)
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
        return 0;
    }

    bool isInit() override
    {
        return _isInit;
    }

private:

    P _timer;
    uint32_t _ms = 0;
    uint32_t _speed = 0;

    bool _isInit = false;
    void (*_cb)(uint32_t) = nullptr;
};

}