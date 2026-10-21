// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — sim tranche 6 subsystem tickers.
 *
 * A first batch of the EngineTick subsystem tickers ported with their
 * observed structure (fresh REA decompiles):
 *
 *   simtick_table_dispatch  FUN_0040aa10  walk a 32-byte-stride table of
 *                                       effect entries (byte +7 bit 0) and
 *                                       hand each live one to a helper.
 *   simtick_post_cluster    FUN_0040d450  the post-step triple
 *                                       (FUN_0040d490 + two FUN_004377b0
 *                                       steps over label regions).
 *   simtick_mute_timer      FUN_0041ba10  reset a 25.0 timer when the
 *                                       mute word changes; when it expires
 *                                       without re-arm, mute (volume 0).
 *   simtick_bobbins         FUN_004091e0  linked-list state machine over a
 *                                       node's +0xd0 flags (1 armed, 2
 *                                       delayed, 4 dead).
 *   simtick_marker_sweep    FUN_0044eff0  when the level block's +0x73d0
 *                                       flag byte has bit 2, walk the grid
 *                                       (+0x8 x +0xc, +0x28 buffer, cell
 *                                       stride 0x28) and clear the per-cell
 *                                       "marker" bit (cell+0x19 bit 4) via a
 *                                       callback, then two render passes.
 *
 * Helpers (FUN_0040aa40 / 0040d490 / 00464ef0 / 00409270…/00450f30 /
 * 00450b60 / 0044f090) stay hook boundaries with corpus addresses; engines
 * of the game layer land them later.
 */
#ifndef LEGORRR_SIMTICK_HPP
#define LEGORRR_SIMTICK_HPP

#include "engine.hpp"

void simtick_table_dispatch(EngineContext* c, float dt, float scale);
void simtick_post_cluster(EngineContext* c);
void simtick_mute_timer(EngineContext* c, float dt);
void simtick_bobbins(EngineContext* c);
void simtick_marker_sweep(EngineContext* c, float dt, float scale);

/* --- test hooks / exposed state ------------------------------------------ */

/* mute timer state (DAT_004de1f4 / 004de1fc / 004a2b58 / 004de200). */
uint32_t simtick_mute_word(void);
void     simtick_set_mute_word(uint32_t v);
int      simtick_mute_engaged(void);
void     simtick_set_mute_hook(int (*hook)(uint32_t id, int vol));

/* table (0x40aa10) — entries are 8 DWORDs; +7 byte bit 0 enables. */
typedef struct SimtickEffect {
    uint32_t words[8];
} SimtickEffect;
void simtick_effect_table(SimtickEffect* table, int count);
int  simtick_effect_last_fired(void);

/* marker sweep (0x44eff0) — cells cleared in the last sweep. */
int  simtick_marker_count(void);

/* bobbins (0x4091e0) — node flags at +0xd0. */
typedef struct SimtickBobbin {
    struct SimtickBobbin* next;  /* +0x24 */
    uint32_t              flags; /* +0xd0 */
} SimtickBobbin;
void simtick_set_bobbin_head(SimtickBobbin* head);
int  simtick_bobbin_advanced(void); /* helper-invocation counter */

#endif /* LEGORRR_SIMTICK_HPP */