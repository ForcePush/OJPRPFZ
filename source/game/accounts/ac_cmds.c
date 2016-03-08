#include "ac_public.h"
#include "ac_cmds.h"

ac_cmd_t ac_cmdTable[MAX_CMDS];
int ac_cmdsAmount;


// commands section
void AC_Test(gentity_t *ent)
{
    trap_SendServerCommand(ent->playerState->clientNum, "print \"^2/ac_test!\n\"");
}

void AC_Register(gentity_t *ent)
{
    trap_SendServerCommand(ent->playerState->clientNum, "print \"^2Coming soon!\n\"");
}

void AC_RemoveAccount(gentity_t *ent)
{
    trap_SendServerCommand(ent->playerState->clientNum, "print \"^2Coming soon!\n\"");
}

void AC_Login(gentity_t *ent)
{
    trap_SendServerCommand(ent->playerState->clientNum, "print \"^2Coming soon!\n\"");
}

void AC_Logout(gentity_t *ent)
{
    trap_SendServerCommand(ent->playerState->clientNum, "print \"^2Coming soon!\n\"");
}
// end commands section


// Skinpack: TODO: AC_RemoveCommand
void AC_AddCommand(const char *cmd, ac_cmdFunction func)
{
    if (ac_cmdsAmount >= MAX_CMDS)
    {
        G_Printf("^1AC_Addcommand: MAX_CMDS reached!\n");
        return;
    }
    
    if (strlen(cmd) > MAX_CMD_NAME_LEN)
    {
        G_Printf("^3AC_Addcommand: too big cmd name.\n");
        return;
    }
    
    for (int i = 0; i < ac_cmdsAmount; i++)
    {
        if (Q_stricmp(cmd, ac_cmdTable[i].name) == 0)
        {
            G_Printf("^3AC_Addcommand: \"%s\" is already defined.\n", cmd);
            return;
        }
    }
    
    strcpy(ac_cmdTable[ac_cmdsAmount].name, cmd);
    ac_cmdTable[ac_cmdsAmount].func = func;
    ac_cmdsAmount++;
}

qboolean AC_ExecuteCommand(const char *cmd, gentity_t *ent)
{
    for (int i = 0; i < ac_cmdsAmount; i++)
    {
        if (stricmp(ac_cmdTable[i].name, cmd) == 0)
        {
            ac_cmdTable[i].func(ent);
            return qtrue;
        }
    }
    
    return qfalse;
}

void AC_InitCommands()
{
    AC_AddCommand("ac_test", AC_Test);
    AC_AddCommand("ac_register", AC_Register);
    AC_AddCommand("ac_removeAccount", AC_RemoveAccount);
    AC_AddCommand("ac_login", AC_Login);
    AC_AddCommand("ac_logout", AC_Logout);
}
