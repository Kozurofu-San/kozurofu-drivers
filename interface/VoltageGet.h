#pragma once

#include <cstdint>
#include <functional>

namespace driver
{

class IVoltageGet
{
    public:

    virtual ~IVoltageGet() = default;

    virtual void start() = 0;
    virtual int32_t getVoltage(size_t channel) = 0;   // Voltage in mV
    virtual int32_t getRawValue(size_t channel) = 0;  // Value in Q0.16. It takes [15:3] for 12-bit value
    virtual bool isInit() = 0;
};

}