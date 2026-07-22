#pragma once

#include <ctdint>

namespace driver
{

class IDisplay
{
    public:

    virtual ~IDisplay() = default;

    virtual void setArea(uint32_t x1, uint32_t x2, uint32_t y1, uint32_t y2) = 0;
    virtual void fillArea(uint8_t *color, size_t len) = 0;
    virtual bool isInit() = 0;
};

}