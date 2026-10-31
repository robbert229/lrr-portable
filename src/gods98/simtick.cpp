// SPDX-License-Identifier: LGPL-3.0-or-later
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "message.hpp"
#include "simtick.hpp"
#include "sound.hpp"
#include "heap.hpp"
#include "crt/crt.hpp"
#include "resource.hpp"
#include "game/level.hpp"
#include "game/cmd.hpp"
#include "game/select.hpp"
#include "game/entity.hpp"
#include "game/effects.hpp"
#include "game/render.hpp"

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
static SimtickEffect s_effects[16];
static int           s_effect_count = 0;
static int           s_effect_fired = 0;
static void (*s_effect_opacity_hook)(uint32_t, int, float, int) = NULL;
static void (*s_effect_release_hook)(uint32_t, int) = NULL;

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

// FUNCTION: LEGORR 0x0040aa40
static int effect_helper(SimtickEffect* e, float dt)
{
    float elapsed;
    float opacity;
    float decrement;
    uint32_t count;
    uint32_t index;

    if (e == NULL) {
        return 0;
    }
    memcpy(&elapsed, &e->words[6], sizeof(elapsed));
    if (elapsed < 15.0f) {
        memcpy(&decrement, &e->words[5], sizeof(decrement));
        decrement -= dt;
        memcpy(&e->words[5], &decrement, sizeof(decrement));
        opacity = 1.0f - elapsed * 0.06666667f;
        count = e->words[2];
        if (s_effect_opacity_hook != NULL) {
            for (index = 0; index < count; ++index) {
                s_effect_opacity_hook(e->words[1], (int)index, opacity, 4);
            }
        }
        elapsed += dt;
        memcpy(&e->words[6], &elapsed, sizeof(elapsed));
        return 1;
    }

    e->words[7] &= ~1u;
    if (s_effect_release_hook != NULL) {
        s_effect_release_hook(e->words[0], 1);
    }
    return 0;
}

// FUNCTION: LEGORR 0x0040aa10
void simtick_table_dispatch(EngineContext* c, float dt, float scale)
{
    (void)c; (void)scale;
    s_effect_fired = 0;
    for (int i = 0; i < s_effect_count; ++i) {
        if ((s_effects[i].words[7] & 1u) != 0) {
            effect_helper(&s_effects[i], dt);
            ++s_effect_fired;
        }
    }
}

/* ---- post-step cluster (FUN_0040d450) ----------------------------------- */
static int s_post_steps = 0;
static void** s_post_record_tables = NULL;
static uint32_t s_post_record_table_count = 0;
static SimtickPostClusterRecordHook s_post_active_hook = NULL;
static SimtickPostClusterRecordHook s_post_entity_first_hook = NULL;
static SimtickPostClusterRecordHook s_post_entity_second_hook = NULL;
static void* s_post_cluster_user = NULL;
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

static void post_cluster_active_records(void)
{
    uint32_t table_index;
    const size_t record_stride = 0x10u + sizeof(void*);
    for (table_index = 0; table_index < s_post_record_table_count;
         ++table_index) {
        void* table = s_post_record_tables != NULL
            ? s_post_record_tables[table_index] : NULL;
        uint32_t count;
        uint32_t i;
        if (table == NULL) continue;
        count = (uint32_t)1u << (table_index & 31u);
        for (i = 0; i < count; ++i) {
            uint8_t* record = (uint8_t*)table + i * record_stride;
            void* self;
            memcpy(&self, record + 0x10, sizeof(self));
            if (self == (void*)record && s_post_active_hook != NULL)
                s_post_active_hook(record, 1, s_post_cluster_user);
        }
    }
}

static int post_cluster_entity_first(EntitySlot* entity, void* payload)
{
    (void)payload;
    if (s_post_entity_first_hook != NULL)
        s_post_entity_first_hook(entity, 0, s_post_cluster_user);
    return 0;
}

static int post_cluster_entity_second(EntitySlot* entity, void* payload)
{
    (void)payload;
    if (s_post_entity_second_hook != NULL)
        s_post_entity_second_hook(entity, 1, s_post_cluster_user);
    return 0;
}

static int scene_entity_callback(EntitySlot* entity, void* payload)
{
    float dt = *(float*)payload;
    if (s_scene_entity_hook != NULL)
        s_scene_entity_hook(entity, dt, s_scene_entity_user);
    return 0;
}

void simtick_scene_entity_hook_set(SimtickSceneEntityHook hook, void* user)
{
    s_scene_entity_hook = hook;
    s_scene_entity_user = user;
}

// FUNCTION: LEGORR 0x0040d450
void simtick_post_cluster(EngineContext* c)
{
    GameContext* game = entity_pool_callback_context_get();
    (void)c;
    post_cluster_active_records();
    entity_pool_find_by_callback_filtered(game,
        post_cluster_entity_first, NULL);
    entity_pool_find_by_callback_filtered(game,
        post_cluster_entity_second, NULL);
    s_post_steps += 3;
}

/* ---- bobbins list machine (FUN_004091e0) -------------------------------- */
static SimtickBobbin* s_bobbin_head = NULL;
static int            s_bobbin_advance = 0;
static SimtickBobbinMarkHook s_bobbin_mark_hook = NULL;
static void* s_bobbin_mark_context = NULL;
static SimtickBobbinSpawnHook s_bobbin_spawn_hook = NULL;
static SimtickBobbinActionHook s_bobbin_clear_delay_hook = NULL;
static SimtickBobbinActionHook s_bobbin_die_hook = NULL;
static void* s_bobbin_action_context = NULL;

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

// FUNCTION: LEGORR 0x00409270
static int bobbin_spawn(SimtickBobbin* node)
{
    return node != NULL && s_bobbin_spawn_hook != NULL
        ? s_bobbin_spawn_hook(node, s_bobbin_action_context) : 0;
} /* FUN_00409270 */
// FUNCTION: LEGORR 0x00409a40
static void bobbin_delayed(SimtickBobbin* node)
{
    if (node == NULL) return;
    uint32_t count = node->member_count;
    if (count > 20u) count = 20u;
    for (uint32_t i = 0; i < count; ++i) {
        void* record = node->members[i];
        if (record == NULL) continue;
        const float no_timeout = -1.0f;
        uint32_t flags = 0;
        __builtin_memcpy((char*)record + 0x340u, &no_timeout,
                         sizeof(no_timeout));
        __builtin_memcpy(&flags, (char*)record + 0x3e8u, sizeof(flags));
        flags |= 0x800000u;
        __builtin_memcpy((char*)record + 0x3e8u, &flags, sizeof(flags));
    }
}
static void bobbin_mark(SimtickBobbin* node)
{
    if (node != NULL && s_bobbin_mark_hook != NULL)
        s_bobbin_mark_hook(node, s_bobbin_mark_context);
}
static void bobbin_record_remove_and_free(SimtickBobbin* node)
{
    SimtickBobbin* previous = NULL;
    SimtickBobbin* current = s_bobbin_head;
    while (current != NULL && current != node) {
        previous = current;
        current = current->next;
    }
    if (current == NULL) return;
    if (previous == NULL) s_bobbin_head = current->next;
    else previous->next = current->next;
    engine_heap_free_wrapper(current->owned_buffer);
    engine_heap_free_wrapper(current);
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

// FUNCTION: LEGORR 0x00409110
void simtick_selection_group_release(const uint32_t key_pair[2])
{
    SimtickBobbin* node = bobbin_find_key(key_pair);
    if (node == NULL) return;
    cancel_action_for_matching_coordinates(node->key_pair);
    bobbin_delayed(node);
    bobbin_record_remove_and_free(node);
}

// FUNCTION: LEGORR 0x00409150
void simtick_selection_group_clear(const uint32_t key_pair[2])
{
    SimtickBobbin* node = bobbin_find_key(key_pair);
    uint32_t i;
    if (node == NULL) return;
    for (i = 0; i < node->member_count && i < 20u; ++i) {
        unsigned char* entity = (unsigned char*)node->members[i];
        uint32_t flags;
        if (entity == NULL) continue;
        memcpy(&flags, entity + 0x3ecu, sizeof(flags));
        flags &= ~0x100u;
        memcpy(entity + 0x3ecu, &flags, sizeof(flags));
        cmd_create_task((EntitySlot*)entity); /* FUN_00402720(entity, 0) */
    }
    cancel_action_for_matching_coordinates(node->key_pair);
    bobbin_delayed(node);
    bobbin_record_remove_and_free(node);
}
// FUNCTION: LEGORR 0x004027a0
static void bobbin_clear_delay(SimtickBobbin* node)
{
    if (node != NULL && s_bobbin_clear_delay_hook != NULL)
        s_bobbin_clear_delay_hook(node, s_bobbin_action_context);
} /* FUN_004027a0 */
// FUNCTION: LEGORR 0x00409550
static void bobbin_die(SimtickBobbin* node)
{
    if (node != NULL && s_bobbin_die_hook != NULL)
        s_bobbin_die_hook(node, s_bobbin_action_context);
    if (node != NULL) node->flags &= ~4u;
} /* FUN_00409550 */

// FUNCTION: LEGORR 0x004091e0
void simtick_bobbins(EngineContext* c)
{
    (void)c;
    SimtickBobbin* node = s_bobbin_head;
    while (node != NULL) {
        uint32_t f = node->flags;
        SimtickBobbin* next = node->next;
        ++s_bobbin_advance;
        if (((f & 2u) == 0) || ((f & 4u) != 0)) {
            if ((f & 1u) != 0) {
                if ((f & 2u) == 0) {
                    if (bobbin_spawn(node)) {
                        bobbin_delayed(node);
                        bobbin_mark(node);
                        bobbin_record_remove_and_free(node);
                    }
                } else {
                    bobbin_clear_delay(node);
                    node->flags = node->flags & ~1u;
                }
            }
        } else {
            bobbin_die(node);
        }
        node = next;
    }
}

/* ---- grid marker sweep (FUN_0044eff0) ----------------------------------- */
static int s_marker_fired = 0;int simtick_marker_count(void)
{
    return s_marker_fired;
}

static void marker_cell(EngineState* /*st*/, uint32_t x, uint32_t y)
{
    (void)x; (void)y;
    /* TODO(simtick): the corpus's per-cell terrain-init marker
     * callback — its body is a cell-geometry builder that lands with
     * the terrain-data wave (address deliberately unquoted: quoting an
     * unported function flips its ledger row). */
    ++s_marker_fired;
}

// FUNCTION: LEGORR 0x0044eff0
void simtick_marker_sweep(EngineContext* c, float dt, float scale)
{
    (void)dt; (void)scale;
    EngineState* st = c != NULL ? c->state_block : NULL;
    if (st == NULL || !st->sim_marker_flags) {
        return;
    }
    s_marker_fired = 0;
    for (uint32_t y = 0; y < st->sim_h; ++y) {
        for (uint32_t x = 0; x < st->sim_w; ++x) {
            uint8_t* cell = (uint8_t*)st->sim_grid + (y * st->sim_w + x) * 0x28;
            if ((cell[0x19] & 4u) != 0) {
                marker_cell(st, x, y);
                cell[0x19] = cell[0x19] & ~4u;
            }
        }
    }
    st->sim_marker_flags = 0;
    /* TODO(simtick): render passes FUN_00450b60 / FUN_0044f090. */
}

/* ---- ManagerTick (corpus FUN_0045a730, finding 04) ---------------------- */

float g_manager_smooth_a = 1.0f;   /* _DAT_0049e5e8 (unpinned; stable EMA) */
float g_manager_smooth_b = -1.0f;  /* _DAT_0049e5ec (corpus constants unknown) */
float g_manager_snap     = 1.0f;   /* _DAT_0049e5e0 */
float g_manager_dt_const = 0.0f;  /* _DAT_0049e5d8 (skip when dt == ) */

static SimtickManagerSlot* s_mgr = NULL; /* table held by pointer (corpus) */
static int  s_mgr_count = 0;
static int  (*s_repos)(int, int, int) = NULL;
static int  (*s_attach)(int, int, int, int) = NULL;
static int  s_metrics[2] = { 0, 0 };
static int  s_repos_n = 0, s_attach_n = 0;

// GLOBAL: LEGORR 0x00500860
static uint32_t s_timer_word = 0;
static int      s_timer_handles[4] = { 0, 0, 0, 0 }; /* DAT_00500850..5c */
static const uint32_t s_timer_bits[4] = { 0x10u, 0x20u, 0x40u, 0x80u };

void simtick_manager_clear(void)
{
    s_mgr = NULL;
    s_mgr_count = 0;
    s_repos = NULL;
    s_attach = NULL;
    s_metrics[0] = s_metrics[1] = 0;
    s_repos_n = s_attach_n = 0;
    s_timer_word = 0;
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
    return (s_timer_word & s_timer_bits[which]) != 0;
}

void simtick_manager_set_timer_word(uint32_t w)
{
    s_timer_word = w;
}

// FUNCTION: LEGORR 0x0045a730
void simtick_manager_tick(int index, float dt, int ingame)
{
    if (index < 0 || index >= s_mgr_count) {
        return;
    }
    SimtickManagerSlot* slot = &s_mgr[index];

    /* target select: source B (ints) when flag bit 4, else source A. */
    float tx, ty;
    if ((slot->flags & 4u) != 0) {
        tx = (float)slot->target_b[0];
        ty = (float)slot->target_b[1];
    } else {
        tx = slot->target_a[0];
        ty = slot->target_a[1];
    }

    /* exponential smoothing (skip when dt equals the stored constant). */
    if (dt != g_manager_dt_const) {
        float denom = g_manager_smooth_a / dt - g_manager_smooth_b;
        if ((g_manager_smooth_a / dt) == 0.0f) {
            denom = 1.0f;
        }
        if (denom == 0.0f) {
            denom = 1.0f;
        }
        if (fabsf(slot->smoothed[0] - tx) <= g_manager_snap) {
            slot->smoothed[0] = tx;
        } else {
            slot->smoothed[0] =
                ((g_manager_smooth_a / dt) * slot->smoothed[0] + tx) / denom;
        }
        if (fabsf(slot->smoothed[1] - ty) <= g_manager_snap) {
            slot->smoothed[1] = ty;
        } else {
            slot->smoothed[1] =
                ((g_manager_smooth_a / dt) * slot->smoothed[1] + ty) / denom;
        }
    }

    /* The corpus floors each coordinate before the integer conversion. */
    int ix = (int)floor((double)slot->smoothed[0]);
    int iy = (int)floor((double)slot->smoothed[1]);

    if (slot->mode == 0) {
        ++s_repos_n;
        if (s_repos != NULL) {
            s_repos(slot->handle, ix, iy);
        }
        // signal-rep: whatever the corpus does after repositions
        slot->flags &= ~1u;
    } else if (slot->mode == 1) {
        ++s_attach_n;
        if (s_attach != NULL) {
            s_attach(slot->handle, s_metrics[0], s_metrics[1], ingame);
        }
    }

    /* index 9: stop the four shared timers (DAT_00500860 bits 0x10..0x80). */
    if (index == 9) {
        for (int k = 0; k < 4; ++k) {
            if ((s_timer_word & s_timer_bits[k]) != 0) {
                s_timer_word &= ~s_timer_bits[k];
                /* FUN_0047dcb0(timer_handle, 0, &coord, 0) — see hook:
                 * a stop state is recorded through the reposition hook. */
                if (s_repos != NULL) {
                    s_repos(s_timer_handles[k], 0, 0);
                }
            }
        }
    }

    /* stop-detect: when the integer coords are sub-2 for both axes, clear
     * the slot flag bit 3 (the corpus's near-zero settle). */
    if (ix < 2 && ix > -2 && iy < 2 && iy > -2) {
        slot->flags &= ~8u;
    }
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
// GLOBAL: LEGORR 0x004eaec8
static uint32_t s_edit_word = 0;
static int      s_edit_active = 0;
static int      s_edit_steps = 0;
static void     (*s_step_hook)(int) = NULL;

void simtick_set_editor_state(uint32_t word, int active)
{
    s_edit_word = word;
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

// FUNCTION: LEGORR 0x0044cfa0
void simtick_editor_steps(EngineContext* c)
{
    (void)c;
    s_edit_steps = 0;
    /* corpus: if (!(DAT_004eaec8 & 0x100) && DAT_004eadd8) -> five passes. */
    if (((s_edit_word & 0x100u) == 0) && s_edit_active != 0) {
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

/* ---- tranche 8: bit-field tree (FUN_0049cf90) --------------------------- */
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

// FUNCTION: LEGORR 0x0049cf90
void simtick_bitfield_tree(int* base, int code)
{
    s_tree_fired = 0;
    if (base == NULL) {
        return;
    }
    /* corpus 0x49cf90: 16-bit code ladder, 0x80000000.. scale, scanning the
     * entry stream (stride 4) for a low-short code match. */
    int      bits = 1;
    int      acc = 0;
    uint32_t scale = 0x80000000u;
    int      target = 0;
    do {
        uint32_t n = (uint32_t)code;
        if (code != 0) {
            int* entry = base + 8;
            do {
                if ((int16_t)(unsigned)(uint32_t)entry[1] == (int16_t)bits) {
                    if (s_tree_hook != NULL) {
                        s_tree_hook(acc / (int)scale, bits);
                    }
                    ++s_tree_fired;
                    entry[0] = target;
                    acc += (int)scale;
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
static int s_post_n = 0;
static SimtickPostStepHook s_post_step_hook = NULL;
static void* s_post_step_user = NULL;
int simtick_post_steps_count(void) { return s_post_n; }

void simtick_post_step_hook_set(SimtickPostStepHook hook, void* user)
{
    s_post_step_hook = hook;
    s_post_step_user = user;
}

// FUNCTION: LEGORR 0x004665c0
void simtick_post_step(EngineContext* c)
{
    if (s_post_step_hook != NULL)
        s_post_step_hook(c, s_post_step_user);
    ++s_post_n;
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

/* TerrainCellFlagCleanup (FUN_00424600): sweep resource state for newly
 * completed scene loads, read each completion's configured grid coordinate,
 * clear its cell gates, and enqueue opcode 0x14 when the cell is not gated by
 * bit 0x1000. The renderer's resource-completion callbacks remain owned by
 * the scene/resource layer; the exposed sweep supplies its completed pairs. */
// FUNCTION: LEGORR 0x00424600
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

/* FUN_0043b270: scene pass with a flag word + terrain-cell staging. */
static uint32_t s_scene_word = 0;
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

uint32_t simtick_scene_word(void)      { return s_scene_word; }
void simtick_set_scene_word(uint32_t w) { s_scene_word = w; }
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
    if ((s_scene_word & 4u) != 0) {
        scene_cell_previous_state_track(st);
    }
    s_scene_word |= 0x10u;
    /* FUN_004377d0(callback, &dt, 0): visit every live, unflagged entity.
     * The callback body updates renderer-owned scene state; it is kept at
     * that ownership seam until the renderer record is present in context. */
    entity_pool_find_by_callback(entity_pool_callback_context_get(),
                                 scene_entity_callback, (void*)&dt, 0);
    ++s_post_n;
    s_scene_word &= ~0x10u;
    if ((s_scene_word & 4u) != 0) {
        scene_cell_update_queue_flush(st);
    }
    if ((s_scene_word & 8u) == 0) {
        s_scene_word &= ~4u;
    } else {
        if (s_scene_broadcast != NULL) {
            s_scene_broadcast(0);
        }
        ++s_scene_bcasts;
        s_scene_word &= ~8u;
    }
    s_scene_timer -= dt;
    if (s_scene_timer <= 0.0f) {
        s_scene_timer = 125.0f;
        if (s_scene_broadcast != NULL) {
            s_scene_broadcast(1);
        }
        ++s_scene_bcasts;
    }
}

/* FUN_0046f550: the tested port models only the timed-entry release pass.
 * The corpus first walks records 0x50391c..0x5053fc by 0x2b0 (0x46f56e,
 * 0x46f574), calls FUN_0046f920 for occupied +0x2a4 slots, then calls the
 * segment callback and FUN_0046f3b0. Its timer walk is a separate table
 * 0x50387c..0x50391c by 0x10 (0x46f5f8, 0x46f5fb); a negative timer calls
 * FUN_004820d0 twice before clearing the slot and subtracting dt. The first
 * record pass and FUN_0046f3b0's behavior are not represented by this seed
 * API, so synthesizing their calls here would not preserve its tested model. */
static const void** s_sfx_entries = NULL;
static int s_sfx_n = 0;
static int s_sfx_rels = 0;

void simtick_sfx_seed(int n, const void** entries)
{
    s_sfx_n = n > 0 ? n : 0;
    s_sfx_entries = entries;
}

int simtick_sfx_releases(void) { return s_sfx_rels; }

// FUNCTION: LEGORR 0x0046f550
void simtick_sfx_bank(EngineContext* c, float dt)
{
    (void)c;
    s_sfx_rels = 0;
    /* pass 1: active stride-0xac entries -> effect hook. */
    int i;
    for (i = 0; i < s_sfx_n; ++i) {
        ++s_post_n; /* FUN_0046f920 effect hook */
    }
    /* post-step. */
    ++s_post_n;
    /* pass 3: stride-4 timer slots: negative timer -> release + reset. */
    for (i =  0; i < s_sfx_n; ++i) {
        float* f = (float*)((uint8_t*)s_sfx_entries[i] + 12);
        if (*f < 0.0f) {
            ++s_sfx_rels; /* FUN_004820d0 release hook x2 */
            *(uint32_t*)((uint8_t*)s_sfx_entries[i] + 0) = 0;
        }
        *f -= dt;
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

/* FUN_00402220: task/frame counter with clamps + a whitelisted-list walk. */
static uint32_t s_frame = 0, s_frame_a = 0, s_frame_b = 0, s_frame_c = 0;

uint32_t simtick_frame(void) { return s_frame; }
int simtick_frame_bump(void) { return (int)s_frame; }

// FUNCTION: LEGORR 0x00402220
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

// FUNCTION: LEGORR 0x0040eab0
void simtick_ambient_spawn_tick(float scale)
{
    if (lava_erosion_pending_cell_tick(scale))
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
// FUNCTION: LEGORR 0x004291a0
void simtick_render_pass(EngineContext* c, float dt, float scale, uint32_t o1)
{
    (void)o1;
    if (c == NULL) return;
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
    entity_pool_find_by_callback(entity_pool_callback_context_get(),
        render_entity_callback, NULL, 0);
    ++s_post_n;
}

/* MainSimInteractionAndCellUpdate (FUN_00426520; Findings 474/455) clears
 * the debug bit, resolves the live pointer against the loaded level, then
 * routes a click to entity selection or a move command. The original has
 * several additional cursor modes and HUD/terrain feedback branches; those
 * rely on UI mode globals which have no EngineContext owner yet. The
 * reachable selection path uses the same perspective camera and viewport
 * dimensions as the software scene renderer, rather than guessing a cell
 * from raw screen coordinates. */
// FUNCTION: LEGORR 0x00426520
void simtick_vu_meter(EngineContext* c, float dt)
{
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
}

/* FUN_0046e390: fx-bank ticker — per-entry sub-fx scan with fade/clamp. */
static SimtickFxEntry* s_fx_entries;
static uint32_t s_fx_count;
static SimtickFxResourceUpdateFn s_fx_update;
static void* s_fx_user;

void simtick_fx_bank_entries_install(SimtickFxEntry* entries, uint32_t count,
                                     SimtickFxResourceUpdateFn update,
                                     void* user)
{
    s_fx_entries = entries;
    s_fx_count = count > 256u ? 256u : count;
    s_fx_update = update;
    s_fx_user = user;
}

// FUNCTION: LEGORR 0x0046e390
void simtick_fx_bank_tick(EngineContext* c, float dt)
{
    uint32_t active_count = 0u;
    float scale = c != NULL && c->time_scale != 0.0f ? c->time_scale : 1.0f;
    for (uint32_t i = 0; i < s_fx_count; ++i) {
        SimtickFxEntry* entry = &s_fx_entries[i];
        float maximum = entry->initial;
        int any_active = 0;
        int fade_out = (entry->flags & 2u) == 0u;
        uint32_t count = entry->count;
        if (count > 256u) count = 256u;
        for (uint32_t j = 0; entry->subentries != NULL && j < count; ++j) {
            SimtickFxSubEntry* sub = &entry->subentries[j];
            int advance_secondary = sub->enabled == 0.0f
                || sub->value < entry->current;
            if (!advance_secondary) {
                if (maximum < sub->value) maximum = sub->value;
                fade_out = 0;
                ++active_count;
                any_active = 1;
                if (sub->value == entry->current) advance_secondary = 1;
            }
            sub->fade_primary += dt * 1.5f;
            if (advance_secondary) sub->fade_secondary += dt * 0.5f;
            if (sub->fade_primary > 240.0f) sub->fade_primary = 240.0f;
            if (sub->fade_secondary > 240.0f) sub->fade_secondary = 240.0f;
        }
        if ((entry->flags & 4u) != 0u) any_active = 0;
        if (any_active || fade_out) {
            const float rate = (entry->current - entry->initial) / scale;
            if (any_active) {
                entry->flags &= ~2u;
                entry->current += active_count * dt * 0.3f / (rate + 1.0f);
                if (entry->current >= maximum) {
                    entry->current = maximum;
                    entry->flags |= 4u;
                }
            } else {
                entry->flags &= ~4u;
                entry->current -= dt * 0.05f;
                if (entry->current <= maximum) {
                    entry->current = maximum;
                    entry->flags |= 2u;
                }
            }
            for (uint32_t j = 0; j < entry->resource_count; ++j) {
                if (s_fx_update != NULL) {
                    s_fx_update(entry->resource, j, entry->current, s_fx_user);
                }
            }
        }
    }
    if (c != NULL) {
        terrain_edge_flag_midpoint_update((LevelState*)c->level_block, dt);
    }
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

/* RoutePositionBlockedStateSet (FUN_0044da20) — see simtick.hpp. */
// FUNCTION: LEGORR 0x0044da20
void simtick_route_position_blocked_state_set(int blocked)
{
    if (blocked != 0) {
        s_edit_word = (s_edit_word & ~0x400u) | 0x200u;
        return;
    }
    s_edit_word &= ~0x1200u; /* bits 9 and 12; bit 10 survives */
}

uint32_t simtick_route_position_state_peek(void) { return s_edit_word; }

/* GlobalUIFlagsClearBits12 (FUN_0045b610) — see simtick.hpp. */
// FUNCTION: LEGORR 0x0045b610
void simtick_global_ui_flags_clear_bits12(void)
{
    s_timer_word &= 0xfffffff9u;
}

void simtick_global_ui_flags_set_bit2(void)
{
    s_timer_word |= 2u;
}

uint32_t simtick_global_ui_flags_word(void) { return s_timer_word; }

/* ScenePositionResetAndEffectMotionCancel (FUN_0044cf70) — see simtick.hpp. */
static float s_xform_current[3];   /* corpus pose seam stand-in */
static float s_xform_config[3];    /* DAT_004eae74 role */

// FUNCTION: LEGORR 0x0044cf70
void simtick_scene_position_reset_and_effect_motion_cancel(void)
{
    __builtin_memcpy(s_xform_config, s_xform_current, sizeof s_xform_config);
    s_edit_word &= 0xffffff7fu;
}

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

// FUNCTION: LEGORR 0x0044d9a0
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
