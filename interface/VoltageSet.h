#pragma once

#include <cstdint>
#include <functional>

class IVoltageSet
{
    public:

    virtual ~IVoltageSet() = default;

    // Set voltage in volts
    virtual void setVoltage(float voltage) = 0;
    virtual void setRawVoltage(float voltage) = 0;
};