#pragma once

#include "interface/Spi.h"

#include "stm32f1xx.h"

namespace driver
{

class SpiDriver : public Spi
{
    public:

    enum class Mode: uint32_t
    {
        Master = 0x4,
        Slave = 0x0
    };

    enum class ClockPolarity: uint32_t
    {
        IdleLow = 0x0,
        IdleHigh = 0x2
    };

    enum class ClockPhase: uint32_t
    {
        FirstEdge = 0x0,
        SecondEdge = 0x1
    };

    enum class DataSize: uint32_t
    {
        Bits8 = 0x0,
        Bits16 = 0x800
    };

    enum class BaudRatePrescaler: uint32_t
    {
        Div2 = 0x0,
        Div4 = 0x8,
        Div8 = 0x10,
        Div16 = 0x18,
        Div32 = 0x20,
        Div64 = 0x28,
        Div128 = 0x30,
        Div256 = 0x38
    };

    SpiDriver(SPI_TypeDef *spi, Mode mode, ClockPolarity clockPolarity, ClockPhase clockPhase, DataSize dataSize, BaudRatePrescaler baudRatePrescaler)
        : _spi(spi)
    {
        // Clock enable
        if (_spi == SPI1) RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
        else if (_spi == SPI2) RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;

        // Configure mode
        _spi->CR1 = static_cast<uint32_t>(mode) | static_cast<uint32_t>(clockPolarity) | static_cast<uint32_t>(clockPhase) | static_cast<uint32_t>(dataSize) | static_cast<uint32_t>(baudRatePrescaler);
        _spi->CR2 = 0x0;

        // Enable SPI
        _spi->CR1 |= SPI_CR1_SPE;
    }

    void init() override
    {
    };

    void write(uint8_t data) override
    {
        // Wait until TXE is set
        while (!(_spi->SR & SPI_SR_TXE));

        // Write data to the DR register
        _spi->DR = data;
    };

    uint8_t read() override
    {
        // Wait until TXE is set
        while (!(_spi->SR & SPI_SR_TXE));

        // Write dummy data to the DR register
        _spi->DR = 0xFF;

        // Wait until RXNE is set
        while (!(_spi->SR & SPI_SR_RXNE));

        // Read the received data
        return _spi->DR;
    };
    
    private:

    SPI_TypeDef *_spi;
};

}