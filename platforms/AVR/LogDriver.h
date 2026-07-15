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

    /*
    * @param  portMask: 0xFFFFFFFF to enable all ports
    * @param  cpuCoreFreqHz: CPU core frequency in Hz
    * @param  baudrate: SWO baudrate in Hz
    * @retval None
    * @note   The SWO baudrate must be less than or equal to 2.25MHz for ST-LINK V2
    */
    static LogDriver& getInstance()
    {
        static LogDriver instance;
        return instance;
    }

    LogDriver(const LogDriver&) = delete;
    LogDriver& operator=(const LogDriver&) = delete;
    LogDriver(LogDriver&&) = delete;
    LogDriver& operator=(LogDriver&&) = delete;

    void init(ICommunication *uart)
    {
        _uart = uart;
    }

    void i(const char* message, ...) override
    {
        va_list args;
        va_start(args, message);
        vsnprintf(_buffer, sizeof(_buffer), message, args);
        va_end(args);
        printString(0, _buffer);
    }

    void w(const char* message, ...) override
    {
        va_list args;
        va_start(args, message);
        vsnprintf(_buffer, sizeof(_buffer), message, args);
        va_end(args);
        printString(1, _buffer);
    }

    void e(const char* message, ...) override
    {
        va_list args;
        va_start(args, message);
        vsnprintf(_buffer, sizeof(_buffer), message, args);
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

    bool printChar(char c)
    {
        _uart->write(reinterpret_cast<uint8_t*>(c), 1);
    }

    private:

    LogDriver() = default;
    ICommunication *_uart;
    char _buffer[60];
    const uint8_t crlf[2] = {'\r', '\n'};

    void printString(uint32_t channel, char *symbol)
    {
        while (*symbol != 0)
        {
            _uart->write(reinterpret_cast<uint8_t*>(symbol), 1);
            symbol++;
        }
        _uart->write(const_cast<uint8_t*>(crlf), 2);
    }
};

}
