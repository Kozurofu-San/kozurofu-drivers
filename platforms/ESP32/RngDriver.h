#pragma once

#include "interface/Random.h"

#include <cstdint>
#include <cstddef>
#include <cassert>

#include "esp_random.h"

namespace driver
{

class RngDriver : public IRandom
{
    public:

    RngDriver()
    {
    }

    uint32_t getValue() override
    {
        return esp_random();
    }

    bool isInit() override
    {
        return _isInit;
    }

    private:

    bool _isInit = false;
};

} // namespace driver