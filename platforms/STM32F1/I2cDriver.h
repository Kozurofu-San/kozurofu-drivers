#pragma once

#include "interface/Communication.h"

#include "stm32f1xx.h"

extern uint32_t SystemCoreClock;

namespace driver
{

class I2cController
{

public:

    struct Cfg
    {
        uint8_t divider;
        uint32_t baudrate;
    };

    I2cController(I2C_TypeDef *i2c)
        : _i2c(i2c)
    {
    }

    bool init(uint32_t speed)
    {

        _speed = 0;
        printf("I2C speed %ld\n", _speed);
        _isInit = true;
        return true;
    }

    void write(uint8_t data)
    {
        
        wait();
    }

    // Read byte and return ACK (continue reading)
    uint8_t readAck()
    {
        
        wait();
        return 0;
    }

    // Read byte and return NACK (stop reading)
    uint8_t readNack()
    {
        
        wait();
        return 0;
    }

    uint32_t getSpeed() const
    {
        return _speed;
    }

    void start()
    {
        
        wait(); // Wait for transmission to complete
    }

    inline void stop()
    {
        
    }

    inline bool isInit()
    {
        return _isInit;
    }

    inline I2C_TypeDef* getInstance()
    {
        return _i2c;
    }

    bool check(uint8_t address)
    {
        bool ret = false;
        printf("I2C address 0x%X ", address);
        address <<= 1;
        start();
        write(address);
        //
        stop();
        printf("%d\n", ret);
        return ret;
    }

private:

    static bool wait()
    {
        uint16_t count = 0;
        while (false)
        {
            if (++count > Timeout)
            {
                return false;
            }
        }
        return true;
    }

    I2C_TypeDef *_i2c;
    uint32_t _speed = 0;
    bool _isInit = false;

    static constexpr uint8_t Timeout = 100;
};

class I2cDriver: public ICommunication
{
    public:

    I2cDriver(I2cController &i2c)
        : _i2c(i2c)
    {
    }

    bool init(uint8_t address)
    {
        
        _address = address;
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
        return _i2c.getSpeed();
    }

    void enable() override
    {
    }

    void disable() override
    {
    }

    bool isInit() override
    {
        return _i2c.isInit();
    }
    
    I2C_TypeDef* getInstance()
    {
        return _i2c.getInstance();
    }

    private:

    I2cController &_i2c;
    uint8_t _address = 0;
};

}