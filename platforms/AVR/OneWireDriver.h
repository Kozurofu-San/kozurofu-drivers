#pragma once

#include "interface/Communication.h"
#include "interface/Gpio.h"
#include "interface/Timer.h"

#include <avr/io.h>

#define ONEWIRE_BAUDRATE ((F_CPU / (UART_SPEED * 8)) - 1)

namespace driver
{

class OneWireDriver: public ICommunication
{
    public:

    OneWireDriver(IGpio &gpio)
        : _gpio(gpio)
    {
    }

    bool init(ITimer *timer = nullptr)
    {
        _timer = timer;

        _speed = 0;
        _isInit = true;
        return true;
    };
    
    void write(uint8_t *data, size_t len, [[maybe_unused]] size_t bytes = 1) override
    {
    };

    void read(uint8_t *data, size_t len, [[maybe_unused]] size_t bytes = 1) override
    {
    };
    
    uint32_t sendCommand([[maybe_unused]] uint32_t cmd) override
    {
        return 0;
    }

    uint32_t getSpeed() const override
    {
        return _speed;
    }

    void enable() override
    {
    }

    void disable() override
    {
    }

    bool isInit() override
    {
        return _isInit;
    }
    

    private:

    IGpio &_gpio;
    ITimer *_timer = nullptr;
    uint32_t _speed = 0;
    bool _isInit = false;
};

}
