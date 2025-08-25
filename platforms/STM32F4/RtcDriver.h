#pragma once

#include "interface/DateTime.h"
#include "utils/Utils.h"

#include "stm32f4xx.h"

namespace driver
{

class RtcDriver: public IDateTime
{
    public:

    enum class ClockSource: uint8_t
    {
        Internal,
        External
    };

    RtcDriver(RTC_TypeDef *rtc)
        : _rtc(rtc) {}

    bool init(ClockSource clockSource)
    {
        // Clock
        RCC->APB1ENR |= RCC_APB1ENR_PWREN;

        // Confiig
        PWR->CR |= PWR_CR_DBP;
        if (!(_rtc->ISR & RTC_ISR_INITS))
        {
            if (clockSource == ClockSource::External)
            {
                RCC->BDCR |= RCC_BDCR_BDRST;     // LSE
                RCC->BDCR &= ~RCC_BDCR_BDRST;
                RCC->BDCR |= RCC_BDCR_LSEON;  
                while (!(RCC->BDCR & RCC_BDCR_LSERDY));
                RCC->BDCR |= 1 << RCC_BDCR_RTCSEL_Pos;

                _speed = LSE_VALUE;
            }
            else
            {
            	RCC->CSR |= RCC_CSR_LSION;		// LSI
            	while(!(RCC->CSR & RCC_CSR_LSIRDY));
            	RCC->APB1ENR |= RCC_APB1ENR_PWREN;
            	PWR->CR |= PWR_CR_DBP;
            	RCC->BDCR |= 2 << RCC_BDCR_RTCSEL_Pos;

                _speed = LSI_VALUE;
            }
            
            _rtc->WPR = 0xCA;	//Open access to RTC
            _rtc->WPR = 0x53;	//Open access to RTC
            RTC->ISR |= RTC_ISR_INIT;   //Enter initialization mode
            _rtc->CR = 0;

            while(!(_rtc->ISR & RTC_ISR_ALRAWF));   // Wait until it is allowed to modify RTC_ALRMAR
            _rtc->ALRMAR = RTC_ALRMAR_MSK1 | RTC_ALRMAR_MSK2 | RTC_ALRMAR_MSK3 | RTC_ALRMAR_MSK4;	//Alarm every 1 second
            // _rtc->ALRMAR |= RTC_ALRMAR_SU_0;	// 1 second in BCD format	
            _rtc->CR |= RTC_CR_ALRAE;
            _rtc->CR |= RTC_CR_ALRAIE;
            
            // while(!(_rtc->ISR & RTC_ISR_ALRBWF));   // Wait until it is allowed to modify RTC_ALRMBR
            // _rtc->ALRMBR |= RTC_ALRMBR_MSK1 | RTC_ALRMBR_MSK2 | RTC_ALRMBR_MSK3 | RTC_ALRMBR_MSK4;	//Alarm every 1 day
            // _rtc->ALRMBR |= RTC_ALRMBR_DU_0;	// 1 day in BCD format	
            // _rtc->CR |= RTC_CR_ALRBE;
            // _rtc->CR |= RTC_CR_ALRBIE;

            RTC->ISR &= ~RTC_ISR_INIT;
            _rtc->WPR = 0xFF;	//Close access to RTC  

	        RCC->BDCR |= RCC_BDCR_RTCEN;
        }

        // Interrupts. EXTI17 - RTC IRQ
        EXTI->PR =    EXTI_PR_PR17;
        EXTI->IMR |=  EXTI_IMR_MR17;
        EXTI->EMR &= ~EXTI_EMR_MR17;
        EXTI->RTSR |= EXTI_RTSR_TR17;
        NVIC_SetPriority(RTC_Alarm_IRQn, 40);
        NVIC_EnableIRQ  (RTC_Alarm_IRQn);
        _rtc->ISR &= ~RTC_ISR_ALRAF;
        _rtc->ISR &= ~RTC_ISR_ALRBF;

        _isInit = true;
        return true;
    }

    void setTime(struct tm *t) override
    {
        _rtc->WPR = 0xCA;	// Read protection
        _rtc->WPR = 0x53;
        _rtc->ISR |= RTC_ISR_INIT;
        while(!(_rtc->ISR & RTC_ISR_INITF));
        _rtc->PRER = 
               99 << RTC_PRER_PREDIV_A_Pos
            | 399 << RTC_PRER_PREDIV_S_Pos;
        _rtc->TR =
            Utils::binToBcd ( t->tm_hour ) << RTC_TR_HU_Pos  |
            Utils::binToBcd ( t->tm_min  ) << RTC_TR_MNU_Pos |
            Utils::binToBcd ( t->tm_sec  ) << RTC_TR_SU_Pos  ;
        _rtc->DR =
            Utils::binToBcd ( t->tm_year ) << RTC_DR_YU_Pos  |
            Utils::binToBcd ( t->tm_wday ) << RTC_DR_WDU_Pos |
            Utils::binToBcd ( t->tm_mon  ) << RTC_DR_MU_Pos  |
            Utils::binToBcd ( t->tm_mday ) << RTC_DR_DU_Pos  ;
        _rtc->ISR &= ~RTC_ISR_INIT;
        _rtc->WPR = 0xFF;
    }

    time_t now() override
    {
        struct tm t;
        t.tm_sec  = Utils::bcdToBin ((_rtc->TR & (RTC_TR_ST  | RTC_TR_SU )) >> RTC_TR_SU_Pos );
        t.tm_min  = Utils::bcdToBin ((_rtc->TR & (RTC_TR_MNT | RTC_TR_MNU)) >> RTC_TR_MNU_Pos);
        t.tm_hour = Utils::bcdToBin ((_rtc->TR & (RTC_TR_HT  | RTC_TR_HU )) >> RTC_TR_HU_Pos );
        t.tm_mday = Utils::bcdToBin ((_rtc->DR & (RTC_DR_DT  | RTC_DR_DU )) >> RTC_DR_DU_Pos );
        t.tm_mon  = Utils::bcdToBin ((_rtc->DR & (RTC_DR_MT  | RTC_DR_MU )) >> RTC_DR_MU_Pos );
        t.tm_year = Utils::bcdToBin ((_rtc->DR & (RTC_DR_YT  | RTC_DR_YU )) >> RTC_DR_YU_Pos );

        return mktime(&t);
    }
    
    void callback(void (*cb)(uint32_t)) override
    {
        _cb = cb;
    }
    
    void interrupt()
    {
        EXTI->PR = EXTI_RTSR_TR17;
        if (_rtc->ISR & RTC_ISR_ALRAF)
        {
            if (_cb != nullptr)
            {
                _cb(now());
            }
            _rtc->ISR &= ~RTC_ISR_ALRAF;
        }
        else if (_rtc->ISR & RTC_ISR_ALRBF)
        {
            _rtc->ISR &= ~RTC_ISR_ALRBF;
        }
    }

    inline uint32_t getSpeed() const override
    {
        return _speed;
    }

    inline bool isInit() override
    {
        return _isInit;
    }

    private:

    RTC_TypeDef *_rtc;
    uint32_t _speed = 0;
    bool _isInit = false;

    void (*_cb)(uint32_t) = nullptr;
};

}