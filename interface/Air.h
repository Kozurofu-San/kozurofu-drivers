#pragma once

#include <cstdint>

namespace driver
{

class IAir
{
    public:

    virtual ~IAir() = default;

    virtual uint16_t getAqi()  = 0;     // Air Quality Index according to the UBA 0..5
    virtual uint16_t getTvoc() = 0;     // VOC concentration in ppb
    virtual uint16_t getEco2() = 0;     // Equivalent CO2-concentration in ppm, based on the detected VOCs and hydrogen

    virtual bool isInit() = 0;
};

}