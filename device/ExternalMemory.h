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

    static constexpr uint32_t MaxSpeed = 133'000'000; // 133 MHz for W25Q16

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
        readCmd(ExternalMemory::Instruction::JedecId, buffer, 4);
        uint8_t manufacturerId = buffer[0];
        uint8_t type = buffer[1];
        uint8_t capacity = buffer[2];
        readCmd(ExternalMemory::Instruction::ReadUniqueId, buffer, 12);
        uint64_t uniqueId = 
            static_cast<uint64_t>(buffer[5]) << 40 |
            static_cast<uint64_t>(buffer[6]) << 32 |
            static_cast<uint64_t>(buffer[7]) << 24 |
            static_cast<uint64_t>(buffer[8]) << 16 |
            static_cast<uint64_t>(buffer[9]) << 8  |
            static_cast<uint64_t>(buffer[10]);
        readCmd(ExternalMemory::Instruction::EnableReset, buffer, 1);
        readCmd(ExternalMemory::Instruction::ResetDevice, buffer, 1);
        _timer.delay(100);
    }

    void write(uint8_t *data, uint32_t address, size_t len) override
    {
        uint8_t addressBytes = ExternalMemory::HighCap ? 4 : 3;

        readCmd(ExternalMemory::Instruction::WriteEnable, buffer, 0);
        _p.enable();
        _p.sendCommand(ExternalMemory::Instruction::PageProgram);
        _p.write((uint8_t*)&address, addressBytes);
        _p.write(data, len);
        _p.disable();
        
        do
        {
            readCmd(ExternalMemory::Instruction::ReadStatusRegister1, buffer, 1);
        } while (buffer[0] & ExternalMemory::Status::Busy);
        readCmd(ExternalMemory::Instruction::WriteDisable, buffer, 0);

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
        readCmd(ExternalMemory::Instruction::WriteEnable, buffer, 0);

        _p.enable();
        _p.sendCommand(ExternalMemory::Instruction::ChipErase);
        _p.disable();
        do
        {
            readCmd(ExternalMemory::Instruction::ReadStatusRegister1, buffer, 1);
        } while (buffer[0] & ExternalMemory::Status::Busy);
        readCmd(ExternalMemory::Instruction::WriteDisable, buffer, 0);
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

    uint8_t buffer[20];
};