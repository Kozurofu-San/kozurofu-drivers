#pragma once

#include <cstdint>
#include <cstddef>

namespace driver
{

class IGenerator
{
    public:

    virtual ~IGenerator() = default;

    virtual void setFrequency(size_t channel, uint32_t frequency) = 0;
    virtual void setPhase(size_t channel, uint16_t phase) = 0;
    virtual void setPower(size_t channel, int32_t power) = 0;

    virtual void reset() = 0;

    virtual bool isInit() = 0;
};

}