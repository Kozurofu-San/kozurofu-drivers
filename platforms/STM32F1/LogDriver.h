#pragma once

#include "interface/Log.h"
#include "interface/Itm.h"
#include "interface/Rtt.h"
#include "interface/Uart.h"

#include <cstdarg>
#include <cstdio>
#include <cstdint>
#include <concepts>

#include <stm32f1xx.h>

namespace driver
{

template <typename T>
requires std::same_as<T, IItm> ||
         std::same_as<T, IRtt> ||
         std::same_as<T, IUart>
class LogDriver : public ILog
{
    static_assert(std::same_as<T, IItm> || std::same_as<T, IRtt> || std::same_as<T, IUart>,
                "Interface must be ITM, RTT or UART");

    public:

    LogDriver(T &p)
        : _p(p)
    {}

    bool init()
    {
        if (!_p.isInit())
        {
            return false;
        }
        return true;
    }

    void print(uint8_t channel, const char* message, ...) override
    {
        #ifdef LOG
        va_list args;
        va_start(args, message);
        uint32_t len = vsnprintf(_buffer, sizeof(_buffer), message, args);
        va_end(args);
        printString(channel, _buffer);
        if (len > sizeof(_buffer))
        {
            printString(ILog::E, ErrorMsg);
            return;
        }
        #endif
    }

    void value(uint8_t channel, int32_t value) override
    {
        #ifdef LOG
        if constexpr (std::same_as<T, IItm>)
        {
            _p.writeInt(channel, value);
        }
        #endif
    }

    bool scan(char* string) override
    {
        return true;
    }
    
    bool scan(int& number) override
    {
        return true;
    }

    bool isInit() override
    {
        return true;
    }

    private:

    T &_p;
    char _buffer[80];
    const char ErrorMsg[16] = "Buffer overflow";

    void printString(uint8_t channel, const char *symbol)
    {
        while (*symbol != 0)
        {
            if constexpr (std::same_as<T, IItm> || std::same_as<T, IRtt>)
            {
                _p.writeChar(channel, *symbol);
            }
            symbol++;
        }
        if constexpr (std::same_as<T, IItm> || std::same_as<T, IRtt>)
        {
            _p.writeChar(channel, '\n');
        }
    }

};

}
