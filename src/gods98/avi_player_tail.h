// SPDX-License-Identifier: LGPL-3.0-or-later
/* Header inline copy that closes the AVI player's object (finding 694).
 *
 * A ghost AVIFIL32 import: a member whose only reference is code /OPT:REF
 * removed. Its slot takes part in LINK's qsort of the import table and is
 * then dropped. The original's IAT order needs one straight after
 * AVIFileInit. The unit calls AVIFileInit and never AVIFileExit, so a
 * removed shutdown is the likely carrier; the name is not evidenced.
 *
 * As in sound3d_tail.h (finding 693), an unused `extern __inline` copy
 * carries the reference and /OPT:REF removes its code. Nothing calls it.
 * Only the matching build needs it. */
#ifndef LEGORRR_GODS98_AVI_PLAYER_TAIL_H
#define LEGORRR_GODS98_AVI_PLAYER_TAIL_H
#if defined(_MSC_VER) && _MSC_VER < 1200
#ifdef __cplusplus
extern "C" {
#endif
void __stdcall AVIFileExit(void);
#ifdef __cplusplus
}
#endif
extern __inline void avi_player_tail_shutdown(void) { AVIFileExit(); }
#endif
#endif
