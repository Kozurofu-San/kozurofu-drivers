#pragma once

#include "interface/System.h"

#include <avr/io.h>
#include <avr/boot.h>
#include <avr/wdt.h>

namespace driver
{

class SystemDriver : public ISystem
{
    public:

    ResetReason getReasonValue() override
    {
        ResetReason ret;
        uint32_t reason = MCUSR;

        // Clear the register so it's ready for the next reset
        MCUSR = 0;

        if      ( reason & _BV(PORF) ) { ret = ResetReason::PowerOn   ; _reasonIdx = 0; }
        else if ( reason & _BV(BORF) ) { ret = ResetReason::Brownout  ; _reasonIdx = 1; }
        else if ( reason & _BV(WDRF) ) { ret = ResetReason::IndWdt    ; _reasonIdx = 2; }
        else if ( reason & _BV(EXTRF)) { ret = ResetReason::Ext       ; _reasonIdx = 3; }
        else                           { ret = ResetReason::Unknown   ; _reasonIdx = 4; }

        return ret;
    }

    const char* getReasonString() override
    {
        getReasonValue();
        return resetReasonString[_reasonIdx];
    }

    inline uint32_t getCpuSpeed() override
    {
        return F_CPU;
    }
    
    uint32_t getChipId() override
    {
        return static_cast<uint64_t>(
            static_cast<uint32_t>(boot_signature_byte_get(0x02)) << 16 |
            static_cast<uint32_t>(boot_signature_byte_get(0x01)) << 8  |
            static_cast<uint32_t>(boot_signature_byte_get(0x00)) << 0
        );
    }
    
    void restart() override
    {
        do {
            wdt_enable(WDTO_15MS);
            for(;;) {}
        } while(0);
    }
    
    void updateMemoryInfo() override
    {
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
        "Wdt",
        "Ext",
        "Unknown"
    };

    MemoryInfo _memoryInfo;
    size_t _reasonIdx;
};

}