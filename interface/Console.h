#pragma once

#include <cstdint>

namespace driver
{

class  IConsole
{
    public:

    struct Command
    {
        const char* cmd;
        const char* help;
        const char* hint;
        int (*func)(int argc, char **argv);
    };

    virtual ~ IConsole() = default;
    virtual bool cmdAdd(Command& cmd) = 0;
    virtual bool cmdRemove(const char* name) = 0;
};

}