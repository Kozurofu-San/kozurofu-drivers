#pragma once

#include <cstdint>

// Clock generator
class Si5351
{
    public:

    // Registers

    static constexpr uint8_t DeviceStatus           = 0;
        static constexpr uint8_t SYS_INIT               = 1 << 7;   // System Initialization Status
        static constexpr uint8_t LOL_B                  = 1 << 6;   // PLLB Loss Of Lock Status
        static constexpr uint8_t LOL_A                  = 1 << 5;   // PLLA Loss Of Lock Status
        static constexpr uint8_t LOS                    = 1 << 4;   // CLKIN Loss Of Signal (Si5351C only)
        // bits 3:2 Reserved
        static constexpr uint8_t REVID_MASK             = 0x03;     // Revision ID [1:0]

    static constexpr uint8_t InterruptStatusSticky  = 1;
        static constexpr uint8_t SYS_INIT_STKY          = 1 << 7;   // System Initialization Status Sticky
        static constexpr uint8_t LOL_B_STKY             = 1 << 6;   // PLLB Loss Of Lock Status Sticky
        static constexpr uint8_t LOL_A_STKY             = 1 << 5;   // PLLA Loss Of Lock Status Sticky
        static constexpr uint8_t LOS_STKY               = 1 << 4;   // CLKIN Loss Of Signal Sticky (Si5351C only)
        // bits 3:0 Reserved

    static constexpr uint8_t InterruptStatusMask    = 2;
        static constexpr uint8_t SYS_INIT_MASK          = 1 << 7;   // System Initialization Status Mask
        static constexpr uint8_t LOL_B_MASK             = 1 << 6;   // PLLB Loss Of Lock Status Mask
        static constexpr uint8_t LOL_A_MASK             = 1 << 5;   // PLLA Loss Of Lock Status Mask
        static constexpr uint8_t LOS_MASK               = 1 << 4;   // CLKIN Loss Of Signal Mask (Si5351C only)
        // bits 3:0 Reserved

    static constexpr uint8_t OutputEnableControl    = 3;
        static constexpr uint8_t CLK7_OEB               = 1 << 7;   // Output Disable for CLK7
        static constexpr uint8_t CLK6_OEB               = 1 << 6;   // Output Disable for CLK6
        static constexpr uint8_t CLK5_OEB               = 1 << 5;   // Output Disable for CLK5
        static constexpr uint8_t CLK4_OEB               = 1 << 4;   // Output Disable for CLK4
        static constexpr uint8_t CLK3_OEB               = 1 << 3;   // Output Disable for CLK3
        static constexpr uint8_t CLK2_OEB               = 1 << 2;   // Output Disable for CLK2
        static constexpr uint8_t CLK1_OEB               = 1 << 1;   // Output Disable for CLK1
        static constexpr uint8_t CLK0_OEB               = 1 << 0;   // Output Disable for CLK0
        // 0 = Enable CLKx, 1 = Disable CLKx

    static constexpr uint8_t OEBPinEnableControl    = 9;
        static constexpr uint8_t OEB_CLK7               = 1 << 7;   // OEB pin enable control of CLK7
        static constexpr uint8_t OEB_CLK6               = 1 << 6;   // OEB pin enable control of CLK6
        static constexpr uint8_t OEB_CLK5               = 1 << 5;   // OEB pin enable control of CLK5
        static constexpr uint8_t OEB_CLK4               = 1 << 4;   // OEB pin enable control of CLK4
        static constexpr uint8_t OEB_CLK3               = 1 << 3;   // OEB pin enable control of CLK3
        static constexpr uint8_t OEB_CLK2               = 1 << 2;   // OEB pin enable control of CLK2
        static constexpr uint8_t OEB_CLK1               = 1 << 1;   // OEB pin enable control of CLK1
        static constexpr uint8_t OEB_CLK0               = 1 << 0;   // OEB pin enable control of CLK0
        // 0 = OEB pin controls enable/disable of CLKx
        // 1 = OEB pin does not control CLKx

    // Registers 10–14 — Reserved

    static constexpr uint8_t PLLInputSource         = 15;
        // bits 7:4 Reserved
        static constexpr uint8_t PLLB_SRC               = 1 << 3;   // Input Source Select for PLLB
        static constexpr uint8_t PLLA_SRC               = 1 << 2;   // Input Source Select for PLLA
        // bits 1:0 Reserved
        // 0 = XTAL, 1 = CLKIN (Si5351C only)

    // ---- CLKx Control registers (16–23) ----
    // Common bit positions for all CLKx Control registers:

        static constexpr uint8_t CLK_PDN               = 1 << 7;   // Clock x Power Down
        static constexpr uint8_t MS_INT                = 1 << 6;   // MultiSynth x Integer Mode (MS0–MS5)
                                                                // or FBA_INT / FBB_INT for CLK6/CLK7
        static constexpr uint8_t MS_SRC                = 1 << 5;   // MultiSynth Source Select (0=PLLA, 1=PLLB/VCXO)
        static constexpr uint8_t CLK_INV               = 1 << 4;   // Output Clock x Invert
        static constexpr uint8_t CLK_SRC_PLLA          = 0x00;
        static constexpr uint8_t CLK_SRC_PLLB          = MS_SRC;
        static constexpr uint8_t CLK_SRC_MASK          = 0x0C;     // Output Clock x Input Source [3:2]
        static constexpr uint8_t CLK_SRC_XTAL          = 0x00;     // 00: XTAL
        static constexpr uint8_t CLK_SRC_CLKIN         = 0x04;     // 01: CLKIN
        static constexpr uint8_t CLK_SRC_MS            = 0x0C;     // 11: MultiSynth x
        static constexpr uint8_t CLK_IDRV_MASK         = 0x03;     // Drive Strength [1:0]
        static constexpr uint8_t CLK_IDRV_2mA          = 0x00;     // 00: 2 mA
        static constexpr uint8_t CLK_IDRV_4mA          = 0x01;     // 01: 4 mA
        static constexpr uint8_t CLK_IDRV_6mA          = 0x02;     // 10: 6 mA
        static constexpr uint8_t CLK_IDRV_8mA          = 0x03;     // 11: 8 mA

    static constexpr uint8_t CLKxControl             = 16;
        // bit7 CLK0_PDN, bit6 MS0_INT, bit5 MS0_SRC, bit4 CLK0_INV,
        // bits[3:2] CLK0_SRC, bits[1:0] CLK0_IDRV

    // ---- Disable State registers ----

    static constexpr uint8_t CLK3_0_DisableState    = 24;
        // bits[7:6] CLK3_DIS_STATE[1:0]
        // bits[5:4] CLK2_DIS_STATE[1:0]
        // bits[3:2] CLK1_DIS_STATE[1:0]
        // bits[1:0] CLK0_DIS_STATE[1:0]

    static constexpr uint8_t CLK7_4_DisableState    = 25;
        // bits[7:6] CLK7_DIS_STATE[1:0]
        // bits[5:4] CLK6_DIS_STATE[1:0]
        // bits[3:2] CLK5_DIS_STATE[1:0]
        // bits[1:0] CLK4_DIS_STATE[1:0]

        // Common values for CLKx_DIS_STATE[1:0]:
        static constexpr uint8_t CLKx_DIS_LOW           = 0x00;     // 00: LOW when disabled
        static constexpr uint8_t CLKx_DIS_HIGH          = 0x01;     // 01: HIGH when disabled
        static constexpr uint8_t CLKx_DIS_HiZ           = 0x02;     // 10: High-Z when disabled
        static constexpr uint8_t CLKx_DIS_NEVER         = 0x03;     // 11: Never disabled

    // Registers 26–41 — PLL / MultiSynth / output clock delay
    // (via ClockBuilder Pro)

    static constexpr uint8_t PLLAParameters         = 26;
    static constexpr uint8_t PLLBParameters         = 34;

    static constexpr uint8_t MSNA_P3_15_8           = 26;
    static constexpr uint8_t MSNA_P3_7_0            = 27;
    static constexpr uint8_t MSNA_P1_17_16          = 28;
    static constexpr uint8_t MSNA_P1_15_8           = 29;
    static constexpr uint8_t MSNA_P1_7_8            = 30;
    static constexpr uint8_t MSNA_P3_19_16_P2_19_16 = 31;
    static constexpr uint8_t MSNA_P2_15_8           = 32;
    static constexpr uint8_t MSNA_P2_7_0            = 33;

    static constexpr uint8_t MSNB_P3_15_8           = 34;
    static constexpr uint8_t MSNB_P3_7_0            = 35;
    static constexpr uint8_t MSNB_P1_17_16          = 36;
    static constexpr uint8_t MSNB_P1_15_8           = 37;
    static constexpr uint8_t MSNB_P1_7_8            = 38;
    static constexpr uint8_t MSNB_P3_19_16_P2_19_16 = 39;
    static constexpr uint8_t MSNB_P2_15_8           = 40;
    static constexpr uint8_t MSNB_P2_7_0            = 41;

    // ---- Multisynth0 Parameters (registers 42–49) ----
    static constexpr uint8_t MSxParameters          = 42;

    static constexpr uint8_t MSx_P3_15_8            = 42;       // MS0_P3[15:8]
    static constexpr uint8_t MSx_P3_7_0             = 43;       // MS0_P3[7:0]

    static constexpr uint8_t MSx_DIV                = 44;       // R0_DIV[2:0] MS0_P1[17:16]
        // bit7     Unused
        // bits[6:4] R0_DIV[2:0]
        // bits[3:2] Reserved (or MS0_DIVBY4[1:0] in some revisions)
        // bits[1:0] MS0_P1[17:16]

    static constexpr uint8_t MSx_P1_15_8            = 45;       // MS0_P1[15:8]
    static constexpr uint8_t MSx_P1_7_0             = 46;       // MS0_P1[7:0]

    static constexpr uint8_t MSx_P3_19_16_P2_19_16  = 47;       // MS0_P3[19:16] MS0_P2[19:16]
        // bits[7:4] MS0_P3[19:16]
        // bits[3:0] MS0_P2[19:16]

    static constexpr uint8_t MSx_P2_15_8            = 48;       // MS0_P2[15:8]
    static constexpr uint8_t MSx_P2_7_0             = 49;       // MS0_P2[7:0]

        // R Divider values (R0_DIV[2:0] and others):
        static constexpr uint8_t Rx_DIV_1               = 0x00;     // ÷1
        static constexpr uint8_t Rx_DIV_2               = 0x01;     // ÷2
        static constexpr uint8_t Rx_DIV_4               = 0x02;     // ÷4
        static constexpr uint8_t Rx_DIV_8               = 0x03;     // ÷8
        static constexpr uint8_t Rx_DIV_16              = 0x04;     // ÷16
        static constexpr uint8_t Rx_DIV_32              = 0x05;     // ÷32
        static constexpr uint8_t Rx_DIV_64              = 0x06;     // ÷64
        static constexpr uint8_t Rx_DIV_128             = 0x07;     // ÷128

    // Multisynth Parameters repeat every 8 registers
    static constexpr uint8_t MSN = 8;

    // Beginning of SS parameters
    static constexpr uint8_t SpreadSpectrumParameters = 149;

    // CLK Initial Phase Offset
    static constexpr uint8_t CLKxInitialPhaseOffset   = 165;

    // PLL Reset
    static constexpr uint8_t PLLReset   = 177;
    static constexpr uint8_t PLLB_RST   = 1 << 7;   // PLLB_Reset
    static constexpr uint8_t PLLA_RST   = 1 << 5;   // PLLA_Reset
    
    // Crystal Internal Load Capacitance
    static constexpr uint8_t CrystalInternalLoadCapacitance = 183;
    static constexpr uint8_t InternalCL6pF      = 0x40;      // Internal CL = 6 pF
    static constexpr uint8_t InternalCL8pF      = 0x80;      // Internal CL = 8 pF
    static constexpr uint8_t InternalCL10pF     = 0xC0;      // Internal CL = 10 pF
};
