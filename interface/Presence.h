#pragma once

#include <cstdint>

namespace driver
{

class IPresence
{
    public:

    virtual ~IPresence() = default;

    virtual uint16_t getRange() = 0;
    virtual bool isPresent() = 0;

    virtual bool isInit() = 0;
};

}