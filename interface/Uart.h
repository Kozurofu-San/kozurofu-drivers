#pragma once

#include "interface/Communication.h"
#include <cstdint>
#include <functional>

class IUart : ICommunication
{
    public:

    virtual ~IUart() = default;
    
    virtual void callback(void cb(uint32_t)) = 0;
};