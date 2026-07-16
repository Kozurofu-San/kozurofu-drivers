#pragma once

#include "interface/Communication.h"

#include <stddef.h>
#include <stdint.h>

#include <avr/io.h>
#include <util/twi.h>

#define I2C_DIV ((F_CPU / I2C_SPEED - 16) / 2)
#define I2C_BAUDRATE (F_CPU / (16 + 2 * I2C_SPEED))

namespace driver
{

class I2cController
{
public:

    I2cController()
    {
    }

    bool init()
    {
        // Set SCL to 100kHz with 16MHz clock
        TWSR = 0x00;            // Prescaler = 1
        TWBR = I2C_DIV;         // (F_CPU / F_SCL - 16) / 2 = 72 (0x48)

        _speed = I2C_BAUDRATE;  // Real I2C speed
        _isInit = true;
        return true;
    }

    void write(uint8_t data)
    {
        TWDR = data;
        TWCR = _BV(TWEN) | _BV(TWINT);
        while (!(TWCR & _BV(TWINT)));
    }

    // Read byte and return ACK (continue reading)
    uint8_t readAck()
    {
        TWCR = _BV(TWEN) | _BV(TWINT) | _BV(TWEA);
        while (!(TWCR & _BV(TWINT)))
            ;
        return TWDR;
    }

    // Read byte and return NACK (stop reading)
    uint8_t readNack()
    {
        TWCR = _BV(TWEN) | _BV(TWINT);
        while (!(TWCR & _BV(TWINT)))
            ;
        return TWDR;
    }

    uint32_t getSpeed() const
    {
        return _speed;
    }

    void start()
    {
        TWCR = _BV(TWSTA) | _BV(TWEN) | _BV(TWINT);
        while (!(TWCR & _BV(TWINT))); // Wait for transmission to complete
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
        address <<= 1;
        start();
        write(address);
        // 3. Проверка ответа (ACK)
        if ((TWSR & 0xF8) == TW_MT_SLA_ACK)
        {
            ret = true;
        }
        stop();
        return ret;
    }

private:

    uint32_t _speed = 0;
    bool _isInit = false;
};

class I2cDriver : public ICommunication
{
public:

    I2cDriver(I2cController &i2c)
        : _i2c(i2c)
    {
    }

    bool init(uint8_t address)
    {
        _address = address << 1;
        return true;
    }

    // Uses the address selected by sendCommand().
    void write(uint8_t* data, [[maybe_unused]] size_t len, [[maybe_unused]] size_t bytes = 1) override
    {
        _i2c.write(*data);
    }

    // Uses the address selected by sendCommand().
    void read([[maybe_unused]] uint8_t* data, [[maybe_unused]] size_t len, [[maybe_unused]] size_t bytes = 1) override
    {
    }

    // Select a 7-bit I2C address for subsequent write()/read() calls.
    uint32_t sendCommand([[maybe_unused]] uint32_t readBit) override
    {
        _i2c.write(readBit |= _address);
        return 0;
    }

    uint32_t getSpeed() const override
    {
        return _i2c.getSpeed();
    }

    void enable() override
    {
        _i2c.start();
    }

    void disable() override
    {
        _i2c.stop();
    }

    bool isInit() override
    {
        return _i2c.isInit();
    }

    uint8_t getInstance()
    {
        return _address >> 1;
    }

private:

    I2cController &_i2c;
    uint8_t _address = 0;
};

}
