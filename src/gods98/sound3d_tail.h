// SPDX-License-Identifier: LGPL-3.0-or-later
/* Header inline copy that closes Sound3D.c's object (finding 693).
 *
 * In the original, LIBC's fwrite.obj comes straight after fseek.obj
 * (Sound3D.c's own reference), ahead of Files.c's remove, although the
 * image's only fwrite call is Files.c's File_Write. As in
 * src/game/advisor_tail.h, an unused `extern __inline` copy carries the
 * reference and /OPT:REF removes its code. Its original name is not known;
 * nothing calls it.
 *
 * GlobalLock and GlobalUnlock are two of the seven ghost KERNEL32 imports
 * the original's IAT order needs from gods98 (finding 694; main_tail.h
 * explains them). The count is forced; the names are placeholders, since the
 * removed code leaves no trace of them. Only the matching build needs
 * them. */
#ifndef LEGORRR_GODS98_SOUND3D_TAIL_H
#define LEGORRR_GODS98_SOUND3D_TAIL_H
#include <stdio.h>
extern __inline size_t sound3d_tail_write(const void* p, size_t n, FILE* f) { return fwrite(p, 1, n, f); }
#if defined(_MSC_VER) && _MSC_VER < 1200
#include <windows.h>
extern __inline LPVOID sound3d_tail_lock(HGLOBAL h) { return GlobalLock(h); }
extern __inline BOOL sound3d_tail_unlock(HGLOBAL h) { return GlobalUnlock(h); }
#endif
#endif
