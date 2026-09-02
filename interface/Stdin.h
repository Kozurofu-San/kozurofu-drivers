#pragma once

#include <cstdint>

namespace driver
{

class  IStdin
{
    public:

    virtual ~ IStdin() = default;

    virtual bool scan(char* string) = 0;
    virtual bool scan(int& number) = 0;

    virtual bool isInit() = 0;
};

}