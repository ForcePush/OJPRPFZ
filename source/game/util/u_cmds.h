#ifndef U_CMDS_H
#define U_CMDS_H
#pragma once

#include <map>
#include <string>

#include "u_main.h"

namespace Util
{
    namespace Commands
    {
        typedef void(*cmdFunction)(gentity_t*);

        typedef std::map<std::string, cmdFunction> cmd_t;
        extern cmd_t cmdTable;

        void addCommand(const char *cmd, cmdFunction func);
    }
}

#endif  // U_MAIN_H