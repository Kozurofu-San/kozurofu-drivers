#pragma once

#include <cstdint>

// Temperature / Humidity sensor
class Pcd8544
{
    public:

    // Instructions Simple/Extended

    static constexpr uint8_t Nop            = 0x00;     // No operation
    static constexpr uint8_t FunctionSet    = 0x20;     // Power down control; entry mode; extended instruction set

    // Bit fields for FunctionSet
    static constexpr uint8_t PowerDown      = 0x04;     // Power down
    static constexpr uint8_t Horizontal     = 0x00;     // Horizontal addressing
    static constexpr uint8_t Vertical       = 0x02;     // Vertical addressing
    static constexpr uint8_t Extended       = 0x01;     // Extended instruction set

    // Instructions Simple
    static constexpr uint8_t DisplayControl     = 0x08;     // Sets display configuration
    
    // Bit fields for DisplayControl
    static constexpr uint8_t Blank              = 0x00;     // Display blank
    static constexpr uint8_t Normal             = 0x04;     // Normal mode
    static constexpr uint8_t AllSegments        = 0x01;     // All display segments on
    static constexpr uint8_t Inverse            = 0x05;     // Inverse video mode
    
    static constexpr uint8_t SetYAddress        = 0x40;     // Sets Y-address of RAM; 0 ≤ Y ≤ 5
    static constexpr uint8_t SetXAddress        = 0x80;     // Sets X-address of RAM; 0 ≤ X ≤ 83

    // Instructions Extended
    static constexpr uint8_t TemperatureControl = 0x04;     // Set Temperature Coefficient (TCx)
    static constexpr uint8_t BiasSystem         = 0x10;     // Set Bias System (BSx)
    static constexpr uint8_t SetVOP             = 0x80;     // Write VOP to register
};
