// SPDX-License-Identifier: LGPL-3.0-or-later
/* Win32 entry point — the reference build.
 *
 * Mirrors the shipped binary's GUI-subsystem entry (WinMain, -mwindows). Used
 * to A/B our rebuild against the shipped LegoRR.exe under the A.2 harness
 * (tools/compat/a2.py, docs/adr/0002).
 */
#include "platform/platform.h"
#include "gods98/resource.hpp"
#include "game/port_loop_ports.hpp"
#include <stdio.h>
#include <string.h>

/* Shared game-shell driver — lives in src/main.cpp, backend-neutral. */
int LegorrrMain(void);

/* The original WinMain keeps the executable basename in a process global and
 * uses it as the WAD prefix. Keep this value visible to the Win32 startup
 * while the rest of the original startup is reconstructed. */
static char s_application_prefix[1024];
static DWORD s_startup_no_sound;
static DWORD s_startup_insist_on_cd;

/* WinAPI registry operations used by RegistryPathAccessor (finding 402). */
static void* winmain_registry_create_key(void* parent, const char* component,
                                         void*)
{
    HKEY child = NULL;
    /* RegistryPathCreateKeys ignores RegCreateKeyA's status and tests only
     * whether its output handle is null. Preserve that target condition. */
    (void)RegCreateKeyA((HKEY)parent, component, &child);
    return child;
}

static int winmain_registry_query_value(void* key, const char* value_name,
                                        void* output, uint32_t* size,
                                        uint32_t* value_type, void*)
{
    DWORD native_size;
    DWORD native_type = 0;
    LONG result;
    if (size == NULL) return 0;
    native_size = (DWORD)*size;
    result = RegQueryValueExA((HKEY)key, value_name, NULL, &native_type,
                              (LPBYTE)output, &native_size);
    *size = (uint32_t)native_size;
    if (value_type != NULL) *value_type = (uint32_t)native_type;
    return result == ERROR_SUCCESS;
}

static void winmain_registry_close_key(void* key, void*)
{
    if (key != NULL) RegCloseKey((HKEY)key);
}

static int winmain_registry_read(const char* value_name, char* value,
                                 DWORD value_size)
{
    uint32_t capacity = (uint32_t)value_size;
    const char* key_path = "SOFTWARE\\LEGO Media\\Games\\Rock Raiders";
    if (value == NULL || value_size == 0u) return 0;
    if (!registry_path_accessor(key_path, value_name, 0, value, capacity))
        return 0;
    value[value_size - 1u] = '\0';
    return 1;
}

/* Partial implementation of WinMain's CommandLineOptionParse (finding 398). */
// FUNCTION: LEGORR 0x00477bf0
static void winmain_parse_basic_options(const char* parameters)
{
    s_startup_no_sound = strstr(parameters, "_nosound") != NULL;
    s_startup_insist_on_cd = strstr(parameters, "_insistOnCD") != NULL;
}

// FUNCTION: LEGORR 0x4777a0
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    /* FUN_004777a0 formats "%s_Mutex" with "Lego_Rock_Raiders" and exits
     * when another instance owns that name. */
    char mutex_name[128];
    char no_hal_message[1024];
    char startup_parameters[1024];
    char standard_parameters[1024];
    volatile DWORD startup_state[4] = { 0, 0, 0, 0 };
    volatile float frame_delta = 0.0f;
    registry_path_hooks_set(winmain_registry_create_key,
                            winmain_registry_query_value,
                            winmain_registry_close_key, NULL);
    sprintf(mutex_name, "%s_Mutex", "Lego_Rock_Raiders");
    CreateMutexA(NULL, TRUE, mutex_name);
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        return 0;
    }

    /* The reference takes the executable portion of GetCommandLineA(),
     * removes quotes, uppercases its extension, then removes a .EXE suffix.
     * Its fixed 1024-byte global is mirrored here. */
    const char* command_line = GetCommandLineA();
    const char* basename = command_line;
    const char* cursor = command_line;
    while (*cursor != '\0') {
        if (*cursor == '\\') {
            basename = cursor + 1;
        }
        ++cursor;
    }

    size_t length = 0;
    while (basename[length] != '\0' && basename[length] != '"'
           && length + 1 < sizeof(s_application_prefix)) {
        s_application_prefix[length] = basename[length];
        ++length;
    }
    s_application_prefix[length] = '\0';

    char* last_extension = s_application_prefix;
    char* letter = s_application_prefix;
    while (*letter != '\0') {
        if (*letter == '.') {
            last_extension = letter + 1;
        }
        ++letter;
    }
    if (last_extension != s_application_prefix) {
        letter = last_extension;
        while (*letter != '\0') {
            if (*letter >= 'a' && *letter <= 'z') {
                *letter = (char)(*letter - ('a' - 'A'));
            }
            ++letter;
        }
        char* exe_suffix = strstr(s_application_prefix, ".EXE");
        if (exe_suffix != NULL) {
            *exe_suffix = '\0';
        }
    }

    if (!winmain_registry_read("StandardParameters", standard_parameters,
                               sizeof(standard_parameters))) {
        standard_parameters[0] = '\0';
    }
    if (standard_parameters[0] != '\0') {
        sprintf(startup_parameters, "%s %s", lpCmdLine,
                standard_parameters);
    } else {
        sprintf(startup_parameters, "%s", lpCmdLine);
    }
    winmain_parse_basic_options(startup_parameters);
    startup_state[0] = s_startup_no_sound;
    startup_state[1] = s_startup_insist_on_cd;
    resource_startup_require_cd_set((int)s_startup_insist_on_cd);

    if (!winmain_registry_read("NoHALMessage", no_hal_message,
                               sizeof(no_hal_message))) {
        sprintf(no_hal_message,
                "No DirectX 3D accelerator could be found.");
    }
    resource_set_wad_prefix(s_application_prefix);
    (void)startup_state[2];
    (void)startup_state[3];
    (void)no_hal_message;
    (void)hInstance;
    return LegorrrMain();
}
