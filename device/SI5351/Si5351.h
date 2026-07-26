#pragma once

#include "Si5351Const.h"

#include "interface/Generator.h"
#include "interface/Timer.h"
#include "interface/I2c.h"

namespace driver
{

class Si5351Driver: public IGenerator
{
    public:

    Si5351Driver(II2c &p, ITimer &timer)
        : _p(p), _timer(timer) {}

    bool init()
    {
        // Init check
        if (!_p.isInit() || !_timer.isInit())
        {
            return false;
        }

        // ID
        uint8_t id = read(Si5351::DeviceStatus);

        // Init sequence

        _isInit = true;
        return true;
    }
    
    void setFrequency(size_t channel, uint32_t frequency) override
    {
    }

    uint32_t getFrequency(size_t channel) override
    {
        return 0;
    }

    void setPower(size_t channel, int32_t power) override
    {
    }

    int32_t getPower(size_t channel) override
    {
        return 0;
    }

    bool isInit() override
    {
        return _isInit;
    }

    private:

    II2c &_p;
    ITimer& _timer;

    bool _isInit = false;

    uint8_t read(uint8_t reg)
    {
        _p.start();
        _p.address(II2c::Cmd::Write);
        _p.write(reg);
        _p.start();
        _p.address(II2c::Cmd::Read);
        uint8_t ret = _p.read(true);
        _p.stop();
        return ret;
    }
};

}
