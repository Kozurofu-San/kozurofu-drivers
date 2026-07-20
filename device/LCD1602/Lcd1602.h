#pragma once

#include "Lcd1602Const.h"

#include "interface/Log.h"
#include "interface/Gpio.h"
#include "interface/Timer.h"
#include "interface/Communication.h"
#include "interface/I2c.h"

#include <stdint.h>

namespace driver
{

template <typename T>
requires std::same_as<T, II2c> ||
        std::same_as<T, ICommunication>
class Lcd1602Driver: public ILog
{
    static_assert(std::same_as<T, II2c> || std::same_as<T, ICommunication>,
                  "Interface must be I2C or Parallel");
    public:

    explicit Lcd1602Driver(T &p, ITimer &timer, IGpio *backlight = nullptr)
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
        _bl = Lcd1602::BL;
        // _bl = 0;

        // Init sequence
        _timer.delay(50);

        writeNibble(Type::Cmd, 0x3);
        _timer.delay(5);

        writeNibble(Type::Cmd, 0x3);
        _timer.delay(150);
        
        writeNibble(Type::Cmd, 0x3);
        writeNibble(Type::Cmd, 0x2);

        write(Type::Cmd, Lcd1602::FunctionSet | Lcd1602::DisplaySwitch); // 0x28
        write(Type::Cmd, Lcd1602::DisplaySwitch);                        // 0x08
        write(Type::Cmd, Lcd1602::ScreenClear);                          // 0x01
        _timer.delay(2);

        write(Type::Cmd, Lcd1602::InputSet | Lcd1602::CursorReturn);     // 0x06
        write(Type::Cmd, Lcd1602::InputSet | Lcd1602::DisplaySwitch);    // 0x0C

        _isInit = true;
        return true;
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
        return false;
    }
    bool scan([[maybe_unused]] int& number) override
    {
        return false;
    }

    bool isInit() override
    {
        return _isInit;
    }

    private:

    enum class Type: uint8_t
    {
        Cmd  = 0x00,
        Data = Lcd1602::RS
    };

    void writePins(uint8_t data)
    {
        if constexpr (std::same_as<T, II2c>)
        {
            _p.start();
            _p.address(II2c::Cmd::Write);
            _p.write(data);
            _p.stop();
        }
        else if constexpr (std::same_as<T, ICommunication>)
        {
            _p.write(data);
        }
    }

    // Write one nibble
    void writeNibble(Type isData, uint8_t nibble)
    {
        uint8_t data = ((nibble & 0xF) << 4) | _bl | static_cast<uint8_t>(isData);
        // writePins(data | Lcd1602::EN);
        writePins(data);
        writePins(data | Lcd1602::EN);
        writePins(data);
    }

    // Write cmd or data
    void write(Type isData, uint8_t data)
    {
        writeNibble(isData, data >> 4);
        writeNibble(isData, data & 0xF);
    }

    void printString(uint8_t col, char *str)
    {
        write(Type::Cmd, Lcd1602::DdramAdSet | rowOffsets[col]);
        while (*str)
        {
            write(Type::Data, *str++);
        }
    }

    static constexpr uint8_t rowOffsets[] =
        {
            0x00,
            0x40
        };

    T &_p;
    ITimer& _timer;
    IGpio* _backlight;
    uint8_t _bl;

    char _buffer[20];

    bool _isInit = false;
};

}
