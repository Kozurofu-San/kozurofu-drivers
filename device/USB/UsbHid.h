#pragma once

#include "UsbDeviceCore.h"
#include "interface/UsbController.h"
#include "interface/UsbClass.h"

#include <cstdint>
#include <cstring>
#include <functional>

namespace driver
{

/**
 * Simple USB HID class driver (Device side).
 *
 * Features:
 *  - One Interrupt IN endpoint (required)
 *  - Optional Interrupt OUT endpoint
 *  - Support for GET_REPORT / SET_REPORT / GET_IDLE / SET_IDLE / GET_PROTOCOL / SET_PROTOCOL
 *  - User-provided Report Descriptor
 *  - Simple API: sendReport() / setOutputReportCallback()
 *
 * Usage example:
 *
 *   static const uint8_t reportDesc[] = { ... };
 *   usb::UsbHid hid(1, 0x81, 8, reportDesc, sizeof(reportDesc)); // iface=1, EP IN=0x81, MPS=8
 *   device.registerClass(hid);
 */
class UsbHid : public IUsbClass
{
public:
    using OutputReportCallback = std::function<void(const uint8_t* data, size_t len)>;

    /**
     * @param interfaceNumber  Interface number this HID occupies
     * @param epInAddr         Interrupt IN endpoint address (0x81, 0x82, ...)
     * @param epInMaxPacket    Max packet size for IN (usually 8, 16, 32, 64)
     * @param reportDesc       Pointer to Report Descriptor
     * @param reportDescLen    Length of Report Descriptor
     * @param epOutAddr        Optional Interrupt OUT endpoint (0 = not used)
     * @param epOutMaxPacket   Max packet size for OUT
     */
    UsbHid(uint8_t interfaceNumber,
           uint8_t epInAddr,
           uint16_t epInMaxPacket,
           const uint8_t* reportDesc,
           size_t reportDescLen,
           uint8_t epOutAddr = 0,
           uint16_t epOutMaxPacket = 0)
        : _iface(interfaceNumber)
        , _epIn(epInAddr)
        , _epInMps(epInMaxPacket)
        , _epOut(epOutAddr)
        , _epOutMps(epOutMaxPacket)
        , _reportDesc(reportDesc)
        , _reportDescLen(reportDescLen)
    {
    }

    // -------------------------------------------------------------------------
    // IUsbClass interface
    // -------------------------------------------------------------------------

    void init(UsbDeviceCore& device) override
    {
        _device = &device;
    }

    void deinit() override
    {
        onDeconfigured();
        _device = nullptr;
    }

    void onConfigured(uint8_t /*configuration*/) override
    {
        if (!_device)
            return;

        // Open Interrupt IN
        _device->openEndpoint(_epIn, IUsbController::EpType::Interrupt, _epInMps, 1);

        // Optional Interrupt OUT
        if (_epOut != 0)
        {
            _device->openEndpoint(_epOut, IUsbController::EpType::Interrupt, _epOutMps, 1);
        }

        _idleRate   = 0;                       // 0 = infinite
        _protocol   = 1;                       // Report protocol by default
        _configured = true;
    }

    void onDeconfigured() override
    {
        if (!_device)
            return;

        _device->closeEndpoint(_epIn);
        if (_epOut)
            _device->closeEndpoint(_epOut);

        _configured = false;
    }

    bool onSetup(const SetupPacket& setup) override
    {
        // Only handle requests directed to our interface
        if (setup.recipient() != 1)             // Interface
            return false;

        if ((setup.wIndex & 0xFF) != _iface)
            return false;

        // Class-specific requests (type == 1)
        if (setup.type() != 1)
            return false;

        switch (setup.bRequest)
        {
        case 0x01: // GET_REPORT
            return handleGetReport(setup);

        case 0x02: // GET_IDLE
            return handleGetIdle(setup);

        case 0x03: // GET_PROTOCOL
            return handleGetProtocol(setup);

        case 0x09: // SET_REPORT
            return handleSetReport(setup);

        case 0x0A: // SET_IDLE
            return handleSetIdle(setup);

        case 0x0B: // SET_PROTOCOL
            return handleSetProtocol(setup);

        default:
            return false;
        }
    }

    void process() override
    {
        if (!_configured || !_device)
            return;

        auto& ctrl = _device->controller();

        //------------------------------------------------------------------
        // Optional Interrupt OUT endpoint – receive Output / Feature reports
        //------------------------------------------------------------------
        if (_epOut != 0 && ctrl.isRxReady(_epOut))
        {
            uint8_t buf[64];                        // enough for most HID reports
            size_t  maxLen = (_epOutMps < sizeof(buf)) ? _epOutMps : sizeof(buf);

            size_t got = ctrl.readPacket(_epOut, buf, maxLen);

            if (got > 0 && _outputCb)
            {
                _outputCb(buf, got);
            }
        }

        //------------------------------------------------------------------
        // Optional: idle-rate handling / timed reports can be added here
        //------------------------------------------------------------------
        // if (m_idleRate != 0) { ... }
    }

    uint8_t getInterfaceCount() const override
    {
        return 1;
    }

    // -------------------------------------------------------------------------
    // Public API for application
    // -------------------------------------------------------------------------

    /**
     * Send an Input report (Interrupt IN).
     * Returns true if the transfer was started successfully.
     */
    bool sendReport(const uint8_t* data, size_t len)
    {
        if (!_configured || !_device || len == 0 || len > _epInMps)
            return false;

        // Direct access to controller would be better.
        // For now we assume UsbDeviceCore provides a write helper
        // or the application uses the controller directly.
        // Minimal working version:
        auto& ctrl = _device->controller();
        return ctrl.writePacket(_epIn, data, len) == len;
    }

    /**
     * Set callback that will be called when host sends an Output / Feature report.
     */
    void setOutputReportCallback(OutputReportCallback cb)
    {
        _outputCb = std::move(cb);
    }

    /**
     * Returns pointer to the Report Descriptor (used by descriptor provider).
     */
    const uint8_t* getReportDescriptor(size_t& len) const
    {
        len = _reportDescLen;
        return _reportDesc;
    }

    uint8_t getInterfaceNumber() const { return _iface; }
    uint8_t getEpIn()            const { return _epIn; }
    uint8_t getEpOut()           const { return _epOut; }
    uint8_t getEpInMaxPacket()   const { return _epInMps; }
    uint8_t getEpOutMaxPacket()  const { return _epOutMps; }

private:
    // -------------------------------------------------------------------------
    // Class-specific request handlers
    // -------------------------------------------------------------------------

    bool handleGetReport(const SetupPacket& setup)
    {
        // wValue: high byte = report type (1=Input, 2=Output, 3=Feature)
        //         low byte  = report ID
        // For simplicity we only support report ID 0 and return a zeroed buffer
        // or the last sent report. Real devices keep a report cache.

        const uint8_t reportType = static_cast<uint8_t>(setup.wValue >> 8);
        const uint8_t reportId   = static_cast<uint8_t>(setup.wValue & 0xFF);

        if (reportId != 0)
            return false;                       // only ID 0 supported in this example

        size_t len = setup.wLength;
        if (len > sizeof(_controlReport))
            len = sizeof(_controlReport);

        (void)reportType;
        _device->startControlIn(_controlReport, len);
        return true;
    }

    bool handleSetReport(const SetupPacket& setup)
    {
        // Host is sending a report (Output or Feature) via Control EP0
        // Data stage will come next. In full implementation you would
        // prepare a buffer and finish the transfer in EP0 DataOut handler.

        const uint8_t reportType = static_cast<uint8_t>(setup.wValue >> 8);
        (void)reportType;

        if (setup.wLength > sizeof(_controlReport))
            return false;

        _device->startControlOut(_controlReport, sizeof(_controlReport),
                                 [this](const uint8_t* data, size_t len)
                                 {
                                     if (_outputCb)
                                         _outputCb(data, len);
                                 });
        return true;
    }

    bool handleGetIdle(const SetupPacket& /*setup*/)
    {
        _device->startControlIn(&_idleRate, 1);
        return true;
    }

    bool handleSetIdle(const SetupPacket& setup)
    {
        _idleRate = static_cast<uint8_t>(setup.wValue >> 8);
        _device->sendControlStatus();
        return true;
    }

    bool handleGetProtocol(const SetupPacket& /*setup*/)
    {
        _device->startControlIn(&_protocol, 1);
        return true;
    }

    bool handleSetProtocol(const SetupPacket& setup)
    {
        _protocol = static_cast<uint8_t>(setup.wValue & 0xFF);
        _device->sendControlStatus();
        return true;
    }

    // -------------------------------------------------------------------------
    // Members
    // -------------------------------------------------------------------------

    UsbDeviceCore* _device = nullptr;

    uint8_t  _iface;
    uint8_t  _epIn;
    uint16_t _epInMps;
    uint8_t  _epOut;
    uint16_t _epOutMps;

    const uint8_t* _reportDesc;
    size_t         _reportDescLen;

    uint8_t  _idleRate   = 0;
    uint8_t  _protocol   = 1;                  // 1 = Report Protocol
    bool     _configured = false;
    uint8_t  _controlReport[64] = {};

    OutputReportCallback _outputCb;
};

class HidDescriptorProvider : public IDescriptorProvider
{
public:
    /**
     * @param hid           Reference to the HID class instance (for Report Desc + interface number)
     * @param vid           Vendor ID
     * @param pid           Product ID
     * @param manufacturer  Manufacturer string (UTF-8, will be converted to UTF-16)
     * @param product       Product string
     * @param serial        Serial number string (can be empty)
     */
    HidDescriptorProvider(const UsbHid& hid,
                          uint16_t vid, uint16_t pid,
                          const char* manufacturer,
                          const char* product,
                          const char* serial = "")
        : _hid(hid)
        , _vid(vid)
        , _pid(pid)
        , _manufacturer(manufacturer)
        , _product(product)
        , _serial(serial)
    {
        buildConfigurationDescriptor();
        _deviceDesc[8]  = _vid & 0xFF;
        _deviceDesc[9]  = _vid >> 8;
        _deviceDesc[10] = _pid & 0xFF;
        _deviceDesc[11] = _pid >> 8;
    }

    // -------------------------------------------------------------------------
    // IDescriptorProvider
    // -------------------------------------------------------------------------

    const uint8_t* getDescriptor(uint8_t type, uint8_t index,
                                 uint16_t langId, size_t& length) override
    {
        (void)langId;   // we support only 0x0409 (English US)

        switch (type)
        {
        case 0x01: // DEVICE
            length = sizeof(_deviceDesc);
            return _deviceDesc;

        case 0x02: // CONFIGURATION
            length = _configDescLen;
            return _configDesc;

        case 0x03: // STRING
            return getStringDescriptor(index, length);

        case 0x21: // HID Descriptor (can be requested separately by some hosts)
            // Usually part of configuration, but we can return it if asked
            length = 9;
            return &_configDesc[/* offset of HID desc */ 18];

        case 0x22: // HID REPORT DESCRIPTOR
        {
            size_t reportLen = 0;
            const uint8_t* report = _hid.getReportDescriptor(reportLen);
            length = reportLen;
            return report;
        }

        default:
            length = 0;
            return nullptr;
        }
    }

private:
    // -------------------------------------------------------------------------
    // Build Configuration Descriptor at construction time
    // -------------------------------------------------------------------------

    void buildConfigurationDescriptor()
    {
        // Layout:
        // 0  - Configuration Descriptor (9 bytes)
        // 9  - Interface Descriptor (9 bytes)
        // 18 - HID Descriptor (9 bytes)
        // 27 - Endpoint IN Descriptor (7 bytes)
        // 34 - Endpoint OUT Descriptor (7 bytes) – optional

        uint8_t* p = _configDesc;
        size_t   offset = 0;

        // ----- Configuration Descriptor -----
        p[offset++] = 9;                    // bLength
        p[offset++] = 0x02;                 // bDescriptorType = CONFIGURATION
        // wTotalLength – filled later
        p[offset++] = 0;
        p[offset++] = 0;
        p[offset++] = 1;                    // bNumInterfaces
        p[offset++] = 1;                    // bConfigurationValue
        p[offset++] = 0;                    // iConfiguration
        p[offset++] = 0x80;                 // bmAttributes = Bus powered
        p[offset++] = 50;                   // bMaxPower = 100 mA

        // ----- Interface Descriptor -----
        p[offset++] = 9;                    // bLength
        p[offset++] = 0x04;                 // bDescriptorType = INTERFACE
        p[offset++] = _hid.getInterfaceNumber(); // bInterfaceNumber
        p[offset++] = 0;                    // bAlternateSetting
        p[offset++] = (_hid.getEpOut() != 0) ? 2 : 1; // bNumEndpoints
        p[offset++] = 0x03;                 // bInterfaceClass = HID
        p[offset++] = 0x00;                 // bInterfaceSubClass (0 = no subclass)
        p[offset++] = 0x00;                 // bInterfaceProtocol (0 = none)
        p[offset++] = 0;                    // iInterface

        // ----- HID Descriptor -----
        size_t reportLen = 0;
        _hid.getReportDescriptor(reportLen);

        p[offset++] = 9;                    // bLength
        p[offset++] = 0x21;                 // bDescriptorType = HID
        p[offset++] = 0x11;                 // bcdHID = 1.11
        p[offset++] = 0x01;
        p[offset++] = 0x00;                 // bCountryCode
        p[offset++] = 0x01;                 // bNumDescriptors
        p[offset++] = 0x22;                 // bDescriptorType = Report
        p[offset++] = static_cast<uint8_t>(reportLen & 0xFF);
        p[offset++] = static_cast<uint8_t>(reportLen >> 8);

        // ----- Endpoint IN Descriptor -----
        p[offset++] = 7;                    // bLength
        p[offset++] = 0x05;                 // bDescriptorType = ENDPOINT
        p[offset++] = _hid.getEpIn();       // bEndpointAddress
        p[offset++] = 0x03;                 // bmAttributes = Interrupt
        p[offset++] = _hid.getEpInMaxPacket(); // wMaxPacketSize
        p[offset++] = 0x00;
        p[offset++] = 1;                    // bInterval = 1 ms

        // ----- Optional Endpoint OUT Descriptor -----
        if (_hid.getEpOut() != 0)
        {
            p[offset++] = 7;
            p[offset++] = 0x05;
            p[offset++] = _hid.getEpOut();
            p[offset++] = 0x03;             // Interrupt
            p[offset++] = 8;                // wMaxPacketSize low
            p[offset++] = 0x00;
            p[offset++] = 1;                // bInterval
        }

        // Fill wTotalLength
        _configDesc[2] = static_cast<uint8_t>(offset & 0xFF);
        _configDesc[3] = static_cast<uint8_t>(offset >> 8);
        _configDescLen = offset;
    }

    // -------------------------------------------------------------------------
    // String descriptors (very simple UTF-16LE converter)
    // -------------------------------------------------------------------------

    const uint8_t* getStringDescriptor(uint8_t index, size_t& length)
    {
        static uint8_t strBuf[64];

        if (index == 0)                         // Language ID
        {
            strBuf[0] = 4;
            strBuf[1] = 0x03;
            strBuf[2] = 0x09;                   // 0x0409 English US
            strBuf[3] = 0x04;
            length = 4;
            return strBuf;
        }

        const char* src = nullptr;
        if (index == 1) src = _manufacturer;
        else if (index == 2) src = _product;
        else if (index == 3) src = _serial;
        else
        {
            length = 0;
            return nullptr;
        }

        size_t charCount = strlen(src);
        if (charCount > 30) charCount = 30;

        strBuf[0] = static_cast<uint8_t>(2 + charCount * 2);
        strBuf[1] = 0x03;

        for (size_t i = 0; i < charCount; ++i)
        {
            strBuf[2 + i * 2]     = static_cast<uint8_t>(src[i]);
            strBuf[2 + i * 2 + 1] = 0x00;
        }

        length = strBuf[0];
        return strBuf;
    }

    // -------------------------------------------------------------------------
    // Device Descriptor (fixed)
    // -------------------------------------------------------------------------

    uint8_t _deviceDesc[18] = {
        18,         // bLength
        0x01,       // bDescriptorType = DEVICE
        0x00, 0x02, // bcdUSB = 2.00
        0x00,       // bDeviceClass (defined at interface)
        0x00,       // bDeviceSubClass
        0x00,       // bDeviceProtocol
        8,          // bMaxPacketSize0 (8 for FS, can be 64)
        0x00, 0x00, // idVendor  – filled in constructor
        0x00, 0x00, // idProduct – filled in constructor
        0x00, 0x01, // bcdDevice = 1.00
        1,          // iManufacturer
        2,          // iProduct
        3,          // iSerialNumber
        1           // bNumConfigurations
    };

    // -------------------------------------------------------------------------
    // Members
    // -------------------------------------------------------------------------

    const UsbHid& _hid;
    uint16_t      _vid;
    uint16_t      _pid;
    const char*   _manufacturer;
    const char*   _product;
    const char*   _serial;

    uint8_t _configDesc[64] = {};
    size_t  _configDescLen  = 0;

    // Fill VID/PID in constructor body
    // (C++ doesn't allow it easily in member initializer for array)
    // Add this inside the constructor after buildConfigurationDescriptor():
    //
    // _deviceDesc[8]  = _vid & 0xFF;
    // _deviceDesc[9]  = _vid >> 8;
    // _deviceDesc[10] = _pid & 0xFF;
    // _deviceDesc[11] = _pid >> 8;
};

} // namespace driver
