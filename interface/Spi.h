#pragma once

#include <cstdint>
#include <cstddef>

namespace driver
{

class ISpi
{
    public:

    enum class Mode: uint32_t
    {
        Master = 0x4,
        Slave = 0x0
    };

    enum class ClockPolarity: uint32_t
    {
        IdleLow = 0x0,
        IdleHigh = 0x2
    };

    enum class ClockPhase: uint32_t
    {
        FirstEdge = 0x0,
        SecondEdge = 0x1
    };

    virtual ~ISpi() = default;

    virtual void enable()  = 0;
    virtual void disable() = 0;

    virtual uint8_t transfer(uint8_t data) = 0;
    virtual void write(uint8_t *data, size_t len) = 0;
    virtual void read (uint8_t *data, size_t len) = 0;

    virtual uint32_t getSpeed() const = 0;
    virtual bool isInit() = 0;
};

}