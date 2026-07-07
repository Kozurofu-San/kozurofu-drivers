#pragma once

#include "interface/Gpio.h"

#include <stdint.h>
#include <stddef.h>

#include <avr/io.h>

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

    bool init(Mode mode, Pull pull, Interrupt interrupt = Interrupt::None)
    {
        return true;
    }

    void write(bool state) override
    {
        
    }

    bool read() override
    {
        return false;
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
    
    static void mode(P port, size_t pin, Mode mode)
    {
        
        if (mode == Mode::Input)
        {
            
        }
        else
        {
            
        }
    }

    static uint16_t readPort(P port)
    {
        return (uint16_t)port;
    }

    static void writePort(uint16_t port, uint16_t value)
    {
        
    }

    inline size_t getPin() override
    {
        return _pin;
    }

    inline bool isInit() override
    {
        return false;  // Not implemented
    }

    inline P getInstance()
    {
        return _port;
    }

    private:

    P _port;
    uint8_t _pin;

    void (*_cb)(uint32_t) = nullptr;
};

}
// namespace driver