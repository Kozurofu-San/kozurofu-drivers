#pragma once

#ifdef AVR
#include <stdint.h>
#include <stddef.h>
#else
#include <cstdint>
#include <cstddef>
#endif

namespace driver
{

class IGpio
{
    public:

    enum Direction: bool
    {
        Input = false,
        Output = true,
    };

    virtual ~IGpio() = default;
    virtual void write(bool state) = 0;
    virtual bool read() = 0;
    virtual size_t getPin() = 0;
    virtual void setDir(Direction dir) = 0;

    virtual void callback(void (*cb)(uint32_t)) = 0;
    virtual bool isInit() = 0;
};

}