#pragma once

#include "interface/Gpio.h"

// #include "esp_system.h"
#include "driver/gpio.h"

class GpioDriver : public Gpio
{
private:
    uint32_t _pin;
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


    GpioDriver(size_t pin, Mode mode, Pull pull)
        : _pin(pin)
    {
        gpio_config_t io_conf;
        io_conf.intr_type = GPIO_INTR_DISABLE; // disable interrupt
        io_conf.mode = GPIO_MODE_OUTPUT; // set as output mode
        io_conf.pin_bit_mask = 1 << _pin; // bit mask of the pins that you want to set, eg. GPIO18
        io_conf.pull_down_en = (pull == Pull::Down) ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE; // disable pull-down mode
        io_conf.pull_up_en = (pull == Pull::Up) ?  GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE; // enable pull-up mode
        gpio_config(&io_conf); // configure GPIO with the given settings
    };
    ~GpioDriver() = default;

    void init(){
    }

    void write(bool state) override
    {
        gpio_set_level(static_cast<gpio_num_t>(_pin), state);
    }

    bool read() override
    {
        return gpio_get_level(static_cast<gpio_num_t>(_pin));
    }

};