#pragma once

#include <cstdint>

namespace driver
{

class IPwm
{
    public:

    virtual ~IPwm() = default;

    virtual void setDutyCycle(uint8_t percent) = 0;

    virtual bool isInit() = 0;
};

}