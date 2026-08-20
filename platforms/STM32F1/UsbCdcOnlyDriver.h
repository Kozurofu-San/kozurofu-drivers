#pragma once

#include "interface/Uart.h"

#include "stm32f1xx.h"

#include <cstdint>
#include <cstddef>

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
        if (!_configured || _txBusy || len == 0 || data == nullptr)
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

        _speed = 12'000'000U;
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

    void setCallback(void (*cb)(uint32_t)) override
    {
        _cb = cb;
    }

    void setBuffer(uint8_t *buffer) override
    {
        _buffer = buffer;
    }
    
    uint32_t getSpeed() const override
    {
        return _speed;   // USB Full-Speed
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
    static constexpr uint16_t EP0_SIZE     = 64;
    static constexpr uint16_t EP1_SIZE     = 8;
    static constexpr uint16_t EP2_SIZE     = 64;   // Bulk IN/OUT
    static constexpr uint16_t PMA_SIZE     = 512;

    // ===================== Hardware =====================
    volatile uint32_t* const USB_EPR    = reinterpret_cast<volatile uint32_t*>(USB_BASE);

    // ===================== State =====================
    bool     _initialized = false;
    bool     _configured  = false;
    uint8_t  _deviceAddr  = 0;
    uint8_t  _lineState   = 0;
    bool     _txBusy      = false;
    bool     _sendZlp     = false;

    void (*_cb)(uint32_t) = nullptr;
    uint8_t *_buffer = nullptr;
    uint32_t _speed;

    // BTABLE addresses are byte offsets. Four endpoint descriptors occupy
    // the first 32 bytes of the PMA address space.
    static constexpr uint16_t PMA_BEGIN = 32;

    static_assert(PMA_BEGIN + EP0_SIZE * 2 + EP1_SIZE + EP2_SIZE * 4 <= PMA_SIZE,
                  "USB endpoint buffers exceed the STM32F1 512-byte PMA");

    const uint8_t* _txPtr[4]{};
    uint16_t _txCount[4]{};
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
        _lineState  = 0;
        _txBusy = false;
        _sendZlp = false;

        // Clear BTABLE
        USB->BTABLE = 0;

        // EP0 TX/RX
        btableWrite(0, 0, PMA_BEGIN);                 // ADDR_TX
        btableWrite(0, 1, 0);                         // COUNT_TX
        btableWrite(0, 2, PMA_BEGIN + EP0_SIZE);      // ADDR_RX
        btableWrite(0, 3, rxBufferCount(EP0_SIZE));   // COUNT_RX

        // EP1 is an IN-only interrupt endpoint.
        const uint16_t ep1Tx = PMA_BEGIN + EP0_SIZE * 2;
        btableWrite(1, 0, ep1Tx);
        btableWrite(1, 1, 0);
        btableWrite(1, 2, 0);
        btableWrite(1, 3, 0);

        if constexpr (DOUBLE_BUF)
        {
            // Double-buffered IN: buffer 0 uses TX fields, buffer 1 uses RX fields.
            const uint16_t ep2Buf0 = ep1Tx + EP1_SIZE;
            const uint16_t ep2Buf1 = ep2Buf0 + EP2_SIZE;
            btableWrite(2, 0, ep2Buf0);
            btableWrite(2, 1, 0);
            btableWrite(2, 2, ep2Buf1);
            btableWrite(2, 3, 0);

            // Double-buffered OUT uses the fields in the opposite order:
            // buffer 0 uses RX fields and buffer 1 uses TX fields.
            const uint16_t ep3Buf0 = ep2Buf1 + EP2_SIZE;
            const uint16_t ep3Buf1 = ep3Buf0 + EP2_SIZE;
            btableWrite(3, 0, ep3Buf1);
            btableWrite(3, 1, rxBufferCount(EP2_SIZE));
            btableWrite(3, 2, ep3Buf0);
            btableWrite(3, 3, rxBufferCount(EP2_SIZE));
        }
        else
        {
            // Single buffer version (simplified)
            const uint16_t ep2Tx = ep1Tx + EP1_SIZE;
            const uint16_t ep3Rx = ep2Tx + EP2_SIZE;
            btableWrite(2, 0, ep2Tx);
            btableWrite(2, 1, 0);
            btableWrite(2, 2, 0);
            btableWrite(2, 3, 0);
            btableWrite(3, 0, 0);
            btableWrite(3, 1, 0);
            btableWrite(3, 2, ep3Rx);
            btableWrite(3, 3, rxBufferCount(EP2_SIZE));
        }

        // Endpoint registers
        USB_EPR[0] = (0 << 0) | (1 << 9) | (3 << 12) | (2 << 4); // CONTROL, VALID RX, NAK TX
        USB_EPR[1] = (1 << 0) | (3 << 9) | (0 << 12) | (2 << 4); // INTERRUPT, NAK TX

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
            clearCtr(ep, false);

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
                // TODO: Fill the buffer and call _cb after it fills
            }
        }

        if (epr & (1u << 7)) // CTR_TX
        {
            clearCtr(ep, true);

            if (ep == 0 && _deviceAddr)
            {
                USB->DADDR = _deviceAddr | USB_DADDR_EF;
                _deviceAddr = 0;
            }

            if (ep == 0 && _txCount[0] > 0)
            {
                usbWrite(0);
            }
            else if (ep == 2)
            {
                if (_txCount[2] > 0)
                    usbWrite(2);
                else if (_sendZlp)
                {
                    _sendZlp = false;
                    sendZlp(2);
                }
                else
                    _txBusy = false;
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
                _txPtr[0] = desc;
                _txCount[0] = len;
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
                _txPtr[0] = lc;
                _txCount[0] = 7;
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
    static volatile uint16_t* pmaPtr(uint16_t address)
    {
        // Each 16-bit PMA word is exposed at a 32-bit APB address stride.
        return reinterpret_cast<volatile uint16_t*>(USB_PMAADDR + address * 2u);
    }

    static uint16_t rxBufferCount(uint16_t size)
    {
        // Buffers larger than 62 bytes are allocated in 32-byte blocks.
        return static_cast<uint16_t>(0x8000u | (((size + 31u) / 32u) << 10));
    }

    static void btableWrite(uint8_t ep, uint8_t field, uint16_t value)
    {
        *pmaPtr(static_cast<uint16_t>(ep * 8u + field * 2u)) = value;
    }

    static uint16_t btableRead(uint8_t ep, uint8_t field)
    {
        return *pmaPtr(static_cast<uint16_t>(ep * 8u + field * 2u));
    }

    void readPma(uint8_t ep, uint8_t* dst, uint16_t len)
    {
        const uint16_t addr = btableRead(ep, 2); // ADDR_RX
        readPmaAt(addr, dst, len);
    }

    void readPmaAt(uint16_t addr, uint8_t* dst, uint16_t len)
    {
        for (uint16_t i = 0; i < len; i += 2)
        {
            const uint16_t w = *pmaPtr(static_cast<uint16_t>(addr + i));
            *dst++ = w & 0xFF;
            if (i + 1 < len)
                *dst++ = w >> 8;
        }
    }

    bool writePma(uint8_t ep, const uint8_t* src, uint16_t len)
    {
        const uint16_t addr = btableRead(ep, 0); // ADDR_TX

        btableWrite(ep, 1, len); // COUNT_TX
        return writePmaAt(addr, src, len);
    }

    bool writePmaAt(uint16_t addr, const uint8_t* src, uint16_t len)
    {
        if (addr >= PMA_SIZE || len > PMA_SIZE - addr)
        {
            // Never access an invalid PMA address: it raises a bus fault.
            return false;
        }

        for (uint16_t i = 0; i < len; i += 2)
        {
            uint16_t w = src[i];
            if (i + 1 < len)
                w |= static_cast<uint16_t>(src[i + 1]) << 8;
            *pmaPtr(static_cast<uint16_t>(addr + i)) = w;
        }

        return true;
    }

    static uint16_t endpointControlBits(uint16_t epr)
    {
        return epr & (USB_EP_CTR_RX | USB_EP_CTR_TX | USB_EP_T_FIELD |
                      USB_EP_KIND | USB_EPADDR_FIELD);
    }

    void clearCtr(uint8_t ep, bool isTx)
    {
        const uint16_t epr = USB_EPR[ep];
        uint16_t ctr = epr & (USB_EP_CTR_RX | USB_EP_CTR_TX);
        ctr &= static_cast<uint16_t>(~(isTx ? USB_EP_CTR_TX : USB_EP_CTR_RX));

        // CTR flags clear on zero; all endpoint configuration bits are preserved.
        const uint16_t configuration = endpointControlBits(epr) &
            ~(USB_EP_CTR_RX | USB_EP_CTR_TX);
        USB_EPR[ep] = configuration | ctr;
    }

    void freeDoubleBuffer(uint8_t ep, bool isIn)
    {
        const uint16_t epr = USB_EPR[ep];
        const uint16_t swBuf = isIn ? (1u << 14) : (1u << 6);

        // Toggling SW_BUF returns the application buffer to the USB peripheral.
        USB_EPR[ep] = endpointControlBits(epr) | swBuf;
    }

    void setTxStatus(uint8_t ep, uint16_t stat)
    {
        const uint16_t epr = USB_EPR[ep];
        const uint16_t current = (epr & USB_EPTX_STAT) >> 4;
        const uint16_t toggle = (current ^ stat) << 4;

        USB_EPR[ep] = endpointControlBits(epr) | toggle;
    }

    void setRxStatus(uint8_t ep, uint16_t stat)
    {
        const uint16_t epr = USB_EPR[ep];
        const uint16_t current = (epr & USB_EPRX_STAT) >> 12;
        const uint16_t toggle = (current ^ stat) << 12;

        USB_EPR[ep] = endpointControlBits(epr) | toggle;
    }

    void sendZlp(uint8_t ep)
    {
        _txCount[ep] = 0;
        bool copied;

        if constexpr (DOUBLE_BUF)
        {
            if (ep == 2)
            {
                const bool buffer0 = (USB_EPR[ep] & (1u << 14)) != 0;
                const uint8_t addressField = buffer0 ? 0 : 2;
                const uint8_t countField = buffer0 ? 1 : 3;
                btableWrite(ep, countField, 0);
                copied = writePmaAt(btableRead(ep, addressField), nullptr, 0);
                if (copied)
                    freeDoubleBuffer(ep, true);
            }
            else
                copied = writePma(ep, nullptr, 0);
        }
        else
            copied = writePma(ep, nullptr, 0);

        if (!copied)
            return;
        setTxStatus(ep, 3); // VALID

        if (ep == 0)
        {
            // A control write completes with an IN ZLP. Re-arm EP0 RX for
            // the following setup packet after the USB peripheral NAKed it.
            setRxStatus(0, 3); // VALID
        }
    }

    void usbWrite(uint8_t ep)
    {
        uint16_t maxPacket = (ep == 0) ? EP0_SIZE : EP2_SIZE;
        uint16_t toSend = (_txCount[ep] > maxPacket) ? maxPacket : _txCount[ep];
        bool copied = false;

        if constexpr (DOUBLE_BUF)
        {
            if (ep == 2)
            {
                // For double-buffered IN, SW_BUF=1 selects buffer 0.
                const bool buffer0 = (USB_EPR[ep] & (1u << 14)) != 0;
                const uint8_t addressField = buffer0 ? 0 : 2;
                const uint8_t countField = buffer0 ? 1 : 3;

                btableWrite(ep, countField, toSend);
                copied = writePmaAt(btableRead(ep, addressField), _txPtr[ep], toSend);
                if (copied)
                    freeDoubleBuffer(ep, true);
            }
            else
            {
                copied = writePma(ep, _txPtr[ep], toSend);
            }
        }
        else
        {
            copied = writePma(ep, _txPtr[ep], toSend);
        }

        if (!copied)
        {
            if (ep == 2)
                _txBusy = false;
            return;
        }

        _txPtr[ep] += toSend;
        _txCount[ep] -= toSend;

        setTxStatus(ep, 3); // VALID

        if (ep == 0 && _txCount[0] == 0)
        {
            // A control read completes with an OUT status packet. The USB
            // peripheral NAKs RX after SETUP, so it must be re-armed here.
            setRxStatus(0, 3); // VALID
        }

    }

    void usbRead(uint8_t ep, uint8_t* dst, uint16_t maxLen)
    {
        uint16_t count;

        if constexpr (DOUBLE_BUF)
        {
            if (ep == 3)
            {
                // For double-buffered OUT, SW_BUF=1 selects buffer 0.
                const bool buffer0 = (USB_EPR[ep] & (1u << 6)) != 0;
                const uint8_t addressField = buffer0 ? 2 : 0;
                const uint8_t countField = buffer0 ? 3 : 1;
                count = btableRead(ep, countField) & 0x3FF;

                if (count > maxLen) count = maxLen;
                readPmaAt(btableRead(ep, addressField), dst, count);
                _cntRx = count;
                freeDoubleBuffer(ep, false);
                return;
            }
        }

        count = btableRead(ep, 3) & 0x3FF;

        if (count > maxLen) count = maxLen;
        readPma(ep, dst, count);
        _cntRx = count;

        setRxStatus(ep, 3); // VALID again
    }

    void serialWrite(const uint8_t* data, size_t len)
    {
        if (len > UINT16_MAX)
            len = UINT16_MAX;

        _txPtr[2] = data;
        _txCount[2] = static_cast<uint16_t>(len);
        _sendZlp = (len % EP2_SIZE) == 0;
        _txBusy = true;
        usbWrite(2);
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

            // Clear CTR_RX.
            clearCtr(3, false);

            uint16_t chunk = static_cast<uint16_t>(len - received);
            if (chunk > EP2_SIZE) chunk = EP2_SIZE;

            usbRead(3, data + received, chunk);
            received += _cntRx;

            if (_cntRx < EP2_SIZE) break; // short packet → end of transfer
        }
    }
};

}
