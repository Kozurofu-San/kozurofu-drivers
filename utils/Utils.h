#pragma once

#include <cstdint>

class Utils
{
    public:

    static uint32_t binToBcd(uint32_t bin)
    {
        uint32_t bcd = 0;
        uint8_t cnt = 32;
        for (; cnt--; )
        {
            bcd |= (bin % 10) << (32 - 4);
            bin /= 10;
            bcd >>= 4;
        }
        return bcd;
    }

    static uint32_t bcdToBin(uint32_t bcd)
    {
        uint32_t bin = 0;
        uint32_t mul = 1;
        while (bcd)
        {
            bin += (bcd & 0xF) * mul;
            mul *= 10;
            bcd >>= 4;
        }
        return bin;
    }

    static void evenOddToHalf(int16_t p[], size_t len)
    {
        int16_t t;
        for (size_t i = 1; i < len / 2; ++i)
        {
            t = p[i];
            for (size_t j = i + 1; j < len - 1; ++j)
            {
                p[j - 1] = p[j];
            }
            p[len - 2] = t;
        }
    }

    static int32_t mulQ16(int32_t a, int32_t b)
    {
        int64_t temp = static_cast<int64_t>(a) * static_cast<int64_t>(b);
        if ((temp & 0x8000) == 0x8000)
        {
            temp += 0x10000;
        }
        temp >>= 16;
        return temp;
    }

    static int32_t q16ToInt(int32_t a)
    {
        if ((a & 0x8000) == 0x8000)
        {
            a += 0x10000;
        }
        a >>= 16;
        return a;
    }

    static inline int32_t int_to_q16(int32_t a){
        return a << 16;
    }


};