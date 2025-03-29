#pragma once

#include <cstdint>
#include <functional>
#include <optional>

class Spi
{
    public:

    virtual ~Spi() = default;
    virtual void write(uint8_t *data, size_t len) = 0;
    virtual void read(uint8_t *data, size_t len) = 0;
};