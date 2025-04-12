#pragma once

#include "interface/Communication.h"
#include <cstdint>
#include <functional>

class ISpi : ICommunication
{
    public:

    virtual ~ISpi() = default;
};