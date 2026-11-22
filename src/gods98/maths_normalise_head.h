// SPDX-License-Identifier: LGPL-3.0-or-later
/* The 1.0f that opens a game unit's literal pool in the original: an unused
 * inline copy standing for gods98 Maths.h's Maths_Vector3DNormalise.
 *
 * Most of the original's units start their .rdata with a 1.0f that the
 * unit's own code reads only later, or never (HelpWindow.c 0049e22c,
 * Info.c 0049e25c, Interface.c 0049e274, Lego.c 0049e2b0). The units
 * include Maths.h at the top. Its normalise (`1.0f / modulus`, as
 * camera_maths.h's vec_normalize) is an `extern __inline` function, which
 * MSVC 5.0 compiles although nothing calls it: it emits the copy as a
 * pick-any COMDAT and keeps the literals it creates, and /OPT:REF then
 * removes the code but not the literal (finding 693). The literal is
 * created before any of the unit's own code creates one.
 *
 * As detail_tail.h's copies, this one stands for the original helper: only
 * the literal it creates matters, and nothing calls it. Its body is the
 * smallest that creates the literal, because a definition costs two
 * symbols of MSVC's counter at every function after it (finding 538). A
 * unit includes this file where its first literal would otherwise be
 * created; where the counter matters, the include takes the place of a
 * declaration that cost as much. Only the MSVC 5.0 matching build sees
 * it. */
#ifndef LEGORRR_GODS98_MATHS_NORMALISE_HEAD_H
#define LEGORRR_GODS98_MATHS_NORMALISE_HEAD_H

#if defined(_MSC_VER) && _MSC_VER < 1200
extern __inline float maths_vector3d_normalise_copy(void) { return 1.0f; }
#endif

#endif /* LEGORRR_GODS98_MATHS_NORMALISE_HEAD_H */
