#include "ac_public.h"
#include "ac_cmds.h"
#include "ac_file_format.h"
#include "ac_client.h"
#include "ac_admin.h"

#include "../util/u_cmds.h"

// Skinpack: TODO: a better way of doing it?
qboolean AC_Cmd_AdminCheck(gentity_t *ent)
{
    if (!AC_CheckAdmin(ent))
    {
        AC_Print(ent, "^1You're not an admin! Login with /ac_adminLogin.");
        return qfalse;
    }

    return qtrue;
}

// commands section
void AC_Cmd_Test(gentity_t *ent)
{
    int playerNum = AC_ClientNumFromEnt(ent);
    if (playerNum < 0 || playerNum >= MAX_CLIENTS)
    {
        return;
    }

    trap_SendServerCommand(playerNum, "print \"^2Coming soon!\n\"");
}

void AC_Cmd_Register(gentity_t *ent)
{
    // /ac_register <login> <password> <name>
    if (!AC_Cmd_AdminCheck(ent))
    {
        return;
    }

    int argc = trap_Argc();
    if (argc < 4)
    {
        AC_Print(ent, "^3ac_register: usage: /ac_register <login> <password> <name>");
        return;
    }

    char buffer[AC_ARGV_BUFFER_LEN] = { 0 };
    trap_Argv(1, buffer, sizeof(buffer));  // login

    if (AC_AccountExists(buffer))
    {
        AC_Print(ent, "^3ac_register: account exists.");
        return;
    }

    ac_account_t *acc = (ac_account_t *)calloc(1, sizeof(ac_account_t));

    acc->login = (char *)calloc(strlen(buffer) + 1, sizeof(char));
    strcpy(acc->login, buffer);

    trap_Argv(2, buffer, sizeof(buffer));  // password
    acc->password = (char *)calloc(strlen(buffer) + 1, sizeof(char));
    strcpy(acc->password, buffer);

    trap_Argv(3, buffer, sizeof(buffer));  // name
    acc->name = (char *)calloc(strlen(buffer) + 1, sizeof(char));
    strcpy(acc->name, buffer);

    AC_AddAccount(acc);
}

void AC_Cmd_RemoveAccount(gentity_t *ent)
{
    // /ac_removeAccount <login>
    if (!AC_Cmd_AdminCheck(ent))
    {
        return;
    }

    int argc = trap_Argc();

    if (argc < 2)
    {
        AC_Print(ent, "^3ac_removeAccount: usage: /ac_removeAccount <login>");
        return;
    }

    char buffer[AC_ARGV_BUFFER_LEN] = { 0 };
    trap_Argv(1, buffer, sizeof(buffer));

    if (AC_RemoveAccount(buffer))
    {
        AC_Print(ent, "^2Removed.");
    }
    else
    {
        AC_Print(ent, "^3Account not found.");
    }
}

void AC_Cmd_SetFieldHelper(gentity_t *ent, qboolean overwrite)
{
    // /ac_setf <login> <field> <value>
    if (!AC_Cmd_AdminCheck(ent))
    {
        return;
    }

    int argc = trap_Argc();
    int playerNum = AC_ClientNumFromEnt(ent);
    if (playerNum < 0 || playerNum >= MAX_CLIENTS)
    {
        return;
    }

    if (argc < 4)
    {
        if (overwrite)
        {
            AC_Print(ent, "^3ac_setField: usage: /ac_setf <login> <field> <value>");
        }
        else
        {
            AC_Print(ent, "^3ac_addField: usage: /ac_addf <login> <field> <value>");
        }
        return;
    }

    char buffer[AC_ARGV_BUFFER_LEN] = { 0 };
    trap_Argv(1, buffer, sizeof(buffer));  // login
    ac_account_t *acc = AC_AccountFromLogin(buffer);
    if (!acc)
    {
        AC_Print(ent, "^3Account not found.");
        return;
    }

    trap_Argv(2, buffer, sizeof(buffer));  // field
    const ac_fileField_t *field = AC_GetFieldFromName(buffer);
    if (!field)
    {
        AC_Print(ent, "^3Wrong field.");
        return;
    }

    trap_Argv(3, buffer, sizeof(buffer));  // value
    switch (field->type)
    {
    case AC_FIELDTYPE_STRING:
    {
        char **accField = AC_GetStringField(acc, field);
        if (*accField)
        {
            free(*accField);
        }
        *accField = (char *)calloc(strlen(buffer) + 1, sizeof(char));
        strcpy(*accField, buffer);

        break;
    }
    case AC_FIELDTYPE_NUMBER:
    {
        *AC_GetNumberField(acc, field) = atoi(buffer);
        break;
    }
    case AC_FIELDTYPE_SKILL:
    {
        if (overwrite)
        {
            memset(acc->skills, 0, sizeof(acc->skills)*sizeof(acc->skills[0]));
        }
        AC_ParseSkills(((unsigned char *)acc + field->offset), buffer, -1, playerNum);

        break;
    }
    case AC_FIELDTYPE_FORCE:
    {
        if (overwrite)
        {
            memset(acc->forces, 0, sizeof(acc->forces)*sizeof(acc->forces[0]));
        }
        AC_ParseForces(((unsigned char *)acc + field->offset), buffer, -1, playerNum);

        break;
    }

    default:
    {
        AC_Print(ent, "^1unknown fieldType! Should never happen!");
        return;
    }
    }

    ac_modified = qtrue;
}

void AC_Cmd_SetField(gentity_t *ent)
{
    if (!AC_Cmd_AdminCheck(ent))
    {
        return;
    }

    AC_Cmd_SetFieldHelper(ent, qtrue);
}

// Adds forces/skills instead of overwriting it
void AC_Cmd_AddField(gentity_t *ent)
{
    if (!AC_Cmd_AdminCheck(ent))
    {
        return;
    }

    AC_Cmd_SetFieldHelper(ent, qfalse);
}

void AC_Cmd_FindAccount(gentity_t *ent)
{
    // /ac_find <part of login/name>
    if (!AC_Cmd_AdminCheck(ent))
    {
        return;
    }

    int argc = trap_Argc();
    if (argc < 2)
    {
        AC_Print(ent, "^3ac_find: usage: /ac_find <part of login/name>");
        return;
    }

    char buffer[AC_ARGV_BUFFER_LEN] = { 0 };
    trap_Argv(1, buffer, sizeof(buffer));  // part of login/name

    for (ac_account_t *acc = ac_accountsList->first; acc; acc = acc->next)
    {
        if (strstr(acc->name, buffer) || strstr(acc->login, buffer))
        {
            AC_Print(ent, va("^2Found: ^7%s ^2[^7%s^2]", acc->login, acc->name));
        }
    }
}

#define AC_ACCS_PER_PAGE 15
void AC_Cmd_ListAccounts(gentity_t *ent)
{
    // /ac_list <page>
    if (!AC_Cmd_AdminCheck(ent))
    {
        return;
    }

    int argc = trap_Argc();
    if (argc < 2)
    {
        AC_Print(ent, "^3ac_list: usage: /ac_list <page>");
        int totalPages = ac_accountsList->size / AC_ACCS_PER_PAGE +
            (ac_accountsList->size % AC_ACCS_PER_PAGE == 0 ? 0 : 1);

        AC_Print(ent, va("^3Total pages: %d", totalPages));
        return;
    }

    char buffer[AC_ARGV_BUFFER_LEN] = { 0 };
    trap_Argv(1, buffer, sizeof(buffer));  // page

    int startNumber = (atoi(buffer) - 1) * AC_ACCS_PER_PAGE + 1;  // start from 1
    int endNumber = startNumber + AC_ACCS_PER_PAGE;
    int i = 1;

    if (startNumber < 0 || endNumber < 0)
    {
        AC_Print(ent, "^3Wrong page number.");
        return;
    }

    for (ac_account_t *acc = ac_accountsList->first;
         acc && i < endNumber;
         acc = acc->next, i++)
    {
        if (i < startNumber)
        {
            continue;
        }

        AC_Print(ent, va("%d: ^7%s ^2[^7%s^2]", i, acc->login, acc->name));
    }
}

void AC_Cmd_AccountDetails(gentity_t *ent)
{
    // /ac_info <login>
    if (!AC_Cmd_AdminCheck(ent))
    {
        return;
    }

    int argc = trap_Argc();

    if (argc < 2)
    {
        AC_Print(ent, "^3ac_info: usage: /ac_info <login>");
        return;
    }

    char argBuffer[AC_ARGV_BUFFER_LEN] = { 0 };
    char buffer[AC_MAX_LINE_LEN] = { 0 };
    trap_Argv(1, argBuffer, sizeof(argBuffer));  // login

    ac_account_t *acc = AC_AccountFromLogin(argBuffer);
    if (!acc)
    {
        AC_Print(ent, "^3Account not found.");
        return;
    }

    for (int i = 0; i < AC_FIELD_MAX; i++)
    {
        buffer[0] = '\0';

        const ac_fileField_t *field = ac_fileFields + i;
        switch (field->type)
        {
        case AC_FIELDTYPE_STRING:
        {
            AC_Print(ent, va("%s^2: ^7%s", field->name, *AC_GetStringField(acc, field)));
            break;
        }
        case AC_FIELDTYPE_NUMBER:
        {
            AC_Print(ent, va("%s^2: ^7%d", field->name, *AC_GetNumberField(acc, field)));
            break;
        }
        case AC_FIELDTYPE_FORCE:
        {
            int currentPos = 0;

            sprintf(buffer, "%s^2:^7 ", field->name);
            currentPos = strlen(buffer);

            for (int k = 0; k < NUM_FORCE_POWERS; k++)
            {
                if (acc->forces[forcePowerStrings[k].fp])
                {
                    if (currentPos + forcePowerStrings[k].nameLen + 1 >= sizeof(buffer))
                    {
                        AC_Print(ent, "^1Too long forces string!\n");
                        return;
                    }

                    strcpy(buffer + currentPos, forcePowerStrings[k].fpName);
                    currentPos += forcePowerStrings[k].nameLen - 1;  // don't copy '\0'

                    buffer[currentPos++] = '0' + acc->forces[k];  // digit to ASCII char conversion
                    buffer[currentPos++] = ',';
                }
            }

            buffer[currentPos - 1] = '\0';  // replace last ',' with '\0'
            AC_Print(ent, buffer);
            
            break;
        }
        case AC_FIELDTYPE_SKILL:
        {
            int currentPos = 0;

            sprintf(buffer, "%s^2:^7 ", field->name);
            currentPos = strlen(buffer);

            for (int k = 0; k < NUM_SKILLS; k++)
            {
                if (acc->skills[skillStrings[k].sk])
                {
                    if (currentPos + skillStrings[k].nameLen + 4 >= sizeof(buffer))  // need space for level, ',' '\n' and '\0'
                    {
                        AC_Print(ent, "^1AC_WriteSkillsField: too long skills string!\n");
                        return;
                    }

                    strcpy(buffer + currentPos, skillStrings[k].skName);
                    currentPos += skillStrings[k].nameLen - 1;  // don't copy '\0'

                    buffer[currentPos++] = '0' + acc->skills[k];  // digit to ASCII char conversion
                    buffer[currentPos++] = ',';
                }
            }

            buffer[currentPos - 1] = '\0';  // replace last ',' with '\0'
            AC_Print(ent, buffer);

            break;
        }

        default:
        {
            AC_Print(ent, "^1unknown fieldType! Should never happen!");
            break;
        }
        }
    }
}

void AC_Cmd_ReloadAccounts(gentity_t *ent)
{
    // it is dangerous command, so it has a complicated syntax
    // /ac_reloadAccounts <save or dontSave>
    if (!AC_Cmd_AdminCheck(ent))
    {
        return;
    }

    int argc = trap_Argc();
    if (argc < 2)
    {
        AC_Print(ent, "^3ac_reloadAccounts: usage: /ac_reloadAccounts <save or dontSave>");
        return;
    }

    char buffer[AC_ARGV_BUFFER_LEN] = { 0 };
    trap_Argv(1, buffer, sizeof(buffer));  // save or dontSave

    if (strcmp(buffer, "save") == 0)
    {
        AC_ShutdownAccounts(qtrue);
    }
    else if (strcmp(buffer, "dontSave") == 0)
    {
        AC_ShutdownAccounts(qfalse);
    }
    else
    {
        AC_Print(ent, "^3ac_reloadAccounts: usage: /ac_reloadAccounts <save or dontSave>");
        return;
    }

    AC_InitAccounts();
}

void AC_Cmd_Login(gentity_t *ent)
{
    // /ac_login <login> <password>
    int argc = trap_Argc();
    int playerNum = AC_ClientNumFromEnt(ent);
    if (playerNum < 0 || playerNum >= MAX_CLIENTS)
    {
        return;
    }

    if (argc < 3)
    {
        AC_Print(ent, "^3ac_login: usage: /ac_login <login> <password>");
        return;
    }

    if (ac_loggedPlayers[playerNum] != NULL)
    {
        AC_Print(ent, va("^3You're already logged on, ^7%s^3!", ac_loggedPlayers[playerNum]->name));
        return;
    }

    char buffer[AC_ARGV_BUFFER_LEN] = { 0 };
    trap_Argv(1, buffer, sizeof(buffer));  // login

    ac_account_t *acc = AC_AccountFromLogin(buffer);
    if (!acc)
    {
        AC_Print(ent, "^3Account not found.");
        return;
    }

    trap_Argv(2, buffer, sizeof(buffer));  // password
    if (!AC_CheckPassword(acc, buffer))
    {
        AC_Print(ent, "^3Bad password.");
        return;
    }

    ac_loggedPlayers[playerNum] = acc;
    if (ent->client->sess.sessionTeam != TEAM_SPECTATOR)
    {
        // Skinpack: TODO: maybe copy/paste force/weapon code
        // from ClientSpawn instead of just respawning client?
        ClientSpawn(ent);
    }

    AC_Print(ent, va("^2You're logged on! Hello, ^7%s^7!", acc->name));
    AC_PrintBroadcast(va("^7%s ^2is logged on as ^7%s!", ent->client->pers.netname, acc->name));
}

void AC_Cmd_Logout(gentity_t *ent)
{
    AC_PlayerLogout(ent);
}
// end commands section

void AC_InitCommands(void)
{
    Util::Commands::addCommand("ac_test", AC_Cmd_Test);
    Util::Commands::addCommand("ac_register", AC_Cmd_Register);
    Util::Commands::addCommand("ac_removeAccount", AC_Cmd_RemoveAccount);
    Util::Commands::addCommand("ac_setf", AC_Cmd_SetField);
    Util::Commands::addCommand("ac_addf", AC_Cmd_AddField);
    Util::Commands::addCommand("ac_find", AC_Cmd_FindAccount);
    Util::Commands::addCommand("ac_list", AC_Cmd_ListAccounts);
    Util::Commands::addCommand("ac_info", AC_Cmd_AccountDetails);
    Util::Commands::addCommand("ac_reloadAccounts", AC_Cmd_ReloadAccounts);
    Util::Commands::addCommand("ac_adminLogin", AC_Cmd_AdminLogin);
    Util::Commands::addCommand("ac_adminLogout", AC_Cmd_AdminLogout);

    Util::Commands::addCommand("ac_login", AC_Cmd_Login);
    Util::Commands::addCommand("ac_logout", AC_Cmd_Logout);

	return;
}
