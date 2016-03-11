#ifndef AC_MAIN_H
#define AC_MAIN_H
#pragma once

#include "../g_local.h"
#include "ac_public.h"

typedef struct ac_account_t_ 
{
    char *login;
    char *password;
    char *name;
    int scale;
    int fp;
    int dp;
    int hp;
    int armor;
    unsigned char skills[NUM_SKILLS];
    unsigned char forces[NUM_FORCE_POWERS];
    int faction;  // Skinpack: TODO: make an appropriate enum.

    struct ac_account_t_ *prev;
    struct ac_account_t_ *next;
} ac_account_t;

typedef struct
{
    size_t size;
    ac_account_t *first;
    ac_account_t *last;
} ac_account_list_t;

extern ac_account_list_t *ac_accountsList;

extern qboolean ac_modified;

void AC_InitAccounts();

int AC_ClientNumFromEnt(gentity_t *ent);

void AC_AddAccount(ac_account_t *acc);
qboolean AC_RemoveAccount(const char *login);
void AC_FreeAccount(ac_account_t *acc);
void AC_ShutdownAccounts(qboolean save);

ac_account_t *AC_AccountFromLogin(const char *login);
qboolean AC_AccountExists(const char *login);

qboolean AC_CheckPassword(const ac_account_t *acc, const char *password);

void AC_Print(gentity_t *to, const char *text);
void AC_PrintBroadcast(const char *text);

#endif  // AC_MAIN_H
