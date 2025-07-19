#pragma once

#include <cstdint>
#include <functional>

class IVoltage
{
    public:

    virtual ~IVoltage() = default;

    // Set voltage in volts
    virtual void setVoltage(float voltage) = 0;

    // Get current voltage in volts
    virtual float getVoltage() const = 0;

    // Set callback for voltage change
    virtual void onVoltageChange(std::function<void(float)> callback) = 0;
};