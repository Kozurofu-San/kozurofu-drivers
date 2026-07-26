#pragma once

#include "interface/Spi.h"
#include "interface/Touchscreen.h"
#include "interface/Gpio.h"
#include "interface/Timer.h"

#include "stm32f4xx.h"

namespace driver
{

class Xpt2046Driver: public ITouchScreen
{
    public:

    class Control
    {
        public:

        static constexpr uint8_t Start          = 0x80;
        static constexpr uint8_t Y              = 0x50;
        static constexpr uint8_t X              = 0x10;
        static constexpr uint8_t Mode           = 0x08;
        static constexpr uint8_t SerDfr         = 0x04;
        static constexpr uint8_t PowerDown      = 0x03;
        static constexpr uint8_t PowerDownPos   = 0;
    };

    static constexpr uint32_t MaxSpeed = 2500000; // 2.5 MHz

    Xpt2046Driver(ISpi &p, IGpio &irq, ITimer &timer)
        : _p(p), _irq(irq), _timer(timer)
        {}

    bool init(uint32_t sizeX, uint32_t sizeY)
    {
        // Init check
        if (!_p.isInit() or !_irq.isInit() or !_timer.isInit())
        {
            return false;
        }

        // Speed check
        if (_p.getSpeed() > MaxSpeed or _p.getSpeed() == 0)
        {
            return false;
        }

        _sizeX = sizeX;
        _sizeY = sizeY;

        _isInit = true;
        return _isInit;
    }

    uint32_t getCoordinates() override
    {
        uint32_t x = readCmd(Control::Start | Control::X);
        uint32_t y = readCmd(Control::Start | Control::Y);

        if (_calibrationData)
        {   // int to Q16.16     x = a * x^ + b

            x <<= 4;
            x /= _calibrationData->xScale;
            x += _calibrationData->xBias;
            
            y <<= 4;
            y /= _calibrationData->yScale;
            y += _calibrationData->yBias;

            // Q16.16 to int
            return ((x<<12)&0xFFFF0000)|(y>>4);
        }

        return (x << 16) | y;
    }

    bool isPressed() override
    {
        return !_irq.read();
    }

    void calibrate(CalibrationData *calibrationData) override
    {
        _calibrationData = calibrationData;
    }

    bool isInit() override
    {
        return _isInit;
    }

    private:

    ISpi &_p;
    IGpio &_irq;
    ITimer &_timer;

    uint8_t _buffer[2];
    uint32_t _sizeX;
    uint32_t _sizeY;

    CalibrationData *_calibrationData = nullptr;
    bool _isInit = false;

    uint32_t readCmd(uint8_t cmd)
    {
        uint32_t value = 0;
        uint16_t ret = 0;
        _p.enable();
        _p.sendCommand(cmd);
        for (size_t i = 0; i < 16; ++i)
        {
            _p.read(_buffer, 2);
            ret = _buffer[0] << 8;
            ret |= _buffer[1];
            value += ret;
        }
        _p.disable();
        return value;
    }
};

}