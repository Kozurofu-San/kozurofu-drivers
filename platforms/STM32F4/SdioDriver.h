#pragma once

#include "interface/Memory.h"

#include "stm32f4xx.h"
extern uint32_t SystemCoreClock;

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
        ReadSingleBlock     = 17,
        ReadMultipleBlock   = 18,
        WriteBlock          = 24,
        WriteMultipleBlock  = 25,
        SdAppOpCond         = 41,
        IoRwDirect          = 52,
        AppCmd              = 55,
    };

    SdioDriver(SDIO_TypeDef *sdio)
        : _sdio(sdio)
    {}

    bool init(Bits bits)
    {
        // Clock
        RCC->APB2ENR |= RCC_APB2ENR_SDIOEN;

        // Config
        uint32_t busPrescaler = (RCC->CFGR >> RCC_CFGR_PPRE2_Pos) & 0x7;
        busPrescaler = (busPrescaler < 4) ? 1 : (1 << (busPrescaler - 3));
        uint32_t clkPrescaler = SystemCoreClock / busPrescaler / configFrequency;
        _speed = SystemCoreClock / busPrescaler / clkPrescaler;
        _sdio->CLKCR = clkPrescaler << SDIO_CLKCR_CLKDIV_Pos
            | 0 << SDIO_CLKCR_PWRSAV_Pos
            | 0 << SDIO_CLKCR_BYPASS_Pos
            | 0 << SDIO_CLKCR_WIDBUS_Pos
            | 0 << SDIO_CLKCR_NEGEDGE_Pos
            | 1 << SDIO_CLKCR_HWFC_EN_Pos
            ;

        _sdio->POWER = Power::On;
        _sdio->CLKCR |= SDIO_CLKCR_CLKEN;

        command(Cmd::GoIdleState, 0, Response::None);
        command(Cmd::IoRwDirect, (1UL << 31) | (0UL << 28) | (0UL << 27) | (0x06UL << 9) | 0x08, Response::Short);
        if (command(Cmd::SendIfConf, 0x1AA, Response::Short) != 0x1AA)
        {
            return false;
        }

        uint32_t ret = 0;
        do
        {
            ret = command(Cmd::AppCmd, 0, Response::Short);
            ret = command(Cmd::SdAppOpCond, 0x40FF8000, Response::Short);
        } while (!(ret & (1 << 31)));

        command(Cmd::AllSendCid, 0, Response::Long);
        uint32_t cidAll[4] = {
            _sdio->RESP1,
            _sdio->RESP2,
            _sdio->RESP3,
            _sdio->RESP4
        };

        _rca = command(Cmd::SetRelativeAddr, 0, Response::Short);
        _rca &= 0xFFFF0000;

        command(Cmd::SendCid, _rca, Response::Long);
        uint32_t cidCard[4] = {
            _sdio->RESP1,
            _sdio->RESP2,
            _sdio->RESP3,
            _sdio->RESP4
        };

        command(Cmd::SendCsd, _rca, Response::Long);
        uint32_t csd[4] = {
            _sdio->RESP1,
            _sdio->RESP2,
            _sdio->RESP3,
            _sdio->RESP4
        };

        uint32_t transferRateUnit = csd[0] & 0x7;
        uint32_t kbits = 10;
        for (size_t i = 0; i < transferRateUnit; ++i)
        {
            kbits *= 10;
        }
        uint32_t timeValue = (csd[0] >> 3) & 0xF;
        uint32_t speedSd = timeVector[timeValue] * kbits * 1000;

        _sectorCount = ((csd[1] & 0x3F) << 16) | ((csd[2] >> 16) & 0xFFFF);
        _sectorCount = (_sectorCount + 1) * 1024;

        command(Cmd::SelectDeselectCard, _rca, Response::Short);

        // Set bus width 1 -> 4 bits
        if (bits == Bits::b4)
        {
            _sdio->CLKCR &= ~SDIO_CLKCR_WIDBUS;
            _sdio->CLKCR |= 1 << SDIO_CLKCR_WIDBUS_Pos;     // 4 bits
            command(Cmd::SetBusWidth, 2, Response::Short);
        }
        else
        {
            _sdio->CLKCR &= ~SDIO_CLKCR_WIDBUS;
            _sdio->CLKCR |= 0 << SDIO_CLKCR_WIDBUS_Pos;     // 1 bit
            command(Cmd::SetBusWidth, 0, Response::Short);
        }
        // command(Cmd::AppCmd, _rca, Response::Short);
        // command(Cmd::SetBusWidth, 2, Response::Short);  // 4 bits
        // _sdio->CLKCR &= ~SDIO_CLKCR_WIDBUS;
        // _sdio->CLKCR |= 1 << SDIO_CLKCR_WIDBUS_Pos;     // 4 bits

        // Check if SD card supports High speed mode
        _sdio->DTIMER = 0xFFFFFFFF;
        _sdio->DLEN = 64;
        _sdio->DCTRL =
            6 << SDIO_DCTRL_DBLOCKSIZE_Pos
            | SDIO_DCTRL_DTDIR
            | SDIO_DCTRL_DTEN;
        ret = command(Cmd::AppCmd, 0, Response::Short);
        command(Cmd::SwitchFunc, 0x00FFFFF1, Response::Short);
        uint8_t switchStatus[64];
        for (size_t i = 0; i < 64 / 4; ++i)
        {
            while(!(_sdio->STA & SDIO_STA_RXDAVL));
            ((uint32_t*)switchStatus)[i] = _sdio->FIFO;
        }
        if ((switchStatus[13] & 0xF) != 0x1)
        {
            // Set HS mode
            // _sdio->DTIMER = 0xFFFFFFFF;
            // _sdio->DLEN = 64;
            // _sdio->DCTRL =
            //     6 << SDIO_DCTRL_DBLOCKSIZE_Pos
            //     | SDIO_DCTRL_DTDIR
            //     | SDIO_DCTRL_DTEN;
            // ret = command(Cmd::AppCmd, 0, Response::Short);
            // command(Cmd::SwitchFunc, 0x80FFFFF1, Response::Short);
            // for (size_t i = 0; i < 64 / 4; ++i)
            // {
            //     while(!(_sdio->STA & SDIO_STA_RXDAVL));
            //     ((uint32_t*)switchStatus)[i] = _sdio->FIFO;
            // }
            // _sdio->CLKCR &= ~SDIO_CLKCR_CLKDIV;     // Max SDIO speed
            // _sdio->CLKCR |= 5 << SDIO_CLKCR_CLKDIV_Pos;
        }

        _speed = SystemCoreClock / busPrescaler / (((_sdio->CLKCR & SDIO_CLKCR_CLKDIV) >> SDIO_CLKCR_CLKDIV_Pos) + 2);

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
            | SDIO_DCTRL_DTDIR;
        
        command((len == 1) ? Cmd::ReadSingleBlock : Cmd::ReadMultipleBlock, sector, Response::Short);
        while (!(_sdio->STA & (SDIO_STA_CMDREND | SDIO_STA_CTIMEOUT)));
        if (_sdio->STA & SDIO_STA_CTIMEOUT) return false;
        _sdio->ICR = SDIO_ICR_CMDRENDC | SDIO_ICR_CMDSENTC;

        uint32_t *data32 = (uint32_t*)data;
        uint32_t cnt = len * SectorSize / 4;

        _sdio->DCTRL |= SDIO_DCTRL_DTEN;

        while (cnt--)
        {
            while (!(_sdio->STA & SDIO_STA_RXDAVL));
            *data32++ = _sdio->FIFO;
        }

        while (!(_sdio->STA & SDIO_STA_DATAEND));

        if (len > 1)
        {
            command(Cmd::StopTransmission, 0, Response::Short);
        }

        _sdio->ICR  = 0xFFFFFFFF;

        return true;
    }

    bool writeBlock(const uint8_t *buffer, uint32_t sector, uint32_t count) override
    {
        // Transmission length
        _sdio->DTIMER = 0xFFFFFFFF;
        _sdio->DLEN = SectorSize * count;
        _sdio->DCTRL = 9 << SDIO_DCTRL_DBLOCKSIZE_Pos;
        
        command((count == 1) ? Cmd::WriteBlock : Cmd::WriteMultipleBlock, sector, Response::Short);
        while (!(_sdio->STA & (SDIO_STA_CMDREND | SDIO_STA_CTIMEOUT)));
        if (_sdio->STA & SDIO_STA_CTIMEOUT) return false;
        _sdio->ICR = SDIO_ICR_CMDRENDC | SDIO_ICR_CMDSENTC;

        uint32_t *buffer32 = (uint32_t*)buffer;
        uint32_t cnt = count * SectorSize / 4;

        _sdio->DCTRL |= SDIO_DCTRL_DTEN;

        while (cnt--)
        {
            while (!(_sdio->STA & (SDIO_STA_TXFIFOHE | SDIO_STA_DTIMEOUT | SDIO_STA_DCRCFAIL | SDIO_STA_TXUNDERR)));
            if (SDIO->STA & (SDIO_STA_DTIMEOUT | SDIO_STA_DCRCFAIL | SDIO_STA_TXUNDERR))
            {
                return false;
            }
            _sdio->FIFO = *buffer32++;
        }

        while (!(_sdio->STA & SDIO_STA_DATAEND));

        if (count > 1)
        {
            command(Cmd::StopTransmission, 0, Response::Short);
        }

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

    uint32_t getSectorCount()
    {

        return _sectorCount / 1000;
    }

    uint32_t getSectorSize()
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
            while (!(_sdio->STA & (SDIO_STA_CMDSENT | SDIO_STA_CTIMEOUT)));  // Wait
        }
        else
        {
            while (!(_sdio->STA & (SDIO_STA_CMDREND | SDIO_STA_CTIMEOUT | SDIO_STA_CCRCFAIL)));  // Wait
        }
        if (_sdio->STA & SDIO_STA_CCRCFAIL)
        {
            _sdio->ICR = SDIO_ICR_CCRCFAILC;
        }
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
    uint32_t _speed = 0;

    static constexpr uint32_t configFrequency = 100000;     // 100 KHz
    inline static constexpr uint8_t timeVector[16] = {0, 10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80};
    static constexpr uint16_t SectorSize = 512;
    uint32_t _rca = 0;
    uint32_t _sectorCount = 0;

    class Power
    {
        public:

        static constexpr uint32_t On  = 3 << SDIO_POWER_PWRCTRL_Pos;
        static constexpr uint32_t Off = 0 << SDIO_POWER_PWRCTRL_Pos;
    };

};
}