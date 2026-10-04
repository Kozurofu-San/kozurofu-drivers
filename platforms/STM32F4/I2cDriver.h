#pragma once

#include "interface/I2c.h"
#include "interface/Gpio.h"
#include "GpioDriver.h"

#include "stm32f4xx.h"

namespace driver
{

class I2cController
{

public:

    I2cController(I2C_TypeDef *i2c,
        GPIO_TypeDef *portScl, uint8_t pinScl,
        GPIO_TypeDef *portSda, uint8_t pinSda,
        uint32_t speed)
        : _i2c(i2c)
    {
        GpioDriver::mode(portScl, pinScl, GpioDriver::Mode::OutputOpendrain, GpioDriver::Pull::Up, GpioDriver::Alternate::I2C1_3); // SCL
        GpioDriver::mode(portSda, pinSda, GpioDriver::Mode::OutputOpendrain, GpioDriver::Pull::Up, GpioDriver::Alternate::I2C1_3); // SDA
        init(speed);
    }

    // Speed is a clockrate in Hz
    bool init(uint32_t speed)
    {
        // Clock enable
        if      (_i2c == I2C1) RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
        else if (_i2c == I2C2) RCC->APB1ENR |= RCC_APB1ENR_I2C2EN;

        // Speed calculation
        uint32_t busSpeed = SYSTEM_CORE_CLOCK_HZ;
        uint32_t busDiv = (RCC->CFGR & RCC_CFGR_PPRE1) >> RCC_CFGR_PPRE1_Pos;
        if (busDiv >= 4)
            busSpeed >>= (busDiv - 3);          // divide by 2, 4, 8 or 16
        busSpeed = (busSpeed + 500000) / 1000000;   // round to nearest MHz

        if ((busSpeed < SpeedMin) || (busSpeed > SpeedMax))
        {
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

    inline bool address(uint8_t addressRW)
    {
        if (!_transferOk) return false;
        _i2c->DR = addressRW;
        return _transferOk = waitFor(I2C_SR1_ADDR);
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
        // Do not use CR1.STOP as a completion flag: it is a command bit and
        // may still be set after the bus has become idle.  SR2.BUSY reflects
        // the actual bus state.  A repeated START has no pending STOP and
        // must proceed without this wait.
        if (_i2c->CR1 & I2C_CR1_STOP)
        {
            uint32_t count = Timeout;
            while ((_i2c->SR2 & I2C_SR2_BUSY) != 0U)
            {
                if (--count == 0U)
                {
                    _transferOk = false;
                    return;
                }
            }
            _i2c->CR1 &= ~I2C_CR1_STOP;
        }

        // Error flags from a failed transaction otherwise make every
        // following waitFor() fail immediately.
        _i2c->SR1 &= ~ErrorFlags;
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

    static constexpr uint8_t SpeedMin = 2;   // MHz
    static constexpr uint8_t SpeedMax = 42;  // MHz
    static constexpr uint32_t Timeout = 1'000'000;
    static constexpr uint32_t ErrorFlags = I2C_SR1_AF | I2C_SR1_BERR |
                                           I2C_SR1_ARLO | I2C_SR1_OVR;
};

class I2cDriver: public II2c
{
    public:

    I2cDriver(I2cController &i2c, uint8_t address)
        : _i2c(i2c)
    {
        init(address);
    }

    bool init(uint8_t address)
    {
        _address = address << 1;
        return _i2c.check(address);
    };
    
    inline void start() override
    {
        _i2c.start();
    }

    inline void stop() override
    {
        _i2c.stop();
    }

    inline bool address(bool rw) override
    {
        return _i2c.address(_address | rw);
    }

    inline void write(uint8_t data) override
    {
        _i2c.write(data);
    };

    inline uint8_t read(bool last = false) override
    {
        return _i2c.read(!last);
    };
    
    inline uint32_t getSpeed() const override
    {
        return _i2c.getSpeed();
    }
    inline void setAddress(uint8_t address) override
    {
        _address = address << 1;
    }
    inline uint8_t getAddress() override
    {
        return _address >> 1;
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
