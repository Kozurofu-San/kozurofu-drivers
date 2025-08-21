#pragma once

#include "interface/Memory.h"
#include "interface/Communication.h"
#include "interface/Timer.h"
#include "ExternalMemoryConst.h"

#include <cstdint>
#include <functional>

namespace driver
{

class ExternalMemoryDriver : public IMemory
{
    public:

    static constexpr uint32_t MaxSpeed = 133000000; // 133 MHz for W25Q16
    static constexpr uint8_t Manufacturer = 0xEF;

    ExternalMemoryDriver(ICommunication &p, ITimer &timer)
        : _p(p), _timer(timer)
    {
    }
    ~ExternalMemoryDriver() = default;

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

        readCmd(ExternalMemory::Instruction::JedecId, _buffer, 4);
        _manufacturerId = _buffer[0];
        _type = _buffer[1];
        _sectorCount = _buffer[2];
        _sectorCount = (1 << _sectorCount) / SectorSize;
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

        if (_manufacturerId == Manufacturer)
        {
            _isInit = true;
        }
        
        return _isInit;
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

    bool writeBlock(const uint8_t *data, uint32_t sector, uint32_t len) override
    {
        return true; // Not implemented
    }

    bool readBlock(uint8_t *data, uint32_t sector, uint32_t len) override
    {
        return true; // Not implemented
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

    void eraseSector(uint32_t sector)
    {
        readCmd(ExternalMemory::Instruction::WriteEnable, _buffer, 0);

        _p.enable();
        _p.sendCommand(ExternalMemory::Instruction::SectorErase4Kb);
        _p.disable();
        do
        {
            readCmd(ExternalMemory::Instruction::ReadStatusRegister1, _buffer, 1);
        } while (_buffer[0] & ExternalMemory::Status::Busy);
        readCmd(ExternalMemory::Instruction::WriteDisable, _buffer, 0);
    }

    bool isInit() override
    {
        return _isInit;
    }
    
    uint32_t getSectorCount() override
    {

        return _sectorCount;
    }

    uint32_t getSectorSize() override
    {

        return SectorSize;
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
    uint64_t _uniqueId = 0;

    bool _isInit = false;
    static constexpr uint16_t SectorSize = 512;
    uint32_t _sectorCount = 0;

    uint8_t cash[4096];
};
}