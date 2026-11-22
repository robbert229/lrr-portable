// SPDX-License-Identifier: LGPL-3.0-or-later
/* A header inline copy whose literals open GAME-009's .rdata and make up
 * GAME-010's (finding 693 for unused inline copies).
 *
 * MSVC 5.0 creates a function's float literals when it compiles the
 * function and emits them in that order. An unused `extern __inline`
 * function is compiled where it is defined, so its literals come before
 * those of the functions that follow, although its code is emitted after
 * the object's own functions and /OPT:REF removes it. Immediate stores and
 * pushes of a constant create no literal.
 *
 * - GAME-009 (0049e0a8-0049e0c8): 1.0, 0.0, then the unit's own 1/15, 2.0,
 *   -1.0, 3.0, 0.5 and 15.0. Its first use of 0.0 and 1.0 is the uvs
 *   initializer of damage_font_mesh_render_callback, { 0, 0, 1, ... },
 *   which creates 0.0 first; the original's 1.0 comes first, so something
 *   compiled earlier created 1.0 and then 0.0.
 * - GAME-010 (0049e0c8-0049e0d0): 1.0 and 0.0, which nothing reads. The
 *   unit has no float code of its own.
 * - GAME-012 (0049e104-0049e110): 1.0 and 0.0, which its code reads, then
 *   12.5; its code creates 0.0 before 1.0.
 * - GAME-013 (0049e110-0049e120): 1.0, which nothing reads, then the 0.0,
 *   200 and 0.5 its code reads.
 *
 * This helper stands for that header function: its name, header and body
 * are not known; only the order of its two literals matters, and nothing
 * calls it. */
#ifndef LEGORRR_GAME_SATURATE_INLINE_H
#define LEGORRR_GAME_SATURATE_INLINE_H
extern __inline float header_inline_saturate(float x)
{
    if (x > 1.0f) return 1.0f;
    if (x < 0.0f) return 0.0f;
    return x;
}
#endif
