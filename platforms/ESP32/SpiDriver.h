#pragma once

#include "interface/Spi.h"

#include "driver/spi_master.h"

namespace driver
{

class SpiDriver : public Spi
{
    public:

    SpiDriver(spi_host_device_t spi, int clk, int miso, int mosi, int cs, int speed)
    {
        spi_bus_config_t buscfg {
            .mosi_io_num = mosi,
            .miso_io_num = miso,
            .sclk_io_num = clk,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = 8
        };
        spi_device_interface_config_t devcfg {
            .command_bits = 0,
            .address_bits = 0,
            .mode = 0,                   //SPI mode 0
            .clock_speed_hz = speed,     //Clock out at 10 MHz
            .spics_io_num = cs,          //CS pin
            .queue_size = 7              //We want to be able to queue 7 transactions at a time
        };
        spi_bus_initialize(spi, &buscfg, SPI_DMA_CH_AUTO);
        spi_bus_add_device(spi, &devcfg, &_spi);
    }

    void init() override
    {
    };

    void write(uint8_t data) override
    {
        spi_transaction_t t = {
            .flags = SPI_TRANS_USE_TXDATA,
            .length = 8,
            .tx_data = {data}
        };
        spi_device_acquire_bus(_spi, portMAX_DELAY);
        spi_device_transmit(_spi, &t);
        spi_device_release_bus(_spi);
    };

    uint8_t read() override
    {
        spi_transaction_t t = {
            .flags = SPI_TRANS_USE_RXDATA,
            .rxlength = 8
        };
        spi_device_polling_transmit(_spi, &t);
        return t.rx_data[0];
    };
    
    private:

    spi_device_handle_t _spi;

};

}