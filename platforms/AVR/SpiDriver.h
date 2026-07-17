#pragma once

#include "interface/Communication.h"
#include "interface/Gpio.h"

#include <avr/io.h>

namespace driver
{

class SpiController
{
    public:

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

    SpiController()
    {
    }

    bool init(Mode mode, ClockPolarity clockPolarity, ClockPhase clockPhase, uint16_t speed, Interrupt interrupt = Interrupt::None)
    {
    
        DDRB |= (1 << PB3) | (1 << PB5) | (1 << PB2);    // Set MOSI, SCK, and SS as outputs
        DDRB &= ~(1 << PB4);    // Set MISO as input
        SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0);    // Enable SPI, Set as Master, set clock rate fck/16
        _isInit = true;
        return true;
    };

    void write(uint8_t *data, size_t len)
    {
        
    };

    void read(uint8_t *data, size_t len)
    {
        
    };
    
    uint32_t getSpeed() const
    {
        return _speed;
    }

    bool isInit()
    {
        return _isInit;
    }
    
    private:

    uint32_t _speed;
    bool _isInit = false;
};



class SpiDriver : public ICommunication
{
    public:

    SpiDriver(SpiController &spi)
        : _spi(spi)
    {
    }

    bool init(IGpio *cs = nullptr)
    {
        _cs = cs;
        return true;
    };

    void write(uint8_t *data, size_t len, [[maybe_unused]] size_t bytes = 1) override
    {
        _spi.write(data, len);
    };

    void read(uint8_t *data, size_t len, [[maybe_unused]] size_t bytes = 1) override
    {
        _spi.read(data, len);
    };
    
    uint32_t sendCommand(uint32_t cmd) override
    {
        return 0;
    }
    
    void enable() override
    {
    }

    void disable() override
    {
    }

    inline uint32_t getSpeed() const override
    {
        return _spi.getSpeed();
    }

    inline bool isInit() override
    {
        return _spi.isInit();
    }
    
    private:

    SpiController &_spi;
    IGpio *_cs = nullptr;
};

}