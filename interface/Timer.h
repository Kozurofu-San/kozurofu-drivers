#pragma once

#ifdef AVR
#include <stdint.h>
#else
#include <cstdint>
#endif

namespace driver
{

class ITimer
{
    public:

    enum Units: int8_t
    {
        ns = -9,
        us = -6,
        ms = -3,
        s  = 0
    };

    struct Time
    {
        uint32_t value;
        Units unit;
    };

    virtual ~ITimer() = default;

    virtual void setPeriod(Time time) = 0;
    virtual Time getPeriod() = 0;

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void reset() = 0;

    virtual void delay(uint32_t ms) = 0;
    virtual uint32_t now() = 0;

    virtual void callback(void (*cb)(uint32_t)) = 0;
    virtual uint32_t getSpeed() = 0;
    virtual bool isInit() = 0;
};

}