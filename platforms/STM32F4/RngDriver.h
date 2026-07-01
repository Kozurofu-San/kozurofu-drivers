#pragma once

#include "interface/Random.h"

#include <cstdint>
#include <cstddef>
#include <cassert>

#include "stm32f4xx.h"

#define TRNG_KEY 0x524E4700

namespace driver
{

class RngDriver : public IRandom
{
    public:

    RngDriver(RNG_TypeDef *rng)
        : _rng(rng)
    {
    }

    bool init()
    {
        RCC->AHB2ENR |= RCC_AHB2ENR_RNGEN;
        RNG->CR |= RNG_CR_RNGEN;

        _isInit = true;
        return true;
    }

    uint32_t getValue() override
    {
        while (!(_rng->SR & RNG_SR_DRDY));
        return _rng->DR;
    }

    bool isInit() override
    {
        return _isInit;
    }

    private:

    RNG_TypeDef *_rng;
    
    bool _isInit = false;
};

} // namespace driver