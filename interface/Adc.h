#pragma once

#include "interface/VoltageGet.h"
#include <cstdint>
#include <cstddef>
#include <functional>

class IAdc : public IVoltageGet
{
    public:

    virtual ~IAdc() = default;

};