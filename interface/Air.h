#pragma once

#include <cstdint>

namespace driver
{

class IAir
{
    public:

    virtual ~IAir() = default;

    virtual uint16_t getAqi() = 0;
    virtual uint16_t getTvoc() = 0;
    virtual uint16_t getEco2() = 0;

    virtual bool isInit() = 0;
};

}