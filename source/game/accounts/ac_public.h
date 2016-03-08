#ifndef AC_PUBLIC_H
#define AC_PUBLIC_H
#pragma once

#include "../g_local.h"

#define MAX_CMD_NAME_LEN 128
#define MAX_CMDS 128

typedef void (*ac_cmdFunction)(gentity_t*);

typedef struct
{
    char name[MAX_CMD_NAME_LEN];
    ac_cmdFunction func;
} ac_cmd_t;

qboolean AC_ExecuteCommand(const char *cmd, gentity_t *ent);
void AC_AddCommand(const char *cmd, ac_cmdFunction func); 
void AC_Init();

#endif  // AC_PUBLIC_H
