#pragma once

#include <cstdint>

// Pressure sensor
class Bmp280
{
    public:

    // Registers

    static constexpr uint8_t Id                 = 0xD0;     // Contains the chip identification number chip_id[7:0], which is 0x58. This number can be read as soon as the device finished the power-on-reset
    static constexpr uint8_t IdValue            = 0x58;     // Default ID
    static constexpr uint8_t Reset              = 0xE0;     // Contains the soft reset word reset[7:0]. If the value 0xB6 is written to the register, the device is reset using the complete power-on-reset procedure. Writing other values than 0xB6 has no effect. The readout value is always 0x00
    static constexpr uint8_t ResetValue         = 0xB6;     // Write this value to reset
    static constexpr uint8_t Status             = 0xF3;     // Contains two bits which indicate the status of the device
    static constexpr uint8_t StatusImUpdate     = 1 << 0;   // Automatically set to ‘1’ when the NVM data are being copied to image registers and back to ‘0’ when the copying is done. The data are copied at power-on-reset and before every conversion
    static constexpr uint8_t StatusMeasuring    = 1 << 3;   // Automatically set to ‘1’ whenever a conversion is running and back to ‘0’ when the results have been transferred to the data registers
    
    static constexpr uint8_t CtrlMeas           = 0xF4;     // Sets the data acquisition options of the device
    static constexpr uint8_t Mode               = 0;        // Controls the power mode of the device
    static constexpr uint8_t ModeSleep          = 0 << Mode;        // set by default after power on reset. In sleep mode, no measurements are performed and power consumption is at a minimum
    static constexpr uint8_t ModeForced         = 1 << Mode;        // A single measurement is performed according to selected measurement and filter options. When the measurement is finished, the sensor returns to sleep mode and the measurement results can be obtained from the data registers. For a next measurement, forced mode needs to be selected again
    static constexpr uint8_t ModeNormal         = 3 << Mode;        // After setting the mode,measurement and filter options, the last measurement results can be obtained from the data registers without the need of further write accesses
    static constexpr uint8_t OsrsP              = 2;        // Controls oversampling of pressure data
    static constexpr uint8_t OsrsT              = 5;        // Controls oversampling of temperature  data
    static constexpr uint8_t OsrsSkipped        = 0;        // Skipped (output set to 0x80000)
    static constexpr uint8_t Osrs1              = 1;        // Oversampling ×1
    static constexpr uint8_t Osrs2              = 2;        // Oversampling ×2
    static constexpr uint8_t Osrs4              = 4;        // Oversampling ×4
    static constexpr uint8_t Osrs8              = 8;        // Oversampling ×8
    static constexpr uint8_t Osrs16             = 9;        // Oversampling ×16
    
    static constexpr uint8_t Config            = 0xF5;      // Sets the rate, filter and interface options of the device
    static constexpr uint8_t ConfigSpi3w       = 1 << 0;    // Enables 3-wire SPI interface when set to ‘1’
    static constexpr uint8_t ConfigFilter      = 2;         // Controls the time constant of the IIR filter
    static constexpr uint8_t ConfigTsb         = 5;         // Controls inactive duration t_standby in normal mode
    
    static constexpr uint8_t Press            = 0xF7;      // The raw pressure measurement output data up[19:0]
    static constexpr uint8_t Temp             = 0xFA;      // The raw temperature measurement output data up[19:0]
    
    static constexpr uint8_t CalibrationData  = 0x88;       // 0x88..0x9F. Values for correction
};
