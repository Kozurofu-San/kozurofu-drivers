#pragma once

#include "interface/Communication.h"
#include "interface/Gpio.h"

#include "stm32f4xx.h"
extern uint32_t SystemCoreClock;

namespace driver
{

class UartDriver : public ICommunication
{
    public:

    enum class Oversampling : uint8_t
    {
        Bits16 = 0x0,
        Bits8  = 0x1,
    };

    UartDriver(USART_TypeDef *uart)
        : _uart(uart)
    {
    }
    
    bool init(uint32_t baudRate, Oversampling oversampling)
    {
        // Clock enable
        if      (_uart == USART1) RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
        else if (_uart == USART2) RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
        else if (_uart == USART3) RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
        else if (_uart == UART4)  RCC->APB1ENR |= RCC_APB1ENR_UART4EN;
        else if (_uart == UART5)  RCC->APB1ENR |= RCC_APB1ENR_UART5EN;

        // Calculate baud rate
        uint32_t busPrescalerPos = (_uart == USART1) ? RCC_CFGR_PPRE2_Pos : RCC_CFGR_PPRE1_Pos;
        uint32_t busPrescaler = (RCC->CFGR >> busPrescalerPos) & 0x7;
        busPrescaler = (busPrescaler < 4) ? 1 : (1 << (busPrescaler - 3));
        uint32_t oversamplingBits = (2 - static_cast<uint32_t>(oversampling)) * 8;
        uint32_t busSpeed = SystemCoreClock / busPrescaler / oversamplingBits;
        uint32_t mantissa = busSpeed / baudRate;
        uint32_t fraction = (busSpeed % baudRate) * 16 / baudRate;
        _uart->BRR = (mantissa << USART_BRR_DIV_Mantissa_Pos) | (fraction << USART_BRR_DIV_Fraction_Pos);
        
        
        // Get actual baudrate
        mantissa = _uart->BRR;
        busSpeed <<= USART_BRR_DIV_Mantissa_Pos;
        _speed = busSpeed / mantissa;

        // Configure UART
        _uart->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE; // Enable transmitter and receiver
        _uart->CR2 = 0;
        _uart->CR3 = 0;
        
        _isInit = true;
        return true;
    };

    void write(uint8_t *data, size_t len) override
    {
        for (size_t i = 0; i < len; ++i)
        {
        }
    };

    void read(uint8_t *data, size_t len) override
    {
        for (size_t i = 0; i < len; ++i)
        {
        }
    };

    void sendCommand(uint32_t cmd) override
    {
    }

    USART_TypeDef* getSpi()
    {
        return _uart;
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

    USART_TypeDef *_uart;
    uint32_t _speed; // Speed in Hz
    
    bool _isInit = false;
};

}