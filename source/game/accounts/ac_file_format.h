#ifndef AC_FILE_FORMAT_H
#define AC_FILE_FORMAT_H
#pragma once

#include "../g_local.h"
#include "ac_public.h"
#include "ac_main.h"

#define AC_ACCOUNTS_FILENAME "accounts.dat"
#define AC_MAX_FILEFIELD_NAME 64
#define AC_MAX_FIELD_VALUE_LEN 4096
#define AC_MAX_LINE_LEN (AC_MAX_FILEFIELD_NAME + AC_MAX_FIELD_VALUE_LEN + 3)  // + ': ' + \n

typedef enum
{
    AC_FIELDTYPE_STRING,
    AC_FIELDTYPE_NUMBER,
    AC_FIELDTYPE_FORCE,
    AC_FIELDTYPE_SKILL,
    AC_FIELDTYPE_STANCE,
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
    AC_FIELD_MAX,
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
} forcePowerToString_t;

typedef struct
{
    const char *skName;
    skills_t sk;
} skillToString_t;

extern const forcePowerToString_t forcePowerStrings[NUM_FORCE_POWERS];
extern const skillToString_t skillStrings[NUM_SKILLS];

extern const ac_fileField_t ac_fileFields[AC_FIELD_MAX];
extern FILE *ac_accountsFile;

void AC_ReadAccounts();
void AC_SaveAccounts();

#endif  // AC_FILE_FORMAT_H
