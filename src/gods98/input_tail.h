// SPDX-License-Identifier: LGPL-3.0-or-later
/* Header inline copies that close Input.c's object (finding 694).
 *
 * The original's hint/name strings show gods98's KERNEL32 pull order as
 * ... GlobalFree GlobalAlloc OutputDebugStringA GetDriveTypeA CloseHandle.
 * Files.c references GetDriveTypeA (File_FindDataCD) before
 * OutputDebugStringA (File_Error), so an earlier gods98 object referenced
 * OutputDebugStringA first. Which one is not known; this copy makes Input.c,
 * the object before Files.c, that reference.
 *
 * GetKeyState is a ghost import: a USER32 member whose only reference is
 * code /OPT:REF removed. Its slot takes part in LINK's qsort and is then
 * dropped (finding 694). The original's IAT order forces one such USER32
 * member after Main.c's CreateWindowExA and before SoundSys.c's wsprintfA;
 * the name is a placeholder, since the removed code leaves no trace of it.
 *
 * As in sound3d_tail.h (finding 693), unused `extern __inline` copies carry
 * the references and /OPT:REF removes their code. Nothing calls them. Only
 * the matching build needs them. */
#ifndef LEGORRR_GODS98_INPUT_TAIL_H
#define LEGORRR_GODS98_INPUT_TAIL_H
#if defined(_MSC_VER) && _MSC_VER < 1200
#include <windows.h>
extern __inline void input_tail_debug(const char* s) { OutputDebugStringA(s); }
extern __inline SHORT input_tail_key_state(int key) { return GetKeyState(key); }
#endif
#endif
