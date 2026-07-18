#pragma once

#include "interface/Log.h"
#include "interface/Communication.h"

#include <avr/io.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>

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

    void init(ICommunication *uart = nullptr)
    {
        _uart = uart;
    }

    void print(uint8_t channel, const char* message, ...) override
    {
        va_list args;
        va_start(args, message);
        vsnprintf(_buffer, sizeof(_buffer), message, args);
        va_end(args);
        printString(channel, _buffer);
    }

    void value([[maybe_unused]] uint8_t channel, [[maybe_unused]] int32_t value) override
    {

    }
    
    bool scan([[maybe_unused]] char* string) override
    {
        return true;
    }
    
    bool scan([[maybe_unused]] int& number) override
    {
        return true;
    }
    
    bool isInit() override
    {
        return true;
    }

    void printChar(char c)
    {
        _uart->write(reinterpret_cast<uint8_t*>(c), 1);
    }

    private:

    LogDriver() = default;
    ICommunication *_uart;
    char _buffer[60];
    const uint8_t crlf[6] = {'\r', '\n', 'I', 'W', 'E', ':'};

    void printString(uint8_t channel, char *symbol)
    {
        if (channel < 3)
        {
            _uart->write(const_cast<uint8_t*>(&crlf[channel + 2]), 1);
            _uart->write(const_cast<uint8_t*>(&crlf[5]), 1);
        }
        while (*symbol != 0)
        {
            _uart->write(reinterpret_cast<uint8_t*>(symbol), 1);
            symbol++;
        }
        _uart->write(const_cast<uint8_t*>(crlf), 2);
    }
};

}
