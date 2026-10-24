// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_engine_bind — engine <-> level binding (game/level.h offsets).
 *
 * Verifies that binding a LevelState block into EngineContext.level_block
 * makes the engine-side accessors read the real voxel grid (width +0x14,
 * height +0x18, voxels +0xb8), and that the view's terrain pick sees the
 * voxel surface byte (+0x04).
 */
#include <stdio.h>
#include <string.h>

#include "engine.hpp"
#include "view.hpp"

static int s_failures = 0;

static void CHECK(int cond, const char* what)
{
    if (!cond) {
        printf("FAIL: %s\n", what);
        ++s_failures;
    }
}

int main(void)
{
    /* A synthetic LevelState block: 2x1 grid, voxel cell 0 surface = 7. */
    uint8_t level[0x284];
    memset(level, 0, sizeof(level));
    uint8_t voxels[2 * 0x48];
    memset(voxels, 0, sizeof(voxels));
    voxels[0x04] = 7; /* cell (0,0) surface byte */

    uint32_t w = 2, h = 1;
    memcpy(level + 0x14, &w, 4);
    memcpy(level + 0x18, &h, 4);
    uintptr_t vp = (uintptr_t)voxels;
    memcpy(level + 0xb8, &vp, sizeof(vp));

    EngineState st = {};
    InputState  in = {};
    input_reset(&in);
    EngineContext ctx = {};
    ctx.state_block = &st;
    ctx.input = &in;

    /* not bound yet: accessors report 0. */
    CHECK(engine_level_width(&ctx) == 0 && engine_level_voxel(&ctx, 0, 0) == NULL,
          "bind: unbound accessors are empty");
    ctx.level_block = level;
    CHECK(engine_level_width(&ctx) == 2 && engine_level_height(&ctx) == 1,
          "bind: width/height read from the level block");
    const uint8_t* cell = engine_level_voxel(&ctx, 0, 0);
    const uint8_t* cell1 = engine_level_voxel(&ctx, 1, 0);
    CHECK(cell == voxels && cell1 == voxels + 0x48,
          "bind: voxel pointers resolve at the corpus stride");
    CHECK(engine_level_voxel(&ctx, 2, 0) == NULL,
          "bind: out-of-bounds cell is NULL");
    CHECK(engine_voxel_byte(&ctx, 0, 0, 0x04) == 7,
          "bind: surface byte reads through");

    /* view terrain pick uses the bound voxel surface byte. */
    ViewContext v = {};
    v.mode = 1;
    v.offset[0] = 0.0f;
    v.offset[1] = 0.0f;
    view_tick(&v, &ctx, 1.0f / 30.0f, 1.0f);
    CHECK(v.cam_facing.pos[2] == 7.0f,
          "view: camera height follows the voxel surface byte");

    /* The engine-state flag one-liner (2026-10-24). */
    {
        EngineContext ec = {};
        CHECK(runtime_flag40000000_get(&ec) == 0u
                  && runtime_flag40000000_get(NULL) == 0u,
              "engine_bind: the clear word and the null guard answer zero");
        ec.state = 0x40000001u;
        CHECK(runtime_flag40000000_get(&ec) == 0x40000000u,
              "engine_bind: bit 30 reads back masked");
    }

    /* The mode-dependent field-24 read (2026-10-24). */
    {
        EngineContext ec = {};
        ViewContext vga = {};
        ViewContext vfree = {};
        vga.follow_span = 5.0f;   /* DAT_005560e4's word 0x24 */
        vfree.follow_span = 9.0f; /* DAT_005560ec's word 0x24 */
        ec.views[0] = &vga;
        ec.views[2] = &vfree;
        ec.context_mode = 1;
        CHECK(mode_dependent_field24_get(&ec) == 5,
              "engine_bind: in-game mode reads the first view");
        ec.context_mode = 0;
        CHECK(mode_dependent_field24_get(&ec) == 9,
              "engine_bind: editor mode reads the third view");
        ec.context_mode = 7;
        CHECK(mode_dependent_field24_get(&ec) == 7,
              "engine_bind: other modes answer the mode word");
        ec.views[0] = NULL;
        ec.views[2] = NULL;
        ec.context_mode = 1;
        CHECK(mode_dependent_field24_get(&ec) == 0
                  && mode_dependent_field24_get(NULL) == 0,
              "engine_bind: null view slots and context guard answer zero");
    }

    /* The 0x425-0x426 engine-state band (2026-10-24). */
    {
        EngineContext ec = {};
        CHECK(current_engine_mode_is_zero(NULL) == false
                  && current_engine_mode_is_zero(&ec) == true,
              "engine_bind: mode zero answers for the zero word and null");
        ec.context_mode = 1;
        CHECK(current_engine_mode_is_zero(&ec) == false,
              "engine_bind: a nonzero mode is not zero");
        ec.context_mode = 0;

        global_state_flags_bit7_set(&ec, 1);
        CHECK((ec.debug & 0x80u) != 0u, "engine_bind: bit 7 raises");
        global_state_flags_bit7_set(&ec, 0);
        CHECK((ec.debug & 0x80u) == 0u
                  && ec.debug == 0u,
              "engine_bind: bit 7 clears and the word stays otherwise untouched");
        global_state_flags_bit7_set(NULL, 1);

        global_scroll_guard_set(7);
        CHECK(global_scroll_guard_peek() == 7, "engine_bind: the guard stores");
        global_scroll_guard_set(0);

        uint32_t out[3] = { 1, 2, 3 };
        global_triple_copy_out(NULL);
        global_triple_copy_out(out);
        CHECK(out[0] == 0u && out[1] == 0u && out[2] == 0u,
              "engine_bind: the inert triple copies zero");
    }

    /* The app-mode read (2026-10-24). */
    EngineContext am = {};
    CHECK(engine_app_mode_get(NULL) == 0, "the null context answers zero");
    am.app_mode = 10;
    CHECK(engine_app_mode_get(&am) == 10, "the mode word reads through");
    am.app_mode = 0;


    if (s_failures == 0) {
        printf("engine_bind_test: PASS\n");
        return 0;
    }
    printf("engine_bind_test: %d failure(s)\n", s_failures);
    return 1;
}