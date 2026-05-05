#pragma once

#include "interface/Timer.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"


#ifdef STM32F1
    #include "stm32f1xx.h"
#elif defined(STM32F4)
    #include "stm32f4xx.h"
#elif defined(ATSAM3X8E)
    #include "asf.h"
#endif

namespace driver
{

class TimerFreertos : public ITimer
{
    public:

    TimerFreertos() = default;

    bool init(TimerHandle_t *timer = nullptr)
    {
    #ifndef ESP32
        SysTick->LOAD = (SystemCoreClock / 1000) - 1;   // 1 ms
        SysTick->VAL = 0;                               // Clear current value
        SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk;     // AHB clock
    #endif
        _timer = timer;

        _isInit = true;
        return _isInit;
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
        if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
        {
            vTaskDelay(ms / portTICK_PERIOD_MS);
        }
        else
        {
        #ifndef ESP32
            SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
            for(uint32_t i=0; i < ms; i++) {
                SysTick->VAL = 0;
                while(!(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk));
            }
            SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
                SysTick->VAL = 0;
        #endif
        }
    }
    inline uint32_t now() override
    {
        return xTaskGetTickCount();
    }

    void callback(void (*cb)(uint32_t)) override
    {
        _cb = cb;
    }
    
    void interrupt()
    {
        if (_cb != nullptr)
        {
            _cb(0);
        }
    }
    
    inline uint32_t getSpeed() override
    {
        return configTICK_RATE_HZ;
    }
    
    bool isInit() override
    {
        return _isInit;
    }

    private:
    
    TimerHandle_t *_timer;
    bool _isInit = false;

    void (*_cb)(uint32_t) = nullptr;
};

}