#pragma once

#include "Ili9341Const.h"

#include "interface/Display.h"
#include "interface/Gpio.h"
#include "interface/Timer.h"
#include "interface/Communication.h"

#include "stm32f4xx.h"

namespace driver
{

class Ili9341Driver: IDisplay
{
    public:

    Ili9341Driver(uint32_t x, uint32_t y, ICommunication &p, ITimer &timer, IGpio *backlight = nullptr)
        : _p(p), _timer(timer), _backlight(backlight) {_sizeX = x; _sizeY = y;}

    void init()
    {
        if (_backlight)
        {
            _backlight->write(1);
        }

        // Get ID
        readCmd(Ili9341::ReadId1, &_manufacturerId, 1);
        readCmd(Ili9341::ReadId2, &_driverVersion, 1);
        readCmd(Ili9341::ReadId3, &_driverId, 1);
        readCmd(Ili9341::ReadId4, (uint8_t*)&_id, 3);

        // Init display
        writeCmd(Ili9341::Rst, nullptr, 0);
        _timer.delay(100);
        writeCmd(Ili9341::DisplayOff, nullptr, 0);
        writeCmd(Ili9341::PowerControlA,            (uint8_t[]){0x39, 0x2C, 0x00, 0x34, 0x02}, 5);
        writeCmd(Ili9341::PowerControlB,            (uint8_t[]){0x00, 0x83, 0x30}, 3);
        writeCmd(Ili9341::DriverTimingControlA,     (uint8_t[]){0x85, 0x01, 0x79}, 3);
        writeCmd(Ili9341::DriverTimingControlB,     (uint8_t[]){0x00, 0x00}, 2);
        writeCmd(Ili9341::PowerOnSequenceControl,   (uint8_t[]){0x64, 0x03, 0x12, 0x81}, 4);
        writeCmd(Ili9341::PumpRatioControl,         (uint8_t[]){0x20}, 1);
        writeCmd(Ili9341::PowerControl1,            (uint8_t[]){0x26}, 1);
        writeCmd(Ili9341::PowerControl2,            (uint8_t[]){0x11}, 1);
        writeCmd(Ili9341::VcomControl1,             (uint8_t[]){0x35, 0x3E}, 2);
        writeCmd(Ili9341::VcomControl2,             (uint8_t[]){0xBE}, 1);
        writeCmd(Ili9341::PixelFormatSet,           (uint8_t[]){0x55}, 1);
        writeCmd(Ili9341::FrameControlNormal,       (uint8_t[]){0x00, 0x1B}, 2);
        writeCmd(Ili9341::FunctionControl,          (uint8_t[]){0x0A, 0x82, 0x27, 0x00}, 4);
        writeCmd(Ili9341::Enable3G,                 (uint8_t[]){0x08}, 1);
        writeCmd(Ili9341::GammaSet,                 (uint8_t[]){0x01}, 1);
        writeCmd(Ili9341::PositiveGammaCorrection,  (uint8_t[]){0x1F, 0x1A, 0x18, 0x0A, 0x0F, 0x06, 0x45, 0x87, 
                                                                0x32, 0x0A, 0x07, 0x02, 0x07, 0x05, 0x00}, 15);
        writeCmd(Ili9341::NegativeGammaCorrection,  (uint8_t[]){0x00, 0x25, 0x27, 0x05, 0x10, 0x09, 0x3A, 0x78,
                                                                0x4D, 0x05, 0x18, 0x0D, 0x38, 0x3A, 0x1F}, 15);
        writeCmd(Ili9341::EntryModeSet,             (uint8_t[]){0x07}, 1);
        writeCmd(Ili9341::SleepOut, nullptr, 0);
        _timer.delay(100);
        writeCmd(Ili9341::DisplayOn, nullptr, 0);
        _timer.delay(100);
    }

    void setPixel(uint32_t x, uint32_t y, uint32_t color)
    {

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

    private:

    void readCmd(uint8_t cmd, uint8_t *data, size_t len)
    {
        _p.enable();
        _p.sendCommand(cmd);
        _p.read(data, 1);
        _p.read(data, len);
        _p.disable();
    }

    void writeCmd(uint8_t cmd, uint8_t *data, size_t len)
    {
        _p.enable();
        _p.sendCommand(cmd);
        _p.write(data, len);
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
};

}