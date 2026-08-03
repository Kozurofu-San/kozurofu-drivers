#pragma once

#include <cstdint>
#include <cstddef>

namespace driver
{

class IUsbController
{
    public:

        enum class EpType
        {
            Control,
            Bulk,
            Interrupt,
            Isochronous
        };

        enum class Speed : uint8_t
        {
            Low,        // 1.5 Mbit/s
            Full,       // 12 Mbit/s
            High,       // 480 Mbit/s
        };

        virtual ~IUsbController() = default;

        // Address
        virtual void setAddress(uint8_t addr) = 0;

        // EPs, ep[7] - direction: 0 OUT, 1 IN
        virtual void openEndpoint(uint8_t ep, EpType type, uint16_t maxPacket, uint8_t interval = 0) = 0;
        virtual void closeEndpoint(uint8_t ep) = 0;
        virtual void stallEndpoint(uint8_t ep, bool stall) = 0;
        virtual bool isEndpointStalled(uint8_t ep) const = 0;
        virtual void flushEndpoint(uint8_t ep) = 0;

        // Data transmission (FIFO / DMA)
        virtual size_t writePacket(uint8_t ep, const uint8_t *data, size_t len) = 0;
        virtual size_t readPacket(uint8_t ep, uint8_t *data, size_t maxLen) = 0;
        virtual void setRxBuffer(uint8_t ep, uint8_t *buf, size_t size) = 0;    // For double-buffer / DMA
        virtual bool isTxComplete(uint8_t ep) const = 0;
        virtual bool isRxReady(uint8_t ep) const = 0;

        // Bus / device state
        virtual bool    isConnected() const = 0;
        virtual bool    isSuspended() const = 0;
        virtual uint32_t getFrameNumber() const = 0;

        // Interrupts
        virtual void enableInterrupts(uint32_t mask) = 0;
        virtual uint32_t getAndClearIrqFlags() = 0;
        virtual void handleInterrupt() = 0;     // Is calles from ISR

        virtual Speed getSpeed() const = 0;  // MHz
        virtual bool isInit() = 0;
};

}