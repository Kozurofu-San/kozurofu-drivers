#pragma once

#include "interface/Gpio.h"

#include <cstdint>
#include <cstddef>

#undef REG
#define REG(p, bias) (*(volatile uint16_t *)((uint16_t)p + (uint16_t)& bias - (uint16_t)& P1IN))

namespace driver
{
    
class GpioDriver : public IGpio
{
    public:

    enum class P: uint16_t
    {
        Port1 = 0x0020,
        Port2 = 0x0028,
    };

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

    enum class Interrupt: uint8_t
    {
        None    = 0x8,
        Rise    = 0x0,
        Fall    = 0x1
    };

    GpioDriver(P port, uint8_t pin)
        : _port(port), _pin(1 << pin)
    {
    }

    void init(Mode mode, Pull pull, Interrupt interrupt = Interrupt::None)
    {
        __disable_interrupt();

        if (mode == Mode::Input)
        {
            REG(_port, P1DIR) &= ~_pin;
        }
        else
        {
            REG(_port, P1DIR) |= _pin;
        }

        REG(_port, P1REN) &= ~_pin;
        if (pull == Pull::Up)
        {
            REG(_port, P1REN) |= _pin;
            REG(_port, P1OUT) |= _pin;
        }
        else if (pull == Pull::Down)
        {
            REG(_port, P1REN) |= _pin;
            REG(_port, P1OUT) &= ~_pin;
        }

        REG(_port, P1IE) |= _pin;
        if (interrupt == Interrupt::None)
        {
            REG(_port, P1IE) &= ~_pin;
        }
        if (interrupt == Interrupt::Rise)
        {
            REG(_port, P1IES) |= _pin;
        }
        else if (interrupt == Interrupt::Fall)
        {
            REG(_port, P1IES) &= ~_pin;
        }

        __enable_interrupt();
    }

    void write(bool state) override
    {
        state ? (REG(_port, P1OUT) |=  _pin) : (REG(_port, P1OUT) &= ~_pin);
    }

    bool read() override
    {
        return ((uintptr_t)_port & _pin) != 0;
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
    
    enum class Peripheral: uint8_t
    {
        Gpio    = 0x0,
        P1      = 0x1,
        P2      = 0x2,
        P3      = 0x3,
    };


    static void remap(P port, size_t pin, Peripheral mode)
    {
        REG(port, P1SEL)    = static_cast<uint8_t>(mode) & pin;
        REG(port, P1SEL2)   = (static_cast<uint8_t>(mode) >> 1) & pin;
    }

    static void mode(P port, size_t pin, Mode mode)
    {
        
        if (mode == Mode::Input)
        {
            REG(port, P1DIR) &= ~pin;
        }
        else
        {
            REG(port, P1DIR) |= pin;
        }
    }

    static uint16_t readPort(P port)
    {
        return (uint16_t)port;
    }

    static void writePort(uint16_t port, uint16_t value)
    {
        REG(port, P1OUT) = value;
    }

    private:

    P _port;
    uint8_t _pin;

    void (*_cb)(uint32_t) = nullptr;
};

}
// namespace driver