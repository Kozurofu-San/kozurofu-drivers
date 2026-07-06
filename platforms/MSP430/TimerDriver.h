#pragma once

#include "interface/Timer.h"
#include "interface/Gpio.h"

#include <msp430.h>

#include <cstdint>
#include <cstddef>

#undef REG
#define REG(p, bias) (*(volatile uint16_t *)((uint16_t)p + (uint16_t)& bias - (uint16_t)& TACTL))

namespace driver
{
    
class TimerDriver : public ITimer
{
    public:

    enum class P: uint16_t
    {
        TimerA0 = 0x0160,
        TimerA1 = 0x0180,
    };

    enum class Mode
    {
        Normal = 0,
        Pwm = 1,
    };

    TimerDriver(P timer, uint32_t frequency)
        : _timer(timer), _frequency(frequency)
    {
    }

    void init(Mode mode)
    {
        __disable_interrupt();

        if (mode == Mode::Normal)
        {
            _ms = 0;
            REG(_timer, TACTL)
                = TASSEL_2  // Source SMCLK
                | MC_0      // Stop mode
                | ID_3      // Divider 1
                | TACLR     // Clear timer
                | TAIE      // Enable interrupt
            ;
            REG(_timer, TACCR0) = _frequency / 1'000 / 8 - 1; // 1 ms
        }
        else
        {
            REG(_timer, TACCR0) = 0;
            REG(_timer, TACTL)
                = TASSEL_2  // Source SMCLK
                | MC_2      // Continue mode
                | ID_0      // Divider 1
                | TACLR     // Clear timer
                | TAIE      // Enable interrupt
            ;
            REG(_timer, TACCTL0)
                = CM1       // Capture on rising edge
                | CCIS_0    // CCIxA
                | CCIE      // Interrupt enable
            ;
        }
        __enable_interrupt();
    }
    
    void start() override
    {
        REG(_timer, TACTL) &= ~MC_3;
        REG(_timer, TACTL) |= MC_1;
    }
    
    void stop() override
    {
        REG(_timer, TACTL) &= ~MC_3;
    }
    
    void reset() override
    {
        stop();
        _ms = 0;
        *(volatile uint16_t *)((uintptr_t)_timer + (uintptr_t)& TACTL - (uintptr_t)& TACTL) |= TACLR;
    }
    
    void delay(uint32_t ms) override
    {
        reset();
        start();
        while (_ms < ms) 
        { 
            // __low_power_mode_0();
        }
        stop();
    }

    void pwm(uint16_t pwm)
    {
        REG(_timer, TACCR0) = pwm;
    }

    void callback(void cb(uint32_t))
    {
        _cb = cb;
    }

    uint32_t getSpeed() override
    {
        return _speed;
    }

    bool isInit() override
    {
        return _isInit;
    }

    inline void load(uint32_t ms) { _ms = ms; }
    inline uint32_t now() { return _ms; }
    
    private:

    P _timer;
    uint32_t _frequency;
    uint32_t _ms = 0;

    IGpio *_gpioPwm = nullptr;
    uint32_t _speed = 0;

    bool _isInit = false;

    void (*_cb)(uint32_t) = nullptr;
};
}
