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

class I2cDriver : public ICommunication
{
public:

    I2cDriver(uint8_t address)
        : _address(address << 1)
    {
    }

    bool init()
    {
        // Set SCL to 100kHz with 16MHz clock
        TWSR = 0x00;        // Prescaler = 1
        TWBR = I2C_DIV;        // (F_CPU / F_SCL - 16) / 2 = 72 (0x48)

        _speed = I2C_BAUDRATE;
        _isInit = true;
        return true;
    }

    // Uses the address selected by sendCommand().
    void write(uint8_t* data, [[maybe_unused]] size_t len, [[maybe_unused]] size_t bytes = 1) override
    {
        TWDR = data[0];
        TWCR = (1 << TWEN) | (1 << TWINT);
        while (!(TWCR & (1 << TWINT)));
    }

    // Uses the address selected by sendCommand().
    void read([[maybe_unused]] uint8_t* data, [[maybe_unused]] size_t len, [[maybe_unused]] size_t bytes = 1) override
    {
    }

    // Select a 7-bit I2C address for subsequent write()/read() calls.
    uint32_t sendCommand([[maybe_unused]] uint32_t readBit) override
    {
        // Send address
        TWDR = _address | readBit;
        TWCR = (1 << TWEN) | (1 << TWINT);
        while (!(TWCR & (1 << TWINT)));

        return 1;
    }

    uint32_t getSpeed() const override
    {
        return _speed;
    }

    void enable() override
    {
        TWCR = _BV(TWSTA) | _BV(TWEN) | _BV(TWINT);
        while (!(TWCR & _BV(TWINT))); // Wait for transmission to complete
    }

    void disable() override
    {
        TWCR = _BV(TWSTO) | _BV(TWEN) | _BV(TWINT);
    }

    bool isInit() override
    {
        return _isInit;
    }

    bool check(uint8_t address)
    {
        bool ret = false;

        // Start
        TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
        while (!(TWCR & (1 << TWINT))); // Wait

        // 2. Отправка адреса на запись
        TWDR = (address << 1) | TW_WRITE;
        TWCR = (1 << TWINT) | (1 << TWEN);
        while (!(TWCR & (1 << TWINT)))
            ;

        // 3. Проверка ответа (ACK)
        if ((TWSR & 0xF8) == TW_MT_SLA_ACK)
        {
            ret = true;
        }
        TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
        return ret;
    }

    uint8_t getInstance()
    {
        return _address >> 1;
    }

private:

    uint8_t _address = 0;
    uint32_t _speed = 0;
    bool _isInit = false;
};

}
