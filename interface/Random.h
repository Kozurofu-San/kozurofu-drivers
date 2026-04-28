#pragma once

#include <cstdint>
#include <functional>

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