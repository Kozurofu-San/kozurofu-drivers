#pragma once

#include <cstdint>

class Math
{
    public:
    // =========================
    // 32-bit (Q16.16)
    // =========================

    /**
     * @brief Multiplies two signed 32-bit fixed-point (Q16.16) numbers.
     *
     * This function multiplies two signed fixed-point values where
     * 16 bits represent the integer part and 16 bits represent the fractional part.
     *
     * @param a First operand (Q16.16).
     * @param b Second operand (Q16.16).
     * @return Result of multiplication in Q16.16 format.
     *
     * @note Uses 64-bit intermediate to prevent overflow during multiplication.
     */
    static int32_t mulQ16(int32_t a, int32_t b)
    {
        return static_cast<int32_t>((static_cast<int64_t>(a) * b) >> 16);
    }

    /**
     * @brief Divides two signed 32-bit fixed-point (Q16.16) numbers.
     *
     * Performs division of two fixed-point numbers while preserving precision.
     *
     * @param a Numerator (Q16.16).
     * @param b Denominator (Q16.16).
     * @return Result of division in Q16.16 format.
     *
     * @note Uses 64-bit intermediate to maintain precision.
     * @warning Division by zero is undefined.
     */
    static int32_t divQ16(int32_t a, int32_t b)
    {
        return static_cast<int32_t>((static_cast<int64_t>(a) << 16) / b);
    }

    /**
     * @brief Multiplies two unsigned 32-bit fixed-point (Q16.16) numbers.
     *
     * @param a First operand (Q16.16).
     * @param b Second operand (Q16.16).
     * @return Result of multiplication in Q16.16 format.
     */
    static uint32_t umulQ16(uint32_t a, uint32_t b)
    {
        return static_cast<uint32_t>((static_cast<uint64_t>(a) * b) >> 16);
    }

    /**
     * @brief Divides two unsigned 32-bit fixed-point (Q16.16) numbers.
     *
     * @param a Numerator (Q16.16).
     * @param b Denominator (Q16.16).
     * @return Result of division in Q16.16 format.
     *
     * @warning Division by zero is undefined.
     */
    static uint32_t udivQ16(uint32_t a, uint32_t b)
    {
        return static_cast<uint32_t>((static_cast<uint64_t>(a) << 16) / b);
    }


    // =========================
    // 64-bit (Q32.32)
    // =========================

    /**
     * @brief Multiplies two signed 64-bit fixed-point (Q32.32) numbers.
     *
     * This function multiplies two signed fixed-point values where
     * 32 bits represent the integer part and 32 bits represent the fractional part.
     *
     * @param a First operand (Q32.32).
     * @param b Second operand (Q32.32).
     * @return Result of multiplication in Q32.32 format.
     *
     * @note Uses 128-bit intermediate (__int128) to avoid overflow.
     */
    static int64_t mulQ32(int64_t a, int64_t b)
    {
        return static_cast<int64_t>((static_cast<__int128>(a) * b) >> 32);
    }

    /**
     * @brief Divides two signed 64-bit fixed-point (Q32.32) numbers.
     *
     * @param a Numerator (Q32.32).
     * @param b Denominator (Q32.32).
     * @return Result of division in Q32.32 format.
     *
     * @note Uses 128-bit intermediate (__int128) for precision.
     * @warning Division by zero is undefined.
     */
    static int64_t divQ32(int64_t a, int64_t b)
    {
        return static_cast<int64_t>((static_cast<__int128>(a) << 32) / b);
    }

    /**
     * @brief Multiplies two unsigned 64-bit fixed-point (Q32.32) numbers.
     *
     * @param a First operand (Q32.32).
     * @param b Second operand (Q32.32).
     * @return Result of multiplication in Q32.32 format.
     */
    static uint64_t umulQ32(uint64_t a, uint64_t b)
    {
        return static_cast<uint64_t>((static_cast<__uint128_t>(a) * b) >> 32);
    }

    /**
     * @brief Divides two unsigned 64-bit fixed-point (Q32.32) numbers.
     *
     * @param a Numerator (Q32.32).
     * @param b Denominator (Q32.32).
     * @return Result of division in Q32.32 format.
     *
     * @warning Division by zero is undefined.
     */
    static uint64_t udivQ32(uint64_t a, uint64_t b)
    {
        return static_cast<uint64_t>((static_cast<__uint128_t>(a) << 32) / b);
    }

}