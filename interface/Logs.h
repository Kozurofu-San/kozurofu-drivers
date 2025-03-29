#pragma once

#include <cstdint>

class Logs
{
    public:

    enum class Turn: bool
    {
        On = true,
        Off = false
    };

    virtual ~Logs() = default;
    virtual void LOGI(const char* message, ...) = 0;
    virtual void LOGW(const char* message, ...) = 0;
    virtual void LOGE(const char* message, ...) = 0;
    virtual void LOGV(uint32_t channel, int32_t value) = 0;
};