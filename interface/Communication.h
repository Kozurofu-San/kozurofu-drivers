#pragma once

#include <cstdint>
#include <functional>

class ICommunication
{
    public:

    virtual ~ICommunication() = default;
    virtual void write(uint8_t *data, size_t len) = 0;
    virtual void read(uint8_t *data, size_t len) = 0;
};