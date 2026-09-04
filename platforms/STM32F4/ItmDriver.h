#pragma once

#include "interface/Itm.h"

#include "stm32f4xx.h"

extern uint32_t SystemCoreClock;

namespace driver
{

class ItmDriver : public IItm
{
    public:

    enum class Oversampling : uint8_t
    {
        Bits16 = 0x0,
        Bits8  = 0x1,
    };

    ItmDriver()
    {
    }
    
    /*
    * @param  portMask: 0xFFFFFFFF to enable all 32 ports
    * @retval None
    * @note   The SWO baudrate must be less than or equal to 2.25MHz for ST-LINK V2
    * Init OK
    */
    bool init(uint32_t portMask = 0xFFFFFFFF)
    {
        uint32_t swoPrescaler = (SystemCoreClock / ItmBaudrate) - 1u ;   // baudrate in Hz, note that cpuCoreFreqHz is expected to match the CPU core clock
        
        CoreDebug->DEMCR = CoreDebug_DEMCR_TRCENA_Msk;      // Debug Exception and Monitor Control Register (DEMCR): enable trace in core debug
        DBGMCU->CR	= 0x00000027u;                          // DBGMCU_CR : TRACE_IOEN DBG_STANDBY DBG_STOP 	DBG_SLEEP
        TPI->SPPR	= 0x00000002u;                          // Selected PIN Protocol Register: Select which protocol to use for trace output (2: SWO)
        TPI->ACPR	= swoPrescaler;                         // Async Clock Prescaler Register: Scale the baud rate of the asynchronous output
        ITM->LAR	= 0xC5ACCE55u;                          // ITM Lock Access Register: C5ACCE55 enables more write access to Control Register 0xE00 :: 0xFFC
        // ITM, timestamp/sync packets, the trace bus and SWO output.
        // SWOENA is required for asynchronous output through the TPIU.
        ITM->TCR	= (1UL << 16U) | ITM_TCR_SWOENA_Msk |
                      ITM_TCR_ITMENA_Msk | (1UL << 2U) | (1UL << 3U);
        ITM->TPR	= ITM_TPR_PRIVMASK_Msk;                 // ITM Trace Privilege Register: All stimulus ports
        ITM->TER	= portMask;                             // ITM Trace Enable Register: Enabled tracing on stimulus ports. One bit per stimulus port.
        DWT->CTRL	= 0x400003FEu;                          // Data Watchpoint and Trace Register
        TPI->FFCR	= 0x00000100u;                          // Formatter and Flush Control Register
        
        _speed = SystemCoreClock / (TPI->ACPR + 1);
        _isInit = true;
        return true;
    }

    void writeChar(uint8_t channel, char symbol) override
    {
        if (isChannelReady(channel))
        {
            ITM->PORT[channel].u8 = symbol;
        }
    };

    void writeInt(uint8_t channel, uint32_t data) override
    {
        if (isChannelReady(channel))
        {
            ITM->PORT[channel].u32 = data;
        }
    };

    uint32_t getSpeed() const override
    {
        return _speed;
    }

    bool isInit() override
    {
        return _isInit;
    }

    private:

    uint32_t _speed;    // Speed in Hz
    bool _isInit = false;

    static constexpr uint32_t ItmBaudrate = 2250000;
    static constexpr uint32_t ItmReadyTimeout = 10'000U;
    
    bool isChannelReady(uint32_t channel)
    {
        if (channel >= 32U ||
            (ITM->TCR & ITM_TCR_ITMENA_Msk) == 0U ||
            (ITM->TER & (1UL << channel)) == 0U)
        {
            return false;
        }

        // With no SWO consumer the stimulus port stays at zero forever.
        // Logging must never stop application code in that case.
        uint32_t timeout = ItmReadyTimeout;
        while (ITM->PORT[channel].u32 == 0U)
        {
            if (--timeout == 0U)
            {
                return false;
            }
        }
        return true;
    }

};

}
