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
        MASTER = 0x4,
        SLAVE = 0x0
    };

    enum class ClockPolarity: uint32_t
    {
        IDLE_LOW = 0x0,
        IDLE_HIGH = 0x2
    };

    enum class ClockPhase: uint32_t
    {
        FIRST_EDGE = 0x0,
        SECOND_EDGE = 0x1
    };

    enum class DataSize: uint32_t
    {
        BITS_8 = 0x0,
        BITS_16 = 0x800
    };

    enum class BaudRatePrescaler: uint32_t
    {
        DIV2 = 0x0,
        DIV4 = 0x1,
        DIV8 = 0x2,
        DIV16 = 0x3,
        DIV32 = 0x4,
        DIV64 = 0x5,
        DIV128 = 0x6,
        DIV256 = 0x7
    };

    SpiDriver(SPI_TypeDef *spi, Mode mode, ClockPolarity clockPolarity, ClockPhase clockPhase, DataSize dataSize, BaudRatePrescaler baudRatePrescaler)
        : _spi(spi), _mode(mode), _clockPolarity(clockPolarity), _clockPhase(clockPhase), _dataSize(dataSize), _baudRatePrescaler(baudRatePrescaler)
    {
    }

    private:

    SPI_TypeDef *_spi;
    Mode _mode;
    ClockPolarity _clockPolarity;
    ClockPhase _clockPhase;
    DataSize _dataSize;
    BaudRatePrescaler _baudRatePrescaler;

    void init() override
    {
        // Clock enable
        if (_spi == SPI1) RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
        else if (_spi == SPI2) RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;

        // Configure mode
        _spi->CR1 = static_cast<uint32_t>(_mode) | static_cast<uint32_t>(_clockPolarity) | static_cast<uint32_t>(_clockPhase) | static_cast<uint32_t>(_dataSize) | static_cast<uint32_t>(_baudRatePrescaler);
        _spi->CR2 = 0x0;

        // Enable SPI
        _spi->CR1 |= SPI_CR1_SPE;
    };

    void spiWrite(uint8_t data) override
    {
        // Wait until TXE is set
        while (!(_spi->SR & SPI_SR_TXE));

        // Write data to the DR register
        _spi->DR = data;

        // Wait until RXNE is set
        while (!(_spi->SR & SPI_SR_RXNE));

        // Read the received data
        uint8_t receivedData = _spi->DR;
    };

    uint8_t spiRead() override
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
};

}