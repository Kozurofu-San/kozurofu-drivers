#pragma once

#include "interface/Communication.h"
#include "interface/Timer.h"
#include "Nrf24Const.h"

#include <cstdint>
#include <functional>

class Nrf24Driver
{
    public:

    static constexpr uint32_t MaxSpeed = 10000000; // 10 MHz

    Nrf24Driver(ICommunication &p, ITimer &timer)
        : _p(p), _timer(timer)
    {
    }
    ~Nrf24Driver() = default;

    void init()
    {
        if (_p.getSpeed() > MaxSpeed or _p.getSpeed() == 0)
        {
            return; // Speed is too high for this memory
        }


        readCmd(Nrf24::SetupAw, _buffer, 1);
        readCmd(Nrf24::RfCh,    _buffer, 1);
        readCmd(Nrf24::Status,  _buffer, 1);
    }

    void write(uint8_t *data, size_t len)
    {
    }

    void read (uint8_t *data, size_t len)
    {
    }

    private:

    void readCmd(uint8_t cmd, uint8_t *data, size_t len)
    {
        _p.enable();
        _p.sendCommand(cmd);
        _p.read(data, len);
        _p.disable();
    }

    ICommunication &_p;
    ITimer &_timer;

    uint8_t _buffer[20];

    uint8_t _manufacturerId = 0;
    uint8_t _type = 0;
    uint8_t _capacity = 0;
    uint64_t _uniqueId = 0;
};