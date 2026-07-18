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
        Master  = _BV(MSTR),
        Slave   = 0
    };

    enum class Polarity: uint8_t
    {
        IdleLow     = 0,
        IdleHigh    = _BV(CPOL)
    };

    enum class Phase: uint8_t
    {
        FirstEdge   = 0,
        SecondEdge  = _BV(CPHA)
    };

    struct Cfg
    {
        uint8_t prescaler;
        uint32_t baudrate;
    };

    static constexpr uint8_t Prescaler[] = {1, 3, 5, 6};
    static Cfg calculatePrescaler(uint32_t speed)
    {
        uint32_t baudrate = 0;
        uint8_t prescaler = 0;
        for (uint8_t i = 0; i < sizeof(Prescaler) / sizeof(Prescaler[0]); i++)
        {
            baudrate = F_CPU >> Prescaler[0];
            if (baudrate <= speed)
            {
                prescaler = i + 3;
                break;
            }
            baudrate >>= 1;
            if (baudrate <= speed)
            {
                prescaler = i;
                break;
            }
        }
        return {prescaler, baudrate};
    }

    SpiController()
    {
    }

    bool init(uint32_t speed, Mode mode = Mode::Master, Polarity polarity = Polarity::IdleLow, Phase phase = Phase::FirstEdge)
    {
        DDRB |=
            _BV(PB3) |
            _BV(PB5) |
            _BV(PB2);    // Set MOSI, SCK, and SS as outputs
        DDRB &= ~_BV(PB4);    // Set MISO as input
        auto cfg = calculatePrescaler(speed);
        if (!cfg.prescaler)
        {
            return false;
        }
        SPCR =
            static_cast<uint8_t>(mode) |
            static_cast<uint8_t>(polarity) |
            static_cast<uint8_t>(phase) |
            (cfg.prescaler & 0x3); // Enable SPI, Set as Master, set clock rate
        SPCR = cfg.prescaler >> 2;
        SPCR = _BV(SPE);

        _speed = cfg.baudrate;
        _isInit = true;
        return true;
    };

    uint8_t transfer(uint8_t data)
    {
        SPDR = data;
        while (!(SPSR & (1 << SPIF)));
        return SPDR;
    }

    uint32_t getSpeed() const
    {
        return _speed;
    }

    bool isInit()
    {
        return _isInit;
    }
    
    private:

    uint32_t _speed = 0;
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
        for(uint8_t i = 0; i < len; i++)
        {
            _spi.transfer(data[i]);
        }
    };

    inline void read(uint8_t *data, size_t len, [[maybe_unused]] size_t bytes = 1) override
    {
        for(uint8_t i = 0; i < len; i++)
        {
            data[i] = _spi.transfer(0);
        }
    };
    
    inline uint32_t sendCommand([[maybe_unused]] uint32_t cmd) override
    {
        return 0;
    }
    
    void enable() override
    {
        if (_cs)
        {
            _cs->write(0);
        }
    }

    void disable() override
    {
        if (_cs)
        {
            _cs->write(1);
        }
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
    IGpio *_cs;
};

}