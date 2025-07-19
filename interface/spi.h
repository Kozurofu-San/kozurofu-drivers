#pragma once

#include "interface/Communication.h"
#include <cstdint>
#include <functional>

class ISpi : public ICommunication
{
    public:

    virtual ~ISpi() = default;
};