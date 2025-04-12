#pragma once

#include "interface/Gpio.h"

#include "stm32f4xx.h"
#include <cstdint>
#include <cstddef>

namespace driver
{
    
class GpioDriver : public IGpio
{
    public:

    enum class Mode: uint8_t
    {
        Input           = 0x0,
        OutputPushpull  = 0x2,
        OutputOpendrain = 0x3
    };

    enum class Speed: uint8_t
    {
        Low         = 0x0,
        Meduim      = 0x1,
        High        = 0x2,
        VeryHigh    = 0x3
    };

    enum class Pull: uint8_t
    {
        None        = 0x0,
        Up          = 0x1,
        Down        = 0x2
    };

    GpioDriver(GPIO_TypeDef *port, size_t pin)
        : _port(port), _pin(pin)
    {
    }

    enum class Interrupt: uint8_t
    {
        None,
        Rise,
        Fall,
        RiseFall
    };

    enum class Alternate: uint8_t
    {
        None                = 0x80,
        Sys                 = 0,
        Tim1_2              = 1,
        Tim3_4_5            = 2,
        Tim8_9_10_11        = 3,
        I2c1_2_3            = 4,
        Spi1_2_I2s2_I2s2ext = 5,
        Spi3_I2sext_I2s3    = 6,
        Usart1_2_3_I2s3ext  = 7,
        Usart4_5_6          = 8,
        Can1_2_Tim12_13_14  = 9,
        OtgFs_Hs            = 10,
        Eth                 = 11,
        Fsmc_OtgFs          = 12,
        Dcmi                = 13,
    };

    void init(Mode mode, Speed speed, Pull pull, Interrupt interrupt = Interrupt::None)
    {
        // Clock enable
        uint32_t rccPort = 0;
        if (_port == GPIOA) rccPort = RCC_AHB1ENR_GPIOAEN;
        else if (_port == GPIOB) rccPort = RCC_AHB1ENR_GPIOBEN;
        else if (_port == GPIOC) rccPort = RCC_AHB1ENR_GPIOCEN;
        else if (_port == GPIOD) rccPort = RCC_AHB1ENR_GPIODEN;
        else if (_port == GPIOE) rccPort = RCC_AHB1ENR_GPIOEEN;
        RCC->AHB1ENR |= rccPort;

        // Default "0" state
        _port->BSRR = 0x10000 << _pin;

        // Configure mode
        _port->MODER &= ~(0x3 << (_pin * 2));
        _port->MODER |= (static_cast<uint8_t>(mode) >> 1) << (_pin * 2);
        _port->OTYPER &= ~(0x1 << _pin);
        _port->OTYPER |= (static_cast<uint8_t>(mode) & 0x1) << _pin;
        _port->OSPEEDR &= ~(0x3 << (_pin * 2));
        _port->OSPEEDR |= static_cast<uint8_t>(speed) << (_pin * 2);
        _port->PUPDR &= ~(0x3 << (_pin * 2));
        _port->PUPDR |= static_cast<uint8_t>(pull) << (_pin * 2);

        // Interrupts
        if (interrupt == Interrupt::None)
        {
            return;
        }
        uint32_t portNumber = ((uint32_t) _port - AHB1PERIPH_BASE) >> 10;
        uint8_t syscfgNumber = _pin >> 2;
        SYSCFG->EXTICR[syscfgNumber] |= portNumber << ((_pin % 4) * 4);
        EXTI->IMR |= 1 << _pin;
        if (interrupt == Interrupt::Rise || interrupt == Interrupt::RiseFall)
        {
            EXTI->RTSR |= 1 << _pin;
        }
        if (interrupt == Interrupt::Fall || interrupt == Interrupt::RiseFall)
        {
            EXTI->FTSR |= 1 << _pin;
        }
        EXTI->PR = 1 << _pin;

        uint32_t irqn = -1;
        if (_pin == 0) irqn = EXTI0_IRQn;
        else if (_pin == 1) irqn = EXTI1_IRQn;
        else if (_pin == 2) irqn = EXTI2_IRQn;
        else if (_pin == 3) irqn = EXTI3_IRQn;
        else if (_pin == 4) irqn = EXTI4_IRQn;
        else if (_pin >= 5 && _pin <= 9) irqn = EXTI9_5_IRQn;
        else if (_pin >= 10 && _pin <= 15) irqn = EXTI15_10_IRQn;

        NVIC_SetPriority(static_cast<IRQn_Type>(irqn), 5 + 1);    // configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1
        NVIC_EnableIRQ(static_cast<IRQn_Type>(irqn));

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
        uint32_t setReset = state ? (1 << _pin) : (0x10000 << _pin);
        _port->BSRR = setReset;
    }

    inline bool read() override
    {
        return (_port->IDR & (1 << _pin)) != 0;
    }

    static void mode(GPIO_TypeDef *port, size_t pin, Mode mode, Pull pull = Pull::None, Alternate alternate = Alternate::None)
    {
        // Clock
        RCC->AHB1ENR |= (port == GPIOA) ? RCC_AHB1ENR_GPIOAEN :
                        (port == GPIOB) ? RCC_AHB1ENR_GPIOBEN :
                        (port == GPIOC) ? RCC_AHB1ENR_GPIOCEN :
                        (port == GPIOD) ? RCC_AHB1ENR_GPIODEN :
                        (port == GPIOE) ? RCC_AHB1ENR_GPIOEEN :
                        (port == GPIOF) ? RCC_AHB1ENR_GPIOFEN : 0;
        
        // Mode
        uint8_t m = (alternate != Alternate::None) ? 0x2 : static_cast<uint8_t>(mode) >> 1;
        port->MODER &= ~(0x3 << (pin * 2));
        port->MODER |= m << (pin * 2);
        port->OTYPER &= ~(0x1 << pin);
        port->OTYPER |= (static_cast<uint8_t>(mode) & 0x1) << pin;
        port->OSPEEDR |= 0x3 << (pin * 2);
        port->PUPDR &= ~(0x3 << (pin * 2));
        port->PUPDR |= static_cast<uint8_t>(pull) << (pin * 2);

        // Alternate function
        if (alternate != Alternate::None)
        {
            size_t pinLowHigh = pin < 8 ? 0 : 1;
            port->AFR[pinLowHigh] &= ~(0xF << ((pin % 8) * 4));
            port->AFR[pinLowHigh] |= (static_cast<uint8_t>(alternate) & 0xF) << ((pin % 8) * 4);
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