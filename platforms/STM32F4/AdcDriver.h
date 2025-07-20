#pragma once

#include "interface/Adc.h"

#include "stm32f4xx.h"
#include <cstdint>
#include <cstddef>

namespace driver
{

class AdcDriver : public IAdc
{
    public:

    AdcDriver(ADC_TypeDef *adc)
        : _adc(adc)
    {
    }

    private:

    ADC_TypeDef *_adc;
};

}