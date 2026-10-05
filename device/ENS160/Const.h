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
    static constexpr uint8_t DATA_AQI           = 0x21;     // Air Quality Index (UBA, 1..5)
    static constexpr uint8_t DATA_TVOC          = 0x22;     // TVOC Concentration (ppb)
    static constexpr uint8_t DATA_ECO2          = 0x24;     // Equivalent CO2 Concentration (ppm)
    static constexpr uint8_t DATA_T             = 0x30;     // Temperature used in calculations
    static constexpr uint8_t DATA_RH            = 0x32;     // Relative Humidity used in calculations
    static constexpr uint8_t DATA_MISR          = 0x34;     // Data Integrity Field (optional)
    static constexpr uint8_t GPR_WRITE          = 0x40;     // General Purpose Write Registers
    static constexpr uint8_t GPR_READ           = 0x48;     // General Purpose Read Registers

    // PART_ID value (little endian: 0x60 @0x00, 0x01 @0x01)
    static constexpr uint16_t PART_ID_VALUE     = 0x0160;
 
    // OPMODE values
    static constexpr uint8_t OPMODE_DEEP_SLEEP  = 0x00;
    static constexpr uint8_t OPMODE_IDLE        = 0x01;
    static constexpr uint8_t OPMODE_STANDARD    = 0x02;
    static constexpr uint8_t OPMODE_RESET       = 0xF0;
 
    // COMMAND values
    static constexpr uint8_t CMD_NOP            = 0x00;
    static constexpr uint8_t CMD_GET_APPVER     = 0x0E;
    static constexpr uint8_t CMD_CLRGPR         = 0xCC;
 
    // DEVICE_STATUS bits
    static constexpr uint8_t STATUS_STATAS      = 0x80;     // OPMODE running
    static constexpr uint8_t STATUS_STATER      = 0x40;     // Error
    static constexpr uint8_t STATUS_VALIDITY    = 0x0C;     // Validity flag mask
    static constexpr uint8_t STATUS_NEWDAT      = 0x02;     // New data in DATA_x registers
    static constexpr uint8_t STATUS_NEWGPR      = 0x01;     // New data in GPR_READ registers
 
    static constexpr uint8_t AQI_MASK           = 0x07;
 
};
