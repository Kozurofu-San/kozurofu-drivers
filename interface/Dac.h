#pragma once

#include <cstdint>
#include <cstddef>
#include <functional>

class IDac
{
    public:

    virtual ~IDac() = default;

    virtual void setVoltage(float voltage) = 0;
};