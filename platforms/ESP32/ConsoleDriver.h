#pragma once

#include "interface/Console.h"

#include "esp_console.h"
#include "esp_log.h"

namespace driver
{

class ConsoleDriver : public IConsole
{
    public:
    esp_console_repl_t *s_repl = nullptr;

    ConsoleDriver()
    {
    }

    bool init()
    {
        if (s_repl != nullptr)
        {
            return true;
        }

        esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
        esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
        esp_err_t ret = esp_console_new_repl_uart(&uart_config, &repl_config, &s_repl);
        if (ret != ESP_OK)
        {
            ESP_LOGE("ConsoleDriver", "UART REPL init failed: %s", esp_err_to_name(ret));
            s_repl = nullptr;
            return false;
        }
        return true;
    }

    bool start() override
    {
        if (s_repl == nullptr && !init())
        {
            return false;
        }

        esp_err_t ret = esp_console_start_repl(s_repl);
        if (ret != ESP_OK)
        {
            ESP_LOGE("ConsoleDriver", "REPL start failed: %s", esp_err_to_name(ret));
            return false;
        }
        return true;
    }

    bool cmdAdd(Command& command) override
    {
        esp_console_cmd_t cmd = { };
        cmd.command = command.cmd;
        cmd.help    = command.help;
        cmd.hint    = command.hint;
        cmd.func    = command.func;
        esp_err_t ret = esp_console_cmd_register(&cmd);
        return ret == ESP_OK;
    }

    bool cmdRemove(const char* name) override
    {
        
        return true;
    }

    private:

    char _buffer[20];

};

}
