#pragma once

#include "interface/Gpio.h"
#include "stm32f1xx.h"
#include <cstdint>
#include <cstddef>

namespace driver
{
    
class GpioDriver : public Gpio
{
    public:

    enum class Mode: uint8_t
    {
        INPUT = 0x0,
        OUTPUT_PUSHPULL = 0x8,
        OUTPUT_OPENDRAIN = 0x4
    };

    enum class Speed: uint8_t
    {
        LOW = 0x1,
        MEDIUM = 0x2,
        HIGH = 0x3
    };

    GpioDriver(GPIO_TypeDef *port, size_t pin, Mode mode, Speed speed)
        : _port(port), _pin(pin), _mode(mode), _speed(speed)
    {
    }

    void init() override
    {
        // Clock enable
        uint32_t rccPort = 0;
        if (_port == GPIOA) rccPort = RCC_APB2ENR_IOPAEN;
        else if (_port == GPIOB) rccPort = RCC_APB2ENR_IOPBEN;
        else if (_port == GPIOC) rccPort = RCC_APB2ENR_IOPCEN;
        else if (_port == GPIOD) rccPort = RCC_APB2ENR_IOPDEN;
        else if (_port == GPIOE) rccPort = RCC_APB2ENR_IOPEEN;
        RCC->APB2ENR |= rccPort;

        // Default "0" state
        _port->BRR = 1 << _pin;

        // Configure mode
        uint8_t conf = (_mode == Mode::INPUT) ? static_cast<uint8_t>(_mode) : (static_cast<uint8_t>(_mode) | static_cast<uint8_t>(_speed)) & ~0x8;
        if (_pin < 8)
        {
            _port->CRL &= ~(0xF << (_pin * 4));
            _port->CRL |= conf << (_pin * 4);
        }
        else
        {
            _port->CRH &= ~(0xF << ((_pin - 8) * 4));
            _port->CRH |= conf << ((_pin - 8) * 4);
        }
    };

    void gpioWrite(bool state) override
    {
        state ? (_port->BSRR = 1 << _pin) : (_port->BRR = 1 << _pin);
    };

    bool gpioRead() override
    {
        return (_port->IDR & (1 << _pin)) != 0;
    };

    private:

    GPIO_TypeDef *_port;
    size_t _pin;
    Mode _mode;
    Speed _speed;
};

}
// namespace driver