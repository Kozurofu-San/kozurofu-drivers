#pragma once

#include "interface/UsbController.h"
#include "interface/UsbClass.h"

#include <cstdint>
#include <cstddef>
#include <functional>

namespace driver
{
// -----------------------------------------------------------------------------
// Basic types
// -----------------------------------------------------------------------------

enum class DeviceState : uint8_t
{
    Default,
    Address,
    Configured,
    Suspended
};

// Standard request codes (USB 2.0 Table 9-4)
enum class StdRequest : uint8_t
{
    GetStatus        = 0,
    ClearFeature     = 1,
    SetFeature       = 3,
    SetAddress       = 5,
    GetDescriptor    = 6,
    SetDescriptor    = 7,
    GetConfiguration = 8,
    SetConfiguration = 9,
    GetInterface     = 10,
    SetInterface     = 11,
    SynchFrame       = 12
};

// Descriptor types
enum class DescType : uint8_t
{
    Device        = 1,
    Configuration = 2,
    String        = 3,
    Interface     = 4,
    Endpoint      = 5,
    DeviceQualifier = 6,
    OtherSpeedConfig = 7,
    InterfacePower = 8
};

// Feature selectors
enum class FeatureSelector : uint8_t
{
    EndpointHalt       = 0,
    DeviceRemoteWakeup = 1,
    TestMode           = 2
};

// Simple IRQ flag bits (adapt to your controller implementation)
namespace Irq
{
    constexpr uint32_t Reset     = 1u << 0;
    constexpr uint32_t Suspend   = 1u << 1;
    constexpr uint32_t Resume    = 1u << 2;
    constexpr uint32_t Sof       = 1u << 3;
    constexpr uint32_t Setup     = 1u << 4;   // SETUP received on EP0
    constexpr uint32_t Ep0In     = 1u << 5;
    constexpr uint32_t Ep0Out    = 1u << 6;
    // Add more as needed (EP1..EPn)
}

// -----------------------------------------------------------------------------
// Descriptor provider interface
// -----------------------------------------------------------------------------

class IDescriptorProvider
{
public:
    virtual ~IDescriptorProvider() = default;

    // Return pointer to descriptor data and its length.
    // Return nullptr / length=0 if not found.
    virtual const uint8_t* getDescriptor(uint8_t type, uint8_t index,
                                         uint16_t langId, size_t& length) = 0;
};

// -----------------------------------------------------------------------------
// UsbDeviceCore – header-only implementation
// -----------------------------------------------------------------------------

class UsbDeviceCore
{
public:
    using EventCallback = std::function<void(DeviceState)>;

    explicit UsbDeviceCore(driver::IUsbController& usb)
        : _usb(usb)
    {
    }

    void setDescriptorProvider(IDescriptorProvider& provider)
    {
        _descProvider = &provider;
    }

    void setEventCallback(EventCallback cb)
    {
        _eventCb = std::move(cb);
    }

    // Call after the controller itself has been initialized (clocks, pins, etc.)
    void start()
    {
        if (!_usb.isInit())
            return;

        // Open control endpoint 0 (bidirectional)
        // Max packet size will be updated after we know the speed
        _usb.openEndpoint(0x00, IUsbController::EpType::Control, 64, 0);    // EP0 OUT
        _usb.openEndpoint(0x80, IUsbController::EpType::Control, 64, 0);    // EP0 IN

        _state         = DeviceState::Default;
        _address       = 0;
        _configuration = 0;
        _ep0Stage      = Ep0Stage::Idle;

        // Enable common interrupts (adapt mask to your controller)
        _usb.enableInterrupts(Irq::Reset | Irq::Suspend | Irq::Resume |
                                Irq::Setup | Irq::Ep0In | Irq::Ep0Out);

        notifyStateChange();
    }

    void stop()
    {
        _usb.closeEndpoint(0x00);
        _usb.closeEndpoint(0x80);
        _state = DeviceState::Default;
        notifyStateChange();
    }

    // Call from main loop / dedicated task
    void process()
    {
        // Most work is done in onInterrupt(). 
        // Here you can add deferred processing if needed.
        for (size_t i = 0; i < _classCount; ++i)
            _classes[i]->process();
    }

    // Call from ISR after _usb.handleInterrupt()
    void onInterrupt()
    {
        const uint32_t flags = _usb.getAndClearIrqFlags();

        if (flags & Irq::Reset)
            handleReset();

        if (flags & Irq::Suspend)
            handleSuspend();

        if (flags & Irq::Resume)
            handleResume();

        if (flags & Irq::Setup)
            handleSetup();

        if ((flags & (Irq::Ep0In | Irq::Ep0Out)) && _ep0Stage != Ep0Stage::Idle)
            handleEp0Data();
    }

    // -------------------------------------------------------------------------
    // Public state accessors
    // -------------------------------------------------------------------------

    DeviceState getState()         const { return _state; }
    uint8_t     getAddress()       const { return _address; }
    uint8_t     getConfiguration() const { return _configuration; }

    bool openEndpoint(uint8_t ep, IUsbController::EpType type, uint16_t maxPacket, uint8_t interval = 0U)
    {
        _usb.openEndpoint(ep, type, maxPacket, interval);
        return true;
    }

    void closeEndpoint(uint8_t epAddr)
    {
        _usb.closeEndpoint(epAddr);
    }

    void registerClass(IUsbClass& cls)
    {
        if (_classCount < MaxClasses)
        {
            _classes[_classCount++] = &cls;
            cls.init(*this);
        }
    }

    IUsbController& controller() { return _usb; }

    
    // -------------------------------------------------------------------------
    // Control transfer helpers (for class / vendor requests)
    // -------------------------------------------------------------------------

    /**
     * Start EP0 Data IN stage with the given buffer.
     * Automatically handles short packets and ZLP when needed.
     * After data is sent, Status OUT is expected.
     */
    void startControlIn(const uint8_t* data, size_t len)
    {
        if (_ep0Stage != Ep0Stage::Idle && _ep0Stage != Ep0Stage::DataIn)
            return;

        // Limit to the length requested by host
        if (len > _setup.wLength)
            len = _setup.wLength;

        _ep0DataPtr = data;
        _ep0DataLen = len;
        _ep0DataPos = 0;

        // ZLP needed if len is multiple of MPS and host requested more
        const uint16_t mps = getEp0MaxPacketSize();
        _ep0Zlp = (len > 0) && (len % mps == 0) && (len < _setup.wLength);

        _ep0Stage = Ep0Stage::DataIn;
        sendNextDataIn();
    }

    using ControlOutCallback = std::function<void(const uint8_t* data, size_t len)>;

    /**
     * Prepare for EP0 Data OUT stage.
     * The provided buffer will be filled when the host sends data.
     * After the complete data stage, Status IN is sent automatically
     * and the optional callback is invoked.
     */
    void startControlOut(uint8_t* buffer, size_t maxLen, ControlOutCallback cb = nullptr)
    {
        if (_ep0Stage != Ep0Stage::Idle)
            return;

        _ep0OutBuf     = buffer;
        _ep0OutMaxLen  = maxLen;
        _ep0OutReceived = 0;
        _ep0OutCb      = std::move(cb);

        _ep0Stage = Ep0Stage::DataOut;

        // Arm EP0 OUT for the first packet
        // (actual read happens in handleEp0Data)
    }


private:
    // -------------------------------------------------------------------------
    // Internal state
    // -------------------------------------------------------------------------

    enum class Ep0Stage : uint8_t
    {
        Idle,
        DataIn,
        DataOut,
        StatusIn,
        StatusOut
    };

    driver::IUsbController&  _usb;
    IDescriptorProvider*     _descProvider = nullptr;
    EventCallback            _eventCb;

    DeviceState _state         = DeviceState::Default;
    uint8_t     _address       = 0;
    uint8_t     _configuration = 0;
    bool        _remoteWakeup  = false;
    bool        _selfPowered   = false;   // can be set from descriptor

    // EP0 control transfer state
    IUsbClass::SetupPacket     _setup{};
    Ep0Stage        _ep0Stage   = Ep0Stage::Idle;
    const uint8_t*  _ep0DataPtr = nullptr;
    size_t          _ep0DataLen = 0;
    size_t          _ep0DataPos = 0;
    bool            _ep0Zlp     = false;  // zero-length packet needed?

    static constexpr size_t MaxClasses = 4;
    IUsbClass* _classes[MaxClasses] = {};
    size_t     _classCount = 0;

    // New members for Control OUT
    uint8_t*            _ep0OutBuf      = nullptr;
    size_t              _ep0OutMaxLen   = 0;
    size_t              _ep0OutReceived = 0;
    ControlOutCallback  _ep0OutCb;

    // Helper
    uint16_t getEp0MaxPacketSize() const
    {
        // Full-speed default 8, can be 64 after enumeration.
        // You may store the real value when opening EP0.
        return (_usb.getSpeed() == driver::IUsbController::Speed::High) ? 64 : 8;
    }

    // -------------------------------------------------------------------------
    // Event notification
    // -------------------------------------------------------------------------

    void notifyStateChange()
    {
        if (_eventCb)
            _eventCb(_state);
    }

    // -------------------------------------------------------------------------
    // Bus events
    // -------------------------------------------------------------------------

    void handleReset()
    {
        _address       = 0;
        _configuration = 0;
        _state         = DeviceState::Default;
        _ep0Stage      = Ep0Stage::Idle;

        // Re-open EP0 with correct max packet size for current speed
        const auto speed = _usb.getSpeed();
        const uint16_t mps = (speed == driver::IUsbController::Speed::High) ? 64 : 8;

        _usb.openEndpoint(0x00, IUsbController::EpType::Control, mps, 0);
        _usb.openEndpoint(0x80, IUsbController::EpType::Control, mps, 0);

        _usb.setAddress(0);
        notifyStateChange();
    }

    void handleSuspend()
    {
        if (_state != DeviceState::Suspended)
        {
            _state = DeviceState::Suspended;
            notifyStateChange();
        }
    }

    void handleResume()
    {
        // Restore previous state (simplified – real stacks keep previous state)
        if (_configuration != 0)
            _state = DeviceState::Configured;
        else if (_address != 0)
            _state = DeviceState::Address;
        else
            _state = DeviceState::Default;

        notifyStateChange();
    }

    // -------------------------------------------------------------------------
    // SETUP packet handling
    // -------------------------------------------------------------------------

    void handleSetup()
    {
        uint8_t buf[8];
        const size_t got = _usb.readPacket(0x00, buf, 8);
        if (got != 8)
        {
            stallEp0();
            return;
        }

        // Parse SETUP packet (little-endian)
        _setup.bmRequestType = buf[0];
        _setup.bRequest      = buf[1];
        _setup.wValue        = static_cast<uint16_t>(buf[2] | (buf[3] << 8));
        _setup.wIndex        = static_cast<uint16_t>(buf[4] | (buf[5] << 8));
        _setup.wLength       = static_cast<uint16_t>(buf[6] | (buf[7] << 8));

        _ep0Stage   = Ep0Stage::Idle;
        _ep0DataPtr = nullptr;
        _ep0DataLen = 0;
        _ep0DataPos = 0;
        _ep0Zlp     = false;

        // Only standard requests are handled here.
        // Class/Vendor requests should be forwarded to registered classes later.
        if (_setup.type() == 0)
        {
            handleStandardRequest();
        }
        else
        {
            bool handled = false;
            for (size_t i = 0; i < _classCount; ++i)
            {
                if (_classes[i]->onSetup(_setup))
                {
                    handled = true;
                    break;
                }
            }
            if (!handled)
                stallEp0();
        }
    }

    // -------------------------------------------------------------------------
    // Standard request dispatcher
    // -------------------------------------------------------------------------

    void handleStandardRequest()
    {
        const auto req = static_cast<StdRequest>(_setup.bRequest);

        switch (req)
        {
        case StdRequest::GetStatus:
            handleGetStatus();
            break;

        case StdRequest::ClearFeature:
            handleClearFeature();
            break;

        case StdRequest::SetFeature:
            handleSetFeature();
            break;

        case StdRequest::SetAddress:
            handleSetAddress();
            break;

        case StdRequest::GetDescriptor:
            handleGetDescriptor();
            break;

        case StdRequest::GetConfiguration:
            handleGetConfiguration();
            break;

        case StdRequest::SetConfiguration:
            handleSetConfiguration();
            break;

        case StdRequest::GetInterface:
            handleGetInterface();
            break;

        case StdRequest::SetInterface:
            handleSetInterface();
            break;

        default:
            stallEp0();
            break;
        }
    }

    // -------------------------------------------------------------------------
    // Individual standard requests
    // -------------------------------------------------------------------------

    void handleGetStatus()
    {
        uint8_t status[2] = {0, 0};

        switch (_setup.recipient())
        {
        case 0: // Device
            if (_selfPowered)   status[0] |= 0x01;
            if (_remoteWakeup)  status[0] |= 0x02;
            break;

        case 1: // Interface
            // Always 0
            break;

        case 2: // Endpoint
        {
            const uint8_t epAddr = static_cast<uint8_t>(_setup.wIndex & 0xFF);
            if (_usb.isEndpointStalled(epAddr))
                status[0] = 0x01;
            break;
        }

        default:
            stallEp0();
            return;
        }

        startDataIn(status, 2);
    }

    void handleClearFeature()
    {
        const auto feature = static_cast<FeatureSelector>(_setup.wValue & 0xFF);

        switch (_setup.recipient())
        {
        case 0: // Device
            if (feature == FeatureSelector::DeviceRemoteWakeup)
            {
                _remoteWakeup = false;
                statusIn();
            }
            else
            {
                stallEp0();
            }
            break;

        case 2: // Endpoint
            if (feature == FeatureSelector::EndpointHalt)
            {
                const uint8_t epAddr = static_cast<uint8_t>(_setup.wIndex & 0xFF);
                _usb.stallEndpoint(epAddr, false);
                _usb.flushEndpoint(epAddr);
                statusIn();
            }
            else
            {
                stallEp0();
            }
            break;

        default:
            stallEp0();
            break;
        }
    }

    void handleSetFeature()
    {
        const auto feature = static_cast<FeatureSelector>(_setup.wValue & 0xFF);

        switch (_setup.recipient())
        {
        case 0: // Device
            if (feature == FeatureSelector::DeviceRemoteWakeup)
            {
                _remoteWakeup = true;
                statusIn();
            }
            else
            {
                stallEp0();
            }
            break;

        case 2: // Endpoint
            if (feature == FeatureSelector::EndpointHalt)
            {
                const uint8_t epAddr = static_cast<uint8_t>(_setup.wIndex & 0xFF);
                _usb.stallEndpoint(epAddr, true);
                statusIn();
            }
            else
            {
                stallEp0();
            }
            break;

        default:
            stallEp0();
            break;
        }
    }

    void handleSetAddress()
    {
        // Address is applied AFTER the status stage (USB 2.0 9.4.6)
        const uint8_t newAddr = static_cast<uint8_t>(_setup.wValue & 0x7F);

        // Prepare status stage first
        _address = newAddr;          // store, but do not set in hardware yet
        statusIn();

        // Hardware address will be programmed in statusIn() completion
        // (see handleEp0Data)
    }

    void handleGetDescriptor()
    {
        if (!_descProvider)
        {
            stallEp0();
            return;
        }

        const uint8_t  type  = static_cast<uint8_t>(_setup.wValue >> 8);
        const uint8_t  index = static_cast<uint8_t>(_setup.wValue & 0xFF);
        const uint16_t lang  = _setup.wIndex;

        size_t len = 0;
        const uint8_t* data = _descProvider->getDescriptor(type, index, lang, len);

        if (!data || len == 0)
        {
            stallEp0();
            return;
        }

        // Limit to requested length
        if (len > _setup.wLength)
            len = _setup.wLength;

        startDataIn(data, len);
    }

    void handleGetConfiguration()
    {
        uint8_t cfg = _configuration;
        startDataIn(&cfg, 1);
    }

    void handleSetConfiguration()
    {
        const uint8_t cfg = static_cast<uint8_t>(_setup.wValue & 0xFF);
        _configuration = cfg;

        if (cfg == 0)
        {
            _state = (_address == 0) ? DeviceState::Default : DeviceState::Address;
            for (size_t i = 0; i < _classCount; ++i)
                _classes[i]->onDeconfigured();
        }
        else
        {
            _state = DeviceState::Configured;
            for (size_t i = 0; i < _classCount; ++i)
                _classes[i]->onConfigured(cfg);
        }

        statusIn();
        notifyStateChange();
    }

    void handleGetInterface()
    {
        // Only valid when configured. Alternate setting 0 for now.
        if (_state != DeviceState::Configured)
        {
            stallEp0();
            return;
        }

        uint8_t alt = 0;
        startDataIn(&alt, 1);
    }

    void handleSetInterface()
    {
        // Minimal implementation – accept alternate setting 0 only
        if (_state != DeviceState::Configured || (_setup.wValue & 0xFF) != 0)
        {
            stallEp0();
            return;
        }

        statusIn();
    }

    // -------------------------------------------------------------------------
    // EP0 data stage helpers
    // -------------------------------------------------------------------------

    void startDataIn(const uint8_t* data, size_t len)
    {
        _ep0DataPtr = data;
        _ep0DataLen = len;
        _ep0DataPos = 0;
        _ep0Zlp     = (len > 0) && (len % 64 == 0) && (len < _setup.wLength);
        // Note: 64 is a simplification; use real EP0 MPS in production

        _ep0Stage = Ep0Stage::DataIn;
        sendNextDataIn();
    }

    void sendNextDataIn()
    {
        const uint16_t mps = getEp0MaxPacketSize();

        if (_ep0DataPos >= _ep0DataLen)
        {
            if (_ep0Zlp)
            {
                _ep0Zlp = false;
                _usb.writePacket(0x80, nullptr, 0);   // Zero-Length Packet
            }
            else
            {
                // Data stage complete → wait for Status OUT
                _ep0Stage = Ep0Stage::StatusOut;
            }
            return;
        }

        const size_t remaining = _ep0DataLen - _ep0DataPos;
        const size_t chunk     = (remaining > mps) ? mps : remaining;

        _usb.writePacket(0x80, _ep0DataPtr + _ep0DataPos, chunk);
        _ep0DataPos += chunk;
    }

    void statusIn()
    {
        _ep0Stage = Ep0Stage::StatusIn;
        _usb.writePacket(0x80, nullptr, 0);   // zero-length status packet
    }

    void statusOut()
    {
        _ep0Stage = Ep0Stage::StatusOut;
        // Just wait for the zero-length OUT packet from host
    }

    void stallEp0()
    {
        _usb.stallEndpoint(0x00, true);
        _usb.stallEndpoint(0x80, true);
        _ep0Stage = Ep0Stage::Idle;
    }

    // -------------------------------------------------------------------------
    // EP0 data / status stage progress
    // -------------------------------------------------------------------------

    void handleEp0Data()
    {
        switch (_ep0Stage)
        {
        case Ep0Stage::DataIn:
            if (_usb.isTxComplete(0x80))
            {
                sendNextDataIn();
            }
            break;

        case Ep0Stage::StatusIn:
            if (_usb.isTxComplete(0x80))
            {
                // Special handling for SET_ADDRESS
                if (static_cast<StdRequest>(_setup.bRequest) == StdRequest::SetAddress)
                {
                    _usb.setAddress(_address);
                    _state = (_address == 0) ? DeviceState::Default : DeviceState::Address;
                    notifyStateChange();
                }

                _ep0Stage = Ep0Stage::Idle;
            }
            break;

        case Ep0Stage::StatusOut:
            if (_usb.isRxReady(0x00))
            {
                uint8_t dummy;
                _usb.readPacket(0x00, &dummy, 0);   // discard ZLP
                _ep0Stage = Ep0Stage::Idle;
            }
            break;

        case Ep0Stage::DataOut:
            if (_usb.isRxReady(0x00))
            {
                size_t toRead = _ep0OutMaxLen - _ep0OutReceived;
                if (toRead > getEp0MaxPacketSize())
                    toRead = getEp0MaxPacketSize();

                size_t got = _usb.readPacket(0x00,
                                               _ep0OutBuf + _ep0OutReceived,
                                               toRead);
                _ep0OutReceived += got;

                // Short packet or requested length reached → data stage finished
                if (got < getEp0MaxPacketSize() || _ep0OutReceived >= _setup.wLength)
                {
                    // Call user callback
                    if (_ep0OutCb)
                        _ep0OutCb(_ep0OutBuf, _ep0OutReceived);

                    // Send Status IN
                    statusIn();
                }
                // else: wait for next OUT packet
            }
            break;

        default:
            break;
        }
    }


};

}