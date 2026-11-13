// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-039 ScrollInfo.c's port tail (finding 611): the port's model of
 * the information panel's scroll bar, unannotated (ADR 0006 decision 4).
 * The original-shape unit is scrollinfo.c.
 *
 * In the matching build (MSVC 5.0) scrollinfo.c defines the port's names of
 * the unit's functions itself (scrollinfo_link_names.h), so their
 * definitions here are compiled out there (LRR_SCROLLINFO_PORT_NAMES). The
 * port's track-pointer word stays in every build: ui_list.cpp and
 * pointer.cpp read it. Relocated from the verbatim info_track_overlay.cpp
 * (body hashes: docs/coverage/relocations/GAME-039.csv); SelectPlace_Create
 * (00463f00) moved to placement_footprint.cpp (GAME-040). */
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
#include "info.h"

/* Port state of 0x005013c0 */
int s_ui_track_pointer_outer_gate;

/* The port's definitions of the names scrollinfo.c takes in the matching
 * build (scrollinfo_link_names.h): every build but MSVC 5.0's. */
#if defined(_MSC_VER) && _MSC_VER < 1200
#define LRR_SCROLLINFO_PORT_NAMES 0
#else
#define LRR_SCROLLINFO_PORT_NAMES 1
#endif

#if LRR_SCROLLINFO_PORT_NAMES /* scrollinfo.c serves these names in the matching build */
/* Defined below (forward declarations added by tools/relocate_unit.py). */
static uint32_t float_bits(float f);

/* Port model of 0x00463730 */
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
/* Port model of 0x00463770 */
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

/* ViewBlitRectPositionsApply (FUN_00463d20) — see crt.hpp. Rect records:
 * +0x00/+0x04 position, +0x10/+0x14 the size pair (bit-for-bit blit
 * handles), +0x18 the flag word. The corpus strides its own block and
 * dereferences every window blindly — the guards here are the documented
 * deviation. */
/* Port model of 0x00463d20 */
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
/* Port model of 0x00463e40 */
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

/* Port model of 0x00463ea0 */
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
/* Port model of 0x00463ed0 */
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

static uint32_t float_bits(float f)
{
    uint32_t b = 0u;
    __builtin_memcpy(&b, &f, sizeof(b));
    return b;
}

/* --- InfoPanel entry points Panel.c calls (game/info.h) --------------------
 * FUN_004636b0 creates the information panel's scroll widget and
 * FUN_004638a0 tests the mouse against it. Natively these adapters serve
 * Panel.c (GAME-032, finding 554): the widget is not built, and the mouse
 * is never over it. In the matching build scrollinfo.c serves both
 * names. */
// PORT-ONLY: adapter or forwarder for the port contract
void info_panel_initialise(const char* name, int a, int b, int panel, int c)
{
    (void)name;
    (void)a;
    (void)b;
    (void)panel;
    (void)c;
}

// PORT-ONLY: adapter or forwarder for the port contract
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
#endif /* LRR_SCROLLINFO_PORT_NAMES */
