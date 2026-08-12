#pragma once

#include <cstdint>

namespace driver
{

class IPressure
{
    public:

    virtual ~IPressure() = default;

    virtual int16_t getPressure() = 0;
    
    virtual bool isInit() = 0;
};

}