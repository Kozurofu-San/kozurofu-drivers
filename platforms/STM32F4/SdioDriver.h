#pragma once

#include "interface/Memory.h"
#include "interface/Timer.h"

#include "stm32f4xx.h"
extern uint32_t SystemCoreClock;

#define SDMMC_VOLTAGE_WINDOW_SD                       0x80100000U
#define SDMMC_HIGH_CAPACITY                           0x40000000U
#define SDMMC_STD_CAPACITY                            0x00000000U
#define SDMMC_CHECK_PATTERN                           0x000001AAU
#define SD_SWITCH_1_8V_CAPACITY                       0x01000000U

namespace driver
{

class SdioDriver : public IMemory
{
    public:

    enum class Response: uint16_t
    {
        None  = 0 << SDIO_CMD_WAITRESP_Pos,
        Short = 1 << SDIO_CMD_WAITRESP_Pos,
        Long  = 3 << SDIO_CMD_WAITRESP_Pos,
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

    SdioDriver(SDIO_TypeDef *sdio, ITimer &timer)
        : _sdio(sdio), _timer(timer)
    {}

    bool init(Bits bits)
    {
        // Clock
        RCC->APB2ENR |= RCC_APB2ENR_SDIOEN;
        _speed = HSE_VALUE
            * ((RCC->PLLCFGR & RCC_PLLCFGR_PLLN) >> RCC_PLLCFGR_PLLN_Pos)
            / ((RCC->PLLCFGR & RCC_PLLCFGR_PLLM) >> RCC_PLLCFGR_PLLM_Pos)
            / ((RCC->PLLCFGR & RCC_PLLCFGR_PLLQ) >> RCC_PLLCFGR_PLLQ_Pos)
            ;

        // Config
        _sdio->CLKCR = 0x76 << SDIO_CLKCR_CLKDIV_Pos
            | 0 << SDIO_CLKCR_PWRSAV_Pos
            | 0 << SDIO_CLKCR_BYPASS_Pos
            | 0 << SDIO_CLKCR_WIDBUS_Pos
            | 0 << SDIO_CLKCR_NEGEDGE_Pos
            | 0 << SDIO_CLKCR_HWFC_EN_Pos
            ;

        _sdio->POWER = SDIO_POWER_PWRCTRL;
        _sdio->CLKCR |= SDIO_CLKCR_CLKEN;
        _timer.delay(2);

        command(Cmd::GoIdleState, 0, Response::None);
        // command(Cmd::IoRwDirect, (1UL << 31) | (0UL << 28) | (0UL << 27) | (0x06UL << 9) | 0x08, Response::Short);
        if (command(Cmd::SendIfConf, 0x1AA, Response::Short) != 0x1AA)
        {
            return false;
        }
        // SD V2.x

        uint32_t ret = 0;
        do
        {
            ret = command(Cmd::AppCmd, 0, Response::Short);     // 0x477
            ret = command(Cmd::SdAppOpCond, SDMMC_VOLTAGE_WINDOW_SD | SDMMC_HIGH_CAPACITY | SD_SWITCH_1_8V_CAPACITY, Response::Short);
        } while (!(ret & (1 << 31)));

        command(Cmd::AllSendCid, 0, Response::Long);

        _rca = command(Cmd::SetRelativeAddr, 0, Response::Short);
        _rca &= 0xFFFF0000;     // 0x21310000

        command(Cmd::SendCid, _rca, Response::Long);
        _cid[0] = _sdio->RESP1;
        _cid[1] = _sdio->RESP2;
        _cid[2] = _sdio->RESP3;
        _cid[3] = _sdio->RESP4;

        command(Cmd::SendCsd, _rca, Response::Long);
        _csd[0] = _sdio->RESP1;
        _csd[1] = _sdio->RESP2;
        _csd[2] = _sdio->RESP3;
        _csd[3] = _sdio->RESP4;

        uint32_t transferRateUnit = _csd[0] & 0x7;
        uint32_t kbits = 10;
        for (size_t i = 0; i < transferRateUnit; ++i)
        {
            kbits *= 10;
        }
        uint32_t timeValue = (_csd[0] >> 3) & 0xF;
        uint32_t speedSd = timeVector[timeValue] * kbits * 1000;
        printf("SD speed: %u bps\n", (unsigned int)speedSd);

        _sectorCount = ((_csd[1] & 0x3F) << 16) | ((_csd[2] >> 16) & 0xFFFF);
        _sectorCount = (_sectorCount + 1) * 1024;

        command(Cmd::SelectDeselectCard, _rca, Response::Short);

        // Set bus width 1 -> 4 bits
        if (bits == Bits::b4)
        {
            command(Cmd::AppCmd, _rca, Response::Short);
            command(Cmd::SetBusWidth, 2, Response::Short);
            _sdio->CLKCR &= ~SDIO_CLKCR_WIDBUS;
            _sdio->CLKCR |= 1 << SDIO_CLKCR_WIDBUS_Pos;     // 4 bits
        }
        _sdio->CLKCR &= ~SDIO_CLKCR_CLKDIV;     // Max SDIO speed

        _speed /= (((_sdio->CLKCR & SDIO_CLKCR_CLKDIV) >> SDIO_CLKCR_CLKDIV_Pos) + 2);

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
        // Transmission length
        _sdio->DTIMER = 0xFFFFFFFF;
        _sdio->DLEN = SectorSize * len;
        _sdio->DCTRL = 9 << SDIO_DCTRL_DBLOCKSIZE_Pos
            | SDIO_DCTRL_DTDIR | SDIO_DCTRL_DTEN;
        
        command((len == 1) ? Cmd::ReadSingleBlock : Cmd::ReadMultipleBlock, sector, Response::Short);
        if (_sdio->STA & SDIO_STA_RXOVERR)
        {
            _sdio->ICR = SDIO_STA_RXOVERR;
        }

        uint32_t *data32 = (uint32_t*)data;
        uint32_t cnt = len * SectorSize / 4;

        _sdio->DCTRL;

        while (cnt--)
        {
            while (!(_sdio->STA & (SDIO_STA_RXDAVL | SDIO_STA_RXFIFOHF)));
            *data32++ = _sdio->FIFO;
        }

        while (!(_sdio->STA & SDIO_STA_DATAEND));

        _sdio->ICR  = 0xFFFFFFFF;

        return true;
    }

    bool writeBlock(const uint8_t *buffer, uint32_t sector, uint32_t count) override
    {
        // Transmission length
        _sdio->DTIMER = 0xFFFFFFFF;
        _sdio->DLEN = SectorSize * count;
        _sdio->DCTRL = 9 << SDIO_DCTRL_DBLOCKSIZE_Pos | SDIO_DCTRL_DTEN;
        
        command((count == 1) ? Cmd::WriteBlock : Cmd::WriteMultipleBlock, sector, Response::Short);
        while (!(_sdio->STA & (SDIO_STA_CMDREND | SDIO_STA_CTIMEOUT)));

        _sdio->ICR = SDIO_ICR_CMDRENDC | SDIO_ICR_CMDSENTC;

        uint32_t *buffer32 = (uint32_t*)buffer;
        uint32_t cnt = count * SectorSize / 4;

        _sdio->DCTRL;

        while (cnt--)
        {
            while (!(_sdio->STA & SDIO_STA_TXFIFOHE));
            _sdio->FIFO = *buffer32++;
        }

        while (!(_sdio->STA & SDIO_STA_DATAEND));

        _sdio->ICR  = 0xFFFFFFFF;

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
        _sdio->ICR = 0xFFFFFFFF;    // Clear flags
        _sdio->ARG = arg;
        _sdio->CMD = (static_cast<uint8_t>(cmd) & 0x3F)
            | static_cast<uint32_t>(resp)
            | SDIO_CMD_CPSMEN;      // CPSM enable
        if (resp == Response::None)
        {
            while (!(_sdio->STA & SDIO_STA_CMDSENT));  // Wait
        }
        else
        {
            while (!(_sdio->STA & (SDIO_STA_CMDREND | SDIO_STA_CCRCFAIL)));  // Wait
        }
        if (_sdio->STA & SDIO_STA_CCRCFAIL)
        {
            _sdio->ICR = SDIO_ICR_CCRCFAILC;
        }
        _sdio->ICR = SDIO_STA_CMDREND;
        return _sdio->RESP1;
    }
    
    SDIO_TypeDef* getSdio()
    {
        return _sdio;
    }

    uint32_t getSpeed() const
    {
        return _speed;
    }

    private:

    SDIO_TypeDef *_sdio;
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