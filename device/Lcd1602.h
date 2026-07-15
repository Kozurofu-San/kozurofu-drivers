#pragma once

#include "Lcd1602Const.h"

#include "interface/Log.h"
#include "interface/Gpio.h"
#include "interface/Timer.h"
#include "interface/Communication.h"

#include <stdint.h>

namespace driver
{

class Lcd1602Driver: public ILog
{
    public:

    static constexpr uint32_t MaxSpeed = 400'000;   // Hz

    Lcd1602Driver(ICommunication &p, ITimer &timer, IGpio *backlight = nullptr)
        : _p(p), _timer(timer), _backlight(backlight) {}

    bool init()
    {
        // Init check
        if (!_p.isInit() && !_timer.isInit())
        {
            return false;
        }

        // Backlight
        if (_backlight)
        {
            _backlight->write(1);
        }

        // Init sequence
        _timer.delay(50);

        writeNibble(0x3, 0);
        _timer.delay(5);

        writeNibble(0x3, 0);
        _timer.delay(150);
        
        writeNibble(0x3, 0);
        writeNibble(0x2, 0);

        writeCmdData(0x28, 0);
        writeCmdData(0x08, 0);
        writeCmdData(0x01, 0);
        _timer.delay(2);

        writeCmdData(0x06, 0);
        writeCmdData(0x0C, 0);

        _isInit = true;
        return true;
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

    void e([[maybe_unused]] const char* message, ...) override
    {
    }
    
    void v([[maybe_unused]] uint32_t channel, [[maybe_unused]] int32_t value) override
    {

    }
    
    bool readString([[maybe_unused]] char* string) override
    {
        return false;
    }
    bool readNumber([[maybe_unused]] int& number) override
    {
        return false;
    }

    bool isInit() override
    {
        return _isInit;
    }

    private:

    // Write to LCD GPIO 
    void write(uint8_t data)
    {
        _p.enable(); // Start
        _p.sendCommand(0); // Send address, RW = 0
        _p.write(&data, 1); // Write data
        _p.disable(); // Stop
    }

    // Write one nibble
    void writeNibble(uint8_t nibble, bool isData)
    {
        uint8_t data = ((nibble & 0xF) << 4) | Lcd1602::BL | isData;
        write(data);
        write(data | Lcd1602::EN);
        write(data);
    }

    // Write cmd or data
    void writeCmdData(uint8_t data, bool isData)
    {
        writeNibble(data >> 4, isData);
        writeNibble(data & 0xF, isData);
    }

    void printString(uint8_t col, char *str)
    {
        static const uint8_t rowOffsets[] =
        {
            0x00,
            0x40
        };
        writeCmdData(0x80 | rowOffsets[col], 0);
        while (*str)
        {
            writeCmdData(*str++, 1);
        }
    }

    ICommunication& _p;
    ITimer& _timer;
    IGpio* _backlight;

    char _buffer[20];

    bool _isInit = false;
};

}