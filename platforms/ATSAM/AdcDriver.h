#pragma once

#include "interface/VoltageGet.h"

#include "asf.h"
#include "component/component_adc.h"
#include "adc/adc.h"

namespace driver
{

class AdcDriver : public IVoltageGet
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

    bool init()
    {
        if ((_adc == nullptr) || (_channels == nullptr) || (_channelCount == 0))
        {
            _isInit = false;
            return false;
        }

        pmc_enable_periph_clk(ID_ADC);
        adc_init(_adc, sysclk_get_cpu_hz(), ADC_FREQ_MAX, ADC_STARTUP_TIME_7);
        adc_configure_timing(_adc, 15, ADC_SETTLING_TIME_3, 3);
        adc_set_resolution(_adc, ADC_12_BITS);
        adc_configure_trigger(_adc, ADC_TRIG_SW, ADC_MR_FREERUN_OFF);
        adc_disable_all_channel(_adc);

        _channelMask = 0;

        for (size_t i = 0; i < _channelCount; i++)
        {
            const uint8_t ch = _channels[i].channel;
            if (ch > static_cast<uint8_t>(ADC_TEMPERATURE_SENSOR))
            {
                _isInit = false;
                return false;
            }

            _channels[i].data = 0;
            _channelMask |= (1u << ch);
            adc_enable_channel(_adc, static_cast<enum adc_channel_num_t>(ch));
        }

        _isInit = true;
        return true;
    }

    void start() override
    {
        if (!_isInit || (_channelCount == 0))
            return;

        for (size_t pass = 0; pass < 2; ++pass)
        {
            for (size_t i = 0; i < _channelCount; i++)
            {
                const uint8_t ch = _channels[i].channel;
                adc_disable_all_channel(_adc);
                adc_enable_channel(_adc, static_cast<enum adc_channel_num_t>(ch));

                // The first sample after switching the analog multiplexer can be stale.
                adc_start(_adc);
                while ((adc_get_status(_adc) & ADC_ISR_DRDY) != ADC_ISR_DRDY)
                {
                }
                (void)(adc_get_latest_value(_adc) & 0x0FFFu);

                uint32_t accumulated = 0;
                static constexpr size_t sampleCount = 4;
                for (size_t sample = 0; sample < sampleCount; ++sample)
                {
                    adc_start(_adc);
                    while ((adc_get_status(_adc) & ADC_ISR_DRDY) != ADC_ISR_DRDY)
                    {
                    }
                    accumulated += adc_get_latest_value(_adc) & 0x0FFFu;
                }

                if (pass == 0)
                {
                    continue;
                }

                int32_t corrected = static_cast<int32_t>(accumulated / sampleCount)
                    + _channels[i].offset;

                if (corrected < 0)
                {
                    corrected = 0;
                }
                else if (corrected > 4095)
                {
                    corrected = 4095;
                }

                _channels[i].data = static_cast<uint16_t>(corrected << 4);
            }
        }

        adc_disable_all_channel(_adc);
        for (size_t i = 0; i < _channelCount; i++)
        {
            adc_enable_channel(_adc, static_cast<enum adc_channel_num_t>(_channels[i].channel));
        }
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

    Adc *_adc;
    ChannelConfig *_channels;
    size_t _channelCount;
    uint32_t _channelMask = 0;
    
    bool _isInit = false;
};

}
