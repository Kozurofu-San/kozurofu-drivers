#pragma once

#include <cstdint>

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

    struct MemoryInfo {
        char* ramEnd;       // The end of RAM
    
        char* dataEnd;      // The end of .bss / the beginning of heap
        char* heapEnd;      // The current end of heap
    
        char* stackPtr;     // The current stack pointer
    
        int32_t ramFree;   // Calculated free RAM
        
        bool heapStackCollision; // Critical
        size_t usedHeap;
        size_t usedStack;
    };

    virtual ~ISystem() = default;

    virtual ResetReason getReasonValue() = 0;
    virtual const char* getReasonString() = 0;
    
    virtual uint32_t getCpuSpeed() = 0;
    virtual uint64_t getChipId() = 0;
    virtual void restart() = 0;

    virtual void updateMemoryInfo() = 0;
    virtual int32_t getFreeMemory() = 0;
    virtual int32_t getUsedHeap() = 0;
    virtual int32_t getUsedStack() = 0;
};

}