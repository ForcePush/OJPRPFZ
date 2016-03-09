#ifndef AC_CMDS_H
#define AC_CMDS_H
#pragma once

#include "ac_public.h"

#define AC_MAX_CMD_NAME_LEN 128
#define AC_MAX_CMDS 128

typedef struct
{
    char name[AC_MAX_CMD_NAME_LEN];
    ac_cmdFunction func;
} ac_cmd_t;

extern ac_cmd_t ac_cmdTable[AC_MAX_CMDS];
extern int ac_cmdsAmount;

void AC_AddCommand(const char *cmd, ac_cmdFunction func);
void AC_InitCommands();

#endif  // AC_CMDS_H
