#pragma once

#include <cstdint>
#include <functional>

class IMemory
{
    public:

    virtual ~IMemory() = default;

    // For communication
    virtual void write(uint8_t *data, uint32_t address, size_t len) = 0;
    virtual void read (uint8_t *data, uint32_t address, size_t len) = 0;
    virtual void erase() = 0;
};