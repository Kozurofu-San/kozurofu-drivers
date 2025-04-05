#pragma once

#include "interface/Logs.h"

#include "esp_log.h"

class LogsDriver : public ILogs
{
    public:


    LogsDriver()
    {
    }

    void init()
    {
        // esp_log_level_set("*", ESP_LOG_MAX);        // set all components to ERROR level
    }

    void LOGI(const char* message, ...) override
    {
        va_list args;
        va_start(args, message);
        printf("I:");
        vsprintf(_buffer, message, args);
        printf("%s\n", _buffer);
        va_end(args);
    }

    void LOGW(const char* message, ...) override
    {
        va_list args;
        va_start(args, message);
        printf("W:");
        vsprintf(_buffer, message, args);
        printf("%s\n", _buffer);
        va_end(args);
    }

    void LOGE(const char* message, ...) override
    {
        va_list args;
        va_start(args, message);
        printf("E:");
        vsprintf(_buffer, message, args);
        printf("%s\n", _buffer);
        va_end(args);
    }

    void LOGV(uint32_t channel, int32_t value) override
    {
        sprintf(_buffer, "V%ld: %ld", channel, value);
        printf("%s\n", _buffer);
    }

    private:

    char _buffer[20];

};