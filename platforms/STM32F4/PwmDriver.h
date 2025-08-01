#pragma once

#include "interface/Pwm.h"

#include "stm32f4xx.h"
#include <cstdint>
#include <cstddef>

namespace driver
{

class PwmDriver : public IPwm
{
    public:

    PwmDriver(TIM_TypeDef *tim, uint32_t channel)
        : _tim(tim), _channel(channel)
    {
        // Initialize the timer and PWM channel
        RCC->APB1ENR |= RCC_APB1ENR_TIM2EN; // Enable TIM2 clock
        _tim->CR1 |= TIM_CR1_CEN; // Enable the timer
        _tim->CCMR1 |= TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2; // Set PWM mode 1
        _tim->CCER |= TIM_CCER_CC1E; // Enable output for channel 1
    }

    void setVoltage(float voltage) override
    {
        uint32_t value = static_cast<uint32_t>((voltage / 3.3f) * 4095); // Convert voltage to PWM value (12-bit resolution)
        _tim->CCR1 = value; // Set the output value for channel 1
    }

    private:

    TIM_TypeDef *_tim;
    uint32_t _channel;
};

} // namespace driver