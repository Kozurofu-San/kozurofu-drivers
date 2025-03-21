#pragma once

#include "interface/Spi.h"

#include "driver/spi_master.h"

namespace driver
{

class SpiDriver : public Spi
{
    public:

    SpiDriver(spi_host_device_t spi, size_t clk, size_t miso, size_t mosi, size_t cs, size_t speed)
    {
        spi_device_handle_t spiHandle;
        spi_bus_config_t buscfg {
            .miso_io_num = miso,
            .mosi_io_num = mosi,
            .sclk_io_num = clk,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = 8
        };
        spi_device_interface_config_t devcfg {
            .clock_speed_hz = speed,     //Clock out at 10 MHz
            .mode = 0,                   //SPI mode 0
            .spics_io_num = cs,          //CS pin
            .queue_size = 7,             //We want to be able to queue 7 transactions at a time
        };
        spi_bus_initialize(spi, &buscfg, SPI_DMA_CH_AUTO);
        spi_bus_add_device(spi, &devcfg, &spiHandle);
    }

    void init() override
    {
    };

    void write(uint8_t data) override
    {
    };

    uint8_t read() override
    {
        return 0;
    };
    
    private:

};

}