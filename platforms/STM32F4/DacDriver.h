#pragma once

#include "interface/Dac.h"

#include "stm32f4xx.h"
#include <cstdint>
#include <cstddef>

namespace driver
{

class DacDriver : public IDac
{
    public:

    DacDriver(DAC_TypeDef *dac)
        : _dac(dac)
    {
    }

    void init()
    {
        RCC->APB1ENR |= RCC_APB1ENR_DACEN; // Enable DAC clock
        _dac->CR |= DAC_CR_EN1; // Enable DAC channel 1
    }

    void setVoltage(float voltage) override
    {
        uint32_t value = static_cast<uint32_t>((voltage / 3.3f) * 4095); // Convert voltage to DAC value (12-bit resolution)
        _dac->DHR12R1 = value; // Set the output value for channel 1
    }

    private:

    DAC_TypeDef *_dac;
};

} // namespace driver