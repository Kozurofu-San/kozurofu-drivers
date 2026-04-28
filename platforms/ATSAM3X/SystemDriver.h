#pragma once

#include "interface/System.h"

#include "asf.h"
#include <cstdint>
#include <cstddef>

extern "C" {
    void* _sbrk(int);
    extern char _end;     // конец .bss
    extern char _estack;  // верх RAM
}

namespace driver
{

class SystemDriver : public ISystem
{
    public:

    ResetReason getValue() override
    {
        ResetReason ret;
        uint32_t reason = (RSTC->RSTC_SR & RSTC_SR_RSTTYP_Msk) >> RSTC_SR_RSTTYP_Pos;

        if      (reason == 0) { ret = ResetReason::PowerOn; _reasonIdx = 0; }
        else if (reason == 1) { ret = ResetReason::Backup ; _reasonIdx = 1; }
        else if (reason == 2) { ret = ResetReason::Wdt    ; _reasonIdx = 2; }
        else if (reason == 3) { ret = ResetReason::Sw     ; _reasonIdx = 3; }
        else if (reason == 4) { ret = ResetReason::Ext    ; _reasonIdx = 4; }
        else                  { ret = ResetReason::Unknown; _reasonIdx = 5; }

        return ret;
    }

    const std::string_view& getString() override
    {
        size_t idx = static_cast<size_t>(getValue());
        return resetReasonString[_reasonIdx];
    }

    uint32_t getCpuSpeed() override
    {
        return SystemCoreClock;
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

    static constexpr std::array<std::string_view, 19> resetReasonString = {
        "PowerOn",
        "Backup",
        "Wdt",
        "Sw",
        "Ext",
        "Unknown"
    };

    MemoryInfo _memoryInfo;
    size_t _reasonIdx;
};

}