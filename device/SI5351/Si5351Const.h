#pragma once

#include <cstdint>

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

    static constexpr uint8_t CLKx_PDN               = 1 << 7;   // Clock x Power Down
    static constexpr uint8_t MSx_INT                = 1 << 6;   // MultiSynth x Integer Mode (MS0–MS5)
                                                               // or FBA_INT / FBB_INT for CLK6/CLK7
    static constexpr uint8_t MSx_SRC                = 1 << 5;   // MultiSynth Source Select (0=PLLA, 1=PLLB/VCXO)
    static constexpr uint8_t CLKx_INV               = 1 << 4;   // Output Clock x Invert
    static constexpr uint8_t CLKx_SRC_MASK          = 0x0C;     // Output Clock x Input Source [3:2]
    static constexpr uint8_t CLKx_SRC_XTAL          = 0x00;     // 00: XTAL
    static constexpr uint8_t CLKx_SRC_CLKIN         = 0x04;     // 01: CLKIN
    static constexpr uint8_t CLKx_SRC_MS            = 0x0C;     // 11: MultiSynth x
    static constexpr uint8_t CLKx_IDRV_MASK         = 0x03;     // Drive Strength [1:0]
    static constexpr uint8_t CLKx_IDRV_2mA          = 0x00;     // 00: 2 mA
    static constexpr uint8_t CLKx_IDRV_4mA          = 0x01;     // 01: 4 mA
    static constexpr uint8_t CLKx_IDRV_6mA          = 0x02;     // 10: 6 mA
    static constexpr uint8_t CLKx_IDRV_8mA          = 0x03;     // 11: 8 mA

    static constexpr uint8_t CLK0Control            = 16;
    // bit7 CLK0_PDN, bit6 MS0_INT, bit5 MS0_SRC, bit4 CLK0_INV,
    // bits[3:2] CLK0_SRC, bits[1:0] CLK0_IDRV

    static constexpr uint8_t CLK1Control            = 17;
    // bit7 CLK1_PDN, bit6 MS1_INT, bit5 MS1_SRC, bit4 CLK1_INV,
    // bits[3:2] CLK1_SRC, bits[1:0] CLK1_IDRV

    static constexpr uint8_t CLK2Control            = 18;
    // bit7 CLK2_PDN, bit6 MS2_INT, bit5 MS2_SRC, bit4 CLK2_INV,
    // bits[3:2] CLK2_SRC, bits[1:0] CLK2_IDRV

    static constexpr uint8_t CLK3Control            = 19;
    // bit7 CLK3_PDN, bit6 MS3_INT, bit5 MS3_SRC, bit4 CLK3_INV,
    // bits[3:2] CLK3_SRC, bits[1:0] CLK3_IDRV

    static constexpr uint8_t CLK4Control            = 20;
    // bit7 CLK4_PDN, bit6 MS4_INT, bit5 MS4_SRC, bit4 CLK4_INV,
    // bits[3:2] CLK4_SRC, bits[1:0] CLK4_IDRV

    static constexpr uint8_t CLK5Control            = 21;
    // bit7 CLK5_PDN, bit6 MS5_INT, bit5 MS5_SRC, bit4 CLK5_INV,
    // bits[3:2] CLK5_SRC, bits[1:0] CLK5_IDRV

    static constexpr uint8_t CLK6Control            = 22;
    // bit7 CLK6_PDN, bit6 FBA_INT, bit5 MS6_SRC, bit4 CLK6_INV,
    // bits[3:2] CLK6_SRC, bits[1:0] CLK6_IDRV
    // Note: FBA_INT — Force PLLA into integer mode

    static constexpr uint8_t CLK7Control            = 23;
    // bit7 CLK7_PDN, bit6 FBB_INT, bit5 MS7_SRC (or MS6_SRC in some docs), bit4 CLK7_INV,
    // bits[3:2] CLK7_SRC, bits[1:0] CLK7_IDRV
    // Note: FBB_INT — Force PLLB into integer mode
};