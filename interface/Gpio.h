#pragma once

class IGpio
{
    public:

    virtual ~IGpio() = default;
    virtual void write(bool state) = 0;
    virtual bool read() = 0;
};