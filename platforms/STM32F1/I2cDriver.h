#pragma once

#include "interface/Communication.h"

#include "stm32f1xx.h"
extern uint32_t SystemCoreClock;

namespace driver
{

class I2cDriver: public ICommunication
{
    public:

    I2cDriver(I2C_TypeDef *i2c)
        : _i2c(i2c)
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
        for (size_t i = 0; i < len; ++i)
        {
            
        }
    };
    
    
    uint32_t sendCommand(uint32_t cmd) override
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
    
    I2C_TypeDef* getInstance()
    {
        return _i2c;
    }

    private:

    I2C_TypeDef* _i2c;
    uint32_t _speed;

    bool _isInit = false;
};

}