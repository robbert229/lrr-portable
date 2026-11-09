// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stdio.h>
#include <string.h>

#include "mainsim.hpp"
#include "game/entity.hpp"

/* --- boundaries ----------------------------------------------------------
 *  Selecting objects / menus / rename touch: FUN_004525c0 (SelectedEntity),
 *  FUN_00437880 (EntityRename) -> entity layer not landed; skipped (stub).
 *  QuitPoll corner-dwell: FUN_004589c0 -> input_quit_dwell (input.c) is the
 *  gesture model; MainSimStep is driven by the named quit key here.
 */

void main_hud_text(EngineContext* ctx, const char* s)
{
    (void)ctx;
    fprintf(stderr, "hud: %s\n", s != NULL ? s : "");
}

static int m_edge(EngineContext* ctx, int key)
{
    return ctx->input != NULL ? input_key_edge(ctx->input, key) : 0;
}

static int m_held(EngineContext* ctx, int key)
{
    return ctx->input != NULL ? input_key_down(ctx->input, key) : 0;
}

static void m_toggle_hud(EngineContext* ctx, uint32_t* flag, uint32_t bit,
                         const char* label)
{
    *flag ^= bit;
    char buf[96];
    snprintf(buf, sizeof(buf), "%s : %s", label,
             (*flag & bit) != 0 ? "on" : "off");
    main_hud_text(ctx, buf);
}

// FUNCTION: LEGORR 0x004250c0
int main_sim_step(EngineContext* ctx, float scale,
                  uint32_t* o1, uint32_t* o2, uint32_t* o3)
{
    (void)scale;
    if (ctx == NULL) {
        return 0;
    }
    if (o1 != NULL) {
        *o1 = 0;
    }
    if (o2 != NULL) {
        *o2 = 0;
    }
    if (o3 != NULL) {
        *o3 = 0;
    }

    /* 425637..425708: capture each held movement key before later tickers. */
    entity_motion_inputs_capture(ctx->input);

    /* Debug/cheat toggles — gated by the debug word bit 0x10 (finding 04). */
    if ((ctx->debug & 0x10u) != 0) {
        if (m_edge(ctx, MNSIM_K_FRAME_RATE)) {
            m_toggle_hud(ctx, &ctx->state, 0x40u, "Frame Rate Monitor");
        }
        if (m_edge(ctx, MNSIM_K_MEMORY)) {
            m_toggle_hud(ctx, &ctx->state, 0x80u, "Memory Monitor");
        }
        if (m_edge(ctx, MNSIM_K_SOUNDFX)) {
            m_toggle_hud(ctx, &ctx->state, 0x10u, "Sound Effects");
        }
        if (m_edge(ctx, MNSIM_K_MUSIC)) {
            m_toggle_hud(ctx, &ctx->state, 0x8u, "Music");
        }
        /* render-pass toggle (corpus state 0x2/0x1000/0x2000 cluster). */
        if (m_edge(ctx, MNSIM_K_SAYFB_RENDER)) {
            if ((ctx->state & 0x2u) == 0) {
                ctx->state |= 0x2u;
            } else {
                ctx->state &= ~0x2u;
            }
        }
    }

    /* The retail air cheat is enabled by feature bit 0x8000 and decrements
     * the live LevelState float at +0x1fc by 1.0 while its key is held.
     * EngineState::air_beat is only a reconstruction-side ticker counter;
     * the level block is the source used by the game logic. */
    if ((ctx->feature & 0x8000u) != 0u && ctx->level_block != NULL &&
        m_held(ctx, MNSIM_K_AIRCHEAT)) {
        float air;
        memcpy(&air, (const char*)ctx->level_block + 0x1fc, sizeof(air));
        air -= 1.0f;
        memcpy((char*)ctx->level_block + 0x1fc, &air, sizeof(air));
    }
    else if (ctx->level_block == NULL && ctx->state_block != NULL &&
             m_held(ctx, MNSIM_K_AIRCHEAT) &&
             ctx->state_block->air_beat != 0u) {
        /* Isolated engine tests and preview contexts may have only the
         * ticker sidecar bound. Keep that adapter useful without changing
         * the live level-block path above. */
        ctx->state_block->air_beat--;
    }

    /* The source camera keys and deltas are identified (see mainsim.hpp and
     * Finding 101), but the called helpers expect the original camera-control
     * record. ViewContext's reconstructed fields do not match that record's
     * +0x78/+0x84/+0x90 values or scene-node offsets, so applying the deltas
     * here would mutate unrelated state. Keep this path deferred until that
     * record has a faithful adapter. */

    /* The three edge outputs EngineTick's later tickers consume. */
    if (m_edge(ctx, '1')) {
        *o1 = 1;
    }
    if (m_edge(ctx, '2')) {
        *o2 = 1;
    }
    if (m_edge(ctx, '3')) {
        *o3 = 1;
    }

    /* Quit gate: the corpus ends the frame through QuitPoll + the
     * keep-running probe; ours keys it to the named key + EngineKeepRunning. */
    if (m_edge(ctx, MNSIM_K_QUIT)) {
        return 0;
    }
    if (EngineKeepRunning(ctx) == 0) {
        return 0;
    }
    return 1;
}

int main_sim_step(EngineContext* ctx, float scale, float dt,
                  uint32_t* o1, uint32_t* o2, uint32_t* o3)
{
    (void)dt;
    return main_sim_step(ctx, scale, o1, o2, o3);
}
