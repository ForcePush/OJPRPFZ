#ifndef AC_CLIENT_H
#define AC_CLIENT_H
#pragma once

#include "../g_local.h"

#include "ac_public.h"
#include "ac_main.h"

extern ac_account_t *ac_loggedPlayers[MAX_CLIENTS];

void AC_PlayerLogout(gentity_t *ent);

#endif  // AC_CLIENT_H
