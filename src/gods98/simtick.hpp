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
/* Effect-entry seams: update each indexed item and release the effect owner. */
void simtick_effect_set_hooks(void (*set_opacity)(uint32_t handle, int index,
                                                   float opacity, int mode),
                              void (*release)(uint32_t handle, int mode));

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
typedef void (*SimtickPostStepHook)(EngineContext* context, void* user);
void simtick_post_step_hook_set(SimtickPostStepHook hook, void* user);
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

/* --- tranche 10: the remaining table tickers ----------------------------- */
void simtick_mouse_world(EngineContext* c, float dt);      /* FUN_00425d90 */
void simtick_frame_counter(EngineContext* c);              /* FUN_00402220 */
void simtick_ambient_spawn(EngineContext* c, float dt);    /* FUN_0040eab0 */
void simtick_sound_bank_tick(EngineContext* c, float dt);  /* FUN_00465b00 */
void simtick_render_pass(EngineContext* c, float dt, float scale,
                         uint32_t o1);                     /* FUN_004291a0 */
void simtick_render_step(EngineContext* c);                /* FUN_00449c00 */
void simtick_vu_meter(EngineContext* c, float dt);         /* FUN_00426520 */
void simtick_fx_bank_tick(EngineContext* c, float dt);     /* FUN_0046e390 */
typedef struct SimtickFxSubEntry {
    float value;
    float fade_primary;
    float fade_secondary;
    float enabled;
    float padding[2];
} SimtickFxSubEntry;
typedef struct SimtickFxEntry {
    uint32_t count;
    float initial;
    float current;
    uint32_t resource;
    uint32_t flags;
    uint32_t resource_count;
    SimtickFxSubEntry* subentries;
} SimtickFxEntry;
typedef void (*SimtickFxResourceUpdateFn)(uint32_t resource, uint32_t index,
                                          float value, void* user);
void simtick_fx_bank_entries_install(SimtickFxEntry* entries, uint32_t count,
                                     SimtickFxResourceUpdateFn update,
                                     void* user);

/* hooks / observable state for tranche-10 tickers */
void simtick_set_pick_hook(int (*h)(int cx, int cy));
int  simtick_mouse_picks(void);
void simtick_set_spawn_hook(int (*h)(int slot));
int  simtick_spawn_calls(void);
uint32_t simtick_frame(void);
int  simtick_frame_bump(void);
int  simtick_render_steps(void);

/* bobbins (0x4091e0) — node flags at +0xd0. */
typedef struct SimtickBobbin {
    unsigned char         reserved00[0x08];
    uint32_t              key;          /* +0x08: single-key lookup */
    uint32_t              key_pair[2];  /* +0x0c/+0x10: coordinate key */
    void*                 owned_buffer; /* +0x14: released with record */
    unsigned char         reserved18[0x0c];
    struct SimtickBobbin* next;       /* +0x24 */
    void*                 members[20]; /* +0x2c .. +0xcb */
    uint32_t              member_count; /* +0xcc */
    uint32_t              flags;        /* +0xd0 */
} SimtickBobbin;
typedef void (*SimtickBobbinMarkHook)(SimtickBobbin* node, void* context);
typedef int (*SimtickBobbinSpawnHook)(SimtickBobbin* node, void* context);
typedef void (*SimtickBobbinActionHook)(SimtickBobbin* node, void* context);
void simtick_set_bobbin_head(SimtickBobbin* head);
void simtick_bobbin_mark_hook_set(SimtickBobbinMarkHook hook, void* context);
void simtick_bobbin_action_hooks_set(SimtickBobbinSpawnHook spawn,
    SimtickBobbinActionHook clear_delay, SimtickBobbinActionHook die,
    void* context);
int  simtick_bobbin_advanced(void); /* helper-invocation counter */
void simtick_selection_group_release(const uint32_t key_pair[2]); /* FUN_00409110 */
void simtick_selection_group_clear(const uint32_t key_pair[2]);   /* FUN_00409150 */

/* SimulationGlobalWordSet (FUN_0041ba90, finding 474): the corpus assigns
 * its argument to the global DAT_004de1f4 — the mute word the simtick
 * models. NULL-free by shape: this is a blind global store. */
void simulation_global_word_set(uint32_t value);

/* DirectSoundListenerViewSet (FUN_0047ab80): the view provider stores the
 * current listener object in the DAT_00506688 slot (this module's sound
 * guard) only while the sound system is up. The keep-alive ticker reads
 * the slot; the provider call feeds it exactly where the corpus does. */
void simtick_directsound_listener_view_set(int view); /* FUN_0047ab80 */

/* RoutePositionBlockedStateSet (FUN_0044da20): a blocked route sets the
 * scene-transform word's blocking bit 9 and drops bit 10; an unblocked
 * route clears bits 9 and 12 and keeps bit 10 (the corpus's 0xffffedff
 * mask, faithful).
 * GlobalUIFlagsClearBits12 (FUN_0045b610): clears bits 1 and 2 of the
 * index-9 timer word (the corpus's DAT_00500860 — the same word the
 * shared-timer dispatch reads). The peek hooks stand in for the corpus's
 * readers of both words. */
void simtick_route_position_blocked_state_set(int blocked);
                                        /* FUN_0044da20 */
void simtick_global_ui_flags_clear_bits12(void); /* FUN_0045b610 */
uint32_t simtick_route_position_state_peek(void);
uint32_t simtick_global_ui_flags_word(void);

/* ScenePositionResetAndEffectMotionCancel (FUN_0044cf70): copies the current
 * owner transform onto the configured position triple and clears the scene-
 * transform word's bit 7 (the motion-cancel gate the drift/jitter tickers
 * read). The corpus feeds both triples from the unported transform-state
 * initializer, so they land as module statics with hooks — deviation: the
 * corpus reads the owner pose through the scene-object vtable seam, modeled
 * here as the current triple. */
void simtick_scene_position_reset_and_effect_motion_cancel(void);
                                     /* FUN_0044cf70 */
void         simtick_scene_transform_current_set(const float xyz[3]);
const float* simtick_scene_transform_config_peek(void);

/* VectorWithinAxisTolerance (FUN_0044d9a0): every axis of the vector must
 * sit inside the tolerance triple's window about the config triple. The
 * corpus's tolerance words (DAT_004eae80/84/88 role) land as own storage —
 * their corpus writer is the recorded light-effects parameter band
 * (deviation); the install hook stands in. */
int simtick_vector_within_axis_tolerance(const float xyz[3]);
void simtick_vector_tolerance_install(const float xyz[3]); /* hook */
#endif /* LEGORRR_SIMTICK_HPP */
