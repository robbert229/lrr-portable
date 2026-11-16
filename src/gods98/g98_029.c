// SPDX-License-Identifier: LGPL-3.0-or-later
/* G98-029: registry path traversal and value query (finding 505). */
#include "g98_029.h"

/* A word holding 16 that nothing in the image reads; the original's .data
 * has it here, after DXError.c's literals and before Wad.c's. */
// GLOBAL: LEGORR 0x004af9e4
static int s_registryWord = 16;

// FUNCTION: LEGORR 0x0048b330
const char* registry_path_split(const char* path, char* out)
{
    char c;
    *out = '\0';
    c = *path;
    while (c != '\0' && c != '\\') {
        path++;
        *out++ = c;
        c = *path;
    }
    if (*path == '\\') path++;
    *out = '\0';
    return path;
}

// FUNCTION: LEGORR 0x0048b360
int registry_path_accessor(const char* path, const char* name, int mode,
                           void* output, unsigned int size)
{
    return registry_path_create_keys(HKEY_LOCAL_MACHINE, path, name, mode,
                                     output, size);
}

// FUNCTION: LEGORR 0x0048b390
int registry_path_create_keys(HKEY root, const char* path, const char* name,
                              int mode, void* output, unsigned int size)
{
    HKEY child;
    char component[100];
    int result;
    DWORD type;
    path = registry_path_split(path, component);
    if (component[0] == '\0') {
        type = 0;
#define REGISTRY_TYPE (&type)
        switch (mode) {
        case 0:
            return RegQueryValueExA(root, name, 0, REGISTRY_TYPE, (unsigned char*)output, (DWORD*)&size) == 0;
        case 1:
            return RegQueryValueExA(root, name, 0, REGISTRY_TYPE, (unsigned char*)output, (DWORD*)&size) == 0;
        default:
            return 0;
        }
    }
#undef REGISTRY_TYPE
    RegCreateKeyA(root, component, &child);
    if (child == 0) return 0;
    result = registry_path_create_keys(child, path, name, mode, output, size);
    RegCloseKey(child);
    return result;
}
