#pragma once

#include "stm32f407xx.h"

namespace driver
{

class DmaDriver
{
    public:

    static constexpr uint8_t InterruptFlag[8] =
    {
        DMA_LIFCR_CTCIF0_Pos,
        DMA_LIFCR_CTCIF1_Pos,
        DMA_LIFCR_CTCIF2_Pos,
        DMA_LIFCR_CTCIF3_Pos,
        DMA_LIFCR_CTCIF0_Pos,
        DMA_LIFCR_CTCIF1_Pos,
        DMA_LIFCR_CTCIF2_Pos,
        DMA_LIFCR_CTCIF3_Pos
    };

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

    static DMA_TypeDef* getDma(DMA_Stream_TypeDef *stream)
    {
        DMA_TypeDef *dma = DMA1;
        if ((uint32_t)stream - (uint32_t)dma > (ChannelsDiff * 8))
        {
            dma = DMA2;
            if ((uint32_t)stream - (uint32_t)dma > (ChannelsDiff * 8))
            {
                dma = nullptr;
            }
        }
        return dma;
    }

    static size_t getStreamNumber(DMA_Stream_TypeDef *stream)
    {
        return ((uint32_t)stream - (uint32_t)getDma(stream) - DmaDriver::ChannelsPad) / DmaDriver::ChannelsDiff;
    }

    static uint32_t* getStatusReg(DMA_Stream_TypeDef *stream)
    {
        auto streamNumber = getStreamNumber(stream);
        auto dma = getDma(stream);
        if (streamNumber < 4)
        {
            return reinterpret_cast<uint32_t*>(dma + offsetof(DMA_TypeDef, LISR));
        }
        else
        {
            return reinterpret_cast<uint32_t*>(dma + offsetof(DMA_TypeDef, HISR));
        }
    }

    static uint32_t* getClearReg(DMA_Stream_TypeDef *stream)
    {
        auto statusReg = getStatusReg(stream);
        return reinterpret_cast<uint32_t*>(reinterpret_cast<uint32_t>(statusReg) + IsrDiff);
    }

    private:

    static constexpr uint32_t ChannelsDiff = DMA1_Stream1_BASE - DMA1_Stream0_BASE;
    static constexpr uint32_t ChannelsPad  = DMA1_Stream0_BASE - DMA1_BASE;
    static constexpr uint32_t IsrDiff = offsetof(DMA_TypeDef, LIFCR) - offsetof(DMA_TypeDef, LISR);
};
}