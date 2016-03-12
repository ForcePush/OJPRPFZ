#ifndef AC_PUBLIC_H
#define AC_PUBLIC_H
#pragma once

#ifdef __cplusplus
extern "C" {
#define NO_ICARUS 

#include "../g_local.h"

    qboolean AC_ExecuteCommand(const char *cmd, gentity_t *ent);

    void AC_Init(void);
    void AC_Shutdown(void);

    void AC_ReadAccounts(void);
    void AC_SaveAccounts(qboolean shutdown);

    // called after player's respawn
    void AC_SetPlayerStats(gentity_t *ent);
    void AC_PlayerLeaving(int clientNum);
}
#endif  // __cplusplus
#endif  // AC_PUBLIC_H
