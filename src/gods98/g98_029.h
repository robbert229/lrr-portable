// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef LEGORRR_GODS98_G98_029_H
#define LEGORRR_GODS98_G98_029_H
#include "registry_api.h"
#ifdef __cplusplus
extern "C" {
#endif
const char* registry_path_split(const char* path, char* out);
int registry_path_accessor(const char* path, const char* name, int mode,
                           void* output, unsigned int size);
int registry_path_create_keys(HKEY root, const char* path, const char* name,
                              int mode, void* output, unsigned int size);
#ifdef __cplusplus
}
#endif
#endif
