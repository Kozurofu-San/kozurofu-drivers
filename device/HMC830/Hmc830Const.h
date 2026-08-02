#pragma once

#include <cstdint>

// Clock generator
class Hmc830
{
    public:

    // Registers

    // R0 ID Register (Read Only)
    static constexpr uint8_t ID = 0x00;

    // R0 Open Mode Read Address/RST Strobe Register (Write Only)
    static constexpr uint8_t OpenMode = 0x00;
    static constexpr uint8_t ReadAddress                = 0x1F;     // Read Address for next cycle 
    static constexpr uint8_t ReadAddress_pos            = 0x00;     // Read Address for next cycle
    static constexpr uint8_t SoftReset                  = 1 << 5;   // Soft Reset - both SPI modes reset (set to 0 for proper operation)

    // R1 Enables Register
    static constexpr uint8_t Enables = 0x01;
    static constexpr uint8_t chipen_pin_select          = 1 << 0;    // Selects whether device enable is controlled by the CHIP_EN pin or SPI.
    static constexpr uint8_t chipen_from_spi            = 1 << 1;    // Software chip-enable control when SPI mode is selected.
    static constexpr uint8_t Keep_bias_on               = 1 << 2;    // Keeps the internal bias generator powered during standby.
    static constexpr uint8_t Keep_PD_on                 = 1 << 3;    // Keeps the phase detector powered.
    static constexpr uint8_t Keep_CP_on                 = 1 << 4;    // Keeps the charge pump powered.
    static constexpr uint8_t Keep_Ref_buf_on            = 1 << 5;    // Keeps the reference input buffer enabled.
    static constexpr uint8_t Keep_VCO_on                = 1 << 6;    // Keeps the VCO circuitry powered.
    static constexpr uint8_t Keep_GPO_driver_on         = 1 << 7;    // Keeps the GPO output driver active.

    // R2 REFDIV Register
    static constexpr uint8_t REFDIV                     = 0x02;

    // R3 Frequency Register - Integer Part
    static constexpr uint8_t FrequencyInteger           = 0x03;

    // R4 Frequency Register - Fractional Part
    static constexpr uint8_t FrequencyFractional        = 0x04;

    // R5 VCO SPI Register
    static constexpr uint8_t VCOSPI = 0x05;
    static constexpr uint8_t VCO_Subsystem_ID               = 0;    // Selects the target VCO subsystem.
    static constexpr uint8_t VCO_Subsystem_register_address = 3;    // Address of the VCO subsystem register.
    static constexpr uint8_t VCO_Subsystem_data             = 7;    // Data transferred to or from the selected VCO register.
    
    // R6 SD CFG Register
    static constexpr uint8_t SDCFG = 0x06;
    static constexpr uint8_t seed                    = 0;   // Initial seed value for the sigma-delta modulator.
    static constexpr uint8_t order                   = 2;   // Selects sigma-delta modulator order.
    static constexpr uint8_t frac_bypass             = 7;   // Bypasses fractional modulation for integer-N operation.
    static constexpr uint8_t AutoSeed                = 8;   // Automatically generates the sigma-delta seed.
    static constexpr uint8_t clkrq_refdiv_sel        = 9;   // Selects the reference divider clock source.
    static constexpr uint8_t SD_Modulator_Clk_Select = 10;  // Selects the sigma-delta modulator clock source.
    static constexpr uint8_t SD_Enable               = 11;  // Enables fractional sigma-delta modulation.
    static constexpr uint8_t BIST_Enable             = 18;  // Enables sigma-delta built-in self-test.
    static constexpr uint8_t RDiv_BIST_Cycles        = 19;  // Number of reference-divider cycles used during BIST.
    static constexpr uint8_t auto_clock_config       = 21;  // Automatically configures internal clock routing.

    // R7 Lock Detect Register 
    static constexpr uint8_t LockDetect = 0x07;
    static constexpr uint8_t lkd_wincnt_max                = 0;     // Number of consecutive valid lock windows required.
    static constexpr uint8_t Enable_Internal_Lock_Detect   = 3;     // Enables the internal digital lock detector.
    static constexpr uint8_t Lock_Detect_Window_type       = 6;     // Selects lock detection window behavior.
    static constexpr uint8_t LD_Digital_Window_duration    = 7;     // Sets digital lock detection window length.
    static constexpr uint8_t LD_Digital_Timer_Freq_Control = 10;    // Adjusts digital lock detector timing.
    static constexpr uint8_t LD_Timer_Test_Mode            = 12;    // Enables lock detector timer test mode.
    static constexpr uint8_t Auto_Relock_One_Try           = 13;    // Automatically retries calibration after lock loss.

    // R8 Analog EN Register
    static constexpr uint8_t AnalogEn = 0x08;
    static constexpr uint8_t bias_en                               = 0;     // Enables the internal bias circuitry.
    static constexpr uint8_t cp_en                                 = 1;     // Enables the charge pump.
    static constexpr uint8_t PD_en                                 = 2;     // Enables the phase detector.
    static constexpr uint8_t refbuf_en                             = 3;     // Enables the reference input buffer.
    static constexpr uint8_t vcobuf_en                             = 4;     // Enables the VCO output buffer.
    static constexpr uint8_t gpo_pad_en                            = 5;     // Enables the GPO output driver.
    static constexpr uint8_t VCO_Div_Clk_to_dig_en                 = 7;     // Routes the VCO divider clock to the digital logic.
    static constexpr uint8_t Prescaler_Clock_enable                = 9;     // Enables the prescaler clock.
    static constexpr uint8_t VCO_Buffer_and_Prescaler_Bias_Enable  = 10;    // Enables bias for the VCO buffer and prescaler.
    static constexpr uint8_t Charge_Pump_Internal_Opamp_enable     = 11;    // Enables the internal charge pump amplifier.
    static constexpr uint8_t High_Frequency_Reference              = 21;    // Optimizes operation for high-frequency reference inputs.

    // R9 Charge Pump Register
    static constexpr uint8_t ChargePump = 0x09;
    static constexpr uint8_t CP_DN_Gain       = 0;      // Sets the charge pump sink current.
    static constexpr uint8_t CP_UP_Gain       = 7;      // Sets the charge pump source current.
    static constexpr uint8_t Offset_Magnitude = 14;     // Magnitude of programmable charge pump offset current.
    static constexpr uint8_t Offset_UP_enable = 21;     // Enables positive offset current.
    static constexpr uint8_t Offset_DN_enable = 22;     // Enables negative offset current.
    static constexpr uint8_t HiKcp            = 23;     // Selects the high-gain charge pump mode.

    // R10 AutoCal Configuration Register
    static constexpr uint8_t VCOAutoCal = 0x0A;
    static constexpr uint8_t Vtune_Resolution           = 0;     // Sets VTUNE measurement resolution.
    static constexpr uint8_t VCO_Curve_Adjustment       = 3;     // Adjusts the selected VCO tuning curve.
    static constexpr uint8_t Wait_State_Set_Up          = 6;     // Sets delay before calibration begins.
    static constexpr uint8_t Num_of_SAR_BIts_in_VCO     = 8;     // Number of SAR search bits used during calibration.
    static constexpr uint8_t Force_Curve                = 10;    // Forces use of a specific VCO tuning curve.
    static constexpr uint8_t Bypass_VCO_Tuning          = 11;    // Disables automatic VCO calibration.
    static constexpr uint8_t No_VSPI_Trigger            = 12;    // Prevents automatic VCO SPI transactions.
    static constexpr uint8_t FSM_VSPI_Clock_Select      = 13;    // Selects FSM clock for VCO SPI interface.
    static constexpr uint8_t Xtal_Falling_Edge_for_FSM  = 15;    // Uses the crystal clock falling edge for FSM timing.
    static constexpr uint8_t Force_RDivider_Bypass      = 16;    // Forces the reference divider to be bypassed.

    // R11 PD Register
    static constexpr uint8_t PD = 0x0B;
    static constexpr uint8_t PD_del_sel             = 0;    // Selects phase detector delay.
    static constexpr uint8_t Short_PD_Inputs        = 3;    // Shorts the phase detector inputs for test.
    static constexpr uint8_t pd_phase_sel           = 4;    // Selects phase detector operating polarity.
    static constexpr uint8_t PD_up_en               = 5;    // Enables UP output from the phase detector.
    static constexpr uint8_t PD_dn_en               = 6;    // Enables DOWN output from the phase detector.
    static constexpr uint8_t CSP_Mode               = 7;    // Selects charge sharing compensation mode.
    static constexpr uint8_t Force_CP_UP            = 9;    // Forces the charge pump UP output active.
    static constexpr uint8_t Force_CP_DN            = 10;   // Forces the charge pump DOWN output active.
    static constexpr uint8_t Force_CP_MId_Rail      = 11;   // Forces the charge pump output to mid-supply.
    static constexpr uint8_t CP_Internal_OpAmp_Bias = 15;   // Sets internal charge pump amplifier bias.
    static constexpr uint8_t MCounter_Clock_Gating  = 17;   // Enables clock gating for the M counter.

    // R12 Fine Frequency Correction Register
    static constexpr uint8_t FineFrequencyCorrection    = 0x0C;

    // R15 GPO_SPI_RDIV Register
    static constexpr uint8_t GPO_SPI_RDIV = 0x0F;
    static constexpr uint8_t gpo_select             = 0;    // Selects the signal routed to the GPO pin.
    static constexpr uint8_t GPO_Test_Data          = 5;    // Selects internal test data for GPO output.
    static constexpr uint8_t Prevent_Automux_SDO    = 6;    // Prevents automatic multiplexing of the SDO pin.
    static constexpr uint8_t LDO_Driver_Always_On   = 7;    // Keeps the output driver supply enabled.
    static constexpr uint8_t Disable_PFET           = 8;    // Disables the pull-up output transistor.
    static constexpr uint8_t Disable_NFET           = 9;    // Disables the pull-down output transistor.

    // R16 VCO Tune Register
    static constexpr uint8_t VCOTune = 0x10;
    static constexpr uint8_t VCO_Switch_Setting     = 0;    // Current VCO band selected by calibration.
    static constexpr uint8_t AutoCal_Busy           = 0x80; // Indicates that automatic VCO calibration is in progress.

    // R17 SAR Register
    static constexpr uint8_t SAR = 0x11;
    static constexpr uint8_t SAR_Error_Mag_Counts   = 0;    // Magnitude of the SAR tuning error.
    static constexpr uint8_t SAR_Error_Sign         = 19;   // Sign of the SAR tuning error.

    // R18 GPO2 Register
    static constexpr uint8_t GPO2 = 0x12;
    static constexpr uint8_t GPO                    = 0;    // Drives the programmable GPO output.
    static constexpr uint8_t Lock_Detect            = 1;    // Reflects the lock detector status.

    // R19 BIST Register
    static constexpr uint8_t BIST = 0x13;
    static constexpr uint8_t BIST_Signature         = 0;    // Signature produced by the built-in self-test.
    static constexpr uint8_t BIST_Busy              = 16;   // Indicates that the built-in self-test is running.

    // VCO subsystem registers
    
    // R0 Tuning
    static constexpr uint8_t VCO_Tuning = 0x00;
    static constexpr uint8_t Cal                    = 0;    // Starts or controls VCO calibration.
    static constexpr uint8_t CAPS                   = 1;    // Sets or reports the switched capacitor tuning value.

    // R1 Enables
    static constexpr uint8_t VCO_Enables = 0x01;
    static constexpr uint8_t Master_Enable_VCO_Subsystem    = 0;    // Master enable for the VCO subsystem.
    static constexpr uint8_t Manual_Mode_PLL_buffer_enable  = 1;    // Enables the PLL output buffer in manual mode.
    static constexpr uint8_t Manual_Mode_RF_buffer_enable   = 2;    // Enables the RF output buffer in manual mode.
    static constexpr uint8_t Manual_Mode_Divide_by_1_enable = 3;    // Enables the divide-by-1 output path.
    static constexpr uint8_t Manual_Mode_RF_Divider_enable  = 4;    // Enables the RF output divider in manual mode.

    static constexpr uint8_t VCO_Biases = 0x02;
    static constexpr uint8_t RF_Divide_ratio                    = 0;
    static constexpr uint8_t RF_output_buffer_gain_control      = 6;
    static constexpr uint8_t Divider_output_stage_gain_control  = 8;

    static constexpr uint8_t VCO_Config = 0x03;
    static constexpr uint8_t RF_buffer_SE_enable    = 0;            // 1 - Single ended, 0 - differential output
    static constexpr uint8_t Manual_RFO_Mode        = 2;            // 0 - AutoRFO mode controls output buffers and RF divider enables according to RF divider setting in VCO_Reg 02h[5:0]
    static constexpr uint8_t RF_buffer_bias         = 3;
};
