#include "../g_local.h"
#include "ac_public.h"
#include "ac_main.h"

#include "ac_client.h"

ac_account_t *ac_loggedPlayers[MAX_CLIENTS];

void AC_SetPlayerStats(gentity_t *ent)
{
    if (!ent->playerState ||
        !ent->client ||
        ent->playerState->clientNum < 0 ||
        ent->playerState->clientNum >= MAX_CLIENTS)
    {
        return;
    }

    ac_account_t *acc = ac_loggedPlayers[ent->playerState->clientNum];
    if (!acc)  // not logged in
    {
        return;
    }

    ent->playerState->iModelScale = acc->scale;
    ent->playerState->stats[STAT_DODGE] = ent->playerState->stats[STAT_MAX_DODGE] = acc->dp;
    ent->playerState->stats[STAT_HEALTH] = ent->playerState->stats[STAT_MAX_HEALTH] = acc->hp;
    ent->playerState->stats[STAT_ARMOR] = acc->armor;
    ent->playerState->fd.forcePower = ent->playerState->fd.forcePowerMax = acc->fp;
    
    for (int i = 0; i < NUM_FORCE_POWERS; i++)
    {
        ent->playerState->fd.forcePowerLevel[i] = acc->forces[i];
    }

    for (int i = 0; i < NUM_SKILLS; i++)
    {
        ent->client->skillLevel[i] = acc->skills[i];
    }
}

void AC_PlayerLogout(gentity_t *ent)
{
    if (!ent->playerState ||
        ent->playerState->clientNum < 0 ||
        ent->playerState->clientNum >= MAX_CLIENTS ||
        !ac_loggedPlayers[ent->playerState->clientNum])
    {
        AC_Print(ent, "^3You are not logged in.");
        return;
    }

    ac_loggedPlayers[ent->playerState->clientNum] = NULL;

    // Skinpack: TODO: FIXME: this is bad. Need to find a better way
    // of resetting player's stats.
    ent->playerState->fd.forcePower = ent->playerState->fd.forcePowerMax = 100;
    SetTeam(ent, "s");

    AC_Print(ent, "^2Logged out!");
}

void AC_PlayerLeaving(int clientNum)
{
    if (clientNum < 0 || clientNum >= MAX_CLIENTS)
    {
        return;
    }

    ac_loggedPlayers[clientNum] = NULL;
}
