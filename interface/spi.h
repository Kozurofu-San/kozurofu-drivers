#pragma once

#include <cstdint>

class Spi
{
    public:

    virtual ~Spi() = default;
    virtual void init() = 0;
    virtual void write(uint8_t data) = 0;
    virtual uint8_t read() = 0;
};