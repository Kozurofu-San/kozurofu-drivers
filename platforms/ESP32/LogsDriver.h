#pragma once
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

#include "interface/Logs.h"

#include "esp_log.h"
#include "esp_vfs_dev.h"
#include "driver/uart.h"
#include "driver/uart_vfs.h"

namespace driver
{

class LogsDriver : public ILogs
{
    public:


    LogsDriver()
    {
    }

    bool init()
    {
        // esp_log_level_set("*", ESP_LOG_MAX);        // set all components to ERROR level
        
        uart_config_t uart_config = {
            .baud_rate = 115200,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .rx_flow_ctrl_thresh = 0
        };
    
        // For blocking scanf
        uart_param_config(UART_NUM_0, &uart_config);
        uart_driver_install(UART_NUM_0, 2048, 0, 0, NULL, 0);
        uart_vfs_dev_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);
    
        return true;
    }

    void i(const char* message, ...) override
    {
        va_list args;
        va_start(args, message);
        printf("I:");
        vsprintf(_buffer, message, args);
        printf("%s\n", _buffer);
        va_end(args);
    }

    void w(const char* message, ...) override
    {
        va_list args;
        va_start(args, message);
        printf("W:");
        vsprintf(_buffer, message, args);
        printf("%s\n", _buffer);
        va_end(args);
    }

    void e(const char* message, ...) override
    {
        va_list args;
        va_start(args, message);
        printf("E:");
        vsprintf(_buffer, message, args);
        printf("%s\n", _buffer);
        va_end(args);
    }

    void v(uint32_t channel, int32_t value) override
    {
        sprintf(_buffer, "V%ld: %ld", channel, value);
        printf("%s\n", _buffer);
    }
    
    bool readString(char* string)
    {
        int ret = scanf("%s", string);
        if (ret == 1)
        {
            i(">>> %s", string);
            return true;
        }
        else
        {
            e("Input error");
            return false;
        }
    }
    
    bool readNumber(int number)
    {
        int ret = scanf("%d", &number);
        if (ret == 1)
        {
            i(">>> %d", number);
            return true;
        }
        else
        {
            e("Input error");
            return false;
        }
    }

    private:

    char _buffer[20];

};

}