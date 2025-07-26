#pragma once

#include <cstdint>
#include <functional>

class IVoltageGet
{
    public:

    virtual ~IVoltageGet() = default;

    virtual float getVoltage(uint32_t channel) = 0;
    virtual int32_t getRawValue(uint32_t channel) = 0;
};