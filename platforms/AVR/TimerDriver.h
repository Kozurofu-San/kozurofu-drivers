#pragma once

#include "interface/Timer.h"
#include "interface/Gpio.h"

#include <avr/io.h>

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

    TimerDriver(P timer)
        : _timer(timer)
    {
    }

    struct Cfg
    {
        uint8_t cs;
        uint16_t cnt;
    };

    static constexpr uint8_t Prescaler01[] = {0, 3, 6, 8, 10};
    static constexpr uint8_t Prescaler2 [] = {0, 3, 5, 6, 7, 8, 10};
    static constexpr uint16_t MaxCnt02 = (1UL << 8) - 1;
    static constexpr uint16_t MaxCnt1 = (1UL << 16) - 1;

    static Cfg calculatePrescaler(P timer, uint32_t us)
    {
        Cfg cfg = {0, 0};
        uint16_t tick;
        if (timer == P::Tim0)
        {
            for (uint8_t i = 0; i < sizeof(Prescaler01) / sizeof(Prescaler01[0]); i++)
            {
                tick = (F_CPU / 1'000'000UL) * us >> Prescaler01[i];
                if (tick <= MaxCnt02)
                {
                    cfg.cs = i + 1;
                    cfg.cnt = tick - 1;
                    break;
                }
            }
        }
        else if (timer == P::Tim1)
        {
            for (uint8_t i = 0; i < sizeof(Prescaler01) / sizeof(Prescaler01[0]); i++)
            {
                tick = (F_CPU / 1'000'000UL) * us >> Prescaler01[i];
                if (tick <= MaxCnt1)
                {
                    cfg.cs = i + 1;
                    cfg.cnt = tick - 1;
                    break;
                }
            }
        }
        else
        {
            for (uint8_t i = 0; i < sizeof(Prescaler2) / sizeof(Prescaler2[0]); i++)
            {
                tick = (F_CPU / 1'000'000UL) * us >> Prescaler2[i];
                if(tick <= MaxCnt02)
                {
                    cfg.cs = i + 1;
                    cfg.cnt = tick - 1;
                    break;
                }
            }
        }
        return cfg;
    }
    
    bool init(uint32_t us)
    {
        // _speed = speed;
        _ms = 0;
        cli();

        auto cfg = calculatePrescaler(_timer, us);

        if (_timer == P::Tim0)
        {
            TCCR0A |= _BV(WGM01);
            TCCR0B |= cfg.cs;
            OCR0A = cfg.cnt;
            TIMSK0 |= _BV(OCIE0A);
        }
        else if (_timer == P::Tim1)
        {
            TCCR1A = 0;
            TCCR1B |= _BV(WGM12) | cfg.cs;
            OCR1A = cfg.cnt;
            TIMSK1 |= _BV(OCIE1A);
        }
        else if (_timer == P::Tim2)
        {
            TCCR2A |=  _BV(WGM21);
            TCCR2B = cfg.cs;
            OCR2A = cfg.cnt;
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
    uint32_t _ms;
    uint32_t _speed = 0;
    bool _isInit = false;
    void (*_cb)(uint32_t);
};


} // namespace driver