#pragma once

#include <cstdint>
#include <functional>

class ITimer
{
    public:

    virtual ~ITimer() = default;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void reset() = 0;
    virtual void delay(uint32_t ms) = 0;
    virtual uint32_t now() = 0;

    virtual void callback(void (*cb)(uint32_t)) = 0;
    virtual uint32_t getSpeed() = 0;
    virtual bool isInit() = 0;
};