// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-039 GAME-039: the unit's functions in original address order and the
 * state they own, relocated verbatim by tools/relocate_unit.py (ADR 0006).
 * Body hashes: docs/coverage/relocations/GAME-039.csv. */
#include <math.h>
#include <float.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "gods98/texture.hpp"
#include "crt/crt.hpp"
#include "heap.hpp"
#include "string.hpp"
#include "crt/crt_for_info_track_overlay.hpp"
#include <stdarg.h>
#include <time.h>
#include "frontend.hpp"
#include "frontend.h"
#include "panel_unit.h"
#include "objective.hpp"
#include "level.hpp"
#include "entity.hpp"
#include "terrain_feedback.hpp"
#include "port_loop_ports.hpp"
#include "effects.hpp"
#include "savegame.hpp"
#include "gods98/audio.hpp"
#include "gods98/engine.hpp"
#include "gods98/simtick.hpp"
#include "gods98/message.hpp"
#include "gods98/flic.hpp"
#include "gods98/media.hpp"
#include "gods98/record.hpp"
#include "gods98/surface.hpp"
#include "gods98/sound.hpp"
#include "gods98/block.hpp"
#include "gods98/input.hpp"
#include "util.hpp"
#include "select.hpp"
#include "cmd.hpp"
#include "priority_config_ports.hpp"
#include "tutorial_hud_ports.hpp"
#include "ui_action_dispatch.hpp"
#include "simulation_ui_ports.hpp"
#include "camera_drag.hpp"
#include "screen_pick.hpp"
#include "pointer_cursor.hpp"
#include "ui_list.hpp"
#include "gods98/view.hpp"
#include "gods98/resource.hpp"
#include "frontend_for_info_track_overlay.hpp"
#include <stdint.h>
#include "ai.hpp"
#include "render.hpp"
#include "scene_render.hpp"
#include "terrain_grid.hpp"
#include "priorities.h"
#include "ui_list_for_info_track_overlay.hpp"
#include "scene_resources.hpp"
#include "scene_pool.hpp"
#include "model.hpp"
#include "view_for_info_track_overlay.hpp"

// GLOBAL: LEGORR 0x005013c0
int s_ui_track_pointer_outer_gate;

/* Defined below (forward declarations added by tools/relocate_unit.py). */
static uint32_t float_bits(float f);
static void scene_quad_grid_vertices_build(uint32_t quad, float vertices[4][9]);

// FUNCTION: LEGORR 0x00463730
void indexed_float_pair_set(int index, uint32_t x, uint32_t y)
{
    if (index < 0 || index >= DAT_WINDOW_COUNT) {
        return; /* deviation: the corpus strides blindly */
    }
    s_fpair_a[index] = (float)x; /* the corpus's uint->float cast */
    s_fpair_b[index] = (float)y;
}

/* ScrollRectResourceRecordCreate (FUN_00463770) — see frontend.hpp. The
 * view-window store preserves the window's other fields (peek/set), and
 * the surface-load seam (FUN_0047d9d0 with the handle and zero positions)
 * answers NULL (the panel-load precedent), so the dim halves stay zero
 * (deviation). */
// FUNCTION: LEGORR 0x463770
void scroll_rect_resource_record_create(int view, int mode,
                                        const float* quad, int tex_a,
                                        int tex_b)
{
    float* pf = (float*)engine_heap_alloc(0x1c);
    if (pf == NULL) {
        return; /* deviation: the corpus strides the store blindly */
    }
    if (quad != NULL) {
        pf[0] = quad[0];
        pf[1] = quad[1];
        pf[2] = quad[2] + quad[0];
        pf[3] = quad[3] + quad[1];
    } else {
        pf[0] = 0.0f;
        pf[1] = 0.0f;
        pf[2] = 0.0f;
        pf[3] = 0.0f;
    }
    pf[4] = 0.0f; /* (float)FUN_0047d9d0(tex_a,0,0): seam answers NULL */
    pf[5] = 0.0f; /* (float)FUN_0047d9d0(tex_b,0,0): seam answers NULL */
    pf[6] = 0.0f;

    void* rect_a = NULL;
    void* rect_b = NULL;
    uint32_t handle = 0u;
    int region = 0;
    view_blit_window_peek(view, &handle, &rect_a, &rect_b, &region);
    if (mode == 0) {
        rect_a = pf;
    } else {
        rect_b = pf;
    }
    view_blit_window_set(view, handle, rect_a, rect_b, region);
}

namespace unported_ledger_stubs {
// SimTrackCursorHitTest
// FUNCTION: LEGORR 0x004638a0
int ledger_stub_004638a0_SimTrackCursorHitTest(
    UiTrackCursorState* state, int x, int y, int mode, int allow_mode_change,
    const UiTrackCursorHitHooks* hooks, unsigned int* slot_index_out)
{
    return ui_sim_track_cursor_hit_test(state, x, y, mode,
                                        allow_mode_change, hooks,
                                        slot_index_out);
}
} // namespace unported_ledger_stubs

/* ViewBlitRectPositionsApply (FUN_00463d20) — see crt.hpp. Rect records:
 * +0x00/+0x04 position, +0x10/+0x14 the size pair (bit-for-bit blit
 * handles), +0x18 the flag word. The corpus strides its own block and
 * dereferences every window blindly — the guards here are the documented
 * deviation. */
// FUNCTION: LEGORR 0x00463d20
void view_blit_rect_positions_apply(int view)
{
    if (view < 0 || view >= DAT_WINDOW_COUNT) {
        return;
    }
    const int k = s_view_region[view];
    if (k < 0 || k >= PANEL_TYPE_COUNT) {
        return; /* deviation: the corpus strides its own block blindly */
    }
    const float ox = s_fpair_a[view] + panelGlobs.panelTable[k].xCurr;
    const float oy = s_fpair_b[view] + panelGlobs.panelTable[k].yCurr;
    crt_surface_blit_record(s_view_handle[view], ox, oy);

    for (int which = 0; which <= 1; ++which) {
        const float* pf = (const float*)(which == 0 ? s_view_rect_a[view]
                                                   : s_view_rect_b[view]);
        if (pf == NULL) {
            continue; /* deviation: the corpus reads blindly */
        }
        uint32_t flags = 0u;
        __builtin_memcpy(&flags, pf + 6, sizeof(flags));
        uint32_t handle = 0u;
        if (((flags & 2u) == 0u) || (pf[4] == 0.0f)) {
            if ((flags & 4u) != 0u && pf[5] != 0.0f) {
                handle = float_bits(pf[5]);
            } else {
                continue;
            }
        } else {
            handle = float_bits(pf[4]);
        }
        crt_surface_blit_record(handle, pf[0] + ox, pf[1] + oy);
    }

    const float* pf = (const float*)s_metric_records[view];
    if (pf == NULL) {
        return; /* deviation: the corpus dereferences its slot blindly */
    }
    crt_surface_blit_record(float_bits(pf[6]),
                            pf[0] + pf[4] + ox, pf[1] + pf[5] + oy);
}

/* FloatRoundToNearestInt (FUN_00463e40): the caller uses the returned
 * integer as a row index. The corpus compares the fractional part against
 * 0.5, choosing floor below it and ceil at/above it (ties toward +infinity). */
// FUNCTION: LEGORR 0x00463e40
int float_round_to_nearest_int(float value)
{
    const double value_double = (double)value;
    if (value_double - crt_round_to_integral_with_math_error_handling(
            value_double) < 0.5) {
        return crt_float_to_long(
            crt_round_to_integral_with_math_error_handling(value_double));
    }
    return crt_float_to_long(
        crt_round_to_integral_with_math_error_handling_variant(value_double));
}

// FUNCTION: LEGORR 0x00463ea0
void indexed_text_metrics_pair_get(int index, uint32_t* out_x,
                                   uint32_t* out_y)
{
    if (index < 0 || index >= DAT_WINDOW_COUNT
        || out_x == NULL || out_y == NULL) {
        return;
    }
    const char* rec = (const char*)s_metric_records[index];
    if (rec == NULL) {
        return;
    }
    __builtin_memcpy(out_x, rec + 0x1c, sizeof(*out_x));
    __builtin_memcpy(out_y, rec + 0x20, sizeof(*out_y));
}
// FUNCTION: LEGORR 0x00463ed0
void indexed_text_metrics_pair_set(int index, uint32_t x, uint32_t y)
{
    if (index < 0 || index >= DAT_WINDOW_COUNT) {
        return;
    }
    char* rec = (char*)s_metric_records[index];
    if (rec == NULL) {
        return; /* deviation: the corpus dereferences its slot blindly */
    }
    __builtin_memcpy(rec + 0x1c, &x, sizeof(x));
    __builtin_memcpy(rec + 0x20, &y, sizeof(y));
    const uint32_t one = 1u; /* the corpus's dirty flag */
    __builtin_memcpy(rec + 0x24, &one, sizeof(one));
}

// SceneQuadGridEmit
// FUNCTION: LEGORR 0x00463f00
SceneQuadGridRecord* scene_quad_grid_emit(
    void* scene, uint32_t parameter, const SceneQuadGridEmitHooks* hooks)
{
    static const uint32_t base_indices[6] = { 0u, 1u, 3u, 1u, 2u, 3u };
    uint32_t indices[30];
    uint16_t index_words[60];
    SceneQuadGridRecord* record;
    uint32_t face;
    if (hooks == NULL || hooks->mesh_create == NULL ||
        hooks->mesh_release == NULL ||
        hooks->face_vertices_emit == NULL ||
        hooks->face_texture_bind_by_mode == NULL ||
        hooks->scene_render_lock_toggle == NULL)
        return NULL;
    record = (SceneQuadGridRecord*)engine_heap_alloc(
        (long)sizeof(*record));
    if (record == NULL) return NULL;
    record->mesh = NULL;
    record->parameter = parameter;
    record->mesh = hooks->mesh_create(hooks->context, scene, 3);
    if (record->mesh == NULL) {
        engine_heap_free(record);
        return NULL;
    }
    for (face = 0; face < 5u; ++face) {
        uint32_t i;
        for (i = 0; i < 6u; ++i)
            indices[face * 6u + i] = base_indices[i] + face * 4u;
    }
    for (face = 0; face < 10u; ++face) {
        uint32_t quad;
        uint32_t index;
        int group;
        /* FUN_00463f00 appends a 20-vertex record for each face group before
         * filling that group's five quads. The retail index pointer is a
         * dword table; the in-memory pool adapter consumes the low word of
         * each dword through its two-word source stride. */
        for (index = 0u; index < 30u; ++index) {
            index_words[index * 2u] = (uint16_t)indices[index];
            index_words[index * 2u + 1u] = 0u;
        }
        if (hooks->mesh_record_allocate_or_append != NULL) {
            group = hooks->mesh_record_allocate_or_append(
                hooks->context, record->mesh, 20u, 10u, 3u, indices);
        }
        else {
            group = mesh_record_allocate_or_append(
                record->mesh, 20u, 10u, 3u, index_words);
        }
        /* A fresh mesh must append in face order; every later write uses
         * that group index. Reject stale or incomplete adapter storage. */
        if (group != (int)face) {
            scene_quad_grid_release(record, hooks);
            return NULL;
        }
        for (quad = 0; quad < 5u; ++quad) {
            float vertices[4][9];
            scene_quad_grid_vertices_build(quad, vertices);
            if (!hooks->face_vertices_emit(hooks->context, record->mesh,
                                           face, quad, vertices)) {
                scene_quad_grid_release(record, hooks);
                return NULL;
            }
        }
        if (!hooks->face_texture_bind_by_mode(hooks->context, record->mesh,
                                              face, 3u)) {
            scene_quad_grid_release(record, hooks);
            return NULL;
        }
    }
    if (!hooks->scene_render_lock_toggle(hooks->context, record->mesh, 1)) {
        scene_quad_grid_release(record, hooks);
        return NULL;
    }
    return record;
}

static uint32_t float_bits(float f)
{
    uint32_t b = 0u;
    __builtin_memcpy(&b, &f, sizeof(b));
    return b;
}

static void scene_quad_grid_vertices_build(uint32_t quad,
                                           float vertices[4][9])
{
    static const float position_components[4][2] = {
        { 0.0f, 0.0f }, { 1.0f, 0.0f },
        { 1.0f, 1.0f }, { 0.0f, 1.0f }
    };
    static const float face_normals[5][3] = {
        { 0.0f, 0.0f, -1.0f },
        { 0.0f, 1.0f, 0.0f },
        { 1.0f, 0.0f, 0.0f },
        { 0.0f, -1.0f, 0.0f },
        { -1.0f, 0.0f, 0.0f }
    };
    uint32_t vertex;
    uint32_t color_word = 0xffffffffu;

    /* The output record is three indeterminate words, normal xyz, the two
     * unit-square components in y,x order, then the opaque word. Clear only
     * the three untouched words on the portable path. */
    memset(vertices, 0, 4u * 9u * sizeof(float));
    for (vertex = 0u; vertex < 4u; ++vertex) {
        vertices[vertex][3] = face_normals[quad][0];
        vertices[vertex][4] = face_normals[quad][1];
        vertices[vertex][5] = face_normals[quad][2];
        vertices[vertex][6] = position_components[vertex][0];
        vertices[vertex][7] = position_components[vertex][1];
        memcpy(&vertices[vertex][8], &color_word, sizeof(color_word));
    }
}

/* --- InfoPanel entry points Panel.c calls (game/info.h) --------------------
 * FUN_004636b0 creates the information panel's scroll widget and
 * FUN_004638a0 tests the mouse against it. The port has no entries at
 * these addresses; these unannotated adapters (ADR 0005 step 6) serve
 * Panel.c (GAME-032, finding 554) until this unit is converted: the widget
 * is not built, and the mouse is never over it. */
#include "info.h"

void info_panel_initialise(const char* name, int a, int b, int panel, int c)
{
    (void)name;
    (void)a;
    (void)b;
    (void)panel;
    (void)c;
}

int info_panel_check_collision(int* released, int mouseX, int mouseY,
                               int leftButton, int leftButtonLast)
{
    (void)mouseX;
    (void)mouseY;
    (void)leftButton;
    (void)leftButtonLast;
    *released = 0;
    return 0;
}
