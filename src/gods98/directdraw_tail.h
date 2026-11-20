// SPDX-License-Identifier: LGPL-3.0-or-later
/* Header inline copy that closes DirectDraw.c's object (finding 694).
 *
 * A ghost DDRAW import: a member whose only reference is code /OPT:REF
 * removed. Its slot takes part in LINK's qsort of the import table and is
 * then dropped. The original's IAT order needs one after DirectDraw.c's
 * DirectDrawCreate. Which function the removed code called is not known;
 * DirectDrawCreateClipper is a placeholder.
 *
 * As in sound3d_tail.h (finding 693), an unused `extern __inline` copy
 * carries the reference and /OPT:REF removes its code. Nothing calls it.
 * Only the matching build needs it. */
#ifndef LEGORRR_GODS98_DIRECTDRAW_TAIL_H
#define LEGORRR_GODS98_DIRECTDRAW_TAIL_H
#if defined(_MSC_VER) && _MSC_VER < 1200
#include <windows.h>
HRESULT WINAPI DirectDrawCreateClipper(DWORD flags, void** clipper, void* outer);
extern __inline HRESULT directdraw_tail_clipper(void** clipper)
{
    return DirectDrawCreateClipper(0, clipper, NULL);
}
#endif
#endif
