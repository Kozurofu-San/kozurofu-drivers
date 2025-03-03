#pragma once

class GpioDriver
{
    public:

    virtual ~GpioDriver() = default;
    virtual void init() = 0;
    virtual void gpioWrite(bool state) = 0;
    virtual bool gpioRead() = 0;
};