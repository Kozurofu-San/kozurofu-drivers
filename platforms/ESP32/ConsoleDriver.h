#pragma once
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

#include "interface/Console.h"

#include "esp_console.h"
// #include "esp_log.h"
// #include "esp_vfs_dev.h"
// #include "driver/uart.h"
// #include "driver/uart_vfs.h"
// #include "driver/usb_serial_jtag.h"
// #include "driver/usb_serial_jtag_vfs.h"

namespace driver
{

class ConsoleDriver : public IConsole
{
    public:


    ConsoleDriver()
    {
    }

    bool init()
    {
        
        return true;
    }

    bool cmdAdd(Command& command) override
    {
        esp_console_cmd_t cmd =
        {
            .command = command.cmd,
            .help    = command.help,
            .hint    = command.hint,
            .func    = command.func
        };
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