#ifndef U_PUBLIC_H
#define U_PUBLIC_H
#pragma once

#define NO_ICARUS
#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus
#include "../g_local.h"

   qboolean Util_ExecuteCommand(const char *cmd, gentity_t *ent);

#ifdef __cplusplus
}
#endif  // __cplusplus
#endif  // U_MAIN_H