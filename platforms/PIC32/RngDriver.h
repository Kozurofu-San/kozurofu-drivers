#pragma once

#include "interface/Random.h"

#include <cstdint>
#include <cstddef>
#include <cassert>

#include <xc.h>

namespace driver
{

class RngDriver : public IRandom
{
    public:

    enum class P: uint32_t
    {
        PortB = 0xBF886040,
        PortC = 0xBF886080,
        PortD = 0xBF8860C0,
        PortE = 0xBF886100,
        PortF = 0xBF886140,
        PortG = 0xBF886180,
    };

    RngDriver(P rng)
        : _rng(rng)
    {
    }

    bool init()
    {

        _isInit = true;
        return true;
    }

    uint32_t getValue() override
    {
        return 0;
    }

    bool isInit() override
    {
        return _isInit;
    }

    private:

    P _rng;
    
    bool _isInit = false;
};

} // namespace driver