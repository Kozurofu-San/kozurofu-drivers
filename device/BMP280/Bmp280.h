#pragma once

#include "Bmp280Const.h"

#include "interface/Temperature.h"
#include "interface/Pressure.h"
#include "interface/Gpio.h"
#include "interface/Timer.h"

#include <cstdint>

/* // Temperature / Humidity sensor

#include "device/DHT22/Dht22.h"

    GpioDriver gpio_tempHum {GPIOB, 8};
    TimerDriver timer_us {TIM3};
    Dht22 tempHum {gpio_tempHum, timer_us};
    
    // Timer
    p.timer_us.init({1, ITimer::Units::us});
    p.timer_us.start();

    // Temperature / Humidity sensor
    p.gpio_tempHum.init(GpioDriver::Mode::OutputPushpull, GpioDriver::Speed::High);
    p.gpio_tempHum.write(1);    // Sleep
    p.tempHum.init();
*/

namespace driver
{

template <typename T>
requires std::same_as<T, II2c> ||
         std::same_as<T, ISpi>
class Bmp280Driver : ITemperature, IPressure
{
    static_assert(std::same_as<T, II2c> || std::same_as<T, ISpi>,
                "Interface must be I2C or SPI");
    public:

    Bmp280Driver(II2c &p)
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
        uint8_t id;
        read(Bmp280::Id, &id, 1);
        if (id != Bmp280::IdValue)
        {
            return false;
        }

        // // Soft reset
        // writeByte(Bmp280::Reset, Bmp280::ResetValue);

        // Read Compensation/Calibration coefficients
        read(Bmp280::CalibrationData, reinterpret_cast<uint8_t*>(&_cal), 24);

        // Configure Sensor Operations
        writeByte(Bmp280::CtrlMeas, Bmp280::Osrs1 << Bmp280::OsrsP | Bmp280::Osrs1 << Bmp280::OsrsT | Bmp280::ModeNormal);
        
        _isInit = true;
        
        return _isInit;
    }

    int16_t getTemperature() override
    {
        uint8_t data[3];
        read(Bmp280::Temp, data, 3);

        const int32_t raw =
            (static_cast<int32_t>(data[0]) << 12) |
            (static_cast<int32_t>(data[1]) << 4)  |
            (static_cast<int32_t>(data[2]) >> 4);

        const int32_t x = raw >> 3;
        const int32_t y = raw >> 4;

        const int32_t var1 =
            ((x - (static_cast<int32_t>(_cal.dig_T1) << 1)) *
            static_cast<int32_t>(_cal.dig_T2)) >> 11;

        const int32_t var2 =
            (((y - static_cast<int32_t>(_cal.dig_T1)) *
            (y - static_cast<int32_t>(_cal.dig_T1))) >> 12) *
            static_cast<int32_t>(_cal.dig_T3) >> 14;

        // t_fine is required for pressure compensation.
        _t_fine = var1 + var2;

        // Temperature in 0.01 degrees Celsius.
        const int32_t temperature =
            (_t_fine * 5 + 128) >> 8;

        // Return temperature in 0.1 degrees Celsius.
        return static_cast<int16_t>(
            temperature >= 0
                ? (temperature + 5) / 10
                : (temperature - 5) / 10);
    }
    
    uint32_t getPressurePa() override
    {
        uint8_t data[3];
        read(Bmp280::Press, data, 3);

        // Convert the 20-bit raw pressure value.
        const int32_t raw =
            (static_cast<int32_t>(data[0]) << 12) |
            (static_cast<int32_t>(data[1]) << 4)  |
            (static_cast<int32_t>(data[2]) >> 4);

        // Pressure compensation according to the BMP280 datasheet.
        int64_t var1 = static_cast<int64_t>(_t_fine) - 128000;

        int64_t var2 =
            var1 * var1 * static_cast<int64_t>(_cal.dig_P6);

        var2 +=
            (var1 * static_cast<int64_t>(_cal.dig_P5)) << 17;

        var2 +=
            static_cast<int64_t>(_cal.dig_P4) << 35;

        var1 =
            ((var1 * var1 * static_cast<int64_t>(_cal.dig_P3)) >> 8) +
            ((var1 * static_cast<int64_t>(_cal.dig_P2)) << 12);

        var1 =
            (((static_cast<int64_t>(1) << 47) + var1) *
            static_cast<int64_t>(_cal.dig_P1)) >> 33;

        if (var1 == 0)
        {
            // Avoid division by zero.
            return 0;
        }

        int64_t p = 1048576LL - raw;

        p =
            (((p << 31) - var2) * 3125LL) / var1;

        var1 =
            (static_cast<int64_t>(_cal.dig_P9) *
            (p >> 13) *
            (p >> 13)) >> 25;

        var2 =
            (static_cast<int64_t>(_cal.dig_P8) * p) >> 19;

        p =
            ((p + var1 + var2) >> 8) +
            (static_cast<int64_t>(_cal.dig_P7) << 4);

        // Pressure in Pa.
        return static_cast<uint32_t>(p);
    }

    uint16_t getPressuremmHg() override
    {
        // Convert Pa to mmHg without floating-point arithmetic.
        return static_cast<uint16_t>(
            (static_cast<uint64_t>(getPressurePa()) * 7500617ULL + 50000000ULL)
            / 100000000ULL
    );
    }

    bool isInit()
    {
        return _isInit;
    }
    
    private:

    T &_p;
    
    int32_t _t_fine = 0;
    
    // 3.11.2 - Correction data
    struct calibration_s
    {
        uint16_t dig_T1;
        int16_t  dig_T2;
        int16_t  dig_T3;
        uint16_t dig_P1;
        int16_t  dig_P2;
        int16_t  dig_P3;
        int16_t  dig_P4;
        int16_t  dig_P5;
        int16_t  dig_P6;
        int16_t  dig_P7;
        int16_t  dig_P8;
        int16_t  dig_P9;
    } _cal;

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
