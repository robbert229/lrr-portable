// SPDX-License-Identifier: LGPL-3.0-or-later
/* Native process-local registry. It starts empty; no Windows values exist. */
#ifndef _WIN32
#include <ctype.h>
#include <map>
#include <string>
#include <utility>
#include "registry_api.h"

typedef std::pair<HKEY, std::string> RegistryPath;
static std::map<RegistryPath, HKEY> s_keys;
static HKEY s_next_key = 1;

extern "C" {
LONG RegCreateKeyA(HKEY key, const char* subkey, HKEY* result)
{
    std::string name(subkey);
    std::string::iterator c;
    for (c = name.begin(); c != name.end(); ++c)
        *c = (char)tolower((unsigned char)*c);
    RegistryPath path(key, name);
    std::map<RegistryPath, HKEY>::iterator found = s_keys.find(path);
    if (found == s_keys.end())
        found = s_keys.insert(std::make_pair(path, s_next_key++)).first;
    *result = found->second;
    return 0;
}
LONG RegCloseKey(HKEY key) { (void)key; return 0; }
LONG RegQueryValueExA(HKEY key, const char* name, DWORD* reserved,
                      DWORD* type, unsigned char* data, DWORD* size)
{
    (void)key; (void)name; (void)reserved; (void)type; (void)data; (void)size;
    return 2; /* ERROR_FILE_NOT_FOUND: no native registry values are seeded. */
}
}
#endif
