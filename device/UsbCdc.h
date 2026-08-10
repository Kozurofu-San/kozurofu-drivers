#pragma once

#include "interface/UsbClass.h"
#include "device/UsbDeviceCore.h"
#include "interface/UsbController.h"

#include <cstdint>
#include <cstring>
#include <functional>

namespace driver
{

/**
 * USB CDC-ACM (Virtual Serial Port) class driver.
 *
 * Occupies two interfaces:
 *   - Communication Interface (CI) – Interrupt IN notification endpoint
 *   - Data Interface (DI)         – Bulk IN + Bulk OUT
 *
 * Typical endpoint layout when used together with HID (iface 0):
 *   CDC CI  = interface 1, EP 0x82 (Interrupt IN)
 *   CDC DI  = interface 2, EP 0x03 (Bulk OUT), EP 0x83 (Bulk IN)
 *
 * Public API is intentionally simple (Arduino-like):
 *   - write() / read()
 *   - available()
 *   - setLineCodingCallback() / setControlLineStateCallback()
 */
class UsbCdc : public IUsbClass
{
public:
    struct LineCoding
    {
        uint32_t dwDTERate;     // baud rate
        uint8_t  bCharFormat;   // 0 = 1 stop bit, 1 = 1.5, 2 = 2
        uint8_t  bParityType;   // 0 = None, 1 = Odd, 2 = Even, 3 = Mark, 4 = Space
        uint8_t  bDataBits;     // 5, 6, 7, 8, 16
    };

    using LineCodingCallback      = std::function<void(const LineCoding&)>;
    using ControlLineStateCallback = std::function<void(bool dtr, bool rts)>;

    /**
     * @param commIface     Communication Class interface number
     * @param dataIface     Data Class interface number
     * @param epNotif       Interrupt IN endpoint for Serial State notifications (0x82…)
     * @param epNotifMps    Max packet size for notification EP (usually 8…16)
     * @param epDataOut     Bulk OUT endpoint (0x03…)
     * @param epDataIn      Bulk IN  endpoint (0x83…)
     * @param epDataMps     Max packet size for bulk endpoints (64 for FS)
     */
    UsbCdc(uint8_t  commIface,
           uint8_t  dataIface,
           uint8_t  epNotif,
           uint16_t epNotifMps,
           uint8_t  epDataOut,
           uint8_t  epDataIn,
           uint16_t epDataMps = 64)
        : _commIface(commIface)
        , _dataIface(dataIface)
        , _epNotif(epNotif)
        , _epNotifMps(epNotifMps)
        , _epDataOut(epDataOut)
        , _epDataIn(epDataIn)
        , _epDataMps(epDataMps)
    {
        // Default line coding: 115200 8N1
        _lineCoding.dwDTERate   = 115200;
        _lineCoding.bCharFormat = 0;
        _lineCoding.bParityType = 0;
        _lineCoding.bDataBits   = 8;
    }

    // -------------------------------------------------------------------------
    // IUsbClass
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

        auto& usb = _device->controller();

        // Notification endpoint (Interrupt IN)
        usb.openEndpoint(_epNotif, IUsbController::EpType::Interrupt, _epNotifMps, 10);

        // Bulk OUT
        usb.openEndpoint(_epDataOut, IUsbController::EpType::Bulk, _epDataMps, 0);

        // Bulk IN
        usb.openEndpoint(_epDataIn, IUsbController::EpType::Bulk, _epDataMps, 0);

        _rxHead = _rxTail = 0;
        _txBusy = false;
        _configured = true;
    }

    void onDeconfigured() override
    {
        if (!_device)
            return;

        auto& usb = _device->controller();
        usb.closeEndpoint(_epNotif);
        usb.closeEndpoint(_epDataOut);
        usb.closeEndpoint(_epDataIn);

        _configured = false;
        _dtr = _rts = false;
    }

    bool onSetup(const SetupPacket& setup) override
    {
        // We only care about class-specific requests to our Communication interface
        if (setup.type() != 1)                  // Class
            return false;
        if (setup.recipient() != 1)             // Interface
            return false;
        if ((setup.wIndex & 0xFF) != _commIface)
            return false;

        switch (setup.bRequest)
        {
        case 0x20: // SET_LINE_CODING
            return handleSetLineCoding(setup);

        case 0x21: // GET_LINE_CODING
            return handleGetLineCoding(setup);

        case 0x22: // SET_CONTROL_LINE_STATE
            return handleSetControlLineState(setup);

        case 0x23: // SEND_BREAK (optional)
            // ignore for now
            _device->sendControlStatus();
            return true;

        default:
            return false;
        }
    }

    void process() override
    {
        if (!_configured || !_device)
            return;

        auto& usb = _device->controller();

        //------------------------------------------------------------------
        // Receive data from host (Bulk OUT)
        //------------------------------------------------------------------
        if (usb.isRxReady(_epDataOut))
        {
            uint8_t tmp[64];
            size_t got = usb.readPacket(_epDataOut, tmp, sizeof(tmp));

            for (size_t i = 0; i < got; ++i)
            {
                size_t next = (_rxHead + 1) % RxBufSize;
                if (next != _rxTail)           // not full
                {
                    _rxBuf[_rxHead] = tmp[i];
                    _rxHead = next;
                }
                // else drop byte (buffer overflow)
            }
        }

        //------------------------------------------------------------------
        // Transmit state (Bulk IN)
        //------------------------------------------------------------------
        if (_txBusy && usb.isTxComplete(_epDataIn))
        {
            _txBusy = false;
        }
    }

    uint8_t getInterfaceCount() const override
    {
        return 2;                               // Comm + Data
    }

    // -------------------------------------------------------------------------
    // Public serial-port like API
    // -------------------------------------------------------------------------

    /** Number of bytes available for reading */
    size_t available() const
    {
        if (_rxHead >= _rxTail)
            return _rxHead - _rxTail;
        return RxBufSize - _rxTail + _rxHead;
    }

    /** Read one byte (-1 if empty) */
    int read()
    {
        if (_rxHead == _rxTail)
            return -1;

        uint8_t b = _rxBuf[_rxTail];
        _rxTail = (_rxTail + 1) % RxBufSize;
        return b;
    }

    /** Read up to maxLen bytes, returns number of bytes actually read */
    size_t read(uint8_t* buf, size_t maxLen)
    {
        size_t cnt = 0;
        while (cnt < maxLen)
        {
            int b = read();
            if (b < 0)
                break;
            buf[cnt++] = static_cast<uint8_t>(b);
        }
        return cnt;
    }

    /**
     * Write data to host.
     * Returns number of bytes accepted (may be less than len if busy).
     * Non-blocking.
     */
    size_t write(const uint8_t* data, size_t len)
    {
        if (!_configured || !_device || _txBusy || len == 0)
            return 0;

        auto& ctrl = _device->controller();

        size_t toSend = (len > _epDataMps) ? _epDataMps : len;
        size_t written = ctrl.writePacket(_epDataIn, data, toSend);

        if (written > 0)
            _txBusy = true;

        return written;
    }

    size_t write(const char* str)
    {
        return write(reinterpret_cast<const uint8_t*>(str), strlen(str));
    }

    /** True when DTR is asserted by host (terminal is open) */
    bool dtr() const { return _dtr; }
    bool rts() const { return _rts; }

    const LineCoding& lineCoding() const { return _lineCoding; }

    void setLineCodingCallback(LineCodingCallback cb)
    {
        _lineCodingCb = std::move(cb);
    }

    void setControlLineStateCallback(ControlLineStateCallback cb)
    {
        _controlLineCb = std::move(cb);
    }

    // Getters needed by descriptor provider
    uint8_t  getCommInterface() const { return _commIface; }
    uint8_t  getDataInterface() const { return _dataIface; }
    uint8_t  getEpNotif()       const { return _epNotif; }
    uint8_t  getEpDataOut()     const { return _epDataOut; }
    uint8_t  getEpDataIn()      const { return _epDataIn; }
    uint16_t getEpDataMps()     const { return _epDataMps; }
    uint16_t getEpNotifMps()    const { return _epNotifMps; }

private:
    // -------------------------------------------------------------------------
    // CDC class requests
    // -------------------------------------------------------------------------

    bool handleSetLineCoding(const SetupPacket& setup)
    {
        if (setup.wLength != sizeof(_lineCodingBytes))
            return false;

        _device->startControlOut(_lineCodingBytes, sizeof(_lineCodingBytes), [this](const uint8_t* data, size_t len)
        {
            if (len == sizeof(_lineCodingBytes))
            {
                _lineCoding.dwDTERate   = static_cast<uint32_t>(data[0]) |
                                           (static_cast<uint32_t>(data[1]) << 8) |
                                           (static_cast<uint32_t>(data[2]) << 16) |
                                           (static_cast<uint32_t>(data[3]) << 24);
                _lineCoding.bCharFormat = data[4];
                _lineCoding.bParityType = data[5];
                _lineCoding.bDataBits   = data[6];

                if (_lineCodingCb)
                    _lineCodingCb(_lineCoding);
            }
        });
        return true;
    }

    bool handleGetLineCoding(const SetupPacket& setup)
    {
        _lineCodingBytes[0] = static_cast<uint8_t>(_lineCoding.dwDTERate);
        _lineCodingBytes[1] = static_cast<uint8_t>(_lineCoding.dwDTERate >> 8);
        _lineCodingBytes[2] = static_cast<uint8_t>(_lineCoding.dwDTERate >> 16);
        _lineCodingBytes[3] = static_cast<uint8_t>(_lineCoding.dwDTERate >> 24);
        _lineCodingBytes[4] = _lineCoding.bCharFormat;
        _lineCodingBytes[5] = _lineCoding.bParityType;
        _lineCodingBytes[6] = _lineCoding.bDataBits;

        size_t len = sizeof(_lineCodingBytes);
        if (len > setup.wLength)
            len = setup.wLength;

        _device->startControlIn(_lineCodingBytes, len);
        return true;
    }

    bool handleSetControlLineState(const SetupPacket& setup)
    {
        _dtr = (setup.wValue & 0x01) != 0;
        _rts = (setup.wValue & 0x02) != 0;

        if (_controlLineCb)
            _controlLineCb(_dtr, _rts);

        _device->sendControlStatus();
        return true;
    }

    // -------------------------------------------------------------------------
    // Members
    // -------------------------------------------------------------------------

    UsbDeviceCore* _device = nullptr;

    uint8_t  _commIface;
    uint8_t  _dataIface;
    uint8_t  _epNotif;
    uint16_t _epNotifMps;
    uint8_t  _epDataOut;
    uint8_t  _epDataIn;
    uint16_t _epDataMps;

    LineCoding _lineCoding{};
    uint8_t    _lineCodingBytes[7] = {};
    bool       _dtr = false;
    bool       _rts = false;

    LineCodingCallback       _lineCodingCb;
    ControlLineStateCallback _controlLineCb;

    // Simple ring buffer for RX
    static constexpr size_t RxBufSize = 256;
    uint8_t _rxBuf[RxBufSize];
    size_t  _rxHead = 0;
    size_t  _rxTail = 0;

    bool _txBusy     = false;
    bool _configured = false;
};

} // namespace driver
