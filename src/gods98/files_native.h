// SPDX-License-Identifier: LGPL-3.0-or-later
/* The filesystem substitution (ADR 0007, [library:filesystem]): the VC5
 * LIBC directory routines and KERNEL32 calls outside miniwin's scope that
 * the units make, the one place a unit's Windows path becomes a host path,
 * and the save directory. platform/native/files_native.cpp defines them.
 *
 * Each LIBC and KERNEL32 name maps to a files_native_* function, so these
 * declarations cannot clash with a host declaration of the same name.
 * files.c includes this file, and the units that open files outside the
 * file layer (the video and sound units) for the host-path calls.
 *
 * Path handling follows isle-portable's MxString::MapPathToFilesystem: a
 * unit builds Windows paths ('\\' separators, names whose case need not
 * match the files on disk), and every file call maps the path first. */
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
/* Working: the entries of one directory that match a Windows wildcard
 * ("Data\\*.*"), through SDL_GlobDirectory. */
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

/* [library:filesystem]: the host path of a Windows path ("Data\\Avi\\x.flh",
 * or an absolute path from _fullpath): the separators become the host's,
 * and a component missing as spelled takes the spelling of an entry that
 * matches it without regard to case, as on the original's file system.
 * NULL when `path` is NULL or the result does not fit. */
char* files_native_host_path(char* hostPath, const char* path,
                             unsigned int maxLength);
/* [library:filesystem]: fopen of the host path of a Windows path, for the
 * units that open files outside the file layer. Needs <stdio.h>. */
FILE* files_native_fopen(const char* path, const char* mode);

/* [library:filesystem]: the save directory. The original keeps its saves in
 * the data directory's "Saves" (FrontEnd.c's "Saves\\%i.sav" and
 * "Saves\\%i.osf", Menu::SaveImage::Path's "%s\\%d.dat"). As in
 * isle-portable, the port can keep them elsewhere, in the directory the
 * configuration's savepath names (platform/native/config.cpp); files.c
 * maps a name under "Saves" there. NULL or "" keeps the original's
 * Data\\Saves. */
void files_native_set_save_path(const char* path);
const char* files_native_save_path(void);

/* [library:config]: the data directory. The original finds its data in the
 * working directory: File_Initialise mounts <program>0.wad to
 * <program>9.wad there and takes its "Data" directory as the base. As
 * isle-portable's isle:diskpath, the port can take them from the directory
 * the configuration's diskpath names (platform/native/config.cpp); then a
 * relative Windows path maps into it (files_native_host_path), and
 * File_Initialise starts from it. NULL or "" keeps the working directory. */
void files_native_set_disk_path(const char* path);
const char* files_native_disk_path(void);

/* [library:filesystem]: the host shell's save notifications, as
 * isle-portable's e_saveSlotWritten and e_saveStateChanged game events: a
 * slot's progress was written, or the set of slots changed (a slot was
 * created or reset). The web build forwards them to the page; elsewhere
 * they only log. */
void files_native_save_slot_written(int slot);
void files_native_save_state_changed(void);

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
