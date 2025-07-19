#pragma once

#include "interface/Voltage.h"
#include <cstdint>
#include <functional>

class IPwm : public IVoltage
{
    public:

    virtual ~IPwm() = default;
};