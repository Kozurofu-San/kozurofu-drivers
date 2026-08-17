#pragma once

#include "interface/Random.h"
#include "interface/Adc.h"

#include <cstdint>

#include "stm32f1xx.h"

namespace driver
{

class RngDriver : public IRandom
{
    public:

    RngDriver(IAdc &adc)
        : _adc(adc)
    {
    }

    bool init()
    {
        _state ^= static_cast<uint32_t>(_adc.getRawValue()) + 0x9E3779B9u;
        _isInit = true;
        return true;
    }

    uint32_t getValue() override
    {
        uint32_t entropy = static_cast<uint32_t>(_adc.getRawValue());
        _state ^= entropy + 0x9E3779B9u + (_state << 6) + (_state >> 2);
        _state ^= _state << 13;
        _state ^= _state >> 17;
        _state ^= _state << 5;
        return _state;
    }

    bool isInit() override
    {
        return _isInit;
    }

    private:

    IAdc &_adc;
    uint32_t _state = 0xA5A55A5Au;
    
    bool _isInit = false;
};

} // namespace driver
