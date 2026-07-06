#pragma once

#include "interface/Communication.h"

#include <xc.h>

namespace driver
{

class UartDriver: public ICommunication
{
    public:

    enum class P: uint32_t
    {
        PortB = 0xBF886040,
        PortC = 0xBF886080,
        PortD = 0xBF8860C0,
        PortE = 0xBF886100,
        PortF = 0xBF886140,
        PortG = 0xBF886180,
    };

    UartDriver(P uart)
        : _uart(uart)
    {
    }

    bool init(uint32_t speed)
    {
        
        _isInit = true;
        return true;
    };
    
    void write(uint8_t *data, size_t len, size_t bytes = 1) override
    {
        for (size_t i = 0; i < len; ++i)
        {
            
        }
    };

    void read(uint8_t *data, size_t len, size_t bytes = 1) override
    {
        static uint16_t ret;
        for (size_t i = 0; i < len; ++i)
        {
            
        }
    };
    
    uint32_t sendCommand(uint32_t cmd) override
    {
        return 0;
    }

    P getInstance()
    {
        return _uart;
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

    P _uart;
    uint32_t _speed;
    bool _isInit = false;
};

}