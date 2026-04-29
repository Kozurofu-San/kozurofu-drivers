#pragma once

#include "interface/VoltageSet.h"

#include <cstdint>
#include <cstddef>
#include <cassert>

#include "asf.h"
#include "component/component_dacc.h"
#include "dacc/dacc.h"

namespace driver
{

class DacDriver : public IVoltageSet
{
    public:

    enum class Trigger : uint8_t
    {
        Timer6Trgo,
        Timer8Trgo,
        Timer7Trgo,
        Timer5Trgo,
        Timer2Trgo,
        Timer4Trgo,
        Exti9,
        SwStart,
        None,
    };

    struct ChannelConfig
    {
        uint8_t channel;
    };

    DacDriver(Dacc *dac, ChannelConfig *channels, size_t channelCount)
        : _dac(dac), _channels(channels), _channelCount(channelCount)
    {
        assert(channelCount <= 2);
        for (size_t i = 0; i < channelCount; ++i)
        {
            assert(channels[i].channel <= 2);
        }
    }

    bool init(Trigger trigger)
    {
        // Clock
        uint32_t rccId = 0;
        if      (_dac == DACC) rccId = ID_DACC;
        sysclk_enable_peripheral_clock(rccId);

        // DAC
        dacc_reset(_dac);
        dacc_set_transfer_mode(_dac, 0);
        dacc_set_power_save(_dac, 0, 0);
        dacc_set_timing(_dac, 0x08, 0, 0x10);
        dacc_set_channel_selection(_dac, 0); // TODO
        dacc_enable_channel(_dac, 0); // TODO
        dacc_set_analog_control(_dac, DACC_ACR_IBCTLCH0(0x02) | DACC_ACR_IBCTLCH1(0x02) | DACC_ACR_IBCTLDACCORE(0x01));

        _isInit = true;
        return true;
    }

    inline void start() override
    {
    }

    void setVoltage(uint32_t voltage, size_t channel) override
    {
        uint32_t value = static_cast<uint32_t>((voltage * 4095) / 3000);
        if (value > 4095) value = 4095;
        if ((dacc_get_interrupt_status(_dac) & DACC_ISR_TXRDY) == DACC_ISR_TXRDY)
        {
            dacc_write_conversion_data(_dac, value);
        }
    }

    void setRawValue(uint32_t value, size_t channel) override
    {
        if ((dacc_get_interrupt_status(_dac) & DACC_ISR_TXRDY) == DACC_ISR_TXRDY)
        {
            dacc_write_conversion_data(_dac, value);
        }
    }

    bool isInit() override
    {
        return _isInit;
    }

    private:

    Dacc *_dac;
    ChannelConfig *_channels;
    size_t _channelCount;
    
    bool _isInit = false;
};

} // namespace driver