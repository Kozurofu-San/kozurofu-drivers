#pragma once

#include "device/UsbHid.h"
#include "device/UsbCdc.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace driver
{

class HidCdcDescriptorProvider : public IDescriptorProvider
{
public:
    HidCdcDescriptorProvider(const UsbHid& hid, const UsbCdc& cdc,
                             uint16_t vid, uint16_t pid,
                             const char* manufacturer,
                             const char* product,
                             const char* serial = "")
        : _hid(hid), _cdc(cdc), _manufacturer(manufacturer),
          _product(product), _serial(serial)
    {
        _deviceDesc[8]  = static_cast<uint8_t>(vid);
        _deviceDesc[9]  = static_cast<uint8_t>(vid >> 8);
        _deviceDesc[10] = static_cast<uint8_t>(pid);
        _deviceDesc[11] = static_cast<uint8_t>(pid >> 8);
        buildConfigurationDescriptor();
    }

    const uint8_t* getDescriptor(uint8_t type, uint8_t index,
                                 uint16_t langId, size_t& length) override
    {
        (void)langId;
        switch (type)
        {
        case 0x01:
            length = sizeof(_deviceDesc);
            return _deviceDesc;
        case 0x02:
            length = _configDescLen;
            return _configDesc;
        case 0x03:
            return getStringDescriptor(index, length);
        case 0x21:
            length = 9;
            return &_configDesc[_hidDescOffset];
        case 0x22:
            return _hid.getReportDescriptor(length);
        default:
            length = 0;
            return nullptr;
        }
    }

private:
    void append(uint8_t value) { _configDesc[_configDescLen++] = value; }
    void append16(uint16_t value)
    {
        append(static_cast<uint8_t>(value));
        append(static_cast<uint8_t>(value >> 8));
    }

    void buildConfigurationDescriptor()
    {
        _configDescLen = 0;
        append(9); append(0x02); append16(0); append(3); append(1); append(0); append(0x80); append(50);

        // HID interface 0.
        append(9); append(0x04); append(_hid.getInterfaceNumber()); append(0);
        append(_hid.getEpOut() ? 2 : 1); append(0x03); append(0); append(0); append(0);

        size_t reportLength = 0;
        _hid.getReportDescriptor(reportLength);
        _hidDescOffset = _configDescLen;
        append(9); append(0x21); append(0x11); append(0x01); append(0); append(1); append(0x22);
        append16(static_cast<uint16_t>(reportLength));

        appendEndpoint(_hid.getEpIn(), 0x03, _hid.getEpInMaxPacket(), 1);
        if (_hid.getEpOut())
            appendEndpoint(_hid.getEpOut(), 0x03, _hid.getEpOutMaxPacket(), 1);

        // CDC ACM function: IAD, communication interface, functional descriptors,
        // notification endpoint, then data interface with Bulk IN/OUT endpoints.
        append(8); append(0x0B); append(_cdc.getCommInterface()); append(2);
        append(0x02); append(0x02); append(0x01); append(0);

        append(9); append(0x04); append(_cdc.getCommInterface()); append(0); append(1);
        append(0x02); append(0x02); append(0x01); append(0);
        append(5); append(0x24); append(0x00); append(0x10); append(0x01);       // Header
        append(5); append(0x24); append(0x01); append(0x00); append(_cdc.getDataInterface());
        append(4); append(0x24); append(0x02); append(0x02);                       // ACM
        append(5); append(0x24); append(0x06); append(_cdc.getCommInterface()); append(_cdc.getDataInterface());
        appendEndpoint(_cdc.getEpNotif(), 0x03, _cdc.getEpNotifMps(), 16);

        append(9); append(0x04); append(_cdc.getDataInterface()); append(0); append(2);
        append(0x0A); append(0x00); append(0x00); append(0);
        appendEndpoint(_cdc.getEpDataOut(), 0x02, _cdc.getEpDataMps(), 0);
        appendEndpoint(_cdc.getEpDataIn(), 0x02, _cdc.getEpDataMps(), 0);

        _configDesc[2] = static_cast<uint8_t>(_configDescLen);
        _configDesc[3] = static_cast<uint8_t>(_configDescLen >> 8);
    }

    void appendEndpoint(uint8_t address, uint8_t attributes, uint16_t packetSize, uint8_t interval)
    {
        append(7); append(0x05); append(address); append(attributes); append16(packetSize); append(interval);
    }

    const uint8_t* getStringDescriptor(uint8_t index, size_t& length)
    {
        if (index == 0)
        {
            _stringDesc[0] = 4; _stringDesc[1] = 0x03; _stringDesc[2] = 0x09; _stringDesc[3] = 0x04;
            length = 4;
            return _stringDesc;
        }

        const char* source = index == 1 ? _manufacturer : index == 2 ? _product : index == 3 ? _serial : nullptr;
        if (!source)
        {
            length = 0;
            return nullptr;
        }

        size_t count = strlen(source);
        if (count > 30) count = 30;
        _stringDesc[0] = static_cast<uint8_t>(2 + count * 2);
        _stringDesc[1] = 0x03;
        for (size_t i = 0; i < count; ++i)
        {
            _stringDesc[2 + i * 2] = static_cast<uint8_t>(source[i]);
            _stringDesc[3 + i * 2] = 0;
        }
        length = _stringDesc[0];
        return _stringDesc;
    }

    const UsbHid& _hid;
    const UsbCdc& _cdc;
    const char* _manufacturer;
    const char* _product;
    const char* _serial;
    uint8_t _deviceDesc[18] = {18, 0x01, 0x00, 0x02, 0xEF, 0x02, 0x01, 8,
                               0, 0, 0, 0, 0x00, 0x01, 1, 2, 3, 1};
    uint8_t _configDesc[128] = {};
    uint8_t _stringDesc[64] = {};
    size_t _configDescLen = 0;
    size_t _hidDescOffset = 0;
};

} // namespace driver
