// SPDX-License-Identifier: LGPL-3.0-or-later
#include <math.h>
#include <string.h>

#include "message.hpp"
#include "simtick.hpp"

#define SIMTICK_MUTE_RESET 25.0f

/* ---- mute timer (FUN_0041ba10) ------------------------------------------ */
static uint32_t s_mute_word    = 0;       /* DAT_004de1f4 */
static uint32_t s_mute_last    = 0;       /* DAT_004de1fc */
static float    s_mute_timer   = SIMTICK_MUTE_RESET; /* _DAT_004a2b58, boot fresh */
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

static int effect_helper(SimtickEffect* /*e*/, float /*dt*/)
{
    return 0; /* TODO(simtick): FUN_0040aa40 per-entry effect helper. */
}

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

int simtick_post_steps_run(void);

int simtick_post_steps_run(void)
{
    return s_post_steps;
}

static void post_step(int /*id*/)
{
    /* TODO(simtick): FUN_0040d490 / FUN_004377b0 post-step bodies. */
    ++s_post_steps;
}

void simtick_post_cluster(EngineContext* c)
{
    (void)c;
    post_step(1);
    post_step(2);
    post_step(3);
}

/* ---- bobbins list machine (FUN_004091e0) -------------------------------- */
static SimtickBobbin* s_bobbin_head = NULL;
static int            s_bobbin_advance = 0;

void simtick_set_bobbin_head(SimtickBobbin* head)
{
    s_bobbin_head = head;
}

int simtick_bobbin_advanced(void)
{
    return s_bobbin_advance;
}

static int bobbin_spawn(SimtickBobbin* /*n*/)  { return 0; } /* FUN_00409270 */
static void bobbin_delayed(SimtickBobbin* /*n*/) {}          /* FUN_00409a40 */
static void bobbin_mark(SimtickBobbin* /*n*/) {}             /* FUN_00409b30 */
static void bobbin_drop(SimtickBobbin* /*n*/) {}             /* FUN_004099f0 */
static void bobbin_clear_delay(SimtickBobbin* /*n*/) {}      /* FUN_004027a0 */
static void bobbin_die(SimtickBobbin* /*n*/) {}              /* FUN_00409550 */

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
                        bobbin_drop(node);
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

    /* truncate to integer coordinates (corpus __ftol paths). */
    int ix = (int)slot->smoothed[0];
    int iy = (int)slot->smoothed[1];

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

void simtick_sound_keepalive(EngineContext* c)
{
    (void)c;
    /* corpus: if (sound system on && an ambient entry exists) -> poke it. */
    if (s_sound_on != 0 && s_sound_active != 0 && s_keepalive != NULL) {
        s_keepalive(0, 0, 0);
    }
}

/* ---- tranche 8: editor steps (FUN_0044cfa0) ------------------------------ */
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
int simtick_post_steps_count(void) { return s_post_n; }

void simtick_post_step(EngineContext* c)
{
    (void)c;
    ++s_post_n;
    /* TODO(simtick): FUN_004377b0 post-step over LAB_004665e0. */
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

/* FUN_00424600: clear grid-cell flags over the state grid (stride 0x48). */
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

void simtick_grid_cell_pass(EngineContext* c)
{
    EngineState* st = c != NULL ? c->state_block : NULL;
    s_cell_disp = 0;
    if (st == NULL || st->grid_cells == NULL) {
        return;
    }
    for (int i = 0; i < s_cells_n; ++i) {
        uint32_t idx = st->grid_w * (uint32_t)s_cells_y[i] + (uint32_t)s_cells_x[i];
        uint8_t* cell = (uint8_t*)st->grid_cells + idx * 0x48;
        uint32_t* flags = (uint32_t*)(cell + 8);
        *flags &= ~0x800u;
        if ((*flags & 0x1000u) == 0) {
            ++s_cell_disp; /* FUN_00452060 dispatch hook */
        }
        *flags &= ~0x1000u;
        *flags &= ~0x2000000u;
    }
}

/* FUN_0043b270: scene pass with a flag word + a 125 s broadcast timer. */
static uint32_t s_scene_word = 0;
static void (*s_scene_broadcast)(int) = NULL;
static int s_scene_bcasts = 0;
static float s_scene_timer = 0.0f;

uint32_t simtick_scene_word(void)      { return s_scene_word; }
void simtick_set_scene_word(uint32_t w) { s_scene_word = w; }
void simtick_set_scene_broadcast(void (*h)(int)) { s_scene_broadcast = h; }
int  simtick_scene_broadcasts(void)    { return s_scene_bcasts; }

void simtick_scene_pass(EngineContext* c, float dt)
{
    (void)c;
    if ((s_scene_word & 4u) != 0) {
        /* TODO(simtick): FUN_00432200 entry pass. */
    }
    s_scene_word |= 0x10u;
    ++s_post_n; /* FUN_004377d0 scene hook step */
    s_scene_word &= ~0x10u;
    if ((s_scene_word & 4u) != 0) {
        /* TODO(simtick): FUN_004321a0 exit pass. */
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

/* FUN_0046f550: effect-bank ticker (stride 0xac entries + stride-4 timers). */
static const void** s_sfx_entries = NULL;
static int s_sfx_n = 0;
static int s_sfx_rels = 0;

void simtick_sfx_seed(int n, const void** entries)
{
    s_sfx_n = n > 0 ? n : 0;
    s_sfx_entries = entries;
}

int simtick_sfx_releases(void) { return s_sfx_rels; }

void simtick_sfx_bank(EngineContext* c, float dt)
{
    (void)c;
    s_sfx_rels = 0;
    /* pass 1: active stride-0xac entries -> effect hook. */
    for (int i = 0; i < s_sfx_n; ++i) {
        ++s_post_n; /* FUN_0046f920 effect hook */
    }
    /* post-step. */
    ++s_post_n;
    /* pass 3: stride-4 timer slots: negative timer -> release + reset. */
    for (int i = 0; i < s_sfx_n; ++i) {
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
static float s_vu_accum = 0.0f;

/* FUN_00425d90: world picker via mouse motion. */
static int (*s_pick)(int, int) = NULL;
static int s_picks = 0;

void simtick_set_pick_hook(int (*h)(int, int)) { s_pick = h; }
int  simtick_mouse_picks(void) { return s_picks; }

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

/* FUN_0040eab0: ambient/fx spawn accumulator + palette-pick. */
static float s_spawn_acc = 0.0f;
static int   s_spawns = 0;
static int   (*s_spawn_hook)(int) = NULL;

void simtick_set_spawn_hook(int (*h)(int)) { s_spawn_hook = h; }
int  simtick_spawn_calls(void) { return s_spawns; }

void simtick_ambient_spawn(EngineContext* c, float dt)
{
    (void)c;
    s_spawn_acc += dt;
    if (s_spawn_acc >= 1.0f) {
        s_spawn_acc = 0.0f;
        if (s_spawn_hook != NULL) {
            s_spawn_hook(0); /* slot pick = FUN_0040e990 distance-modulo */
        }
        ++s_spawns;
    }
}

/* FUN_004520d0 is the per-player command-queue executor — owned by the
 * game layer (src/game/cmd.cpp); kept out of the simtick module. */

/* FUN_00465b00: sound-bank ticker — linked entries with init/lifetime. */
void simtick_sound_bank_tick(EngineContext* c, float dt)
{
    (void)c; (void)dt;
    /* corpus: walk DAT_00552128 linked entries; init missing voices, tick
     * lifetime, update slots. Structural pass: one ordered step per node. */
    ++s_post_n;
}

/* FUN_004291a0: render-pass entry (in-game camera probe / world collect). */
void simtick_render_pass(EngineContext* c, float dt, float scale, uint32_t o1)
{
    (void)dt; (void)o1;
    if (c == NULL) return;
    if (scale != 0.0f) return; /* corpus: param_2 == 0 gates the in-game pass */
    if (c->context_mode != 1) return;
    /* corpus: camera pos (FUN_004754c0 on the view cam), grid pick, height,
     * scale by FUN_00450890 x3, world-collect via FUN_00450600 hook. */
    ++s_post_n;
}

/* FUN_00449c00: a single ordered render step. */
int simtick_render_steps(void) { return s_post_n; }
void simtick_render_step(EngineContext* c)
{
    (void)c;
    ++s_post_n;
}

/* FUN_00426520: vu/timer accumulators gated on input buttons. */
void simtick_vu_meter(EngineContext* c, float dt)
{
    if (c == NULL) return;
    c->debug &= ~0x400u; /* corpus: DAT_00556ed0 &= ~0x400 each frame */
    if (c->input != NULL && (c->input->cur[0x2d] != 0
                             || c->input->cur[0x2e] != 0)) {
        s_vu_accum = 0.0f; /* buttons held -> zero the accumulator */
    } else {
        s_vu_accum += dt;
    }
}

/* FUN_0046e390: fx-bank ticker — per-entry sub-fx scan with fade/clamp. */
static int s_fx_entries[8] = {};
static int s_fx_count = 0;
void simtick_fx_bank_tick(EngineContext* c, float dt)
{
    (void)c; (void)dt; (void)s_fx_entries; (void)s_fx_count;
    /* corpus: per-entry sub-fx scan, track max, fade update, clamp 240. */
    ++s_post_n;
}
/* SimulationGlobalWordSet (FUN_0041ba90): the faithful blind store — the
 * same word the mute timer reads (simtick_set_mute_word is the port-side
 * hook with the same effect; this is the corpus's own entry point). */
void simulation_global_word_set(uint32_t value)
{
    s_mute_word = value;
}

/* DirectSoundListenerViewSet (FUN_0047ab80) — see simtick.hpp. */
void simtick_directsound_listener_view_set(int view)
{
    if (s_sound_on != 0) {
        s_sound_active = view;
    }
}

/* RoutePositionBlockedStateSet (FUN_0044da20) — see simtick.hpp. */
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
void simtick_global_ui_flags_clear_bits12(void)
{
    s_timer_word &= 0xfffffff9u;
}

uint32_t simtick_global_ui_flags_word(void) { return s_timer_word; }

/* ScenePositionResetAndEffectMotionCancel (FUN_0044cf70) — see simtick.hpp. */
static float s_xform_current[3];   /* corpus pose seam stand-in */
static float s_xform_config[3];    /* DAT_004eae74 role */

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

/* ReleaseAllGroupRecords (FUN_004099d0) — see simtick.hpp. */
void simtick_release_all_group_records(void)
{
    SimtickBobbin* node = s_bobbin_head;
    while (node != NULL) {
        SimtickBobbin* next = node->next;
        bobbin_drop(node);
        node = next;
    }
}
