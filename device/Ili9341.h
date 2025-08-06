#pragma once

#include "Ili9341Const.h"

#include "interface/Display.h"
#include "interface/Gpio.h"
#include "interface/Timer.h"
#include "interface/Communication.h"

#include "stm32f4xx.h"
#include <type_traits>
#include <array>
#include <cstdint>

namespace driver
{

class Ili9341Driver: public IDisplay
{
    public:

    static constexpr uint32_t MaxSpeed = 10000000;
    static constexpr uint32_t Id = 0x419300;

    Ili9341Driver(ICommunication &p, ITimer &timer, IGpio *backlight = nullptr)
        : _p(p), _timer(timer), _backlight(backlight) {}

    bool init(uint32_t x, uint32_t y)
    {
        // Init check
        if (!_p.isInit() && !_timer.isInit())
        {
            return false;
        }

        // Backlight
        if (_backlight)
        {
            _backlight->write(1);
        }

        _sizeX = x; _sizeY = y;

        // Get ID
        readCmd(Ili9341::ReadId1, &_manufacturerId, 1);
        readCmd(Ili9341::ReadId2, &_driverVersion, 1);
        readCmd(Ili9341::ReadId3, &_driverId, 1);
        readCmd(Ili9341::ReadId4, reinterpret_cast<uint8_t*>(&_id), 3);

        if (_id != Id)
        {
            return false;
        }

        // Init display
        writeCmd(Ili9341::Rst, nullptr, 0);
        _timer.delay(100);
        writeCmd(Ili9341::DisplayOff, nullptr, 0);
        writeCmd(Ili9341::PowerControlA,            std::array<uint8_t, 5>{0x39, 0x2C, 0x00, 0x34, 0x02}.data(), 5);
        writeCmd(Ili9341::PowerControlB,            std::array<uint8_t, 3>{0x00, 0x83, 0x30}.data(), 3);
        writeCmd(Ili9341::DriverTimingControlA,     std::array<uint8_t, 3>{0x85, 0x01, 0x79}.data(), 3);
        writeCmd(Ili9341::DriverTimingControlB,     std::array<uint8_t, 2>{0x00, 0x00}.data(), 2);
        writeCmd(Ili9341::PowerOnSequenceControl,   std::array<uint8_t, 4>{0x64, 0x03, 0x12, 0x81}.data(), 4);
        writeCmd(Ili9341::PumpRatioControl,         std::array<uint8_t, 1>{0x20}.data(), 1);
        writeCmd(Ili9341::PowerControl1,            std::array<uint8_t, 1>{0x26}.data(), 1);
        writeCmd(Ili9341::PowerControl2,            std::array<uint8_t, 1>{0x11}.data(), 1);
        writeCmd(Ili9341::VcomControl1,             std::array<uint8_t, 2>{0x35, 0x3E}.data(), 2);
        writeCmd(Ili9341::VcomControl2,             std::array<uint8_t, 1>{0xBE}.data(), 1);
        writeCmd(Ili9341::PixelFormatSet,           std::array<uint8_t, 1>{0x55}.data(), 1);
        writeCmd(Ili9341::FrameControlNormal,       std::array<uint8_t, 2>{0x00, 0x1B}.data(), 2);
        writeCmd(Ili9341::FunctionControl,          std::array<uint8_t, 4>{0x0A, 0x82, 0x27, 0x00}.data(), 4);
        writeCmd(Ili9341::Enable3G,                 std::array<uint8_t, 1>{0x08}.data(), 1);
        writeCmd(Ili9341::GammaSet,                 std::array<uint8_t, 1>{0x01}.data(), 1);
        writeCmd(Ili9341::PositiveGammaCorrection,  std::array<uint8_t, 15>{0x1F, 0x1A, 0x18, 0x0A, 0x0F, 0x06, 0x45, 0x87, 
                                                                0x32, 0x0A, 0x07, 0x02, 0x07, 0x05, 0x00}.data(), 15);
        writeCmd(Ili9341::NegativeGammaCorrection,  std::array<uint8_t, 15>{0x00, 0x25, 0x27, 0x05, 0x10, 0x09, 0x3A, 0x78,
                                                                0x4D, 0x05, 0x18, 0x0D, 0x38, 0x3A, 0x1F}.data(), 15);
        writeCmd(Ili9341::EntryModeSet,             std::array<uint8_t, 1>{0x07}.data(), 1);
        writeCmd(Ili9341::SleepOut, nullptr, 0);
        _timer.delay(100);
        writeCmd(Ili9341::DisplayOn, nullptr, 0);
        _timer.delay(100);

        _isInit = true;
        return true;
    }

    void fillArea(uint8_t  *color, size_t len) override
    {
        writeCmd(Ili9341::MemoryWrite, color, len, 2);
    }

    void setArea(uint32_t x0x1, uint32_t y0y1)
    {
        writeCmd(Ili9341::ColumnAddressSet, reinterpret_cast<uint8_t*> (&x0x1), 4);
        writeCmd(Ili9341::PageAddressSet,   reinterpret_cast<uint8_t*> (&y0y1), 4);
    }

    enum class Orientation : uint8_t
    {
        VerticalNormal      = 0x48,
        HorizontalNormal    = 0x28,
        VerticalFlipped     = 0x88,
        HorizontalFlipped   = 0xE8,
    };

    void orientation(Orientation orientation)
    {
        uint8_t orient = static_cast<uint8_t>(orientation);
        writeCmd(Ili9341::MemoryAccessControl, &orient, 1);
        if (orient % 2 == 0) // Vertical
        {
            _sizeX = 240;
            _sizeY = 320;
        }
        else // Horizontal
        {
            _sizeX = 320;
            _sizeY = 240;
        }
    }

    bool isInit() override
    {
        return _isInit;
    }

    private:

    template<typename Base, typename T>
    inline bool instanceof(const T*) {
        return std::is_base_of<Base, T>::value;
    }

    void readCmd(uint8_t cmd, uint8_t *data, size_t len, size_t bytes = 1)
    {
        _p.enable();
        _p.sendCommand(cmd);
        _p.read(data, 1);
        _p.read(data, len, bytes);
        _p.disable();
    }

    void writeCmd(uint8_t cmd, uint8_t *data, size_t len, size_t bytes = 1)
    {
        _p.enable();
        _p.sendCommand(cmd);
        _p.write(data, len, bytes);
        _p.disable();
    }

    ICommunication& _p;
    ITimer& _timer;
    IGpio* _backlight;

    uint8_t _buffer[20];
    uint32_t _sizeX;
    uint32_t _sizeY;

    uint8_t _manufacturerId = 0;
    uint8_t _driverVersion = 0;
    uint8_t _driverId = 0;
    uint32_t _id = 0;

    bool _isInit = false;
};

}