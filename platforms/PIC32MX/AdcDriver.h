#pragma once

#include "interface/VoltageGet.h"

#include <xc.h>

namespace driver
{

class AdcDriver : public IVoltageGet
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

    struct ChannelConfig
    {
        uint8_t channel;
        int16_t offset;
        uint16_t data = 0;
    };

    AdcDriver(P adc, ChannelConfig *channels, size_t channelCount)
        : _adc(adc), _channels(channels), _channelCount(channelCount)
    {
    }

    bool init()
    {
        
        _isInit = true;
        return true;
    }

    void start() override
    {
        
    }

    int32_t getVoltage(size_t channel) override
    {
        if (channel >= _channelCount)
            return 0;

        return static_cast<int32_t>(_channels[channel].data) * 3300 / 65535;
    }

    int32_t getRawValue(size_t channel) override
    {
        if (channel >= _channelCount)
            return 0;

        return _channels[channel].data;
    }
    
    bool isInit() override
    {
        return _isInit;
    }

    private:

    P _adc;
    ChannelConfig *_channels;
    size_t _channelCount;
    uint32_t _channelMask = 0;
    
    bool _isInit = false;
};

}
