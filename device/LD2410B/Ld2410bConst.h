#pragma once

#include <cstdint>

// Temperature / Humidity sensor
class Ld2410b
{
    public:

    // Frame
    static constexpr uint32_t Header    = 0xFAFBFCFD;       // Start of the frame
    static constexpr uint32_t Tail      = 0x01020304;       // End of the frame

    // Commands
    static constexpr uint16_t ReadFirmwareVersion   = 0x00A0;       // This command reads the radar firmware version information
    static constexpr uint32_t EnableConfig          = 0x000100FF;   // Any other command issued to the radar must be issued before it can be executed
    static constexpr uint16_t EndConfig             = 0x00FE;       // End the configuration command and execute it to restore the radar to working mode

};
