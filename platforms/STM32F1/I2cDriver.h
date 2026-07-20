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
        
        if (speed == 0 || speed > 400'000)
        {
            return false;
        }

        // Reset and configure the selected peripheral.
        _i2c->CR1 = I2C_CR1_SWRST;
        _i2c->CR1 = 0;
        _i2c->CR2 = busSpeed;       // Peripheral clock frequency in MHz
        if (speed <= 100'000)
        {
            _i2c->CCR = (busSpeed * 1'000'000U) / (speed * 2U);
            _i2c->TRISE = busSpeed + 1U;
        }
        else
        {
            _i2c->CCR = I2C_CCR_FS | ((busSpeed * 1'000'000U) / (speed * 3U));
            _i2c->TRISE = (busSpeed * 300U) / 1000U + 1U;
        }
        _i2c->CR1 = I2C_CR1_PE;     // Enable I2C module

        _speed = speed;
        printf("I2C speed %ld\n", _speed);
        _isInit = true;
        return true;
    }

    inline void address(uint8_t addressRW)
    {
        if (!_transferOk) return;
        _i2c->DR = addressRW;
        _transferOk = waitFor(I2C_SR1_ADDR);
    }

    void clearFlag()
    {
        // ADDR is cleared only by the SR1, then SR2 read sequence.
        if (_i2c->SR1 & I2C_SR1_ADDR)
        {
            [[maybe_unused]] volatile uint32_t temp = _i2c->SR1;
            temp = _i2c->SR2;
        }
    }

    void write(uint8_t data)
    {
        if (!_transferOk) return;
        clearFlag();
        _i2c->DR = data;
        _transferOk = waitFor(I2C_SR1_BTF);
    }

    // Read byte and return ACK (continue reading)
    uint8_t read(bool ack)
    {
        if (!_transferOk) return 0;
        if (ack) _i2c->CR1 |= I2C_CR1_ACK;
        else     _i2c->CR1 &= ~I2C_CR1_ACK;
        clearFlag();
        if (!ack) stop();
        if (!waitFor(I2C_SR1_RXNE)) return 0;
        return _i2c->DR;
    }

    uint32_t getSpeed() const
    {
        return _speed;
    }

    void start()
    {
        _transferOk = true;
        _i2c->CR1 |= I2C_CR1_START;
        _transferOk = waitFor(I2C_SR1_SB);
    }

    inline void stop()
    {
        _i2c->CR1 |= I2C_CR1_STOP;
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
        printf("I2C address 0x%X ", addr);
        addr <<= 1;
        start();
        address(addr);
        const bool ret = _transferOk;
        if (ret) clearFlag();
        stop();
        _i2c->SR1 &= ~ErrorFlags;
        printf("%d\n", ret);
        return ret;
    }

private:

    bool waitFor(uint32_t flag)
    {
        uint32_t count = Timeout;
        while ((_i2c->SR1 & flag) == 0U)
        {
            if ((_i2c->SR1 & ErrorFlags) != 0U || --count == 0U)
            {
                return false;
            }
        }
        return true;
    }

    I2C_TypeDef *_i2c;
    uint32_t _speed = 0;
    bool _isInit = false;
    bool _transferOk = false;

    static constexpr uint8_t DivMin = 2;   // MHz
    static constexpr uint8_t DivMax = 36;  // MHz
    static constexpr uint32_t Timeout = 1'000'000;
    static constexpr uint32_t ErrorFlags = I2C_SR1_AF | I2C_SR1_BERR |
                                           I2C_SR1_ARLO | I2C_SR1_OVR;
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
        return _i2c.read(false);
    };
    
    inline uint32_t getSpeed() const override
    {
        return _i2c.getSpeed();
    }
    void setAddress(uint8_t address) override
    {
        _address = address << 1;
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
