#pragma once

#include "interface/Gpio.h"

#include <cstdint>
#include <cstddef>

#include "asf.h"
#include "component/component_pio.h"
#include "pio/pio.h"

namespace driver
{
    
class GpioDriver : public IGpio
{
    public:

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

    GpioDriver(Pio *port, size_t pin)
        : _port(port), _pin(pin)
    {
    }

    void init(Mode mode, Pull pull)
    {
        // Clock enable
        uint32_t rccId = 0;
        if      (_port == PIOA) rccId = ID_PIOA;
        else if (_port == PIOB) rccId = ID_PIOB;
        else if (_port == PIOC) rccId = ID_PIOC;
        else if (_port == PIOD) rccId = ID_PIOD;
        pmc_enable_periph_clk(rccId);
        
        pio_set_output(_port, PIO_PA23, LOW, DISABLE, ENABLE);

        _port->PIO_PER |= 1 << _pin; // Enable pin

        _port->PIO_CODR = 1 << _pin; // "0" state

        if (mode == Mode::Input)
        {
            _port->PIO_ODR = 1 << _pin;     // Input
            _port->PIO_IFER = 1 << _pin;    // Filters
        } else
        {
            _port->PIO_OER = 1 << _pin;     // Output
            _port->PIO_IFDR = 1 << _pin;    // Filters off
            _port->PIO_OWER = 1 << _pin;    // Write enable
            if (mode == Mode::OutputPushpull)
            {
                _port->PIO_MDDR = 1 << _pin; // Push-pull
            }
            else if (mode == Mode::OutputOpendrain)
            {
                _port->PIO_MDER = 1 << _pin; // Open-drain
            }
            
        }
        
        if (pull == Pull::Up)
        {
            _port->PIO_PUER = 1 << _pin;
        }
        else
        {
            _port->PIO_PUDR = 1 << _pin;
        }

    }

    

    inline Pio* getPort()
    {
        return _port;
    }

    inline size_t getPin() override
    {
        return _pin;
    }

    void write(bool state) override
    {
        state ? (_port->PIO_SODR = 1 << _pin) : (_port->PIO_CODR = 1 << _pin);
    }

    bool read() override
    {
        return (_port->PIO_PDSR & (1 << _pin)) != 0;
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


    static void mode(Pio *port, size_t pin, Peripheral mode)
    {
        port->PIO_PDR |= (1 << pin); // Disable pin
        if (mode == Peripheral::A)
        {
            port->PIO_ABSR &= ~(1 << pin);
        }
        else
        {
            port->PIO_ABSR |= (1 << pin);
        }
    }

    static uint16_t readPort(Pio *port)
    {
        return port->PIO_PDSR;
    }

    static void writePort(Pio *port, uint16_t value)
    {
        port->PIO_SODR = value;
        port->PIO_CODR = ~value;
    }

    private:

    Pio *_port;
    size_t _pin;

    void (*_cb)(uint32_t) = nullptr;
};

}
// namespace driver