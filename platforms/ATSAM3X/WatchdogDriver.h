#pragma once

#include "interface/Timer.h"

#include "asf.h"
#include "component/component_wdt.h"
#include "wdt/wdt.h"

namespace driver
{

class WatchdogDriver : public ITimer
{
    public:

    WatchdogDriver(Wdt* timer)
        : _timer(timer) {}

    bool init(uint32_t periodMs)
    {
        if (periodMs == 0 || periodMs > 16000) {
            return false;
        }

        // Period = (WDV + 1) * 128 / 32768 * 1000 ms
        _speed = 32768UL;
        uint32_t wdv = (periodMs * _speed) / (128 * 1000UL);

        if (wdv == 0) wdv = 1;
        if (wdv > 0xFFF) wdv = 0xFFF;   // Max ~16 sec

        _ms = (wdv + 1) * 128 * 1000 / 32768;  // Real period [ms]

        // Config WDT_MR:
        // WDV       = period
        // WDD       = the same value (turn off window mode)
        // WDRSTEN   = 1  → Reset after timeout
        // WDRPROC   = 0  → Reset of processor
        // WDDBGHLT  = 1  → halt in debug
        // WDIDLEHLT = 1  → halt in idle
        // WDDIS     = 0  → watchdog on

        uint32_t mode =
            WDT_MR_WDRSTEN |            // Reset enable
            WDT_MR_WDDBGHLT |           // Halt in debug
            WDT_MR_WDIDLEHLT;           // Halt in idle
            
        wdt_init(_timer, mode, (uint16_t)wdv, (uint16_t)wdv);

        _isInit = true;
        return _isInit;
    }

    void start() override
    {
        reset();
    }

    void stop() override
    {
        wdt_disable(_timer);
        _isInit = false;
    }

    void reset() override
    {
        if (!_isInit) return;
        wdt_restart(_timer);
    }

    void delay(uint32_t ms) override
    {
        
    }

    uint32_t now() override
    {
        return 0;
    }
    
    void callback(void (*cb)(uint32_t)) override
    {
        
    }
    
    uint32_t getSpeed() override
    {
        return _speed;
    }

    bool isInit() override
    {
        return _isInit;
    }

private:

    Wdt* _timer;
    uint32_t _ms = 0;
    uint32_t _speed = 0;

    bool _isInit = false;
    void (*_cb)(uint32_t) = nullptr;
};

}