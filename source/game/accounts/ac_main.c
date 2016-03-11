#include "ac_public.h"
#include "ac_cmds.h"
#include "ac_file_format.h"
#include "ac_main.h"
#include "ac_client.h"

ac_account_list_t *ac_accountsList;

void AC_Print(gentity_t *to, const char *text)
{
    if (!to->client)
    {
        return;
    }

    int playerNum = AC_ClientNumFromEnt(to);
    if (playerNum < 0 || playerNum >= MAX_CLIENTS)
    {
        return;
    }

    trap_SendServerCommand(playerNum, va("print \"%s\n\"", text));
}

void AC_PrintBroadcast(const char *text)
{
    char *buffer = va("print \"%s\n\"", text);

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        trap_SendServerCommand(i, buffer);
    }
}

void AC_FreeAccount(ac_account_t *acc)
{
    if (!acc)
    {
        return;
    }

    for (int i = 0; i < AC_FIELD_MAX; i++)
    {
        if (ac_fileFields[i].type == AC_FIELDTYPE_STRING)
        {
            char **accField = AC_GetStringField(acc, &ac_fileFields[i]);
            if (accField && *accField)
            {
                free(*accField);
            }
        }
    }

    free(acc);
}

void AC_AddAccount(ac_account_t *acc)
{
    if (!ac_accountsList)
    {
        G_Error("^1AC_AddAccount: called before intialization!!!");
        return;
    }

    acc->prev = ac_accountsList->last;
    acc->next = NULL;

    if (ac_accountsList->last)
    {
        ac_accountsList->last->next = acc;
    }
    else
    {
        ac_accountsList->first = acc;
    }

    ac_accountsList->last = acc;
    ac_accountsList->size++;

    ac_modified = qtrue;
}

void AC_InitAccounts()
{
    if (ac_accountsList)
    {
        G_LogPrintf("^3AC_InitAccounts: already initialized.\n");
        return;
    }

    ac_accountsList = (ac_account_list_t *)calloc(1, sizeof(ac_account_list_t));

    AC_ReadAccounts();
}

void AC_ShutdownAccounts(qboolean save)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (ac_loggedPlayers[i])
        {
            AC_PlayerLogout(g_entities + i);
        }
    }

    if (save)
    {
        AC_SaveAccounts(qtrue);
    }

    ac_account_t *tempAcc = ac_accountsList->first;

    while (tempAcc)
    {
        ac_accountsList->first = tempAcc->next;
        ac_accountsList->size--;

        AC_FreeAccount(tempAcc);

        tempAcc = ac_accountsList->first;
    }

    free(ac_accountsList);
    ac_accountsList = NULL;
}

ac_account_t *AC_AccountFromLogin(const char *login)
{
    for (ac_account_t *acc = ac_accountsList->first; acc; acc = acc->next)
    {
        if (strcmp(login, acc->login) == 0)
        {
            return acc;
        }
    }

    return NULL;
}

qboolean AC_AccountExists(const char *login)
{
    return (AC_AccountFromLogin(login) != NULL);
}

qboolean AC_RemoveAccount(const char *login)
{
    if (!login)
    {
        return qfalse;
    }

    ac_account_t *acc = NULL;
    for (acc = ac_accountsList->first; acc; acc = acc->next)
    {
        if (strcmp(login, acc->login) == 0)
        {
            break;
        }
    }

    if (acc)
    {
        if (acc->prev)
        {
            acc->prev->next = acc->next;
        }

        if (acc->next)
        {
            acc->next->prev = acc->prev;
        }

        AC_FreeAccount(acc);

        ac_accountsList->size--;
        ac_modified = qtrue;

        return qtrue;
    }
    else
    {
        return qfalse;
    }
}

int AC_ClientNumFromEnt(gentity_t *ent)
{
    return ent - g_entities;
}

// Skinpack: TODO: add some password hashing/encryption
qboolean AC_CheckPassword(const ac_account_t *acc, const char *password)
{
    return (strcmp(acc->password, password) == 0);
}

void AC_Init()
{
    AC_InitCommands();
    AC_InitAccounts();
}

void AC_Shutdown()
{
    if (ac_accountsFile)  // G_Error happened while parsing accounts file
    {
        fclose(ac_accountsFile);
        ac_accountsFile = NULL;
    }

    AC_ShutdownAccounts(qtrue);
}
