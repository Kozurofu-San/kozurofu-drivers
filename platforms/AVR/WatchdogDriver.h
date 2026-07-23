#pragma once

#include "interface/Timer.h"

#include <avr/io.h>
#include <avr/wdt.h>

namespace driver
{

class WatchdogDriver : public ITimer
{
    public:

    WatchdogDriver() {}

    bool init()
    {
        wdt_enable(WDTO_2S);    // 2 seconds

        _isInit = true;
        return _isInit;
    }

    void setPeriod([[maybe_unused]] Time time) override
    {

    }
    Time getPeriod() override
    {
        return {0, ITimer::s};
    }

    void start() override
    {
        init();
        reset();
    }

    void stop() override
    {
        _isInit = false;
        wdt_disable();
    }

    void reset() override
    {
        if (!_isInit) return;
        wdt_reset(); 
    }

    void delay([[maybe_unused]] uint32_t ms) override
    {
        
    }

    uint32_t now() override
    {
        return 0;
    }
    
    void callback([[maybe_unused]] void (*cb)(uint32_t)) override
    {
        
    }
    
    uint32_t getSpeed() override
    {
        return _speed;
    }

    bool isInit() override
    {
        return _isInit;
    }

private:

    uint32_t _ms = 0;
    uint32_t _speed = 0;

    bool _isInit = false;
    void (*_cb)(uint32_t) = nullptr;
};

}