#pragma once

#include <cstdint>

class IDateTime
{
    public:

    enum class Week: uint8_t
    {
        Monday = 1,
        Tuesday,
        Wednesday,
        Thursday,
        Friday,
        Saturday,
        Sunday,
    };

    struct DateTime
    {
        uint8_t year  : 6;
        uint8_t month : 4;
        Week    week  : 3; 
        uint8_t date  : 5;
        uint8_t hour  : 5;
        uint8_t min   : 6;
        uint8_t sec   : 6;
    };

    virtual ~IDateTime() = default;

    virtual void getTime(DateTime *dateTime) = 0;
    virtual void setTime(DateTime *dateTime) = 0;
    
    virtual uint32_t getSpeed() const = 0;
};