#pragma once

#include "interface/Log.h"

#include <avr/io.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>

namespace driver
{

class LogDriver : public ILog
{
    public:

    /*
    * @param  portMask: 0xFFFFFFFF to enable all ports
    * @param  cpuCoreFreqHz: CPU core frequency in Hz
    * @param  baudrate: SWO baudrate in Hz
    * @retval None
    * @note   The SWO baudrate must be less than or equal to 2.25MHz for ST-LINK V2
    */
    LogDriver()
    {
    }

    void init()
    {
    }

    void i(const char* message, ...) override
    {
        va_list args;
        va_start(args, message);
        vsprintf(_buffer, message, args);
        va_end(args);
        printString(0, _buffer);
    }

    void w(const char* message, ...) override
    {
        va_list args;
        va_start(args, message);
        vsprintf(_buffer, message, args);
        va_end(args);
        printString(1, _buffer);
    }

    void e(const char* message, ...) override
    {
        va_list args;
        va_start(args, message);
        vsprintf(_buffer, message, args);
        va_end(args);
        printString(2, _buffer);
    }

    void v(uint32_t channel, int32_t value) override
    {

    }
    
    bool readString(char* string) override
    {
        return true;
    }
    
    bool readNumber(int& number) override
    {
        return true;
    }


    private:

    char _buffer[20];

    void printString(uint32_t channel, char *symbol)
    {
        while (*symbol != 0)
        {
            // ITM_SendCharToChannel(channel, *symbol);
            symbol++;
        }
        // ITM_SendCharToChannel(channel, '\n');
    }
    

};

}