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
    { AC_FF(stances), AC_FIELDTYPE_STANCE },
    { AC_FF(faction), AC_FIELDTYPE_NUMBER },
};

#undef AC_FF

const forcePowerToString_t forcePowerStrings[NUM_FORCE_POWERS] =
{
    ENUM2STRING(FP_HEAL),
    ENUM2STRING(FP_LEVITATION),
    ENUM2STRING(FP_SPEED),
    ENUM2STRING(FP_PUSH),
    ENUM2STRING(FP_PULL),
    ENUM2STRING(FP_TELEPATHY),
    ENUM2STRING(FP_GRIP),
    ENUM2STRING(FP_LIGHTNING),
    ENUM2STRING(FP_RAGE),
    ENUM2STRING(FP_PROTECT),
    ENUM2STRING(FP_ABSORB),
    ENUM2STRING(FP_TEAM_HEAL),
    ENUM2STRING(FP_TEAM_FORCE),
    ENUM2STRING(FP_DRAIN),
    ENUM2STRING(FP_SEE),
    ENUM2STRING(FP_SABER_OFFENSE),
    ENUM2STRING(FP_SABER_DEFENSE),
    ENUM2STRING(FP_SABERTHROW),
};

const skillToString_t skillStrings[NUM_SKILLS] =
{
    ENUM2STRING(SK_JETPACK),
    ENUM2STRING(SK_PISTOL),
    ENUM2STRING(SK_BLASTER),
    ENUM2STRING(SK_THERMAL),
    ENUM2STRING(SK_ROCKET),
    ENUM2STRING(SK_BACTA),
    ENUM2STRING(SK_FLAMETHROWER),
    ENUM2STRING(SK_BOWCASTER),
    ENUM2STRING(SK_FORCEFIELD),
    ENUM2STRING(SK_CLOAK),
    ENUM2STRING(SK_SEEKER),
    ENUM2STRING(SK_SENTRY),
    ENUM2STRING(SK_DETPACK),
    ENUM2STRING(SK_REPEATER),
    ENUM2STRING(SK_DISRUPTOR),
    ENUM2STRING(SK_BLUESTYLE),
    ENUM2STRING(SK_REDSTYLE),
    ENUM2STRING(SK_PURPLESTYLE),
    ENUM2STRING(SK_GREENSTYLE),
    ENUM2STRING(SK_DUALSTYLE),
    ENUM2STRING(SK_STAFFSTYLE),
    ENUM2STRING(SK_REPEATERUPGRADE),
    ENUM2STRING(SK_FLECHETTE),
    ENUM2STRING(SK_BLASTERRATEOFFIREUPGRADE),
};

FILE *ac_accountsFile;

void AC_ParseSkill(unsigned char *skillsArray, char *value, int currentLine, ac_account_t *currentAcc)
{
    char *skill = strtok(value, ",");
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
            G_LogPrintf("^3AC_ParseSkill: skill %s has 0 level (line %d).\n", skill, currentLine);
        }

        if (skillLevel > 3)
        {
            G_LogPrintf("^3AC_ParseSkill: skill %s has too big level (line %d).\n", skill, currentLine);
            skillLevel = 3;
        }

        for (int i = 0; i <= NUM_SKILLS; i++)
        {
            if (i >= NUM_SKILLS)
            {
                AC_FreeAccount(currentAcc);
                G_Error("^1AC_ParseSkill: unknown skill %s (line %d)\n", skill, currentLine);

                return;
            }

            if (strcmp(skill, skillStrings[i].skName) == 0)
            {
                skillsArray[skillStrings[i].sk] = skillLevel;
                break;
            }
        }

        skill = strtok(NULL, ",");
    }
}

void AC_ParseForce(unsigned char *forceArray, char *value, int currentLine, ac_account_t *currentAcc)
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
            G_LogPrintf("^3AC_ParseForce: force %s has 0 level (line %d).\n", force, currentLine);
        }

        if (forceLevel > 3)
        {
            G_LogPrintf("^3AC_ParseForce: force %s has too big level (line %d).\n", force, currentLine);
            forceLevel = 3;
        }

        force[forceLen - 1] = '\0';  // remove level
        forceLen--;

        for (int i = 0; i <= NUM_FORCE_POWERS; i++)
        {
            if (i >= NUM_FORCE_POWERS)
            {
                AC_FreeAccount(currentAcc);
                G_Error("^1AC_ParseForce: unknown force %s (line %d)\n", force, currentLine);

                return;
            }

            if (strcmp(force, forcePowerStrings[i].fpName) == 0)
            {
                forceArray[forcePowerStrings[i].fp] = forceLevel;
                break;
            }
        }

        force = strtok(NULL, ",");
    }
}

// adds currentAcc to the accounts list if currentAcc != 0 and name == "login",
// creates new currentAcc after it.
// adds field to currentAcc if it's != 0.
// creates new account if currentAcc == 0 and name == "login".
ac_account_t *AC_ParseField(char *name, char *value, ac_account_t *currentAcc, int currentLine)
{
    ac_fileField field = 0;
    ac_fileFieldType type = 0;

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

    for (int i = AC_FIELD_LOGIN + 1; i <= AC_FIELD_MAX; i++)
    {
        if (i >= AC_FIELD_MAX)
        {
            AC_FreeAccount(currentAcc);
            G_Error("^1AC_ParseField: unknown field %s on line %d\n", name, currentLine);

            return NULL;
        }

        if (strcmp(name, ac_fileFields[i].name) == 0)
        {
            field = i;
            type = ac_fileFields[i].type;

            break;
        }
    }

    switch (type)
    {
    case AC_FIELDTYPE_STRING:
        {
            char **str = (char **)((char *)currentAcc + ac_fileFields[field].offset);
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
            *(int *)((char *)currentAcc + ac_fileFields[field].offset) = atoi(value);

            return currentAcc;
        }
    case AC_FIELDTYPE_STANCE:
    case AC_FIELDTYPE_SKILL:
        {
            AC_ParseSkill(((unsigned char *)currentAcc + ac_fileFields[field].offset), value, currentLine, currentAcc);
            
            return currentAcc;
        }
    case AC_FIELDTYPE_FORCE:
        {
            AC_ParseForce(((unsigned char *)currentAcc + ac_fileFields[field].offset), value, currentLine, currentAcc);

            return currentAcc;
        }

    default:
        {
            AC_FreeAccount(currentAcc);
            G_Error("^1AC_ParseField: unknown filetype!\n");

            return NULL;
        }
    }
}

void AC_ReadAccounts()
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
}

void AC_SaveAccounts()
{

}
