#include "u_main.h"
#include "u_cmds.h"

namespace Util
{
    namespace Commands
    {
        cmd_t cmdTable;

        void addCommand(const char *cmd, cmdFunction func)
        {
            cmdTable[std::string(cmd)] = func;
        }
    }
}

extern "C"
{
    qboolean Util_ExecuteCommand(const char *cmd, gentity_t *ent)
    {
        std::string cmdName(cmd);
        if (Util::Commands::cmdTable.find(cmdName) != Util::Commands::cmdTable.end())
        {
            Util::Commands::cmdTable.at(cmdName)(ent);
            return qtrue;
        }
        else
        {
            return qfalse;
        }
    }
}