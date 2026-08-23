#pragma once

#include <cstdint>

namespace driver
{

class IModbus
{
    public:

    virtual ~IModbus() = default;

    virtual bool write(uint8_t *data, uint8_t len) = 0;

    virtual bool isInit() = 0;
};

}