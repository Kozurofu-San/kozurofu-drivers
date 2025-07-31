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

    static constexpr uint32_t MaxSpeed = 133000000; // 133 MHz for W25Q16

    ExternalMemoryDriver(ICommunication &p, ITimer &timer)
        : _p(p), _timer(timer)
    {
    }
    ~ExternalMemoryDriver() = default;

    void init()
    {
        if (_p.getSpeed() > MaxSpeed or _p.getSpeed() == 0)
        {
            return; // Speed is too high for this memory
        }
        readCmd(ExternalMemory::Instruction::JedecId, _buffer, 4);
        _manufacturerId = _buffer[0];
        _type = _buffer[1];
        _capacity = _buffer[2];
        readCmd(ExternalMemory::Instruction::ReadUniqueId, _buffer, 12);
        _uniqueId = 
            static_cast<uint64_t>(_buffer[5]) << 40 |
            static_cast<uint64_t>(_buffer[6]) << 32 |
            static_cast<uint64_t>(_buffer[7]) << 24 |
            static_cast<uint64_t>(_buffer[8]) << 16 |
            static_cast<uint64_t>(_buffer[9]) << 8  |
            static_cast<uint64_t>(_buffer[10]);
        readCmd(ExternalMemory::Instruction::EnableReset, _buffer, 1);
        readCmd(ExternalMemory::Instruction::ResetDevice, _buffer, 1);
        _timer.delay(100);
    }

    void write(uint8_t *data, uint32_t address, size_t len) override
    {
        uint8_t addressBytes = ExternalMemory::HighCap ? 4 : 3;

        readCmd(ExternalMemory::Instruction::WriteEnable, _buffer, 0);
        _p.enable();
        _p.sendCommand(ExternalMemory::Instruction::PageProgram);
        _p.write((uint8_t*)&address, addressBytes);
        _p.write(data, len);
        _p.disable();
        
        do
        {
            readCmd(ExternalMemory::Instruction::ReadStatusRegister1, _buffer, 1);
        } while (_buffer[0] & ExternalMemory::Status::Busy);
        readCmd(ExternalMemory::Instruction::WriteDisable, _buffer, 0);

    }

    void read (uint8_t *data, uint32_t address, size_t len) override
    {
        uint8_t addressBytes = ExternalMemory::HighCap ? 4 : 3;

        _p.enable();
        _p.sendCommand(ExternalMemory::Instruction::ReadData);
        _p.write((uint8_t*)&address, addressBytes);
        _p.read(data, 20);
        _p.disable();
        
    }

    void erase() override
    {
        readCmd(ExternalMemory::Instruction::WriteEnable, _buffer, 0);

        _p.enable();
        _p.sendCommand(ExternalMemory::Instruction::ChipErase);
        _p.disable();
        do
        {
            readCmd(ExternalMemory::Instruction::ReadStatusRegister1, _buffer, 1);
        } while (_buffer[0] & ExternalMemory::Status::Busy);
        readCmd(ExternalMemory::Instruction::WriteDisable, _buffer, 0);
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