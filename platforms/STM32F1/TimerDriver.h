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

        // Clock
        uint32_t busPrescalerPos = (( _timer == TIM1 )) ? RCC_CFGR_PPRE2_Pos : RCC_CFGR_PPRE1_Pos;
        uint32_t busPrescaler = (RCC->CFGR >> busPrescalerPos) & 0x7;
        busPrescaler = (busPrescaler < 4) ? 1 : (1 << (busPrescaler - 3));
        _speed = SystemCoreClock / busPrescaler;
        
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
        
        // Config
        _timer->CR1 = TIM_CR1_ARPE; // Auto reload
        _timer->PSC = (_speed / pow(10, -time.unit)) - 1;  // 1 ms
        _timer->DIER = TIM_DIER_UIE;
        _timer->ARR = time.value;
        _timer->SR = 0;

        uint32_t irq = (_timer == TIM1 ) ? TIM1_TRG_COM_TIM11_IRQn  :
                    (_timer == TIM2 ) ? TIM2_IRQn  :
                    (_timer == TIM3 ) ? TIM3_IRQn  :
                    (_timer == TIM4 ) ? TIM4_IRQn  :
                    0;
        NVIC_SetPriority(static_cast<IRQn_Type>(irq), 5);
        NVIC_EnableIRQ(static_cast<IRQn_Type>(irq));

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
        _timer->SR = 0;
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

    void delay(uint32_t units) override
    {
        if (!_isInit)
            return;
        _ms = 0;
        reset();
        start();

        while (_ms < units)
        {
            asm volatile("nop");
        }

        stop();
    }

    inline uint32_t now() override
    {
        return _ms;
    }
    
    void callback(void (*cb)(uint32_t)) override
    {
        _cb = cb;
    }
    
    void interrupt()
    {
        _ms++;
        if (_cb != nullptr)
        {
            _cb(0);
        }
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
    uint32_t _ms = 0;
    uint32_t _speed = 0;

    bool _isInit = false;
    void (*_cb)(uint32_t) = nullptr;
};

}