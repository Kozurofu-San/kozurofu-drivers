#pragma once

#include <cstdint>

namespace driver
{

class IRtt
{
    public:

    virtual ~IRtt() = default;

    virtual void writeChar(uint8_t channel, char data) = 0;
    virtual void writeInt(uint8_t channel, uint32_t data) = 0;
    // virtual void read () = 0;
    
    virtual bool isInit() = 0;
};

}