#pragma once

#include <cstdint>

namespace driver
{

class IAdc
{
    public:

    virtual ~IAdc() = default;

    virtual bool start() = 0;

    virtual uint32_t getVoltage() = 0;   // Q16.16 Voltage in mV
    virtual uint16_t getRawValue() = 0;  // 16-bit unsigned value: [15:3] for 12-bit ADC and [15:5] for 10-bit ADC
    
    virtual bool isInit() = 0;
};

}