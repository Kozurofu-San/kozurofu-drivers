#pragma once

#include "interface/Timer.h"

#include "esp_task_wdt.h"

namespace driver
{

class WatchdogDriver : public ITimer
{
    public:

    WatchdogDriver() {}

    bool init(uint32_t periodMs)
    {
        esp_task_wdt_config_t twdt_config = {
            .timeout_ms = periodMs,
            .idle_core_mask = (1 << CONFIG_FREERTOS_NUMBER_OF_CORES) - 1,    // Bitmask of all cores
            .trigger_panic = false,
        };
        ESP_ERROR_CHECK(esp_task_wdt_init(&twdt_config));

        _isInit = true;
        return _isInit;
    }

    void start() override
    {
        reset();
    }

    void stop() override
    {
        esp_task_wdt_deinit();
        _isInit = false;
    }

    void reset() override
    {
        if (!_isInit) return;
        esp_task_wdt_reset();
    }

    // Delay implementation depends on whether we use polling or RTOS delay
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

    uint32_t _ms = 0;
    uint32_t _speed = 0;

    bool _isInit = false;
    void (*_cb)(uint32_t) = nullptr;
};

}