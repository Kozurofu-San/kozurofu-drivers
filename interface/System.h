#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace driver
{

class ISystem
{
    public:

    enum class ResetReason: uint8_t
    {
        Unknown,    //!< Reset reason can not be determined
        PowerOn,    //!< Reset due to power-on event
        Ext,        //!< Reset by external pin 
        Sw,         //!< Software reset via esp_restart
        Panic,      //!< Software reset due to exception/panic
        IntWdt,     //!< Reset (software or hardware) due to interrupt watchdog
        IndWdt,     //!< Reset due to independent watchdog
        WinWdt,     //!< Reset due to window watchdog
        TaskWdt,    //!< Reset due to task watchdog
        Wdt,        //!< Reset due to other watchdogs
        DeepSleep,  //!< Reset after exiting deep sleep mode
        Brownout,   //!< Brownout low voltage reset (software or hardware)
        Sdio,       //!< Reset over SDIO
        Usb,        //!< Reset by USB peripheral
        Jtag,       //!< Reset by JTAG
        Efuse,      //!< Reset due to efuse error
        PwrGlitch,  //!< Reset due to power glitch detected
        CpuLockup,  //!< Reset due to CPU lock up (double exception)
        Backup,     //!< Reset due to backup
    };

    // static constexpr std::array<std::string_view, 19> resetReasonString = {
    //     "Unknown",
    //     "PowerOn",
    //     "Ext",
    //     "Sw",
    //     "Panic",
    //     "IntWdt",
    //     "IndWdt",
    //     "WinWdt",
    //     "TaskWdt",
    //     "Wdt",
    //     "DeepSleep",
    //     "Brownout",
    //     "Sdio",
    //     "Usb",
    //     "Jtag",
    //     "Efuse",
    //     "PwrGlitch",
    //     "CpuLockup",
    //     "Backup"
    // };

    struct MemoryInfo {
        char* ramEnd;       // конец RAM
    
        char* dataEnd;      // конец .bss / начало heap
        char* heapEnd;      // текущий конец heap
    
        char* stackPtr;     // текущая позиция стека
    
        int32_t ramFree;   // вычисленное свободное место
        
        bool heapStackCollision; // 🔥 критично
        size_t usedHeap;
        size_t usedStack;
    };

    virtual ~ISystem() = default;

    virtual ResetReason getValue() = 0;
    virtual const std::string_view& getString() = 0;

    virtual uint32_t getCpuSpeed() = 0;
    virtual void restart() = 0;

    virtual void updateMemoryInfo() = 0;
    virtual int32_t getFreeMemory() = 0;
    virtual int32_t getUsedHeap() = 0;
    virtual int32_t getUsedStack() = 0;
};

}