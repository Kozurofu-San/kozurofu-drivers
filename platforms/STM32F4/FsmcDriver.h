#pragma once

#include "interface/Communication.h"

#include "stm32f4xx.h"
extern uint32_t SystemCoreClock;

namespace driver
{

class FsmcDriver: public ICommunication
{
    public:

    enum class P: uint8_t
    {
        Bank1,
        Bank1E,
        Bank2,
        Bank3,
        Bank4
    };

    FsmcDriver(P fsmc)
        : _fsmc(fsmc)
    {
    }

    
    bool init()
    {
        RCC->AHB3ENR |= RCC_AHB3ENR_FSMCEN;
        if (_fsmc == P::Bank1)
        {
            _addrCmd  = 0x60000000;   // Bank 1
            _addrData = 0x60080000;   // A18 -> 18+1 bit

            _speed = SystemCoreClock;

            FSMC_Bank1E->BWTR[0] = 0x0FFFFFFF;
            FSMC_Bank1->BTCR[0 + 0]	            // BCR1
                = 0 << FSMC_BCR1_CBURSTRW_Pos   // write 0 - async 1 - sync
                | 0 << FSMC_BCR1_ASYNCWAIT_Pos  // Wait signal during asynchronous transfers
                | 0 << FSMC_BCR1_EXTMOD_Pos     // Extended mode enable. Use BWTR register or no
                | 0 << FSMC_BCR1_WAITEN_Pos     // Wait enable bit.
                | 1 << FSMC_BCR1_WREN_Pos       // Write enable bit
                | 0 << FSMC_BCR1_WAITCFG_Pos    // Wait timing configuration. 0: NWAIT signal is active one data cycle before wait state 1: NWAIT signal is active during wait state
                | 0 << FSMC_BCR1_WRAPMOD_Pos    // Wrapped burst mode support
                | 0 << FSMC_BCR1_WAITPOL_Pos    // Wait signal polarity bit. 0: NWAIT active low. 1: NWAIT active high
                | 0 << FSMC_BCR1_BURSTEN_Pos    // Burst enable bit
                | 1 << FSMC_BCR1_FACCEN_Pos     // Flash access enable
                | 1 << FSMC_BCR1_MWID_Pos       // 0 = 8b 1 = 16b
                | 2 << FSMC_BCR1_MTYP_Pos       // 0 = SRAM 1 = CRAM 2 = NOR
                | 0 << FSMC_BCR1_MUXEN_Pos      // Multiplexing Address/Data
                | 1 << FSMC_BCR1_MBKEN_Pos      // Memory bank enable bit
            ;
            FSMC_Bank1->BTCR[0 + 1]             // BTR1
                = 0  << FSMC_BTR1_ADDSET_Pos    // Address setup phase duration 0..F * HCLK
                | 0  << FSMC_BTR1_ADDHLD_Pos    // Address-hold phase duration 1..F * 2 * HCLK
                | 15 << FSMC_BTR1_DATAST_Pos    // Data-phase duration 1..FF * 2 * HCLK
                | 0  << FSMC_BTR1_BUSTURN_Pos   // Bus turnaround phase duration 0...F
                | 1  << FSMC_BTR1_CLKDIV_Pos    // for FSMC_CLK signal 1 = HCLK/2, 2 = HCLK/3 ...  F= HCLK/16
                | 0  << FSMC_BTR1_DATLAT_Pos    // Data latency for synchronous NOR Flash memory 0(2CK)...F(17CK)
                | 0  << FSMC_BTR1_ACCMOD_Pos    // Access mode 0 = A, 1 = B, 2 = C, 3 = D Use w/EXTMOD bit
            ;
        }
        
        _isInit = true;
        return true;
    }

    void write(uint8_t *data, size_t len, size_t bytes = 1) override
    {
        if (bytes == 1)
        {
            for (size_t i = 0; i < len; ++i)
            {
                *(volatile uint16_t*) _addrData = data[i];
            }
        }
        else if (bytes == 2)
        {
            uint16_t *ptr = reinterpret_cast<uint16_t*>(data);
            for (size_t i = 0; i < len / 2; ++i)
            {
                *(volatile uint16_t*) _addrData = ptr[i];
            }
        }
    }

    void read(uint8_t *data, size_t len, size_t bytes = 1) override
    {
        if (bytes == 1)
        {
            for (size_t i = 0; i < len; ++i)
            {
                data[i] = *(volatile uint16_t*) _addrData;
            }
        }
        else if (bytes == 2)
        {
            uint16_t *ptr = reinterpret_cast<uint16_t*>(data);
            for (size_t i = 0; i < len / 2; ++i)
            {
                ptr[i] = *(volatile uint16_t*) _addrData;
            }
        }
    }

    inline uint32_t sendCommand(uint32_t cmd) override
    {
        *(volatile uint16_t*) _addrCmd = cmd;
        return cmd;
    }

    uint32_t getSpeed() const override
    {
        // FSMC does not have a speed, return 0
        return _speed;
    }

    void enable() override
    {
        // FSMC does not have an enable, do nothing
    }

    void disable() override
    {
        // FSMC does not have a disable, do nothing
    }

    bool isInit() override
    {
        return _isInit;
    }

    private:

    P _fsmc;

    uint32_t _addrCmd  = 0x60000000;
    uint32_t _addrData = 0x60080000;
    
    uint32_t _speed = 0;
    bool _isInit = false;
};

}