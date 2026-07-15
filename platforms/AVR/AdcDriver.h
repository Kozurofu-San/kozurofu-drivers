#pragma once

#include "interface/VoltageGet.h"

#include <avr/io.h>

namespace driver
{

class AdcDriver : public IVoltageGet
{
    public:

    struct ChannelConfig
    {
        uint8_t channel = 0;
        int16_t offset;
        uint16_t data = 0;
    };

    AdcDriver(ChannelConfig *channels, size_t channelCount)
        : _channels(channels), _channelCount(channelCount)
    {
    }

    bool init()
    {
        ADMUX  |= _BV(REFS0);  // Select AVcc with external capacitor at AREF pin
        ADCSRA |= _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);   // Set ADC Prescaler to 128 (16MHz / 128 = 125kHz ADC clock)
        ADCSRA |= _BV(ADEN);  // Enable ADC

        _isInit = true;
        return true;
    }

    void start() override
    {
        for (size_t i = 0; i < _channelCount; i++)
        {
            ADMUX = (ADMUX & 0xF0) | (_channels[i].channel & 0x0F);  // Select ADC channel (0 to 7) without affecting other bits
            ADCSRA |= _BV(ADSC);          // Start conversion
            while (ADCSRA & _BV(ADSC));   // Wait for conversion to complete
            _channels[i].data = ADC;
        }
    }

    int32_t getVoltage(size_t channel) override
    {
        if (channel >= _channelCount)
            return 0;

        return static_cast<int32_t>(_channels[channel].data) * 3300 / 1024;
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

    ChannelConfig *_channels;
    size_t _channelCount;
    uint32_t _channelMask = 0;
    
    bool _isInit = false;
};

}
