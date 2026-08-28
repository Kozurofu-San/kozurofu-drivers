#pragma once

#include <cstdint>
#include <cstddef>

namespace driver
{

class IDac
{
    public:

    virtual ~IDac() = default;

    virtual void start() = 0;

    virtual void setVoltage(uint32_t voltage, size_t channel) = 0;  // Q16.16 Voltage in mV
    virtual void setRawValue(uint16_t value, size_t channel) = 0;   // 16-bit unsigned value: [15:3] for 12-bit DAC and [15:5] for 10-bit DAC

    virtual bool isInit() = 0;
};

}