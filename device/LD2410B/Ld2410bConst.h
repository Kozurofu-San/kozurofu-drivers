#pragma once

#include <cstdint>
#include <array>

// Temperature / Humidity sensor
class Ld2410b
{
    public:

    // Frame
    static constexpr std::array<uint8_t, 4> Header    = {0xFD, 0xFC, 0xFB, 0xFA};       // Start of the frame
    static constexpr std::array<uint8_t, 4> Tail      = {0x04, 0x03, 0x02, 0x01};       // End of the frame

    // Commands
    static constexpr std::array<uint8_t, 2> ReadFirmwareVersion   = {0xA0, 0x00};       // This command reads the radar firmware version information
    static constexpr std::array<uint8_t, 4> EnableConfig          = {0xFF, 0x00, 0x01, 0x00};   // Any other command issued to the radar must be issued before it can be executed
    static constexpr std::array<uint8_t, 2> EndConfig             = {0xFF, 0x00};       // End the configuration command and execute it to restore the radar to working mode

};
