// SPDX-License-Identifier: LGPL-3.0-or-later
#include <string.h>

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
static int s_marker_fired = 0;

int simtick_marker_count(void)
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