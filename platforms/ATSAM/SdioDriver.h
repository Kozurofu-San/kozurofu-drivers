#pragma once

#include "interface/Memory.h"
#include "interface/Timer.h"

#include <cstdio>

#include "asf.h"
extern uint32_t SystemCoreClock;

namespace driver
{

class SdioDriver : public IMemory
{
    public:

    enum class Response: uint16_t
    {
        None  = 0,
        Short = 0,
        Long  = 0,
    };

    enum class Bits: uint8_t
    {
        b1,
        b4
    };

    enum class Cmd: uint8_t
    {
        GoIdleState         = 0,
        SendOpCond          = 1,
        AllSendCid          = 2,
        SetRelativeAddr     = 3,
        SetBusWidth         = 6,
        SwitchFunc          = 6,
        SelectDeselectCard  = 7,
        SendIfConf          = 8,
        SendCsd             = 9,
        SendCid             = 10,
        StopTransmission    = 12,
        SendStatus          = 13,
        BlockLength         = 16,
        ReadSingleBlock     = 17,
        ReadMultipleBlock   = 18,
        WriteBlock          = 24,
        WriteMultipleBlock  = 25,
        SdAppOpCond         = 41,
        IoRwDirect          = 52,
        AppCmd              = 55,
    };

    SdioDriver(Hsmci *sdio, ITimer &timer)
        : _sdio(sdio), _timer(timer)
    {}

    bool init(Bits bits)
    {

        return true;
    }

    bool status()
    {
        return true;
    }

    void read (uint8_t *data, uint32_t address, size_t len) override
    {
        // Not implemented
    }

    void write (uint8_t *data, uint32_t address, size_t len) override
    {
        // Not implemented
    }

    bool readBlock(uint8_t *data, uint32_t sector, uint32_t len) override
    {
        return true;
    }

    bool writeBlock(const uint8_t *buffer, uint32_t sector, uint32_t count) override
    {
        return true;
    }

    void erase() override
    {
        // Not implemented
    }

    bool isInit() override
    {
        return true;
    }

    uint32_t getSectorCount() override
    {

        return _sectorCount;
    }

    uint32_t getSectorSize() override
    {

        return SectorSize;
    }

    uint32_t command(Cmd cmd, uint32_t arg, Response resp)
    {
        return 0;
    }
    
    Hsmci* getInstance()
    {
        return _sdio;
    }

    uint32_t getSpeed() const
    {
        return _speed;
    }

    private:

    Hsmci *_sdio;
    ITimer &_timer;
    uint32_t _speed = 0;

    static constexpr uint32_t configFrequency = 100000;     // 100 KHz
    inline static constexpr uint8_t timeVector[16] = {0, 10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80};
    static constexpr uint16_t SectorSize = 512;
    uint32_t _sectorCount = 0;
    uint32_t _rca = 0;
    uint32_t _cid[4] = {0};
    uint32_t _csd[4] = {0};
};
}