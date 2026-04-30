#pragma once

#include "interface/Timer.h"

#include "driver/gptimer.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace driver
{

class TimerDriver : public ITimer
{
    public:

    enum class Mode
    {
        Normal = 0,
        Pwm = 1,
    };

    TimerDriver() = default;

    bool init(Mode mode, uint32_t periodMs)
    {
        if (mode != Mode::Normal)
        {
            return false;
        }

        if (periodMs == 0)
        {
            periodMs = 1;
        }

        _periodMs = periodMs;
        _ms = 0;

        if (_timer != nullptr)
        {
            stop();
            gptimer_disable(_timer);
            gptimer_del_timer(_timer);
            _timer = nullptr;
        }

        gptimer_config_t timer_config = {
            .clk_src = GPTIMER_CLK_SRC_DEFAULT,
            .direction = GPTIMER_COUNT_UP,
            .resolution_hz = 1000000,
        };
        if (gptimer_new_timer(&timer_config, &_timer) != ESP_OK)
        {
            return false;
        }

        gptimer_event_callbacks_t callbacks = {
            .on_alarm = &TimerDriver::onAlarm,
        };
        if (gptimer_register_event_callbacks(_timer, &callbacks, this) != ESP_OK)
        {
            gptimer_del_timer(_timer);
            _timer = nullptr;
            return false;
        }

        gptimer_alarm_config_t alarm_config = {
            .alarm_count = static_cast<uint64_t>(_periodMs) * 1000ULL,
            .reload_count = 0,
            .flags = {
                .auto_reload_on_alarm = true,
            },
        };
        if (gptimer_set_alarm_action(_timer, &alarm_config) != ESP_OK)
        {
            gptimer_del_timer(_timer);
            _timer = nullptr;
            return false;
        }

        if (gptimer_enable(_timer) != ESP_OK)
        {
            gptimer_del_timer(_timer);
            _timer = nullptr;
            return false;
        }

        _speed = timer_config.resolution_hz;
        _isInit = true;
        return true;
    }

    inline void clearInterrupt()
    {
    }

    inline void start() override
    {
        if (_isInit && _timer != nullptr)
        {
            gptimer_start(_timer);
        }
    }

    inline void stop() override
    {
        if (_isInit && _timer != nullptr)
        {
            gptimer_stop(_timer);
        }
    }

    void reset() override
    {
        _ms = 0;
        if (_isInit && _timer != nullptr)
        {
            gptimer_set_raw_count(_timer, 0);
        }
    }

    void delay(uint32_t ms) override
    {
        if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
        {
            vTaskDelay(pdMS_TO_TICKS(ms));
        }
        else
        {
            esp_rom_delay_us(ms * 1000U);
        }
    }

    inline uint32_t now() override
    {
        return _ms;
    }
    
    void callback(void (*cb)(uint32_t)) override
    {
        _cb = cb;
    }
    
    void interrupt()
    {
        _ms += _periodMs;
        if (_cb != nullptr)
        {
            _cb(_ms);
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

    private:

    static bool onAlarm(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
    {
        (void)timer;
        (void)edata;
        auto *self = static_cast<TimerDriver *>(user_ctx);
        if (self != nullptr)
        {
            self->interrupt();
        }
        return false;
    }

    gptimer_handle_t _timer = nullptr;
    uint32_t _periodMs = 1;
    uint32_t _ms = 0;
    uint32_t _speed = 0;

    bool _isInit = false;
    void (*_cb)(uint32_t) = nullptr;
};

}