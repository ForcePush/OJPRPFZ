#ifndef AC_ADMIN_H
#define AC_ADMIN_H
#pragma once

#define NO_ICARUS 
extern "C" {
#include "../g_local.h"
}
#include "ac_public.h"
#include "ac_main.h"
#include "ac_cmds.h"

void AC_Cmd_AdminLogin(gentity_t *ent);
void AC_Cmd_AdminLogout(gentity_t *ent);

qboolean AC_CheckAdmin(gentity_t *ent);

extern qboolean ac_adminLoggedOn[MAX_CLIENTS];

#endif  // AC_ADMIN_H
