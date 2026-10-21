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

/* --- ManagerTick (corpus FUN_0045a730, finding 04) -----------------------
 * The 12 positional trackers (stride 0x30, block DAT_00500110): exponential
 * smoothing of a target into a per-slot smoothed position.
 */
#define SIMTICK_MANAGER_SLOTS 12

typedef struct SimtickManagerSlot {
    int32_t  handle;     /* +0x110 object/sprite handle */
    int32_t  mode;       /* +0x114 0 = reposition, 1 = screen-attach */
    float    target_a[2];/* +0x118/0x11c source A (floats) */
    int32_t  target_b[2];/* +0x120/0x124 source B (ints)     */
    float    smoothed[2];/* +0x128/0x12c exponential-smoothed */
    uint32_t flags;      /* +0x13c (bit 4 selects source B)   */
} SimtickManagerSlot;

void simtick_manager_clear(void);
void simtick_manager_set_slots(SimtickManagerSlot* slots, int count);

/* One tracked slot: target select -> EMA smoothing -> mode routing.
 * `ingame` is the engine-state derived flag (corpus ~DAT_00556ecc >> 0x14). */
void simtick_manager_tick(int index, float dt, int ingame);

/* smoothing model-parameters (corpus _DAT_0049e5e8/e5ec/e5e0/e5d8). */
extern float g_manager_smooth_a;
extern float g_manager_smooth_b;
extern float g_manager_snap;
extern float g_manager_dt_const;

/* actionable hooks (FUN_0047dcb0 reposition, FUN_00484070 attach). Defaults
 * are counters; override to observe. */
void simtick_manager_set_reposition_hook(int (*h)(int handle, int x, int y));
void simtick_manager_set_attach_hook(int (*h)(int handle, int x, int y,
                                              int ingame));
void simtick_manager_set_metrics(int mx, int my); /* FUN_004897d0/004850e0 */
int  simtick_manager_repositions(void);
int  simtick_manager_attaches(void);
int  simtick_manager_timer_bit(int which);        /* index-9 timer word bit */
void simtick_manager_set_timer_word(uint32_t w);  /* DAT_00500860 seed */

/* --- tranche 8 tickers ---------------------------------------------------- */
void simtick_sound_keepalive(EngineContext* c);  /* FUN_0047aba0 */
void simtick_editor_steps(EngineContext* c);     /* FUN_0044cfa0 */
void simtick_cooldown_tick(EngineContext* c, float dt); /* FUN_0041a2c0 -> bus */
void simtick_bitfield_tree(int* base, int code); /* FUN_0049cf90 */

void simtick_set_sound_guards(int sound_on, int active);       /* 0x533878/0x506688 */
void simtick_set_keepalive_hook(int (*h)(int entry, int a, int b)); /* 0x47abc0 */
void simtick_set_editor_state(uint32_t word, int active);      /* 0x4eaec8/0x4eadd8 */
void simtick_set_step_hook(void (*h)(int step));
int  simtick_editor_steps_run(void);
void simtick_set_tree_hook(int (*h)(int idx, int bits));       /* 0x49d000 */
int  simtick_tree_fired(void);

/* --- tranche 9 tickers --------------------------------------------------- */
void simtick_post_step(EngineContext* c);              /* FUN_004665c0 */
void simtick_minion_states(EngineContext* c, float dt, float scale); /* 0x4313b0 */
void simtick_grid_cell_pass(EngineContext* c);         /* FUN_00424600 */
void simtick_scene_pass(EngineContext* c, float dt);   /* FUN_0043b270 */
void simtick_sfx_bank(EngineContext* c, float dt);     /* FUN_0046f550 */

/* hooks / state for the tranche-9 tickers */
int  simtick_post_steps_count(void);
void simtick_set_minion_list(void* list);              /* state +0x88 */
void simtick_set_minion_hooks(int (*dist)(int h, float s),
                              void (*act)(int h),
                              void (*fail)(void* node));
int  simtick_minion_actions(void), simtick_minion_failures(void);
void simtick_cell_seed(int n, const int* xs, const int* ys);
int  simtick_cell_dispatches(void);
uint32_t simtick_scene_word(void);
void simtick_set_scene_word(uint32_t w);
void simtick_set_scene_broadcast(void (*h)(int));
int  simtick_scene_broadcasts(void);
void simtick_sfx_seed(int n, const void** entries);     /* stride 0xac */
int  simtick_sfx_releases(void);

/* bobbins (0x4091e0) — node flags at +0xd0. */
typedef struct SimtickBobbin {
    struct SimtickBobbin* next;  /* +0x24 */
    uint32_t              flags; /* +0xd0 */
} SimtickBobbin;
void simtick_set_bobbin_head(SimtickBobbin* head);
int  simtick_bobbin_advanced(void); /* helper-invocation counter */

#endif /* LEGORRR_SIMTICK_HPP */