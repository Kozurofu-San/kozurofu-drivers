#pragma once

#ifdef AVR
#include <stdint.h>
#else
#include <ctdint>
#endif

namespace driver
{

class  ILog
{
    public:

    virtual ~ ILog() = default;
    virtual void i(const char* message, ...) = 0;
    virtual void w(const char* message, ...) = 0;
    virtual void e(const char* message, ...) = 0;
    virtual void v(uint32_t channel, int32_t value) = 0;
    virtual bool readString(char* string) = 0;
    virtual bool readNumber(int& number) = 0;
    virtual bool isInit() = 0;
};

}