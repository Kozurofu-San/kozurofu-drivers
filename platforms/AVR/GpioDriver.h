#pragma once

#include "interface/Gpio.h"

#include <avr/interrupt.h>
#include <avr/io.h>

#undef REG
#define REG(p, bias) (*(volatile uint16_t *)((uint16_t)p + (uint16_t)& bias - (uint16_t)& P1IN))

namespace driver
{

// CMSIS-like register structures for AVR GPIO
struct GPIO_TypeDef
{
    volatile uint8_t PIN;   // Input pins
    volatile uint8_t DDR;   // Data Direction Register
    volatile uint8_t PORT;  // Data Register (output / pull-up)
};

// Base addresses (memory-mapped for ATmega328P and similar)
#define GPIO_PORTB_BASE 0x23
#define GPIO_PORTC_BASE 0x26
#define GPIO_PORTD_BASE 0x29

// Pointers to port structures
#define GPIOB ((GPIO_TypeDef *) GPIO_PORTB_BASE)
#define GPIOC ((GPIO_TypeDef *) GPIO_PORTC_BASE)
#define GPIOD ((GPIO_TypeDef *) GPIO_PORTD_BASE)

class GpioDriver : public IGpio
{
public:

    enum class Mode : uint8_t
    {
        Input,
        Output,
    };

    enum class Pull : uint8_t
    {
        None,
        Up,
    };

    enum class Interrupt : uint8_t
    {
        None,
        Rise,
        Fall,
    };

    GpioDriver(GPIO_TypeDef *port, size_t pin)
        : _port(port), _pin(pin)
    {
        _pinMask = static_cast<uint8_t>(_BV(pin));
    }

    bool init(Mode mode, Pull pull, Interrupt interrupt = Interrupt::None)
    {
        // Set pin mode
        this->mode(_port, _pin, mode);

        // Pull-up configuration (only for input)
        if (mode == Mode::Input)
        {
            if (pull == Pull::Up)
                _port->PORT |= _pinMask;
            else
                _port->PORT &= ~_pinMask;
        }
        else // Output
        {
            _port->PORT &= ~_pinMask; // Start low by default
        }

        // TODO: External interrupt support (INT0/INT1) can be added here if needed
        // For full PCINT support more complex handling is required

        _isInit = true;
        return true;
    }

    void write(bool state) override
    {
        if (state)
            _port->PORT |= _pinMask;
        else
            _port->PORT &= ~_pinMask;
    }

    bool read() override
    {
        return (_port->PIN & _pinMask) != 0;
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

    static void mode(GPIO_TypeDef *port, size_t pin, Mode m)
    {
        uint8_t mask = static_cast<uint8_t>(_BV(pin));

        if (m == Mode::Output)
            port->DDR |= mask;
        else
            port->DDR &= ~mask;
    }

    static uint16_t readPort(GPIO_TypeDef *port)
    {
        return port->PIN;
    }

    static void writePort(GPIO_TypeDef *port, uint16_t value)
    {
        port->PORT = static_cast<uint8_t>(value);
    }

    size_t getPin() override
    {
        return _pin;
    }

    bool isInit() override
    {
        return _isInit;
    }

    GPIO_TypeDef* getInstance()
    {
        return _port;
    }

private:
    GPIO_TypeDef * _port;
    size_t _pin;
    uint8_t _pinMask = 0;
    bool _isInit = false;
    void (*_cb)(uint32_t) = nullptr;
};

}