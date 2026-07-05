#pragma once

#include "interface/Random.h"
#include "interface/VoltageGet.h"

#include <cstdint>
#include <random>

#include "stm32f1xx.h"

namespace driver
{

class RngDriver : public IRandom
{
    public:

    RngDriver(IVoltageGet &adc)
        : _adc(adc)
    {
    }

    bool init()
    {
        _isInit = true;
        return true;
    }

    uint32_t getValue() override
    {
        uint32_t seed = _adc.getRawValue(0);
        std::mt19937 rng(seed);
        std::uniform_int_distribution<uint32_t> dist(0, 2048);
        return dist(rng);
    }

    bool isInit() override
    {
        return _isInit;
    }

    private:

    IVoltageGet &_adc; // TODO: &_adc doesn't exist after Freertos starts
    
    bool _isInit = false;
};

} // namespace driver