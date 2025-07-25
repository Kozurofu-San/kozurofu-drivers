#pragma once

#include "interface/LedStrip.h"

#include <cstdint>
#include <cstddef>

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

    void init()
    {
        ESP_LOGI(TAG, "Create RMT TX channel");
        rmt_channel_handle_t led_chan = NULL;
        rmt_tx_channel_config_t tx_chan_config = {
            .gpio_num = _pin,
            .clk_src = RMT_CLK_SRC_DEFAULT, // select source clock
            .resolution_hz = 10000000,
            .mem_block_symbols = 64, // increase the block size can make the LED less flickering
            .trans_queue_depth = 4, // set the number of transactions that can be pending in the background
        };
        ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &led_chan));

        ESP_LOGI(TAG, "Install led strip encoder");
        rmt_encoder_handle_t led_encoder = NULL;

    }

    gpio_num_t getPin()
    {
        return _pin;
    }

    void setColor(uint8_t *array, size_t len) override
    {
        
    }
    void setBacklight(size_t value) override
    {
        
    }

    private:

    gpio_num_t _pin;
};

}