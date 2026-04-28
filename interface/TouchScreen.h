#pragma once

#include <cstdint>
#include <functional>

namespace driver
{

class ITouchScreen
{
    public:

    struct CalibrationData
    {
        int32_t xScale;
        int32_t xBias;
        int32_t yScale;
        int32_t yBias;
    };
    

    virtual ~ITouchScreen() = default;

    virtual uint32_t getCoordinates() = 0;
    virtual bool isPressed() = 0;
    virtual void calibrate(CalibrationData *calibrationData) = 0;
    virtual bool isInit() = 0;
};

}