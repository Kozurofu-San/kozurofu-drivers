#pragma once

#include "interface/Uart.h"

#include "stm32f1xx.h"

#include <cstdint>
#include <cstddef>
#include <cstring>

namespace driver
{

class UsbCdc : public IUart
{
public:
    UsbCdc()
    {
        init();
    }

    void write(uint8_t *data, size_t len) override
    {
        if (!_configured || len == 0 || data == nullptr)
            return;

        serialWrite(data, len);
    }

    void read(uint8_t *data, size_t len) override
    {
        if (!_configured || len == 0 || data == nullptr)
            return;

        serialRead(data, len);
    }

    void init()
    {
        // Clocks
        RCC->APB1ENR |= RCC_APB1ENR_USBEN;
        RCC->APB2ENR |= RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPAEN;

        // Force reset
        USB->CNTR = USB_CNTR_FRES;
        USB->ISTR = 0;
        USB->CNTR = USB_CNTR_RESETM;

        // NVIC
        NVIC_SetPriority(USB_LP_CAN1_RX0_IRQn, 1);
        NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);

        _initialized = true;
    }

    // Call this from USB_LP_CAN1_RX0_IRQHandler
    void interrupt()
    {
        uint16_t istr = USB->ISTR;

        if (istr & USB_ISTR_RESET)
        {
            usbReset();
            USB->ISTR = 0;
            return;
        }

        if (istr & USB_ISTR_CTR)
        {
            correctTransfer();
        }

        USB->ISTR = 0;
    }

    uint32_t getSpeed() const override
    {
        return 12'000'000u;   // USB Full-Speed
    }

    bool isInit() override
    {
        return _initialized;
    }

    // Optional: line state (DTR/RTS) from host
    bool isPortOpen() const { return (_lineState & 0x01) != 0; }

private:
    // ===================== Configuration =====================
    static constexpr bool     DOUBLE_BUF   = true;
    static constexpr uint16_t EP0_SIZE     = 8;
    static constexpr uint16_t EP1_SIZE     = 8;
    static constexpr uint16_t EP2_SIZE     = 64;   // Bulk IN/OUT

    // ===================== Hardware =====================
    static constexpr uint32_t USB_BASE_ADDR     = 0x40005C00u;
    static constexpr uint32_t USB_PMA_BASE      = 0x40006000u;
    volatile uint32_t* const USB_EPR    = reinterpret_cast<volatile uint32_t*>(USB_BASE_ADDR);

    // ===================== State =====================
    bool     _initialized = false;
    bool     _configured  = false;
    uint8_t  _deviceAddr  = 0;
    uint8_t  _lineState   = 0;   // from SET_CONTROL_LINE_STATE
    uint8_t  _cdcActive   = 0;

    // PMA buffer table offsets (words)
    static constexpr uint16_t PMA_BEGIN = 16; // 4 endpoints * 4 entries

    // Runtime pointers / counters (mirror of assembly usb_param)
    uint8_t* _ptrTx     = nullptr;
    uint8_t* _ptrRx     = nullptr;
    uint16_t _cntTx     = 0;
    uint16_t _cntRx     = 0;

    // Setup packet buffer
    alignas(4) uint8_t _setup[8]{};

    // ===================== Descriptors (from your .s) =====================
    static constexpr uint8_t deviceDescriptor[] = {
        18,                     // bLength
        0x01,                   // bDescriptorType DEVICE
        0x10, 0x01,             // bcdUSB 1.10
        0x02,                   // bDeviceClass CDC
        0x00,                   // bDeviceSubClass
        0x00,                   // bDeviceProtocol
        EP0_SIZE,               // bMaxPacketSize0
        0x34, 0x12,             // idVendor  0x1234
        0x78, 0x56,             // idProduct 0x5678
        0x08, 0x27,             // bcdDevice
        1,                      // iManufacturer
        2,                      // iProduct
        3,                      // iSerialNumber
        1                       // bNumConfigurations
    };

    static constexpr uint8_t configDescriptor[] = {
        // Configuration
        9, 0x02,
        67, 0x00,               // wTotalLength
        2,                      // bNumInterfaces
        1,                      // bConfigurationValue
        0,                      // iConfiguration
        0xC0,                   // bmAttributes Self-powered
        50,                     // bMaxPower 100 mA

        // Interface 0 - CDC Control
        9, 0x04, 0, 0, 1, 0x02, 0x02, 0x00, 4,

        // Header Functional
        5, 0x24, 0x00, 0x10, 0x01,
        // Call Management
        5, 0x24, 0x01, 0x00, 1,
        // ACM
        4, 0x24, 0x02, 0x02,
        // Union
        5, 0x24, 0x06, 0, 1,

        // EP1 IN Interrupt
        7, 0x05, 0x81, 0x03, EP1_SIZE, 0x00, 0x00,

        // Interface 1 - CDC Data
        9, 0x04, 1, 0, 2, 0x0A, 0x00, 0x00, 5,

        // EP2 IN Bulk
        7, 0x05, 0x82, 0x02, EP2_SIZE, 0x00, 0x00,
        // EP3 OUT Bulk
        7, 0x05, 0x03, 0x02, EP2_SIZE, 0x00, 0x00
    };

    // String descriptors (Unicode)
    static constexpr uint8_t stringLang[] = { 4, 0x03, 0x09, 0x04 };
    static constexpr uint8_t stringMfg[]  = { 12, 0x03, 'R',0,'o',0,'m',0,'a',0,'n',0 };
    static constexpr uint8_t stringProd[] = { 8,  0x03, 'G',0,'P',0,'R',0 };
    static constexpr uint8_t stringSN[]   = { 10, 0x03, '8',0,'0',0,'5',0,'1',0 };
    static constexpr uint8_t stringCtrl[] = { 24, 0x03, 'C',0,'D',0,'C',0,' ',0,'C',0,'o',0,'n',0,'t',0,'r',0,'o',0,'l',0 };
    static constexpr uint8_t stringData[] = { 18, 0x03, 'C',0,'D',0,'C',0,' ',0,'D',0,'a',0,'t',0,'a',0 };

private:
    void usbReset()
    {
        _configured = false;
        _deviceAddr = 0;
        _cdcActive  = 0;
        _lineState  = 0;

        // Clear BTABLE
        USB->BTABLE = 0;

        // Build BTABLE in PMA
        volatile uint16_t* pma = reinterpret_cast<volatile uint16_t*>(USB_PMA_BASE);

        // EP0 TX/RX
        pma[0]  = PMA_BEGIN;                          // ADDR_TX
        pma[1]  = 0;                                  // COUNT_TX
        pma[2]  = PMA_BEGIN + EP0_SIZE / 2;           // ADDR_RX
        pma[3]  = (EP0_SIZE / 2) << 10;               // COUNT_RX

        // EP1 TX/RX (interrupt)
        pma[4]  = PMA_BEGIN + EP0_SIZE;
        pma[5]  = 0;
        pma[6]  = PMA_BEGIN + EP0_SIZE + EP1_SIZE / 2;
        pma[7]  = 0;

        if constexpr (DOUBLE_BUF)
        {
            // EP2 IN double buffered
            pma[8]  = PMA_BEGIN + EP0_SIZE + EP1_SIZE / 2;
            pma[9]  = 0;
            pma[10] = PMA_BEGIN + EP0_SIZE + EP1_SIZE / 2 + EP2_SIZE / 2;
            pma[11] = 0;

            // EP3 OUT double buffered
            pma[12] = PMA_BEGIN + EP0_SIZE + EP1_SIZE / 2 + EP2_SIZE;
            pma[13] = (1 << 15) | (1 << 10);          // BL_SIZE=1, NUM_BLOCK=1 → 64 bytes
            pma[14] = PMA_BEGIN + EP0_SIZE + EP1_SIZE / 2 + EP2_SIZE + EP2_SIZE / 2;
            pma[15] = (1 << 15) | (1 << 10);
        }
        else
        {
            // Single buffer version (simplified)
            pma[8]  = PMA_BEGIN + EP0_SIZE + EP1_SIZE / 2;
            pma[9]  = 0;
            pma[10] = PMA_BEGIN + EP0_SIZE + EP1_SIZE / 2 + EP2_SIZE / 2;
            pma[11] = (1 << 15) | (1 << 10);

            pma[12] = PMA_BEGIN + EP0_SIZE + EP1_SIZE / 2 + EP2_SIZE / 2;
            pma[13] = 0;
            pma[14] = PMA_BEGIN + EP0_SIZE + EP1_SIZE / 2 + EP2_SIZE / 2;
            pma[15] = (1 << 15) | (1 << 10);
        }

        // Endpoint registers
        USB_EPR[0] = (0 << 0) | (1 << 9) | (3 << 12) | (3 << 4); // CONTROL, VALID RX/TX
        USB_EPR[1] = (1 << 0) | (3 << 9) | (3 << 12) | (3 << 4); // INTERRUPT

        if constexpr (DOUBLE_BUF)
        {
            USB_EPR[2] = (2 << 0) | (0 << 9) | (2 << 12) | (2 << 4) | (1 << 8) | (1 << 14); // BULK, KIND, DTOG
            USB_EPR[3] = (3 << 0) | (0 << 9) | (3 << 12) | (0 << 4) | (1 << 8) | (1 << 6);
        }
        else
        {
            USB_EPR[2] = (2 << 0) | (0 << 9) | (3 << 12) | (3 << 4);
            USB_EPR[3] = (3 << 0) | (0 << 9) | (3 << 12) | (3 << 4);
        }

        USB->CNTR  = USB_CNTR_CTRM | USB_CNTR_RESETM;
        USB->DADDR = USB_DADDR_EF;
        USB->ISTR  = 0;
    }

    void correctTransfer()
    {
        uint16_t istr = USB->ISTR;
        uint8_t  ep   = istr & 0x0F;
        uint16_t epr  = USB_EPR[ep];

        if (epr & (1u << 15)) // CTR_RX
        {
            // Clear CTR_RX
            USB_EPR[ep] = epr & 0x8F8F;

            if (ep == 0)
            {
                // Setup?
                if (epr & (1u << 11))
                {
                    readPma(0, _setup, 8);
                    handleSetup();
                }
                else
                {
                    // Out data on EP0 (rare for CDC)
                    setRxStatus(0, 3); // VALID
                }
            }
            else if (ep == 3) // Bulk OUT
            {
                // Data received – user will call read()
                setRxStatus(3, 3); // re-enable
            }
        }

        if (epr & (1u << 7)) // CTR_TX
        {
            USB_EPR[ep] = epr & 0x8F8F;

            if (ep == 0 && _deviceAddr)
            {
                USB->DADDR = _deviceAddr | USB_DADDR_EF;
                _deviceAddr = 0;
            }

            // Continue multi-packet TX if needed
            if (_cntTx > 0 && ep == 2)
            {
                usbWrite(2);
            }
        }
    }

    void handleSetup()
    {
        uint8_t bmRequestType = _setup[0];
        uint8_t bRequest      = _setup[1];
        uint16_t wValue       = _setup[2] | (_setup[3] << 8);
        uint16_t wLength      = _setup[6] | (_setup[7] << 8);

        uint8_t type = (bmRequestType >> 5) & 0x03;

        if (type == 0) // Standard
        {
            switch (bRequest)
            {
            case 5: // SET_ADDRESS
                _deviceAddr = wValue & 0x7F;
                sendZlp(0);
                break;

            case 6: // GET_DESCRIPTOR
            {
                uint8_t descType = _setup[3];
                const uint8_t* desc = nullptr;
                uint16_t len = 0;

                if (descType == 1) // DEVICE
                {
                    desc = deviceDescriptor;
                    len  = sizeof(deviceDescriptor);
                }
                else if (descType == 2) // CONFIG
                {
                    desc = configDescriptor;
                    len  = sizeof(configDescriptor);
                }
                else if (descType == 3) // STRING
                {
                    switch (_setup[2])
                    {
                    case 0: desc = stringLang;  len = sizeof(stringLang);  break;
                    case 1: desc = stringMfg;   len = sizeof(stringMfg);   break;
                    case 2: desc = stringProd;  len = sizeof(stringProd);  break;
                    case 3: desc = stringSN;    len = sizeof(stringSN);    break;
                    case 4: desc = stringCtrl;  len = sizeof(stringCtrl);  break;
                    case 5: desc = stringData;  len = sizeof(stringData);  break;
                    default: sendZlp(0); return;
                    }
                }
                else
                {
                    sendZlp(0);
                    return;
                }

                if (wLength < len) len = wLength;
                _ptrTx = const_cast<uint8_t*>(desc);
                _cntTx = len;
                usbWrite(0);
                break;
            }

            case 9: // SET_CONFIGURATION
                _configured = true;
                sendZlp(0);
                break;

            default:
                sendZlp(0);
                break;
            }
        }
        else if (type == 1) // Class (CDC)
        {
            if (bRequest == 0x22) // SET_CONTROL_LINE_STATE
            {
                _lineState = wValue;
                _cdcActive = 1;
                sendZlp(0);
            }
            else if (bRequest == 0x20) // SET_LINE_CODING
            {
                sendZlp(0);
            }
            else if (bRequest == 0x21) // GET_LINE_CODING
            {
                // Return default 115200 8N1
                static const uint8_t lc[7] = { 0x00, 0xC2, 0x01, 0x00, 0x00, 0x00, 0x08 };
                _ptrTx = const_cast<uint8_t*>(lc);
                _cntTx = 7;
                usbWrite(0);
            }
            else
            {
                sendZlp(0);
            }
        }
        else
        {
            sendZlp(0);
        }
    }

    // ===================== Low-level PMA access =====================
    void readPma(uint8_t ep, uint8_t* dst, uint16_t len)
    {
        volatile uint16_t* btable = reinterpret_cast<volatile uint16_t*>(USB_PMA_BASE);
        uint16_t addr = btable[ep * 4 + 2]; // ADDR_RX
        volatile uint16_t* pma = reinterpret_cast<volatile uint16_t*>(USB_PMA_BASE + addr * 2);

        for (uint16_t i = 0; i < (len + 1) / 2; ++i)
        {
            uint16_t w = *pma++;
            *dst++ = w & 0xFF;
            if (--len == 0) break;
            *dst++ = w >> 8;
        }
    }

    void writePma(uint8_t ep, const uint8_t* src, uint16_t len)
    {
        volatile uint16_t* btable = reinterpret_cast<volatile uint16_t*>(USB_PMA_BASE);
        uint16_t addr = btable[ep * 4 + 0]; // ADDR_TX
        volatile uint16_t* pma = reinterpret_cast<volatile uint16_t*>(USB_PMA_BASE + addr * 2);

        btable[ep * 4 + 1] = len; // COUNT_TX

        for (uint16_t i = 0; i < (len + 1) / 2; ++i)
        {
            uint16_t w = *src++;
            if (len > 1) { w |= (*src++) << 8; len -= 2; }
            else           { len = 0; }
            *pma++ = w;
        }
    }

    void setTxStatus(uint8_t ep, uint16_t stat)
    {
        uint16_t epr = USB_EPR[ep];
        USB_EPR[ep] = (epr ^ (stat << 4)) & 0x8FBF;
    }

    void setRxStatus(uint8_t ep, uint16_t stat)
    {
        uint16_t epr = USB_EPR[ep];
        USB_EPR[ep] = (epr ^ (stat << 12)) & 0xBF8F;
    }

    void sendZlp(uint8_t ep)
    {
        _cntTx = 0;
        writePma(ep, nullptr, 0);
        setTxStatus(ep, 3); // VALID
    }

    // ===================== Transfer helpers (from assembly) =====================
    void usbWrite(uint8_t ep)
    {
        uint16_t maxPacket = (ep == 0) ? EP0_SIZE : EP2_SIZE;
        uint16_t toSend = (_cntTx > maxPacket) ? maxPacket : _cntTx;

        writePma(ep, _ptrTx, toSend);
        _ptrTx += toSend;
        _cntTx -= toSend;

        setTxStatus(ep, 3); // VALID
    }

    void usbRead(uint8_t ep, uint8_t* dst, uint16_t maxLen)
    {
        volatile uint16_t* btable = reinterpret_cast<volatile uint16_t*>(USB_PMA_BASE);
        uint16_t count = btable[ep * 4 + 3] & 0x3FF;

        if (count > maxLen) count = maxLen;
        readPma(ep, dst, count);
        _cntRx = count;

        setRxStatus(ep, 3); // VALID again
    }

    // Blocking serial API (mirrors assembly serial_write / serial_read)
    void serialWrite(const uint8_t* data, size_t len)
    {
        _ptrTx = const_cast<uint8_t*>(data);
        _cntTx = static_cast<uint16_t>(len);

        // Number of packets (including possible ZLP)
        size_t packets = (len + EP2_SIZE - 1) / EP2_SIZE;
        if (len % EP2_SIZE == 0) packets++; // ZLP

        for (size_t i = 0; i < packets; ++i)
        {
            // Wait for previous TX complete
            uint32_t timeout = 0x100000;
            while (!(USB_EPR[2] & (1u << 7)) && --timeout);

            // Clear CTR_TX
            USB_EPR[2] = USB_EPR[2] & 0x8F8F;

            usbWrite(2);
        }
    }

    void serialRead(uint8_t* data, size_t len)
    {
        size_t received = 0;

        while (received < len)
        {
            // Wait for data
            uint32_t timeout = 0x100000;
            while (!(USB_EPR[3] & (1u << 15)) && --timeout);

            if (timeout == 0) break;

            // Clear CTR_RX
            USB_EPR[3] = USB_EPR[3] & 0x8F8F;

            uint16_t chunk = static_cast<uint16_t>(len - received);
            if (chunk > EP2_SIZE) chunk = EP2_SIZE;

            usbRead(3, data + received, chunk);
            received += _cntRx;

            if (_cntRx < EP2_SIZE) break; // short packet → end of transfer
        }
    }
};

}

// ============================================================
// In your interrupt file:
//
// extern UsbCdc g_usbCdc;   // or however you instantiate it
//
// extern "C" void USB_LP_CAN1_RX0_IRQHandler(void)
// {
//     g_usbCdc.interrupt();
// }
// ============================================================
