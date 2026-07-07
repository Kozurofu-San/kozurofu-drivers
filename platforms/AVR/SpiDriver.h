#pragma once

#include "interface/Communication.h"

#include <avr/io.h>

#undef REG
#define REG(p, bias) (*(volatile uint16_t *)((uint16_t)p + (uint16_t)& bias - (uint16_t)& UCA0CTL0))

namespace driver
{

class SpiDriver : public ICommunication
{
    public:

    enum class P: uint16_t
    {
        UsciA0 = 0x60,
        UsciB0 = 0x68,
    };

    enum class Mode: uint8_t
    {
        Master  = 0,
        Slave   = 0
    };

    enum class ClockPolarity: uint8_t
    {
        IdleLow     = 0,
        IdleHigh    = 0
    };

    enum class ClockPhase: uint8_t
    {
        FirstEdge   = 0,
        SecondEdge  = 0
    };

    enum class Interrupt: uint8_t
    {
        None    = 0,
        Tx      = 0,
        Rx      = 0
    };

    SpiDriver(P spi)
        : _spi(spi)
    {
    }

    bool init(Mode mode, ClockPolarity clockPolarity, ClockPhase clockPhase, uint16_t speed, Interrupt interrupt = Interrupt::None)
    {
        return true;
    };

    void write(uint8_t *data, size_t len, size_t bytes = 1) override
    {
        
    };

    void read(uint8_t *data, size_t len, size_t bytes = 1) override
    {
        
    };
    
    uint32_t sendCommand(uint32_t cmd) override
    {
        return 0;
    }
    uint32_t getSpeed() const override
    {
        return _speed;
    }

    void enable() override
    {
    }

    void disable() override
    {
    }

    bool isInit() override
    {
        return _isInit;
    }
    
    P getInstance()
    {
        return _spi;
    }

    private:

    P _spi;
    uint32_t _speed;
    bool _isInit = false;
};

}