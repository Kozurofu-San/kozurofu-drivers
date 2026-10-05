#pragma once

#include "interface/Adc.h"
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

    QuickCharge(IAdc &usbDm, IAdc &usbDp, ITimer &timer)
        : _usbDm(usbDm), _usbDp(usbDp), _timer(timer)
        {
            init();
        }

    void init()
    {
        // Start Quick Charge
        // _usbDp.setVoltage(3300);
        _timer.delay(1500);
    }

    // Quick Charge 2.0
    void setVoltageFix(Voltage voltage)
    {
        switch (voltage)
        {
            case Voltage::V5_0:
                // _usbDm.setVoltage(0.0f);
                // _usbDp.setVoltage(0.6f);
                break;
            case Voltage::V9_0:
                // _usbDm.setVoltage(0.6f);
                // _usbDp.setVoltage(3.3f);
                break;
            case Voltage::V12_0:
                // _usbDm.setVoltage(0.6f);
                // _usbDp.setVoltage(0.6f);
                break;
            case Voltage::V20_0:
                // _usbDm.setVoltage(3.3f);
                // _usbDp.setVoltage(3.3f);
                break;
        }

    }

    
    private:

    IAdc &_usbDm;
    IAdc &_usbDp;
    ITimer &_timer;
};

}
