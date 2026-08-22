#pragma once

#include <cstdint>
#include <array>

// Temperature / Humidity sensor
class Ld2410
{
    public:

    // Frame
    static constexpr std::array<uint8_t, 4> Header          = {0xFD, 0xFC, 0xFB, 0xFA};       // Start of the frame
    static constexpr std::array<uint8_t, 4> Tail            = {0x04, 0x03, 0x02, 0x01};       // End of the frame
    static constexpr std::array<uint8_t, 4> HeaderReport    = {0xF4, 0xF3, 0xF2, 0xF1};       // Start of the frame report
    static constexpr std::array<uint8_t, 4> TailReport      = {0xF8, 0xF7, 0xF6, 0xF5};       // End of the frame report

    // Commands
    static constexpr std::array<uint8_t, 2> ReadFirmwareVersion     = {0xA0, 0x00};                 // Read the radar firmware version information
    static constexpr std::array<uint8_t, 4> GetMacAddress           = {0xA5, 0x00, 0x01, 0x00};     // Query the MAC address
    static constexpr std::array<uint8_t, 2> Reset                   = {0xA2, 0x00};                 // Restore all configuration values to their original values, and the configuration values will take effect after restarting the module
    static constexpr std::array<uint8_t, 2> Restart                 = {0xA3, 0x00};                 // Module  will automatically restart after the response is sent
    static constexpr std::array<uint8_t, 4> EnableConfig            = {0xFF, 0x00, 0x01, 0x00};     // Any other command issued to the radar must be issued before it can be executed
    static constexpr std::array<uint8_t, 2> EndConfig               = {0xFE, 0x00};                 // End the configuration command and execute it to restore the radar to working mode
    static constexpr std::array<uint8_t, 2> EnableEngineeringMode   = {0x62, 0x00};                 // The energy value of each range gate will be added to the data reported by the radar
    static constexpr std::array<uint8_t, 2> CloseEngineeringMode    = {0x63, 0x00};                 // Turn off radar engineering mode
    static constexpr std::array<uint8_t, 4> SetBaudrate460800       = {0xA1, 0x00, 0x08, 0x00};     // The configuration value will take effect after restarting the module
    
};
