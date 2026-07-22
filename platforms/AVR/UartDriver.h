#pragma once

#include "interface/Uart.h"

#include <avr/io.h>

#define UART_BAUDRATE ((F_CPU / (UART_SPEED * 8)) - 1)

namespace driver
{

class UartDriver: public IUart
{
    public:

    struct Cfg
    {
        uint16_t divider;
        uint32_t baudrate;
    };

    static Cfg calculatePrescaler(uint32_t speed)
    {
        uint8_t divider = (F_CPU / (speed << 3)) - 1;
        uint32_t baudrate = F_CPU / ((divider + 1) << 3);
        return {divider, baudrate};
    }

    UartDriver()
    {
    }

    bool init(uint32_t speed)
    {
        Cfg cfg = calculatePrescaler(speed);
        UBRR0 = cfg.divider;
        UCSR0A = _BV(U2X0);
        UCSR0B = _BV(TXEN0) | _BV(RXEN0);
        UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);

        printf("UART0 speed is %ld Hz\n", cfg.baudrate);
        _speed = cfg.baudrate;
        _isInit = true;
        return true;
    };
    
    void write(uint8_t *data, size_t len) override
    {
        for (size_t i = 0; i < len; ++i)
        {
            while ((UCSR0A & _BV(UDRE0)) == 0) {}
            UDR0 = data[i];
        }
    };

    void read(uint8_t *data, size_t len) override
    {
        for (size_t i = 0; i < len; ++i)
        {
            while ((UCSR0A & _BV(RXC0)) == 0) {}
            data[i] = UDR0;
        }
    };
    
    uint32_t getSpeed() const override
    {
        return _speed;
    }

    bool isInit() override
    {
        return _isInit;
    }

    private:

    uint32_t _speed = 0;
    bool _isInit = false;
};

}
