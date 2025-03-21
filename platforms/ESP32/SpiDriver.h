#pragma once

#include "interface/Spi.h"

#include "driver/spi_master.h"

namespace driver
{

class SpiDriver : public Spi
{
    public:

    SpiDriver()
    {
        
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