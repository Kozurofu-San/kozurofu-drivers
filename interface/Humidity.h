#pragma once

#include <cstdint>

namespace driver
{

class IHumidity
{
    public:

    virtual ~IHumidity() = default;

    virtual uint16_t getHumidity() = 0;

    virtual bool isInit() = 0;
};

}