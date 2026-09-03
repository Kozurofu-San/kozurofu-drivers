#pragma once

#include "interface/Adc.h"

#include <cstdint>

#include "stm32f4xx.h"

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
        if      (_adc == ADC1) RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
        else if (_adc == ADC2) RCC->APB2ENR |= RCC_APB2ENR_ADC2EN;
        else if (_adc == ADC3) RCC->APB2ENR |= RCC_APB2ENR_ADC3EN;
        ADC->CCR &= ~ADC_CCR_ADCPRE;
        ADC->CCR |= ADC_CCR_ADCPRE_1; // Set ADC clock 01: PCLK2 / 4

        // ADC config
        _adc->CR2 |= ADC_CR2_ADON; // Enable ADC

        // Wait for ADC stabilization (tSTAB ~ 3 us, simple delay loop is enough)
        for (volatile uint32_t i = 0; i < 100; ++i) {}

        _adc->CR1 |= ADC_CR1_SCAN;      // Enable scan mode (for DMA)
        _adc->CR2 |= ADC_CR2_ALIGN;

        // Software start: EXTEN = 00 (software trigger), no external trigger
        _adc->CR2 &= ~(ADC_CR2_EXTEN | ADC_CR2_EXTSEL);

        // Independent mode (MULTI = 00000 in CCR)
        ADC->CCR &= ~ADC_CCR_MULTI;

        // DMA setup (ADC1 -> DMA2 Stream0 Channel0, ADC2 -> DMA2 Stream2/3 Ch1, ADC3 -> DMA2 Stream1 Ch2)
        RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
        if      (_adc == ADC1) _dma = DMA2_Stream0;
        else if (_adc == ADC2) _dma = DMA2_Stream2;
        else if (_adc == ADC3) _dma = DMA2_Stream1;

        // Disable stream before configuration
        _dma->CR &= ~DMA_SxCR_EN;
        while (_dma->CR & DMA_SxCR_EN) {}

        // Clear all interrupt flags
        if      (_adc == ADC1)
        {
            DMA2->LIFCR = DMA_LIFCR_CTCIF0  |
                          DMA_LIFCR_CHTIF0  |
                          DMA_LIFCR_CTEIF0  |
                          DMA_LIFCR_CDMEIF0 |
                          DMA_LIFCR_CFEIF0  ;
        }
        else if (_adc == ADC2)
        {
            DMA2->LIFCR = DMA_LIFCR_CTCIF2  |
                          DMA_LIFCR_CHTIF2  |
                          DMA_LIFCR_CTEIF2  |
                          DMA_LIFCR_CDMEIF2 |
                          DMA_LIFCR_CFEIF2  ;
        }
        else if (_adc == ADC3)
        {
            DMA2->LIFCR = DMA_LIFCR_CTCIF1  |
                          DMA_LIFCR_CHTIF1  |
                          DMA_LIFCR_CTEIF1  |
                          DMA_LIFCR_CDMEIF1 |
                          DMA_LIFCR_CFEIF1  ;
        }
            

        // Peripheral address = ADC data register
        _dma->PAR  = reinterpret_cast<uint32_t>(&_adc->DR);
        // Memory address = internal buffer
        _dma->M0AR = reinterpret_cast<uint32_t>(_data);

        // Enable DMA requests from ADC
        _adc->CR2 |= ADC_CR2_DMA;
        // DDS = 0 -> DMA requests stop after last transfer (single-shot style)
        _adc->CR2 &= ~ADC_CR2_DDS;

        _isInit = true;
        return  true;

    }

    uint8_t addChannel(uint8_t channel)
    {
        // Check if adding channel is out of limit
        if (!_isInit || (_channelCount >= 16) || (channel >= 16))
        {
            return -1;
        }

        // Add a new ADC channel
        _channel[_channelCount] = channel;

        // The L field stores the number of conversions minus one.
        _adc->SQR1 &= ~ADC_SQR1_L;
        _adc->SQR1 |= _channelCount << ADC_SQR1_L_Pos;

        // Add to conversion sequence
        if (_channelCount < 6)
        {
            _adc->SQR3 |= channel << ((_channelCount - 0) * 5);
        }
        else if (_channelCount < 12)
        {
            _adc->SQR2 |= channel << ((_channelCount - 6) * 5);
        }
        else
        {
            _adc->SQR1 |= channel << ((_channelCount - 12) * 5);
        }

        // Sample rate
        if (channel < 10)
        {
            _adc->SMPR2 &= ~(0x7 << (channel * 3));
            _adc->SMPR2 |= SampleTime::Cycles15 << (channel * 3);
        }
        else
        {
            _adc->SMPR1 &= ~(0x7 << ((channel - 10) * 3));
            _adc->SMPR1 |= SampleTime::Cycles15 << ((channel - 10) * 3);
        }
        
        return _channelCount++;
    }

    bool start()
    {
        if (!_isInit || _channelCount == 0)
        {
            return false;
        }

        if (_dma != nullptr)
        {
            // Re-configure DMA for the current number of channels
            _dma->CR &= ~DMA_SxCR_EN;
            while (_dma->CR & DMA_SxCR_EN) {}

            DMA2->LIFCR = DMA_LIFCR_CTCIF0 | DMA_LIFCR_CHTIF0 |
                          DMA_LIFCR_CTEIF0 | DMA_LIFCR_CDMEIF0 | DMA_LIFCR_CFEIF0;

            _dma->NDTR = _channelCount;

            // Channel 0, Peripheral->Memory, 16-bit, memory increment, no peripheral increment
            _dma->CR = (0U << DMA_SxCR_CHSEL_Pos)   // Channel 0 (ADC1)
                     | (0U << DMA_SxCR_DIR_Pos)     // Peripheral to memory
                     | DMA_SxCR_MINC                // Memory increment
                     | (1U << DMA_SxCR_PSIZE_Pos)   // Peripheral size 16-bit
                     | (1U << DMA_SxCR_MSIZE_Pos)   // Memory size 16-bit
                     | DMA_SxCR_TCIE;               // Transfer complete interrupt (optional)

            _dma->CR |= DMA_SxCR_EN;

            // Start conversion by software
            _adc->CR2 |= ADC_CR2_SWSTART;

            // Wait for DMA transfer complete
            while (!(DMA2->LISR & DMA_LISR_TCIF0)) {}
            DMA2->LIFCR = DMA_LIFCR_CTCIF0;
        }
        else
        {
            // Fallback: polling without DMA (single conversion per channel)
            for (uint8_t i = 0; i < _channelCount; ++i)
            {
                // For single-channel mode we would reconfigure SQR, but here we keep scan
                // and just wait for each EOC (works when EOCS=0, end of sequence)
                _adc->CR2 |= ADC_CR2_SWSTART;
                while (!(_adc->SR & ADC_SR_EOC)) {}
                _data[i] = static_cast<uint16_t>(_adc->DR);
            }
        }

        return true;
    }

    uint16_t getRawValue(uint8_t channel)
    {
        return _data[channel];
    }

    bool isInit()
    {
        return _isInit;
    }

    private:

    ADC_TypeDef *_adc;
    DMA_Stream_TypeDef *_dma = nullptr;

    uint8_t _channel[16];
    uint16_t _data [16];
    uint8_t _channelCount;
    
    enum SampleTime : uint32_t
    {
        Cycles3   = 0,
        Cycles15  = 1,
        Cycles28  = 2,
        Cycles56  = 3,
        Cycles84  = 4,
        Cycles112 = 5,
        Cycles144 = 6,
        Cycles480 = 7,
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

    uint16_t getRawValue() override
    {
        if (!_isInit)
        {
            return -1;
        }
        return _adc.getRawValue(_channelEnum);
    }

    // Q16.16
    uint32_t getVoltage() override
    {
        if (!_isInit)
        {
            return -1;
        }
        return static_cast<uint32_t>(_adc.getRawValue(_channelEnum)) * 3300ULL / 0xFFF0; // Q16.16 unsigned, millivolts
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
