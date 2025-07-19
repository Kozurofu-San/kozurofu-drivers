#pragma once

#include "interface/Communication.h"
#include <cstdint>
#include <functional>

class II2c : public ICommunication
{
    public:

    virtual ~II2c() = default;
};