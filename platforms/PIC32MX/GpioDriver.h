#pragma once

#include "interface/Gpio.h"

#include <cstdint>
#include <cstddef>

#include <xc.h>
#include <sys/attribs.h>

#define REG(p, bias) (*(volatile uint32_t *)((uint32_t)p + (uint32_t)& bias - (uint32_t)& TRISB))

namespace driver
{
    
class GpioDriver : public IGpio
{
    public:

    enum class P: uint32_t
    {
        PortB = 0xBF886040,
        PortC = 0xBF886080,
        PortD = 0xBF8860C0,
        PortE = 0xBF886100,
        PortF = 0xBF886140,
        PortG = 0xBF886180,
    };

    enum class Mode: uint8_t
    {
        Input = 0x0,
        OutputPushpull = 0x3,
        OutputOpendrain = 0x7,
        AlternatePushpull = 0xB,
        AlternateOpendrain = 0xF
    };

    enum class Pull: uint8_t
    {
        None    = 0x1,
        Up      = 0x2
    };

    GpioDriver(P port, size_t pin)
        : _port(port), _pin(pin)
    {
    }

    bool init(Mode mode, Pull pull)
    {
        SYSKEY = 0xAA996655;
        SYSKEY = 0x556699AA;

        _isInit = true;
        return true;
    }

    inline P getInstance()
    {
        return _port;
    }

    inline size_t getPin() override
    {
        return _pin;
    }

    void write(bool state) override
    {
        // state ? (_port->PIO_SODR = 1 << _pin) : (_port->PIO_CODR = 1 << _pin);
    }

    bool read() override
    {
        // return (_port->PIO_PDSR & (1 << _pin)) != 0;
        return false;
    }

    void callback(void (*cb)(uint32_t)) override
    {
        _cb = cb;
    }
    
    inline bool isInit() override
    {
        return false;  // Not implemented
    }

    enum class Peripheral: uint8_t
    {
        A   = 0x0,
        B   = 0x1
    };


    static void mode(P port, size_t pin, Peripheral mode)
    {
        // port->PIO_PDR |= (1 << pin); // Disable pin
        // if (mode == Peripheral::A)
        // {
        //     port->PIO_ABSR &= ~(1 << pin);
        // }
        // else
        // {
        //     port->PIO_ABSR |= (1 << pin);
        // }
    }

    static uint16_t readPort(P port)
    {
        // return port->PIO_PDSR;
        return 0;
    }

    static void writePort(P port, uint16_t value)
    {
        // port->PIO_SODR = value;
        // port->PIO_CODR = ~value;
    }

    private:

    P _port;
    size_t _pin;

    void (*_cb)(uint32_t) = nullptr;
    uint32_t _speed = 0;
    bool _isInit = false;
};

}
// namespace driver