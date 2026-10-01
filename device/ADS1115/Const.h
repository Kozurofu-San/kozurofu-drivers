#pragma once

#include <cstdint>

// Pressure sensor
class Ads1115
{
    public:

    // Registers

    static constexpr uint8_t Conversion         = 0x00;     // The result of the last conversion in binary two's-complement format
    static constexpr uint8_t Config             = 0x01;     // The operating mode, input selection, data rate, full-scale range, and comparator modes

    // Bit fields for Config register
    static constexpr uint16_t OS                = 1 << 15;  // Operational status or single-shot conversion start
    static constexpr uint16_t MUX               = 7 << 12;  // Input multiplexer configuration (ADS1115 only)
    static constexpr uint16_t MUX_Pos           = 12;       // Input multiplexer configuration (ADS1115 only)
    static constexpr uint16_t PGA               = 7 << 9 ;  // Programmable gain amplifier configuration
    static constexpr uint16_t PGA_Pos           = 9;        // Programmable gain amplifier configuration
    static constexpr uint16_t MODE              = 1 << 8 ;  // Device operating mode
    static constexpr uint16_t DR                = 7 << 5 ;  // Data rate
    static constexpr uint16_t DR_Pos            = 5;        // Data rate
    static constexpr uint16_t COMP_MODE         = 1 << 4 ;  // Comparator mode (ADS1114 and ADS1115 only)
    static constexpr uint16_t COMP_POL          = 1 << 3 ;  // Comparator polarity (ADS1114 and ADS1115 only)
    static constexpr uint16_t COMP_LAT          = 1 << 2 ;  // Latching comparator (ADS1114 and ADS1115 only)
    static constexpr uint16_t COMP_QUE          = 3 << 0 ;  // Comparator queue and disable (ADS1114 and ADS1115 only)
    static constexpr uint16_t COMP_QUE_Pos      = 0;        // Comparator queue and disable (ADS1114 and ADS1115 only)

    static constexpr uint8_t Lo_thresh          = 0x02;     // The lower threshold value
    static constexpr uint8_t Hi_thresh          = 0x03;     // The higher threshold value

};
