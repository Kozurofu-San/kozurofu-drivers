#pragma once

#include <cstdint>

namespace driver
{

class ITemperature
{
    public:

    virtual ~ITemperature() = default;

    virtual int16_t getTemperature() = 0;
    
    virtual bool isInit() = 0;
};

}