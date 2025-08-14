#pragma once

#include "stm32f407xx.h"

namespace driver
{

class DmaDriver
{
    public:

    enum class Direction
    {
        PeripheralToMemory  = 0 << DMA_SxCR_DIR_Pos,
        MemoryToPeripheral  = 1 << DMA_SxCR_DIR_Pos,
        MemoryToMemory      = 2 << DMA_SxCR_DIR_Pos,
    };

    enum class Priority
    {
        Low         = 0 << DMA_SxCR_PL_Pos,
        Medium      = 1 << DMA_SxCR_PL_Pos,
        High        = 2 << DMA_SxCR_PL_Pos,
        VeryHigh    = 3 << DMA_SxCR_PL_Pos,
    };
};
}