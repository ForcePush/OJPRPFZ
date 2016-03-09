#include "ac_public.h"
#include "ac_cmds.h"
#include "ac_file_format.h"
#include "ac_main.h"

ac_account_list_t *ac_accountsList;

void AC_Print(gentity_t *to, const char *text)
{
    if (!to->playerState)
    {
        return;
    }

    trap_SendServerCommand(to->playerState->clientNum, va("print \"%s\n\"", text));
}

void AC_FreeAccount(ac_account_t *acc)
{
    if (!acc)
    {
        return;
    }

    if (acc->login) { free(acc->login); }
    if (acc->name) { free(acc->name); }
    if (acc->password) { free(acc->password); }

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

void AC_ShutdownAccounts()
{
    AC_SaveAccounts();

    ac_account_t *tempAcc = ac_accountsList->first;

    while (tempAcc)
    {
        ac_accountsList->first = tempAcc->next;
        ac_accountsList->size--;

        AC_FreeAccount(tempAcc);

        tempAcc = ac_accountsList->first;
    }

    free(ac_accountsList);
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

    AC_ShutdownAccounts();
}
