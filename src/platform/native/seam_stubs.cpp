// SPDX-License-Identifier: LGPL-3.0-or-later
/* The seam's imports that neither miniwin nor a library substitution
 * provides yet (ADR 0007): the Win32 and DirectX calls outside miniwin's
 * scope that the units make. Each stub logs its name once and returns the
 * failure its API documents, which the units already handle; none aborts.
 * The library substitutions of ADR 0007 replace these calls in the units:
 *
 *   ADVAPI32 registry             -> [library:config]
 *
 * The LIBC directory routines and KERNEL32 calls that gods98/files.c makes
 * live with the rest of the filesystem substitution in files_native.cpp.
 *
 * A few calls are small enough to work here already (GlobalAlloc,
 * GlobalFree, wsprintfA, GetLastError and GetCommandLineA, whose string
 * the process entry builds); they are marked as working. Each is defined
 * under a seam_ name that the units' headers map the Win32 name to
 * (gods98/winmm.h, gods98/registry_api.h). */
#include <SDL3/SDL.h>

#include <miniwin/windows.h>

#include "gods98/registry_api.h"
#include "gods98/winmm.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
#include <string>

namespace {

/* Logs the stub's name the first time it is called. */
void SeamStub(const char* name)
{
    static std::set<std::string> logged;
    if (logged.insert(name).second) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "seam stub: %s", name);
    }
}

#define SEAM_STUB() SeamStub(__func__)

DWORD g_lastError = 0;

} // namespace

extern "C" {

/* --- KERNEL32 --------------------------------------------------------- */

/* The process's command line, which entry.cpp builds from argv. */
char seam_default_command_line[] = "\"LegoRR.exe\"";
char* seam_command_line = seam_default_command_line;

/* Working. */
LPSTR seam_get_command_line(void)
{
    return seam_command_line;
}

/* Stub: the single-instance mutex. No other instance is ever found. */
HANDLE seam_create_mutex(void* attributes, BOOL initialOwner, LPCSTR name)
{
    SEAM_STUB();
    g_lastError = 0;
    static int mutex;
    return &mutex;
}

/* Working. */
DWORD seam_get_last_error(void)
{
    return g_lastError;
}

/* Working: GMEM_FIXED memory is plain heap memory. */
HGLOBAL seam_global_alloc(UINT flags, size_t bytes)
{
    return calloc(1, bytes ? bytes : 1);
}

/* Working. */
HGLOBAL seam_global_free(HGLOBAL memory)
{
    free(memory);
    return NULL;
}

/* Working: USER32's wsprintfA writes at most 1024 characters. */
int seam_wsprintf(LPSTR output, LPCSTR format, ...)
{
    va_list args;
    va_start(args, format);
    int written = vsnprintf(output, 1025, format, args);
    va_end(args);
    return written;
}

/* --- ADVAPI32 ([library:config]) --------------------------------------- */

/* Stub: an empty registry, so every value takes its default. */
LONG seam_reg_create_key(HKEY key, const char* subkey, HKEY* result)
{
    SEAM_STUB();
    if (result) *result = 0;
    return 2; /* ERROR_FILE_NOT_FOUND */
}

LONG seam_reg_close_key(HKEY key)
{
    SEAM_STUB();
    return 0;
}

LONG seam_reg_query_value_ex(HKEY key, const char* name, DWORD* reserved,
                             DWORD* type, unsigned char* data, DWORD* size)
{
    SEAM_STUB();
    return 2; /* ERROR_FILE_NOT_FOUND */
}

} /* extern "C" */
