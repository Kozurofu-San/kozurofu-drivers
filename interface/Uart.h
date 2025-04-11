#pragma once

#include <cstdint>
#include <functional>

class IUart
{
    public:

    virtual ~IUart() = default;
    virtual void write(uint8_t *data, size_t len) = 0;
    virtual void read(uint8_t *data, size_t len) = 0;
    
    virtual void callback(void cb(uint32_t)) = 0;
};