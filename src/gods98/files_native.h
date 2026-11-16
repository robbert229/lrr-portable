// SPDX-License-Identifier: LGPL-3.0-or-later
/* The VC5 LIBC directory routines and the two KERNEL32 calls outside
 * miniwin's scope that gods98/files.c makes. Each name maps to a
 * files_native_* function in platform/native/seam_stubs.cpp, so these
 * declarations cannot clash with a host declaration of the same name, until
 * the filesystem substitution of ADR 0007 replaces the calls in files.c.
 * files.c includes this file, and movie_unit.cpp for files_native_fopen. */
#ifndef LEGORRR_GODS98_FILES_NATIVE_H
#define LEGORRR_GODS98_FILES_NATIVE_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* <io.h> */
struct files_native_finddata {
    unsigned int attrib;
    long time_create;
    long time_access;
    long time_write;
    unsigned long size;
    char name[260];
};
#define _finddata_t files_native_finddata
#define _A_SUBDIR 0x10
long files_native_findfirst(const char* filespec,
                            struct files_native_finddata* fileinfo);
int files_native_findnext(long handle, struct files_native_finddata* fileinfo);
int files_native_findclose(long handle);
int files_native_chmod(const char* path, int mode);
#define _findfirst files_native_findfirst
#define _findnext files_native_findnext
#define _findclose files_native_findclose
#define _chmod files_native_chmod

/* <direct.h> and <stdlib.h> */
char* files_native_getcwd(char* buffer, int maxlen);
int files_native_mkdir(const char* path);
char* files_native_fullpath(char* absPath, const char* relPath,
                            unsigned int maxLength);
#define _getcwd files_native_getcwd
#define _mkdir files_native_mkdir
#define _fullpath files_native_fullpath

/* [library:filesystem]: fopen of a Windows path ("Data\\Avi\\x.flh", or
 * an absolute path from _fullpath) on the host: the separators become the
 * host's, and a component missing as spelled is matched without regard to
 * case, as on the original's file system. Needs <stdio.h>. */
FILE* files_native_fopen(const char* path, const char* mode);

/* KERNEL32 */
#define DRIVE_CDROM 5
unsigned int files_native_get_drive_type(const char* rootPathName);
void files_native_output_debug_string(const char* text);
#define GetDriveTypeA files_native_get_drive_type
#define OutputDebugStringA files_native_output_debug_string

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_FILES_NATIVE_H */
