#pragma once

#include "interface/Communication.h"

#include "asf.h"
#include "component/component_spi.h"
#include "spi/spi.h"

namespace driver
{

class SpiController: public ICommunication
{
    public:

    enum class Mode: uint32_t
    {
        Master = 0x1,
        Slave = 0x0
    };

    enum class ClockPolarity: uint32_t
    {
        IdleLow = 0x0,
        IdleHigh = 0x1
    };

    enum class ClockPhase: uint32_t
    {
        FirstEdge = 0x0,
        SecondEdge = 0x2
    };

    enum class DataSize: uint32_t
    {
        Bits8 = 0x0,
        Bits16 = 0x80
    };

    SpiController(Spi *spi)
        : _spi(spi)
    {
    }

    bool init(Mode mode, ClockPolarity clockPolarity, ClockPhase clockPhase, DataSize dataSize, uint32_t speed)
    {
        spi_enable_clock(_spi);
        spi_disable(_spi);
        spi_reset(_spi);
        spi_set_lastxfer(_spi);
        spi_set_master_mode(_spi);
        spi_disable_mode_fault_detect(_spi);
        spi_set_peripheral_chip_select_value(_spi, 0);
        spi_set_clock_polarity(_spi, 0, 0);
        spi_set_clock_phase(_spi, 0, 0);
        spi_set_bits_per_transfer(_spi, 0, SPI_CSR_BITS_8_BIT);
        spi_set_baudrate_div(_spi, 0, (sysclk_get_peripheral_hz() / speed));
        spi_set_transfer_delay(_spi, 0, 0x40, 0x10);
        spi_enable(_spi);
        return true;
    };
    
    void write(uint8_t *data, size_t len, size_t bytes = 1) override
    {
        for (size_t i = 0; i < len; ++i)
        {
            spi_write(_spi, data[i], 0, 0);
        }
    };

    void read(uint8_t *data, size_t len, size_t bytes = 1) override
    {
        static uint16_t ret;
        for (size_t i = 0; i < len; ++i)
        {
            spi_write(_spi, 0, 0, 0);
            /* Wait transfer done. */
            while ((spi_read_status(_spi) & SPI_SR_RDRF) == 0);
            spi_read(_spi, &ret, 0);
            data[i] = ret;
        }
    };
    
    
    uint32_t sendCommand(uint32_t cmd) override
    {
        return 0;
    }

    Spi* getSpi()
    {
        return _spi;
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

    Spi* _spi;
    uint32_t _speed;

    bool _isInit = false;
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
        : _spi(spi)
    {
    }

    bool init(GpioDriver *cs, IdleState idleState)
    {
        _cs = cs;
        _cs->write(!_idleState);
        _idleState = static_cast<bool>(idleState);
        return _spi.isInit();
    }
    
    void write(uint8_t *data, size_t len, size_t bytes = 1) override
    {
        _spi.write(data, len);
    };

    void read(uint8_t *data, size_t len, size_t bytes = 1) override
    {
        _spi.read(data, len);
    };

    inline uint32_t sendCommand(uint32_t cmd) override
    {
        return _spi.sendCommand(cmd);
    }
    
    inline void enable() override
    {
        if (_cs)
        {
            _cs->write(!_idleState);
        }
    }

    inline void disable() override
    {
        if (_cs)
        {
            _cs->write(_idleState);
        }
    }

    inline uint32_t getSpeed() const override
    {
        return _spi.getSpeed();
    }

    inline bool isInit() override
    {
        return _spi.isInit();
    }
    private:

    SpiController &_spi;
    
    GpioDriver *_cs;
    bool _idleState; // CS state when idle, true - high, false - low
};

}