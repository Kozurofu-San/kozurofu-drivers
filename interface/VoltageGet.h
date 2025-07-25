#pragma once

#include <cstdint>
#include <functional>

class IVoltageGet
{
    public:

    virtual ~IVoltageGet() = default;

    // Get current voltage in volts
    virtual float getVoltage() = 0;
    virtual int32_t getRawValue() = 0;

    // Set callback for voltage change
    virtual void onVoltageChange(void (*cb)(uint32_t)) = 0;
};