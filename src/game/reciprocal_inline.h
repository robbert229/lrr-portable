// SPDX-License-Identifier: LGPL-3.0-or-later
/* A header inline copy whose one literal, 1.0, is a word of .rdata that
 * nothing reads (findings 505 and 693).
 *
 * Many original units' .rdata ends with an unreferenced 1.0. MSVC 5.0
 * creates a float literal when it compiles a function that loads it (or an
 * aggregate initializer that holds it) and pools it once per object;
 * immediate stores and pushes create none. An unused `extern __inline`
 * function is compiled where it is defined and its literals go to the
 * object's .rdata, while /OPT:REF removes its code, which is emitted after
 * the object's own functions. So in a unit whose code creates no literal
 * 1.0, this copy adds one, after the literals created before it.
 *
 * Its includers and their original words:
 * - GAME-004 (animation_object.c): 0049e05c, after the named 0.0 and 1.0;
 * - GAME-008 (entity_cell_effects.c): 0049e0a4, after its literal 0.0;
 * - GAME-014 (encyclopedia.c, no float code): 0049e120, its whole .rdata;
 * - GAME-015 (erode.c): 0049e134, after its literals 25, 0.2, 0.0 and 4.
 *
 * Where a literal precedes the 1.0, the include follows the unit's last
 * function. The helper stands for an original header function: its name,
 * header and body are not known, only its 1.0 matters, and nothing calls
 * it. */
#ifndef LEGORRR_GAME_RECIPROCAL_INLINE_H
#define LEGORRR_GAME_RECIPROCAL_INLINE_H
extern __inline float header_inline_reciprocal(float x) { return 1.0f / x; }
#endif
