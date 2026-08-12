#pragma once

#include "interface/Memory.h"
#include "interface/Spi.h"
#include "interface/Timer.h"
#include "ExternalMemoryConst.h"

#include <cstdint>

namespace driver
{

class ExternalMemoryDriver : public IMemory
{
    public:

    static constexpr uint32_t MaxSpeed = 133000000; // 133 MHz for W25Q16
    static constexpr uint8_t Manufacturer = 0xEF;

    ExternalMemoryDriver(ISpi &p, ITimer &timer)
        : _p(p), _timer(timer)
    {
    }
    ~ExternalMemoryDriver() = default;

    // // Cashe management for FS
    // void cacheLoad(uint32_t addr);
    // void cacheWrite(uint32_t addr, const uint8_t *data, uint32_t len);
    // void flash_cache_read(uint32_t addr, uint8_t *data, uint32_t len);
    // void flash_cache_flush();

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

        while (len > 0)
        {
            size_t page_off = address % PageSize;
            size_t chunk = PageSize - page_off;
            if (chunk > len) chunk = len;

            readCmd(ExternalMemory::Instruction::WriteEnable, _buffer, 0);
            _p.enable();
            _p.sendCommand(ExternalMemory::Instruction::PageProgram);
            _p.write((uint8_t*)&address, addressBytes);
            _p.write(data, chunk);
            _p.disable();
            
            do
            {
                readCmd(ExternalMemory::Instruction::ReadStatusRegister1, _buffer, 1);
            } while (_buffer[0] & ExternalMemory::Status::Busy);

            read(_cache2, address, chunk);
            for (size_t i = 0; i < chunk; i++)
            {
                if (data[i] != _cache2[i])
                {
                    // Error
                    while (1);
                }
            }

            address += chunk;
            data    += chunk;
            len     -= chunk;            
        }
        readCmd(ExternalMemory::Instruction::WriteDisable, _buffer, 0);

    }

    void read (uint8_t *data, uint32_t address, size_t len) override
    {
        uint8_t addressBytes = ExternalMemory::HighCap ? 4 : 3;

        _p.enable();
        _p.sendCommand(ExternalMemory::Instruction::ReadData);
        _p.write((uint8_t*)&address, addressBytes);
        _p.read(data, len);
        _p.disable();
        
    }

    bool writeBlock(const uint8_t *data, uint32_t sector, uint32_t len) override
    {
        uint32_t address = sector * SectorSize;
        for (size_t i = 0; i < len; ++i)
        {
            cacheWrite(address, data, SectorSize);
            address += SectorSize;
            data += SectorSize;
        }
        return true;
    }

    bool readBlock(uint8_t *data, uint32_t sector, uint32_t len) override
    {
        uint32_t address = sector * SectorSize;
        for (size_t i = 0; i < len; ++i)
        {
            cacheRead(address, data, SectorSize);
            address += SectorSize;
            data += SectorSize;
        }
        return true;
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
        _p.write((uint8_t*)&sector, ExternalMemory::HighCap ? 4 : 3);
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

    void cacheFlush()
    {
        if (_cacheDirty)
        {
            eraseSector(_cacheAddr);
            write(_cache, _cacheAddr, CacheSize);
            _cacheDirty = false;
        }
    }

    private:

    void readCmd(uint8_t cmd, uint8_t *data, size_t len)
    {
        _p.enable();
        _p.sendCommand(cmd);
        _p.read(data, len);
        _p.disable();
    }

    ISpi &_p;
    ITimer &_timer;

    uint8_t _buffer[20];

    uint8_t _manufacturerId = 0;
    uint8_t _type = 0;
    uint64_t _uniqueId = 0;

    bool _isInit = false;
    static constexpr uint16_t SectorSize = 512;
    static constexpr uint16_t PageSize = 256;
    uint32_t _sectorCount = 0;

    static constexpr uint16_t CacheSize = 4096; // Cache size for read/write operations
    uint8_t _cache[CacheSize];
    uint8_t _cache2[CacheSize];
    uint32_t _cacheAddr = 0xFFFFFFFF;
    bool _cacheDirty = false;

    void cacheLoad(uint32_t addr)
    {
        uint32_t blockAddr = addr & ~0xFFF;  // align to 4 KB

        if (_cacheDirty)
        {
            // Flush old block
            eraseSector(_cacheAddr);
            write(_cache, _cacheAddr, CacheSize);
            read(_cache2, _cacheAddr, CacheSize);
            for (size_t i = 0; i < CacheSize; i++)
            {
                if (_cache[i] != _cache2[i])
                {
                    // Error
                    while (1);
                }
            }
            _cacheDirty = false;
        }

        // Read new block
        read(_cache, blockAddr, CacheSize);
        _cacheAddr = blockAddr;
    }

    void cacheWrite(uint32_t addr, const uint8_t *data, uint32_t len)
    {
        uint32_t blockAddr = addr & ~0xFFF;

        if (_cacheAddr != blockAddr)
        {
            cacheLoad(addr);
        }

        uint32_t offset = addr - _cacheAddr;
        memcpy(&_cache[offset], data, len);
        _cacheDirty = true;
    }

    void cacheRead(uint32_t addr, uint8_t *data, uint32_t len)
    {
        uint32_t blockAddr = addr & ~0xFFF;

        if (_cacheAddr == blockAddr)
        {
            memcpy(data, &_cache[addr - _cacheAddr], len);
        } else {
            read(data, addr, len);
        }
    }

};
}