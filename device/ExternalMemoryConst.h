#pragma once

#include <cstdint>

class ExternalMemory
{
    public:

//#define W25Q_BLOCK_COUNT 1024			// W25Q512
//#define W25Q_BLOCK_COUNT 512			// W25Q256
//#define W25Q_BLOCK_COUNT 256			// W25Q128
//#define W25Q_BLOCK_COUNT 128			// W25Q64
//#define W25Q_BLOCK_COUNT 64			// W25Q32
#define W25Q_BLOCK_COUNT 32			// W25Q16
//#define W25Q_BLOCK_COUNT 16			// W25Q80
//#define W25Q_BLOCK_COUNT 8			// W25Q40
//#define W25Q_BLOCK_COUNT 4			// W25Q20
//#define W25Q_BLOCK_COUNT 2			// W25Q10

    static constexpr uint32_t   PageSize                       = 256;
    static constexpr uint32_t   SectorSize                     = 4096;
    static constexpr uint32_t   BlockSize                      = SectorSize * 16;
    static constexpr uint32_t   BlockCount                     = W25Q_BLOCK_COUNT;
#if W25Q_BLOCK_COUNT >= 512
    static constexpr uint32_t   HighCap                        = 1;
#else 
    static constexpr uint32_t   HighCap                        = 0;
#endif
    static constexpr uint32_t   SectorCount                    = BlockCount * 16;
    static constexpr uint32_t   PageCount                      = (SectorCount * SectorSize) / PageSize;
    static constexpr uint32_t   NumKb                          = (SectorCount * SectorSize) / 1024;

    // Status register
    static constexpr uint8_t    StatusBusy                     = 0;
    static constexpr uint8_t    StatusWel                      = 1;
    static constexpr uint8_t    StatusBp0                      = 2;
    static constexpr uint8_t    StatusBp1                      = 3;
    static constexpr uint8_t    StatusBp2                      = 4;
    static constexpr uint8_t    StatusTb                       = 5;
    static constexpr uint8_t    StatusSec                      = 6;
    static constexpr uint8_t    StatusSrp                      = 7;

    // Standard SPI Instructions
    static constexpr uint8_t    WriteEnable                    = 0x06;
    static constexpr uint8_t    VolatileSrWriteEnable          = 0x50;
    static constexpr uint8_t    WriteDisable                   = 0x04;
    static constexpr uint8_t    ReleasePowerDownDeviceId       = 0xAB;
    static constexpr uint8_t    ManufacturerDeviceId           = 0x90;
    static constexpr uint8_t    JedecId                        = 0x9F;
    static constexpr uint8_t    ReadUniqueId                   = 0x4B;
    static constexpr uint8_t    ReadData                       = 0x03;
    static constexpr uint8_t    FastRead                       = 0x0B;
    static constexpr uint8_t    PageProgram                    = 0x02;
    static constexpr uint8_t    SectorErase4Kb                 = 0x20;
    static constexpr uint8_t    BlockErase32Kb                 = 0x52;
    static constexpr uint8_t    BlockErase64Kb                 = 0xD8;
    static constexpr uint8_t    ChipErase                      = 0xC7;
    static constexpr uint8_t    ReadStatusRegister1            = 0x05;
    static constexpr uint8_t    WriteStatusRegister1           = 0x01;
    static constexpr uint8_t    ReadStatusRegister2            = 0x35;
    static constexpr uint8_t    WriteStatusRegister2           = 0x31;
    static constexpr uint8_t    ReadStatusRegister3            = 0x15;
    static constexpr uint8_t    WriteStatusRegister3           = 0x11;
    static constexpr uint8_t    ReadSfdpRegister               = 0x5A;
    static constexpr uint8_t    EraseSecurityRegister          = 0x44;
    static constexpr uint8_t    ProgramSecurityRegister        = 0x42;
    static constexpr uint8_t    ReadSecurityRegister           = 0x48;
    static constexpr uint8_t    GlobalBlockLock                = 0x7E;
    static constexpr uint8_t    GlobalBlockUnlock              = 0x98;
    static constexpr uint8_t    ReadBlockLock                  = 0x3D;
    static constexpr uint8_t    IndividualBlockLock            = 0x36;
    static constexpr uint8_t    IndividualBlockUnlock          = 0x39;
    static constexpr uint8_t    EraseProgramSuspend            = 0x75;
    static constexpr uint8_t    EraseProgramResume             = 0x7A;
    static constexpr uint8_t    PowerDown                      = 0xB9;
    static constexpr uint8_t    EnableReset                    = 0x66;
    static constexpr uint8_t    ResetDevice                    = 0x99;

    // Dual/Quad SPI Instructions
    static constexpr uint8_t    FastReadDualOutput             = 0x3B;
    static constexpr uint8_t    FastReadDualIo                 = 0xBB;
    static constexpr uint8_t    ManufacturerDeviceIdDual       = 0x92;
    static constexpr uint8_t    QuadPageProgram                = 0x32;
    static constexpr uint8_t    FastReadQuadOutput             = 0x6B;
    static constexpr uint8_t    ManufacturerDeviceIdQuad       = 0x94;
    static constexpr uint8_t    FastReadQuadIo                 = 0xEB;
    static constexpr uint8_t    SetBurstWithWrap               = 0x77;

};
