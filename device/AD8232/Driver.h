#pragma once

#include "interface/Ecg.h"
#include "interface/Adc.h"
#include "interface/Timer.h"
#include "interface/Gpio.h"

#include <cstdint>

/*

// ECG sensor

#include "device/AT24/At24.h"

    I2cController i2c1 {I2C1};
    I2cDriver i2c_mem {i2c1};
    At24 mem {i2c_mem, timer};
    
    // I2C
    GpioDriver::remap(AFIO_MAPR_I2C1_REMAP, false);
    GpioDriver::mode(GPIOB, 7, GpioDriver::Mode::AlternateOpendrain);   // SDA
    GpioDriver::mode(GPIOB, 6, GpioDriver::Mode::AlternateOpendrain);   // SCL
    CHECK(p.i2c1.init(100'000));

    // EEPROM
    CHECK(p.i2c1.check(II2c::Address::AT24));
    CHECK(p.i2c_mem.init(II2c::Address::AT24));
    CHECK(p.mem.init());
*/

namespace driver
{

class Ad8232Driver: public IEcg
{
    public:

    Ad8232Driver(IAdc &adc, IGpio &loP, IGpio &loN, ITimer &timer)
        : _adc(adc), _loP(loP), _loN(loN), _timer(timer)
    {
    }

    bool init(IGpio *sdn = nullptr)
    {
        _sdn = sdn;

        // Init check
        if (!_adc.isInit())
        {
            return false;
        }


        _isInit = true;
        return true;
    }

    int16_t getSample() override
    {
        _adc.start();
        return _adc.getRawValue();
    }

    bool isConnected() override
    {
        if (_loP.read() | _loN.read())
        {
            return false;
        }
        return true;
    }

    bool isInit() override
    {
        return _isInit;
    }
    
    private:

    IAdc &_adc;     // Output
    IGpio &_loP;    // LO-
    IGpio &_loN;    // LO+
    ITimer &_timer; // ms

    IGpio *_sdn;

    bool _isInit = false;
};
}
