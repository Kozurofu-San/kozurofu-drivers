#pragma once

#include "interface/Communication.h"
#include "interface/Gpio.h"

#include "stm32f4xx.h"
extern uint32_t SystemCoreClock;

namespace driver
{

class SpiController : public ICommunication
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

    SpiController(SPI_TypeDef *spi)
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
        // _spi->CR2 = SPI_CR2_SSOE;
        
        // Get baudrate
        uint32_t busPrescalerPos = (_spi == SPI1) ? RCC_CFGR_PPRE2_Pos : RCC_CFGR_PPRE1_Pos;
        uint32_t busPrescaler = (RCC->CFGR >> busPrescalerPos) & 0x7;
        busPrescaler = (busPrescaler < 4) ? 1 : (1 << (busPrescaler - 3));
        uint32_t spiPrescaler = (_spi->CR1 & SPI_CR1_BR) >> SPI_CR1_BR_Pos;
        spiPrescaler = 1 << (spiPrescaler + 1);
        _speed = SystemCoreClock / busPrescaler / spiPrescaler;

        // Enable SPI
        _spi->CR1 |= SPI_CR1_SPE;
    };

    void write(uint8_t *data, size_t len) override
    {
        for (size_t i = 0; i < len; ++i)
        {
            while (!(_spi->SR & SPI_SR_TXE));
            _spi->DR = data[i];
            while (!(_spi->SR & SPI_SR_RXNE));
            (void) _spi->DR; // Read data to clear RXNE flag
        }
    };

    void read(uint8_t *data, size_t len) override
    {
        for (size_t i = 0; i < len; ++i)
        {
            while (!(_spi->SR & SPI_SR_TXE));
            _spi->DR = 0x00;
            while (!(_spi->SR & SPI_SR_RXNE));
            data[i] = _spi->DR;
        }
    };

    void sendCommand(uint32_t cmd) override
    {
        while (!(_spi->SR & SPI_SR_TXE));
        _spi->DR = cmd;
        while (!(_spi->SR & SPI_SR_RXNE));
        cmd = _spi->DR; // Read data to clear RXNE flag
    }

    SPI_TypeDef* getSpi()
    {
        return _spi;
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

    private:

    SPI_TypeDef *_spi;
    uint32_t _speed; // Speed in Hz
};

class SpiDriver : public ICommunication
{
    public:

    enum class IdleState : bool
    {
        Low = false,
        High = true
    };

    SpiDriver(SpiController &spi)
        : _spi(spi)
    {
    }

    void init( GpioDriver *cs, IdleState idleState)
    {
        _cs = cs;
        _cs->write(!_idleState);
        _idleState = static_cast<bool>(idleState);
    }

    inline void write(uint8_t *data, size_t len) override
    {
        _spi.write(data, len);
    };

    inline void read(uint8_t *data, size_t len) override
    {
        _spi.read(data, len);
    };

    inline void sendCommand(uint32_t cmd) override
    {
        _spi.sendCommand(cmd);
    }

    void enable() override
    {
        if (_cs)
        {
            _cs->write(!_idleState);
        }
    }

    void disable() override
    {
        if (_cs)
        {
            _cs->write(_idleState);
        }
    }

    inline uint32_t getSpeed() const override
    {
        return _spi.getSpeed();
    }

    private:

    SpiController &_spi;
    GpioDriver *_cs;
    bool _idleState; // CS state when idle, true - high, false - low

};

}