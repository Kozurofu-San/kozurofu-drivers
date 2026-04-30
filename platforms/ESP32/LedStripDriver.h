#pragma once

#include "interface/LedStrip.h"

#include <cstdint>
#include <cstddef>
#include <algorithm>

#include "driver/rmt_tx.h"

namespace driver
{

class LedStripDriver : public ILedStrip
{
public:

    LedStripDriver(uint32_t pin)
        : _pin(static_cast<gpio_num_t>(pin))
    {
    }

    ~LedStripDriver()
    {
        if (_rmt_chan && _isInit) rmt_disable(_rmt_chan);
        if (_encoder) rmt_del_encoder(_encoder);
        if (_rmt_chan) rmt_del_channel(_rmt_chan);
    }

    bool init(size_t stripLen)
    {
        _stripLen = stripLen;

        // 1. Create RMT TX channel
        rmt_tx_channel_config_t tx_chan_config = {
            .gpio_num = _pin,
            .clk_src = RMT_CLK_SRC_DEFAULT,
            .resolution_hz = 10'000'000,        // 10 MHz → 0.1us/tick
            .mem_block_symbols = 64,            // It's able to increase up to 128-256 for long strips
            .trans_queue_depth = 4,
            .intr_priority = 0,
            .flags = { .invert_out = false, .with_dma = false }
        };

        ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &_rmt_chan));
        ESP_ERROR_CHECK(rmt_enable(_rmt_chan));

        // 2. Setup bytes encoder for SK6812
        rmt_bytes_encoder_config_t bytes_encoder_config = {
            .bit0 = {
                .duration0 = 3,     // T0H = 0.3us
                .level0 = 1,
                .duration1 = 9,     // T0L = 0.9us
                .level1 = 0
            },
            .bit1 = {
                .duration0 = 6,     // T1H = 0.6us
                .level0 = 1,
                .duration1 = 6,     // T1L = 0.6us
                .level1 = 0
            },
            .flags = {
                .msb_first = true   // SK6812/WS2812 send MSB first
            }
        };

        ESP_ERROR_CHECK(rmt_new_bytes_encoder(&bytes_encoder_config, &_encoder));

        _isInit = true;
        return _isInit;
    }

    gpio_num_t getPin() const
    {
        return _pin;
    }

    void setColor(uint8_t *array, size_t len) override
    {
        if (!_isInit || len == 0) return;

        // GRB array (SK6812)

        rmt_transmit_config_t tx_config = {
            .loop_count = 0,
            .flags = { .eot_level = 0 }   // Low level after transmission
        };

        // Send data
        ESP_ERROR_CHECK(rmt_transmit(_rmt_chan, _encoder, array, len, &tx_config));

        // Wait for transmission ending
        ESP_ERROR_CHECK(rmt_tx_wait_all_done(_rmt_chan, 100));

        // WS2812/SK6812 latch data when the line stays low for at least 80 us.
        esp_rom_delay_us(100);
    }

    void setBacklight(size_t brightness) override
    {
        _brightness = brightness > 255 ? 255 : brightness;
    }

    inline bool isInit() override
    {
        return _isInit;
    }

private:

    uint8_t applyBrightness(uint8_t value)
    {
        return (value * _brightness) / 255;
    }

private:

    gpio_num_t _pin;
    size_t _stripLen = 0;
    
    uint8_t _brightness = 255;
    rmt_channel_handle_t _rmt_chan = nullptr;
    rmt_encoder_handle_t _encoder = nullptr;      // bytes encoder

    bool _isInit = false;
};

}
