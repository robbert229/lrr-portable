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
/* In the seam (platform/native/seam_stubs.cpp) under names of their own,
 * so that on Windows they do not collide with ADVAPI32's. */
LONG seam_reg_create_key(HKEY key, const char* subkey, HKEY* result);
LONG seam_reg_close_key(HKEY key);
LONG seam_reg_query_value_ex(HKEY key, const char* name, DWORD* reserved,
                             DWORD* type, unsigned char* data, DWORD* size);
#define RegCreateKeyA seam_reg_create_key
#define RegCloseKey seam_reg_close_key
#define RegQueryValueExA seam_reg_query_value_ex
#ifdef __cplusplus
}
#endif
#endif
