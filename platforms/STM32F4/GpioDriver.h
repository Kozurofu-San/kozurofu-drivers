#pragma once

#include "interface/Gpio.h"

#include <cstdint>
#include <cstddef>

#include "stm32f4xx.h"

namespace driver
{
    
class GpioDriver : public IGpio
{
    public:

    enum class Mode: uint8_t
    {
        Analog          = 0x3,
        Input           = 0x0,
        OutputPushpull  = 0x1,
        OutputOpendrain = 0x5,
    };

    GpioDriver(
        GPIO_TypeDef *port, size_t pin,
        IGpio::Direction dir = IGpio::Direction::Input,
        IGpio::Pull pull = IGpio::Pull::None,
        IGpio::Interrupt interrupt = IGpio::Interrupt::None
    )
        : _port(port), _pin(pin)
    {
        init(dir, pull);
    }

    bool init(Direction dir, Pull pull = Pull::None)
    {
        // Check if it's used
        if (_port->OSPEEDR & (0x3 << (_pin * 2)))
        {
            while(true);
        }

        mode(_port, _pin,
            (dir == Direction::Input) ? Mode::Input : Mode::OutputPushpull,
            pull
        );

        return true;
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

    void write(bool state) override
    {
        uint32_t setReset = state ? (1 << _pin) : (0x10000 << _pin);
        _port->BSRR = setReset;
    }

    void setDir(Direction dir) override
    {
        _port->MODER &= ~(0x3 << (_pin * 2));
        _port->MODER |= dir << (_pin * 2);
        if (dir == Direction::Input)
        {
            _port->BSRR = 1 << _pin;
        }
    }

    inline bool read() override
    {
        return (_port->IDR & (1 << _pin)) != 0;
    }

    // RM 8.3.2
    enum class Alternate: uint8_t
    {
        None                = 0x80,
        System              = 0,
        TIM1_2              = 1,
        TIM3_5              = 2,
        TIM8_11             = 3,
        I2C1_3              = 4,
        SPI1_2_I2S2_I2S2ext = 5,
        SPI3_I2Sext_I2S3    = 6,
        USART1_3_I2s3ext    = 7,
        USART4_6            = 8,
        CAN1_2_TIM12_14     = 9,
        OTGFS_HS            = 10,
        ETH_                = 11,
        FSMC_SDIO_OTGFS     = 12,
        DCMI_               = 13,
    };

    static void mode(GPIO_TypeDef *port, uint8_t pin, Mode mode, Pull pull = Pull::None, Alternate alternate = Alternate::None)
    {
        // Check if it's used
        if (port->OSPEEDR & (0x3 << (pin * 2)))
        {
            while(true);
        }

        // Clock
        RCC->AHB1ENR |= (port == GPIOA) ? RCC_AHB1ENR_GPIOAEN :
                        (port == GPIOB) ? RCC_AHB1ENR_GPIOBEN :
                        (port == GPIOC) ? RCC_AHB1ENR_GPIOCEN :
                        (port == GPIOD) ? RCC_AHB1ENR_GPIODEN :
                        (port == GPIOE) ? RCC_AHB1ENR_GPIOEEN :
                        (port == GPIOF) ? RCC_AHB1ENR_GPIOFEN : 0;
        
        // Mode
        uint8_t m = (alternate != Alternate::None) ? 0x2 : static_cast<uint8_t>(mode) & 3;
        port->MODER &= ~(0x3 << (pin * 2));
        port->MODER |= m << (pin * 2);
        port->OTYPER &= ~(0x1 << pin);
        port->OTYPER |= (static_cast<uint8_t>(mode) >> 2) << pin;
        port->OSPEEDR |= 0x3 << (pin * 2);    // Very high speed
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

    bool setCallback(void (*cb)(uint32_t)) override
    {
        if (!(_port->LCKR & (1 << _pin)))
        {
            return false;
        }

        _cb = cb;

        // Init interrupts

        __disable_irq();

        // Interrupts
        uint32_t portNumber = ((uint32_t) _port - AHB1PERIPH_BASE) >> 10;
        uint8_t syscfgNumber = _pin >> 2;
        SYSCFG->EXTICR[syscfgNumber] |= portNumber << ((_pin % 4) * 4);
        EXTI->IMR |= 1 << _pin;
        EXTI->RTSR |= 1 << _pin;    // Rise edge trigger
        EXTI->PR = 1 << _pin;

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
        return _port->OSPEEDR & (0x3 << (_pin * 2));
    }

    private:

    GPIO_TypeDef *_port;
    size_t _pin;
    
    void (*_cb)(uint32_t) = nullptr;

    enum Speed: uint8_t
    {
        Low         = 0x0,
        Meduim      = 0x1,
        High        = 0x2,
        VeryHigh    = 0x3,
    };

};

}
// namespace driver