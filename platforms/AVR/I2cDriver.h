#pragma once

#include "interface/I2c.h"

#include <stddef.h>
#include <stdint.h>

#include <avr/io.h>
#include <util/twi.h>

namespace driver
{

class I2cController
{
    
public:

    struct Cfg
    {
        uint8_t divider;
        uint32_t baudrate;
    };

    static Cfg calculatePrescaler(uint32_t speed)
    {
        uint8_t divider = (F_CPU / speed - 16) >> 1;
        uint32_t baudrate = F_CPU / (16 + (divider << 1));
        return {divider, baudrate};
    }

    I2cController()
    {
    }

    bool init(uint32_t speed)
    {
        // Set SCL to 100kHz with 16MHz clock
        auto cfg = calculatePrescaler(speed);
        TWSR = 0x00;            // Prescaler = 1
        TWBR = cfg.divider;

        _speed = cfg.baudrate;  // Real I2C speed
        printf("I2C speed %ld\n", cfg.baudrate);
        _isInit = true;
        return true;
    }

    void write(uint8_t data)
    {
        TWDR = data;
        TWCR = _BV(TWEN) | _BV(TWINT);
        wait();
    }

    // Read byte and return ACK (continue reading)
    uint8_t read(bool ack)
    {
        TWCR = _BV(TWEN) | _BV(TWINT) | (ack << TWEA);
        wait();
        return TWDR;
    }

    uint32_t getSpeed() const
    {
        return _speed;
    }

    void start()
    {
        TWCR = _BV(TWSTA) | _BV(TWEN) | _BV(TWINT);
        wait(); // Wait for transmission to complete
    }

    inline void stop()
    {
        TWCR = _BV(TWSTO) | _BV(TWEN) | _BV(TWINT);
    }

    inline bool isInit()
    {
        return _isInit;
    }

    bool check(uint8_t address)
    {
        bool ret = false;
        printf("I2C address 0x%X ", address);
        address <<= 1;
        start();
        write(address);
        if ((TWSR & 0xF8) == TW_MT_SLA_ACK)
        {
            ret = true;
        }
        stop();
        printf("%d\n", ret);
        return ret;
    }

private:

    static bool wait()
    {
        uint16_t count = 0;
        while (!(TWCR & _BV(TWINT)))
        {
            if (++count > Timeout)
            {
                return false;
            }
        }
        return true;
    }

    uint32_t _speed = 0;
    bool _isInit = false;

    static constexpr uint8_t Timeout = 100;
};

class I2cDriver : public II2c
{
public:

    I2cDriver(I2cController &i2c)
        : _i2c(i2c)
    {
    }

    bool init(uint8_t address)
    {
        if (_i2c.check(address))
        {
            _address = address << 1;
            return true;
        }
        return false;
    }

    void start() override
    {
        _i2c.start();
    }

    void stop() override
    {
        _i2c.stop();
    }

    // Select a 7-bit I2C address for subsequent write()/read() calls.
    void address(bool cmd) override
    {
        _i2c.write(_address | static_cast<uint8_t>(cmd));
    }

    // Uses the address selected by sendCommand().
    void write(uint8_t data) override
    {
        _i2c.write(data);
    }

    // Uses the address selected by sendCommand().
    uint8_t read(bool last = false) override
    {
        return _i2c.read(!last);
    }

    uint32_t getSpeed() const override
    {
        return _i2c.getSpeed();
    }

    bool isInit() override
    {
        return _i2c.isInit();
    }

    void setAddress(uint8_t addr)
    {
        _address = addr << 1;
    }

    uint8_t getAddress()
    {
        return _address >> 1;
    }

private:

    I2cController &_i2c;
    uint8_t _address = 0;
};

}
