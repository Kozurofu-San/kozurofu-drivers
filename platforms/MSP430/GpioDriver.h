#pragma once

#include "interface/Gpio.h"

#include "msp430g2553.h"
#include <cstdint>
#include <cstddef>

namespace driver
{
    
class GpioDriver : public IGpio
{
    public:

    enum class Mode: uint8_t
    {
        Input = 0x0,
        Output = 0x1
    };

    enum class Pull: uint8_t
    {
        None    = 0x8,
        Up      = 0x1,
        Down    = 0x0
    };

    GpioDriver(uint16_t port, uint8_t pin)
        : _port(port), _pin(1 << pin)
    {
    }

    void init(Mode mode, Pull pull)
    {
        if (mode == Mode::Input)
        {
            *(volatile uint8_t *)(_port + (uintptr_t)&P1DIR - (uintptr_t)&P1IN) &= ~_pin;
        }
        else
        {
            *(volatile uint8_t *)(_port + (uintptr_t)&P1DIR - (uintptr_t)&P1IN) |= _pin;
        }

        *(volatile uint8_t *)(_port + (uintptr_t)&P1REN- (uintptr_t)&P1IN) &= ~_pin;
        if (pull == Pull::Up)
        {
            *(volatile uint8_t *)(_port + (uintptr_t)&P1REN - (uintptr_t)&P1IN) |= _pin;
            *(volatile uint8_t *)(_port + (uintptr_t)&P1OUT - (uintptr_t)&P1IN) |= _pin;
        }
        else if (pull == Pull::Down)
        {
            *(volatile uint8_t *)(_port + (uintptr_t)&P1REN - (uintptr_t)&P1IN) |= _pin;
            *(volatile uint8_t *)(_port + (uintptr_t)&P1OUT - (uintptr_t)&P1IN) &= ~_pin;
        }
    }

    void write(bool state) override
    {
        state ? (*(volatile uint8_t *)(_port + (uintptr_t)&P1OUT - (uintptr_t)&P1IN) |=  _pin) : (*(volatile uint8_t *)(_port + (uintptr_t)&P1OUT - (uintptr_t)&P1IN) &= ~_pin);
    }

    bool read() override
    {
        return (_port & _pin) != 0;
    }

    
    // enum class Peripheral: uint8_t
    // {
    //     A   = 0x0,
    //     B   = 0x1
    // };


    // static void mode(uint16_t port, size_t pin, Peripheral mode)
    // {
    //     // port->PIO_PDR |= (1 << pin); // Disable pin
    //     // if (mode == Peripheral::A)
    //     // {
    //     //     port->PIO_ABSR &= ~(1 << pin);
    //     // }
    //     // else
    //     // {
    //     //     port->PIO_ABSR |= (1 << pin);
    //     // }
    // }

    // static uint16_t readPort(uint16_t port)
    // {
    //     return port;
    // }

    // static void writePort(uint16_t port, uint16_t value)
    // {
    //     port = value;
    // }

    private:

    uint16_t _port;
    uint8_t _pin;
};

}
// namespace driver