#pragma once

#include "interface/System.h"

#include <cstdint>
#include <cstddef>

#include "stm32f1xx.h"

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
};

}