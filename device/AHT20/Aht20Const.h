#pragma once

#include <cstdint>

// Clock generator
class Aht20
{
    public:

    // Commands

    static constexpr uint8_t Initialization     = 0xBE;     // First stage setting
    static constexpr uint8_t TriggerMeasurement = 0xAC;     // Start measuring
    static constexpr uint8_t SoftReset          = 0xBA;     // Soft reset
    static constexpr uint8_t Read               = 0x71;     // Read status or data

    // Status bits
    
    static constexpr uint8_t CalEnable      = 1 << 3;     // 1 - Calibrated, 0 - Uncalibrated
    static constexpr uint8_t ModeStatus     = 3 << 5;     // Mode mask
    static constexpr uint8_t ModeNOR        = 0 << 5;     // NOR mode
    static constexpr uint8_t ModeCYC        = 1 << 5;     // CYC mode
    static constexpr uint8_t ModeCMD        = 2 << 5;     // CMD mode
    static constexpr uint8_t Busy           = 1 << 7;     // 1 - Busy in measurements, 0 - Free in dormant state
};
