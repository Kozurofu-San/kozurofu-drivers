#pragma once

#include <cstdint>
#include <cstddef>
#include <functional>

namespace driver
{

class IGpio
{
    public:

    virtual ~IGpio() = default;
    virtual void write(bool state) = 0;
    virtual bool read() = 0;
    virtual size_t getPin() = 0;

    virtual void callback(void (*cb)(uint32_t)) = 0;
    virtual bool isInit() = 0;
};

}