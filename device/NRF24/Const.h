#pragma once

#include <cstdint>

class Nrf24
{
public:
    // Main regs
    static constexpr uint8_t    CONFIG          = 0x00;  // Configuration Register
    static constexpr uint8_t    EN_AA           = 0x01;  // Enable "Auto Acknowledgment"
    static constexpr uint8_t    EN_RXADDR       = 0x02;  // Enable RX Addresses
    static constexpr uint8_t    SETUP_AW        = 0x03;  // Setup of Address Widths
    static constexpr uint8_t    SETUP_RETR      = 0x04;  // Setup of Automatic Retransmission
    static constexpr uint8_t    RF_CH           = 0x05;  // RF Channel
    static constexpr uint8_t    RF_SETUP        = 0x06;  // RF Setup Register
    static constexpr uint8_t    STATUS          = 0x07;  // Status Register
    static constexpr uint8_t    OBSERVE_TX      = 0x08;  // Observe TX
    static constexpr uint8_t    CD              = 0x09;  // Carrier Detect
    static constexpr uint8_t    RX_ADDR_P0      = 0x0A;  // Receive Address Data Pipe 0
    static constexpr uint8_t    TX_ADDR         = 0x10;  // Transmit Address
    static constexpr uint8_t    RX_PW_P0        = 0x11;  // Number of Bytes in RX Payload
    static constexpr uint8_t    FIFO_STATUS     = 0x17;  // FIFO Status Register
    static constexpr uint8_t    DYNPD           = 0x1C;  // Enable Dynamic Payload Length
    static constexpr uint8_t    FEATURE         = 0x1D;  // Feature Register

    class Config
    {
    public:
        static constexpr uint8_t    PRIM_RX       = 1 << 0;  // Primary Receiver/TX
        static constexpr uint8_t    PWR_UP        = 1 << 1;  // Power Up
        static constexpr uint8_t    CRCO          = 1 << 2;  // CRC Encoding Scheme
        static constexpr uint8_t    EN_CRC        = 1 << 3;  // Enable CRC
        static constexpr uint8_t    MASK_MAX_RT   = 1 << 4;  // Mask MAX_RT Interrupt
        static constexpr uint8_t    MASK_TX_DS    = 1 << 5;  // Mask TX_DS Interrupt
        static constexpr uint8_t    MASK_RX_DR    = 1 << 6;  // Mask RX_DR Interrupt
    };

    class Status
    {
    public:
        static constexpr uint8_t    TX_FULL       = 1 << 0;  // TX FIFO Full Flag
        static constexpr uint8_t    RX_P_NO       = 0x0E;    // RX Pipe Number (bits 3:1)
        static constexpr uint8_t    MAX_RT        = 1 << 4;  // Max Retransmits Reached
        static constexpr uint8_t    TX_DS         = 1 << 5;  // Data Sent TX FIFO
        static constexpr uint8_t    RX_DR         = 1 << 6;  // Data Ready RX FIFO
        
        static constexpr uint8_t GetPipe(uint8_t status) {
            return (status & RX_P_NO) >> 1;
        }
    };

    class SetupAw
    {
    public:
        static constexpr uint8_t    Aw3Bytes      = 0x01;  // 3-byte address length
        static constexpr uint8_t    Aw4Bytes      = 0x02;  // 4-byte address length
        static constexpr uint8_t    Aw5Bytes      = 0x03;  // 5-byte address length
    };

    class RfSetup
    {
    public:
        static constexpr uint8_t    PLL_LOCK      = 1 << 4;  // Force PLL lock signal. Only used in test
        static constexpr uint8_t    RF_DR_1Mbps   = 0 << 3;  // RF Data Rate 1 Mbps
        static constexpr uint8_t    RF_DR_2Mbps   = 1 << 3;  // RF Data Rate 2 Mbps
        static constexpr uint8_t    RF_PWR_0dBm   = 3 << 1;  // RF Output Power 0 dBm
        static constexpr uint8_t    RF_PWR_6dBm   = 2 << 1;  // RF Output Power -6 dBm
        static constexpr uint8_t    RF_PWR_12dBm  = 1 << 1;  // RF Output Power -12 dBm
        static constexpr uint8_t    RF_PWR_18dBm  = 0 << 1;  // RF Output Power -18 dBm
        static constexpr uint8_t    LNA_HCURR     = 1 << 0;  // Setup LNA gain
    };

    // SPI commands
    static constexpr uint8_t    CmdReadRegister   = 0x00;  // Read command
    static constexpr uint8_t    CmdWriteRegister  = 0x20;  // Write command
    static constexpr uint8_t    CmdReadRxPayload  = 0x61;  // Read RX payload
    static constexpr uint8_t    CmdWriteTxPayload = 0xA0;  // Write TX payload
    static constexpr uint8_t    CmdFlushTx        = 0xE1;  // Flush TX FIFO
    static constexpr uint8_t    CmdFlushRx        = 0xE2;  // Flush RX FIFO
    static constexpr uint8_t    CmdReuseTxPl      = 0xE3;  // Reuse TX payload
    static constexpr uint8_t    CmdNop            = 0xFF;  // No operation

};
