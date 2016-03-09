#include "ac_public.h"
#include "ac_cmds.h"
#include "ac_file_format.h"

ac_cmd_t ac_cmdTable[AC_MAX_CMDS];
int ac_cmdsAmount;

// commands section
void AC_Cmd_Test(gentity_t *ent)
{
    trap_SendServerCommand(ent->playerState->clientNum, "print \"^2Coming soon!\n\"");
}

void AC_Cmd_Register(gentity_t *ent)
{
    // /ac_register <login> <password>
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
        AC_Print(ent, "ac_register: account exists.");
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
    int argc = trap_Argc();
    int selfNum = ent->playerState->clientNum;

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
        strcpy(*AC_GetStringField(acc, field), buffer);
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
        AC_ParseSkills(((unsigned char *)acc + field->offset), buffer, -1, selfNum);

        break;
    }
    case AC_FIELDTYPE_FORCE:
    {
        if (overwrite)
        {
            memset(acc->forces, 0, sizeof(acc->forces)*sizeof(acc->forces[0]));
        }
        AC_ParseForces(((unsigned char *)acc + field->offset), buffer, -1, selfNum);

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
    AC_Cmd_SetFieldHelper(ent, qtrue);
}

// Adds forces/skills instead of overwriting it
void AC_Cmd_AddField(gentity_t *ent)
{
    AC_Cmd_SetFieldHelper(ent, qfalse);
}

void AC_Cmd_FindAccount(gentity_t *ent)
{
    // /ac_find <part of login/name>
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

void AC_Cmd_ListAccounts(gentity_t *ent)
{
    int i = 1;
    for (ac_account_t *acc = ac_accountsList->first; acc; acc = acc->next)
    {
        AC_Print(ent, va("%d: ^7%s ^2[^7%s^2]", i, acc->login, acc->name));
        i++;
    }
}

void AC_Cmd_AccountDetails(gentity_t *ent)
{
    // /ac_info <login>
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
            AC_Print(ent, va("%s: %s", field->name, *AC_GetStringField(acc, field)));
            break;
        }
        case AC_FIELDTYPE_NUMBER:
        {
            AC_Print(ent, va("%s: %d", field->name, *AC_GetNumberField(acc, field)));
            break;
        }
        case AC_FIELDTYPE_FORCE:
        {
            int currentPos = 0;

            sprintf(buffer, "%s: ", field->name);
            currentPos = strlen(buffer);

            for (int i = 0; i < NUM_FORCE_POWERS; i++)
            {
                if (acc->forces[forcePowerStrings[i].fp])
                {
                    if (currentPos + forcePowerStrings[i].nameLen + 1 >= sizeof(buffer))
                    {
                        AC_Print(ent, "^1Too long forces string!\n");
                        return;
                    }

                    strcpy(buffer + currentPos, forcePowerStrings[i].fpName);
                    currentPos += forcePowerStrings[i].nameLen - 1;  // don't copy '\0'

                    buffer[currentPos++] = '0' + acc->forces[i];  // digit to ASCII char conversion
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

            sprintf(buffer, "%s: ", field->name);
            currentPos = strlen(buffer);

            for (int i = 0; i < NUM_SKILLS; i++)
            {
                if (acc->skills[skillStrings[i].sk])
                {
                    if (currentPos + skillStrings[i].nameLen + 4 >= sizeof(buffer))  // need space for level, ',' '\n' and '\0'
                    {
                        AC_Print(ent, "^1AC_WriteSkillsField: too long skills string!\n");
                        return;
                    }

                    strcpy(buffer + currentPos, skillStrings[i].skName);
                    currentPos += skillStrings[i].nameLen - 1;  // don't copy '\0'

                    buffer[currentPos++] = '0' + acc->skills[i];  // digit to ASCII char conversion
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

void AC_Cmd_Login(gentity_t *ent)
{
    AC_Print(ent, "^2Coming soon!");
}

void AC_Cmd_Logout(gentity_t *ent)
{
    AC_Print(ent, "^2Coming soon!");
}
// end commands section


// Skinpack: TODO: AC_RemoveCommand
void AC_AddCommand(const char *cmd, ac_cmdFunction func)
{
    if (ac_cmdsAmount >= AC_MAX_CMDS)
    {
        G_Printf("^1AC_Addcommand: AC_MAX_CMDS reached!\n");
        return;
    }
    
    if (strlen(cmd) > AC_MAX_CMD_NAME_LEN)
    {
        G_Printf("^3AC_Addcommand: too big cmd name.\n");
        return;
    }
    
    for (int i = 0; i < ac_cmdsAmount; i++)
    {
        if (Q_stricmp(cmd, ac_cmdTable[i].name) == 0)
        {
            G_Printf("^3AC_Addcommand: \"%s\" is already defined.\n", cmd);
            return;
        }
    }
    
    strcpy(ac_cmdTable[ac_cmdsAmount].name, cmd);
    ac_cmdTable[ac_cmdsAmount].func = func;
    ac_cmdsAmount++;
}

qboolean AC_ExecuteCommand(const char *cmd, gentity_t *ent)
{
    for (int i = 0; i < ac_cmdsAmount; i++)
    {
        if (stricmp(ac_cmdTable[i].name, cmd) == 0)
        {
            ac_cmdTable[i].func(ent);
            return qtrue;
        }
    }
    
    return qfalse;
}

void AC_InitCommands()
{
    AC_AddCommand("ac_test", AC_Cmd_Test);
    AC_AddCommand("ac_register", AC_Cmd_Register);
    AC_AddCommand("ac_removeAccount", AC_Cmd_RemoveAccount);
    AC_AddCommand("ac_setf", AC_Cmd_SetField);
    AC_AddCommand("ac_addf", AC_Cmd_AddField);
    AC_AddCommand("ac_find", AC_Cmd_FindAccount);
    AC_AddCommand("ac_list", AC_Cmd_ListAccounts);
    AC_AddCommand("ac_info", AC_Cmd_AccountDetails);

    AC_AddCommand("ac_login", AC_Cmd_Login);
    AC_AddCommand("ac_logout", AC_Cmd_Logout);
}
