#pragma once

#include <cstdint>

namespace driver
{

class IAdc
{
    public:

    virtual ~IAdc() = default;

    virtual bool start() = 0;

    virtual int32_t getVoltage() = 0;   // Voltage in mV
    virtual int16_t getRawValue() = 0;  // 16-bit signed value: [15:3] for 12-bit ADC and [15:5] for 10-bit ADC
    
    virtual bool isInit() = 0;
};

}