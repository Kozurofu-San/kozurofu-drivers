#pragma once

#include "interface/Gpio.h"

#include <cstdint>
#include <cstddef>

#include "stm32f1xx.h"

extern uint32_t SystemCoreClock;

namespace driver
{
    
class GpioDriver : public IGpio
{
    public:

    enum class Mode: uint8_t
    {
        Analog = 0x0,
        Input = 0x8,
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

    bool init(Mode mode, Speed speed)
    {
        _speed = speed;

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

        setDir(mode == Mode::Input ? Direction::Input : Direction::Output);

        _isInit = true;
        return _isInit;
    }

    inline void clearInterrupt()
    {
        EXTI->PR = 1 << _pin;
    }

    inline GPIO_TypeDef * getInstance()
    {
        return _port;
    }

    inline size_t getPin() override
    {
        return _pin;
    }

    void setDir(Direction dir) override
    {
        uint8_t conf = (dir == Direction::Input) ? static_cast<uint8_t>(Mode::Input) : ((static_cast<uint8_t>(Mode::OutputPushpull) & 0xC) | static_cast<uint8_t>(_speed));
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
        if (dir == Direction::Input)
        {
            _port->BSRR = 1 << _pin;
        }
    }

    inline void write(bool state) override
    {
        state ? (_port->BSRR = 1 << _pin) : (_port->BRR = 1 << _pin);
    }

    inline bool read() override
    {
        return (_port->IDR & (1 << _pin)) != 0;
    }

    // RM 9.1.11
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

    // RM 9.3
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

    bool setCallback(void (*cb)(uint32_t)) override
    {
        if (!isInit())
        {
            return false;
        }

        _cb = cb;

        // Init interrupts

        __disable_irq();

        // AFIO clock
        RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;

        // Select GPIO port for EXTI line
        const uint32_t extiIndex = _pin / 4;
        const uint32_t extiShift = (_pin % 4) * 4;

        uint32_t portCode = 0;

        if (_port == GPIOA)      portCode = 0;
        else if (_port == GPIOB) portCode = 1;
        else if (_port == GPIOC) portCode = 2;
        else if (_port == GPIOD) portCode = 3;
        else if (_port == GPIOE) portCode = 4;
        else
        {
            __enable_irq();
            return false;
        }

        // AFIO->EXTICR[n]: select GPIO port for EXTI line
        AFIO->EXTICR[extiIndex] &= ~(0xF << extiShift);
        AFIO->EXTICR[extiIndex] |=  (portCode << extiShift);

        // Enable EXTI line
        EXTI->IMR |= (1U << _pin);

        // Example: interrupt on falling edge
        EXTI->FTSR |= (1U << _pin);
        EXTI->RTSR &= ~(1U << _pin);

        // Clear pending interrupt
        EXTI->PR = (1U << _pin);

        // Enable corresponding NVIC interrupt
        
        IRQn_Type irqn;
        if (_pin == 0) irqn = EXTI0_IRQn;
        else if (_pin == 1) irqn = EXTI1_IRQn;
        else if (_pin == 2) irqn = EXTI2_IRQn;
        else if (_pin == 3) irqn = EXTI3_IRQn;
        else if (_pin == 4) irqn = EXTI4_IRQn;
        else if (_pin >= 5 && _pin <= 9) irqn = EXTI9_5_IRQn;
        else if (_pin >= 10 && _pin <= 15) irqn = EXTI15_10_IRQn;

        NVIC_SetPriority(irqn, 5 + 1);
        NVIC_EnableIRQ(irqn);

        __enable_irq();

        return true;
    }
    
    void interrupt(uint32_t arg)
    {
        if (_cb != nullptr)
        {
            _cb(arg);
        }
    }
    
    bool isInit() override
    {
        return _isInit;
    }

    private:

    GPIO_TypeDef *_port;
    size_t _pin;

    Speed _speed;
    
    void (*_cb)(uint32_t) = nullptr;
    bool _isInit = false;
};

}
// namespace driver