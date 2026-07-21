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

    static Cfg calculatePrescaler(P timer, const Time &time)
    {
        const uint32_t unitsPerSecond =
            time.unit == ns ? 1'000'000'000UL :
            time.unit == us ? 1'000'000UL :
            time.unit == ms ? 1'000UL : 1UL;
        const uint64_t timerTicks =
            (static_cast<uint64_t>(F_CPU) * time.value) / unitsPerSecond;

        const auto findConfig = [timerTicks](const uint8_t* prescalers, uint8_t count, uint16_t maxCount) -> Cfg
        {
            for (uint8_t i = 0; i < count; ++i)
            {
                const uint32_t ticks = timerTicks >> prescalers[i];
                if (ticks > 0 && ticks <= static_cast<uint32_t>(maxCount) + 1)
                {
                    return {static_cast<uint8_t>(i + 1), static_cast<uint16_t>(ticks - 1)};
                }
            }
            return {0, 0};
        };

        if (timer == P::Tim0)
        {
            return findConfig(Prescaler01, sizeof(Prescaler01) / sizeof(Prescaler01[0]), MaxCnt02);
        }
        else if (timer == P::Tim1)
        {
            return findConfig(Prescaler01, sizeof(Prescaler01) / sizeof(Prescaler01[0]), MaxCnt1);
        }

        return findConfig(Prescaler2, sizeof(Prescaler2) / sizeof(Prescaler2[0]), MaxCnt02);
    }
    
    bool init(Time time)
    {
        _cnt = 0;
        cli();

        auto cfg = calculatePrescaler(_timer, time);
        if (cfg.cs == 0)
        {
            sei();
            return false;
        }

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

    void setPeriod(Time time)
    {

    }
    Time getPeriod()
    {
        return {0, ITimer::s};
    }

    void start() override
    {
        if (!_isInit)
            return;

    }

    void stop() override
    {
        
    }

    void reset() override
    {
        _cnt = 0;
        if      (_timer == P::Tim0) TCNT0 = 0;
        else if (_timer == P::Tim1) TCNT1 = 0;
        else if (_timer == P::Tim2) TCNT2 = 0;
    }

    void delay(uint32_t value) override
    {
        if (!_isInit)
            return;

        reset();
        start();

        _cnt = 0;

        while (_cnt < value)
        {
            asm volatile("nop");
        }
        stop();
    }

    void callback(void (*cb)(uint32_t)) override
    {
        _cb = cb;
    }

    void interrupt()
    {
        _cnt++;
        if (_cb)
        {
            _cb(0);
        }
    }

    uint32_t getSpeed() override
    {
        return _speed;
    }

    bool isInit() override
    {
        return _isInit;
    }

    inline uint32_t now() override
    {
        return _cnt;
    }

private:

    P _timer;
    volatile uint32_t _cnt;
    uint32_t _speed = 0;
    bool _isInit = false;
    void (*_cb)(uint32_t) = nullptr;
};


} // namespace driver
