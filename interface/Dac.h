#pragma once

#include "interface/VoltageSet.h"
#include <cstdint>
#include <cstddef>
#include <functional>

class IDac : public IVoltageSet
{
    public:

    virtual ~IDac() = default;

};