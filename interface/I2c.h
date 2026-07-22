#pragma once

#include <cstdint>
#include <cstddef>

namespace driver
{

class II2c
{
    public:

    enum Address
    {
        PCF8574 = 0x27,
        AT24    = 0x50,
        SI5351  = 0x60,
        ADS1115 = 0x48,
    };

    enum Cmd: bool
    {
        Read = 1,
        Write = 0,
    };

    virtual ~II2c() = default;

    virtual void start()  = 0;
    virtual void stop() = 0;

    virtual void address(bool cmd) = 0;
    virtual void write(uint8_t data) = 0;
    virtual uint8_t read(bool last = false) = 0;

    virtual void setAddress(uint8_t address) = 0;
    virtual uint8_t getAddress() = 0;

    virtual uint32_t getSpeed() const = 0;
    virtual bool isInit() = 0;
};

}