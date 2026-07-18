#pragma once

#include "interface/Communication.h"
#include "interface/Gpio.h"

#include <cmath>
#include <cstdio>

#include "stm32f1xx.h"

extern uint32_t SystemCoreClock;

namespace driver
{

class SpiController
{
    public:

    enum class Mode: uint32_t
    {
        Master = 0x4,
        Slave = 0x0
    };

    enum class Polarity: uint32_t
    {
        IdleLow = 0x0,
        IdleHigh = 0x2
    };

    enum class Phase: uint32_t
    {
        FirstEdge = 0x0,
        SecondEdge = 0x1
    };

    enum class DataSize: uint32_t
    {
        Bits8 = 0x0,
        Bits16 = 0x800
    };

    SpiController(SPI_TypeDef *spi)
        : _spi(spi)
    {
    }

    bool init(uint32_t speed, Mode mode = Mode::Master, Polarity clockPolarity = Polarity::IdleLow, Phase clockPhase = Phase::FirstEdge, DataSize dataSize = DataSize::Bits8)
    {
        // Clock enable
        if      (_spi == SPI1) RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
        else if (_spi == SPI2) RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;

        // Speed calculation
        uint32_t busPrescalerPos = (_spi == SPI1) ? RCC_CFGR_PPRE2_Pos : RCC_CFGR_PPRE1_Pos;
        uint32_t busPrescaler = (RCC->CFGR >> busPrescalerPos) & 0x7;
        busPrescaler = (busPrescaler < 4) ? 1 : (1 << (busPrescaler - 3));
        uint32_t busSpeed = SystemCoreClock / busPrescaler;
        uint32_t desiredPrescaler = busSpeed / speed;
        if ((desiredPrescaler < 2) || (desiredPrescaler > 256))
        {
            printf("Desired speed %lu is out of limits %lu - %lu", speed, busSpeed / DivMax, busSpeed / DivMin);
            return false;
        }
        uint32_t baudRatePrescaler = static_cast<int>(std::log2(desiredPrescaler));

        // Configure mode
        _spi->CR1 &= ~SPI_CR1_SPE;
        _spi->CR1 = static_cast<uint32_t>(mode)
            | static_cast<uint32_t>(clockPolarity) 
            | static_cast<uint32_t>(clockPhase) 
            | static_cast<uint32_t>(dataSize) 
            | static_cast<uint32_t>(baudRatePrescaler << SPI_CR1_BR_Pos)
            | SPI_CR1_SSM
            | SPI_CR1_SSI
        ;
        _spi->CR2 = 0x0;
        
        // Get baudrate
        uint32_t spiPrescaler = (_spi->CR1 & SPI_CR1_BR) >> SPI_CR1_BR_Pos;
        spiPrescaler = 1 << (spiPrescaler + 1);
        _speed = SystemCoreClock / busPrescaler / spiPrescaler;
        if (_speed == 0)
        {
            return false;
        }

        // Enable SPI
        _spi->CR1 |= SPI_CR1_SPE;
        
        _isInit = true;
        return _isInit;
    };

    uint16_t transfer(uint16_t data)
    {
        while (!(_spi->SR & SPI_SR_TXE));
        _spi->DR = data;
        while (!(_spi->SR & SPI_SR_RXNE));
        return _spi->DR;
    };

    uint32_t getSpeed() const
    {
        return _speed;
    }

    bool isInit()
    {
        return _isInit;
    }

    SPI_TypeDef* getInstance()
    {
        return _spi;
    }

    private:

    SPI_TypeDef* _spi;
    uint32_t            _speed;

    DMA_TypeDef         *_dmaRx = nullptr;
    size_t              _dmaRxChannel = 0;
    uint32_t            *_dmaRxStatusReg = nullptr;
    uint32_t            *_dmaRxClearReg = nullptr;
    uint32_t            _dmaRxInterruptFlag = 0;

    DMA_TypeDef         *_dmaTx = nullptr;
    size_t              _dmaTxChannel = 0;
    uint32_t            *_dmaTxStatusReg = nullptr;
    uint32_t            *_dmaTxClearReg = nullptr;
    uint32_t            _dmaTxInterruptFlag = 0;

    bool _isInit = false;

    static constexpr uint16_t DivMin = 2;
    static constexpr uint16_t DivMax = 256;
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

    void init(IGpio* cs = nullptr, uint8_t idleState = true)
    {
        _spi.getInstance()->CR1 &= ~SPI_CR1_SPE;
        if (cs)
        {
            _cs = cs;
            _spi.getInstance()->CR2 |= SPI_CR2_SSOE;  // Soft Chip Select
        }
        else
        {
            _spi.getInstance()->CR2 &= ~SPI_CR2_SSOE; // Hard Chip Select
        }
        _spi.getInstance()->CR1 |= SPI_CR1_SPE;

        if (idleState)
        {
            _idleState = idleState;
        }
    };

    void write(uint8_t *data, size_t len, size_t bytes = 1) override
    {
        for (size_t i = 0; i < len; i++)
        {
            _spi.transfer(data[i]);
        }
    };

    void read(uint8_t *data, size_t len, size_t bytes = 1) override
    {
        for (size_t i = 0; i < len; i++)
        {
            data[i] = _spi.transfer(0);
        }
    };

    inline uint32_t sendCommand(uint32_t cmd) override
    {
        return _spi.transfer(cmd);
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

    bool isInit() override
    {
        return _spi.isInit();
    }

    private:

    SpiController &_spi;
    IGpio* _cs = nullptr;
    bool _idleState = true; // CS state when idle, true - high, false - low
};

}