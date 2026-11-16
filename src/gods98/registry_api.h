// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef LEGORRR_REGISTRY_API_H
#define LEGORRR_REGISTRY_API_H
#include <stdint.h>
#include <miniwin/windows.h>
#ifndef HKEY_LOCAL_MACHINE
#define HKEY_LOCAL_MACHINE ((HKEY)0x80000002u)
#endif
#ifdef __cplusplus
extern "C" {
#endif
LONG RegCreateKeyA(HKEY key, const char* subkey, HKEY* result);
LONG RegCloseKey(HKEY key);
LONG RegQueryValueExA(HKEY key, const char* name, DWORD* reserved,
                      DWORD* type, unsigned char* data, DWORD* size);
#ifdef __cplusplus
}
#endif
#endif
