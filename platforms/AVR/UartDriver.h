#pragma once

#include "interface/Communication.h"

#include <avr/io.h>

#define UART_BAUDRATE ((F_CPU / (UART_SPEED * 8)) - 1)

namespace driver
{

class UartDriver: public ICommunication
{
    public:

    UartDriver()
    {
    }

    bool init(uint32_t speed)
    {
        // const uint16_t baudDivider = static_cast<uint16_t>(((F_CPU) / (speed * 8UL)) - 1UL);
        UBRR0H = static_cast<uint8_t>(UART_BAUDRATE >> 8);
        UBRR0L = static_cast<uint8_t>(UART_BAUDRATE);
        UCSR0A = _BV(U2X0);
        UCSR0B = _BV(TXEN0) | _BV(RXEN0);
        UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);

        _speed = speed;
        _isInit = true;
        return true;
    };
    
    void write(uint8_t *data, size_t len, [[maybe_unused]] size_t bytes = 1) override
    {
        for (size_t i = 0; i < len; ++i)
        {
            while ((UCSR0A & _BV(UDRE0)) == 0) {}
            UDR0 = data[i];
        }
    };

    void read(uint8_t *data, size_t len, [[maybe_unused]] size_t bytes = 1) override
    {
        for (size_t i = 0; i < len; ++i)
        {
            while ((UCSR0A & _BV(RXC0)) == 0) {}
            data[i] = UDR0;
        }
    };
    
    uint32_t sendCommand([[maybe_unused]] uint32_t cmd) override
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
    

    private:

    uint32_t _speed;
    bool _isInit = false;
};

}
