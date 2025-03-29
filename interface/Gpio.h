#pragma once

class Gpio
{
    public:

    virtual ~Gpio() = default;
    virtual void write(bool state) = 0;
    virtual bool read() = 0;
};