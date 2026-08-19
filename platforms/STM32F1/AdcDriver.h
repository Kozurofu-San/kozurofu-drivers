#pragma once

#include "interface/Adc.h"

#include <cstdint>

#include "stm32f1xx.h"

namespace driver
{

class AdcController
{
    public:

    AdcController(ADC_TypeDef *adc)
        : _adc(adc)
    {
        _channelCount = 0;
    }

    bool init()
    {
        // Clock
        RCC->APB2ENR |= (_adc == ADC1) ? RCC_APB2ENR_ADC1EN : RCC_APB2ENR_ADC2EN;
        RCC->CFGR &= ~RCC_CFGR_ADCPRE;
        RCC->CFGR |= RCC_CFGR_ADCPRE_DIV6; // Set ADC clock to APB2 / 6

        _adc->CR1 |= ADC_CR1_SCAN; // Enable scan mode
        _adc->CR2 |= ADC_CR2_ADON; // Enable ADC

        _adc->CR2 |= ADC_CR2_RSTCAL;
        while(_adc->CR2 & ADC_CR2_RSTCAL);
        
        _adc->CR2 |= ADC_CR2_CAL;
        while(_adc->CR2 & ADC_CR2_CAL);

        _isInit = true;
        return  true;

    }

    uint8_t addChannel(uint8_t channel)
    {
        // Check if adding channel is out of limit
        if ((_channelCount >= 16) | (channel >= 16))
        {
            return -1;
        }

        // Add a new ADC channel
        _channels[_channelCount].channel = channel;

        // Number of conversions
        _adc->SQR1 &= ~ADC_SQR1_L;
        _adc->SQR1 |= _channelCount << ADC_SQR1_L_Pos;

        // Add to conversion sequence
        if (_channelCount < 6)
        {
            _adc->SQR3 |= channel << ((_channelCount - 0) * 4);
        }
        else if (_channelCount < 12)
        {
            _adc->SQR2 |= channel << ((_channelCount - 6) * 4);
        }
        else
        {
            _adc->SQR1 |= channel << ((_channelCount - 12) * 4);
        }

        // Sample rate
        if (channel < 10)
        {
            _adc->SMPR2 &= ~(0x7 << (channel * 3));
            _adc->SMPR2 |= SampleTime::Cycles7_5 << (channel * 3);
        }
        else
        {
            _adc->SMPR1 &= ~(0x7 << ((channel - 10) * 3));
            _adc->SMPR1 |= SampleTime::Cycles7_5 << ((channel - 10) * 3);
        }

        _adc->CR2 |= ADC_CR2_EXTTRIG | ADC_CR2_ALIGN | ExtSel::SwStart;
        _adc->CR1 |= DualMode::RegularOnly;
        _adc->CR2 |= ADC_CR2_ADON; // Enable ADC

        return _channelCount++;
    }

    bool start()
    {
        _adc->CR2 |= ADC_CR2_SWSTART;           // Start conversion
        while (!(_adc->SR & ADC_SR_EOC));       // Wait for conversion to complete
        _adc->SR &= ~ADC_SR_EOC;                // Clear end of conversion flag
        for (uint8_t i = 0; i < _channelCount; ++i)
        {
            _channels[i].data = _adc->DR - (1 << 15) - 1;      // Put data to reg 
        }
        return true;
    }

    int16_t getRawValue(uint8_t channel)
    {
        return _channels[channel].data;
    }

    bool isInit()
    {
        return _isInit;
    }

    private:

    ADC_TypeDef *_adc;

    struct ChannelConfig
    {
        uint8_t channel;
        uint16_t data = 0;
    } _channels[16];
    uint8_t _channelCount;

    enum SampleTime : uint32_t
    {
        Cycles1_5   = 0,
        Cycles7_5   = 1,
        Cycles13_5  = 2,
        Cycles28_5  = 3,
        Cycles41_5  = 4,
        Cycles55_5  = 5,
        Cycles71_5  = 6,
        Cycles239_5 = 7,
    };

    enum ExtSel : uint32_t
    {
        Timer1Cc1        = 0 << ADC_CR2_EXTSEL_Pos,
        Timer1Cc2        = 1 << ADC_CR2_EXTSEL_Pos,
        Timer1Cc3        = 2 << ADC_CR2_EXTSEL_Pos,
        Timer2Cc2        = 3 << ADC_CR2_EXTSEL_Pos,
        Timer3Trgo       = 4 << ADC_CR2_EXTSEL_Pos,
        Timer4Cc4        = 5 << ADC_CR2_EXTSEL_Pos,
        Exti11Tim8Trgo   = 6 << ADC_CR2_EXTSEL_Pos,
        SwStart          = 7 << ADC_CR2_EXTSEL_Pos,
    };

    enum JExtSel : uint32_t
    {
        Timer1Trgo      = 0 << ADC_CR2_JEXTSEL_Pos,
        Timer1Cc4       = 1 << ADC_CR2_JEXTSEL_Pos,
        Timer2Trgo      = 2 << ADC_CR2_JEXTSEL_Pos,
        Timer2Cc1       = 3 << ADC_CR2_JEXTSEL_Pos,
        Timer2Cc4       = 4 << ADC_CR2_JEXTSEL_Pos,
        Timer4Trgo      = 5 << ADC_CR2_JEXTSEL_Pos,
        Exti15Tim8Cc4   = 6 << ADC_CR2_JEXTSEL_Pos,
        JSwStart        = 7 << ADC_CR2_JEXTSEL_Pos,
    };

    enum DualMode : uint32_t
    {
        Independent                  = 0 << ADC_CR1_DUALMOD_Pos,
        RegularInjected              = 1 << ADC_CR1_DUALMOD_Pos,
        RegularAlternateTrigger      = 2 << ADC_CR1_DUALMOD_Pos,
        InjectedFastInterleaved      = 3 << ADC_CR1_DUALMOD_Pos,
        InjectedSlowInterleaved      = 4 << ADC_CR1_DUALMOD_Pos,
        InjectedOnly                 = 5 << ADC_CR1_DUALMOD_Pos,
        RegularOnly                  = 6 << ADC_CR1_DUALMOD_Pos,
        FastInterleavedOnly          = 7 << ADC_CR1_DUALMOD_Pos,
        SlowInterleavedOnly          = 8 << ADC_CR1_DUALMOD_Pos,
        AlternateTriggerOnly         = 9 << ADC_CR1_DUALMOD_Pos,
    };

    bool _isInit = false;
};

class AdcDriver : public IAdc
{
    public:

    AdcDriver(AdcController &adc, uint8_t channel)
        : _adc(adc), _channel(channel)
    {
    }

    bool init()
    {
        if (_isInit)
        {
            return false;
        }
        _channelEnum = _adc.addChannel(_channel);

        _isInit = true;
        return  true;
    }

    bool start() override
    {
        if (!_isInit)
        {
            return -1;
        }
        return _adc.start();
    }

    int16_t getRawValue() override
    {
        if (!_isInit)
        {
            return -1;
        }
        return _adc.getRawValue(_channelEnum);
    }

    int32_t getVoltage() override
    {
        if (!_isInit)
        {
            return -1;
        }
        return _adc.getRawValue(_channelEnum) * 3300 / (1 << 15);   // 16 bit signed
    }

    uint8_t getChannel()
    {
        return _channel;
    }

    bool isInit() override
    {
        return _isInit;
    }

    private:

    AdcController &_adc;
    uint8_t _channel;
    uint8_t _channelEnum;

    bool _isInit = false;
};

}