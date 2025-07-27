#pragma once

#include "interface/Adc.h"

#include "sam3x8e.h"
#include <cstdint>
#include <cstddef>

namespace driver
{

class AdcDriver : public IAdc
{
    public:

    struct ChannelConfig
    {
        uint8_t channel;
        int16_t offset;
        uint16_t data = 0;
    };

    AdcDriver(Adc *adc, ChannelConfig *channels, size_t channelCount)
        : _adc(adc), _channels(channels), _channelCount(channelCount)
    {
    }

    void init()
    {
    }

    void start()
    {
    }

    int32_t getVoltage(size_t channel) override
    {
        return _channels[channel].data * 3300 / 65536;
    }

    int32_t getRawValue(size_t channel) override
    {
        return _channels[channel].data;
    }

    private:

    Adc *_adc;
    ChannelConfig *_channels;
    size_t _channelCount;
};

}