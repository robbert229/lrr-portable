// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef LEGORRR_REGISTRY_API_H
#define LEGORRR_REGISTRY_API_H
#ifdef _WIN32
#include <windows.h>
#else
#include <stdint.h>
#if defined(__cplusplus) && defined(LEGORRR_BACKEND_MINIWIN)
#include <miniwin/windows.h>
#else
typedef int HKEY;
typedef uint32_t DWORD;
typedef int32_t LONG;
#endif
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
#endif
