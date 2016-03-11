#include "ac_public.h"

#include "ac_file_format.h"
#include "ac_main.h"


#ifdef AC_FF
#error AC_FF is already defined!
#endif

// macro to keep ac_fileFields synchronized with ac_account_t structure
#define AC_FF(x) #x, offsetof(ac_account_t, x)

// MUST be synchronized with ac_fileField enum!
const ac_fileField_t ac_fileFields[AC_FIELD_MAX] =
{
    { AC_FF(login), AC_FIELDTYPE_STRING },
    { AC_FF(password), AC_FIELDTYPE_STRING },
    { AC_FF(name), AC_FIELDTYPE_STRING },
    { AC_FF(scale), AC_FIELDTYPE_NUMBER },
    { AC_FF(dp), AC_FIELDTYPE_NUMBER },
    { AC_FF(fp), AC_FIELDTYPE_NUMBER },
    { AC_FF(hp), AC_FIELDTYPE_NUMBER },
    { AC_FF(armor), AC_FIELDTYPE_NUMBER },
    { AC_FF(skills), AC_FIELDTYPE_SKILL },
    { AC_FF(forces), AC_FIELDTYPE_FORCE },
    { AC_FF(faction), AC_FIELDTYPE_NUMBER },
};

#undef AC_FF

#define AC_ENUM2STRING(arg)   { #arg, arg, sizeof(#arg) }
const forcePowerToString_t forcePowerStrings[NUM_FORCE_POWERS] =
{
    AC_ENUM2STRING(FP_HEAL),
    AC_ENUM2STRING(FP_LEVITATION),
    AC_ENUM2STRING(FP_SPEED),
    AC_ENUM2STRING(FP_PUSH),
    AC_ENUM2STRING(FP_PULL),
    AC_ENUM2STRING(FP_TELEPATHY),
    AC_ENUM2STRING(FP_GRIP),
    AC_ENUM2STRING(FP_LIGHTNING),
    AC_ENUM2STRING(FP_RAGE),
    AC_ENUM2STRING(FP_PROTECT),
    AC_ENUM2STRING(FP_ABSORB),
    AC_ENUM2STRING(FP_TEAM_HEAL),
    AC_ENUM2STRING(FP_TEAM_FORCE),
    AC_ENUM2STRING(FP_DRAIN),
    AC_ENUM2STRING(FP_SEE),
    AC_ENUM2STRING(FP_SABER_OFFENSE),
    AC_ENUM2STRING(FP_SABER_DEFENSE),
    AC_ENUM2STRING(FP_SABERTHROW),
};

const skillToString_t skillStrings[NUM_SKILLS] =
{
    AC_ENUM2STRING(SK_JETPACK),
    AC_ENUM2STRING(SK_PISTOL),
    AC_ENUM2STRING(SK_BLASTER),
    AC_ENUM2STRING(SK_THERMAL),
    AC_ENUM2STRING(SK_ROCKET),
    AC_ENUM2STRING(SK_BACTA),
    AC_ENUM2STRING(SK_FLAMETHROWER),
    AC_ENUM2STRING(SK_BOWCASTER),
    AC_ENUM2STRING(SK_FORCEFIELD),
    AC_ENUM2STRING(SK_CLOAK),
    AC_ENUM2STRING(SK_SEEKER),
    AC_ENUM2STRING(SK_SENTRY),
    AC_ENUM2STRING(SK_DETPACK),
    AC_ENUM2STRING(SK_REPEATER),
    AC_ENUM2STRING(SK_DISRUPTOR),
    AC_ENUM2STRING(SK_BLUESTYLE),
    AC_ENUM2STRING(SK_REDSTYLE),
    AC_ENUM2STRING(SK_PURPLESTYLE),
    AC_ENUM2STRING(SK_GREENSTYLE),
    AC_ENUM2STRING(SK_DUALSTYLE),
    AC_ENUM2STRING(SK_STAFFSTYLE),
    AC_ENUM2STRING(SK_REPEATERUPGRADE),
    AC_ENUM2STRING(SK_FLECHETTE),
    AC_ENUM2STRING(SK_BLASTERRATEOFFIREUPGRADE),
};

FILE *ac_accountsFile;
qboolean ac_modified;

const ac_fileField_t *AC_GetFieldFromName(const char *name)
{
    for (int i = 0; i < AC_FIELD_MAX; i++)
    {
        if (strcmp(name, ac_fileFields[i].name) == 0)
        {
            return ac_fileFields + i;
        }
    }

    return NULL;
}

char **AC_GetStringField(ac_account_t *acc, const ac_fileField_t *field)
{
    if (!acc || !field || field->type != AC_FIELDTYPE_STRING)
    {
        return NULL;
    }

    return (char **)((char *)acc + field->offset);
}

int *AC_GetNumberField(ac_account_t *acc, const ac_fileField_t *field)
{
    if (!acc || !field || field->type != AC_FIELDTYPE_NUMBER)
    {
        return NULL;
    }

    return (int *)((char *)acc + field->offset);
}

forcePowers_t AC_ForcePowerFromString(const char *name)
{
    for (int i = 0; i < NUM_FORCE_POWERS; i++)
    {
        if (strcmp(name, forcePowerStrings[i].fpName) == 0)
        {
            return forcePowerStrings[i].fp;
        }
    }

    return -1;
}

skills_t AC_SkillFromString(const char *name)
{
    for (int i = 0; i < NUM_SKILLS; i++)
    {
        if (strcmp(name, skillStrings[i].skName) == 0)
        {
            return skillStrings[i].sk;
        }
    }

    return -1;
}


// if playerNum is valid client, send error messages to him
// print in log otherwise
void AC_ParseSkills(unsigned char *skillsArray, char *value, int currentLine, int playerNum)
{
    char *skill = strtok(value, ", ");
    if (!skill)
    {
        // player has no skills, it's okay
        return;
    }

    while (skill != NULL)
    {
        size_t skillLen = strlen(skill);
        unsigned char skillLevel = atoi(skill + skillLen - 1);

        // atoi returned 0, but the last character is not 0 => last character is not digit
        if (skillLevel == 0 && skill[skillLen - 1] != '0')  // no level, default 1
        {
            skillLevel = 1;
        }
        else
        {
            skill[skillLen - 1] = '\0';  // remove level
            skillLen--;
        }

        if (skillLevel == 0)
        {
            if (playerNum >= 0 && playerNum <= MAX_CLIENTS)
            {
                trap_SendServerCommand(playerNum, va("print \"^3AC_ParseSkill: skill %s has 0 level.\n\"", skill));
            }
            else
            {
                G_LogPrintf("^3AC_ParseSkill: skill %s has 0 level (line %d).\n", skill, currentLine);
            }
        }

        if (skillLevel > 3)
        {
            if (playerNum >= 0 && playerNum <= MAX_CLIENTS)
            {
                trap_SendServerCommand(playerNum, va("print \"^3AC_ParseSkill: skill %s has too big level.\n\"", skill));
            }
            else
            {
                G_LogPrintf("^3AC_ParseSkill: skill %s has too big level (line %d).\n", skill, currentLine);
            }
            skillLevel = 3;
        }

        skills_t skillInt = AC_SkillFromString(skill);

        if (skillInt  < 0)
        {
            if (playerNum >= 0 && playerNum <= MAX_CLIENTS)
            {
                trap_SendServerCommand(playerNum, va("print \"^1AC_ParseSkill: unknown skill %s.\n\"", skill));
            }
            else
            {
                G_LogPrintf("^1AC_ParseSkill: unknown skill %s (line %d)\n", skill, currentLine);
            }

            skill = strtok(NULL, ", ");
            continue;
        }

        skillsArray[skillInt] = skillLevel;

        skill = strtok(NULL, ", ");
    }
}

// if playerNum is valid client, send error messages to him
// print in log otherwise
void AC_ParseForces(unsigned char *forceArray, char *value, int currentLine, int playerNum)
{
    char *force = strtok(value, ",");
    if (!force)
    {
        // player has no force, it's okay
        return;
    }

    while (force != NULL)
    {
        size_t forceLen = strlen(force);
        unsigned char forceLevel = atoi(force + forceLen - 1);  // last character must be force level (0-3)

        if (forceLevel == 0)
        {
            if (playerNum >= 0 && playerNum <= MAX_CLIENTS)
            {
                trap_SendServerCommand(playerNum, va("print \"^3AC_ParseForces: force %s has 0 level.\n\"", force));
            }
            else
            {
                G_LogPrintf("^3AC_ParseForces: force %s has 0 level (line %d).\n", force, currentLine);
            }
        }

        if (forceLevel > 3)
        {
            if (playerNum >= 0 && playerNum <= MAX_CLIENTS)
            {
                trap_SendServerCommand(playerNum, va("print \"^3AC_ParseForces: force %s has too big level.\n\"", force));
            }
            else
            {
                G_LogPrintf("^3AC_ParseForces: force %s has too big level (line %d).\n", force, currentLine);
            }
            forceLevel = 3;
        }

        force[forceLen - 1] = '\0';  // remove level
        forceLen--;

        forcePowers_t forceInt = AC_ForcePowerFromString(force);
        if (forceInt  < 0)
        {
            if (playerNum >= 0 && playerNum <= MAX_CLIENTS)
            {
                trap_SendServerCommand(playerNum, va("print \"^1AC_ParseForces: unknown force %s.\n\"", force));
            }
            else
            {
                G_LogPrintf("^1AC_ParseForces: unknown force %s (line %d)\n", force, currentLine);
            }

            force = strtok(NULL, ", ");
            continue;
        }

        forceArray[forceInt] = forceLevel;

        force = strtok(NULL, ", ");
    }
}

// adds currentAcc to the accounts list if currentAcc != 0 and name == "login",
// creates new currentAcc after it.
// adds field to currentAcc if it's != 0.
// creates new account if currentAcc == 0 and name == "login".
ac_account_t *AC_ParseField(char *name, char *value, ac_account_t *currentAcc, int currentLine)
{
    if (strcmp(name, ac_fileFields[AC_FIELD_LOGIN].name) == 0)
    {
        if (currentAcc)
        {
            // flush current account
            AC_AddAccount(currentAcc);
        }

        currentAcc = (ac_account_t *)calloc(1, sizeof(ac_account_t));
        currentAcc->dp = 50;
        currentAcc->fp = 100;
        currentAcc->hp = 100;
        currentAcc->scale = 100;
        currentAcc->login = (char *)calloc(strlen(value) + 1, sizeof(char));
        strcpy(currentAcc->login, value);
        // other values are 0

        return currentAcc;
    }

    if (currentAcc == NULL)
    {
        G_Error("^1AC_ParseField: accounts file must starts with login field. [line %d]\n", currentLine);
        return NULL;
    }

    const ac_fileField_t *field = AC_GetFieldFromName(name);
    if (!field)
    {
        AC_FreeAccount(currentAcc);
        G_Error("^1AC_ParseField: unknown field %s on line %d\n", name, currentLine);

        return NULL;
    }
    
    switch (field->type)
    {
    case AC_FIELDTYPE_STRING:
        {
            char **str = AC_GetStringField(currentAcc, field);
            if (*str)
            {
                AC_FreeAccount(currentAcc);
                G_Error("^1AC_ParseField: duplicated string field %s on line %d\n", name, currentLine);

                return NULL;
            }

            // create a new string and write it's address
            // Skinpack: TODO: make it a bit cleaner
            *str = (char *)calloc(strlen(value) + 1, sizeof(char));

            strcpy(*str, value);

            return currentAcc;
        }
    case AC_FIELDTYPE_NUMBER:
        {
            // Skinpack: TODO: make it a bit cleaner
            *AC_GetNumberField(currentAcc, field) = atoi(value);

            return currentAcc;
        }
    case AC_FIELDTYPE_SKILL:
        {
            AC_ParseSkills(((unsigned char *)currentAcc + field->offset), value, currentLine, -1);
            
            return currentAcc;
        }
    case AC_FIELDTYPE_FORCE:
        {
            AC_ParseForces(((unsigned char *)currentAcc + field->offset), value, currentLine, -1);

            return currentAcc;
        }

    default:
        {
            AC_FreeAccount(currentAcc);
            G_Error("^1AC_ParseField: unknown field type!\n");

            return NULL;
        }
    }
}

void AC_ReadAccounts(void)
{
    FILE *accountsFile = fopen(AC_ACCOUNTS_FILENAME, "r");

    if (!accountsFile)
    {
        G_LogPrintf("^3AC_ReadAccounts: can't open the accounts file.\n");
        return;
    }

    char lineBuffer[AC_MAX_LINE_LEN] = { 0 };
    char nameBuffer[AC_MAX_FILEFIELD_NAME] = { 0 };
    char valueBuffer[AC_MAX_FIELD_VALUE_LEN] = { 0 };

    int currentLine = 1;

    ac_account_t *currentAcc = NULL;

    // fgets returns NULL when it reaches the end of file
    while (fgets(lineBuffer, sizeof(lineBuffer), accountsFile))
    {
        size_t lineLen = strlen(lineBuffer);
        int currentPos = 0;
        int valueOffset = 0;

        if (lineBuffer[0] == '\n')  // empty string
        {
            continue;
        }

        // if lineLen < sizeof(lineBuffer), fgets writes \n into the end of lineBuffer
        // otherwise, the line is too long
        if (lineBuffer[lineLen - 1] != '\n')
        {
            AC_FreeAccount(currentAcc);
            G_Error("^1AC_ReadAccounts: line %d is too long!\n", currentLine);

            return;
        }

        lineBuffer[lineLen - 1] = '\0';  // remove \n
        lineLen--;

        // read field's name
        for (currentPos; lineBuffer[currentPos] && lineBuffer[currentPos] != ':'; currentPos++)
        {
            if (currentPos >= sizeof(nameBuffer) - 1)  // need 1 char for '\0'
            {
                AC_FreeAccount(currentAcc);
                G_Error("^1AC_ReadAccounts: field name on line %d is too long!\n", currentLine);

                return;
            }

            nameBuffer[currentPos] = lineBuffer[currentPos];
        }

        nameBuffer[currentPos] = 0;

        // reached end of line and ':' not found
        if (lineBuffer[currentPos] != ':')
        {
            AC_FreeAccount(currentAcc);
            G_Error("^1AC_ReadAccounts: no field value found on line %d!\n", currentLine);

            return;
        }

        currentPos += 2;  // skip ': '
        valueOffset = currentPos;

        // read field's value
        for (currentPos; lineBuffer[currentPos] && lineBuffer[currentPos] != ':'; currentPos++)
        {
            if (currentPos - valueOffset >= sizeof(valueBuffer) - 1)  // need 1 char for '\0'
            {
                AC_FreeAccount(currentAcc);
                G_Error("^1AC_ReadAccounts: field value on line %d is too long!\n", currentLine);

                return;
            }

            valueBuffer[currentPos - valueOffset] = lineBuffer[currentPos];
        }

        valueBuffer[currentPos - valueOffset] = '\0';

        currentAcc = AC_ParseField(nameBuffer, valueBuffer, currentAcc, currentLine);

        currentLine++;
    }

    if (currentAcc)
    {
        if (!currentAcc->login)
        {
            AC_FreeAccount(currentAcc);
        }
        else
        {
            AC_AddAccount(currentAcc);
        }
    }

    fclose(accountsFile);

    ac_modified = qfalse;
}

void AC_WriteStringField(const char *name, const char *value, FILE *file)
{
    fputs(va("%s: %s\n", name, value), file);
}

void AC_WriteNumberField(const char *name, int value, FILE *file)
{
    fputs(va("%s: %d\n", name, value), file);
}

void AC_WriteSkillsField(const char *name, const unsigned char skillsArray[NUM_SKILLS], FILE *file)
{
    char buffer[AC_MAX_LINE_LEN] = { 0 };
    int currentPos = 0;

    sprintf(buffer, "%s: ", name);
    currentPos = strlen(buffer);

    for (int i = 0; i < NUM_SKILLS; i++)
    {
        if (skillsArray[skillStrings[i].sk])
        {
            if (currentPos + skillStrings[i].nameLen + 4 >= sizeof(buffer))  // need space for level, ',' '\n' and '\0'
            {
                G_LogPrintf("^1AC_WriteSkillsField: too long skills string!\n");
                return;
            }

            strcpy(buffer + currentPos, skillStrings[i].skName);
            currentPos += skillStrings[i].nameLen - 1;  // don't copy '\0'

            buffer[currentPos++] = '0' + skillsArray[i];  // digit to ASCII char conversion
            buffer[currentPos++] = ',';
        }
    }

    buffer[currentPos - 1] = '\n';  // replace last ',' with '\n'

    fputs(buffer, file);
}

void AC_WriteForcesField(const char *name, const unsigned char forcesArray[NUM_FORCE_POWERS], FILE *file)
{
    char buffer[AC_MAX_LINE_LEN] = { 0 };
    int currentPos = 0;

    sprintf(buffer, "%s: ", name);
    currentPos = strlen(buffer);

    for (int i = 0; i < NUM_FORCE_POWERS; i++)
    {
        if (forcesArray[forcePowerStrings[i].fp])
        {
            if (currentPos + forcePowerStrings[i].nameLen + 1 >= sizeof(buffer))
            {
                G_LogPrintf("^1AC_WriteForcesField: too long forces string!\n");
                return;
            }

            strcpy(buffer + currentPos, forcePowerStrings[i].fpName);
            currentPos += forcePowerStrings[i].nameLen - 1;  // don't copy '\0'

            buffer[currentPos++] = '0' + forcesArray[i];  // digit to ASCII char conversion
            buffer[currentPos++] = ',';
        }
    }

    buffer[currentPos - 1] = '\n';  // replace last ',' with '\n'

    fputs(buffer, file);
}

void AC_WriteAccount(ac_account_t *acc, FILE *file)
{
    for (int i = 0; i < AC_FIELD_MAX; i++)
    {
        switch (ac_fileFields[i].type)
        {
        case AC_FIELDTYPE_STRING:
            {
                // Skinpack: rww: make it a bit cleaner. Use macro?
                AC_WriteStringField(ac_fileFields[i].name, *(char **)((char *)acc + ac_fileFields[i].offset), file);
                break;
            }
        case AC_FIELDTYPE_NUMBER:
            {
                AC_WriteNumberField(ac_fileFields[i].name, *(int *)((char *)acc + ac_fileFields[i].offset), file);
                break;
            }
        case AC_FIELDTYPE_SKILL:
            {
                AC_WriteSkillsField(ac_fileFields[i].name, (unsigned char *)acc + ac_fileFields[i].offset, file);
                break;
            }
        case AC_FIELDTYPE_FORCE:
            {
                AC_WriteForcesField(ac_fileFields[i].name, (unsigned char *)acc + ac_fileFields[i].offset, file);
                break;
            }

        default:
            {
                G_LogPrintf("^1AC_WriteAccount: unknown fieldType %d\n", ac_fileFields[i].type);
                break;
            }
        }
    }

    fputs("\n", file);
}

void AC_SaveAccounts(qboolean shutdown)
{
    if (!ac_modified)
    {
        return;
    }

    // back up current accounts file if we're shutting down
    if (shutdown)
    {
        ac_accountsFile = fopen(AC_ACCOUNTS_FILENAME, "r");
        if (ac_accountsFile)
        {
            FILE *accountsBackup = fopen(AC_ACCOUNTS_FILENAME_BACKUP, "w");
            if (!accountsBackup)
            {
                G_LogPrintf("^1AC_SaveAccounts: cannot open backup file.\n");
                return;
            }

            char lineBuffer[AC_MAX_LINE_LEN] = { 0 };

            while (fgets(lineBuffer, sizeof(lineBuffer), ac_accountsFile))
            {
                fputs(lineBuffer, accountsBackup);
            }

            fclose(accountsBackup);
            fclose(ac_accountsFile);
            ac_accountsFile = NULL;
        }
    }

    ac_accountsFile = fopen(AC_ACCOUNTS_FILENAME, "w");
    if (!ac_accountsFile)
    {
        G_LogPrintf("^1AC_SaveAccounts: cannot open accounts file.\n");
        return;
    }

    for (ac_account_t *acc = ac_accountsList->first; acc; acc = acc->next)
    {
        AC_WriteAccount(acc, ac_accountsFile);
    }

    fclose(ac_accountsFile);
    ac_modified = qfalse;
}
