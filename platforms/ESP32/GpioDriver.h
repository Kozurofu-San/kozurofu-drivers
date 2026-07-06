#pragma once

#include "interface/Gpio.h"

// #include "esp_system.h"
#include "driver/gpio.h"

namespace driver
{

class GpioDriver : public IGpio
{
    public:

    enum class Mode: uint32_t
    {
        Input = GPIO_MODE_INPUT,
        OutputPushpull = GPIO_MODE_OUTPUT,
        OutputOpendrain = GPIO_MODE_OUTPUT_OD
    };

    enum class Pull: uint32_t
    {
        None = 0,
        Up = 1,
        Down = 2
    };


    GpioDriver(uint32_t pin)
        : _pin(static_cast<gpio_num_t>(pin))
    {
    }

    bool init(Mode mode, Pull pull, gpio_int_type_t intr_type)
    {
        gpio_config_t io_conf {
            .pin_bit_mask = 1ULL << _pin,
            .mode = static_cast<gpio_mode_t>(mode),
            .pull_up_en   = (pull == Pull::Up)   ? GPIO_PULLUP_ENABLE   : GPIO_PULLUP_DISABLE,
            .pull_down_en = (pull == Pull::Down) ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE,
            .intr_type = intr_type
        };
        gpio_config(&io_conf);
        if (intr_type != GPIO_INTR_DISABLE)
        {
            gpio_install_isr_service(0);
            // gpio_isr_handler_add(static_cast<gpio_num_t>(_pin), [](void* arg) {
            //     auto gpio = static_cast<GpioDriver*>(arg);
            //     if (gpio->_callback.has_value())
            //     {
            //         gpio->_callback.value()();
            //     }
            // }, this);
        }
        _isInit = true;
        return _isInit;
    }

    inline size_t getPin() override
    {
        return _pin;
    }

    void write(bool state) override
    {
        gpio_set_level(_pin, state);
    }

    bool read() override
    {
        return gpio_get_level(_pin);
    }

    void callback(void (*cb)(uint32_t)) override
    {
        _cb = cb;
    }
    
    inline bool isInit() override
    {
        return _isInit;  // Not implemented
    }

    private:

    gpio_num_t _pin;
    bool _isInit = false;
    void (*_cb)(uint32_t) = nullptr;
};

}