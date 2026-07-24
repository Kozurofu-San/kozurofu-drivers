#pragma once

#include <cstdint>
#include <cstddef>

namespace driver
{

class ILedStrip
{
    public:

    virtual ~ILedStrip() = default;

    virtual void setColor(uint8_t *array, size_t len) = 0;
    virtual void setBacklight(size_t value) = 0;
    virtual bool isInit() = 0;
};

}