// SPDX-License-Identifier: LGPL-3.0-or-later
/* Header inline copies that close Main.c's object (finding 694).
 *
 * Ghost KERNEL32 imports: members whose only references are code /OPT:REF
 * removed. Their slots take part in LINK's qsort of the import table and are
 * then dropped, so they move the visible slots. The original's IAT order
 * needs seven of them among gods98's KERNEL32 references, before LIBC's
 * crt0dat.obj (GetCurrentProcess). The count and that range are forced;
 * the names are placeholders, since the removed code leaves no trace of
 * them. This header carries three; files_tail.h and sound3d_tail.h carry
 * the rest.
 *
 * As in sound3d_tail.h (finding 693), unused `extern __inline` copies carry
 * the references and /OPT:REF removes their code. Nothing calls them. Only
 * the matching build needs them. */
#ifndef LEGORRR_GODS98_MAIN_TAIL_H
#define LEGORRR_GODS98_MAIN_TAIL_H
#if defined(_MSC_VER) && _MSC_VER < 1200
#include <windows.h>
extern __inline DWORD main_tail_ticks(void) { return GetTickCount(); }
extern __inline BOOL main_tail_counter(LARGE_INTEGER* c) { return QueryPerformanceCounter(c); }
extern __inline BOOL main_tail_frequency(LARGE_INTEGER* f) { return QueryPerformanceFrequency(f); }
#endif
#endif
