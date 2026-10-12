#pragma once

#include "Const.h"

#include "interface/I2c.h"
#include "interface/Temperature.h"
#include "interface/Humidity.h"
#include "interface/Air.h"
#include "interface/Gpio.h"
#include "interface/Timer.h"

#include <concepts>
#include <cstdint>

/* // Air quality sensor

#include "device/DHT22/Dht22.h"

    I2cController i2c2 {I2C2};
    I2cDriver i2c_tempPress {i2c2};
    Bmp280Driver<II2c> tempPress {i2c_tempPress};
    
    // I2C
    GpioDriver::remap(AFIO_MAPR_I2C1_REMAP, false);
    GpioDriver::mode(GPIOB, 11, GpioDriver::Mode::AlternateOpendrain);   // SDA
    GpioDriver::mode(GPIOB, 10, GpioDriver::Mode::AlternateOpendrain);   // SCL
    CHECK(p.i2c2.init(400'000));

    // Temperature / Pressure sensor
    p.i2c2.check(II2c::Address::BMP280 + 1);
    p.i2c_tempPress.init(II2c::Address::BMP280 + 1);
    p.tempPress.init();
*/

namespace driver
{

template <typename T>
requires std::same_as<T, II2c> ||
         std::same_as<T, ISpi>
class Ens160Driver : public IAir
{
    static_assert(std::same_as<T, II2c> || std::same_as<T, ISpi>,
                "Interface must be I2C or SPI");
    public:

    Ens160Driver(T &p, ITimer &timer)
        : _p(p), _timer(timer)
    {
        init();
    }

    bool init()
    {
        // Init check
        if (!_p.isInit() && _p.getSpeed() > MaxSpeed)
        {
            return false;
        }

        // Get ID
        uint8_t id[2];
        if (!read(Ens160::PART_ID, id, sizeof(id)))
        {
            return false;
        }
        if ((static_cast<uint16_t>(id[1] << 8) | id[0]) != Ens160::PART_ID_VALUE)
        {
            return false;
        }
 
        // Idle -> clear general purpose registers -> standard operation
        if (!setMode(Ens160::OPMODE_IDLE))
        {
            return false;
        }
        if (!writeByte(Ens160::COMMAND, Ens160::CMD_NOP) ||
            !writeByte(Ens160::COMMAND, Ens160::CMD_CLRGPR))
        {
            return false;
        }
        if (!setMode(Ens160::OPMODE_STANDARD))
        {
            return false;
        }
 
        _isInit = true;
        return _isInit;
    }

    // Air Quality Index (UBA): 1 = excellent ... 5 = unhealthy, 0 on error
    uint16_t getAqi() override
    {
        uint8_t v;
        if (!_isInit || !read(Ens160::DATA_AQI, &v, 1))
        {
            return 0;
        }
        return readByte(Ens160::DATA_AQI) & Ens160::AQI_MASK;
    }
 
    // TVOC in ppb, 0 on error
    uint16_t getTvoc() override
    {
        return readWord(Ens160::DATA_TVOC);
    }
 
    // Equivalent CO2 in ppm, 0 on error
    uint16_t getEco2() override
    {
        return readWord(Ens160::DATA_ECO2);
    }
 
    bool isInit()
    {
        return _isInit;
    }
    
    private:

    T &_p;
    ITimer &_timer;
    
    bool _isInit = false;

    static constexpr uint32_t MaxSpeed = std::same_as<T, II2c> ? 1'000'000u : 10'000'000u;
    static const size_t Timeout = 10;

    // Set OPMODE and verify by reading it back
    bool setMode(uint8_t mode)
    {
        if (!writeByte(Ens160::OPMODE, mode))
        {
            return false;
        }
 
        for (size_t i = 0; i < Timeout; i++)
        {
            _timer.delay(1);
 
            uint8_t v;
            if (read(Ens160::OPMODE, &v, 1) && v == mode)
            {
                return true;
            }
        }
        return false;
    }
 
    // Write single byte
    bool writeByte(uint8_t addr, uint8_t data)
    {
        if constexpr (std::same_as<T, II2c>)
        {
            _p.start();
            _p.address(II2c::Write);
            _p.write(addr);
            _p.write(data);
            _p.stop();
        }
        else if constexpr (std::same_as<T, ISpi>)
        {
            _p.enable();
            _p.transfer(addr);     // addr[7] = 0 - write
            _p.transfer(data);
            _p.disable();
        }

        return true;
    }
    
    // Read single byte
    uint8_t readByte(uint8_t addr)
    {
        uint8_t ret;
        if constexpr (std::same_as<T, II2c>)
        {
            _p.start();
            _p.address(II2c::Write);
            _p.write(addr);
            _p.start();
            _p.address(II2c::Write);
            ret =_p.read();
            _p.stop();
        }
        else if constexpr (std::same_as<T, ISpi>)
        {
            _p.enable();
            _p.transfer(0x80 | addr);     // addr[7] = 1 - read
            ret = _p.transfer(0);
            _p.disable();
        }

        return ret;
    }

    // Read little-endian 16-bit register, 0 on error
    uint16_t readWord(uint8_t addr)
    {
        uint8_t b[2];
        if (!_isInit || !read(addr, b, sizeof(b)))
        {
            return 0;
        }
        return static_cast<uint16_t>(b[1] << 8) | b[0];
    }
 
    
    // Read multiple bytes
    bool read(uint8_t addr, uint8_t *data, uint8_t len)
    {
        if constexpr (std::same_as<T, II2c>)
        {
            _p.start();
            _p.address(II2c::Write);
            _p.write(addr);
            _p.start();
            _p.address(II2c::Read);
            while (len--)
            {
                *data++ = _p.read(!len);
            }
            _p.stop();
        }
        else if constexpr (std::same_as<T, ISpi>)
        {
            _p.enable();
            _p.write(0x80 | addr);     // addr[7] = 1 - read
            while (len--)
            {
                *data++ = _p.transfer(0);
            }
            _p.disable();
        }

        return true;
    }
    
};
}
