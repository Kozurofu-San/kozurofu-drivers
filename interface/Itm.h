#pragma once

#include <cstdint>

namespace driver
{

class IItm
{
    public:

    virtual ~IItm() = default;

    virtual void writeChar(uint8_t channel, char data) = 0;
    virtual void writeInt(uint8_t channel, uint32_t data) = 0;
    // virtual void read () = 0;
    
    virtual uint32_t getSpeed() const = 0;
    virtual bool isInit() = 0;
};

}