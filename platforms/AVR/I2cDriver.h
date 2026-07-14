#pragma once

#include "interface/Communication.h"

#include <stddef.h>
#include <stdint.h>

#include <avr/io.h>
#include <util/twi.h>

namespace driver
{

class I2cDriver : public ICommunication
{
public:

    I2cDriver()
    {
    }

    bool init(uint32_t speed)
    {
        if (speed == 0 || speed > F_CPU / 16UL)
        {
            return false;
        }

        // Prescaler = 1: SCL = F_CPU / (16 + 2 * TWBR).
        const uint32_t divider = (F_CPU / speed - 16UL) / 2UL;
        if (divider > UINT8_MAX)
        {
            return false;
        }

        TWSR = 0;
        TWBR = static_cast<uint8_t>(divider);

        // Arduino Uno: SDA = PC4/A4, SCL = PC5/A5.
        DDRC &= static_cast<uint8_t>(~(_BV(PC4) | _BV(PC5)));
        PORTC |= _BV(PC4) | _BV(PC5);

        enable();
        _speed = speed;
        _isInit = true;
        return true;
    }

    // Uses the address selected by sendCommand().
    void write(uint8_t* data, size_t len, size_t bytes = 1) override
    {
        (void)bytes;
        writeTo(_address, data, len);
    }

    // Uses the address selected by sendCommand().
    void read(uint8_t* data, size_t len, size_t bytes = 1) override
    {
        (void)bytes;
        readFrom(_address, data, len);
    }

    bool writeTo(uint8_t address, const uint8_t* data, size_t len)
    {
        if (!_isInit || data == nullptr || address > 0x7F || !start())
        {
            return false;
        }

        if (!sendAddress(address, false))
        {
            stop();
            return false;
        }

        for (size_t i = 0; i < len; ++i)
        {
            if (!writeByte(data[i]))
            {
                stop();
                return false;
            }
        }

        stop();
        return true;
    }

    bool readFrom(uint8_t address, uint8_t* data, size_t len)
    {
        if (!_isInit || data == nullptr || len == 0 || address > 0x7F || !start())
        {
            return false;
        }

        if (!sendAddress(address, true))
        {
            stop();
            return false;
        }

        for (size_t i = 0; i < len; ++i)
        {
            if (!readByte(data[i], i + 1 < len))
            {
                stop();
                return false;
            }
        }

        stop();
        return true;
    }

    // Select a 7-bit I2C address for subsequent write()/read() calls.
    uint32_t sendCommand(uint32_t cmd) override
    {
        if (cmd > 0x7F)
        {
            return 0;
        }

        _address = static_cast<uint8_t>(cmd);
        return 1;
    }

    uint32_t getSpeed() const override
    {
        return _speed;
    }

    void enable() override
    {
        TWCR = _BV(TWEN);
    }

    void disable() override
    {
        TWCR = 0;
        _isInit = false;
    }

    bool isInit() override
    {
        return _isInit;
    }

private:
    static constexpr uint16_t WaitLimit = UINT16_MAX;

    bool waitForTwint()
    {
        for (uint16_t timeout = WaitLimit; timeout != 0; --timeout)
        {
            if ((TWCR & _BV(TWINT)) != 0)
            {
                return true;
            }
        }
        return false;
    }

    static uint8_t status()
    {
        return TWSR & 0xF8;
    }

    bool start()
    {
        TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN);
        if (!waitForTwint())
        {
            return false;
        }

        const uint8_t twStatus = status();
        return twStatus == TW_START || twStatus == TW_REP_START;
    }

    bool sendAddress(uint8_t address, bool read)
    {
        TWDR = static_cast<uint8_t>((address << 1) | (read ? TW_READ : TW_WRITE));
        TWCR = _BV(TWINT) | _BV(TWEN);
        if (!waitForTwint())
        {
            return false;
        }

        return status() == (read ? TW_MR_SLA_ACK : TW_MT_SLA_ACK);
    }

    bool writeByte(uint8_t value)
    {
        TWDR = value;
        TWCR = _BV(TWINT) | _BV(TWEN);
        return waitForTwint() && status() == TW_MT_DATA_ACK;
    }

    bool readByte(uint8_t& value, bool acknowledge)
    {
        TWCR = _BV(TWINT) | _BV(TWEN) | (acknowledge ? _BV(TWEA) : 0);
        if (!waitForTwint())
        {
            return false;
        }

        const uint8_t expectedStatus = acknowledge ? TW_MR_DATA_ACK : TW_MR_DATA_NACK;
        if (status() != expectedStatus)
        {
            return false;
        }

        value = TWDR;
        return true;
    }

    static void stop()
    {
        TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWSTO);
    }

    uint32_t _speed = 0;
    uint8_t _address = 0;
    bool _isInit = false;
};

}
