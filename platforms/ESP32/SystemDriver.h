#pragma once

#include "interface/System.h"

#include "esp_system.h"
#include "esp_clk_tree.h"
#include "esp_chip_info.h"

namespace driver
{

class SystemDriver : public ISystem
{
    public:

    ResetReason getValue() override
    {
        ResetReason ret;
        esp_reset_reason_t reason = esp_reset_reason();

        if      ( reason == ESP_RST_UNKNOWN    ) { ret = ResetReason::Unknown   ; }
        else if ( reason == ESP_RST_POWERON    ) { ret = ResetReason::PowerOn   ; }
        else if ( reason == ESP_RST_EXT        ) { ret = ResetReason::Ext       ; }
        else if ( reason == ESP_RST_SW         ) { ret = ResetReason::Sw        ; }
        else if ( reason == ESP_RST_PANIC      ) { ret = ResetReason::Panic     ; }
        else if ( reason == ESP_RST_INT_WDT    ) { ret = ResetReason::IntWdt    ; }
        else if ( reason == ESP_RST_TASK_WDT   ) { ret = ResetReason::TaskWdt   ; }
        else if ( reason == ESP_RST_WDT        ) { ret = ResetReason::Wdt       ; }
        else if ( reason == ESP_RST_DEEPSLEEP  ) { ret = ResetReason::DeepSleep ; }
        else if ( reason == ESP_RST_BROWNOUT   ) { ret = ResetReason::Brownout  ; }
        else if ( reason == ESP_RST_SDIO       ) { ret = ResetReason::Sdio      ; }
        else if ( reason == ESP_RST_USB        ) { ret = ResetReason::Usb       ; }
        else if ( reason == ESP_RST_JTAG       ) { ret = ResetReason::Jtag      ; }
        else if ( reason == ESP_RST_EFUSE      ) { ret = ResetReason::Efuse     ; }
        else if ( reason == ESP_RST_PWR_GLITCH ) { ret = ResetReason::PwrGlitch ; }
        else if ( reason == ESP_RST_CPU_LOCKUP ) { ret = ResetReason::CpuLockup ; }

        _reasonIdx = static_cast<size_t>(reason);

        return ret;
    }

    const std::string_view& getString() override
    {
        getValue();
        return resetReasonString[_reasonIdx];
    }

    uint32_t getCpuSpeed() override
    {
        uint32_t freqHz;
        esp_clk_tree_src_get_freq_hz(SOC_MOD_CLK_CPU, ESP_CLK_TREE_SRC_FREQ_PRECISION_EXACT, &freqHz);
        return freqHz;
    }

    uint64_t getChipId() override
    {
        esp_chip_info_t chipId;
        esp_chip_info(&chipId);
        uint64_t *ptr = reinterpret_cast<uint64_t*>(&chipId);
        return *ptr;
    }
    
    inline void restart() override
    {
        esp_restart();
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

    static constexpr std::array<std::string_view, 16> resetReasonString = {
        "Unknown",
        "PowerOn",
        "Ext",
        "Sw",
        "Panic",
        "IntWdt",
        "TaskWdt",
        "Wdt",
        "DeepSleep",
        "Brownout",
        "Sdio",
        "Usb",
        "Jtag",
        "Efuse",
        "PwrGlitch",
        "CpuLockup"
    };

    MemoryInfo _memoryInfo;
    size_t _reasonIdx;
};

}