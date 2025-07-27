#pragma once

#include "interface/Gpio.h"

#include "stm32f1xx.h"
#include <cstdint>
#include <cstddef>

namespace driver
{
    
class GpioDriver : public IGpio
{
    public:

    enum class Mode: uint8_t
    {
        Analog = 0x0,
        Input = 0x4,
        OutputPushpull = 0x3,
        OutputOpendrain = 0x7,
        AlternatePushpull = 0xB,
        AlternateOpendrain = 0xF
    };

    enum class Speed: uint8_t
    {
        Low = 0x1,
        Meduim = 0x2,
        High = 0x3
    };

    GpioDriver(GPIO_TypeDef *port, size_t pin)
        : _port(port), _pin(pin)
    {
    }

    void init(Mode mode, Speed speed)
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
        uint8_t conf = (mode == Mode::Input) ? static_cast<uint8_t>(mode) : ((static_cast<uint8_t>(mode) & 0xC) | static_cast<uint8_t>(speed));
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
    }

    inline void clearInterrupt()
    {
        EXTI->PR = 1 << _pin;
    }

    inline GPIO_TypeDef * getPort()
    {
        return _port;
    }

    inline size_t getPin() override
    {
        return _pin;
    }

    void write(bool state) override
    {
        state ? (_port->BSRR = 1 << _pin) : (_port->BRR = 1 << _pin);
    }

    bool read() override
    {
        return (_port->IDR & (1 << _pin)) != 0;
    }

    static void mode(GPIO_TypeDef *port, size_t pin, Mode mode)
    {
        RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
        RCC->APB2ENR |= (port == GPIOA) ? RCC_APB2ENR_IOPAEN :
                          (port == GPIOB) ? RCC_APB2ENR_IOPBEN :
                          (port == GPIOC) ? RCC_APB2ENR_IOPCEN :
                          (port == GPIOD) ? RCC_APB2ENR_IOPDEN :
                          (port == GPIOE) ? RCC_APB2ENR_IOPEEN : 0;
        if (pin < 8)
        {
            port->CRL &= ~(0xF << (pin * 4));
            port->CRL |= static_cast<uint8_t>(mode) << (pin * 4);
        }
        else
        {
            port->CRH &= ~(0xF << ((pin - 8) * 4));
            port->CRH |= static_cast<uint8_t>(mode) << ((pin - 8) * 4);
        }
    }

    static void remap(size_t periph, bool on)
    {
        if (on)
        {
            AFIO->MAPR |= periph;
        }
        else
        {
            AFIO->MAPR &= ~periph;
        }
    }

    static uint16_t readPort(GPIO_TypeDef *port)
    {
        return port->IDR;
    }

    static void writePort(GPIO_TypeDef *port, uint16_t value)
    {
        port->ODR = value;
    }

    void callback(void (*cb)(uint32_t)) override
    {
        _cb = cb;
    }
    
    void interrupt(uint32_t arg)
    {
        if (_cb != nullptr)
        {
            _cb(arg);
        }
    }
    
    private:

    GPIO_TypeDef *_port;
    size_t _pin;
    
    void (*_cb)(uint32_t) = nullptr;
};

}
// namespace driver