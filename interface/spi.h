#pragma once

#include <cstdint>
#include <functional>
#include <optional>

class ISpi
{
    public:

    virtual ~ISpi() = default;
    virtual void write(uint8_t *data, size_t len) = 0;
    virtual void read(uint8_t *data, size_t len) = 0;
};