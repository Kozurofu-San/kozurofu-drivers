#pragma once

#include "interface/Voltage.h"
#include "interface/Timer.h"

namespace driver
{

class QuickCharge
{
    public:

    enum class Voltage
    {
        V5_0,
        V9_0,
        V12_0,
        V20_0,
    };

    QuickCharge(IVoltage &usbDm, IVoltage &usbDp, ITimer &timer)
        : _usbDm(usbDm), _usbDp(usbDp), _timer(timer) {}

    void init()
    {
        // Initialize USB D+ and D- lines
        _usbDm.setVoltage(0.0f);
        _usbDp.setVoltage(0.0f);
        
    }

    void setVoltage(Voltage voltage)
    {

    }

    

    private:

    IVoltage &_usbDm;
    IVoltage &_usbDp;
    ITimer &_timer;
};

}
