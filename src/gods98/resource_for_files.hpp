// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what files uses from resource.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GODS98_RESOURCE_FOR_FILES_HPP
#define LRR_GODS98_RESOURCE_FOR_FILES_HPP

#include "resource_for_wad.hpp"

/* CD/STD fallback state (finding 01). */
#ifdef LEGORRR_BACKEND_WINAPI
#  define RES_PATH_SEP '\\' /* the corpus's "%c:\%s\%s" uses DOS separators */
#else
#  define RES_PATH_SEP '/'  /* host-native on the POSIX port */
#endif

typedef struct ResourceDirectoryScanHandle {
#if defined(_WIN32)
    HANDLE search;
    WIN32_FIND_DATAA data;
    int first;
#else
    DIR* directory;
    char root[1024];
#endif
} ResourceDirectoryScanHandle;

int wad_entry_slot_allocate(void);
extern char s_wad_prefix[RESOURCE_PATH_MAX];
extern char s_resource_directory_root[RESOURCE_PATH_MAX];
extern char s_cd_drive;
extern char s_cd_data_dir[RESOURCE_PATH_MAX];
extern char s_cd_root[RESOURCE_PATH_MAX];
extern ResourceDriveTypeQueryFn s_cd_drive_type_query;
extern ResourcePathExistsFn s_cd_path_exists;
extern void* s_cd_probe_context;
extern ResourceStartupOps s_resource_startup_ops;
extern int s_resource_startup_result;
const char* ares_cd_path(const char* name, char* out, size_t out_sz);
const char* ares_cd_root_path(const char* name, char* out, size_t out_sz);
int resource_file_token_list_parse(const char* token_file, ResourceFileTokenList* list, int clear_before_open);
int resource_directory_scan_default_open(void*, const char* pattern, void** scan_out);
int resource_directory_scan_default_next(void*, void* handle, char name_out[260], int* is_directory_out);
void resource_directory_scan_default_close(void*, void* handle);

#endif /* LRR_GODS98_RESOURCE_FOR_FILES_HPP */
