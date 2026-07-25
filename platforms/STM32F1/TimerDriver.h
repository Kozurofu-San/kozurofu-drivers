#pragma once

#include "interface/Timer.h"

#include "stm32f1xx.h"

extern uint32_t SystemCoreClock;

namespace driver
{

class TimerDriver : public ITimer
{
    public:

    TimerDriver(TIM_TypeDef *timer)
        : _timer(timer) {}

    bool init(Time time)
    {
        if (time.value == 0U)
        {
            return false;
        }
        _units = time.unit;

        // TIMxCLK is PCLKx when the APB prescaler is 1, otherwise 2 * PCLKx.
        uint32_t busPrescalerPos = (( _timer == TIM1 )) ? RCC_CFGR_PPRE2_Pos : RCC_CFGR_PPRE1_Pos;
        uint32_t busPrescaler = (RCC->CFGR >> busPrescalerPos) & 0x7;
        busPrescaler = (busPrescaler < 4) ? 1 : (1 << (busPrescaler - 3));
        _speed = SystemCoreClock / busPrescaler;

        // RM 8.2
        if (busPrescaler != 1U)
        {
            _speed *= 2U;
        }
        
        if (_speed == 0)
        {
            return false;
        }

        // RM 3
        RCC->APB1ENR |= (_timer == TIM2 ) ? RCC_APB1ENR_TIM2EN  :
                        (_timer == TIM3 ) ? RCC_APB1ENR_TIM3EN  :
                        (_timer == TIM4 ) ? RCC_APB1ENR_TIM4EN  :
                        0;
        
        RCC->APB2ENR |= (_timer == TIM1 ) ? RCC_APB2ENR_TIM1EN  :
                        0;
        
        const uint32_t unitsPerSecond = (time.unit == Units::us) ? 1'000'000U :
                                        (time.unit == Units::ms) ? 1'000U : 1U;
        const uint64_t periodTicks = (static_cast<uint64_t>(_speed) * time.value) / unitsPerSecond;
        if (periodTicks == 0U)
        {
            return false;
        }

        // Choose a prescaler which makes ARR fit in the 16-bit timer.
        uint32_t divider;
        uint32_t reload;
        if (time.unit == ITimer::us)
        {
            // Free-running counter for polling delays.  PSC defines one CNT
            // increment per configured microsecond; ARR must not be a tiny
            // period, otherwise CNT wraps before delay() can observe it.
            divider = periodTicks;
            reload = 0x10000U;
        }
        else
        {
            divider = static_cast<uint32_t>((periodTicks + 0xFFFFU) / 0x10000U);
            reload = static_cast<uint32_t>(periodTicks / divider);
        }

        // Config. UG transfers PSC/ARR from their preload registers before CEN.
        _timer->CR1 = 0U;
        _timer->DIER = 0U;
        _timer->PSC = divider - 1U;
        _timer->ARR = reload - 1U;
        _timer->EGR = TIM_EGR_UG;
        _timer->SR = 0;

        if (time.unit != ITimer::us)
        {
            _timer->DIER = TIM_DIER_UIE;
            uint32_t irq = (_timer == TIM1 ) ? TIM1_TRG_COM_TIM11_IRQn  :
                        (_timer == TIM2 ) ? TIM2_IRQn  :
                        (_timer == TIM3 ) ? TIM3_IRQn  :
                        (_timer == TIM4 ) ? TIM4_IRQn  :
                        0;
            NVIC_SetPriority(static_cast<IRQn_Type>(irq), 5);
            NVIC_EnableIRQ(static_cast<IRQn_Type>(irq));
        }

        _isInit = true;
        return true;
    }

    void setPeriod(Time time){

    }
    Time getPeriod(){
        return {0, Units::s};
    }

    inline void clearInterrupt()
    {
        _timer->SR = ~TIM_SR_UIF;   // Status flags are cleared by writing zero.  Keep non-update flags intact.
    }

    inline void start() override
    {
        _timer->CR1 |= TIM_CR1_CEN;
    }

    inline void stop() override
    {
        _timer->CR1 &= ~TIM_CR1_CEN;
    }

    void reset() override
    {
        _timer->CNT = 0;
    }

    void delay(uint32_t value) override
    {
        if (_units == ITimer::us)
        {
            const uint16_t start = static_cast<uint16_t>(_timer->CNT);
            while (static_cast<uint16_t>(static_cast<uint16_t>(_timer->CNT) - start) < value)
            {
                asm volatile("nop");
            }
        }
        else
        {
            const uint32_t start = _cnt;
            while (static_cast<uint32_t>(_cnt - start) < value)
            {
                asm volatile("nop");
            }
        }
    }

    inline uint32_t now() override
    {
        if (_units == ITimer::us)
        {
            return _timer->CNT;
        }
        else
        {
            return _cnt;
        }
    }
    
    void callback(void (*cb)(uint32_t)) override
    {
        _cb = cb;
    }
    
    void interrupt()
    {
        _cnt = _cnt + 1U;
        // if (_cb != nullptr)
        // {
        //     _cb(0);
        // }
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

    TIM_TypeDef *_timer;
    volatile uint32_t _cnt = 0;     // Updated by the timer ISR and polled by delay()
    uint32_t _speed = 0;
    ITimer::Units _units;

    bool _isInit = false;
    void (*_cb)(uint32_t) = nullptr;
};

}
