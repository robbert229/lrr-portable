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
#include "record.hpp"
#include "crt.hpp"
#include "message.hpp"

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

        global_state_flags_bits6_and_9_set(&ec, 1, 1);
        CHECK(ec.debug == (0x40u | 0x200u),
              "engine_bind: the pair raises bit 6 and bit 9");
        global_state_flags_bits6_and_9_set(&ec, 0, 1);
        CHECK(ec.debug == 0x200u,
              "engine_bind: bit 6 clears with bit 9 kept");
        global_state_flags_bits6_and_9_set(&ec, 1, 0);
        CHECK(ec.debug == 0x40u,
              "engine_bind: bit 6 raises with bit 9 cleared");
        global_state_flags_bits6_and_9_set(&ec, 0, 0);
        CHECK(ec.debug == 0u, "engine_bind: the pair clears both");
        global_state_flags_bits6_and_9_set(NULL, 1, 1); /* guarded */

        objective_task_text_origin_set(&ec, NULL);
        CHECK((ec.debug & 0x8000u) == 0u,
              "engine_bind: the null origin clears bit 15");
        const float origin[2] = { 1.5f, 2.5f };
        objective_task_text_origin_set(&ec, origin);
        float peeked[2] = { 0.0f, 0.0f };
        objective_task_text_origin_peek(peeked);
        CHECK((ec.debug & 0x8000u) != 0u && peeked[0] == 1.5f
                  && peeked[1] == 2.5f,
              "engine_bind: the origin pair stores and raises bit 15");
        ec.debug = 0u;
        objective_task_text_origin_set(NULL, origin); /* ctx guarded */
        objective_task_text_origin_peek(NULL); /* guarded */

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


    /* The grid-cell record buffer (2026-10-24): allocate, release, replace. */
    {
        EngineState st = {};
        st.grid_w = 2;
        st.grid_h = 1;
        grid_cell_record_buffer_allocate(&st);
        CHECK(entity_field_grid_buffer_peek() != NULL,
              "allocate lands the field-grid buffer");
        CHECK(entity_field_grid_descriptor_peek() == &st,
              "the descriptor lands");
        const char* buf = (const char*)entity_field_grid_buffer_peek();
        int nonzero = 0;
        for (int i = 0; i < 2 * 1 * 0xc; ++i) {
            if (buf[i] != 0) {
                nonzero = 1;
            }
        }
        CHECK(nonzero == 0, "the buffer reads zeroed");
        *(char*)entity_field_grid_buffer_peek() = 0x5a; /* dirty the buffer */
        grid_cell_record_buffer_replace(&st);
        CHECK(entity_field_grid_buffer_peek() != NULL
                  && *(const char*)entity_field_grid_buffer_peek() == 0,
              "replace re-runs the allocating zero pass");
        entity_field_grid_buffer_release();
        CHECK(entity_field_grid_buffer_peek() == NULL,
              "release clears the word");
        entity_field_grid_buffer_release(); /* idempotent */
        grid_cell_record_buffer_allocate(NULL); /* guarded */
    }

    /* The field-grid cell window band (2026-10-25): the widened-record
     * cell parks an entity and hands its flag windows over. */
    {
        EngineState st = {};
        st.grid_w = 2;
        st.grid_h = 1;
        static unsigned char entity[0x3f0];
        memset(entity, 0, sizeof entity);
        /* Corpus mask 0xfdffffff clears bit 0x02000000 (bit 25), not
         * 0x200000; the poked word carries both so the mask shows. */
        uint32_t w = 0x02200000u;
        memcpy(entity + 0x3e0, &w, sizeof w);
        grid_cell_record_buffer_replace(&st);
        set_expansion_entity_at_grid_cell(1, 0, entity);
        const char* buf = (const char*)entity_field_grid_buffer_peek();
        float timer = 0.0f;
        memcpy(&timer, buf + 1 * 0x10 + 0x08, sizeof timer);
        CHECK(timer == 100.0f, "the parked cell timer lands");
        uint32_t flags = 0u;
        memcpy(&flags, buf + 1 * 0x10 + 0x0c, sizeof flags);
        CHECK((flags & 0x100u) != 0u, "the occupancy bit raises");
        grid_cell_invalidate_and_clear_flags(1, 0);
        uint32_t f3e8 = 0u;
        uint32_t f3e0 = 0u;
        memcpy(&f3e8, entity + 0x3e8, sizeof f3e8);
        memcpy(&f3e0, entity + 0x3e0, sizeof f3e0);
        CHECK((f3e8 & 0x800000u) != 0u && (f3e0 & 0x02000000u) == 0u
                  && (f3e0 & 0x200000u) != 0u,
              "the entity flag windows ride the invalidation");
        memcpy(&flags, buf + 1 * 0x10 + 0x0c, sizeof flags);
        CHECK((flags & 0x100u) == 0u, "the occupancy bit drops");
        grid_cell_invalidate_and_clear_flags(0, 0); /* empty slot guarded */
        entity_field_grid_buffer_release();
        set_expansion_entity_at_grid_cell(0, 0, entity); /* guarded */
    }

    /* GlobalStateLowByteClearAndRefresh (2026-10-24). */
    {
        char obj[0x20];
        memset(obj, 0, sizeof obj);
        char rec[0x10];
        memset(rec, 0, sizeof rec);
        void* p = obj;
        __builtin_memcpy(rec + 0x00, &p, sizeof p);
        scene_visibility_hook_reset();
        global_refresh_record_set(rec);
        global_state_low_word_set(0x12345678u);
        global_state_low_byte_clear_and_refresh();
        CHECK(global_state_low_word_get() == 0x12345600u,
              "the low byte clears and the high bytes stay");
        void* last = NULL;
        int flag = -1;
        scene_visibility_last_call(&last, &flag);
        CHECK(scene_visibility_calls() == 1 && last == (void*)obj && flag == 1,
              "the refresh record rides the visibility leaf with mode 1");
        global_state_low_word_set(0u);
        global_refresh_record_set(NULL);
    }

    /* GlobalHandlePositionApplyIfPresent (2026-10-24). */
    {
        global_position_handle_set(NULL);
        global_position_seam_reset();
        CHECK(global_handle_position_apply_if_present() == 0,
              "the clear handle answers 0");
        CHECK(global_position_seam_calls() == 0, "the clear handle stays off");
        char handle = 0;
        global_position_handle_set(&handle);
        CHECK(global_handle_position_apply_if_present() == 1,
              "the set handle answers 1");
        void* last = NULL;
        int lx = -1, ly = -1;
        global_position_seam_last(&last, &lx, &ly);
        CHECK(global_position_seam_calls() == 1 && last == &handle && lx == 0
                  && ly == 0,
              "the handle rides the reposition seam at zero");
        global_position_handle_set(NULL);
    }

    /* ScrollScaleSetAndClamp (2026-10-24): the gate + clamp + band. */
    {
        global_scroll_guard_set(1);
        scroll_scale_set(1.0f);
        scroll_scale_set_and_clamp(0.5f); /* backwards with the guard up */
        CHECK(scroll_scale_peek() == 1.0f, "the guard blocks a backward scale");
        scroll_scale_set_and_clamp(1.5f);
        CHECK(scroll_scale_peek() == 1.5f, "the forward scale installs");
        global_scroll_guard_set(0);
        scroll_scale_set_and_clamp(-1.0f); /* guard clear lets it through */
        CHECK(scroll_scale_peek() == 0.0f, "the negative scale clamps to zero");
        scroll_scale_set_and_clamp(9.0f);
        CHECK(scroll_scale_peek() == 3.0f, "the high scale clamps to three");

        scroll_scale_set(0.5f);
        CHECK(scroll_speed_index_get() == 0, "the sub-0.667 band answers zero");
        scroll_scale_set(1.0f);
        CHECK(scroll_speed_index_get() == 2, "the one-band lands at two");
        scroll_scale_set(1.4f);
        CHECK(scroll_speed_index_get() == 3, "the sub-1.667 band lands at three");
        scroll_scale_set(1.8f);
        CHECK(scroll_speed_index_get() == 4, "the sub-2.0 band lands at four");
        scroll_scale_set(2.0f);
        CHECK(scroll_speed_index_get() == 5, "the two-and-up band lands at five");
    }

    /* CameraFocusScrollScaleCommit (2026-10-24): gate + guard + scale. */
    {
        EngineContext ec = {};
        EngineState st = {};
        ec.state_block = &st;
        st.focus_gate_27c = 1u;
        uint32_t rec[2] = { 0u, 9u };
        global_scroll_guard_set(0);
        scroll_scale_set(1.0f);
        CHECK(camera_focus_scroll_scale_commit(&ec, rec) == 0
                  && scroll_scale_peek() == 1.0f
                  && global_scroll_guard_peek() == 0,
              "the gated zero-focus commit stays away");
        st.focus_gate_27c = 0u;
        rec[0] = 150u;
        CHECK(camera_focus_scroll_scale_commit(&ec, rec) == 0
                  && scroll_scale_peek() == 1.5f
                  && global_scroll_guard_peek() == 9,
              "the open gate installs one and a half");
        rec[0] = 200u;
        rec[1] = 3u;
        st.focus_gate_27c = 1u; /* nonzero focus ignores the gate */
        CHECK(camera_focus_scroll_scale_commit(&ec, rec) == 0
                  && scroll_scale_peek() == 2.0f
                  && global_scroll_guard_peek() == 3,
              "the live focus commits through the gate");
        rec[0] = 0u;
        CHECK(camera_focus_scroll_scale_commit(NULL, rec) == 0
                  && scroll_scale_peek() == 2.0f,
              "the null state root commits the zero focus (deviation)");
        CHECK(camera_focus_scroll_scale_commit(&ec, NULL) == 0
                  && global_scroll_guard_peek() == 3,
              "the null record answers zero (deviation)");
    }

    /* PanelSelectionHighlightToggle (2026-10-24): switch + timer + flag. */
    {
        EngineContext ec = {};
        ec.debug = 0u;
        ec.exit_timer = 0.0f;
        static char block[0x8c];
        memset(block, 0, sizeof block);
        uint32_t* w = (uint32_t*)(block + 2 * 0x2c + 0x28);
        *w = 0x20u; /* the enable gate for panel 6, item 2 */
        panel_draw_records_set(6, block);

        panel_selection_highlight_toggle(&ec, 1);
        CHECK((ec.debug & 1u) == 1u && ec.exit_timer == 250.0f && *w == 0x28u,
              "the switch raises the flag, arms the timer, sets bit 8");
        panel_selection_highlight_toggle(&ec, 0);
        CHECK((ec.debug & 1u) == 0u && ec.exit_timer == 0.0f && *w == 0x20u,
              "the switch off drops all three");
        ec.exit_timer = 100.0f;
        panel_selection_highlight_toggle(&ec, 1);
        CHECK(ec.exit_timer == 100.0f, "the live timer is not re-armed");

        ec.debug = 2u;
        panel_selection_highlight_toggle(NULL, 1); /* deviation: wordless */
        CHECK(ec.debug == 2u && *w == 0x28u, "the null context still fires the panel write");
        panel_selection_highlight_toggle(NULL, 0);
        panel_draw_records_set(6, NULL);
    }

    /* TerrainCellCommandPredicate (FUN_00428780): the system-word gate
     * chain, then the surface-byte occupancy on the bound 2x1 grid. */
    {
        EngineContext ec = {};
        ec.level_block = level;
        message_set_system_state(0u);
        CHECK(terrain_cell_command_predicate(&ec, 0, 0) == 0,
              "the dead system word answers not-allowed");
        message_set_system_state(2u);
        CHECK(terrain_cell_command_predicate(&ec, 9, 9) == 1,
              "with bits 8 and 4 clear the gate free-passes (deviation: "
              "out-of-window cells answer open too)");
        message_set_system_state(3u);
        CHECK(terrain_cell_command_predicate(&ec, 0, 0) == 1,
              "the occupied cell cannot block the free-pass branch");
        message_set_system_state(14u);
        CHECK(terrain_cell_command_predicate(&ec, 1, 0) == 1
                  && terrain_cell_command_predicate(&ec, 0, 0) == 0,
              "with bit 8 raising the occupancy branch, the empty cell "
              "opens and the occupied one blocks");
        CHECK(terrain_cell_command_predicate(&ec, 2, 0) == 0,
              "the out-of-bounds cell answers not-open (deviation)");
        message_set_system_state(13u);
        CHECK(terrain_cell_command_predicate(&ec, 1, 0) == 0,
              "bit 4 without bit 1 answers not-allowed");
        message_set_system_state(0u);
    }

    if (s_failures == 0) {
        printf("engine_bind_test: PASS\n");
        return 0;
    }
    printf("engine_bind_test: %d failure(s)\n", s_failures);
    return 1;
}