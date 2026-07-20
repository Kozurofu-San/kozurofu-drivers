#pragma once

#include "interface/I2c.h"

#include "stm32f1xx.h"

extern uint32_t SystemCoreClock;

namespace driver
{

class I2cController
{

public:

    I2cController(I2C_TypeDef *i2c)
        : _i2c(i2c)
    {
    }

    bool init(uint32_t speed)
    {
        // Clock enable
        if      (_i2c == I2C1) RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
        else if (_i2c == I2C2) RCC->APB1ENR |= RCC_APB1ENR_I2C2EN;

        // Speed calculation
        uint32_t busPrescalerPos = RCC_CFGR_PPRE1_Pos;
        uint32_t busPrescaler = (RCC->CFGR >> busPrescalerPos) & 0x7;
        busPrescaler = (busPrescaler < 4) ? 1 : (1 << (busPrescaler - 3));
        uint32_t busSpeed = SystemCoreClock / busPrescaler / 1'000'000;
        if ((busSpeed < DivMin) || (busSpeed > DivMax))
        {
            printf("Desired speed %lu is out of limits %u - %u", busSpeed, DivMin, DivMax);
            return false;
        }
        
        // Reset
        I2C1->CR1 = I2C_CR1_SWRST;
        I2C1->CR1 &= ~I2C_CR1_SWRST;

        I2C1->CR2 = busSpeed;       // Set peripheral clock frequency in MHz
        I2C1->CCR = SystemCoreClock / speed / busPrescaler / 2;
        if (speed == 100'000) I2C1->TRISE = busSpeed + 1;
        else if (speed == 400'000) I2C1->TRISE = busSpeed * 300 / 1000 + 1;
        I2C1->CR1 |= I2C_CR1_PE;    // Enable I2C module

        _speed = speed;
        printf("I2C speed %ld\n", _speed);
        _isInit = true;
        return true;
    }

    inline void address(uint8_t addressRW)
    {
        I2C1->DR = addressRW;              // Send Slave Address with Write (0)
        while (!(I2C1->SR1 & I2C_SR1_ADDR));    // Wait for ADDR (Address sent) flag
        // (void)I2C1->SR2;
    }

    void clearFlag()
    {
        // Clear ADDR flag by reading SR1 and SR2
        [[maybe_unused]] volatile uint32_t temp = I2C1->SR1;
        temp = I2C1->SR2;
        while (!(I2C1->SR1 & I2C_SR1_TXE)); // Wait for TXE (Transmitter Empty) flag
    }

    void write(uint8_t data)
    {
        clearFlag();
        I2C1->DR = data;    // Send Data
        while (!(I2C1->SR1 & I2C_SR1_BTF));     // Wait for BTF (Byte Transfer Finished)
    }

    // Read byte and return ACK (continue reading)
    uint8_t read(bool ack)
    {
        I2C1->CR1 &= ~I2C_CR1_ACK;
        I2C1->CR1 |= I2C_CR1_STOP;
        return 0;
    }

    uint32_t getSpeed() const
    {
        return _speed;
    }

    void start()
    {
        I2C1->CR1 |= I2C_CR1_START;         // Generate START condition
        while (!(I2C1->SR1 & I2C_SR1_SB));  // Wait for SB (Start Bit) flag to be set
    }

    inline void stop()
    {
        I2C1->CR1 |= I2C_CR1_STOP;          // Generate STOP condition
    }

    inline bool isInit()
    {
        return _isInit;
    }

    inline I2C_TypeDef* getInstance()
    {
        return _i2c;
    }

    bool check(uint8_t addr)
    {
        bool ret = false;
        printf("I2C address 0x%X ", addr);
        addr <<= 1;
        start();
        address(addr);
        
        if (I2C1->SR1 & I2C_SR1_ADDR) {
            // Device acknowledged! Address is active.
            // Clear ADDR flag by reading SR1 and SR2
            uint32_t temp = I2C1->SR1;
            temp = I2C1->SR2;
            ret = true;
        } else if (I2C1->SR1 & I2C_SR1_AF) {
            // No device answered, NACK received
            // Clear AF flag
            I2C1->SR1 &= ~I2C_SR1_AF;
        }
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

    static constexpr uint8_t DivMin = 2;   // MHz
    static constexpr uint8_t DivMax = 36;  // MHz
    static constexpr uint8_t Timeout = 100; // Times
};

class I2cDriver: public II2c
{
    public:

    I2cDriver(I2cController &i2c)
        : _i2c(i2c)
    {
    }

    bool init(uint8_t address)
    {
        _address = address << 1;
        return true;
    };
    
    inline void start() override
    {
        _i2c.start();
    }

    inline void stop() override
    {
        _i2c.stop();
    }

    inline void address(bool rw) override
    {
        _i2c.address(_address | rw);
    }

    inline void write(uint8_t data) override
    {
        _i2c.write(data);
    };

    uint8_t read() override
    {
        return 0;
    };
    
    inline uint32_t getSpeed() const override
    {
        return _i2c.getSpeed();
    }
    void setAddress(uint8_t address) override
    {
        _address = address;
    }
    uint8_t getAddress() override
    {
        return _address;
    }

    inline bool isInit() override
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