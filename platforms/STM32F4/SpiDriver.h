#pragma once

#include "interface/Communication.h"
#include "interface/Gpio.h"

#include "DmaDriver.h"

#include <cmath>
#include <cstdio>

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

    SpiController(SPI_TypeDef *spi)
        : _spi(spi)
    {
    }
    
    bool init(Mode mode, ClockPolarity clockPolarity, ClockPhase clockPhase, DataSize dataSize, uint32_t speed)
    {
        // Clock enable
        if      (_spi == SPI1) RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
        else if (_spi == SPI2) RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;
        else if (_spi == SPI3) RCC->APB1ENR |= RCC_APB1ENR_SPI3EN;

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
        // _spi->CR2 = SPI_CR2_SSOE;
        
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

    bool setDma(DMA_Stream_TypeDef *dmaRx, size_t dmaRxChannel, DMA_Stream_TypeDef *dmaTx, size_t dmaTxChannel)
    {
        _dmaRx = dmaRx;
        _dmaTx = dmaTx;
        _dmaRxChannel = dmaRxChannel;
        _dmaTxChannel = dmaTxChannel;

        DMA_TypeDef *dma;
        // Clock DMA
        if (_spi == SPI1)
        {
            RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
            dma = DMA2;
        }
        else
        {
            RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
            dma = DMA1;
        }

        // Config
        _spi->CR1 &= ~SPI_CR1_SPE;              // Turn off SPI
        if (_dmaRx)
        {
            if (DmaDriver::getDma(_dmaRx) != dma)
            {
                return false;
            }
            _dmaRxInterruptFlag = 1 << DmaDriver::InterruptFlag[DmaDriver::getStreamNumber(_dmaRx)];
            _dmaRxStatusReg = DmaDriver::getStatusReg(_dmaRx);
            _dmaRxClearReg = DmaDriver::getClearReg(_dmaRx);

            _dmaRx->CR &= ~DMA_SxCR_EN;         // Turn off DMA stream
            while (_dmaRx->CR & DMA_SxCR_EN);   // Wait for off
            _dmaRx->PAR = (uint32_t)&_spi->DR;
            _spi->CR2 |= SPI_CR2_RXDMAEN;
        }
        if (_dmaTx)
        {
            if (DmaDriver::getDma(_dmaTx) != dma)
            {
                return false;
            }
            _dmaTxInterruptFlag = 1 << DmaDriver::InterruptFlag[DmaDriver::getStreamNumber(_dmaTx)];
            _dmaTxStatusReg = DmaDriver::getStatusReg(_dmaTx);
            _dmaTxClearReg = DmaDriver::getClearReg(_dmaTx);

            _dmaTx->CR &= ~DMA_SxCR_EN;         // Turn off DMA stream
            while (_dmaTx->CR & DMA_SxCR_EN);   // Wait for off
            _dmaTx->PAR = (uint32_t)&_spi->DR;
            _spi->CR2 |= SPI_CR2_TXDMAEN;
        }
        _spi->CR1 |= SPI_CR1_SPE;              // Turn on SPI

        return true;
    }

    void write(uint8_t *data, size_t len, size_t bytes = 1) override
    {
        if (_dmaTx)
        {
            while (!(_spi->SR & SPI_SR_TXE))
            // *_dmaTxClearReg = _dmaTxInterruptFlag;
            _dmaTx->CR &= ~DMA_SxCR_EN;
            while (_dmaTx->CR & DMA_SxCR_EN);
            _dmaTx->M0AR = reinterpret_cast<uint32_t>(data);
            _dmaTx->NDTR = len;
            _dmaTx->CR = (_dmaTxChannel << DMA_SxCR_CHSEL_Pos)
                | DMA_SxCR_MINC                 // Address increment
                | (bytes >> 1) << DMA_SxCR_MSIZE_Pos
                | DMA_SxCR_TCIE                 // Interrupt
                | static_cast<uint32_t>(DmaDriver::Direction::MemoryToPeripheral)
                ;
            _dmaTx->CR |= DMA_SxCR_EN;
            while(!(*_dmaTxStatusReg & _dmaTxInterruptFlag));
            *_dmaTxClearReg = _dmaTxInterruptFlag;
        }
        else
        {
            for (size_t i = 0; i < len; ++i)
            {
                while (!(_spi->SR & SPI_SR_TXE));
                _spi->DR = data[i];
                while (!(_spi->SR & SPI_SR_RXNE));
                (void) _spi->DR; // Read data to clear RXNE flag
            }
        }
    };

    void read(uint8_t *data, size_t len, size_t bytes = 1) override
    {
        if (_dmaRx && _dmaTx)
        {
            // DMA RX
            while (!(_spi->SR & SPI_SR_TXE))
            // *_dmaTxClearReg = _dmaTxInterruptFlag;
            // *_dmaRxClearReg = _dmaRxInterruptFlag;
            _dmaTx->CR &= ~DMA_SxCR_EN;
            while (_dmaTx->CR & DMA_SxCR_EN);
            _dmaRx->M0AR = reinterpret_cast<uint32_t>(data);
            _dmaRx->NDTR = len;
            _dmaRx->CR = (_dmaRxChannel << DMA_SxCR_CHSEL_Pos)
                | DMA_SxCR_MINC                 // Address increment
                | (bytes >> 1) << DMA_SxCR_MSIZE_Pos
                | DMA_SxCR_TCIE                 // Interrupt
                | static_cast<uint32_t>(DmaDriver::Direction::PeripheralToMemory)
                ;

            // DMA TX
            static uint8_t dummy = 0;
            // while(!(*_dmaTxInterruptReg & _dmaTxInterruptFlag));
            _dmaTx->CR &= ~DMA_SxCR_EN;
            while (_dmaTx->CR & DMA_SxCR_EN);
            _dmaTx->M0AR = reinterpret_cast<uint32_t>(&dummy);
            _dmaTx->NDTR = len;
            _dmaTx->CR = (_dmaTxChannel << DMA_SxCR_CHSEL_Pos)
                | DMA_SxCR_MINC                 // Address increment
                | (bytes >> 1) << DMA_SxCR_MSIZE_Pos
                | DMA_SxCR_TCIE                 // Interrupt
                | static_cast<uint32_t>(DmaDriver::Direction::MemoryToPeripheral)
                ;
            _dmaRx->CR |= DMA_SxCR_EN;
            _dmaTx->CR |= DMA_SxCR_EN;

            while (!(_spi->SR & SPI_SR_BSY));
            while(!(*_dmaTxStatusReg & _dmaTxInterruptFlag));
            while(!(*_dmaRxStatusReg & _dmaRxInterruptFlag));  // Wait for revieving ends
            _dmaTx->CR &= ~DMA_SxCR_EN;
            _dmaRx->CR &= ~DMA_SxCR_EN;
            (void)SPI1->DR;
            (void)SPI1->SR;
            *_dmaRxClearReg = _dmaRxInterruptFlag;
            *_dmaTxClearReg = _dmaTxInterruptFlag;
        }
        else
        {
            for (size_t i = 0; i < len; ++i)
            {
                while (!(_spi->SR & SPI_SR_TXE));
                _spi->DR = 0x00;
                while (!(_spi->SR & SPI_SR_RXNE));
                data[i] = _spi->DR;
            }
        }
    };

    uint32_t sendCommand(uint32_t cmd) override
    {
        while (!(_spi->SR & SPI_SR_TXE));
        _spi->DR = cmd;
        while (!(_spi->SR & SPI_SR_RXNE));
        return _spi->DR; // Read data to clear RXNE flag
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

    SPI_TypeDef* getInstance()
    {
        return _spi;
    }

    private:

    SPI_TypeDef         *_spi;
    uint32_t            _speed;

    DMA_Stream_TypeDef  *_dmaRx = nullptr;
    size_t              _dmaRxChannel = 0;
    uint32_t            *_dmaRxStatusReg = nullptr;
    uint32_t            *_dmaRxClearReg = nullptr;
    uint32_t            _dmaRxInterruptFlag = 0;

    DMA_Stream_TypeDef  *_dmaTx = nullptr;
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

    bool init(IGpio *cs, IdleState idleState)
    {
        _cs = cs;
        _cs->write(!_idleState);
        _idleState = static_cast<bool>(idleState);
        return _spi.isInit();
    }

    inline void write(uint8_t *data, size_t len, size_t bytes = 1) override
    {
        _spi.write(data, len);
    };

    inline void read(uint8_t *data, size_t len, size_t bytes = 1) override
    {
        _spi.read(data, len);
    };

    inline uint32_t sendCommand(uint32_t cmd) override
    {
        return _spi.sendCommand(cmd);
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
    IGpio *_cs;
    bool _idleState = true; // CS state when idle, true - high, false - low
};

}