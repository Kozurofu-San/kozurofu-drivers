#pragma once

#include <cstdint>
#include <functional>

class IDisplay
{
    public:

    virtual ~IDisplay() = default;

    virtual void setPixel(uint32_t x, uint32_t y, uint32_t color) = 0;
};