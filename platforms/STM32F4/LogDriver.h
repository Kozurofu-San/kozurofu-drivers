#pragma once

#include "interface/Log.h"

#include <stm32f4xx.h>
#include <cstdarg>
#include <cstdio>
#include <cstdint>

namespace driver
{

class LogDriver : public ILog
{
    public:

    static constexpr uint32_t StLinkV2MaxSpeed = 2250000;

    enum class MsgType
    {
        I,
        W,
        E
    };

    LogDriver()
    {
    }

    /*
    * Working config
        CoreDebug->DEMCR    0x01000000
        DBGMCU->CR          0x00000037
        TPI->SPPR           0x00000002
        TPI->ACPR           0x000000a7
        ITM->LAR            0x00000000
        ITM->TCR            0x0081000f
        ITM->TPR            0x00000000
        ITM->TER            0xffffffff
        DWT->CTRL           0x4001061f
        TPI->FFCR           0x00000100
    */

    /*
    * @param  portMask: 0xFFFFFFFF to enable all ports
    * @param  cpuCoreFreqHz: CPU core frequency in Hz
    * @param  baudrate: SWO baudrate in Hz
    * @retval None
    * @note   The SWO baudrate must be less than or equal to 2.25MHz for ST-LINK V2
    * Init OK
    */
    void init(uint32_t portMask, uint32_t cpuCoreFreqHz, uint32_t baudrate)
    {
        #ifdef LOG
        uint32_t swoPrescaler = (cpuCoreFreqHz / baudrate) - 1u ;   // baudrate in Hz, note that cpuCoreFreqHz is expected to match the CPU core clock
        CoreDebug->DEMCR = CoreDebug_DEMCR_TRCENA_Msk;      // Debug Exception and Monitor Control Register (DEMCR): enable trace in core debug
        DBGMCU->CR	= 0x00000027u;                          // DBGMCU_CR : TRACE_IOEN DBG_STANDBY DBG_STOP 	DBG_SLEEP
        TPI->SPPR	= 0x00000002u;                          // Selected PIN Protocol Register: Select which protocol to use for trace output (2: SWO)
        TPI->ACPR	= swoPrescaler;                         // Async Clock Prescaler Register: Scale the baud rate of the asynchronous output
        ITM->LAR	= 0xC5ACCE55u;                          // ITM Lock Access Register: C5ACCE55 enables more write access to Control Register 0xE00 :: 0xFFC
        ITM->TCR	= 0x0001000Du;                          // ITM Trace Control Register
        ITM->TPR	= ITM_TPR_PRIVMASK_Msk;                 // ITM Trace Privilege Register: All stimulus ports
        ITM->TER	= portMask;                             // ITM Trace Enable Register: Enabled tracing on stimulus ports. One bit per stimulus port.
        DWT->CTRL	= 0x400003FEu;                          // Data Watchpoint and Trace Register
        TPI->FFCR	= 0x00000100u;                          // Formatter and Flush Control Register
        #endif
    }

    void i(const char* message, ...) override
    {
        #ifdef LOG
        va_list args;
        va_start(args, message);
        uint32_t len = vsnprintf(_buffer, sizeof(_buffer), message, args);
        va_end(args);
        printString(static_cast<uint8_t>(MsgType::I), _buffer);
        if (len > sizeof(_buffer))
        {
            printString(static_cast<uint8_t>(MsgType::E), ErrorMsg);
            return;
        }
        #endif
    }

    void w(const char* message, ...) override
    {
        #ifdef LOG
        va_list args;
        va_start(args, message);
        uint32_t len = vsnprintf(_buffer, sizeof(_buffer), message, args);
        va_end(args);
        printString(static_cast<uint8_t>(MsgType::W), _buffer);
        if (len > sizeof(_buffer))
        {
            printString(static_cast<uint8_t>(MsgType::E), ErrorMsg);
            return;
        }
        #endif
    }

    void e(const char* message, ...) override
    {
        #ifdef LOG
        va_list args;
        va_start(args, message);
        uint32_t len = vsnprintf(_buffer, sizeof(_buffer), message, args);
        va_end(args);
        printString(static_cast<uint8_t>(MsgType::E), _buffer);
        if (len > sizeof(_buffer))
        {
            printString(static_cast<uint8_t>(MsgType::E), ErrorMsg);
            return;
        }
        #endif
    }

    void v(uint32_t channel, int32_t value) override
    {
        #ifdef LOG
        if (((ITM->TCR & ITM_TCR_ITMENA_Msk) != 0UL) &&      /* ITM enabled */
            ((ITM->TER & (1UL << channel)  ) != 0UL)   )     /* ITM Port enabled */
        {
            while (ITM->PORT[channel].u32 == 0UL)
            {
                __NOP();
            }
            ITM->PORT[channel].u32 = value;
        }
        #endif
    }

    bool readString(char* string) override
    {
        return true;
    }
    
    bool readNumber(int& number) override
    {
        return true;
    }

    private:

    char _buffer[40];
    static constexpr char ErrorMsg[] = "Buffer overflow";
    
    uint32_t ITM_SendCharToChannel(uint32_t channel, uint32_t symbol)
    {
        if (((ITM->TCR & ITM_TCR_ITMENA_Msk) != 0UL) &&      /* ITM enabled */
            ((ITM->TER & (1UL << channel)  ) != 0UL)   )     /* ITM Port enabled */
        {
        while (ITM->PORT[channel].u32 == 0UL)
        {
            __NOP();
        }
        ITM->PORT[channel].u8 = (uint8_t)symbol;
        }
        return (symbol);
    }

    void printString(uint32_t channel, const char *symbol)
    {
        while (*symbol != 0)
        {
            ITM_SendCharToChannel(channel, *symbol);
            symbol++;
        }
        ITM_SendCharToChannel(channel, '\n');
    }
    

};

}