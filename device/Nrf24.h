#pragma once

#include "interface/Communication.h"
#include "interface/Timer.h"
#include "Nrf24Const.h"

#include <cstdint>

namespace driver
{

class Nrf24Driver
{
    public:

    static constexpr uint32_t MaxSpeed = 10000000; // 10 MHz

    Nrf24Driver(ICommunication &p, ITimer &timer)
        : _p(p), _timer(timer)
    {
    }
    ~Nrf24Driver() = default;

    bool init()
    {
        // Init check
        if (!_p.isInit() or !_timer.isInit())
        {
            return false;
        }

        // Speed check
        if (_p.getSpeed() > MaxSpeed or _p.getSpeed() == 0)
        {
            return false; // Speed is too high for this memory
        }

        _isInit = true;

        readCmd(Nrf24::SetupAw, _buffer, 1);
        _isInit &= _buffer[0] == 0x3;
        readCmd(Nrf24::RfCh,    _buffer, 1);
        _isInit &= _buffer[0] == 0x2;
        readCmd(Nrf24::Status,  _buffer, 1);
        _isInit &= _buffer[0] == 0xE;

        return _isInit;
    }

    void write(uint8_t *data, size_t len)
    {
    }

    void read (uint8_t *data, size_t len)
    {
    }

    bool isInit()// override
    {
        return _isInit;
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

    bool _isInit = false;
};

}