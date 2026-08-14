#pragma once

#include <cstdint>

namespace driver
{

class IPressure
{
    public:

    virtual ~IPressure() = default;

    virtual uint32_t getPressurePa() = 0;
    virtual uint16_t getPressuremmHg() = 0;
    
    virtual bool isInit() = 0;
};

}