#pragma once

#include <cstdint>

namespace driver
{

class IComparator
{
    public:

    enum Comparison: uint8_t
    {
        Equal,
        Higher,
        Lower,
        Error = -1;
    };

    virtual ~IComparator() = default;

    virtual bool start() = 0;

    virtual Comparison compare() = 0;
    
    virtual bool isInit() = 0;
};

}