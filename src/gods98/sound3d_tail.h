// SPDX-License-Identifier: LGPL-3.0-or-later
/* Header inline copy that closes Sound3D.c's object (finding 693).
 *
 * In the original, LIBC's fwrite.obj comes straight after fseek.obj
 * (Sound3D.c's own reference), ahead of Files.c's remove, although the
 * image's only fwrite call is Files.c's File_Write. As in
 * src/game/advisor_tail.h, an unused `extern __inline` copy carries the
 * reference and /OPT:REF removes its code. Its original name is not known;
 * nothing calls it. */
#ifndef LEGORRR_GODS98_SOUND3D_TAIL_H
#define LEGORRR_GODS98_SOUND3D_TAIL_H
#include <stdio.h>
extern __inline size_t sound3d_tail_write(const void* p, size_t n, FILE* f) { return fwrite(p, 1, n, f); }
#endif
