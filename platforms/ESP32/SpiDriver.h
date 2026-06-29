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
            spi_bus_config_t buscfg {};
            buscfg.mosi_io_num = mosi;
            buscfg.miso_io_num = miso;
            buscfg.sclk_io_num = clk;
            buscfg.quadwp_io_num = -1;
            buscfg.quadhd_io_num = -1;
            buscfg.data4_io_num = -1;
            buscfg.data5_io_num = -1;
            buscfg.data6_io_num = -1;
            buscfg.data7_io_num = -1;
            buscfg.data_io_default_level = false;
            buscfg.max_transfer_sz = 8;
            buscfg.flags = SPICOMMON_BUSFLAG_MASTER;
    
            spi_device_interface_config_t devcfg {};
            devcfg.command_bits = 0;
            devcfg.address_bits = 0;
            devcfg.mode = 0;
            devcfg.clock_speed_hz = speed;
            devcfg.spics_io_num = cs;
            devcfg.queue_size = 7;
    
            esp_err_t err = spi_bus_initialize(_spi, &buscfg, SPI_DMA_CH_AUTO);
            if (err != ESP_OK)
            {
                ESP_LOGE(kSpiTag, "bus init failed: host=%d clk=%d miso=%d mosi=%d cs=%d err=%s",
                         static_cast<int>(_spi), clk, miso, mosi, cs, esp_err_to_name(err));
                return false;
            }

            err = spi_bus_add_device(_spi, &devcfg, &_spiDevice);
            if (err != ESP_OK)
            {
                ESP_LOGE(kSpiTag, "device add failed: host=%d cs=%d speed=%d err=%s",
                         static_cast<int>(_spi), cs, speed, esp_err_to_name(err));
                spi_bus_free(_spi);
                return false;
            }
    
            _isInit = true;
            _speed = speed;
            return _isInit;
        };
    
        bool setDma()
        {
            return false;
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
