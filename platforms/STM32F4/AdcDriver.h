#pragma once

#include "interface/VoltageGet.h"

#include "stm32f4xx.h"
#include <cstdint>
#include <cstddef>

namespace driver
{

class AdcDriver : public IVoltageGet
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

    enum class ExtSel : uint8_t
    {
        Timer1Cc1,
        Timer1Cc2,
        Timer1Cc3,
        Timer2Cc2,
        Timer3Trgo,
        Timer4Cc4,
        Exti11Tim8Trgo,
        SwStart,
    };

    enum class JExtSel : uint8_t
    {
        Timer1Trgo,
        Timer1Cc4,
        Timer2Trgo,
        Timer2Cc1,
        Timer2Cc4,
        Timer4Trgo,
        Exti15Tim8Cc4,
        JSwStart,
    };

    struct ChannelConfig
    {
        uint8_t channel;
        int16_t offset;
        uint16_t data = 0;
    };

    AdcDriver(ADC_TypeDef *adc, ChannelConfig *channels, size_t channelCount)
        : _adc(adc), _channels(channels), _channelCount(channelCount)
    {
    }

    void init()
    {
        // Clock
        RCC->APB2ENR |= (_adc == ADC1) ? RCC_APB2ENR_ADC1EN : RCC_APB2ENR_ADC2EN;

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
                    _adc->SMPR2 |= static_cast<uint8_t>(SampleTime::Cycles15) << (_channels[i].channel * 3);
                }
                else
                {
                    _adc->SMPR1 &= ~(0x7 << ((_channels[i].channel - 10) * 3));
                    _adc->SMPR1 |= static_cast<uint8_t>(SampleTime::Cycles15) << ((_channels[i].channel - 10) * 3);
                }
            }
        }
        else
        {
            
        }

        _adc->CR1 |= ADC_CR1_SCAN; // Enable scan mode
        _adc->CR2 |= ADC_CR2_ADON; // Enable ADC
    }

    void start() override
    {
        _adc->CR2 |= ADC_CR2_JSWSTART;          // Start conversion
        while (!(_adc->SR & ADC_SR_JEOC)) {}    // Wait for conversion to complete
        _adc->SR &= ~ADC_SR_JEOC;               // Clear end of conversion flag
        volatile uint32_t *ptr = &_adc->JDR1;
        for (size_t i = 0; i < _channelCount; ++i)
        {
            _channels[i].data = *(ptr + i) << 4;     // Put data to reg
        }
    }

    int32_t getVoltage(size_t channel) override
    {
        return _channels[channel].data * 3300 / 65535;
    }

    int32_t getRawValue(size_t channel) override
    {
        return _channels[channel].data;
    }

    private:

    ADC_TypeDef *_adc;
    ChannelConfig *_channels;
    size_t _channelCount;
};

}