#pragma once

#include <cstdint>
#include <functional>

class IDisplay
{
    public:

    virtual ~IDisplay() = default;

    virtual void setArea (uint32_t x0x1, uint32_t y0y1) = 0;
    virtual void fillArea(uint8_t  *color, size_t len) = 0;
    virtual bool isInit() = 0;
};