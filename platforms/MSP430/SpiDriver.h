#pragma once

#include "interface/Spi.h"

#undef REG
#define REG(p, bias) (*(volatile uint16_t *)((uint16_t)p + (uint16_t)& bias - (uint16_t)& UCA0CTL0))

namespace driver
{

class SpiDriver : public ISpi
{
    public:

    enum class P: uint16_t
    {
        UsciA0 = 0x60,
        UsciB0 = 0x68,
    };

    enum class Mode: uint8_t
    {
        Master  = UCMST,
        Slave   = 0
    };

    enum class ClockPolarity: uint8_t
    {
        IdleLow     = 0,
        IdleHigh    = UCMSB
    };

    enum class ClockPhase: uint8_t
    {
        FirstEdge   = 0,
        SecondEdge  = UCCKPH
    };

    enum class Interrupt: uint8_t
    {
        None    = 0,
        Tx      = UCA0TXIE,
        Rx      = UCA0RXIE
    };

    SpiDriver(P spi)
        : _spi(spi)
    {
    }

    void init(Mode mode, ClockPolarity clockPolarity, ClockPhase clockPhase, uint16_t baudRatePrescaler, Interrupt interrupt = Interrupt::None)
    {
        __disable_interrupt();

        // Reset
        REG(_spi, UCA0CTL1) |= UCSWRST; 

        // Config
        REG(_spi, UCA0CTL0)
            = static_cast<uint8_t>(mode)
            | static_cast<uint8_t>(clockPolarity)
            | static_cast<uint8_t>(clockPhase)
            | UCMSB     // MSB
            | UCMODE_3  // NSS
            | UCSYNC    // SPI
        ;

        // Interrupts
        uint8_t intr = static_cast<uint8_t>(interrupt);
        if (_spi == P::UsciB0)
        {
            intr <<= 2;
        }
        IE2 = intr;
        IFG2 = 0;

        // Clock
        REG(_spi, UCA0CTL1) |= UCSSEL_2;   // SMCLK

        // Prescaler
        REG(_spi, UCA0BR0) = baudRatePrescaler & 0xFF; 
        REG(_spi, UCA0BR1) = (baudRatePrescaler >> 8) & 0xFF; 
        
        // End of init
        REG(_spi, UCA0CTL1) &= ~UCSWRST; 
        __enable_interrupt();
    };

    void write(uint8_t *data, size_t len) override
    {
        uint8_t flagTx = (_spi == P::UsciA0) ? UCA0TXIFG : UCB0TXIFG;
        for (size_t i = 0; i < len; ++i)
        {
            // while (!(IFG2 & flagTx));
            IFG2 &= ~flagTx;
            REG(_spi, UCA0TXBUF) = data[i];
        }
    };

    void read(uint8_t *data, size_t len) override
    {
        uint8_t flagTx = (_spi == P::UsciA0) ? UCA0TXIFG : UCB0TXIFG;
        uint8_t flagRx = (_spi == P::UsciA0) ? UCA0RXIFG : UCB0RXIFG;
        for (size_t i = 0; i < len; ++i)
        {
            while (!(IFG2 & flagTx));
            IFG2 &= ~flagTx;
            REG(_spi, UCA0TXBUF) = 0xFF;
            while (!(IFG2 & flagRx));
            IFG2 &= ~flagRx;
            data[i] = REG(_spi, UCA0RXBUF);
        }
    };
    
    private:

    P _spi;
};

}