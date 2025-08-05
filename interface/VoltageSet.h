#pragma once

#include <cstdint>
#include <functional>

class IVoltageSet
{
    public:

    virtual ~IVoltageSet() = default;

    virtual void start() = 0;
    virtual void setVoltage(uint32_t voltage, size_t channel) = 0;
    virtual void setRawValue(uint32_t value, size_t channel) = 0;
    virtual bool isInit() = 0;
};