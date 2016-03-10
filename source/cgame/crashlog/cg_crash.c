//[CrashLog]
#include "../cg_local.h"

#include <windows.h>
#include <process.h>
#include <imagehlp.h>

char crashLog[MAX_PATH] = { 0 };

void QDECL CG_CrashlogPrintf(const char *fmt, ...) {
    va_list		argptr;
    char		string[4096];

    time_t rawtime;
    struct tm *timev;
    time(&rawtime);
    timev = localtime(&rawtime);

    if (!crashLog[0])
    {
        sprintf(crashLog, "crashlog_%d_%d_%d_%d.log", timev->tm_mon + 1, timev->tm_mday, timev->tm_min, timev->tm_sec);
    }

    va_start(argptr, fmt);
    Q_vsnprintf(string, sizeof(string), fmt, argptr);
    va_end(argptr);


    FILE *f = fopen(crashLog, "a");
    if (!f)
    {
        return;
    }
    fputs(string, f);
    fclose(f);   // flush every write
}

#pragma warning(disable:4090)
#pragma warning(disable:4028)
HMODULE	imagehlp = NULL;

typedef BOOL(WINAPI *PFNSYMINITIALIZE)(HANDLE, LPSTR, BOOL);
typedef BOOL(WINAPI *PFNSYMCLEANUP)(HANDLE);
typedef PGET_MODULE_BASE_ROUTINE PFNSYMGETMODULEBASE;
typedef BOOL(WINAPI *PFNSTACKWALK)(DWORD, HANDLE, HANDLE, LPSTACKFRAME, LPVOID, PREAD_PROCESS_MEMORY_ROUTINE, PFUNCTION_TABLE_ACCESS_ROUTINE, PGET_MODULE_BASE_ROUTINE, PTRANSLATE_ADDRESS_ROUTINE);
typedef BOOL(WINAPI *PFNSYMGETSYMFROMADDR)(HANDLE, DWORD, LPDWORD, PIMAGEHLP_SYMBOL);
typedef BOOL(WINAPI *PFNSYMENUMERATEMODULES)(HANDLE, PSYM_ENUMMODULES_CALLBACK, PVOID);
typedef PFUNCTION_TABLE_ACCESS_ROUTINE PFNSYMFUNCTIONTABLEACCESS;

PFNSYMINITIALIZE pfnSymInitialize = NULL;
PFNSYMCLEANUP pfnSymCleanup = NULL;
PFNSYMGETMODULEBASE pfnSymGetModuleBase = NULL;
PFNSTACKWALK pfnStackWalk = NULL;
PFNSYMGETSYMFROMADDR pfnSymGetSymFromAddr = NULL;
PFNSYMENUMERATEMODULES pfnSymEnumerateModules = NULL;
PFNSYMFUNCTIONTABLEACCESS pfnSymFunctionTableAccess = NULL;

/*
Visual C 7 Users, place the PDB file generated with the build into your ensimod
directory otherwise your stack traces will be useless.

Visual C 6 Users, shouldn't need the PDB file since the DLL will contain COFF symbols.

Watch this space for mingw.
*/

BOOL CALLBACK EnumModules(LPSTR ModuleName, DWORD BaseOfDll, PVOID UserContext) {
    CG_CrashlogPrintf("0x%08x\t%s\n", BaseOfDll, ModuleName);
    return TRUE;
}

char *ExceptionName(DWORD exceptioncode) {
    switch (exceptioncode) {
    case EXCEPTION_ACCESS_VIOLATION: return "Access violation"; break;
    case EXCEPTION_DATATYPE_MISALIGNMENT: return "Datatype misalignment"; break;
    case EXCEPTION_BREAKPOINT: return "Breakpoint"; break;
    case EXCEPTION_SINGLE_STEP: return "Single step"; break;
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: return "Array bounds exceeded"; break;
    case EXCEPTION_FLT_DENORMAL_OPERAND: return "Float denormal operand"; break;
    case EXCEPTION_FLT_DIVIDE_BY_ZERO: return "Float divide by zero"; break;
    case EXCEPTION_FLT_INEXACT_RESULT: return "Float inexact result"; break;
    case EXCEPTION_FLT_INVALID_OPERATION: return "Float invalid operation"; break;
    case EXCEPTION_FLT_OVERFLOW: return "Float overflow"; break;
    case EXCEPTION_FLT_STACK_CHECK: return "Float stack check"; break;
    case EXCEPTION_FLT_UNDERFLOW: return "Float underflow"; break;
    case EXCEPTION_INT_DIVIDE_BY_ZERO: return "Integer divide by zero"; break;
    case EXCEPTION_INT_OVERFLOW: return "Integer overflow"; break;
    case EXCEPTION_PRIV_INSTRUCTION: return "Privileged instruction"; break;
    case EXCEPTION_IN_PAGE_ERROR: return "In page error"; break;
    case EXCEPTION_ILLEGAL_INSTRUCTION: return "Illegal instruction"; break;
    case EXCEPTION_NONCONTINUABLE_EXCEPTION: return "Noncontinuable exception"; break;
    case EXCEPTION_STACK_OVERFLOW: return "Stack overflow"; break;
    case EXCEPTION_INVALID_DISPOSITION: return "Invalid disposition"; break;
    case EXCEPTION_GUARD_PAGE: return "Guard page"; break;
    case EXCEPTION_INVALID_HANDLE: return "Invalid handle"; break;
    default: break;
    }
    return "Unknown exception";
}

void win32_exceptioninfo(LPEXCEPTION_POINTERS e) {
    CG_CrashlogPrintf("Exception: %s (0x%08x)\n", ExceptionName(e->ExceptionRecord->ExceptionCode), e->ExceptionRecord->ExceptionCode);
    CG_CrashlogPrintf("Exception Address: 0x%08x\n", e->ExceptionRecord->ExceptionAddress);
}

void win32_dllinfo(void) {
    CG_CrashlogPrintf("DLL Information:\n");
    pfnSymEnumerateModules(GetCurrentProcess(), EnumModules, NULL);
}

void win32_backtrace(LPEXCEPTION_POINTERS e) {
    PIMAGEHLP_SYMBOL pSym;
    STACKFRAME sf;
    HANDLE process, thread;
    DWORD dwModBase, Disp;
    BOOL more = FALSE;
    int cnt = 0;
    char modname[MAX_PATH] = "";

    pSym = (PIMAGEHLP_SYMBOL)GlobalAlloc(GMEM_FIXED, 16384);

    ZeroMemory(&sf, sizeof(sf));
    sf.AddrPC.Offset = e->ContextRecord->Eip;
    sf.AddrStack.Offset = e->ContextRecord->Esp;
    sf.AddrFrame.Offset = e->ContextRecord->Ebp;
    sf.AddrPC.Mode = AddrModeFlat;
    sf.AddrStack.Mode = AddrModeFlat;
    sf.AddrFrame.Mode = AddrModeFlat;

    process = GetCurrentProcess();
    thread = GetCurrentThread();

    CG_CrashlogPrintf("Backtrace:\n");

    while (1) {

        more = pfnStackWalk(
            IMAGE_FILE_MACHINE_I386,
            process,
            thread,
            &sf,
            e->ContextRecord,
            NULL,
            pfnSymFunctionTableAccess,
            pfnSymGetModuleBase,
            NULL
            );

        if (!more || sf.AddrFrame.Offset == 0) {
            break;
        }

        dwModBase = pfnSymGetModuleBase(process, sf.AddrPC.Offset);

        if (dwModBase) {
            GetModuleFileName((HINSTANCE)dwModBase, modname, MAX_PATH);
        }
        else {
            //wsprintf(modname, "Unknown");
        }

        pSym->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
        pSym->MaxNameLength = MAX_PATH;

        if (pfnSymGetSymFromAddr(process, sf.AddrPC.Offset, &Disp, pSym))
            CG_CrashlogPrintf("(%d) %s(%s+%#0x) [0x%08x]\n", cnt, modname, pSym->Name, Disp, sf.AddrPC.Offset);
        else
            CG_CrashlogPrintf("(%d) %s [0x%08x]\n", cnt, modname, sf.AddrPC.Offset);

        cnt++;
    }

    GlobalFree(pSym);
}

void G_ShutdownGame(int restart);
LONG CALLBACK win32_exception_handler(LPEXCEPTION_POINTERS e) {
    char basepath[MAX_PATH];
    char gamepath[MAX_PATH];

    //search path for symbols...
    trap_Cvar_VariableStringBuffer("fs_basepath", basepath, sizeof(basepath));
    trap_Cvar_VariableStringBuffer("fs_game", gamepath, sizeof(gamepath));
    pfnSymInitialize(GetCurrentProcess(), va("%s\\%s", basepath, gamepath), TRUE);
    CG_CrashlogPrintf("-8<------- Crash Information ------->8-\n");
    CG_CrashlogPrintf("     Please forward to Skinpack :p     \n");
    CG_CrashlogPrintf("---------------------------------------\n");
    CG_CrashlogPrintf("Build Date: %s\n", __DATE__);
    CG_CrashlogPrintf("Build Time: %s\n", __TIME__);
    win32_exceptioninfo(e);
    win32_dllinfo();
    win32_backtrace(e);
    CG_CrashlogPrintf("-8<--------------------------------->8-\n\n");
    pfnSymCleanup(GetCurrentProcess());
    return 1;
}

void win32_initialize_handler(void) {

    imagehlp = LoadLibrary("IMAGEHLP.DLL");
    if (!imagehlp) {
        CG_Printf("imagehlp.dll unavailable\n");
        return;
    }

    pfnSymInitialize = (PFNSYMINITIALIZE)GetProcAddress(imagehlp, "SymInitialize");
    pfnSymCleanup = (PFNSYMCLEANUP)GetProcAddress(imagehlp, "SymCleanup");
    pfnSymGetModuleBase = (PFNSYMGETMODULEBASE)GetProcAddress(imagehlp, "SymGetModuleBase");
    pfnStackWalk = (PFNSTACKWALK)GetProcAddress(imagehlp, "StackWalk");
    pfnSymGetSymFromAddr = (PFNSYMGETSYMFROMADDR)GetProcAddress(imagehlp, "SymGetSymFromAddr");
    pfnSymEnumerateModules = (PFNSYMENUMERATEMODULES)GetProcAddress(imagehlp, "SymEnumerateModules");
    pfnSymFunctionTableAccess = (PFNSYMFUNCTIONTABLEACCESS)GetProcAddress(imagehlp, "SymFunctionTableAccess");

    if (
        !pfnSymInitialize ||
        !pfnSymCleanup ||
        !pfnSymGetModuleBase ||
        !pfnStackWalk ||
        !pfnSymGetSymFromAddr ||
        !pfnSymEnumerateModules ||
        !pfnSymFunctionTableAccess
        ) {
        FreeLibrary(imagehlp);
        CG_Printf("imagehlp.dll missing exports.\n");
        return;
    }

    // Install exception handler
    SetUnhandledExceptionFilter(win32_exception_handler);
}

void win32_deinitialize_handler(void) {
    // Deinstall exception handler
    SetUnhandledExceptionFilter(NULL);
    pfnSymInitialize = NULL;
    pfnSymCleanup = NULL;
    pfnSymGetModuleBase = NULL;
    pfnStackWalk = NULL;
    pfnSymGetSymFromAddr = NULL;
    pfnSymEnumerateModules = NULL;
    pfnSymFunctionTableAccess = NULL;
    FreeLibrary(imagehlp);
}

void EnableCoreDumps() {

}

void DisableCoreDump() {

}

void EnableStackTrace() {
    win32_initialize_handler();
}

void DisableStackTrace() {
    win32_deinitialize_handler();
}
//[/CrashLog]
