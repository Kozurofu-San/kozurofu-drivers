#pragma once

#include "interface/Rtt.h"

#include "stm32f1xx.h"

#include "SEGGER_RTT.h"

extern uint32_t SystemCoreClock;

namespace driver
{

/**
 * @brief Concrete implementation of IRtt using official SEGGER RTT.
 *
 * Usage example:
 * @code
 *   SeggerRtt rtt;                 // auto-init on first write
 *   rtt.writeChar(0, 'A');         // write to Terminal channel
 * @endcode
 *
 * Notes:
 * - SEGGER_RTT_PutChar() is used for single-character output.
 * - The function returns the number of bytes written (0 or 1).
 *   This wrapper ignores the return value for simplicity.
 * - RTT initializes itself on the first API call; an explicit
 *   SEGGER_RTT_Init() is still performed in the constructor for
 *   deterministic behaviour.
 */
class RttDriver : public IRtt
{
    public:

    /**
     * @brief Construct and initialize the RTT control block.
     */
    RttDriver()
    {
    }

    /**
     * @brief Explicitly initialize SEGGER RTT.
     * Safe to call multiple times.
     */
    bool init(uint32_t portMask)
    {
        SEGGER_RTT_Init();
        
        _isInit = true;
        return true;
    }

    /**
     * @brief Write one character to the given RTT up-buffer.
     * @param channel  Buffer index (usually 0 for Terminal).
     * @param data     Character to write.
     */
    void writeChar(uint8_t channel, char symbol) override
    {
        (void)SEGGER_RTT_PutChar(static_cast<unsigned>(channel), symbol);
    };

    /**
     * @brief Write a 32-bit unsigned integer as raw little-endian bytes.
     *
     * The host can read exactly 4 bytes and interpret them as uint32_t
     * (native endianness of most PCs is also little-endian).
     *
     * @param channel  RTT up-buffer index
     * @param data     Value to transmit
     */
    void writeInt(uint8_t channel, uint32_t data) override
    {
        // Send the 4 bytes of the integer directly (no text conversion)
        (void)SEGGER_RTT_Write(static_cast<unsigned>(channel),
                               &data,
                               sizeof(data));
    };

    bool isInit() override
    {
        return _isInit;
    }

    private:

    bool _isInit = false;
    
};

}
