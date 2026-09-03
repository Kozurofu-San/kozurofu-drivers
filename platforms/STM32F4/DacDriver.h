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

    DacController(DAC_TypeDef *dac)
        : _dac(dac)
    {
        _channelCount = 0;
    }

    bool init()
    {
        // Clock
        RCC->APB1ENR |= RCC_APB1ENR_DACEN;          // Enable DAC clock

        // DAC
        
        if (_channelCount == 2)
        {
            // Dual mode
        }

        _isInit = true;
        return true;
    }

    uint8_t addChannel(uint8_t channel)
    {
        if (!_isInit || (_channelCount > 2))
        {
            return -1;
        }

        // Add a new DAC channel
        _channel[_channelCount] = channel;

        if (channel == 1)
        {
            _dac->CR &= ~DAC_CR_EN1;
            _dac->CR |= DAC_CR_EN1;
            _dac->CR &= ~DAC_CR_TSEL1;
            _dac->CR |= Trigger::SwStart << DAC_CR_TSEL1_Pos;
            _dac->CR |= DAC_CR_TEN1; // Enable trigger
            _dac->CR |= DAC_CR_BOFF1;
            _dac->CR |= DAC_CR_EN1;
            _dac->CR |= DAC_CR_MAMP1;
        }
        else if (channel == 2)
        {
            _dac->CR &= ~DAC_CR_EN2;
            _dac->CR |= DAC_CR_EN2;
            _dac->CR &= ~DAC_CR_TSEL2;
            _dac->CR |= Trigger::SwStart << DAC_CR_TSEL2_Pos;
            _dac->CR |= DAC_CR_TEN2; // Enable trigger
            _dac->CR |= DAC_CR_BOFF2;
            _dac->CR |= DAC_CR_EN2;
            _dac->CR |= DAC_CR_MAMP2;
        }
        else
        {
            return -1;
        }
        return _channelCount++;
    }

    inline void start(uint32_t channel)
    {
        _dac->SWTRIGR = channel;
    }

    void setRawValue(uint16_t value, size_t channel)
    {
        if (_channel[channel] == 1)
        {
            _dac->DHR12L1 = value;
        }
        else if (_channel[channel] == 2)
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
    uint8_t _channel[2];
    uint16_t _data [2];
    size_t _channelCount;
    
    bool _isInit = false;
    
    enum Trigger : uint8_t
    {
        Timer6Trgo,
        Timer8Trgo,
        Timer7Trgo,
        Timer5Trgo,
        Timer2Trgo,
        Timer4Trgo,
        Exti9,
        SwStart
    };

};

class DacDriver : public IDac
{
    public:

    DacDriver(DacController &dac, uint8_t channel)
        : _dac(dac), _channel(channel)
    {
    }

    bool init()
    {
        if (_dac.isInit())
        {
            return false;
        }
        _channelEnum = _dac.addChannel(_channel);

        _isInit = true;
        return  true;
    }

    void start() override
    {
        if (!_isInit)
        {
            _dac.start(DAC_SWTRIGR_SWTRIG1 | DAC_SWTRIGR_SWTRIG2);  // Start both channels
        }
    }

    void setRawValue(uint16_t value) override
    {
        if (!_isInit)
        {
            return _dac.setRawValue(value, _channelEnum);
        }
    }

    // Q16.16
    void setVoltage(uint32_t voltage) override
    {
        if (!_isInit)
        {
            _dac.setRawValue(static_cast<uint16_t>(voltage * 0xFFF0 / 3300ULL), _channelEnum); // Q16.16 unsigned, millivolts
        }
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

    DacController &_dac;
    uint8_t _channel;
    uint8_t _channelEnum;

    bool _isInit = false;
};

} // namespace driver
