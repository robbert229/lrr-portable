// SPDX-License-Identifier: LGPL-3.0-or-later
/* Header inline copies that close Files.c's object (finding 694).
 *
 * - CloseHandle: the original's hint/name strings put it straight after
 *   GetDriveTypeA among gods98's KERNEL32 imports, before LIBC's. No live
 *   gods98 code calls it, so removed gods98 code referenced it.
 * - _rmdir and rename: ghost LIBC members. They pull KERNEL32's
 *   RemoveDirectoryA and MoveFileA after getcwd.obj's GetFullPathNameA,
 *   two of the ghost import members the original's IAT order needs in
 *   LIBC's KERNEL32 run. Files.c already wraps remove, _chmod, _mkdir,
 *   _findfirst and _getcwd; neither member pulls anything new.
 * - GetVolumeInformationA and GetFileSize: two of the seven ghost KERNEL32
 *   imports the original's IAT order needs from gods98 (see main_tail.h).
 *   The count is forced; the names are placeholders, since the removed code
 *   leaves no trace of them.
 *
 * As in sound3d_tail.h (finding 693), unused `extern __inline` copies carry
 * the references and /OPT:REF removes their code. Nothing calls them. Only
 * the matching build needs them. */
#ifndef LEGORRR_GODS98_FILES_TAIL_H
#define LEGORRR_GODS98_FILES_TAIL_H
#if defined(_MSC_VER) && _MSC_VER < 1200
#include <stdio.h>
#include <direct.h>
#include <windows.h>
extern __inline BOOL files_tail_close(HANDLE h) { return CloseHandle(h); }
extern __inline int files_tail_remove_dir(const char* path) { return _rmdir(path); }
extern __inline int files_tail_rename(const char* from, const char* to) { return rename(from, to); }
extern __inline BOOL files_tail_volume(const char* root, char* name, DWORD size)
{
    return GetVolumeInformationA(root, name, size, NULL, NULL, NULL, NULL, 0);
}
extern __inline DWORD files_tail_size(HANDLE h) { return GetFileSize(h, NULL); }
#endif
#endif
