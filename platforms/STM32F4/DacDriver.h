#pragma once

#include "interface/VoltageSet.h"

#include "stm32f4xx.h"
#include <cstdint>
#include <cstddef>

namespace driver
{

class DacDriver : public IVoltageSet
{
    public:

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

    DacDriver(DAC_TypeDef *dac, ChannelConfig *channels, size_t channelCount)
        : _dac(dac), _channels(channels), _channelCount(channelCount)
    {
        if (channelCount > 2)
        {
            while(true);
        }

        for (size_t i = 0; i < channelCount; ++i)
        {
            if (channels[i].channel > 2)
            {
                while(true);
            }
        }
    }

    void init(Trigger trigger)
    {
        // Clock
        RCC->APB1ENR |= RCC_APB1ENR_DACEN;          // Enable DAC clock

        // DAC
        for (size_t i = 0; i < _channelCount; ++i)  // Enable DAC channels
        {
            if (_channels[i].channel == 1)
            {
                _dac->CR &= ~DAC_CR_EN1;
                _dac->CR |= DAC_CR_EN1;
                _dac->CR &= ~DAC_CR_TSEL1;
                _dac->CR |= static_cast<size_t> (trigger) << DAC_CR_TSEL1_Pos;
                if (trigger != Trigger::None)
                {
                    _dac->CR |= DAC_CR_TEN1; // Enable trigger
                }
                else
                {
                    _dac->CR &= ~DAC_CR_TEN1; // Disable trigger
                }
                _dac->CR |= DAC_CR_BOFF1;
                _dac->CR |= DAC_CR_EN1;
                _dac->CR |= DAC_CR_MAMP1;
            }
            else if (_channels[i].channel == 2)
            {
                _dac->CR &= ~DAC_CR_EN2;
                _dac->CR |= DAC_CR_EN2;
                _dac->CR &= ~DAC_CR_TSEL2;
                _dac->CR |= static_cast<size_t> (trigger) << DAC_CR_TSEL2_Pos;
                if (trigger != Trigger::None)
                {
                    _dac->CR |= DAC_CR_TEN2; // Enable trigger
                }
                else
                {
                    _dac->CR &= ~DAC_CR_TEN2; // Disable trigger
                }
                _dac->CR |= DAC_CR_BOFF2;
                _dac->CR |= DAC_CR_EN2;
                _dac->CR |= DAC_CR_MAMP2;
            }
        }
        
        if (_channelCount == 2)
        {
            // Dual mode
        }
    }

    inline void start() override
    {
        _dac->SWTRIGR = DAC_SWTRIGR_SWTRIG1 | DAC_SWTRIGR_SWTRIG2;
    }

    void setVoltage(uint32_t voltage, size_t channel) override
    {
        uint32_t value = static_cast<uint32_t>((voltage * 4095) / 3000);
        if (value > 4095) value = 4095;
        
        if (_channels[channel].channel == 1)
        {
            _dac->DHR12R1 = value;
        }
        else if (_channels[channel].channel == 2)
        {
            _dac->DHR12R2 = value;
        }
    }

    void setRawValue(uint32_t value, size_t channel) override
    {
        if (_channels[channel].channel == 1)
        {
            _dac->DHR12L1 = value;
        }
        else if (_channels[channel].channel == 2)
        {
            _dac->DHR12L2 = value;
        }
    }

    private:

    DAC_TypeDef *_dac;
    ChannelConfig *_channels;
    size_t _channelCount;
};

} // namespace driver