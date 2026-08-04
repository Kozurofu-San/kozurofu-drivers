#pragma once

#include "device/UsbDeviceCore.h"          // SetupPacket, DeviceState и т.д.

#include <cstdint>
#include <cstddef>

namespace driver
{

class IUsbClass
{
public:

    struct SetupPacket
    {
        uint8_t  bmRequestType;
        uint8_t  bRequest;
        uint16_t wValue;
        uint16_t wIndex;
        uint16_t wLength;

        // Helpers
        bool isHostToDevice() const { return (bmRequestType & 0x80) == 0; }
        bool isDeviceToHost() const { return (bmRequestType & 0x80) != 0; }
        uint8_t recipient()   const { return bmRequestType & 0x1F; }   // 0=device, 1=interface, 2=endpoint
        uint8_t type()        const { return (bmRequestType >> 5) & 0x03; } // 0=standard, 1=class, 2=vendor
    };

    virtual ~IUsbClass() = default;

    class UsbDeviceCore;
    /**
     * Called once when the class is registered in the device.
     * Here the class opens its endpoints and stores the device pointer.
     */
    virtual void init(UsbDeviceCore& device) = 0;

    /**
     * Called when the device is de-configured or stopped.
     */
    virtual void deinit() = 0;

    /**
     * Called on SET_CONFIGURATION (config != 0).
     * Class should prepare its endpoints / state.
     */
    virtual void onConfigured(uint8_t configuration) = 0;

    /**
     * Called on SET_CONFIGURATION(0) or USB reset.
     */
    virtual void onDeconfigured() = 0;

    /**
     * Handle class-specific or vendor-specific SETUP request.
     * Return true if the request was handled, false otherwise
     * (core will STALL EP0).
     */
    virtual bool onSetup(const SetupPacket& setup) = 0;

    /**
     * Periodic processing (call from main loop / task).
     * Used for state machines, timeouts, buffering etc.
     */
    virtual void process() = 0;

    /**
     * Optional: return number of interfaces this class occupies.
     * Useful for composite devices when building configuration descriptor.
     */
    virtual uint8_t getInterfaceCount() const { return 1; }
};

} // namespace driver