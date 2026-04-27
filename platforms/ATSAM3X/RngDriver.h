#pragma once

#include "interface/Random.h"

#include <cstdint>
#include <cstddef>
#include <cassert>

#include "asf.h"
#include "component/component_trng.h"

#define TRNG_KEY 0x524E4700

namespace driver
{

class RngDriver : public IRandom
{
    public:

    RngDriver(Trng *rng)
        : _rng(rng)
    {
    }

    bool init()
    {
        uint32_t rccId = 0;
        if (_rng == TRNG) rccId = ID_TRNG;
        sysclk_enable_peripheral_clock(rccId);

        _rng->TRNG_CR = TRNG_CR_ENABLE | TRNG_KEY;

        _isInit = true;
        return true;
    }

    uint32_t getValue() override
    {
        while ((_rng->TRNG_ISR & TRNG_ISR_DATRDY) == 0) { }
        return _rng->TRNG_ODATA;
    }

    bool isInit() override
    {
        return _isInit;
    }

    private:

    Trng *_rng;
    
    bool _isInit = false;
};

} // namespace driver