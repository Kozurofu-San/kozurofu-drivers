#pragma once

#include "interface/UsbController.h"
#include "stm32f1xx.h"

#include <cstdint>
#include <cstring>

extern uint32_t SystemCoreClock;

namespace driver
{

/**
 * Low-level USB Device driver for STM32F103 (USB FS Device peripheral).
 * Single-header implementation. Uses the 512-byte Packet Memory Area (PMA).
 *
 * Notes:
 *  - Endpoint address format: bit 7 = direction (0 = OUT, 1 = IN), bits 0-3 = endpoint number.
 *  - Only Full-Speed is supported.
 *  - No DMA (F103 USB does not have it).
 *  - Buffer table is placed at the beginning of PMA (BTABLE = 0).
 */
class UsbDriver : public IUsbController
{
public:
    explicit UsbDriver(USB_TypeDef* usb)
        : _usb(usb)
    {
    }

    // -------------------------------------------------------------------------
    // Initialization (called by user before start)
    // -------------------------------------------------------------------------

    bool init()
    {
        // USB clock = PLLCLK / 1.5 → must be 48 MHz when SYSCLK = 72 MHz
        RCC->CFGR &= ~RCC_CFGR_USBPRE;          // USBPRE = 0 → divide by 1.5

        // Enable clocks
        RCC->APB1ENR |= RCC_APB1ENR_USBEN;
        RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_AFIOEN;

        // PA11 = USB_DM, PA12 = USB_DP.  The USB peripheral drives these
        // pins; GPIO must not actively drive either line.
        GPIOA->CRH &= ~((0xFU << 12) | (0xFU << 16));
        GPIOA->CRH |=  (0x4U << 12) | (0x4U << 16); // input floating

        // Force USB reset
        _usb->CNTR = USB_CNTR_FRES;
        for (volatile int i = 0; i < 50; ) { i = i + 1; /* small delay */ }
        _usb->CNTR = 0;                         // Clear FRES

        // Clear pending interrupts
        _usb->ISTR = 0;

        // Set BTABLE to start of PMA (0)
        _usb->BTABLE = 0;

        // Initialize internal state
        _pmaFreeOffset = 64;                    // first 64 bytes reserved for BTABLE (8 EPs * 8 bytes)
        _speed         = Speed::Full;
        _address       = 0;
        _isInit        = true;
        _usb->DADDR   = USB_DADDR_EF;

        // Clear all endpoint registers and PMA allocation tracking
        for (int i = 0; i < NumEndpoints; ++i)
        {
            _epInfo[i] = EpInfo{};
            _rxReady[i] = false;
            EPnR(i) = static_cast<uint16_t>(i);
        }

        return true;
    }

    // -------------------------------------------------------------------------
    // IUsbController interface
    // -------------------------------------------------------------------------

    void setAddress(uint8_t addr) override
    {
        _address = addr & 0x7F;
        _usb->DADDR = USB_DADDR_EF | _address;  // enable function + address
    }

    void openEndpoint(uint8_t epAddr, EpType type, uint16_t maxPacket, uint8_t interval = 0) override
    {
        (void)interval;                         // not used on F103 device peripheral

        const uint8_t epNum = epAddr & 0x0F;
        if (epNum >= NumEndpoints)
            return;

        const bool isIn = (epAddr & 0x80) != 0;
        EpInfo& info = _epInfo[epNum];

        if ((isIn && info.inUsed) || (!isIn && info.outUsed))
        {
            if (isIn)
            {
                info.txMaxPacket = maxPacket;
                setEpType(epNum, type);
                setTxCount(epNum, 0);
                setTxStatus(epNum, EpStatus::Nak);
            }
            else
            {
                info.rxMaxPacket = maxPacket;
                setEpType(epNum, type);
                setRxCount(epNum, maxPacket);
                setRxStatus(epNum, EpStatus::Valid);
            }
            return;
        }

        // Allocate PMA buffer (word-aligned)
        const uint16_t allocSize = (maxPacket + 1u) & ~1u;   // round up to even
        if (_pmaFreeOffset + allocSize > PmaSize)
            return;                             // out of PMA memory

        const uint16_t pmaOffset = _pmaFreeOffset;
        _pmaFreeOffset += allocSize;

        if (isIn)
        {
            info.txOffset   = pmaOffset;
            info.txMaxPacket = maxPacket;
            info.type       = type;
            info.inUsed     = true;

            // Configure TX part of EPnR
            setEpType(epNum, type);
            setTxStatus(epNum, EpStatus::Nak);  // initially NAK
            setTxCount(epNum, 0);
            setTxAddr(epNum, pmaOffset);
        }
        else
        {
            info.rxOffset   = pmaOffset;
            info.rxMaxPacket = maxPacket;
            info.type       = type;
            info.outUsed    = true;

            setEpType(epNum, type);
            setRxStatus(epNum, EpStatus::Valid);
            setRxCount(epNum, maxPacket);
            setRxAddr(epNum, pmaOffset);
        }

        // Clear CTR flags for this endpoint
        clearCtr(epNum, isIn);
    }

    void closeEndpoint(uint8_t epAddr) override
    {
        const uint8_t epNum = epAddr & 0x0F;
        if (epNum >= NumEndpoints)
            return;

        const bool isIn = (epAddr & 0x80) != 0;
        EpInfo& info = _epInfo[epNum];

        if (isIn)
        {
            setTxStatus(epNum, EpStatus::Disabled);
            info.inUsed = false;
        }
        else
        {
            setRxStatus(epNum, EpStatus::Disabled);
            info.outUsed = false;
        }
    }

    void stallEndpoint(uint8_t epAddr, bool stall) override
    {
        const uint8_t epNum = epAddr & 0x0F;
        if (epNum >= NumEndpoints)
            return;

        const bool isIn = (epAddr & 0x80) != 0;

        if (stall)
        {
            if (isIn)
                setTxStatus(epNum, EpStatus::Stall);
            else
                setRxStatus(epNum, EpStatus::Stall);
        }
        else
        {
            // Clear stall and return to NAK/Valid
            if (isIn)
            {
                // Toggle DATA0/1 is handled by hardware on CLEAR_FEATURE usually
                setTxStatus(epNum, EpStatus::Nak);
            }
            else
            {
                setRxStatus(epNum, EpStatus::Valid);
                setRxCount(epNum, _epInfo[epNum].rxMaxPacket);
            }
        }
    }

    bool isEndpointStalled(uint8_t epAddr) const override
    {
        const uint8_t epNum = epAddr & 0x0F;
        if (epNum >= NumEndpoints)
            return false;

        const bool isIn = (epAddr & 0x80) != 0;
        const uint16_t epr = EPnR(epNum);

        if (isIn)
            return ((epr & USB_EPTX_STAT) >> 4) == static_cast<uint16_t>(EpStatus::Stall);
        else
            return ((epr & USB_EPRX_STAT) >> 12) == static_cast<uint16_t>(EpStatus::Stall);
    }

    void flushEndpoint(uint8_t epAddr) override
    {
        const uint8_t epNum = epAddr & 0x0F;
        if (epNum >= NumEndpoints)
            return;

        const bool isIn = (epAddr & 0x80) != 0;

        if (isIn)
        {
            setTxCount(epNum, 0);
            setTxStatus(epNum, EpStatus::Nak);
        }
        else
        {
            setRxCount(epNum, _epInfo[epNum].rxMaxPacket);
            setRxStatus(epNum, EpStatus::Valid);
        }

        clearCtr(epNum, isIn);
    }

    size_t writePacket(uint8_t epAddr, const uint8_t* data, size_t len) override
    {
        const uint8_t epNum = epAddr & 0x0F;
        if (epNum >= NumEndpoints || !(epAddr & 0x80))
            return 0;

        EpInfo& info = _epInfo[epNum];
        if (!info.inUsed)
            return 0;

        // Limit to max packet size
        if (len > info.txMaxPacket)
            len = info.txMaxPacket;

        // Copy user buffer → PMA
        writePma(info.txOffset, data, len);
        setTxCount(epNum, static_cast<uint16_t>(len));
        setTxStatus(epNum, EpStatus::Valid);

        return len;
    }

    size_t readPacket(uint8_t epAddr, uint8_t* data, size_t maxLen) override
    {
        const uint8_t epNum = epAddr & 0x0F;
        if (epNum >= NumEndpoints || (epAddr & 0x80))
            return 0;

        EpInfo& info = _epInfo[epNum];
        if (!info.outUsed)
            return 0;

        // Read actual received count from BTABLE
        const uint16_t rxCount = getRxCount(epNum);
        size_t toRead = rxCount;
        if (toRead > maxLen)
            toRead = maxLen;

        // Copy PMA → user buffer
        readPma(info.rxOffset, data, toRead);

        // Re-arm RX
        _rxReady[epNum] = false;
        setRxCount(epNum, info.rxMaxPacket);
        setRxStatus(epNum, EpStatus::Valid);
        clearCtr(epNum, false);

        return toRead;
    }

    void setRxBuffer(uint8_t epAddr, uint8_t* buf, size_t size) override
    {
        // F103 has no DMA – this is a no-op.
        // Kept for interface compatibility.
        (void)epAddr;
        (void)buf;
        (void)size;
    }

    bool isTxComplete(uint8_t epAddr) const override
    {
        const uint8_t epNum = epAddr & 0x0F;
        if (epNum >= NumEndpoints || !(epAddr & 0x80))
            return false;

        // CTR_TX is cleared by software after handling; we also check STAT_TX
        const uint16_t epr = EPnR(epNum);
        const uint16_t stat = (epr & USB_EPTX_STAT) >> 4;
        return (stat == static_cast<uint16_t>(EpStatus::Nak)) ||
               (stat == static_cast<uint16_t>(EpStatus::Disabled));
    }

    bool isRxReady(uint8_t epAddr) const override
    {
        const uint8_t epNum = epAddr & 0x0F;
        if (epNum >= NumEndpoints || (epAddr & 0x80))
            return false;

        // CTR_RX is acknowledged in the ISR for non-control endpoints;
        // retain the event in software until the class reads the packet.
        return _rxReady[epNum] || ((EPnR(epNum) & USB_EP_CTR_RX) != 0);
    }

    bool isConnected() const override
    {
        // F103 USB has no dedicated VBUS pin sensing in the peripheral.
        // Return true when device is enabled.
        return (_usb->DADDR & USB_DADDR_EF) != 0;
    }

    bool isSuspended() const override
    {
        return (_usb->ISTR & USB_ISTR_SUSP) != 0;
    }

    uint32_t getFrameNumber() const override
    {
        return _usb->FNR & USB_FNR_FN;
    }

    void enableInterrupts(uint32_t mask) override
    {
        // Map abstract mask bits to CNTR interrupt enable bits
        uint16_t cntr = _usb->CNTR & ~(USB_CNTR_CTRM | USB_CNTR_RESETM |
                                       USB_CNTR_SUSPM | USB_CNTR_WKUPM |
                                       USB_CNTR_SOFM  | USB_CNTR_ESOFM |
                                       USB_CNTR_ERRM  | USB_CNTR_PMAOVRM);

        // Simple mapping (adapt if your higher layer uses different bits)
        if (mask & 0x01) cntr |= USB_CNTR_RESETM;   // Reset
        if (mask & 0x02) cntr |= USB_CNTR_SUSPM;    // Suspend
        if (mask & 0x04) cntr |= USB_CNTR_WKUPM;    // Wakeup / Resume
        if (mask & 0x08) cntr |= USB_CNTR_SOFM;     // SOF
        if (mask & 0x10) cntr |= USB_CNTR_CTRM;     // Correct Transfer (EP activity)

        cntr |= USB_CNTR_CTRM;                      // always useful
        _usb->CNTR = cntr;
    }

    uint32_t getAndClearIrqFlags() override
    {
        const uint16_t istr = _usb->ISTR;
        uint32_t flags = 0;

        if (istr & USB_ISTR_RESET)  flags |= 0x01;
        if (istr & USB_ISTR_SUSP)   flags |= 0x02;
        if (istr & USB_ISTR_WKUP)   flags |= 0x04;
        if (istr & USB_ISTR_SOF)    flags |= 0x08;
        if (istr & USB_ISTR_CTR)
        {
            const uint8_t epNum = istr & USB_ISTR_EP_ID;
            if (epNum == 0)
            {
                if ((istr & USB_ISTR_DIR) == 0)
                {
                    flags |= 0x20;               // EP0 IN complete
                    clearCtr(0, true);
                }
                else if (EPnR(0) & USB_EP_SETUP)
                    flags |= 0x10;               // SETUP received on EP0
                else
                    flags |= 0x40;               // EP0 OUT complete
            }
            else
            {
                const bool isIn = (istr & USB_ISTR_DIR) == 0;
                if (!isIn)
                    _rxReady[epNum] = true;
                clearCtr(epNum, isIn);
            }
        }

        if (istr & USB_ISTR_RESET)
        {
            for (auto& ready : _rxReady)
                ready = false;
        }

        // Clear the flags we report (write 0 to clear)
        _usb->ISTR = static_cast<uint16_t>(~(istr & (USB_ISTR_RESET | USB_ISTR_SUSP |
                                                     USB_ISTR_WKUP  | USB_ISTR_SOF)));

        return flags;
    }

    void handleInterrupt() override
    {
        // Minimal handling: clear CTR bits for endpoints so that
        // higher layer can call isRxReady / isTxComplete / readPacket / writePacket.
        // More advanced processing can be done in UsbDeviceCore::onInterrupt().

        while (_usb->ISTR & USB_ISTR_CTR)
        {
            const uint8_t epNum = _usb->ISTR & USB_ISTR_EP_ID;
            const bool    isIn  = (_usb->ISTR & USB_ISTR_DIR) == 0; // 0 = IN (TX)

            // Clear the CTR flag (toggle by writing 0 while keeping other bits)
            clearCtr(epNum, isIn);

            // Higher layer will react via isRxReady / isTxComplete
            break;  // process one per call; or loop if desired
        }
    }

    Speed getSpeed() const override
    {
        return _speed;
    }

    bool isInit() override
    {
        return _isInit;
    }

    USB_TypeDef* getInstance()
    {
        return _usb;
    }

    void debugPrint() const
    {
        printf("USB regs: ISTR=%04X CNTR=%04X DADDR=%04X FNR=%04X EP0R=%04X EP1R=%04X BTABLE=%04X\n",
               _usb->ISTR, _usb->CNTR, _usb->DADDR, _usb->FNR,
               EPnR(0), EPnR(1), _usb->BTABLE);
    }

private:
    // -------------------------------------------------------------------------
    // Constants & types
    // -------------------------------------------------------------------------

    static constexpr int      NumEndpoints = 8;
    static constexpr uint16_t PmaSize      = 512;

    enum class EpStatus : uint16_t
    {
        Disabled = 0,
        Stall    = 1,
        Nak      = 2,
        Valid    = 3
    };

    struct EpInfo
    {
        uint16_t txOffset    = 0;
        uint16_t rxOffset    = 0;
        uint16_t txMaxPacket = 0;
        uint16_t rxMaxPacket = 0;
        EpType   type        = EpType::Control;
        bool     inUsed      = false;
        bool     outUsed     = false;
    };

    // -------------------------------------------------------------------------
    // PMA helpers (Packet Memory Area is 16-bit addressed)
    // -------------------------------------------------------------------------

    static volatile uint16_t* pmaPtr(uint16_t offset)
    {
        // PMA is mapped at USB_BASE + 0x400, accessed as 16-bit words
        return reinterpret_cast<volatile uint16_t*>(USB_BASE + 0x400u + (offset * 2u));
    }

    void writePma(uint16_t offset, const uint8_t* data, size_t len)
    {
        size_t i = 0;
        while (i + 1 < len)
        {
            uint16_t word = data[i] | (static_cast<uint16_t>(data[i + 1]) << 8);
            *pmaPtr(static_cast<uint16_t>(offset + i)) = word;
            i += 2;
        }
        if (i < len)
        {
            *pmaPtr(static_cast<uint16_t>(offset + i)) = data[i];
        }
    }

    void readPma(uint16_t offset, uint8_t* data, size_t len)
    {
        size_t i = 0;
        while (i + 1 < len)
        {
            uint16_t word = *pmaPtr(static_cast<uint16_t>(offset + i));
            data[i]     = static_cast<uint8_t>(word & 0xFF);
            data[i + 1] = static_cast<uint8_t>(word >> 8);
            i += 2;
        }
        if (i < len)
        {
            data[i] = static_cast<uint8_t>(*pmaPtr(static_cast<uint16_t>(offset + i)) & 0xFF);
        }
    }

    // -------------------------------------------------------------------------
    // Endpoint register helpers
    // -------------------------------------------------------------------------

    void setEpType(uint8_t epNum, EpType type)
    {
        const uint16_t epr = EPnR(epNum);
        uint16_t typeBits = USB_EP_CONTROL;
        switch (type)
        {
        case EpType::Control:     typeBits = USB_EP_CONTROL;     break;
        case EpType::Bulk:        typeBits = USB_EP_BULK;        break;
        case EpType::Interrupt:   typeBits = USB_EP_INTERRUPT;   break;
        case EpType::Isochronous: typeBits = USB_EP_ISOCHRONOUS; break;
        }

        EPnR(epNum) = (epr & (USB_EP_CTR_RX | USB_EP_CTR_TX |
                              USB_EP_KIND | USB_EPADDR_FIELD)) | typeBits;
    }

    void setTxStatus(uint8_t epNum, EpStatus status)
    {
        uint16_t epr = EPnR(epNum);
        // Toggle bits that differ from desired status
        const uint16_t current = (epr & USB_EPTX_STAT) >> 4;
        const uint16_t desired = static_cast<uint16_t>(status);
        uint16_t toggle = 0;
        if ((current ^ desired) & 1) toggle |= USB_EPTX_DTOG1;
        if ((current ^ desired) & 2) toggle |= USB_EPTX_DTOG2;

        EPnR(epNum) = (epr & (USB_EP_CTR_RX | USB_EP_CTR_TX |
                              USB_EP_T_FIELD | USB_EP_KIND | USB_EPADDR_FIELD)) | toggle;
    }

    void setRxStatus(uint8_t epNum, EpStatus status)
    {
        uint16_t epr = EPnR(epNum);
        const uint16_t current = (epr & USB_EPRX_STAT) >> 12;
        const uint16_t desired = static_cast<uint16_t>(status);
        uint16_t toggle = 0;
        if ((current ^ desired) & 1) toggle |= USB_EPRX_DTOG1;
        if ((current ^ desired) & 2) toggle |= USB_EPRX_DTOG2;

        EPnR(epNum) = (epr & (USB_EP_CTR_RX | USB_EP_CTR_TX |
                              USB_EP_T_FIELD | USB_EP_KIND | USB_EPADDR_FIELD)) | toggle;
    }

    void setTxAddr(uint8_t epNum, uint16_t addr)
    {
        // BTABLE entry: TXADDR at offset (epNum * 8)
        *pmaPtr(epNum * 8 + 0) = addr;
    }

    void setRxAddr(uint8_t epNum, uint16_t addr)
    {
        *pmaPtr(epNum * 8 + 4) = addr;
    }

    void setTxCount(uint8_t epNum, uint16_t count)
    {
        *pmaPtr(epNum * 8 + 2) = count;
    }

    void setRxCount(uint8_t epNum, uint16_t maxPacket)
    {
        // BLSIZE + NUM_BLOCK encoding
        uint16_t blocks;
        if (maxPacket > 62)
        {
            blocks = (maxPacket + 31) / 32;
            blocks = (blocks << 10) | 0x8000;   // BL_SIZE = 1
        }
        else
        {
            blocks = (maxPacket + 1) / 2;
            blocks = blocks << 10;              // BL_SIZE = 0
        }
        *pmaPtr(epNum * 8 + 6) = blocks;
    }

    uint16_t getRxCount(uint8_t epNum) const
    {
        return *pmaPtr(epNum * 8 + 6) & 0x3FF;
    }

    void clearCtr(uint8_t epNum, bool isIn)
    {
        uint16_t epr = EPnR(epNum);
        uint16_t ctr = epr & (USB_EP_CTR_RX | USB_EP_CTR_TX);
        if (isIn)
            ctr &= ~USB_EP_CTR_TX;
        else
            ctr &= ~USB_EP_CTR_RX;

        EPnR(epNum) = (epr & (USB_EP_T_FIELD | USB_EP_KIND | USB_EPADDR_FIELD)) | ctr;
    }

    static inline volatile uint16_t &EPnR(uint8_t ep)
    {
        return *reinterpret_cast<volatile uint16_t *>(USB_BASE + ep * 4);
    }

    // -------------------------------------------------------------------------
    // Members
    // -------------------------------------------------------------------------

    USB_TypeDef* _usb;
    Speed        _speed      = Speed::Full;
    uint8_t      _address    = 0;
    bool         _isInit     = false;
    uint16_t     _pmaFreeOffset = 0;

    EpInfo       _epInfo[NumEndpoints];
    volatile bool _rxReady[NumEndpoints] = {};
    
};

} // namespace driver
