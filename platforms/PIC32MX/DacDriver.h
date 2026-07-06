#pragma once

#include "interface/VoltageSet.h"

#include <cstdint>
#include <cstddef>
#include <cassert>

#include <xc.h>

namespace driver
{

class DacDriver : public IVoltageSet
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

    enum class Trigger : uint8_t
    {
        Timer6Trgo,
        Timer8Trgo,
        Timer7Trgo,
        Timer5Trgo,
        Timer2Trgo,
        Timer4Trgo,
        Exti9,
        SwStart,
        None,
    };

    struct ChannelConfig
    {
        uint8_t channel;
    };

    DacDriver(P dac, ChannelConfig *channels, size_t channelCount)
        : _dac(dac), _channels(channels), _channelCount(channelCount)
    {
        assert(channelCount <= 2);
        for (size_t i = 0; i < channelCount; ++i)
        {
            assert(channels[i].channel <= 2);
        }
    }

    bool init(Trigger trigger)
    {
        _isInit = true;
        return true;
    }

    inline void start() override
    {
    }

    void setVoltage(uint32_t voltage, size_t channel) override
    {

    }

    void setRawValue(uint32_t value, size_t channel) override
    {
        
    }

    bool isInit() override
    {
        return _isInit;
    }

    private:

    P _dac;
    ChannelConfig *_channels;
    size_t _channelCount;
    
    bool _isInit = false;
};

} // namespace driver