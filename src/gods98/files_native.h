// SPDX-License-Identifier: LGPL-3.0-or-later
/* PORT-ONLY: the VC5 LIBC directory routines and the three Win32 calls that
 * gods98/files.c makes, for hosts without them (not _WIN32). The Windows
 * builds include <windows.h>, <direct.h> and <io.h> instead. Each name maps
 * to a files_native_* stand-in in gods98/files_port.cpp, so these
 * declarations cannot clash with a host or miniwin declaration of the same
 * name. Only files.c includes this file. */
#ifndef LEGORRR_GODS98_FILES_NATIVE_H
#define LEGORRR_GODS98_FILES_NATIVE_H

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

/* <windows.h> */
#define DRIVE_CDROM 5
#define MB_OK 0x0u
#define MB_OKCANCEL 0x1u
#define IDCANCEL 2
unsigned int files_native_get_drive_type(const char* rootPathName);
int files_native_message_box(void* window, const char* text,
                             const char* caption, unsigned int type);
void files_native_output_debug_string(const char* text);
#define GetDriveTypeA files_native_get_drive_type
#define MessageBoxA files_native_message_box
#define OutputDebugStringA files_native_output_debug_string

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_FILES_NATIVE_H */
