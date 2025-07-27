#pragma once

#include "interface/Spi.h"

#include "sam3x8e.h"
#include "component/component_spi.h"

namespace driver
{

class SpiDevice
{
    public:

    enum class Mode: uint32_t
    {
        Master = 0x1,
        Slave = 0x0
    };

    enum class ClockPolarity: uint32_t
    {
        IdleLow = 0x0,
        IdleHigh = 0x1
    };

    enum class ClockPhase: uint32_t
    {
        FirstEdge = 0x0,
        SecondEdge = 0x2
    };

    enum class DataSize: uint32_t
    {
        Bits8 = 0x0,
        Bits16 = 0x80
    };

    SpiDevice(Spi *spi)
        : _spi(spi)
    {
    }

    void init(Mode mode, ClockPolarity clockPolarity, ClockPhase clockPhase, DataSize dataSize, uint32_t baudRatePrescaler)
    {
        // Clock enable
        if (_spi == SPI0) {
            PMC->PMC_PCER0 = (1 << ID_SPI0);
        }
        
        _spi->SPI_CR = SPI_CR_SPIDIS; // Disable SPI
        // Configure mode
        _spi->SPI_MR = static_cast<uint32_t>(mode)
            | SPI_MR_WDRBT;
        _spi->SPI_CSR[0] = static_cast<uint32_t>(clockPolarity)
            | static_cast<uint32_t>(clockPhase)
            | static_cast<uint32_t>(dataSize)
            | SPI_CSR_SCBR(baudRatePrescaler)
            | SPI_CSR_DLYBCT(0x0); // Delay between chip selects

        // Enable SPI
        _spi->SPI_CR = SPI_CR_SPIEN;
    };

    Spi* getSpi()
    {
        return _spi;
    }

    private:

    Spi* _spi;
};
    
class SpiDriver : public ISpi
{
    public:

    SpiDriver(SpiDevice &spi)
        : _spi(spi.getSpi())
    {
    }

    void init()
    {

    }

    void write(uint8_t *data, size_t len) override
    {
        for (size_t i = 0; i < len; ++i)
        {
            while (!(_spi->SPI_SR & SPI_SR_TDRE));
            _spi->SPI_TDR = data[i];
        }
    };

    void read(uint8_t *data, size_t len) override
    {
        for (size_t i = 0; i < len; ++i)
        {
            while (!(_spi->SPI_SR & SPI_SR_TDRE));
            _spi->SPI_TDR = 0xFF;
            while (!(_spi->SPI_SR & SPI_SR_RDRF));
            data[i] = _spi->SPI_RDR;
        }
    };
    
    
    void sendCommand(uint32_t cmd) override
    {

    }
    void sendData(uint32_t data) override
    {

    }
    uint32_t readData() override
    {
        return 0;
    }

    private:

    Spi *_spi;
};

}