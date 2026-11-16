// SPDX-License-Identifier: LGPL-3.0-or-later
/* Header inline copies that close Detail.c's object (findings 505 and 693).
 *
 * After the unit's own literals (0049e0d0-0049e0ec: 0.0, 1.0, 0.25, the
 * double 0.25 and 0.01) the original has -2.0f (0049e0ec), the double
 * 1/128 (0049e0f0, 8-aligned: 00000000 3f800000), -1.0f (0049e0f8) and
 * 0.5f (0049e0fc), which nothing reads. They are literals: MSVC 5.0 emits
 * named constants first and literals in the order it creates them, and
 * all four values are new to the unit's pool (read as floats, 0049e0f0
 * would be a second 0.0 and 1.0, which a literal pool cannot hold). No
 * live function of the unit uses them. Unused `extern __inline` copies
 * included after the unit's last function create them in that order;
 * /OPT:REF removes their code but not the literals. These copies stand for
 * such original header helpers: their names and bodies are not known, only
 * the order of their literals matters, and nothing calls them. */
#ifndef LEGORRR_GAME_DETAIL_TAIL_H
#define LEGORRR_GAME_DETAIL_TAIL_H
extern __inline float detail_tail_floor_two(float x) { return x < -2.0f ? -2.0f : x; }
extern __inline double detail_tail_div128(double x) { return x / 128.0; }
extern __inline float detail_tail_floor_one_half(float x) { return x < -1.0f ? -1.0f : x * 0.5f; }
#endif
