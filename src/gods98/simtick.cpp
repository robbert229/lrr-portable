// SPDX-License-Identifier: LGPL-3.0-or-later
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "message.hpp"
#include "simtick.hpp"
#include "sound.hpp"
#include "heap.hpp"
#include "crt/crt.hpp"
#include "resource.hpp"
#include "game/level.hpp"
#include "game/ai.hpp"
#include "game/cmd.hpp"
#include "game/select.hpp"
#include "game/entity.hpp"
#include "game/effects.hpp"
#include "game/render.hpp"
#include "game/terrain_grid.hpp"
#include "view.hpp"
#include "simtick_shared.hpp"
#include "simtick_for_panel_unit.hpp"
#include "simtick_from_panel_unit.hpp"
#include "simtick_for_lego.hpp"
#include "simtick_for_weapons.hpp"
#include "simtick_for_sound3d.hpp"
#include "simtick_for_interface.hpp"
#include "simtick_from_interface.hpp"
#include "simtick_for_legoobject.hpp"
#include "simtick_for_aitask.hpp"

int      (*s_mute_hook)(uint32_t id, int vol) = NULL; /* FUN_00464ef0 */

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
uint32_t simtick_mute_word(void)      { return s_mute_word; }
int      simtick_mute_engaged(void)   { return s_mute_engaged; }
#endif /* !LEGORRR_BACKEND_WINAPI */

void simtick_set_mute_word(uint32_t v)
{
    s_mute_word = v;
}

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
void simtick_set_mute_hook(int (*hook)(uint32_t, int))
{
    s_mute_hook = hook;
}
#endif /* !LEGORRR_BACKEND_WINAPI */

/* ---- table dispatch (FUN_0040aa10) -------------------------------------- */
SimtickEffect s_effects[16];
int           s_effect_count = 0;
int           s_effect_fired = 0;
void (*s_effect_opacity_hook)(uint32_t, int, float, int) = NULL;
void (*s_effect_release_hook)(uint32_t, int) = NULL;

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
void simtick_effect_table(SimtickEffect* table, int count)
{
    if (table == NULL || count > 16) {
        count = 0;
    }
    if (count > 16) {
        count = 16;
    }
    s_effect_count = count;
    if (count > 0) {
        memcpy(s_effects, table, (size_t)count * sizeof(SimtickEffect));
    }
}

int simtick_effect_last_fired(void)
{
    return s_effect_fired;
}
#endif /* !LEGORRR_BACKEND_WINAPI */

void simtick_effect_set_hooks(void (*set_opacity)(uint32_t, int, float, int),
                              void (*release)(uint32_t, int))
{
    s_effect_opacity_hook = set_opacity;
    s_effect_release_hook = release;
}

/* ---- post-step cluster (FUN_0040d450) ----------------------------------- */
int s_post_steps = 0;
void** s_post_record_tables = NULL;
uint32_t s_post_record_table_count = 0;
SimtickPostClusterRecordHook s_post_active_hook = NULL;
SimtickPostClusterRecordHook s_post_entity_first_hook = NULL;
SimtickPostClusterRecordHook s_post_entity_second_hook = NULL;
void* s_post_cluster_user = NULL;
SimtickSceneEntityHook s_scene_entity_hook = NULL;
void* s_scene_entity_user = NULL;

int simtick_post_steps_run(void);

int simtick_post_steps_run(void)
{
    return s_post_steps;
}

void simtick_post_cluster_records_set(void** tables, uint32_t table_count,
    SimtickPostClusterRecordHook active_record_hook,
    SimtickPostClusterRecordHook entity_first_hook,
    SimtickPostClusterRecordHook entity_second_hook, void* user)
{
    s_post_record_tables = tables;
    s_post_record_table_count = table_count;
    s_post_active_hook = active_record_hook;
    s_post_entity_first_hook = entity_first_hook;
    s_post_entity_second_hook = entity_second_hook;
    s_post_cluster_user = user;
}

void simtick_scene_entity_hook_set(SimtickSceneEntityHook hook, void* user)
{
    s_scene_entity_hook = hook;
    s_scene_entity_user = user;
}

/* ---- bobbins list machine (FUN_004091e0) -------------------------------- */
SimtickBobbin* s_bobbin_head = NULL;
int            s_bobbin_advance = 0;
SimtickBobbinMarkHook s_bobbin_mark_hook = NULL;
void* s_bobbin_mark_context = NULL;
SimtickBobbinSpawnHook s_bobbin_spawn_hook = NULL;
SimtickBobbinActionHook s_bobbin_clear_delay_hook = NULL;
static SimtickBobbinActionHook s_bobbin_die_hook = NULL;
void* s_bobbin_action_context = NULL;

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
void simtick_set_bobbin_head(SimtickBobbin* head)
{
    s_bobbin_head = head;
}
#endif /* !LEGORRR_BACKEND_WINAPI */

void simtick_bobbin_mark_hook_set(SimtickBobbinMarkHook hook, void* context)
{
    s_bobbin_mark_hook = hook;
    s_bobbin_mark_context = context;
}

void simtick_bobbin_action_hooks_set(SimtickBobbinSpawnHook spawn,
    SimtickBobbinActionHook clear_delay, SimtickBobbinActionHook die,
    void* context)
{
    s_bobbin_spawn_hook = spawn;
    s_bobbin_clear_delay_hook = clear_delay;
    s_bobbin_die_hook = die;
    s_bobbin_action_context = context;
}

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
int simtick_bobbin_advanced(void)
{
    return s_bobbin_advance;
}
#endif /* !LEGORRR_BACKEND_WINAPI */

static SimtickBobbin* bobbin_find_key(const uint32_t key_pair[2])
{
    SimtickBobbin* node = s_bobbin_head;
    if (key_pair == NULL) return NULL;
    while (node != NULL) {
        if (node->key_pair[0] == key_pair[0]
            && node->key_pair[1] == key_pair[1]) return node;
        node = node->next;
    }
    return NULL;
}

/* ---- grid marker sweep (FUN_0044eff0) ----------------------------------- */
int s_marker_fired = 0;

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
int simtick_marker_count(void)
{
    return s_marker_fired;
}
#endif /* !LEGORRR_BACKEND_WINAPI */

/* ---- ManagerTick (corpus FUN_0045a730, finding 04) ---------------------- */

float g_manager_smooth_a = 4.0f;   /* FUN_0045a730's 4.0 / dt */
float g_manager_smooth_b = -1.0f;  /* denominator subtracts -1.0 */
float g_manager_snap     = 1.0f;   /* _DAT_0049e5e0 */
float g_manager_dt_const = 0.0f;  /* _DAT_0049e5d8 (skip when dt == ) */

SimtickManagerSlot* s_mgr = NULL; /* table held by pointer (corpus) */
int  s_mgr_count = 0;
int  (*s_repos)(int, int, int) = NULL;
int  (*s_attach)(int, int, int, int) = NULL;
int  s_metrics[2] = { 0, 0 };
int s_repos_n = 0;
int s_attach_n = 0;

const uint32_t s_timer_bits[4] = { 0x10u, 0x20u, 0x40u, 0x80u };

/* The port's slot-table model of ManagerTick (0x0045a730) and its test
 * hooks are superseded by Panel_Update (game/panel_unit.c, finding 554);
 * their declarations stay in simtick.hpp (ADR 0005 entropy rule 1). */

/* ---- tranche 8: sound keep-alive (FUN_0047aba0) ------------------------- */
int s_sound_on = 0;
int s_sound_active = 0;
int (*s_keepalive)(int, int, int) = NULL;

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
void simtick_set_sound_guards(int sound_on, int active)
{
    s_sound_on = sound_on;
    s_sound_active = active;
}

void simtick_set_keepalive_hook(int (*h)(int, int, int))
{
    s_keepalive = h;
}
#endif /* !LEGORRR_BACKEND_WINAPI */

/* ---- tranche 8: editor steps (FUN_0044cfa0) ------------------------------ */
static int      s_edit_active = 0;
static int      s_edit_steps = 0;
static void     (*s_step_hook)(int) = NULL;

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
void simtick_set_editor_state(uint32_t word, int active)
{
    s_motion_jitter_flags = word;
    s_edit_active = active;
}

void simtick_set_step_hook(void (*h)(int))
{
    s_step_hook = h;
}

int simtick_editor_steps_run(void)
{
    return s_edit_steps;
}

void simtick_editor_steps(EngineContext* c)
{
    (void)c;
    s_edit_steps = 0;
    /* corpus: if (!(DAT_004eaec8 & 0x100) && DAT_004eadd8) -> five passes. */
    if (((s_motion_jitter_flags & 0x100u) == 0) && s_edit_active != 0) {
        for (int i = 1; i <= 5; ++i) {
            ++s_edit_steps;
            if (s_step_hook != NULL) {
                s_step_hook(i);
            }
        }
    }
}
#endif /* !LEGORRR_BACKEND_WINAPI */

/* ---- tranche 8: cooldown tick (FUN_0041a2c0 → message bus) --------------- */
void simtick_cooldown_tick(EngineContext* c, float dt)
{
    (void)c;
    message_cooldown_tick(dt);
}

/* ---- tranche 8: bit-field tree utility ---------------------------------- */
static int (*s_tree_hook)(int, int) = NULL;
static int s_tree_fired = 0;

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
void simtick_set_tree_hook(int (*h)(int, int))
{
    s_tree_hook = h;
}
#endif /* !LEGORRR_BACKEND_WINAPI */

int simtick_tree_fired(void)
{
    return s_tree_fired;
}

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
void simtick_bitfield_tree(int* base, int code)
{
    s_tree_fired = 0;
    if (base == NULL) {
        return;
    }
    /* This independent test utility builds a canonical code table using
     * 16-byte records; the retail decoder target lives in resource.cpp. */
    int      bits = 1;
    uint32_t next_code = 0u;
    uint32_t scale = 0x80000000u;
    do {
        uint32_t n = (uint32_t)code;
        if (code != 0) {
            int* entry = base + 8;
            do {
                if ((int16_t)(unsigned)(uint32_t)entry[1] == (int16_t)bits) {
                    uint32_t canonical = next_code / scale;
                    uint32_t reversed = 0u;
                    int bit;
                    for (bit = 0; bit < bits; ++bit) {
                        reversed = (reversed << 1) | (canonical & 1u);
                        canonical >>= 1;
                    }
                    if (s_tree_hook != NULL) {
                        s_tree_hook((int)(next_code / scale), bits);
                    }
                    ++s_tree_fired;
                    entry[0] = (int)reversed;
                    next_code += scale;
                }
                entry += 4;
                --n;
            } while (n != 0);
        }
        ++bits;
        scale >>= 1;
    } while ((uint16_t)bits < 0x11u);
}
#endif /* !LEGORRR_BACKEND_WINAPI */

/* ---- tranche 9 ---------------------------------------------------------- */

/* FUN_004665c0: a single ordered post-step. */
int s_post_n = 0;
SimtickPostStepHook s_post_step_hook = NULL;
void* s_post_step_user = NULL;
#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
int simtick_post_steps_count(void) { return s_post_n; }
#endif /* !LEGORRR_BACKEND_WINAPI */

void simtick_post_step_hook_set(SimtickPostStepHook hook, void* user)
{
    s_post_step_hook = hook;
    s_post_step_user = user;
}

/* FUN_004313b0: minion activity state machine over the +0x88 list.
 * Cells are 7 uintptr_t: [0] handle, [3] flags byte, [5] next, [6] state.
 * (The corpus's u32 slots are a 32-bit artifact; uintptr_t keeps the next
 * pointer whole on 64-bit hosts.) */
void* s_minion_list = NULL;
int (*s_minion_dist)(int, float) = NULL;
void (*s_minion_act)(int) = NULL;
void (*s_minion_fail)(void*) = NULL;
int s_minion_actions = 0;
int s_minion_fails = 0;

void simtick_set_minion_list(void* list)
{
    s_minion_list = list;
}
#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
void simtick_set_minion_hooks(int (*d)(int, float), void (*a)(int),
                              void (*f)(void*))
{
    s_minion_dist = d;
    s_minion_act = a;
    s_minion_fail = f;
}
int simtick_minion_actions(void)   { return s_minion_actions; }
int simtick_minion_failures(void)  { return s_minion_fails; }
#endif /* !LEGORRR_BACKEND_WINAPI */

/* Deterministic seeded-cell adapter retained for existing local callers. */
static int s_cells_n = 0;
static int s_cells_x[64], s_cells_y[64];
static int s_cell_disp = 0;

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
void simtick_cell_seed(int n, const int* xs, const int* ys)
{
    if (n > 64) {
        n = 64;
    }
    s_cells_n = n;
    for (int i = 0; i < n; ++i) {
        s_cells_x[i] = xs[i];
        s_cells_y[i] = ys[i];
    }
}

int simtick_cell_dispatches(void) { return s_cell_disp; }
#endif /* !LEGORRR_BACKEND_WINAPI */

/* Portable completed-resource grid adapter. Retail TerrainCellFlagCleanup
 * @ 0x00424600 takes an opaque receiver plus an integer resource selector,
 * then obtains completed coordinate pairs through FUN_0040c3a0/FUN_0040c570.
 * This context-and-scale API consumes the port's scene sweep instead and is
 * not the retail entrypoint; the owning resource table/selector are absent. */
void simtick_grid_cell_pass(EngineContext* c, float scale)
{
    EngineState* st = c != NULL ? c->state_block : NULL;
    uint32_t* groups = NULL;
    uint32_t* slots = NULL;
    int count;
    int i;
    s_cell_disp = 0;
    count = scene_resource_availability_sweep(scale, &groups, &slots);
    if (count <= 0 || groups == NULL || slots == NULL || st == NULL ||
        st->grid_cells == NULL || st->grid_w == 0u || st->grid_h == 0u) {
        return;
    }
    for (i = 0; i < count; ++i) {
        uint32_t x = 0u;
        uint32_t y = 0u;
        uint32_t index;
        int coordinate_pair[2];
        uint8_t* cell;
        uint32_t* flags;
        scene_cell_table_pair_read((int)groups[i], (int)slots[i], &x, &y);
        if (x >= st->grid_w || y >= st->grid_h) continue;
        index = st->grid_w * y + x;
        cell = (uint8_t*)st->grid_cells + index * 0x48u;
        flags = (uint32_t*)(cell + 8);
        *flags &= ~0x800u;
        if ((*flags & 0x1000u) == 0u) {
            coordinate_pair[0] = (int)x;
            coordinate_pair[1] = (int)y;
            cmd_produce(0x14, NULL, 0, coordinate_pair);
            ++s_cell_disp;
        }
        *flags &= ~0x1000u;
        *flags &= ~0x08000000u;
    }
}

void simtick_grid_cell_pass(EngineContext* c)
{
    EngineState* st = c != NULL ? c->state_block : NULL;
    s_cell_disp = 0;
    if (st == NULL || st->grid_cells == NULL) {
        return;
    }
    for (int i = 0; i < s_cells_n; ++i) {
        if (s_cells_x[i] < 0 || s_cells_y[i] < 0 ||
            (uint32_t)s_cells_x[i] >= st->grid_w ||
            (uint32_t)s_cells_y[i] >= st->grid_h) continue;
        uint32_t idx = st->grid_w * (uint32_t)s_cells_y[i] +
                       (uint32_t)s_cells_x[i];
        uint8_t* cell = (uint8_t*)st->grid_cells + idx * 0x48u;
        uint32_t* flags = (uint32_t*)(cell + 8);
        *flags &= ~0x800u;
        if ((*flags & 0x1000u) == 0) {
            ++s_cell_disp; /* FUN_00452060 dispatch hook */
        }
        *flags &= ~0x1000u;
        *flags &= ~0x2000000u;
    }
}

/* FUN_0043b270: scene pass uses the shared DAT_004dee14 manager word. */
void (*s_scene_broadcast)(int) = NULL;
int s_scene_bcasts = 0;
SimtickCellCoord* s_scene_tracked = NULL;
uint32_t s_scene_tracked_count = 0;
static uint32_t s_scene_tracked_capacity = 0;
uint32_t s_scene_pending_count = 0;
void (*s_scene_cell_init)(EngineState*, int, int) = NULL;

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
uint32_t simtick_scene_word(void)      { return entity_state_flags_peek(); }
void simtick_set_scene_word(uint32_t w) { entity_state_flags_set(w); }
void simtick_set_scene_broadcast(void (*h)(int)) { s_scene_broadcast = h; }
int  simtick_scene_broadcasts(void)    { return s_scene_bcasts; }
#endif /* !LEGORRR_BACKEND_WINAPI */

int scene_cell_list_append(SimtickCellCoord** list, uint32_t* count,
                                 uint32_t* capacity, int x, int y)
{
    if (*count == *capacity) {
        uint32_t next_capacity = *capacity == 0 ? 16u : *capacity * 2u;
        SimtickCellCoord* next = (SimtickCellCoord*)realloc(
            *list, (size_t)next_capacity * sizeof(SimtickCellCoord));
        if (next == NULL) return 0;
        *list = next;
        *capacity = next_capacity;
    }
    (*list)[*count].x = x;
    (*list)[*count].y = y;
    ++*count;
    return 1;
}

void simtick_scene_cell_track(int x, int y)
{
    scene_cell_list_append(&s_scene_tracked, &s_scene_tracked_count,
                           &s_scene_tracked_capacity, x, y);
}

void simtick_scene_cell_init_hook(void (*hook)(EngineState*, int, int))
{
    s_scene_cell_init = hook;
}

int simtick_scene_cells_pending(void)
{
    return (int)s_scene_pending_count;
}

/* FUN_0046f550: ten 0x2b0 projectile records, an unfiltered entity walk,
 * another pass over all ten records, then ten 0x10 timed-visual records.
 * The owners provide typed sidecar views because their portable records are
 * not binary-compatible with the retail global arrays. */
const void** s_sfx_entries = NULL;
int s_sfx_n = 0;
int s_sfx_rels = 0;
SimtickSfxBankHooks s_sfx_bank_hooks;

void simtick_sfx_bank_hooks_set(const SimtickSfxBankHooks* hooks)
{
    if (hooks == NULL) {
        memset(&s_sfx_bank_hooks, 0, sizeof(s_sfx_bank_hooks));
    } else {
        s_sfx_bank_hooks = *hooks;
    }
}

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
void simtick_sfx_seed(int n, const void** entries)
{
    s_sfx_n = n > 0 ? n : 0;
    s_sfx_entries = entries;
}

int simtick_sfx_releases(void) { return s_sfx_rels; }
#endif /* !LEGORRR_BACKEND_WINAPI */

/* ---- tranche 10: the remaining table tickers ---------------------------- */

SimtickRenderPassHook s_render_pass_hook = NULL;
void* s_render_pass_user = NULL;
SimtickRenderEntityHook s_render_entity_hook = NULL;
void* s_render_entity_user = NULL;

/* FUN_00425d90: world picker via mouse motion. */
int (*s_pick)(int, int) = NULL;
int s_picks = 0;

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
void simtick_set_pick_hook(int (*h)(int, int)) { s_pick = h; }
int  simtick_mouse_picks(void) { return s_picks; }
#endif /* !LEGORRR_BACKEND_WINAPI */

/* Legacy frame-counter helper retained for callers/tests; 0x402220 is the
 * command subsystem's CmdTickGlobal entry (see game/cmd.cpp). */
static uint32_t s_frame = 0, s_frame_a = 0, s_frame_b = 0, s_frame_c = 0;

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
uint32_t simtick_frame(void) { return s_frame; }
#endif /* !LEGORRR_BACKEND_WINAPI */
int simtick_frame_bump(void) { return (int)s_frame; }

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
void simtick_frame_counter(EngineContext* c)
{
    (void)c;
    uint32_t hi = s_frame_b > s_frame_a ? s_frame_b : s_frame_a;
    s_frame = s_frame_c;
    if (s_frame < s_frame_a) {
        s_frame = s_frame_a;
    }
    if (s_frame_c < hi) {
        s_frame = hi; /* clamp to the highest sourced value */
    }
    ++s_frame;
}
#endif /* !LEGORRR_BACKEND_WINAPI */

/* The level module owns the ordered pending coordinates and stage slots.
 * Its tick returns when the ErodeTriggerTime gate elapsed, whether or not a
 * candidate was found. */
static int s_ambient_spawn_triggers;

void simtick_ambient_spawn_tick(float scale)
{
    if (lava_erosion_pending_cell_tick_result(scale))
        ++s_ambient_spawn_triggers;
}

/* Retained only for the existing simtick unit-test seam. Production EngineTick
 * dispatches simtick_ambient_spawn_tick with the reference's scale argument. */
static float s_test_spawn_acc;
static int s_test_spawns;
static int (*s_spawn_hook)(int);

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
void simtick_set_spawn_hook(int (*h)(int)) { s_spawn_hook = h; }
int simtick_spawn_calls(void)
{
    return s_ambient_spawn_triggers + s_test_spawns;
}

void simtick_ambient_spawn(EngineContext* c, float dt)
{
    (void)c;
    s_test_spawn_acc += dt;
    if (s_test_spawn_acc >= 1.0f) {
        s_test_spawn_acc = 0.0f;
        if (s_spawn_hook != NULL) s_spawn_hook(0);
        ++s_test_spawns;
    }
}
#endif /* !LEGORRR_BACKEND_WINAPI */

/* FUN_004520d0 is the per-player command-queue executor — owned by the
 * game layer (src/game/cmd.cpp); kept out of the simtick module. */

/* Legacy tick-list slot advances the transient-effect instance registry. */
void simtick_sound_bank_tick(EngineContext* c, float dt)
{
    (void)c;
    transient_effect_instances_tick(dt);
}

/* FUN_004291a0: render-pass entry (in-game camera probe / world collect). */
int s_camera_detail_collection_ready = 1;
int simtick_camera_detail_collection_ready(void)
{
    return s_camera_detail_collection_ready;
}

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
int simtick_render_steps(void) { return s_post_n; }
#endif /* !LEGORRR_BACKEND_WINAPI */

void simtick_render_pass_hook_set(SimtickRenderPassHook hook, void* user)
{
    s_render_pass_hook = hook;
    s_render_pass_user = user;
}

void simtick_render_entity_hook_set(SimtickRenderEntityHook hook, void* user)
{
    s_render_entity_hook = hook;
    s_render_entity_user = user;
}

/* FUN_0046e390: fx-bank ticker — per-entry sub-fx scan with fade/clamp. */
SimtickFxEntry* s_fx_entries;
uint32_t s_fx_count;
SimtickFxResourceUpdateFn s_fx_update;
void* s_fx_user;

void simtick_fx_bank_entries_install(SimtickFxEntry* entries, uint32_t count,
                                     SimtickFxResourceUpdateFn update,
                                     void* user)
{
    s_fx_entries = entries;
    s_fx_count = count > 256u ? 256u : count;
    s_fx_update = update;
    s_fx_user = user;
}

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
uint32_t simtick_route_position_state_peek(void)
{
    return s_motion_jitter_flags;
}
#endif /* !LEGORRR_BACKEND_WINAPI */

void simtick_global_ui_flags_set_bit2(void)
{
    simtick_timer_word |= 2u;
}

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
uint32_t simtick_global_ui_flags_word(void) { return simtick_timer_word; }
#endif /* !LEGORRR_BACKEND_WINAPI */

/* ScenePositionResetAndEffectMotionCancel (FUN_0044cf70) — see simtick.hpp. */
float s_xform_current[3];   /* corpus pose seam stand-in */
float s_xform_config[3];    /* DAT_004eae74 role */

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
void simtick_scene_transform_current_set(const float xyz[3])
{
    if (xyz != NULL) {
        __builtin_memcpy(s_xform_current, xyz, sizeof s_xform_current);
    }
}

const float* simtick_scene_transform_config_peek(void)
{
    return s_xform_config;
}
#endif /* !LEGORRR_BACKEND_WINAPI */

/* --- the axis-tolerance window (2026-10-25; see simtick.hpp) ---------------
 * The corpus tolerance triple (DAT_004eae80/84/88 role) rides own storage;
 * its corpus writer is the recorded light-effects parameter band
 * (deviation), so the install hook stands in. */
static float s_xform_tolerance[3];

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
void simtick_vector_tolerance_install(const float xyz[3])
{
    if (xyz != NULL) {
        __builtin_memcpy(s_xform_tolerance, xyz, sizeof s_xform_tolerance);
    }
}

int simtick_vector_within_axis_tolerance(const float xyz[3])
{
    if (xyz == NULL) {
        return 0; /* deviation: the corpus strides the vector blindly */
    }
    for (int i = 0; i != 3; ++i) {
        if (s_xform_tolerance[i] < fabsf(xyz[i] - s_xform_config[i])) {
            return 0;
        }
    }
    return 1;
}
#endif /* !LEGORRR_BACKEND_WINAPI */
