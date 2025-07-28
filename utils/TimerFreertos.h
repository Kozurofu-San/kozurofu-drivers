#pragma once

#include "interface/Timer.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"


namespace driver
{

class TimerFreertos : public ITimer
{
    public:

    TimerFreertos() = default;

    void init(TimerHandle_t timer)
    {
        _timer = timer;
    }

    
    void start() override
    {

    }
    void stop() override
    {

    }
    void reset() override
    {}
    void delay(uint32_t ms) override
    {
        vTaskDelay(ms);
    }
    inline uint32_t now() override
    {
        return 0;
    }
    
    private:
    
    TimerHandle_t _timer;
};

}