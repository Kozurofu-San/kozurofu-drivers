#pragma once

#include <cstdint>
#include <cstddef>

namespace driver
{

class IGpio
{
    public:

    enum Direction: uint8_t
    {
        Input,
        Output
    };

    enum Pull: uint8_t
    {
        None,
        Up,
        Down
    };

    enum class Interrupt: uint8_t
    {
        None,
        Rise,
        Fall,
        RiseFall
    };

    virtual ~IGpio() = default;
    
    virtual void write(bool state) = 0;
    virtual bool read() = 0;
    virtual size_t getPin() = 0;
    virtual void setDir(Direction dir) = 0;

    virtual bool setCallback(void (*cb)(uint32_t), Interrupt edge = Interrupt::None) = 0;
    virtual bool isInit() = 0;
};

}
