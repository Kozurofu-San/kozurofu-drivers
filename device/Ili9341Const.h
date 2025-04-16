#pragma once

#include <cstdint>

class Ili9341
{
    public:

    static constexpr uint8_t    Nop                 = 0x00;
    static constexpr uint8_t    Rst                 = 0x01;
    static constexpr uint8_t    ReadId              = 0x04;
    static constexpr uint8_t    ReadSts             = 0x09;
    static constexpr uint8_t    ReadPowerMode       = 0x0A;
    static constexpr uint8_t    ReadMadCtl          = 0x0B;
    static constexpr uint8_t    ReadPixelFormat     = 0x0C;
    static constexpr uint8_t    ReadImageFormat     = 0x0D;
    static constexpr uint8_t    ReadSignalMode      = 0x0E;
    static constexpr uint8_t    ReadSelfDiagnostic  = 0x0F;
    static constexpr uint8_t    EnterSleepMode      = 0x10;
    static constexpr uint8_t    SleepOut            = 0x11;
    static constexpr uint8_t    PartialModeOn       = 0x12;
    static constexpr uint8_t    NormalModeOn        = 0x13;
    static constexpr uint8_t    DisplayInversionOff = 0x20;
    static constexpr uint8_t    DisplayInversionOn  = 0x21;
    static constexpr uint8_t    GammaSet            = 0x26;
    static constexpr uint8_t    DisplayOff          = 0x28;
    static constexpr uint8_t    DisplayOn           = 0x29;
    static constexpr uint8_t    ColumnAddressSet    = 0x2A;
    static constexpr uint8_t    PageAddressSet      = 0x2B;
    static constexpr uint8_t    MemoryWrite         = 0x2C;
    static constexpr uint8_t    ColorSet            = 0x2D;
    static constexpr uint8_t    MemoryRead          = 0x2E;
    static constexpr uint8_t    PartialArea         = 0x30;
    static constexpr uint8_t    VerticalScrollingDefinition = 0x33;
    static constexpr uint8_t    TearingOff          = 0x34;
    static constexpr uint8_t    TearingOn           = 0x35;
    static constexpr uint8_t    MemoryAccessControl = 0x36;
    static constexpr uint8_t    VerticalScrollingStartAddress = 0x37;
    static constexpr uint8_t    IdleOff             = 0x38;
    static constexpr uint8_t    IdleOn              = 0x39;
    static constexpr uint8_t    PixelFormatSet      = 0x3A;
    static constexpr uint8_t    WriteMemoryContinue = 0x3C;
    static constexpr uint8_t    ReadMemoryContinue  = 0x3E;
    static constexpr uint8_t    SetTearScanline     = 0x44;
    static constexpr uint8_t    GetScanline         = 0x45;
    static constexpr uint8_t    WriteBrightness     = 0x51;
    static constexpr uint8_t    ReadBrightness      = 0x52;
    static constexpr uint8_t    WriteCtrl           = 0x53;
    static constexpr uint8_t    ReadCtrl            = 0x54;
    static constexpr uint8_t    WriteContentAdaptiveBrightnessControl = 0x55;
    static constexpr uint8_t    ReadContentAdaptiveBrightnessControl = 0x56;
    static constexpr uint8_t    WriteCabcMinimumBrightness = 0x5E;
    static constexpr uint8_t    ReadCabcMinimumBrightness = 0x5F;
    static constexpr uint8_t    ReadId1             = 0xDA;
    static constexpr uint8_t    ReadId2             = 0xDB;
    static constexpr uint8_t    ReadId3             = 0xDC;
        
    static constexpr uint8_t    RgbInterfaceSignalControl   = 0xB0;
    static constexpr uint8_t    FrameControlNormal          = 0xB1;
    static constexpr uint8_t    FrameControlIdle            = 0xB2;
    static constexpr uint8_t    FrameControlPartial         = 0xB3;
    static constexpr uint8_t    InversionControl            = 0xB4;
    static constexpr uint8_t    BlankingPorchControl        = 0xB5;
    static constexpr uint8_t    FunctionControl             = 0xB6;
    static constexpr uint8_t    EntryModeSet                = 0xB7;
    static constexpr uint8_t    BacklightControl1           = 0xB8;
    static constexpr uint8_t    BacklightControl2           = 0xB9;
    static constexpr uint8_t    BacklightControl3           = 0xBA;
    static constexpr uint8_t    BacklightControl4           = 0xBB;
    static constexpr uint8_t    BacklightControl5           = 0xBC;
    static constexpr uint8_t    BacklightControl6           = 0xBD;
    static constexpr uint8_t    BacklightControl7           = 0xBE;
    static constexpr uint8_t    BacklightControl8           = 0xBF;
    static constexpr uint8_t    PowerControl1               = 0xC0;
    static constexpr uint8_t    PowerControl2               = 0xC1;
    static constexpr uint8_t    VcomControl1                = 0xC5;
    static constexpr uint8_t    VcomControl2                = 0xC7;
    static constexpr uint8_t    NvMemoryWrite               = 0xD0;
    static constexpr uint8_t    NvMemoryProtectionKey       = 0xD1;
    static constexpr uint8_t    NvMemoryStatusRead          = 0xD2;
    static constexpr uint8_t    ReadId4                     = 0xD3;
    static constexpr uint8_t    PositiveGammaCorrection     = 0xE0;
    static constexpr uint8_t    NegativeGammaCorrection     = 0xE1;
    static constexpr uint8_t    DigitalGammaControl1        = 0xE2;
    static constexpr uint8_t    DigitalGammaControl2        = 0xE3;
    static constexpr uint8_t    InterfaceControl            = 0xF6;
    static constexpr uint8_t    PowerControlA               = 0xCB;
    static constexpr uint8_t    PowerControlB               = 0xCF;
    static constexpr uint8_t    DriverTimingControlA        = 0xE8;
    static constexpr uint8_t    DriverTimingControlAE       = 0xE9;
    static constexpr uint8_t    DriverTimingControlB        = 0xEA;
    static constexpr uint8_t    PowerOnSequenceControl      = 0xED;
    static constexpr uint8_t    Enable3G                    = 0xF2;
    static constexpr uint8_t    PumpRatioControl            = 0xF7;
};