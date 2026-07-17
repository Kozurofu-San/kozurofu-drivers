#pragma once

#include "interface/Timer.h"
#include "interface/Gpio.h"

#include <avr/io.h>
#include <avr/interrupt.h>

#include <array>

namespace driver
{

class TimerDriver : public ITimer
{
public:


    struct TimerConfig
    {
        uint16_t div;
        uint8_t cs;
        uint16_t cnt;
    };

    template<uint32_t Fcpu,
            uint32_t PeriodUs,
            uint16_t MaxCounter = 256>
    static constexpr TimerConfig makeTimerConfig()
    {
        constexpr std::array<uint16_t, 5> prescalers{1, 8, 64, 256, 1024};

        uint8_t cs = 1;

        for (auto div : prescalers)
        {
            uint32_t ticks = Fcpu * PeriodUs / 1'000'000UL / div;

            if (ticks < MaxCounter)
                return {div, cs, static_cast<uint16_t>(ticks - 1)};

            ++cs;
        }

        return {1024, 5,
                static_cast<uint16_t>(
                    Fcpu * PeriodUs / 1'000'000UL / 1024 - 1)};
    }

    enum class P
    {
        Tim0,
        Tim1,
        Tim2,
    };

    TimerDriver(P timer)
        : _timer(timer)
    {
    }

    bool init(TimerConfig cfg)
    {
        // _speed = speed;
        _ms = 0;
        cli();

        if (_timer == P::Tim0)
        {
            TCCR0A |= _BV(WGM01);
            TCCR0B |= _BV(CS01) | _BV(CS00);    // 16 MHz / 64 = 250 kHz
            OCR0A = 250 - 1;
            TIMSK0 |= _BV(OCIE0A);
        }
        else if (_timer == P::Tim1)
        {
            TCCR1A = 0;
            TCCR1B |= _BV(WGM12) | _BV(CS01) | _BV(CS00);    // 16 MHz / 64 = 250 kHz
            OCR1A = 250 - 1;
            TIMSK1 |= _BV(OCIE1A);
        }
        else if (_timer == P::Tim2)
        {
            TCCR2A |=  _BV(WGM21);    // 16 MHz / 64 = 250 kHz
            TCCR2A &= ~_BV(WGM20);
            TCCR2B &= ~_BV(WGM22);
            TCCR2B |=  _BV(CS22);
            TCCR2B &= ~_BV(CS21);
            TCCR2B &= ~_BV(CS20);
            OCR2A = 250 - 1;
            TIMSK2 |= _BV(OCIE2A);
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