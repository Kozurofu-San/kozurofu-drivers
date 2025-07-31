#pragma once

#include <cstdint>

class Nrf24
{
public:
    // Основные регистры
    static constexpr uint8_t    Config          = 0x00;  // Configuration Register
    static constexpr uint8_t    EnAa            = 0x01;  // Enable "Auto Acknowledgment"
    static constexpr uint8_t    EnRxAddr        = 0x02;  // Enable RX Addresses
    static constexpr uint8_t    SetupAw         = 0x03;  // Setup of Address Widths
    static constexpr uint8_t    SetupRetr       = 0x04;  // Setup of Automatic Retransmission
    static constexpr uint8_t    RfCh            = 0x05;  // RF Channel
    static constexpr uint8_t    RfSetup         = 0x06;  // RF Setup Register
    static constexpr uint8_t    Status          = 0x07;  // Status Register
    static constexpr uint8_t    ObserveTx       = 0x08;  // Observe TX
    static constexpr uint8_t    Cd              = 0x09;  // Carrier Detect
    static constexpr uint8_t    RxAddrP0        = 0x0A;  // Receive Address Data Pipe 0
    static constexpr uint8_t    TxAddr          = 0x10;  // Transmit Address
    static constexpr uint8_t    RxPwP0          = 0x11;  // Number of Bytes in RX Payload
    static constexpr uint8_t    FifoStatus      = 0x17;  // FIFO Status Register
    static constexpr uint8_t    DynPd           = 0x1C;  // Enable Dynamic Payload Length
    static constexpr uint8_t    Feature         = 0x1D;  // Feature Register

    // Класс для битов Config регистра
    class Config
    {
    public:
        static constexpr uint8_t    PrimRx        = 1 << 0;  // Primary Receiver/TX
        static constexpr uint8_t    PwrUp         = 1 << 1;  // Power Up
        static constexpr uint8_t    Crco          = 1 << 2;  // CRC Encoding Scheme
        static constexpr uint8_t    EnCrc         = 1 << 3;  // Enable CRC
        static constexpr uint8_t    MaskMaxRt     = 1 << 4;  // Mask MAX_RT Interrupt
        static constexpr uint8_t    MaskTxDs      = 1 << 5;  // Mask TX_DS Interrupt
        static constexpr uint8_t    MaskRxDr      = 1 << 6;  // Mask RX_DR Interrupt
    };

    // Класс для битов Status регистра
    class Status
    {
    public:
        static constexpr uint8_t    TxFull        = 1 << 0;  // TX FIFO Full Flag
        static constexpr uint8_t    RxPno         = 0x0E;    // RX Pipe Number (bits 3:1)
        static constexpr uint8_t    MaxRt         = 1 << 4;  // Max Retransmits Reached
        static constexpr uint8_t    TxDs          = 1 << 5;  // Data Sent TX FIFO
        static constexpr uint8_t    RxDr          = 1 << 6;  // Data Ready RX FIFO
        
        // Метод для получения номера трубы
        static constexpr uint8_t GetPipe(uint8_t status) {
            return (status & RxPno) >> 1;
        }
    };

    // Класс для SetupAw регистра
    class SetupAw
    {
    public:
        static constexpr uint8_t    Aw3Bytes      = 0x01;  // 3-byte address length
        static constexpr uint8_t    Aw4Bytes      = 0x02;  // 4-byte address length
        static constexpr uint8_t    Aw5Bytes      = 0x03;  // 5-byte address length
    };

    // Класс для RfSetup регистра
    class RfSetup
    {
    public:
        static constexpr uint8_t    RfDrLow       = 1 << 5;  // RF Data Rate 250kbps
        static constexpr uint8_t    RfDrHigh      = 1 << 3;  // RF Data Rate 2Mbps
        static constexpr uint8_t    RfPwrLow      = 1 << 1;  // RF Output Power -12dBm
        static constexpr uint8_t    RfPwrHigh     = 1 << 2;  // RF Output Power 0dBm
    };

    // SPI команды
    static constexpr uint8_t    CmdReadRegister   = 0x00;  // Read command
    static constexpr uint8_t    CmdWriteRegister  = 0x20;  // Write command
    static constexpr uint8_t    CmdReadRxPayload  = 0x61;  // Read RX payload
    static constexpr uint8_t    CmdWriteTxPayload = 0xA0;  // Write TX payload
    static constexpr uint8_t    CmdFlushTx        = 0xE1;  // Flush TX FIFO
    static constexpr uint8_t    CmdFlushRx        = 0xE2;  // Flush RX FIFO
    static constexpr uint8_t    CmdReuseTxPl      = 0xE3;  // Reuse TX payload
    static constexpr uint8_t    CmdNop            = 0xFF;  // No operation

    // Метод проверки работоспособности чипа
    static bool IsChipAlive(uint8_t setupAw, uint8_t rfCh, uint8_t status)
    {
        // Проверка значений по умолчанию после сброса
        const bool widthOk = (setupAw & 0x03) == SetupAw::Aw5Bytes;
        const bool channelOk = (rfCh & 0x7F) == 0x02;  // Default channel 2
        const bool statusOk = (status & 0x80) == 0;     // Bit 7 always 0
        
        return widthOk && channelOk && statusOk;
    }
};