#pragma once

#include <cstdint>
#include <cmath>

class Image
{

public:
    
    /**
     * @brief Defines output byte order for RGB components.
     */
    enum class Order : uint8_t
    {
        RGB, ///< R, G, B
        BGR  ///< B, G, R
    };

private:

    /**
     * @brief Writes RGB components into output array using selected order.
     */
    static inline void writeOrdered(uint8_t r, uint8_t g, uint8_t b, uint8_t* out, Order order)
    {
        if (order == Order::RGB)
        {
            out[0] = r; out[1] = g; out[2] = b;
        }
        else
        {
            out[0] = b; out[1] = g; out[2] = r;
        }
    }

    static inline void readOrdered(const uint8_t* in, uint8_t& r, uint8_t& g, uint8_t& b, Order order)
    {
        if (order == Order::RGB)
        {
            r = in[0]; g = in[1]; b = in[2];
        }
        else
        {
            r = in[2]; g = in[1]; b = in[0];
        }
    }


public:
    // =========================
    // RGB888
    // =========================

    /**
     * @brief Converts a 32-bit RGB888 color to a 3-byte array with configurable channel order.
     *
     * Extracts red, green, and blue components from a 32-bit integer.
     * Expected format: 0x00RRGGBB (alpha channel is ignored if present).
     *
     * @param color Input color in RGB888 format.
     * @param out Pointer to an array of at least 3 bytes.
     * @param order Desired output channel order (RGB or BGR).
     */
    static void rgb888ToArray(uint32_t color, uint8_t* out, Order order = Order::RGB)
    {
        uint8_t r = static_cast<uint8_t>((color >> 16) & 0xFF);
        uint8_t g = static_cast<uint8_t>((color >> 8) & 0xFF);
        uint8_t b = static_cast<uint8_t>(color & 0xFF);

        writeOrdered(r, g, b, out, order);
    }

    /**
     * @brief Converts a 32-bit RGB888 color to separate components with configurable order.
     *
     * @param color Input color in RGB888 format.
     * @param c0 First output component (depends on order).
     * @param c1 Second output component (depends on order).
     * @param c2 Third output component (depends on order).
     * @param order Desired output channel order (RGB or BGR).
     */
    static void rgb888ToArray(uint32_t color, uint8_t& c0, uint8_t& c1, uint8_t& c2, Order order = Order::RGB)
    {
        uint8_t r = static_cast<uint8_t>((color >> 16) & 0xFF);
        uint8_t g = static_cast<uint8_t>((color >> 8) & 0xFF);
        uint8_t b = static_cast<uint8_t>(color & 0xFF);

        if (order == Order::RGB)
        {
            c0 = r; c1 = g; c2 = b;
        }
        else
        {
            c0 = b; c1 = g; c2 = r;
        }
    }


    // =========================
    // RGB565
    // =========================

    /**
     * @brief Converts a 16-bit RGB565 color to a 3-byte array with configurable channel order.
     *
     * Expands 5-bit red, 6-bit green, and 5-bit blue into 8-bit values.
     *
     * @param color Input color in RGB565 format.
     * @param out Pointer to an array of at least 3 bytes.
     * @param order Desired output channel order (RGB or BGR).
     */
    static void rgb565ToArray(uint16_t color, uint8_t* out, Order order = Order::RGB)
    {
        uint8_t r5 = static_cast<uint8_t>((color >> 11) & 0x1F);
        uint8_t g6 = static_cast<uint8_t>((color >> 5) & 0x3F);
        uint8_t b5 = static_cast<uint8_t>(color & 0x1F);

        uint8_t r = static_cast<uint8_t>((r5 << 3) | (r5 >> 2));
        uint8_t g = static_cast<uint8_t>((g6 << 2) | (g6 >> 4));
        uint8_t b = static_cast<uint8_t>((b5 << 3) | (b5 >> 2));

        writeOrdered(r, g, b, out, order);
    }

    /**
     * @brief Converts a 16-bit RGB565 color to separate components with configurable order.
     *
     * @param color Input color in RGB565 format.
     * @param c0 First output component (depends on order).
     * @param c1 Second output component (depends on order).
     * @param c2 Third output component (depends on order).
     * @param order Desired output channel order (RGB or BGR).
     */
    static void rgb565ToArray(uint16_t color, uint8_t& c0, uint8_t& c1, uint8_t& c2, Order order = Order::RGB)
    {
        uint8_t r5 = static_cast<uint8_t>((color >> 11) & 0x1F);
        uint8_t g6 = static_cast<uint8_t>((color >> 5) & 0x3F);
        uint8_t b5 = static_cast<uint8_t>(color & 0x1F);

        uint8_t r = static_cast<uint8_t>((r5 << 3) | (r5 >> 2));
        uint8_t g = static_cast<uint8_t>((g6 << 2) | (g6 >> 4));
        uint8_t b = static_cast<uint8_t>((b5 << 3) | (b5 >> 2));

        if (order == Order::RGB)
        {
            c0 = r; c1 = g; c2 = b;
        }
        else
        {
            c0 = b; c1 = g; c2 = r;
        }
    }

    
    // =========================
    // HSV -> RGB
    // =========================

    /**
     * @brief Converts HSV color to RGB (8-bit per channel).
     *
     * @param h Hue in range [0, 360).
     * @param s Saturation in range [0, 1].
     * @param v Value in range [0, 1].
     * @param r Output red channel (0-255).
     * @param g Output green channel (0-255).
     * @param b Output blue channel (0-255).
     */
    static void hsvToRgb(float h, float s, float v, uint8_t& r, uint8_t& g, uint8_t& b)
    {
        float c = v * s;
        float x = c * (1.0f - std::fabsf(std::fmod(h / 60.0f, 2.0f) - 1.0f));
        float m = v - c;

        float rf = 0, gf = 0, bf = 0;

        if      (h < 60)  { rf = c; gf = x; bf = 0; }
        else if (h < 120) { rf = x; gf = c; bf = 0; }
        else if (h < 180) { rf = 0; gf = c; bf = x; }
        else if (h < 240) { rf = 0; gf = x; bf = c; }
        else if (h < 300) { rf = x; gf = 0; bf = c; }
        else              { rf = c; gf = 0; bf = x; }

        r = static_cast<uint8_t>((rf + m) * 255.0f);
        g = static_cast<uint8_t>((gf + m) * 255.0f);
        b = static_cast<uint8_t>((bf + m) * 255.0f);
    }

    /**
     * @brief Converts HSV color to RGB888 (0x00RRGGBB).
     *
     * @return Packed RGB888 color.
     */
    static uint32_t hsvToRgb888(float h, float s, float v)
    {
        uint8_t r, g, b;
        hsvToRgb(h, s, v, r, g, b);
        return (static_cast<uint32_t>(r) << 16) |
               (static_cast<uint32_t>(g) << 8)  |
               b;
    }

    /**
     * @brief Converts HSV color to RGB565 (16-bit).
     *
     * @return Packed RGB565 color.
     */
    static uint16_t hsvToRgb565(float h, float s, float v)
    {
        uint8_t r, g, b;
        hsvToRgb(h, s, v, r, g, b);

        return static_cast<uint16_t>(
            ((r >> 3) << 11) |
            ((g >> 2) << 5)  |
            (b >> 3)
        );
    }

    /**
     * @brief Converts HSV color to byte array with configurable order.
     *
     * @param out Output array (size >= 3).
     * @param order Channel order.
     */
    static void hsvToArray(float h, float s, float v, uint8_t* out, Order order = Order::RGB)
    {
        uint8_t r, g, b;
        hsvToRgb(h, s, v, r, g, b);
        writeOrdered(r, g, b, out, order);
    }


    // =========================
    // RGB -> HSV
    // =========================

    /**
     * @brief Converts RGB (8-bit per channel) to HSV.
     *
     * @param r Red channel (0-255).
     * @param g Green channel (0-255).
     * @param b Blue channel (0-255).
     * @param h Output hue [0, 360).
     * @param s Output saturation [0, 1].
     * @param v Output value [0, 1].
     */
    static void rgbToHsv(uint8_t r, uint8_t g, uint8_t b, float& h, float& s, float& v)
    {
        float rf = r / 255.0f;
        float gf = g / 255.0f;
        float bf = b / 255.0f;

        float max = std::fmax(rf, std::fmax(gf, bf));
        float min = std::fmin(rf, std::fmin(gf, bf));
        float delta = max - min;

        v = max;

        if (delta < 1e-6f)
        {
            h = 0;
            s = 0;
            return;
        }

        s = delta / max;

        if (areEqual(max, rf))
        {
            h = 60.0f * std::fmod(((gf - bf) / delta), 6.0f);
        }
        else if (areEqual(max, gf))
        {
            h = 60.0f * (((bf - rf) / delta) + 2.0f);
        }
        else
        {
            h = 60.0f * (((rf - gf) / delta) + 4.0f);
        }

        if (h < 0) h += 360.0f;
    }

    /**
     * @brief Compare floating-point numbers.
     */
    static bool areEqual(float& a, float& b)
    {
        // Scale epsilon relative to the larger magnitude of the two numbers
        return std::fabs(a - b) <= 0.00001;
    }

    /**
     * @brief Converts RGB888 (0x00RRGGBB) to HSV.
     */
    static void rgb888ToHsv(uint32_t color, float& h, float& s, float& v)
    {
        uint8_t r = (color >> 16) & 0xFF;
        uint8_t g = (color >> 8) & 0xFF;
        uint8_t b = color & 0xFF;

        rgbToHsv(r, g, b, h, s, v);
    }

    /**
     * @brief Converts RGB565 to HSV.
     */
    static void rgb565ToHsv(uint16_t color, float& h, float& s, float& v)
    {
        uint8_t r5 = (color >> 11) & 0x1F;
        uint8_t g6 = (color >> 5) & 0x3F;
        uint8_t b5 = color & 0x1F;

        uint8_t r = (r5 << 3) | (r5 >> 2);
        uint8_t g = (g6 << 2) | (g6 >> 4);
        uint8_t b = (b5 << 3) | (b5 >> 2);

        rgbToHsv(r, g, b, h, s, v);
    }

    /**
     * @brief Converts RGB byte array to HSV.
     *
     * @param in Input array (size >= 3).
     * @param order Channel order.
     */
    static void arrayToHsv(const uint8_t* in, float& h, float& s, float& v, Order order = Order::RGB)
    {
        uint8_t r, g, b;
        readOrdered(in, r, g, b, order);
        rgbToHsv(r, g, b, h, s, v);
    }
};