#pragma once

#include "interface/Adc.h"

#include "stm32f4xx.h"
#include <cstdint>
#include <cstddef>

namespace driver
{

class AdcDriver : public IAdc
{
    public:

    enum class SampleTime : uint8_t
    {
        Cycles3,
        Cycles15,
        Cycles28,
        Cycles56,
        Cycles84,
        Cycles112,
        Cycles144,
        Cycles480,
    };

    struct ChannelConfig
    {
        uint8_t channel;
        AdcDriver::SampleTime sampleTime;
        int16_t offset;
    };

    AdcDriver(ADC_TypeDef *adc, ChannelConfig *channels, size_t channelCount)
        : _adc(adc), _channels(channels), _channelCount(channelCount)
    {
    }

    void init()
    {
        // Clock
        RCC->APB2ENR |= (_adc == ADC1) ? RCC_APB2ENR_ADC1EN : RCC_APB2ENR_ADC2EN;
        RCC->CFGR |= RCC_CFGR_PPRE2_DIV16; // Set ADC clock to APB2 / 4

        // ADC
        if (_channelCount < 4)
        {
            _adc->JSQR = (_channelCount - 1) << ADC_JSQR_JL_Pos;
            for (size_t i = 0; i < _channelCount; ++i)
            {
                _adc->JSQR |= _channels[i].channel << ((3 - i) * 5);
            }

            _adc->JOFR1 = _channelCount > 0 ? _channels[0].offset : 0;
            _adc->JOFR2 = _channelCount > 1 ? _channels[1].offset : 0;
            _adc->JOFR3 = _channelCount > 2 ? _channels[2].offset : 0;
            _adc->JOFR3 = _channelCount > 3 ? _channels[3].offset : 0;
            
            for (size_t i = 0; i < _channelCount; ++i)
            {
                if (_channels[i].channel < 10)
                {
                    _adc->SMPR2 &= ~(0x7 << (_channels[i].channel * 3));
                    _adc->SMPR2 |= static_cast<uint8_t>(_channels[i].sampleTime) << (_channels[i].channel * 3);
                }
                else
                {
                    _adc->SMPR1 &= ~(0x7 << ((_channels[i].channel - 10) * 3));
                    _adc->SMPR1 |= static_cast<uint8_t>(_channels[i].sampleTime) << ((_channels[i].channel - 10) * 3);
                }
            }
        }
        else
        {
            
        }

        _adc->CR1 |= ADC_CR1_SCAN; // Enable scan mode
        _adc->CR2 |= ADC_CR2_ADON; // Enable ADC
    }

    inline void start()
    {
        _adc->CR2 |= ADC_CR2_JSWSTART; // Start conversion
    }

    float getVoltage(uint32_t channel) override
    {
        return 0;
    }

    int32_t getRawValue(uint32_t channel) override
    {
        while (!(_adc->SR & ADC_SR_JEOC)) {} // Wait for conversion to complete
        _adc->SR &= ~ADC_SR_JEOC; // Clear end of conversion flag
        volatile uint32_t *ptr = &_adc->JDR1;
        return *(ptr + channel); // Return the converted value
    }

    private:

    ADC_TypeDef *_adc;
    ChannelConfig *_channels;
    size_t _channelCount;
};

}