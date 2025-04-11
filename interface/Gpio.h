#pragma once

#include <cstdint>
#include <functional>

class IGpio
{
    public:

    virtual ~IGpio() = default;
    virtual void write(bool state) = 0;
    virtual bool read() = 0;

    virtual void callback(void (*cb)(uint32_t)) = 0;
};