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
    /* TODO(simtick): FUN_00450f30 marker callback. */
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