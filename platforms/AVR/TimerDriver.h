#pragma once

#include "interface/Timer.h"
#include "interface/Gpio.h"

#include <avr/io.h>
#include <avr/interrupt.h>

namespace driver
{

class TimerDriver : public ITimer
{
public:

    enum class P
    {
        Tim0,
        Tim1,
        Tim2,
    };

    enum class Mode
    {
        Normal  = 2,
        Pwm     = 3,
    };


    TimerDriver(P timer)
        : _timer(timer)
    {
    }


    bool init(Mode mode, uint32_t speed)
    {
        _speed = speed;
        _ms = 0;
        cli();

        if (_timer == P::Tim0)
        {
            TCCR0A |= (1 << WGM01);
            TCCR0B |= (1 << CS01) | (1 << CS00);    // 16 MHz / 64 = 250 kHz
            OCR0A = 250 - 1;
            TIMSK0 |= (1 << OCIE0A);
        }
        else if (_timer == P::Tim1)
        {
            TCCR1A = 0;
            TCCR1B |= (1 << WGM12) | (1 << CS01) | (1 << CS00);    // 16 MHz / 64 = 250 kHz
            OCR1A = 250 - 1;
            TIMSK1 |= (1 << OCIE1A);
        }
        else if (_timer == P::Tim2)
        {
            TCCR2A |= (1 << WGM21);    // 16 MHz / 64 = 250 kHz
            TCCR2A &= ~(1 << WGM20);
            TCCR2B &= ~(1 << WGM22);
            TCCR2B |= (1 << CS22);
            TCCR2B &= ~(1 << CS21);
            TCCR2B &= ~(1 << CS20);
            OCR2A = 250 - 1;
            TIMSK2 |= (1 << OCIE2A);
        }

        sei();


        _isInit = true;

        return true;
    }



    void start() override
    {
        if (!_isInit)
            return;

    }



    void stop() override
    {
        if      (_timer == P::Tim0) TCNT0 = 0;
        else if (_timer == P::Tim1) TCNT1 = 0;
        else if (_timer == P::Tim2) TCNT2 = 0;
    }



    void reset() override
    {
        _ms = 0;
        if      (_timer == P::Tim0) TCNT0 = 0;
        else if (_timer == P::Tim1) TCNT1 = 0;
        else if (_timer == P::Tim2) TCNT2 = 0;
    }



    void delay(uint32_t ms) override
    {
        if (!_isInit)
            return;


        reset();


        _ms = 0;


        while (_ms < ms)
        {
            asm volatile("nop");
        }


        // stop();
    }

    void pwm(uint16_t duty)
    {
    }

    void callback(void (*cb)(uint32_t)) override
    {
        _cb = cb;
    }

    void interrupt()
    {
        _ms++;


        if (_cb)
            _cb(_ms);
    }

    uint32_t getSpeed() override
    {
        return _speed;
    }

    bool isInit() override
    {
        return _isInit;
    }

    inline void load(uint32_t ms)
    {
        _ms = ms;
    }

    inline uint32_t now()
    {
        return _ms;
    }



private:

    P _timer;


    uint32_t _ms = 0;


    uint32_t _speed = 0;

    bool _isInit = false;


    void (*_cb)(uint32_t) = nullptr;
};


} // namespace driver