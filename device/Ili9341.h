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
        uint32_t id = 0;
        _p.sendCommand(Ili9341::ReadId4);
        id = _p.readData();
        id <<= 8;
        id |= _p.readData();
        id <<= 8;
        id |= _p.readData();
        id <<= 8;
        id |= _p.readData();

        // Init display
        _p.sendCommand(Ili9341::Rst);
        _timer.delay(100);


    }

    void setPixel(uint32_t x, uint32_t y, uint32_t color)
    {

    }

    private:

    ICommunication& _p;
    ITimer& _timer;
    IGpio* _backlight;

    uint32_t _sizeX;
    uint32_t _sizeY;

};

}