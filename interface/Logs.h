#pragma once

#include <cstdint>

namespace driver
{

class  ILogs
{
    public:

    virtual ~ ILogs() = default;
    virtual void i(const char* message, ...) = 0;
    virtual void w(const char* message, ...) = 0;
    virtual void e(const char* message, ...) = 0;
    virtual void v(uint32_t channel, int32_t value) = 0;
    virtual bool readString(char* string) = 0;
    virtual bool readNumber(int& number) = 0;
};

}