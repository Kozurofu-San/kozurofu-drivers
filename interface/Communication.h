#pragma once

#include <cstdint>
#include <functional>

class ICommunication
{
    public:

    virtual ~ICommunication() = default;

    // For communication
    virtual void write(uint8_t *data, size_t len) = 0;
    virtual void read(uint8_t *data, size_t len) = 0;

    // For device driving
    virtual void sendCommand(uint32_t cmd) = 0;
    virtual void sendData(uint32_t data) = 0;
    virtual uint32_t readData() = 0;
};