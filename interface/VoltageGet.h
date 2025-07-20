#pragma once

#include <cstdint>
#include <functional>

class IVoltageGet
{
    public:

    virtual ~IVoltageGet() = default;

    // Get current voltage in volts
    virtual float getVoltage() const = 0;

    // Set callback for voltage change
    virtual void onVoltageChange(std::function<void(float)> callback) = 0;
};