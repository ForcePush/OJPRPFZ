#ifndef AC_CMDS_H
#define AC_CMDS_H
#pragma once

#include "ac_public.h"

extern ac_cmd_t ac_cmdTable[MAX_CMDS];
extern int ac_cmdsAmount;

void AC_InitCommands();

#endif  // AC_CMDS_H
