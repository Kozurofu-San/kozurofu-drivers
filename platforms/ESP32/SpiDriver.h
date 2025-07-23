#pragma once
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

#include "interface/Spi.h"

#include "driver/spi_master.h"

namespace driver
{

class SpiDevice
{
    public:

    SpiDevice(spi_host_device_t spi)
        : _spiDevice(spi)
    {
    }

    void init(int clk, int miso, int mosi, int cs, int speed)
    {
        spi_bus_config_t buscfg {
            .mosi_io_num = mosi,
            .miso_io_num = miso,
            .sclk_io_num = clk,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = 8,
        };

        spi_device_interface_config_t devcfg {
            .command_bits = 0,
            .address_bits = 0,
            .mode = 0,
            .clock_speed_hz = speed,
            .spics_io_num = cs,
            .queue_size = 7,
        };

        spi_bus_initialize(_spiDevice, &buscfg, SPI_DMA_CH_AUTO);
        spi_bus_add_device(_spiDevice, &devcfg, _spi);
    };

    spi_device_handle_t* getSpi()
    {
        return _spi;
    }

    private:

    spi_device_handle_t* _spi;
    spi_host_device_t _spiDevice;
};
    
class SpiDriver : public ISpi
{
    public:

    SpiDriver(SpiDevice &spi)
        : _spi(spi.getSpi())
    {}

    void write(uint8_t *data, size_t len) override
    {
        spi_transaction_t t = {
            .flags = SPI_TRANS_USE_TXDATA,
            .length = 8 * len,
            .tx_data = *data
        };
        spi_device_acquire_bus(*_spi, portMAX_DELAY);
        spi_device_polling_transmit(*_spi, &t);
        spi_device_release_bus(*_spi);
    };

    void read(uint8_t *data, size_t len) override
    {
        spi_transaction_t t = {
            .flags = SPI_TRANS_USE_RXDATA,
            .rxlength = 8 * len,
            .rx_data = *data
        };
        spi_device_polling_transmit(*_spi, &t);
    };

    void sendCommand(uint32_t cmd)
    {
        // Not implemented
    }
    inline void sendData(uint32_t data)
    {
        // Not implemented
    }
    inline uint32_t readData()
    {
        return 0;   // Not implemented
    }

    private:

    spi_device_handle_t* _spi;
    void* _callback = nullptr;
};

}