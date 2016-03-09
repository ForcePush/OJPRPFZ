#ifndef AC_PUBLIC_H
#define AC_PUBLIC_H
#pragma once

#include "../g_local.h"

typedef void (*ac_cmdFunction)(gentity_t*);

qboolean AC_ExecuteCommand(const char *cmd, gentity_t *ent);

void AC_Init();
void AC_Shutdown();

void AC_ReadAccounts();
void AC_SaveAccounts();

#endif  // AC_PUBLIC_H
