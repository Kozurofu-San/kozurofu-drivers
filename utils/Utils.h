#pragma once

#include <cstdint>
#include <ctime>


class Utils
{
    public:

    static uint32_t binToBcd(uint32_t bin)
    {
        uint32_t bcd = 0;
        uint32_t shift = 0;
        while (bin)
        {
            bcd |= (bin % 10) << (shift * 4);
            bin /= 10;
            shift++;
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

    static uint32_t binToBcdSimple(uint16_t bin)
    {
        uint32_t bcd = 0;
        for (int i = 0; i < 16; i++) {
            for (int shift = 0; shift < 5; shift++)
            {
                if (((bcd >> (shift * 4)) & 0xF) > 4)
                {
                    bcd += (3 << (shift * 4));
                }
            }
            bcd <<= 1;
            bcd |= (bin >> (15 - i)) & 1;
        }
        return bcd;
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

    
    static void unixDateToDate(time_t unixTime, char *buf)
    {
        struct tm *utc_time_info;
        utc_time_info = gmtime(&unixTime);
        strftime(buf, 40, "%Y-%m-%d %H:%M:%S", utc_time_info);
    }

};