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

#define SIMTICK_MUTE_RESET 25.0f

/* ---- mute timer (FUN_0041ba10) ------------------------------------------ */
// GLOBAL: LEGORR 0x004de1f4
static uint32_t s_mute_word    = 0;       /* DAT_004de1f4 */
// GLOBAL: LEGORR 0x004de1fc
static uint32_t s_mute_last    = 0;       /* DAT_004de1fc */
// GLOBAL: LEGORR 0x004a2b58
static float    s_mute_timer   = SIMTICK_MUTE_RESET; /* _DAT_004a2b58, boot fresh */
// GLOBAL: LEGORR 0x004de200
static int      s_mute_engaged = 0;       /* DAT_004de200 */
static int      (*s_mute_hook)(uint32_t id, int vol) = NULL; /* FUN_00464ef0 */

uint32_t simtick_mute_word(void)      { return s_mute_word; }
int      simtick_mute_engaged(void)   { return s_mute_engaged; }

void simtick_set_mute_word(uint32_t v)
{
    s_mute_word = v;
}

void simtick_set_mute_hook(int (*hook)(uint32_t, int))
{
    s_mute_hook = hook;
}

// FUNCTION: LEGORR 0x0041ba10
void simtick_mute_timer(EngineContext* c, float dt)
{
    (void)c;
    if (s_mute_word != s_mute_last) {
        s_mute_last = s_mute_word;
        s_mute_timer = SIMTICK_MUTE_RESET;
        s_mute_engaged = 0;
        s_mute_word = 0;
        return;
    }
    s_mute_timer -= dt;
    if ((s_mute_timer <= 0.0f) && (s_mute_engaged == 0)) {
        if (s_mute_hook != NULL) {
            s_mute_engaged = s_mute_hook(s_mute_word, 0);
        } else {
            s_mute_engaged = 1;
        }
    }
    s_mute_word = 0;
}

/* ---- table dispatch (FUN_0040aa10) -------------------------------------- */
SimtickEffect s_effects[16];
int           s_effect_count = 0;
int           s_effect_fired = 0;
void (*s_effect_opacity_hook)(uint32_t, int, float, int) = NULL;
void (*s_effect_release_hook)(uint32_t, int) = NULL;

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
static SimtickSceneEntityHook s_scene_entity_hook = NULL;
static void* s_scene_entity_user = NULL;

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

static int scene_entity_callback(EntitySlot* entity, void* payload)
{
    float dt = *(float*)payload;
    /* LAB_0043c810 updates live EntitySlot/runner state. Always continue the
     * pool walk here: its nonzero return is reserved for early branch exits
     * inside one entity, not for stopping FUN_004377d0. */
    ai_entity_frame_update(entity, dt);
    if (s_scene_entity_hook != NULL)
        s_scene_entity_hook(entity, dt, s_scene_entity_user);
    return 0;
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
static SimtickBobbinActionHook s_bobbin_clear_delay_hook = NULL;
static SimtickBobbinActionHook s_bobbin_die_hook = NULL;
void* s_bobbin_action_context = NULL;

void simtick_set_bobbin_head(SimtickBobbin* head)
{
    s_bobbin_head = head;
}

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

int simtick_bobbin_advanced(void)
{
    return s_bobbin_advance;
}

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

// FUNCTION: LEGORR 0x004027a0
void bobbin_clear_delay(SimtickBobbin* node)
{
    if (node == NULL) return;
    if (s_bobbin_clear_delay_hook != NULL) {
        s_bobbin_clear_delay_hook(node, s_bobbin_action_context);
        return;
    }

    /* BobbinClearDelay (FUN_004027a0) only queues the coordinate command
     * when the keyed cell is clear, has a nonzero speed class, and its
     * entry resources are ready. The retail body then allocates type 0x16,
     * stores the pair in +8/+c, and links it globally with kind 0xe. */
    GameContext* game = entity_pool_callback_context_get();
    int cell_xy[2];
    void* command;
    cell_xy[0] = (int)node->key_pair[0];
    cell_xy[1] = (int)node->key_pair[1];
    if (game == NULL || game->level == NULL ||
        level_cell_occupied_at_current(cell_xy) != 0u ||
        level_cell_speed_class(game->level, (int)node->key_pair[0],
                               (int)node->key_pair[1]) == 0 ||
        ensure_cell_entries_loaded(0xfu, 0, 0) == 0) {
        return;
    }
    command = cmd_alloc(0x16);
    if (command == NULL) return;
    cmd_rec_move_xy_set(command, (float)node->key_pair[0],
                        (float)node->key_pair[1]);
    cmd_link_global_head(command, 0xe);
} /* FUN_004027a0 */

/* ---- grid marker sweep (FUN_0044eff0) ----------------------------------- */
int s_marker_fired = 0;

int simtick_marker_count(void)
{
    return s_marker_fired;
}

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

void simtick_manager_clear(void)
{
    s_mgr = NULL;
    s_mgr_count = 0;
    s_repos = NULL;
    s_attach = NULL;
    s_metrics[0] = s_metrics[1] = 0;
    s_repos_n = s_attach_n = 0;
    simtick_timer_word = 0;
}

void simtick_manager_set_slots(SimtickManagerSlot* slots, int count)
{
    simtick_manager_clear();
    if (slots == NULL) {
        return;
    }
    if (count > SIMTICK_MANAGER_SLOTS) {
        count = SIMTICK_MANAGER_SLOTS;
    }
    s_mgr = slots; /* keep the caller's table (the corpus mutates it in place) */
    s_mgr_count = count;
}

void simtick_manager_set_reposition_hook(int (*h)(int, int, int))
{
    s_repos = h;
}

void simtick_manager_set_attach_hook(int (*h)(int, int, int, int))
{
    s_attach = h;
}

void simtick_manager_set_metrics(int mx, int my)
{
    s_metrics[0] = mx;
    s_metrics[1] = my;
}

int simtick_manager_repositions(void) { return s_repos_n; }
int simtick_manager_attaches(void)     { return s_attach_n; }

/* Index-9 timer word bits 0x10..0x80 (DAT_00500860) — still armed state. */
int simtick_manager_timer_bit(int which)
{
    if (which < 0 || which > 3) {
        return 0;
    }
    return (simtick_timer_word & s_timer_bits[which]) != 0;
}

void simtick_manager_set_timer_word(uint32_t w)
{
    simtick_timer_word = w;
}

/* ---- tranche 8: sound keep-alive (FUN_0047aba0) ------------------------- */
static int s_sound_on = 0, s_sound_active = 0;
// GLOBAL: LEGORR 0x00506688
static int s_listener_view = 0;
static int (*s_keepalive)(int, int, int) = NULL;

void simtick_set_sound_guards(int sound_on, int active)
{
    s_sound_on = sound_on;
    s_sound_active = active;
}

void simtick_set_keepalive_hook(int (*h)(int, int, int))
{
    s_keepalive = h;
}

// FUNCTION: LEGORR 0x0047aba0
void simtick_sound_keepalive(EngineContext* c)
{
    (void)c;
    /* corpus: if (sound system on && an ambient entry exists) -> poke it. */
    if (s_sound_on != 0 && s_sound_active != 0 && s_keepalive != NULL) {
        s_keepalive(0, 0, 0);
    }
}

/* ---- tranche 8: editor steps (FUN_0044cfa0) ------------------------------ */
static int      s_edit_active = 0;
static int      s_edit_steps = 0;
static void     (*s_step_hook)(int) = NULL;

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

/* ---- tranche 8: cooldown tick (FUN_0041a2c0 → message bus) --------------- */
void simtick_cooldown_tick(EngineContext* c, float dt)
{
    (void)c;
    message_cooldown_tick(dt);
}

/* ---- tranche 8: bit-field tree utility ---------------------------------- */
static int (*s_tree_hook)(int, int) = NULL;
static int s_tree_fired = 0;

void simtick_set_tree_hook(int (*h)(int, int))
{
    s_tree_hook = h;
}

int simtick_tree_fired(void)
{
    return s_tree_fired;
}

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

/* ---- tranche 9 ---------------------------------------------------------- */

/* FUN_004665c0: a single ordered post-step. */
int s_post_n = 0;
SimtickPostStepHook s_post_step_hook = NULL;
void* s_post_step_user = NULL;
int simtick_post_steps_count(void) { return s_post_n; }

void simtick_post_step_hook_set(SimtickPostStepHook hook, void* user)
{
    s_post_step_hook = hook;
    s_post_step_user = user;
}

/* FUN_004313b0: minion activity state machine over the +0x88 list.
 * Cells are 7 uintptr_t: [0] handle, [3] flags byte, [5] next, [6] state.
 * (The corpus's u32 slots are a 32-bit artifact; uintptr_t keeps the next
 * pointer whole on 64-bit hosts.) */
static void* s_minion_list = NULL;
static int (*s_minion_dist)(int, float) = NULL;
static void (*s_minion_act)(int) = NULL;
static void (*s_minion_fail)(void*) = NULL;
static int s_minion_actions = 0, s_minion_fails = 0;

void simtick_set_minion_list(void* list)
{
    s_minion_list = list;
}
void simtick_set_minion_hooks(int (*d)(int, float), void (*a)(int),
                              void (*f)(void*))
{
    s_minion_dist = d;
    s_minion_act = a;
    s_minion_fail = f;
}
int simtick_minion_actions(void)   { return s_minion_actions; }
int simtick_minion_failures(void)  { return s_minion_fails; }

// FUNCTION: LEGORR 0x004313b0
void simtick_minion_states(EngineContext* c, float dt, float scale)
{
    (void)dt;
    EngineState* st = c != NULL ? c->state_block : NULL;
    uintptr_t* node = (uintptr_t*)(st != NULL && st->minion_list != NULL
                                       ? st->minion_list : s_minion_list);
    s_minion_actions = s_minion_fails = 0;
    while (node != NULL) {
        uintptr_t* next = (uintptr_t*)node[5];
        float dist = 0.0f;
        if (s_minion_dist != NULL) {
            dist = (float)s_minion_dist((int)(uint32_t)node[0], scale);
        }
        if (node[6] == 0) {
            uint8_t* b = (uint8_t*)&node[3];
            if (((*b & 1u) == 0) && (dist != 0.0f)) {
                *b = (uint8_t)(*b | 1u);
                if (s_minion_act != NULL) {
                    s_minion_act((int)(uint32_t)node[0]);
                }
                ++s_minion_actions;
            } else if ((*b & 2u) == 0) {
                if (((*b & 4u) != 0) && (dist != 0.0f)) {
                    *b = (uint8_t)(*b | 2u);
                    ++s_minion_actions;
                }
            } else if (s_minion_fail != NULL) {
                s_minion_fail(node);
                ++s_minion_fails;
            }
        } else if (s_minion_fail != NULL) {
            s_minion_fail(node);
            ++s_minion_fails;
        }
        node = next;
    }
}

/* Deterministic seeded-cell adapter retained for existing local callers. */
static int s_cells_n = 0;
static int s_cells_x[64], s_cells_y[64];
static int s_cell_disp = 0;

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
static void (*s_scene_broadcast)(int) = NULL;
static int s_scene_bcasts = 0;
static float s_scene_timer = 0.0f;
typedef struct SimtickCellCoord { int x, y; } SimtickCellCoord;
static SimtickCellCoord* s_scene_tracked = NULL;
static uint32_t s_scene_tracked_count = 0;
static uint32_t s_scene_tracked_capacity = 0;
static SimtickCellCoord* s_scene_pending = NULL;
static uint32_t s_scene_pending_count = 0;
static uint32_t s_scene_pending_capacity = 0;
static void (*s_scene_cell_init)(EngineState*, int, int) = NULL;

uint32_t simtick_scene_word(void)      { return entity_state_flags_peek(); }
void simtick_set_scene_word(uint32_t w) { entity_state_flags_set(w); }
void simtick_set_scene_broadcast(void (*h)(int)) { s_scene_broadcast = h; }
int  simtick_scene_broadcasts(void)    { return s_scene_bcasts; }

static int scene_cell_list_append(SimtickCellCoord** list, uint32_t* count,
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

/* FUN_00432200 snapshots the tracked coordinates before the entity walk,
 * mirrors each coordinate into its previous-state slots, and clears cell
 * flag 0x100. The host grid is the same +0xb8 / 0x48-byte cell array used by
 * EngineState. */
static void scene_cell_previous_state_track(EngineState* st)
{
    uint32_t i;
    if (s_scene_tracked_count == 0) return;
    for (i = 0; i < s_scene_tracked_count; ++i) {
        SimtickCellCoord coord = s_scene_tracked[i];
        if (st != NULL && st->grid_cells != NULL && coord.x >= 0 && coord.y >= 0
            && (uint32_t)coord.x < st->grid_w
            && (uint32_t)coord.y < st->grid_h) {
            uint8_t* cell = (uint8_t*)st->grid_cells
                + ((uint32_t)coord.y * st->grid_w + (uint32_t)coord.x) * 0x48u;
            uint32_t flags;
            memcpy(&flags, cell + 0x0c, sizeof(flags));
            flags &= ~0x100u;
            memcpy(cell + 0x0c, &flags, sizeof(flags));
        }
        scene_cell_list_append(&s_scene_pending, &s_scene_pending_count,
                              &s_scene_pending_capacity, coord.x, coord.y);
    }
    s_scene_tracked_count = 0;
}

/* FUN_004321a0 drains the snapshot after the entity callback walk. A cell
 * whose +0x0d byte bit 0 is clear is sent to CellInitPass (FUN_0042f6f0). */
static void scene_cell_update_queue_flush(EngineState* st)
{
    uint32_t i;
    for (i = 0; i < s_scene_pending_count; ++i) {
        SimtickCellCoord coord = s_scene_pending[i];
        if (st == NULL || st->grid_cells == NULL || coord.x < 0 || coord.y < 0
            || (uint32_t)coord.x >= st->grid_w
            || (uint32_t)coord.y >= st->grid_h) continue;
        uint8_t* cell = (uint8_t*)st->grid_cells
            + ((uint32_t)coord.y * st->grid_w + (uint32_t)coord.x) * 0x48u;
        if ((cell[0x0d] & 1u) == 0 && s_scene_cell_init != NULL)
            s_scene_cell_init(st, coord.x, coord.y);
    }
    s_scene_pending_count = 0;
}

// FUNCTION: LEGORR 0x0043b270
void simtick_scene_pass(EngineContext* c, float dt)
{
    EngineState* st = c != NULL ? c->state_block : NULL;
    uint32_t scene_word = entity_state_flags_peek();
    if ((scene_word & 4u) != 0) {
        if (s_scene_tracked_count != 0u)
            scene_cell_previous_state_track(st); /* explicit test adapter */
        else
            terrain_cell_previous_state_track_current();
    }
    entity_state_flags_set(scene_word | 0x10u);
    /* FUN_004377d0(callback, &dt, 0): visit every live, unflagged entity and
     * run the recovered per-entity behavior callback. */
    entity_pool_find_by_callback_current(scene_entity_callback,
                                         (void*)&dt, 0);
    ++s_post_n;
    scene_word = entity_state_flags_peek() & ~0x10u;
    entity_state_flags_set(scene_word);
    if ((scene_word & 4u) != 0) {
        if (s_scene_pending_count != 0u)
            scene_cell_update_queue_flush(st); /* explicit test adapter */
        else
            terrain_cell_update_queue_flush_current();
    }
    if ((scene_word & 8u) == 0) {
        entity_state_flags_set(scene_word & ~4u);
    } else {
        entity_activity_scene_state_update(entity_pool_callback_context_get());
        if (s_scene_broadcast != NULL) {
            s_scene_broadcast(0);
        }
        ++s_scene_bcasts;
        entity_state_flags_set(entity_state_flags_peek() & ~8u);
    }
    s_scene_timer -= dt;
    if (s_scene_timer <= 0.0f) {
        s_scene_timer = 125.0f;
        periodic_entity_callback_dispatch((void*)(uintptr_t)1u);
        if (s_scene_broadcast != NULL) {
            s_scene_broadcast(1);
        }
        ++s_scene_bcasts;
    }
    /* FUN_0044bf00's global deferred callback list still needs its renderer
     * record/callback owner; no portable producer is installed for that list. */
}

/* FUN_0046f550: ten 0x2b0 projectile records, an unfiltered entity walk,
 * another pass over all ten records, then ten 0x10 timed-visual records.
 * The owners provide typed sidecar views because their portable records are
 * not binary-compatible with the retail global arrays. */
static const void** s_sfx_entries = NULL;
static int s_sfx_n = 0;
static int s_sfx_rels = 0;
static SimtickSfxBankHooks s_sfx_bank_hooks;

static int sfx_entity_scan_callback(EntitySlot* entity, void* payload)
{
    const float dt = *(const float*)payload;
    if (s_sfx_bank_hooks.entity_segment_scan != NULL)
        s_sfx_bank_hooks.entity_segment_scan(
            entity, dt, s_sfx_bank_hooks.context);
    return 0;
}

void simtick_sfx_bank_hooks_set(const SimtickSfxBankHooks* hooks)
{
    if (hooks == NULL) {
        memset(&s_sfx_bank_hooks, 0, sizeof(s_sfx_bank_hooks));
    } else {
        s_sfx_bank_hooks = *hooks;
    }
}

void simtick_sfx_seed(int n, const void** entries)
{
    s_sfx_n = n > 0 ? n : 0;
    s_sfx_entries = entries;
}

int simtick_sfx_releases(void) { return s_sfx_rels; }

// FUNCTION: LEGORR 0x0046f550
void simtick_sfx_bank(EngineContext* c, float dt)
{
    int i;
    s_sfx_rels = 0;

    /* Ten projectile records: update only records with a spawned entity at
     * +0x2a4. The update owner performs the scene-resource movement from
     * FUN_0046f920 using its retained full record. */
    if (s_sfx_bank_hooks.projectile_record_get != NULL) {
        for (i = 0; i < SIMTICK_SFX_BANK_SLOTS; ++i) {
            SimtickSfxProjectileRecord* record =
                s_sfx_bank_hooks.projectile_record_get(
                    i, s_sfx_bank_hooks.context);
            if (record != NULL && record->tracked_entity != NULL &&
                s_sfx_bank_hooks.projectile_update != NULL) {
                s_sfx_bank_hooks.projectile_update(record->owner_record, dt,
                                                   s_sfx_bank_hooks.context);
            }
        }
    }

    /* LAB_0046f610 is called through the unfiltered entity-pool walk between
     * projectile movement and per-record hit scans. The checked-in full
     * decompile contains no body for that label, so its narrow callback stays
     * optional while the live pool walk and surrounding order are retained. */
    (void)c;
    entity_pool_find_by_callback_current(sfx_entity_scan_callback, &dt, 0);

    /* The hit scanner runs for all ten records, including empty records; the
     * retail helper itself gates on the record's +0x2a4 entity pointer. */
    if (s_sfx_bank_hooks.projectile_record_get != NULL) {
        for (i = 0; i < SIMTICK_SFX_BANK_SLOTS; ++i) {
            SimtickSfxProjectileRecord* record =
                s_sfx_bank_hooks.projectile_record_get(
                    i, s_sfx_bank_hooks.context);
            if (record != NULL && s_sfx_bank_hooks.ray_hit_scan != NULL)
                s_sfx_bank_hooks.ray_hit_scan(record->owner_record,
                                              s_sfx_bank_hooks.context);
        }
    }

    /* Ten 0x10 timed-visual records: negative timers release both handles,
     * clear the first handle as the occupancy sentinel, then every active
     * timer is decremented (including one just released). */
    if (s_sfx_bank_hooks.timed_visual_record_get != NULL) {
        for (i = 0; i < SIMTICK_SFX_BANK_SLOTS; ++i) {
            SimtickSfxTimedVisualRecord* record =
                s_sfx_bank_hooks.timed_visual_record_get(
                    i, s_sfx_bank_hooks.context);
            if (record == NULL || record->first_handle == NULL) continue;
            if (record->remaining < 0.0f) {
                if (s_sfx_bank_hooks.resource_release != NULL) {
                    s_sfx_bank_hooks.resource_release(record->first_handle,
                        record->resource_owner, s_sfx_bank_hooks.context);
                    s_sfx_bank_hooks.resource_release(record->second_handle,
                        record->resource_owner, s_sfx_bank_hooks.context);
                }
                record->first_handle = NULL;
                ++s_sfx_rels;
            }
            record->remaining -= dt;
        }
    } else {
        /* Keep the small raw timer fixture available to existing simtick
         * tests; production records use the typed bank hooks above. */
        for (i = 0; i < s_sfx_n; ++i) {
            float* remaining = (float*)((uint8_t*)s_sfx_entries[i] + 12);
            if (*remaining < 0.0f) {
                ++s_sfx_rels;
                *(uint32_t*)((uint8_t*)s_sfx_entries[i]) = 0;
            }
            *remaining -= dt;
        }
    }
}

/* ---- tranche 10: the remaining table tickers ---------------------------- */

/* shared tranche-10 module state */
static int   s_mouse_anchor_x = 0, s_mouse_anchor_y = 0;
static SimtickRenderPassHook s_render_pass_hook = NULL;
static void* s_render_pass_user = NULL;
static SimtickRenderEntityHook s_render_entity_hook = NULL;
static void* s_render_entity_user = NULL;

/* FUN_00425d90: world picker via mouse motion. */
static int (*s_pick)(int, int) = NULL;
static int s_picks = 0;

void simtick_set_pick_hook(int (*h)(int, int)) { s_pick = h; }
int  simtick_mouse_picks(void) { return s_picks; }

// FUNCTION: LEGORR 0x00425d90
void simtick_mouse_world(EngineContext* c, float dt)
{
    (void)dt;
    /* gate: state & 2 (in-game render pass), &0x1000 required, &0x100 blocks. */
    if (c == NULL) return;
    if ((c->state & 2u) == 0) return;
    if ((c->state & 0x1000u) == 0) return;
    if ((c->state & 0x100u) != 0) return;
    if (c->input == NULL) return;
    /* mouse delta vs stored anchor; accumulate the distance. */
    static float acc = 0.0f;
    float dx = (float)c->input->mouse_x - (float)s_mouse_anchor_x;
    float dy = (float)c->input->mouse_y - (float)s_mouse_anchor_y;
    acc += sqrtf(dx * dx + dy * dy);
    if (acc >= 4.0f) {
        acc = 0.0f;
        if (s_pick != NULL) {
            s_pick(c->input->mouse_x, c->input->mouse_y);
        }
        ++s_picks;
    }
}

/* Legacy frame-counter helper retained for callers/tests; 0x402220 is the
 * command subsystem's CmdTickGlobal entry (see game/cmd.cpp). */
static uint32_t s_frame = 0, s_frame_a = 0, s_frame_b = 0, s_frame_c = 0;

uint32_t simtick_frame(void) { return s_frame; }
int simtick_frame_bump(void) { return (int)s_frame; }

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

/* FUN_004520d0 is the per-player command-queue executor — owned by the
 * game layer (src/game/cmd.cpp); kept out of the simtick module. */

/* Legacy tick-list slot advances the transient-effect instance registry. */
void simtick_sound_bank_tick(EngineContext* c, float dt)
{
    (void)c;
    transient_effect_instances_tick(dt);
}

/* FUN_004291a0: render-pass entry (in-game camera probe / world collect). */
static int s_camera_detail_collection_ready = 1;
int simtick_camera_detail_collection_ready(void)
{
    return s_camera_detail_collection_ready;
}
// FUNCTION: LEGORR 0x004291a0
void simtick_render_pass(EngineContext* c, float dt, float scale, uint32_t o1)
{
    (void)o1;
    if (c == NULL) return;
    if (scale == 0.0f) s_camera_detail_collection_ready = 1;
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
    if (c->level_block != NULL &&
        ((scale == 0.0f && c->context_mode == 1) ||
         (scale != 0.0f && (c->state & 2u) != 0u &&
          (c->state & 0x2000u) != 0u))) {
        const int primary = scale == 0.0f;
        float position[3], forward[3];
        if (!scene_runtime_collection_pose_get(c->views[primary ? 0 : 1],
                primary, position, forward)) {
            fprintf(stderr, "camera-diag: pose failed primary=%d view=%p level=%p\n",
                primary, c->views[primary ? 0 : 1], c->level_block);
            fflush(stderr);
            s_camera_detail_collection_ready = 0;
        } else if (!terrain_camera_detail_collect((LevelState*)c->level_block,
                position, primary ? forward : NULL, primary)) {
            fprintf(stderr, "camera-diag: collection failed primary=%d level=%p pos=%g,%g,%g\n",
                primary, c->level_block, position[0], position[1], position[2]);
            fflush(stderr);
            s_camera_detail_collection_ready = 0;
        }
    }
#endif
    if (scale == 0.0f) {
        if (c->context_mode == 1 || c->context_mode == 0) {
            if (s_render_pass_hook != NULL)
                s_render_pass_hook(c, dt, scale, s_render_pass_user);
            ++s_post_n;
        }
    } else if ((c->state & 2u) != 0 && (c->state & 0x2000u) != 0) {
        if (s_render_pass_hook != NULL)
            s_render_pass_hook(c, dt, scale, s_render_pass_user);
        ++s_post_n;
    }
}

int simtick_render_steps(void) { return s_post_n; }

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

static int render_entity_callback(EntitySlot* entity, void* payload)
{
    (void)payload;
    if (s_render_entity_hook != NULL)
        s_render_entity_hook(entity, s_render_entity_user);
    return 0;
}

/* FUN_00449c00: a single ordered render step. */
// FUNCTION: LEGORR 0x00449c00
void simtick_render_step(EngineContext* c)
{
    (void)c;
    entity_pool_find_by_callback_current(render_entity_callback, NULL, 0);
    ++s_post_n;
}

/* MainSimInteractionAndCellUpdate: retain this subsystem and its real
 * admission gates. Ordinary pointer actions share the frame UI arbiter's
 * ordered samples and central world owner; raw aggregate presses cannot
 * bypass UI or emit a second command after the operation queue has drained. */
// FUNCTION: LEGORR 0x00426520
void simtick_vu_meter(EngineContext* c, float dt)
{
#if defined(LEGORRR_BACKEND_WINAPI)
    GameContext* game;
    LevelState* level;
    int width, height, i;
    (void)dt;
    if (c == NULL) return;
    c->debug &= ~0x400u;
    if ((c->state & 2u) == 0u || (c->state & 0x100u) != 0u ||
        c->context_mode != 1 || c->input == NULL) return;
    game = entity_pool_callback_context_get();
    level = (LevelState*)c->level_block;
    if (game == NULL || level == NULL || game->level != level) return;
    width = display_width_get();
    height = display_height_get();
    for (i = 0; i < c->input->pointer_count; ++i)
        (void)selection_pointer_world_dispatch(game, c,
            &c->input->pointer_samples[i], width, height);
#else
    GameContext* game;
    LevelState* level;
    float cell_x, cell_y;
    EntitySlot* picked;
    int width, height;
    (void)dt;
    if (c == NULL) return;
    c->debug &= ~0x400u; /* corpus's first statement after loading state */

    if ((c->state & 2u) == 0u || (c->state & 0x100u) != 0u ||
        c->context_mode != 1 || c->input == NULL ||
        c->input->mouse_press == 0) return;

    game = entity_pool_callback_context_get();
    level = (LevelState*)c->level_block;
    if (game == NULL || level == NULL || game->level != level) return;
    width = display_width_get();
    height = display_height_get();
    if (width <= 0 || height <= 0 ||
        !level_scene_mouse_cell(game, level, width, height,
                               c->input->mouse_x, c->input->mouse_y,
                               &cell_x, &cell_y)) return;

    picked = level_scene_pick_entity(game, level, width, height,
                                     c->input->mouse_x, c->input->mouse_y,
                                     18.0f);
    if (picked == NULL) {
        /* The renderer's precise screen picker currently covers minifigures.
         * Keep other selectable entity kinds reachable through the shared
         * cell-space selection owner. */
        picked = selection_pick(game, cell_x, cell_y, 0.75f);
    }
    if (picked != NULL) {
        (void)selection_add(game, picked);
    }
    else if (selection_count(game) != 0) {
        (void)selection_cmd_move(game, cell_x, cell_y);
    }
#endif
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

/* SimulationGlobalWordSet (FUN_0041ba90): the faithful blind store — the
 * same word the mute timer reads (simtick_set_mute_word is the port-side
 * hook with the same effect; this is the corpus's own entry point). */
// FUNCTION: LEGORR 0x0041ba90
void simulation_global_word_set(uint32_t value)
{
    s_mute_word = value;
}

/* DirectSoundListenerViewSet (FUN_0047ab80) — see simtick.hpp. */
// FUNCTION: LEGORR 0x0047ab80
void simtick_directsound_listener_view_set(int view)
{
    if (s_bank_sound_on != 0) {
        s_listener_view = view;
    }
}

uint32_t simtick_route_position_state_peek(void)
{
    return s_motion_jitter_flags;
}

void simtick_global_ui_flags_set_bit2(void)
{
    simtick_timer_word |= 2u;
}

uint32_t simtick_global_ui_flags_word(void) { return simtick_timer_word; }

/* ScenePositionResetAndEffectMotionCancel (FUN_0044cf70) — see simtick.hpp. */
float s_xform_current[3];   /* corpus pose seam stand-in */
float s_xform_config[3];    /* DAT_004eae74 role */

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

/* --- the axis-tolerance window (2026-10-25; see simtick.hpp) ---------------
 * The corpus tolerance triple (DAT_004eae80/84/88 role) rides own storage;
 * its corpus writer is the recorded light-effects parameter band
 * (deviation), so the install hook stands in. */
static float s_xform_tolerance[3];

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
