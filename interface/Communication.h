#pragma once

// #include <cstdint>
// #include <functional>


namespace driver
{

class ICommunication
{
    public:

    virtual ~ICommunication() = default;

    // For communication
    virtual void write(uint8_t *data, size_t len, size_t bytes = 1) = 0;
    virtual void read (uint8_t *data, size_t len, size_t bytes = 1) = 0;

    // For device driving
    virtual uint32_t sendCommand(uint32_t cmd) = 0;
    virtual void enable()  = 0;
    virtual void disable() = 0;

    virtual uint32_t getSpeed() const = 0;
    virtual bool isInit() = 0;
};

}