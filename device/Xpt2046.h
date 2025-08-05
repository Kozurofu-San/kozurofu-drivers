#pragma once

#include "interface/Communication.h"
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
        static constexpr uint8_t Address        = 0x70;
        static constexpr uint8_t AddressPos     = 4;
        static constexpr uint8_t Mode           = 0x08;
        static constexpr uint8_t SerDfr         = 0x04;
        static constexpr uint8_t PowerDown      = 0x03;
        static constexpr uint8_t PowerDownPos   = 0;
    };

    static constexpr uint32_t MaxSpeed = 2500000; // 25 MHz

    Xpt2046Driver(ICommunication &p, IGpio &irq, ITimer &timer, uint32_t sizeX, uint32_t sizeY)
        : _p(p), _irq(irq), _timer(timer), _sizeX(sizeX), _sizeY(sizeY) 
        {}

    bool init()
    {

        if (_p.getSpeed() > MaxSpeed or _p.getSpeed() == 0)
        {
            return false; // Speed is too high for this touch controller
        }

        _isInit = true;
        return true; // Initialization successful
    }

    uint32_t getCoordinates() override
    {
        // Implement the logic to get coordinates from the XPT2046 touch controller
        // This is a placeholder implementation
        uint32_t x = 0; // Replace with actual X coordinate reading
        uint32_t y = 0; // Replace with actual Y coordinate reading

        if (_calibrationData)
        {
            x = (_calibrationData->xBias + x) * _calibrationData->xScale / 1000;
            y = (_calibrationData->yBias + y) * _calibrationData->yScale / 1000;
        }

        return (y << 16) | x; // Return coordinates as a single 32-bit value
    }

    bool isPressed() override
    {
        return _irq.read();
    }

    void calibrate(CalibrationData *calibrationData) override
    {

    }

    bool isInit() override
    {
        return _isInit;
    }

    private:

    ICommunication &_p;
    IGpio &_irq;
    ITimer &_timer;

    uint8_t _buffer[20];
    uint32_t _sizeX;
    uint32_t _sizeY;

    CalibrationData *_calibrationData;
    bool _isInit = false;
};

}