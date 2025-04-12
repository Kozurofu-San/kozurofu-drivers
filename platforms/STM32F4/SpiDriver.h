#pragma once

#include "interface/Spi.h"
#include "interface/Gpio.h"

#include "stm32f4xx.h"

namespace driver
{

class SpiDevice
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

    SpiDevice(SPI_TypeDef *spi)
        : _spi(spi)
    {
    }

    void init(Mode mode, ClockPolarity clockPolarity, ClockPhase clockPhase, DataSize dataSize, BaudRatePrescaler baudRatePrescaler)
    {
        // Clock enable
        if (_spi == SPI1) RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
        else if (_spi == SPI2) RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;

        // Configure mode
        _spi->CR1 &= ~SPI_CR1_SPE;
        _spi->CR1 = static_cast<uint32_t>(mode)
            | static_cast<uint32_t>(clockPolarity) 
            | static_cast<uint32_t>(clockPhase) 
            | static_cast<uint32_t>(dataSize) 
            | static_cast<uint32_t>(baudRatePrescaler)
            | SPI_CR1_SSM
            | SPI_CR1_SSI
        ;
        _spi->CR2 = 0x0;

        // Enable SPI
        _spi->CR1 |= SPI_CR1_SPE;
    };

    SPI_TypeDef* getSpi()
    {
        return _spi;
    }

    private:

    SPI_TypeDef* _spi;
};
    
class SpiDriver : public ISpi
{
    public:

    SpiDriver(SpiDevice &spi)
        : _spi(spi.getSpi())
    {
    }

    void init(IGpio* gpioCs = nullptr, uint8_t idleState = 1)
    {
        _spi->CR1 &= ~SPI_CR1_SPE;
        if (gpioCs)
        {
            _cs = gpioCs;
            _spi->CR2 |= SPI_CR2_SSOE;  // Soft Chip Select
        }
        else
        {
            _spi->CR2 &= ~SPI_CR2_SSOE; // Hard Chip Select
        }
        _spi->CR1 |= SPI_CR1_SPE;

        if (idleState)
        {
            _idleState = idleState;
        }
    };

    void write(uint8_t *data, size_t len) override
    {
        if (_cs)
        {
            _cs->write(!_idleState);
        }
        for (size_t i = 0; i < len; ++i)
        {
            while (!(_spi->SR & SPI_SR_TXE));
            _spi->DR = data[i];
            while((_spi->SR & SPI_SR_BSY));
        }
        if (_cs)
        {
            _cs->write(_idleState);
        }
    };

    void read(uint8_t *data, size_t len) override
    {
        if (_cs)
        {
            _cs->write(!_idleState);
        }
        for (size_t i = 0; i < len; ++i)
        {
            while (!(_spi->SR & SPI_SR_TXE));
            _spi->DR = 0xFF;
            while (!(_spi->SR & SPI_SR_RXNE));
            data[i] = _spi->DR;
        }
        if (_cs)
        {
            _cs->write(_idleState);
        }
    };

    void sendCommand(uint32_t cmd)
    {
        // Not implemented
    }
    inline void sendData(uint32_t data)
    {
        // Not implemented
    }
    inline uint32_t readData()
    {
        return 0;   // Not implemented
    }

    
    private:

    SPI_TypeDef* _spi;
    IGpio* _cs = nullptr;

    uint8_t _idleState = 1;
};

}