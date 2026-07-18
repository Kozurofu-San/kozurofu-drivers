#pragma once

#ifdef AVR
#include <stdint.h>
#else
#include <cstdint>
#endif

namespace driver
{

class  ILog
{
    public:

    enum {
        I,
        W,
        E
    };

    virtual ~ ILog() = default;
    virtual void print(uint8_t channel, const char* message, ...) = 0;
    virtual void value(uint8_t channel, int32_t value) = 0;
    virtual bool scan(char* string) = 0;
    virtual bool scan(int& number) = 0;
    virtual bool isInit() = 0;
};

}