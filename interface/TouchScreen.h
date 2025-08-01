#pragma once

#include <cstdint>
#include <functional>

class ITouchScreen
{
    public:

    struct CalibrationData
    {
        int32_t xBias;
        int32_t xScale;
        int32_t yBias;
        int32_t yScale;
    };
    

    virtual ~ITouchScreen() = default;

    virtual uint32_t getCoordinates() = 0;
    virtual bool isPressed() = 0;
    virtual void calibrate(CalibrationData *calibrationData) = 0;
};