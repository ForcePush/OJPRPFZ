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
    int faction;  // Skinpack: TODO: make an appropriate enum.
    int scale;
    int fp;
    int dp;
    int hp;
    int armor;
    unsigned char skills[NUM_SKILLS];
    unsigned char stances[NUM_SKILLS];
    unsigned char forces[NUM_FORCE_POWERS];

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

void AC_AddAccount(ac_account_t *acc);
void AC_FreeAccount(ac_account_t *acc);

#endif  // AC_MAIN_H
