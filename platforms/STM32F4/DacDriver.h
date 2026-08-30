#pragma once

#include "interface/Dac.h"

#include <cstdint>
#include <cstddef>

#include "stm32f4xx.h"
extern uint32_t SystemCoreClock;

namespace driver
{

class DacController
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

    DacController(DAC_TypeDef *dac)
        : _dac(dac)
    {
        _channelCount = 0;
    }

    bool init(Trigger trigger)
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

        _isInit = true;
        return true;
    }

    uint8_t addChannel(uint8_t channel)
    {
        return 0;
    }

    inline void start()
    {
        _dac->SWTRIGR = DAC_SWTRIGR_SWTRIG1 | DAC_SWTRIGR_SWTRIG2;
    }

    void setVoltage(uint32_t voltage, size_t channel)
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

    void setRawValue(uint16_t value, size_t channel)
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

    bool isInit()
    {
        return _isInit;
    }

    private:

    DAC_TypeDef *_dac;
    size_t _channelCount;
    
    bool _isInit = false;
};

} // namespace driver