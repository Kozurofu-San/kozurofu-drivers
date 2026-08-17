#pragma once

#include <cstdint>

namespace driver
{

class IEcg
{
    public:

    virtual ~IEcg() = default;

    virtual int16_t getSample() = 0;

    virtual bool isInit() = 0;
};

}