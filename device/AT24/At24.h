#pragma once

#include "interface/Memory.h"
#include "interface/I2c.h"
#include "interface/Timer.h"

#include <cstdint>

namespace driver
{

class At24 : public IMemory
{
    public:

    At24(II2c &p, ITimer &timer)
        : _p(p), _timer(timer)
    {
    }

    bool init()
    {
        // Init check
        if (!_p.isInit())
        {
            return false;
        }

        // Speed check
        if (_p.getSpeed() > MaxSpeed or _p.getSpeed() == 0)
        {
            return false; // Speed is too high for this memory
        }

        // // Keep the address selected by the application as the first 256-byte
        // // block address. AT24C04 uses the least significant address bit for A8.
        // _baseAddress = _p.getAddress() & 0xFEU;
        _isInit = true;
        return true;
    }

    void write(uint8_t *data, uint32_t address, size_t len) override
    {
        _p.start();
        _p.address(II2c::Cmd::Write | ((address >> 8U) << 1));
        _p.write(address);
        for (uint32_t i = 0; i < len; i++)
        {
            _p.write(data[i]);
        }
        _p.stop();
    }

    void read (uint8_t *data, uint32_t address, size_t len) override
    {
        while (len != 0U)
        {
            // AT24C04 has two 256-byte address blocks: 0x50 and 0x51.
            const size_t bytesToBlockEnd = 256U - (address & 0xFFU);
            const size_t chunk = (len < bytesToBlockEnd) ? len : bytesToBlockEnd;

            _p.start();
            _p.address(II2c::Cmd::Write | ((address >> 8U) << 1));
            _p.write(static_cast<uint8_t>(address));

            // A repeated START must be followed by SLA+R. Without it STM32F1
            // remains in start/address phase and RXNE can never be set.
            _p.start();
            _p.address(II2c::Cmd::Read | ((address >> 8U) << 1));
            for (size_t i = 0; i < chunk; ++i)
            {
                data[i] = _p.read(i == (chunk - 1U));
            }
            _p.stop();

            data += chunk;
            address += static_cast<uint32_t>(chunk);
            len -= chunk;
        }
    }

    bool writeBlock(const uint8_t *data, uint32_t sector, uint32_t len) override
    {
        if (sector >= SectorSize)
        {
            return false;
        }
        _p.start();
        _p.address(II2c::Cmd::Write | (sector << 1));
        _p.write(sector);
        for (uint32_t i = 0; i < len; i++)
        {
            _p.write(data[i]);
        }
        _p.stop();
        _timer.delay(5);
        return true;
    }

    bool readBlock(uint8_t *data, uint32_t sector, uint32_t len) override
    {
        return true;
    }

    void erase() override
    {
    }

    bool eraseSector(uint32_t sector)
    {
        if (sector >= SectorSize)
        {
            return false;
        }
        _p.start();
        _p.address(II2c::Cmd::Write | (sector << 1));
        _p.write(sector);
        for (uint32_t i = 0; i < PageSize; i++)
        {
            _p.write(0xFF);
        }
        _p.stop();
        _timer.delay(5);
        return true;
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

    II2c &_p;
    ITimer &_timer;

    uint8_t _buffer[20];

    bool _isInit = false;
    uint8_t _baseAddress = 0;
    uint32_t _sectorCount = 0;
    static constexpr uint16_t SectorSize = 2;
    static constexpr uint16_t PageSize = 256;
    static constexpr uint32_t MaxSpeed = 400'000;   // Hz
};
}
