#pragma once

#include "interface/Timer.h"

#include "stm32f4xx.h"
extern uint32_t SystemCoreClock;

namespace driver
{

class TimerDriver : public ITimer
{
    public:

    enum class Mode
    {
        Normal = 0,
        Pwm = 1,
    };

    TimerDriver(TIM_TypeDef *timer)
        : _timer(timer) {}

    bool init(Mode mode)
    {
        // Clock
        uint32_t busPrescalerPos = 
            ( ( _timer == TIM1  )
            | ( _timer == TIM8  )
            | ( _timer == TIM9  )
            | ( _timer == TIM10 )
            | ( _timer == TIM11 )
            ) ? RCC_CFGR_PPRE2_Pos : RCC_CFGR_PPRE1_Pos;
        uint32_t busPrescaler = (RCC->CFGR >> busPrescalerPos) & 0x7;
        busPrescaler = (busPrescaler < 4) ? 1 : (1 << (busPrescaler - 3));
        uint32_t spiPrescaler = (_timer->CR1 & SPI_CR1_BR) >> SPI_CR1_BR_Pos;
        spiPrescaler = 1 << (spiPrescaler + 1);
        _speed = SystemCoreClock / busPrescaler / spiPrescaler;
        if (_speed == 0)
        {
            return false;
        }

        RCC->AHB1ENR |= (_timer == TIM2 ) ? RCC_APB1ENR_TIM2EN  :
                        (_timer == TIM3 ) ? RCC_APB1ENR_TIM3EN  :
                        (_timer == TIM4 ) ? RCC_APB1ENR_TIM4EN  :
                        (_timer == TIM5 ) ? RCC_APB1ENR_TIM5EN  :
                        (_timer == TIM6 ) ? RCC_APB1ENR_TIM6EN  :
                        (_timer == TIM7 ) ? RCC_APB1ENR_TIM7EN  :
                        (_timer == TIM12) ? RCC_APB1ENR_TIM12EN :
                        (_timer == TIM13) ? RCC_APB1ENR_TIM13EN :
                        (_timer == TIM14) ? RCC_APB1ENR_TIM14EN :
                        0;
        
        RCC->AHB2ENR |= (_timer == TIM1 ) ? RCC_APB2ENR_TIM1EN  :
                        (_timer == TIM8 ) ? RCC_APB2ENR_TIM8EN  :
                        (_timer == TIM9 ) ? RCC_APB2ENR_TIM9EN  :
                        (_timer == TIM10) ? RCC_APB2ENR_TIM10EN :
                        (_timer == TIM11) ? RCC_APB2ENR_TIM11EN :
                        0;
        
        // Config
        if (mode == Mode::Normal)
        {
            _timer->CR1 = TIM_CR1_ARPE; // Auto reload
            _timer->PSC = (_speed / 1000) - 1;  // 1 ms
            _timer->SR = 0;

            uint32_t irq = (_timer == TIM1 ) ? TIM1_TRG_COM_TIM11_IRQn  :
                        (_timer == TIM2 ) ? TIM2_IRQn  :
                        (_timer == TIM3 ) ? TIM3_IRQn  :
                        (_timer == TIM4 ) ? TIM4_IRQn  :
                        (_timer == TIM5 ) ? TIM5_IRQn  :
                        (_timer == TIM6 ) ? TIM6_DAC_IRQn  :
                        (_timer == TIM7 ) ? TIM7_IRQn  :
                        (_timer == TIM8 ) ? TIM8_CC_IRQn  :
                        (_timer == TIM9 ) ? TIM1_BRK_TIM9_IRQn  :
                        (_timer == TIM10) ? TIM1_UP_TIM10_IRQn :
                        (_timer == TIM11) ? TIM1_TRG_COM_TIM11_IRQn :
                        (_timer == TIM12) ? TIM8_BRK_TIM12_IRQn :
                        (_timer == TIM13) ? TIM8_UP_TIM13_IRQn :
                        (_timer == TIM14) ? TIM8_TRG_COM_TIM14_IRQn :
                        0;
            NVIC_SetPriority(static_cast<IRQn_Type>(irq), 5 + 1);
            NVIC_EnableIRQ(static_cast<IRQn_Type>(irq));
        }

        _isInit = true;
        return true;
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
    {}
    inline uint32_t now() override
    {
        return _ms;
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
};

}