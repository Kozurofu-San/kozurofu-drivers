#pragma once

#include "interface/Log.h"

#include <cstdio>
#include <cstdarg>

namespace driver
{

class LogDriver : public ILog
{
    public:

    static LogDriver& getInstance()
    {
        static LogDriver instance;
        return instance;
    }

    LogDriver(const LogDriver&) = delete;
    LogDriver& operator=(const LogDriver&) = delete;
    LogDriver(LogDriver&&) = delete;
    LogDriver& operator=(LogDriver&&) = delete;

    bool init()
    {
        return true;
    }

    void i(const char* message, ...) override
    {
        va_list args;
        va_start(args, message);
        printf("I:");
        vsnprintf(_buffer, sizeof(_buffer), message, args);
        printf("%s\n", _buffer);
        va_end(args);
    }

    void w(const char* message, ...) override
    {
        va_list args;
        va_start(args, message);
        printf("W:");
        vsnprintf(_buffer, sizeof(_buffer), message, args);
        printf("%s\n", _buffer);
        va_end(args);
    }

    void e(const char* message, ...) override
    {
        va_list args;
        va_start(args, message);
        printf("E:");
        vsnprintf(_buffer, sizeof(_buffer), message, args);
        printf("%s\n", _buffer);
        va_end(args);
    }

    void v(uint32_t channel, int32_t value) override
    {
        snprintf(_buffer, sizeof(_buffer), "V%ld: %ld", channel, value);
        printf("%s\n", _buffer);
    }
    
    bool readString(char* string) override
    {
        int ret = scanf("%s", string);
        if (ret == 1)
        {
            i(">>> %s", string);
            return true;
        }
        else
        {
            e("Input error");
            return false;
        }
    }
    
    bool readNumber(int& number) override
    {
        int ret = scanf("%d", &number);
        if (ret == 1)
        {
            i(">>> %d", number);
            return true;
        }
        else
        {
            e("Input error");
            return false;
        }
    }

    private:
    LogDriver() = default;
    char _buffer[128];

};

}
