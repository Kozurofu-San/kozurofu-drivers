#pragma once
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

#include "interface/Communication.h"

#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"

namespace driver
{
    inline constexpr const char* kSpiTag = "SpiController";
    
    class SpiController : public ICommunication
    {
        public:
    
        SpiController(spi_host_device_t spi)
            : _spi(spi)
        {
        }
        
        bool init(int clk, int miso, int mosi, int cs, int speed)
        {
            spi_bus_config_t buscfg {
                .iocfg = {
                    mosi,      // [0] MOSI / data0
                    miso,      // [1] MISO / data1
                    clk,       // [2] SCLK
                    -1,        // [3] WP / data2
                    -1,        // [4] HD / data3
                    -1,        // [5] data4
                    -1,        // [6] data5
                    -1,        // [7] data6
                    -1         // [8] data7
                },
                .data_io_default_level = false,
                .max_transfer_sz = 8,
                .flags = SPICOMMON_BUSFLAG_MASTER
            };
    
            spi_device_interface_config_t devcfg {
                .command_bits = 0,
                .address_bits = 0,
                .mode = 0,
                .clock_speed_hz = speed,
                .spics_io_num = cs,
                .queue_size = 7,
            };
    
            ESP_ERROR_CHECK(spi_bus_initialize(_spi, &buscfg, SPI_DMA_CH_AUTO));
            ESP_ERROR_CHECK(spi_bus_add_device(_spi, &devcfg, &_spiDevice));
    
            _isInit = true;
            _speed = speed;
            return true;
        };
    
        bool setDma()
        {
            return true;
        }
    
        void write(uint8_t *data, size_t len, size_t bytes = 1) override
        {
            if (_spiDevice == nullptr || data == nullptr || len == 0 || len > 4)
            {
                ESP_LOGE(kSpiTag, "write rejected: init=%d dev=%p data=%p len=%u", _isInit, (void*)_spiDevice, (void*)data, static_cast<unsigned>(len));
                return;
            }
            spi_transaction_t t = {
                .flags = SPI_TRANS_USE_TXDATA,
                .length = 8 * len,
                .tx_data = *data
            };
            // ESP_ERROR_CHECK(spi_device_acquire_bus(_spiDevice, portMAX_DELAY));
            esp_err_t err = spi_device_polling_transmit(_spiDevice, &t);
            if (err != ESP_OK)
            {
                ESP_LOGE(kSpiTag, "write failed: err=%d dev=%p", static_cast<int>(err), (void*)_spiDevice);
            }
            // spi_device_release_bus(_spiDevice);
        };
    
        void read(uint8_t *data, size_t len, size_t bytes = 1) override
        {
            if (_spiDevice == nullptr || data == nullptr || len == 0 || len > 4)
            {
                ESP_LOGE(kSpiTag, "read rejected: init=%d dev=%p data=%p len=%u", _isInit, (void*)_spiDevice, (void*)data, static_cast<unsigned>(len));
                return;
            }
            spi_transaction_t t = {
                .flags = SPI_TRANS_USE_RXDATA,
                .rxlength = 8 * len,
                .rx_data = *data
            };
            esp_err_t err = spi_device_polling_transmit(_spiDevice, &t);
            if (err != ESP_OK)
            {
                ESP_LOGE(kSpiTag, "read failed: err=%d dev=%p", static_cast<int>(err), (void*)_spiDevice);
            }
        };
    
        uint32_t sendCommand(uint32_t cmd) override
        {
            uint8_t ret;
            write(&ret, 1);
            return ret;
        }
    
        spi_host_device_t getSpi()
        {
            return _spi;
        }

        spi_device_handle_t getDeviceHandle() const
        {
            return _spiDevice;
        }
    
        uint32_t getSpeed() const override
        {
            return _speed;
        }
    
        void enable() override
        {
        }
    
        void disable() override
        {
        }
    
        bool isInit() override
        {
            return _isInit;
        }
    
        private:
    
        spi_host_device_t   _spi;
        inline static spi_device_handle_t _spiDevice {};
        inline static uint32_t _speed = 0;

        inline static bool _isInit = false;
    };

class SpiDriver : public ICommunication
{
    public:
    
    enum class IdleState : bool
    {
        Low = false,
        High = true
    };

    SpiDriver(SpiController &spi)
    {
        _spi = &spi;
    }
    
    bool init(GpioDriver *cs, IdleState idleState)
    {
        _cs = cs;
        _cs->write(!_idleState);
        _idleState = static_cast<bool>(idleState);
        return (_spi != nullptr) ? _spi->isInit() : false;
    }

    void write(uint8_t *data, size_t len, size_t bytes = 1) override
    {
        if (_spi == nullptr)
        {
            ESP_LOGE(kSpiTag, "SpiDriver::write rejected: spi controller is null");
            return;
        }
        _spi->SpiController::write(data, len);
    };

    void read(uint8_t *data, size_t len, size_t bytes = 1) override
    {
        if (_spi == nullptr)
        {
            ESP_LOGE(kSpiTag, "SpiDriver::read rejected: spi controller is null");
            return;
        }
        _spi->SpiController::read(data, len);
    };

    inline uint32_t sendCommand(uint32_t cmd) override
    {
        return (_spi != nullptr) ? _spi->SpiController::sendCommand(cmd) : 0;
    }
    inline void enable() override
    {
        // Not implemented
    }
    inline void disable() override
    {
        // Not implemented
    }
    inline uint32_t getSpeed() const override
    {
        return (_spi != nullptr) ? _spi->SpiController::getSpeed() : 0;
    }
    inline bool isInit() override
    {
        return (_spi != nullptr) ? _spi->SpiController::isInit() : false;
    }

    private:

    inline static SpiController *_spi = nullptr;
    void* _callback = nullptr;

    GpioDriver *_cs;
    bool _idleState; // CS state when idle, true - high, false - low
};

}