// SPDX-License-Identifier: LGPL-3.0-or-later
#include <assert.h>
#include <string.h>
#include "registry_api.h"
#include "g98_029.h"

static int depth, closes, queries;
static int fail_create;
static char components[2][100];
extern "C" LONG RegCreateKeyA(HKEY key, const char* name, HKEY* result)
{
    (void)key;
    if (fail_create) { *result = 0; return 5; }
    strcpy(components[depth], name);
    *result = ++depth;
    return 0;
}
extern "C" LONG RegCloseKey(HKEY key)
{
    assert(key == depth--);
    ++closes;
    return 0;
}
extern "C" LONG RegQueryValueExA(HKEY key, const char* name, DWORD*,
                                  DWORD*, unsigned char* data, DWORD* size)
{
    assert(key == 2 && strcmp(name, "Value") == 0 && *size == 8);
    ++queries;
    memcpy(data, "ok", 3);
    return 0;
}
int main()
{
    char out[8];
    assert(registry_path_accessor("A\\B", "Value", 0, out, sizeof(out)));
    assert(strcmp(out, "ok") == 0 && strcmp(components[0], "A") == 0);
    assert(strcmp(components[1], "B") == 0 && closes == 2 && queries == 1 && depth == 0);
    assert(!registry_path_accessor("A\\B", "Value", 2, out, sizeof(out)));
    assert(closes == 4 && queries == 1 && depth == 0);
    fail_create = 1;
    assert(!registry_path_accessor("A", "Value", 1, out, sizeof(out)));
    assert(closes == 4 && queries == 1);
    return 0;
}
