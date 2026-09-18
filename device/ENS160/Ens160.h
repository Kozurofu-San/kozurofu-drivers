#pragma once

#include "Ens160Const.h"

#include "interface/I2c.h"
#include "interface/Temperature.h"
#include "interface/Humidity.h"
#include "interface/Air.h"
#include "interface/Gpio.h"
#include "interface/Timer.h"

#include <cstdint>

/* // Temperature / Pressure sensor

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
class Ens160Driver
{
    static_assert(std::same_as<T, II2c> || std::same_as<T, ISpi>,
                "Interface must be I2C or SPI");
    public:

    Ens160Driver(T &p)
        : _p(p)
    {
    }

    bool init()
    {
        // Init check
        if (!_p.isInit())
        {
            return false;
        }

        // Get ID
        
        _isInit = true;
        
        return _isInit;
    }

    bool isInit()
    {
        return _isInit;
    }
    
    private:

    T &_p;
    
    bool _isInit = false;

    static const size_t Timeout = 10;

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
            _p.write(0x80 & addr);     // addr[7] = 1 - read
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
