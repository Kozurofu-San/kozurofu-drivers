#pragma once

#include "interface/GpioDriver.h"
#include "stm32f1xx.h"
#include <cstdint>
#include <cstddef>

class GpioStm32f1 : public GpioDriver
{
    public:

    enum class Direction
    {
        INPUT,
        OUTPUT
    };

    enum class MODE
    {
        OPEN_DRAIN,
        PULLUP,
        PULLDOWN,
        NOPULL
    };

    GpioStm32f1(GPIO_TypeDef *port, size_t pin, Direction output, MODE mode)
        : _port(port), _pin(pin), _output(output), _mode(mode)
    {
    }

    void init() override
    {
        RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
        GPIOC->BRR = 1 << _pin;
        if (_pin < 8)
        {
            GPIOC->CRH &= ~(0xF << (_pin * 4));
            GPIOC->CRL |= 0xF << (_pin * 4);
        }
        else
        {
            GPIOC->CRH &= ~(0xF << ((_pin - 8) * 4));
            GPIOC->CRH |= 0xF << ((_pin - 8) * 4);
        }
    };

    void gpioWrite(bool state) override
    {
        if (state)
        {
            GPIOC->BSRR = 1 << _pin;
        }
        else
        {
            GPIOC->BRR = 1 << _pin;
        }
    };

    bool gpioRead() override
    {
        return (GPIOC->IDR & (1 << _pin)) != 0;
    };

    private:

    GPIO_TypeDef *_port;
    size_t _pin;
    Direction _output;
    MODE _mode;
};