#pragma once

#include <cstdint>

// Pressure sensor
class Ens160
{
    public:

    // Registers

    static constexpr uint8_t PART_ID            = 0x00;     // Device Identity 0x01, 0x60
    static constexpr uint8_t OPMODE             = 0x10;     // Operating Mode
    static constexpr uint8_t CONFIG             = 0x11;     // Interrupt Pin Configuration
    static constexpr uint8_t COMMAND            = 0x12;     // Additional System Commands
    static constexpr uint8_t TEMP_IN            = 0x13;     // Host Ambient Temperature Information
    static constexpr uint8_t RH_IN              = 0x15;     // Host Relative Humidity Information
    static constexpr uint8_t DEVICE_STATUS      = 0x20;     // Operating Mode
    static constexpr uint8_t DATA_TVOC          = 0x22;     // TVOC Concentration (ppb)
    static constexpr uint8_t DATA_ECO2          = 0x24;     // Equivalent CO2 Concentration (ppm)
    static constexpr uint8_t DATA_T             = 0x30;     // Temperature used in calculations
    static constexpr uint8_t DATA_RH            = 0x32;     // Relative Humidity used in calculations
    static constexpr uint8_t DATA_MISR          = 0x34;     // Data Integrity Field (optional)
    static constexpr uint8_t GPR_WRITE          = 0x40;     // General Purpose Write Registers
    static constexpr uint8_t GPR_READ           = 0x48;     // General Purpose Read Registers

};
