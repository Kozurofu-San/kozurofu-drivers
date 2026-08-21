#pragma once

#include "interface/Uart.h"

#include "stm32f1xx.h"

extern uint32_t SystemCoreClock;

namespace driver
{

class UartDriver : public IUart
{
    public:

    UartDriver(USART_TypeDef *uart)
        : _uart(uart)
    {
    }
    
    bool init(uint32_t speed)
    {
        // Clock enable
        if      (_uart == USART1) RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
        else if (_uart == USART2) RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
        else if (_uart == USART3) RCC->APB1ENR |= RCC_APB1ENR_USART3EN;

        // Calculate baud rate
        uint32_t busPrescalerPos = (_uart == USART1) ? RCC_CFGR_PPRE2_Pos : RCC_CFGR_PPRE1_Pos;
        uint32_t busPrescaler = (RCC->CFGR >> busPrescalerPos) & 0x7;
        busPrescaler = (busPrescaler < 4) ? 1 : (1 << (busPrescaler - 3));
        uint32_t busSpeed = SystemCoreClock / busPrescaler / 16;    // Oversampling x16
        uint32_t mantissa = busSpeed / speed;
        uint32_t fraction = (busSpeed % speed) * 16 / speed;
        _uart->BRR = (mantissa << USART_BRR_DIV_Mantissa_Pos) | (fraction << USART_BRR_DIV_Fraction_Pos);
        
        // Get actual baudrate
        mantissa = _uart->BRR;
        busSpeed <<= USART_BRR_DIV_Mantissa_Pos;
        _speed = busSpeed / mantissa;

        printf("UART speed %lu Hz\n", _speed);

        // Configure UART
        _uart->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE; // Enable transmitter and receiver
        _uart->CR2 = 0;
        _uart->CR3 = 0;
        
        _isInit = true;
        return true;
    };

    bool setDma()
    {
        return true;
    }

    void write(uint8_t *data, size_t len) override
    {
        for (size_t i = 0; i < len; ++i)
        {
             while (!(_uart->SR & USART_SR_TXE));
             _uart->DR = *data++;
        }
    };

    void read(uint8_t *data, size_t len) override
    {
        for (size_t i = 0; i < len; ++i)
        {
            while (!(_uart->SR & USART_SR_RXNE));
            *data++ = _uart->DR;
        }
    };

    void setCallback(void (*cb)(uint32_t)) override
    {
        _cb = cb;
    }

    void setBuffer(uint8_t *buffer, size_t size) override
    {
        _buffer = buffer;
        _bufferSize = size;
    }
    
    USART_TypeDef* getInstance()
    {
        return _uart;
    }

    uint32_t getSpeed() const override
    {
        return _speed;
    }

    bool isInit() override
    {
        return _isInit;
    }

    private:

    USART_TypeDef *_uart;

    void (*_cb)(uint32_t) = nullptr;
    uint8_t *_buffer = nullptr;
    size_t _bufferSize = 0;

    uint32_t _speed; // Speed in Hz
    bool _isInit = false;
    
};

}
