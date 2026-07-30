#pragma once

#include <cstdint>
#include <cstddef>

namespace driver
{

class IGenerator
{
    public:

    enum Turn: bool
    {
        On = true,
        Off = false,
    };

    virtual ~IGenerator() = default;

    virtual bool setFrequency(uint32_t frequency, size_t channel) = 0;
    virtual bool setPhase(uint16_t phase, size_t channel = 0) = 0;
    virtual bool setPower(int32_t power, size_t channel = 0) = 0;
    virtual bool turn(Turn on) = 0;

    virtual void reset() = 0;

    virtual bool isInit() = 0;
};

}