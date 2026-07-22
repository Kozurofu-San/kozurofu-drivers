#pragma once

#include <cstdint>

namespace driver
{

class IRandom
{
    public:

    virtual ~IRandom() = default;
    virtual uint32_t getValue() = 0;

    virtual bool isInit() = 0;
};

}