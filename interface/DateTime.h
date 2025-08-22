#pragma once

#include <cstdint>
#include <ctime>

class IDateTime
{
    public:

    virtual ~IDateTime() = default;

    virtual time_t now() = 0;
    virtual void setTime(struct tm *t) = 0;
    
    virtual uint32_t getSpeed() const = 0;
    virtual bool isInit() = 0;
};