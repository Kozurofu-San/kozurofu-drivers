#pragma once

#include <cstdint>
#include <cstddef>

namespace driver
{

class IUart
{
    public:

    virtual ~IUart() = default;

    virtual void write(uint8_t *data, size_t len) = 0;
    virtual void read (uint8_t *data, size_t len) = 0;

    virtual void setCallback(void (*cb)(uint32_t)) = 0;
    virtual void setBuffer(uint8_t *buffer) = 0;
    virtual uint32_t getSpeed() const = 0;
    virtual bool isInit() = 0;
};

}