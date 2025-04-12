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
        Input   = 0x0,
        Output  = 0x1
    };

    enum class Type: uint8_t
    {
        OutputPushpull  = 0x0,
        OutputOpendrain = 0x1
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

    void init(Mode mode, Type type, Speed speed, Pull pull)
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
        _port->BSRR = 0x100 << _pin;

        // Configure mode
        _port->MODER &= ~(0x3 << (_pin * 2));
        _port->MODER |= static_cast<uint8_t>(mode) << (_pin * 2);
        _port->OTYPER &= ~(0x1 << _pin);
        _port->OTYPER |= static_cast<uint8_t>(type) << _pin;
        _port->OSPEEDR &= ~(0x3 << (_pin * 2));
        _port->OSPEEDR |= static_cast<uint8_t>(speed) << (_pin * 2);
        _port->PUPDR &= ~(0x3 << (_pin * 2));
        _port->PUPDR |= static_cast<uint8_t>(pull) << (_pin * 2);
    }

    void write(bool state) override
    {
        state ? (_port->BSRR = 1 << _pin) : (_port->BSRR = 0x100 << _pin);
    }

    bool read() override
    {
        return (_port->IDR & (1 << _pin)) != 0;
    }

    static void mode(GPIO_TypeDef *port, size_t pin, Mode mode, Type type = Type::OutputPushpull, Pull pull = Pull::None)
    {
        RCC->AHB1ENR |= (port == GPIOA) ? RCC_AHB1ENR_GPIOAEN :
                          (port == GPIOB) ? RCC_AHB1ENR_GPIOBEN :
                          (port == GPIOC) ? RCC_AHB1ENR_GPIOCEN :
                          (port == GPIOD) ? RCC_AHB1ENR_GPIODEN :
                          (port == GPIOE) ? RCC_AHB1ENR_GPIOEEN :
                          (port == GPIOF) ? RCC_AHB1ENR_GPIOFEN : 0;
        
        port->MODER &= ~(0x3 << (pin * 2));
        port->MODER |= static_cast<uint8_t>(mode) << (pin * 2);
        port->OTYPER &= ~(0x1 << pin);
        port->OTYPER |= static_cast<uint8_t>(type) << pin;
        port->OSPEEDR |= 0x3 << (pin * 2);
        port->PUPDR &= ~(0x3 << (pin * 2));
        port->PUPDR |= static_cast<uint8_t>(pull) << (pin * 2);
    }

    enum class Alternate: uint8_t
    {
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

    static void remap(GPIO_TypeDef *port, size_t pin, Alternate alternate)
    {
        size_t pinLowHigh = pin < 8 ? 0 : 1;
        port->AFR[pinLowHigh] &= ~(0xF << (pin * 4));
        port->AFR[pinLowHigh] |= (static_cast<uint8_t>(alternate) & 0xF) << (pin * 4);
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
    
    private:

    GPIO_TypeDef *_port;
    size_t _pin;
    
    void (*_cb)(uint32_t) = nullptr;
};

}
// namespace driver