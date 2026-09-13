#pragma once

#include "Nrf24Const.h"

#include "interface/Spi.h"
#include "interface/Serial.h"
#include "interface/Gpio.h"
#include "interface/Timer.h"

#include <cstdint>

namespace driver
{

class Nrf24Driver: public ISerial
{
    public:

    Nrf24Driver(ISpi &p, IGpio &ce, IGpio &irq, ITimer &timer)
        : _p(p), _ce(ce), _irq(irq), _timer(timer)
    {
    }
    ~Nrf24Driver() = default;

    bool init()
    {
        // Init check
        if (!_p.isInit() or !_timer.isInit())
        {
            return false;
        }

        // Speed check
        if (_p.getSpeed() > MaxSpeed or _p.getSpeed() == 0)
        {
            return false;
        }

        _ce.write(0);

        // Check if SPI works
        uint8_t status = readReg(Nrf24::STATUS);
        if (status = 0x0E)
        {
            _isInit = true;
        }

        return _isInit;
    }

    bool write(uint8_t *data, size_t len) override
    {
        if (!_isInit or data == nullptr or len == 0 or len > MaxPayloadSize)
        {
            return false;
        }
 
        // Make sure we're in standby before touching the FIFO
        _ce.write(0);
 
        // Drop any stale TX payload and clear flags left over from the previous send
        write(Nrf24::CmdFlushTx, nullptr, 0);
        writeReg(Nrf24::STATUS, Nrf24::Status::TX_DS | Nrf24::Status::MAX_RT | Nrf24::Status::RX_DR);
 
        // Load the new payload into the TX FIFO
        write(Nrf24::CmdWriteTxPayload, data, len);
 
        // Pulse CE high for >10us to kick off the transmission, per datasheet timing
        _ce.write(1);
        _timer.delay(15);
        _ce.write(0);
 
        // Poll until the transfer either succeeds (TX_DS) or exhausts its retries (MAX_RT)
        uint8_t status = 0;
        do
        {
            status = readReg(Nrf24::STATUS);
        } while ((status & (Nrf24::Status::TX_DS | Nrf24::Status::MAX_RT)) == 0);
 
        // Clear whichever flag(s) fired
        writeReg(Nrf24::STATUS, status & (Nrf24::Status::TX_DS | Nrf24::Status::MAX_RT));
 
        if (status & Nrf24::Status::MAX_RT)
        {
            // Retries exhausted: drop the unsent payload so it doesn't jam the FIFO
            write(Nrf24::CmdFlushTx, nullptr, 0);
            return false;
        }
 
        return (status & Nrf24::Status::TX_DS) != 0;
    }

    bool read (uint8_t *data, size_t len) override
    {
        
        if (!_isInit or data == nullptr or len == 0 or len > MaxPayloadSize)
        {
            return false;
        }
 
        // Nothing to do if no payload is waiting
        uint8_t status = readReg(Nrf24::STATUS);
        if ((status & Nrf24::Status::RX_DR) == 0)
        {
            return false;
        }
 
        // Pull the payload out of the RX FIFO
        read(Nrf24::CmdReadRxPayload, data, len);
 
        // Ack the RX_DR flag now that we've consumed the payload
        writeReg(Nrf24::STATUS, Nrf24::Status::RX_DR);
 
        return false;
    }

    void setCallback(void (*cb)(uint32_t)) override
    {
        _cb = cb;
    }

    void setBuffer(uint8_t *buffer, size_t size) override
    {
        _buffer = buffer;
        _bufferSize = size;
    }

    // Callback when IRQ is set
    void interrupt()
    {
        _cb(0);
    }
    
    uint32_t getSpeed() const override
    {
        return _speed;
    }

    bool isInit()// override
    {
        return _isInit;
    }

    private:

    ISpi &_p;
    IGpio &_ce;
    IGpio &_irq;
    ITimer &_timer;

    void (*_cb)(uint32_t) = nullptr;
    uint8_t *_buffer = nullptr;
    size_t _bufferSize = 0;

    static constexpr uint32_t MaxSpeed = 8'000'000;     // Hz
    static constexpr uint8_t MaxPayloadSize = 32;       // Static payload width, bytes

    uint32_t _speed; // Speed in Hz
    bool _isInit = false;

    uint8_t read(uint8_t reg, uint8_t *data, size_t len)
    {
        _p.enable();
        uint8_t status = _p.transfer(reg);
        _p.read(data, len);
        _p.disable();
        return status;
    }

    uint8_t write(uint8_t reg, uint8_t *data, size_t len)
    {
        _p.enable();
        uint8_t status = _p.transfer(reg);
        _p.write(data, len);
        _p.disable();
        return status;
    }

    uint8_t readReg(uint8_t reg)
    {
        _p.enable();
        uint8_t status = _p.transfer(Nrf24::CmdReadRegister | reg);
        uint8_t ret = _p.transfer(0);
        _p.disable();
        return ret;
    }

    uint8_t writeReg(uint8_t reg, uint8_t data)
    {
        _p.enable();
        uint8_t status = _p.transfer(Nrf24::CmdWriteRegister | reg);
        _p.transfer(data);
        _p.disable();
        return status;
    }

};

}
