#pragma once

#include "Const.h"

#include "interface/I2c.h"
#include "interface/Adc.h"
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

class Ads1115Controller
{
    public:

    Ads1115Controller(II2c &p)
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

        if (_p.getSpeed() > MaxSpeed)
        {
            return false;
        }

        if (read(Ads1115::Config) == 0x8583)
        {
            _isInit = true;
        }

        _config = 
            Ads1115::OS                 |   // Start a single conversion (when in power-down state)
            2 << Ads1115::PGA_Pos       |   // FSR = ±2.048V
            4 << Ads1115::DR_Pos        |   // 128SPS
            3 << Ads1115::COMP_QUE_Pos  ;   // Disable comparator and set ALERT/RDY pin to high-impedance
        write(Ads1115::Config, _config);
        
        return _isInit;
    }

    bool start()
    {
        // TODO: Start conversion
        return true;
    }

    uint16_t getRawValue(uint8_t channel)
    {
        write(Ads1115::Config, _config | ((channel + 4) << Ads1115::MUX_Pos));
        return read(Ads1115::Conversion);
    }
    
    bool isInit()
    {
        return _isInit;
    }
    
    private:

    II2c &_p;
    
    bool _isInit = false;

    uint16_t _config;
    static const uint32_t MaxSpeed = 3'400'000;

    bool write(uint8_t addr, uint16_t data)
    {
        _p.start();
        _p.address(II2c::Write);
        _p.write(addr);
        _p.write(data >> 8);
        _p.write(data);
        _p.stop();
        return true;
    }
    
    uint16_t read(uint8_t addr)
    {
        _p.start();
        _p.address(II2c::Write);
        _p.write(addr);
        _p.start();
        _p.address(II2c::Read);
        uint16_t ret = static_cast<uint16_t>(_p.read()) << 8;
        ret |= _p.read(true);
        // _p.stop();
        return ret;
    }
    
};

class Ads1115Driver : public IAdc
{
    public:

    Ads1115Driver(Ads1115Controller &p, uint8_t channel)
        : _p(p), _channel(channel)
    {
    }

    bool init()
    {
        // Init check
        if (!_p.isInit())
        {
            return false;
        }

        return _isInit;
    }

    bool start() override
    {
        return _p.start();
    }

    uint16_t getRawValue() override
    {
        return _p.getRawValue(_channel);
    }
    
    uint32_t getVoltage() override
    {
        return _p.getRawValue(_channel) * 3300 / 65535;
    }

    bool isInit() override
    {
        return _isInit;
    }
    
    private:

    Ads1115Controller &_p;
    
    bool _isInit = false;
    uint8_t _channel;

};

}
