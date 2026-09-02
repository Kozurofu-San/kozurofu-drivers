#pragma once

#include "Pcd8544Const.h"

#include "interface/Log.h"
#include "interface/Timer.h"
#include "interface/Spi.h"
#include "interface/Gpio.h"
#include "utils/Fonts.h"

#include <cstdio>

/* // Nokia LCD display

#include "device/Hmc830/Hmc830.h"

    SpiController spi1 {SPI1};
    GpioDriver gpio_rfgen_cs {GPIOA, 4};
    SpiDriver spi_rfgen {spi1};
    Hmc830Driver rfgen {spi_rfgen, timer_us};

    // SPI1
    GpioDriver::remap(AFIO_MAPR_SPI1_REMAP, false);
    GpioDriver::mode(GPIOA, 4, GpioDriver::Mode::AlternatePushpull); // NSS
    GpioDriver::mode(GPIOA, 5, GpioDriver::Mode::AlternatePushpull); // SCK
    GpioDriver::mode(GPIOA, 6, GpioDriver::Mode::Input);             // MISO
    GpioDriver::mode(GPIOA, 7, GpioDriver::Mode::AlternatePushpull); // MOSI
    CHECK(p.spi1.init(1'000'000));

    // Timer
    CHECK(p.timer_us.init({1, ITimer::Units::us}));
    p.timer_us.start();

    // RF generator
    p.gpio_rfgen_cs.init(GpioDriver::Mode::OutputPushpull, GpioDriver::Speed::High);
    p.spi_rfgen.init(&p.gpio_rfgen_cs, 0);
    p.rfgen.init(Hmc830Driver::Mode::Integer);

    // Example
    bool ret = p.rfgen.setFrequency(50'000'000);
    p.rfgen.setPower(9);
*/

namespace driver
{

class Pcd8544Driver: public ILog
{

public:

    Pcd8544Driver(ISpi &p, IGpio &dc, IGpio &rst, ITimer &timer)
        : _p(p), _dc(dc), _rst(rst), _timer(timer) {}

    bool init(IGpio *bl = nullptr)
    {
        _bl = bl;

        if (_p.getSpeed() > MaxSpeed)
        {
            return false;
        }

        reset();

        // Extended instruction set
        writeByte(Pcd8544::FunctionSet | Pcd8544::Extended | Pcd8544::Horizontal); // 0x21

        // Bias system (1:48 is common, try 0x13 or 0x14)
        writeByte(Pcd8544::BiasSystem | 0x04); // 0x14

        // Temperature coefficient (TC0 is usual)
        writeByte(Pcd8544::TemperatureControl | 0x00); // 0x04

        // Contrast / VOP (0xB0..0xBF typical; 0xB8 is a good starting point)
        writeByte(Pcd8544::SetVOP | 0x38); // 0xB8

        // Back to basic instruction set, horizontal addressing
        writeByte(Pcd8544::FunctionSet | Pcd8544::Horizontal); // 0x20

        // Normal display mode
        writeByte(Pcd8544::DisplayControl | Pcd8544::Normal); // 0x0C

        // Clear display (optional but recommended)
        clear();

        // Backlight on (active-high assumed; invert if your module is common-anode)
        if (_bl) {
            _bl->write(1);
        }

        _isInit = true;
        return true;
    }

    void print(uint8_t channel, const char* message, ...) override
    {
        if (!_isInit || channel > 5) {
            return;
        }

        char buf[15];                       // 14 chars max + null (84 / 6 = 14)
        va_list args;
        va_start(args, message);
        vsnprintf(buf, sizeof(buf), message, args);
        va_end(args);

        // Set cursor to start of the requested row
        setCursor(0, channel);

        // Render characters (5 px wide + 1 px spacing = 6)
        for (uint8_t i = 0; buf[i] != '\0' && i < 14; ++i) {
            writeChar(buf[i]);
        }

        // Optional: clear remaining columns of the line
        // (uncomment if you want the rest of the row blanked)
        /*
        uint8_t remaining = 14 - strlen(buf);
        for (uint8_t i = 0; i < remaining; ++i) {
            writeChar(' ');
        }
        */
    }

    void print(uint8_t channel, int32_t value) override
    {   // Not implemented
    }

    bool isInit() override
    {
        return _isInit;
    }

private:

    ISpi   &_p;     // SPI: CLK, DIN, CE
    IGpio  &_dc;    // Latch
    IGpio  &_rst;   // Reset
    ITimer &_timer;

    // Optional
    IGpio *_bl;     // Backlight

    uint32_t MaxSpeed = 4'000'000;  // Hz

    bool _isInit = false;

    // Helpers
    void write(uint8_t *data, uint16_t len)
    {
        _p.enable();
        _p.write(data, len);
        _dc.write(1);
        _dc.write(0);
        _p.disable();
    }

    void writeByte(uint8_t data)
    {
        _p.enable();
        _p.transfer(data);
        _dc.write(1);
        _dc.write(0);
        _p.disable();
    }

    void reset()
    {
        _rst.write(0);
        _timer.delay(100);
        _rst.write(1);
        _timer.delay(10);   // Small settle time
    }

    void setCursor(uint8_t x, uint8_t y)
    {
        // x is pixel column (0..83), y is bank/row (0..5)
        writeByte(Pcd8544::SetXAddress | (x & 0x7F));
        writeByte(Pcd8544::SetYAddress | (y & 0x07));
    }

    void clear()
    {
        setCursor(0, 0);
        uint8_t zero = 0x00;
        for (uint16_t i = 0; i < 84 * 6; ++i) {
            writeByte(zero);
        }
        setCursor(0, 0);
    }

    void writeChar(char c)
    {
        if (c < 0x20 || c > 0x7F) {
            c = '?';
        }

        // 5 columns of the glyph
        for (uint8_t col = 0; col < 5; ++col) {
            writeByte(Fonts::Classic5x7[c - 0x20][col]);
        }
        // 1-pixel spacing
        writeByte(0x00);
    }
};

}
