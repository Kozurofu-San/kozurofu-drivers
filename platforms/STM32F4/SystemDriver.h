#pragma once

#include "interface/System.h"

#include <cstdint>
#include <cstddef>

#include "stm32f4xx.h"

extern "C" {
    void* _sbrk(int);
    extern char _end;
    extern char _estack;
}

namespace driver
{

class SystemDriver : public ISystem
{
    public:

    static constexpr uint32_t SystemCoreClock = 168000000u;     // MHz

    SystemDriver()
    {
        clock();
    }

    ResetReason getReasonValue() override
    {
        ResetReason ret;
        uint32_t reason = RCC-> CSR;

        if      ( reason & RCC_CSR_PORRSTF  ) { ret = ResetReason::PowerOn   ; _reasonIdx = 0; }
        else if ( reason & RCC_CSR_LPWRRSTF ) { ret = ResetReason::Brownout  ; _reasonIdx = 1; }
        else if ( reason & RCC_CSR_IWDGRSTF ) { ret = ResetReason::IndWdt    ; _reasonIdx = 2; }
        else if ( reason & RCC_CSR_WWDGRSTF ) { ret = ResetReason::WinWdt    ; _reasonIdx = 3; }
        else if ( reason & RCC_CSR_SFTRSTF  ) { ret = ResetReason::Sw        ; _reasonIdx = 4; }
        else if ( reason & RCC_CSR_PINRSTF  ) { ret = ResetReason::Ext       ; _reasonIdx = 5; }
        else                                  { ret = ResetReason::Unknown   ; _reasonIdx = 6; }

        return ret;
    }

    bool clock()
    {
        #if defined(__FPU_PRESENT) && (__FPU_PRESENT == 1U)
            SCB->CPACR |= (3UL << (10U * 2U)) | (3UL << (11U * 2U));
            __DSB();
            __ISB();
        #endif

        constexpr uint32_t TIMEOUT = 100000;  // simple loop-count timeout
        uint32_t t;

        // --- Power: enable PWR clock, voltage scale 1 ---
        RCC->APB1ENR |= RCC_APB1ENR_PWREN;
        (void)RCC->APB1ENR;                   // dummy read for clock-enable delay
        PWR->CR |= PWR_CR_VOS;                // VOS = 1 -> Scale 1 (on F407 this is a single bit)
        (void)PWR->CR;

        // --- HSE on ---
        RCC->CR |= RCC_CR_HSEON;
        for (t = TIMEOUT; !(RCC->CR & RCC_CR_HSERDY); )
            if (--t == 0) return false;

        // --- PLL config: HSE / 4 * 168 / 2 = 168 MHz, Q = 7 -> 48 MHz ---
        // (PLL must be off while configuring; it is after reset, but make sure)
        RCC->CR &= ~RCC_CR_PLLON;
        for (t = TIMEOUT; (RCC->CR & RCC_CR_PLLRDY); )
            if (--t == 0) return false;

        RCC->PLLCFGR = (4u   << RCC_PLLCFGR_PLLM_Pos)   // PLLM = 4
                    | (168u << RCC_PLLCFGR_PLLN_Pos)   // PLLN = 168
                    | (0u   << RCC_PLLCFGR_PLLP_Pos)   // PLLP = 2 (00)
                    | RCC_PLLCFGR_PLLSRC_HSE           // HSE as PLL source
                    | (7u   << RCC_PLLCFGR_PLLQ_Pos);  // PLLQ = 7

        // --- PLL on ---
        RCC->CR |= RCC_CR_PLLON;
        for (t = TIMEOUT; !(RCC->CR & RCC_CR_PLLRDY); )
            if (--t == 0) return false;

        // --- Flash: 5 wait states, prefetch + instruction/data caches ---
        FLASH->ACR = FLASH_ACR_LATENCY_5WS
                | FLASH_ACR_PRFTEN
                | FLASH_ACR_ICEN
                | FLASH_ACR_DCEN;
        if ((FLASH->ACR & FLASH_ACR_LATENCY) != FLASH_ACR_LATENCY_5WS)
            return false;

        // --- Bus prescalers: AHB /1, APB1 /4 (42 MHz), APB2 /4 (42 MHz) ---
        RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2))
                | RCC_CFGR_HPRE_DIV1
                | RCC_CFGR_PPRE1_DIV4
                | RCC_CFGR_PPRE2_DIV4;

        // --- Switch SYSCLK to PLL ---
        RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;
        for (t = TIMEOUT; (RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL; )
            if (--t == 0) return false;

        return true;
    }

    const char* getReasonString() override
    {
        getReasonValue();
        return resetReasonString[_reasonIdx];
    }

    inline uint32_t getCpuSpeed() override
    {
        return SystemCoreClock;
    }
    
    uint32_t getChipId() override
    {
        uint32_t *uid = (uint32_t *)UID_BASE;
        return uid[2];
    }
    
    void restart() override
    {
        NVIC_SystemReset();
    }

    void enterCritical() override
    {
        _primask = __get_PRIMASK();     // Save current interrupt state
        __disable_irq();                // Disable all global interrupts
    }
    
    void exitCritical() override
    {
        __set_PRIMASK(_primask);        // Restore previous interrupt state
    }
    
    void updateMemoryInfo() override
    {
        char stack_ptr;

        _memoryInfo.ramEnd   = &_estack;
        _memoryInfo.dataEnd  = &_end;
        _memoryInfo.heapEnd  = (char*)_sbrk(0);
        _memoryInfo.stackPtr = &stack_ptr;
        _memoryInfo.ramFree  = _memoryInfo.stackPtr - _memoryInfo.heapEnd;
        
        _memoryInfo.heapStackCollision = (_memoryInfo.heapEnd >= _memoryInfo.stackPtr);
    
        _memoryInfo.usedHeap  = _memoryInfo.heapEnd - _memoryInfo.dataEnd;
        _memoryInfo.usedStack = _memoryInfo.ramEnd - _memoryInfo.stackPtr;
    }
    
    inline int32_t getFreeMemory() override
    {
        updateMemoryInfo();
        return _memoryInfo.ramFree;
    }
    
    inline int32_t getUsedHeap() override
    {
        updateMemoryInfo();
        return _memoryInfo.usedHeap;
    }
    
    inline int32_t getUsedStack() override
    {
        updateMemoryInfo();
        return _memoryInfo.usedStack;
    }

    private:

    static constexpr const char* const resetReasonString[] =
    {
        "PowerOn",
        "Brownout",
        "IndWdt",
        "WinWdt",
        "Sw",
        "Ext",
        "Unknown"
    };

    MemoryInfo _memoryInfo;
    size_t _reasonIdx;
    uint32_t _primask;
};

}