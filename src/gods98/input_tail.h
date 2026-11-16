// SPDX-License-Identifier: LGPL-3.0-or-later
/* Header inline copy that closes Input.c's object (finding 694).
 *
 * GetKeyState is a ghost import: a USER32 member whose only reference is
 * code /OPT:REF removed. Its slot takes part in LINK's qsort and is then
 * dropped (finding 694). The original's IAT order forces one such USER32
 * member after Main.c's CreateWindowExA and before SoundSys.c's wsprintfA;
 * the name is a placeholder, since the removed code leaves no trace of it.
 *
 * As in sound3d_tail.h (finding 693), an unused `extern __inline` copy
 * carries the reference and /OPT:REF removes its code. Nothing calls it.
 * Only the matching build needs it. */
#ifndef LEGORRR_GODS98_INPUT_TAIL_H
#define LEGORRR_GODS98_INPUT_TAIL_H
#if defined(_MSC_VER) && _MSC_VER < 1200
#include <windows.h>
extern __inline SHORT input_tail_key_state(int key) { return GetKeyState(key); }
#endif
#endif
