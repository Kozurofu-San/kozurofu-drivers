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

class II2c
{
    public:

    enum Cmd: bool
    {
        Read = 1,
        Write = 0,
    };

    virtual ~II2c() = default;

    virtual void start()  = 0;
    virtual void stop() = 0;

    virtual void address(bool cmd) = 0;
    virtual void write(uint8_t data) = 0;
    virtual uint8_t read(bool last) = 0;

    virtual void setAddress(uint8_t address) = 0;
    virtual uint8_t getAddress() = 0;

    virtual uint32_t getSpeed() const = 0;
    virtual bool isInit() = 0;
};

}