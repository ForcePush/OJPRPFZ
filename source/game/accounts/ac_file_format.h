#ifndef AC_FILE_FORMAT_H
#define AC_FILE_FORMAT_H
#pragma once

#include "../g_local.h"
#include "ac_public.h"
#include "ac_main.h"

#define AC_ACCOUNTS_FILENAME "accounts.dat"
#define AC_ACCOUNTS_FILENAME_BACKUP AC_ACCOUNTS_FILENAME".backup"

#define AC_MAX_FILEFIELD_NAME 64
#define AC_MAX_FIELD_VALUE_LEN 4096
#define AC_MAX_LINE_LEN (AC_MAX_FILEFIELD_NAME + AC_MAX_FIELD_VALUE_LEN + 3)  // + ': ' + \n

typedef enum
{
    AC_FIELDTYPE_STRING,
    AC_FIELDTYPE_NUMBER,
    AC_FIELDTYPE_FORCE,
    AC_FIELDTYPE_SKILL,
} ac_fileFieldType;

// MUST be synchronized with ac_fileFields array!
typedef enum
{
    AC_FIELD_LOGIN = 0,  // AC_FIELD_LOGIN must be 0
    AC_FIELD_PASSWORD,
    AC_FIELD_NAME,
    AC_FIELD_SCALE,
    AC_FIELD_DP,
    AC_FIELD_FP,
    AC_FIELD_HP,
    AC_FIELD_ARMOR,
    AC_FIELD_SKILLS,
    AC_FIELD_STANCES,
    AC_FIELD_FORCES,
    AC_FIELD_FACTION,
    AC_FIELD_MAX = AC_FIELD_FACTION,
} ac_fileField;

typedef struct
{
    const char *name;
    size_t offset;  // offset of this field in the ac_account_t structure
    ac_fileFieldType type;
} ac_fileField_t;

typedef struct
{
    const char *fpName;
    forcePowers_t fp;
    size_t nameLen;
} forcePowerToString_t;

typedef struct
{
    const char *skName;
    skills_t sk;
    size_t nameLen;
} skillToString_t;

forcePowers_t AC_ForcePowerFromString(const char *name);
skills_t AC_SkillFromString(const char *name);

extern const forcePowerToString_t forcePowerStrings[NUM_FORCE_POWERS];
extern const skillToString_t skillStrings[NUM_SKILLS];

extern const ac_fileField_t ac_fileFields[AC_FIELD_MAX];
extern FILE *ac_accountsFile;

char **AC_GetStringField(ac_account_t *acc, const ac_fileField_t *field);
int *AC_GetNumberField(ac_account_t *acc, const ac_fileField_t *field);

void AC_ParseForces(unsigned char *forceArray, char *value, int currentLine, int playerNum);
void AC_ParseSkills(unsigned char *skillsArray, char *value, int currentLine, int playerNum);

const ac_fileField_t *AC_GetFieldFromName(const char *name);

#endif  // AC_FILE_FORMAT_H
