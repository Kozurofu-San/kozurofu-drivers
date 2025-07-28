#pragma once

#include "interface/Memory.h"
#include "interface/Communication.h"
#include "interface/Timer.h"
#include "ExternalMemoryConst.h"

#include <cstdint>
#include <functional>

class ExternalMemoryDriver : public IMemory
{
    public:
    ExternalMemoryDriver(ICommunication &p, ITimer &timer)
        : _p(p), _timer(timer)
    {
    }
    ~ExternalMemoryDriver() = default;

    void init()
    {
        _p.enable();
        _p.sendCommand(ExternalMemory::JedecId);
        _p.read(buffer, 4);
        _p.disable();
        _p.sendCommand(ExternalMemory::EnableReset);
        _p.sendCommand(ExternalMemory::ResetDevice);
        _p.disable();
        _timer.delay(100);
    }

    void write(uint8_t *data, uint32_t address, size_t len) override{
        
    }

    void read (uint8_t *data, uint32_t address, size_t len) override{
        _p.sendCommand(ExternalMemory::ReadData);
        _p.write((uint8_t*)&address, 4);
    }

    private:

    ICommunication &_p;
    ITimer &_timer;

    uint8_t buffer[10];
};