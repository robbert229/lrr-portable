// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-018 HelpWindow.c: the unit's functions in original address order and the
 * state they own, relocated verbatim by tools/relocate_unit.py (ADR 0006).
 * Body hashes: docs/coverage/relocations/GAME-018.csv. */
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include "frontend.hpp"
#include "frontend.h"
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
#include "crt/crt.hpp"
#include "gods98/heap.hpp"
#include "gods98/record.hpp"
#include "gods98/texture.hpp"
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
#include "gods98/string.hpp"
#include "gods98/resource.hpp"
#include <stdint.h>
#include "terrain_grid.hpp"
#include "dependency_grid_shared.hpp"
#include "upgrades.hpp"
#include <limits.h>
#include "frontend_for_helpwindow.hpp"
#include "upgrades_for_helpwindow.hpp"

#define LAYOUT_POSITION_STRIDE 7    /* corpus advances 0x1c bytes per entry */

typedef union HelpWindowLayoutWord {
    uint32_t u;
    int32_t i;
    float f;
} HelpWindowLayoutWord;

/* SelectionModeStateSet (FUN_00418190) — zero-init module static (the
 * corpus's DAT_004db8e8 word; its corpus readers are unported). */
// GLOBAL: LEGORR 0x004db8e8
uint32_t s_sel_mode_state;
// GLOBAL: LEGORR 0x004db8f8
static uint32_t s_help_background_handle;
// GLOBAL: LEGORR 0x004db904
static HelpWindowLayoutWord s_layout_position[LAYOUT_POSITION_ENTRIES *
                                              LAYOUT_POSITION_STRIDE];
// GLOBAL: LEGORR 0x004db914
uint32_t s_sel_handles_a[SEL_HANDLE_ENTRIES * SEL_HANDLE_STRIDE];
// GLOBAL: LEGORR 0x004db918
uint32_t s_sel_handles_b[SEL_HANDLE_ENTRIES * SEL_HANDLE_STRIDE];
// GLOBAL: LEGORR 0x004db934
uint32_t s_sel_handle_extra;             /* DAT_004db934 */

// GLOBAL: LEGORR 0x004db974
uintptr_t s_help_text_area_handle;

/* HelpWindowInfoConfigStringsLoad fills three adjacent 15-by-16 pointer
 * tables. The catalog loader owns the name tables/counts; module-owned text
 * storage below supplies the mutable copies for these pointer rows. */
// GLOBAL: LEGORR 0x004db978
const char* s_help_info_strings[3][15 * 16];

// GLOBAL: LEGORR 0x004dc640
float s_help_window_feedback_position;
// GLOBAL: LEGORR 0x004dc644
float s_help_window_feedback_limit;
// GLOBAL: LEGORR 0x004dc648
int      s_sel_hit_index;                /* DAT_004dc648 */

// GLOBAL: LEGORR 0x004dc64c
int      s_band_mode;
// GLOBAL: LEGORR 0x004dc650
uint32_t s_sel_state;                    /* DAT_004dc650 */

static const char* s_layout_position_text[LAYOUT_POSITION_ENTRIES];
static char s_layout_position_text_storage[LAYOUT_POSITION_ENTRIES][256];

static float s_help_background_offset_x;
static float s_help_background_offset_y;

static void* s_help_text_area_native_record;
static char s_help_window_labels[3][256];

/* ConfigStringCopy in the retail loader gives each HelpWindowInfo line its
 * own mutable string before replacing underscores. Keep those copies in
 * module-owned storage so this loader does not modify BlockFile-owned text. */
static char s_help_info_string_storage[3][15 * 16][1024];

static void help_transition_control_noop(int a, int b);

static void help_window_button_layout_load_with_ops(
    BlockFile* cfg, const FeHelpWindowResourceOps* ops);

/* Defined below (forward declarations added by tools/relocate_unit.py). */
static int fe_parse_int_tuple(const char* text, int* out, int count);
void help_window_info_config_strings_load( BlockFile* cfg, const char* const* const group_names[3], const size_t group_counts[3], void (*store)(void* context, int group, size_t catalog_index, int line_index, const char* text), void* context);
static void help_window_info_config_strings_load_native(BlockFile* cfg);
static void help_window_label_copy(char* out, size_t out_size, const char* value);
static int help_window_info_path_build(char* out, size_t out_size, const char* catalog, int line_index);

// FUNCTION: LEGORR 0x418190
void selection_mode_state_set(uint32_t value)
{
    s_sel_mode_state = value;
}
// FUNCTION: LEGORR 0x004181a0
void selection_state_flag_clear_bit0(void)
{
    s_sel_state = s_sel_state & ~1u;
}

// FUNCTION: LEGORR 0x004181b0
void help_window_resources_init(BlockFile* cfg,
                                const FeHelpWindowResourceOps* ops)
{
    if (ops != NULL && ops->info_strings_load != NULL) {
        ops->info_strings_load(ops->context, cfg);
    } else if (ops != NULL && ops->info_string_store != NULL) {
        help_window_info_config_strings_load(
            cfg, ops->info_group_names, ops->info_group_counts,
            ops->info_string_store, ops->context);
    } else {
        help_window_info_config_strings_load_native(cfg);
    }

    const char* background = block_get_strptr(cfg,
        "HelpWindow::BackGroundImage");
    if (background != NULL) {
        const char* first_separator = strchr(background, ':');
        const char* second_separator = first_separator != NULL
            ? strchr(first_separator + 1, ':') : NULL;
        if (first_separator != NULL && second_separator != NULL &&
            first_separator != background && second_separator[1] != '\0') {
            char name[256];
            const size_t length = (size_t)(first_separator - background);
            if (length < sizeof(name)) {
                memcpy(name, background, length);
                name[length] = '\0';
                s_help_background_offset_x = (float)atoi(first_separator + 1);
                s_help_background_offset_y = (float)atoi(second_separator + 1);
                s_help_background_handle = ops != NULL &&
                    ops->resource_load != NULL
                    ? ops->resource_load(ops->context, name, 0, 0)
                    : surface_load_handle(name);
                if (s_help_background_handle != 0u && ops != NULL &&
                    ops->background_apply != NULL) {
                    ops->background_apply(ops->context,
                                          s_help_background_handle);
                }
            }
        }
    }

    help_window_button_layout_load_with_ops(cfg, ops);

    const char* text_area = block_get_strptr(cfg,
        "HelpWindow::TextWindowArea");
    const uintptr_t help_font_handle = (uintptr_t)
        resource_name_cache_get_or_load("Interface\\Fonts\\FONT5_HI.bmp");
    int area[4];
    if (fe_parse_int_tuple(text_area, area, 4)) {
        if (ops != NULL && ops->text_area_create != NULL) {
            s_help_text_area_handle = (uintptr_t)ops->text_area_create(
                ops->context, help_font_handle, area, 0x400u);
            s_help_text_area_native_record = NULL;
        } else {
            float area_f[4];
            uint32_t area_words[4];
            int i;
            for (i = 0; i < 4; ++i) {
                area_f[i] = (float)area[i];
                memcpy(&area_words[i], &area_f[i], sizeof(area_words[i]));
            }
            s_help_text_area_native_record = text_buffer_create(
                help_font_handle, area_words, 0x400);
            s_help_text_area_handle =
                (uintptr_t)s_help_text_area_native_record;
        }
    }

    static const char* const label_keys[3] = {
        "HelpWindow::CanBuildMessage",
        "HelpWindow::CanTransportMessage",
        "HelpWindow::CanBuildAndTransportMessage"
    };
    for (int i = 0; i < 3; ++i) {
        help_window_label_copy(s_help_window_labels[i],
                               sizeof(s_help_window_labels[i]),
                               block_get_strptr(cfg, label_keys[i]));
        if (ops != NULL && ops->label_set != NULL) {
            ops->label_set(ops->context, i, s_help_window_labels[i]);
        }
    }
    s_sel_state |= 6u;
    ui_mode_resource_bit40_set_clear(0, 0u);
    if (ops != NULL && ops->refresh != NULL) {
        ops->refresh(ops->context, 0, 0);
    }
}
// FUNCTION: LEGORR 0x00418450
void help_window_info_config_strings_load(
    BlockFile* cfg, const char* const* const group_names[3],
    const size_t group_counts[3],
    void (*store)(void* context, int group, size_t catalog_index,
                  int line_index, const char* text),
    void* context)
{
    if (group_names == NULL || group_counts == NULL || store == NULL) {
        return;
    }
    for (int group = 0; group < 3; ++group) {
        if (group_names[group] == NULL) {
            continue;
        }
        for (size_t catalog_index = 0;
             catalog_index < group_counts[group]; ++catalog_index) {
            const char* const catalog = group_names[group][catalog_index];
            for (int line_index = 0; line_index < 16; ++line_index) {
                const char* value = NULL;
                char path[1024];
                if (help_window_info_path_build(path, sizeof(path), catalog,
                                                line_index))
                    value = block_get_strptr(cfg, path);

                char text[1024];
                if (value != NULL) {
                    help_window_label_copy(text, sizeof(text), value);
                    store(context, group, catalog_index, line_index, text);
                } else {
                    store(context, group, catalog_index, line_index, NULL);
                }
            }
        }
    }
}

// FUNCTION: LEGORR 0x004185f0
void help_window_button_layout_load(BlockFile* cfg, BlockFile* config_root)
{
    /* The root is folded into the port's BlockFile path lookup. Native
     * surfaces are loaded through the frontend resource cache. */
    (void)config_root;
    help_window_button_layout_load_with_ops(cfg, NULL);
}

/* Corpus shape: one register copy; the true arm ORs bit 1 then masks
 * bit 0, the false arm masks bit 1 then bit 0 as two separate ands; each
 * arm stores and exits on its own. MSVC5 tail-merges the two C stores and
 * folds the false arm's masks into 0xfc, so the body is emitted in naked
 * asm to pin the original's line-for-line sequence. */
// FUNCTION: LEGORR 0x00418800
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void mode_flag_pair_apply_if_bit2(int value)
{
    __asm {
        mov eax, s_sel_state
        test al, 4
        je   done
        mov ecx, dword ptr [esp + 4]
        test ecx, ecx
        je   clear_both
        or   al, 2
        and  al, 0xfe
        mov  s_sel_state, eax
        ret
    clear_both:
        and  al, 0xfd
        and  al, 0xfe
        mov  s_sel_state, eax
    done:
        ret
    }
}
#else
void mode_flag_pair_apply_if_bit2(int value)
{
    if ((s_sel_state & 4u) == 0u) return;
    if (value != 0) {
        s_sel_state |= 2u;
        s_sel_state &= ~1u;
    }
    else {
        s_sel_state &= ~2u;
        s_sel_state &= ~1u;
    }
}
#endif

namespace lego {
// FUNCTION: LEGORR 0x00418830
void building_upgrade_state_changed_dispatch(int type, int row, int column,
                                             int state)
{
    if (s_building_upgrade_begin_hook != 0) s_building_upgrade_begin_hook();
    else unported_ledger_stubs::dependency_grid_occupancy_pass();
    if (s_building_upgrade_changed_hook != 0) {
        if (s_building_upgrade_changed_hook(type, row, column) != 0) return;
    }
    else if (cell_flag100_is_set(type, row, column) != 0u) {
        return;
    }
    if (s_building_upgrade_apply_hook != 0) {
        s_building_upgrade_apply_hook(type, row, column);
    }
    else {
        cell_flag100_set(type, row, column);
    }
    if (state == 0) {
        if (s_building_upgrade_event_hook != 0) {
            s_building_upgrade_event_hook(type, row, column, state);
        }
        else {
            unported_ledger_stubs::dependency_grid_category_candidates_build(
                (uint32_t)type, (uint32_t)row, (uint32_t)column);
        }
    }
}
} // namespace lego

namespace unported_ledger_stubs {
// FUNCTION: LEGORR 0x00418920
void dependency_grid_category_candidates_build(uint32_t category,
                                               uint32_t row,
                                               uint32_t selector)
{
    DependencyGridMatches matches;
    DependencyGridLevelStorage* storage = dependency_grid_current_storage();
    DependencyGridCategoryCandidates* candidates;
    uint32_t index;
    if (storage == NULL) return;
    candidates = &storage->candidates;
    candidates->count = 0u;
    candidates->category = category;
    candidates->row = row;
    candidates->selector = selector;
    dependency_grid_matches_collect(category, row, selector, &matches);
    for (index = 0u; index < matches.count &&
         index < DEPENDENCY_GRID_MATCH_CAPACITY; ++index) {
        const uint32_t x = matches.x[index];
        const uint32_t y = matches.y[index];
        const uint32_t variant = matches.variant[index];
        uint32_t output_index;
        if (cell_flag4_is_set((int)x, (int)y, (int)variant) == 0u) continue;
        output_index = candidates->count;
        if (output_index < DEPENDENCY_GRID_CATEGORY_CANDIDATE_CAPACITY) {
            candidates->x[output_index] = x;
            candidates->y[output_index] = y;
            candidates->variant[output_index] = variant;
        }
        ++candidates->count;
    }
}
} // namespace unported_ledger_stubs

/* RenderTransitionCleanupIfActive (FUN_004189d0): the corpus gates two byte
 * tests of the selection-state DAT, calls TransitionControl(0, 0) and
 * ObjectiveScoreTextOriginSet unconditionally (no hook indirection), then
 * re-reads the DAT and clears bit 0. ObjectiveScoreTextOriginSet arrives with
 * a null pair and a null context, so it writes nothing and drops no debug bit
 * (the corpus's own push of a single 0 reads the same way). */
// FUNCTION: LEGORR 0x004189d0
void help_render_transition_cleanup_if_active(void)
{
    if (((unsigned char)s_sel_state & 0x2u) != 0u) {
        if (((unsigned char)s_sel_state & 0x1u) != 0u) {
            help_transition_control_noop(0, 0);
            objective_score_text_origin_set(NULL);
            s_sel_state &= ~1u;
        }
    }
}

/* OverlayTextBufferRefresh (FUN_00418a00): refresh the help overlay,
 * validate interface/input gates, render its header and scrolled entries,
 * then finish the overlay frame. */
// FUNCTION: LEGORR 0x00418a00
int overlay_text_buffer_refresh(void)
{
    OverlayTextBufferOps* ops = &s_overlay_text_buffer_ops;
    uint32_t flags;
    uint32_t input_flags;
    int count;
    int start;
    int index;
    float bottom = 0.0f;
    float y;
    float clip_y;
    int primary_drawn;
    /* The reference re-arms the transition cleanup when its bit-0x40 mode
     * handler is absent, then checks that mode again. That state is owned by
     * this module, so it does not need a host callback. */
    if (ops->ensure_initialized != NULL) {
        if (!ops->ensure_initialized(ops->context)) return 0;
    } else {
        if (ui_mode_resource_bit40_get() == 0u) {
            help_render_transition_cleanup_if_active();
        }
        if (ui_mode_resource_bit40_get() == 0u) return 0;
    }
    flags = ops->interface_flags_get != NULL ?
        ops->interface_flags_get(ops->context) : s_sel_state;
    if ((flags & 2u) == 0u || (flags & 1u) == 0u) return 0;
    input_flags = ops->input_flags_get != NULL ?
        ops->input_flags_get(ops->context) : system_state_flags_get();
    if ((input_flags & 0x800u) != 0u) return 0;
    if (ops->transition_begin != NULL)
        ops->transition_begin(ops->context);
    else
        help_transition_control_noop(0, 1);
    count = ops->entry_count_get != NULL ?
        ops->entry_count_get(ops->context) : 0;
    if (count <= 0) {
        if (ops->reset != NULL) ops->reset(ops->context);
        else help_render_transition_cleanup_if_active();
        return 1;
    }
    /* SurfaceBlitRect has a repository binding through the resource-record
     * renderer. The per-frame message/list source that decides whether this
     * background is needed is still supplied by the caller's row provider. */
    if (s_help_background_handle != 0u) {
        crt_surface_blit_record(s_help_background_handle,
                                s_help_background_offset_x,
                                s_help_background_offset_y);
    }
    primary_drawn = ops->primary_text_draw != NULL ?
        ops->primary_text_draw(ops->context, &bottom) : 0;
    if ((flags & 8u) != 0u) {
        if (primary_drawn && ops->header_draw != NULL)
            ops->header_draw(ops->context);
        if (ops->header_dirty_clear != NULL)
            ops->header_dirty_clear(ops->context);
    }
    y = bottom + 10.0f;
    if (ops->scroll_position_get != NULL)
        y += ops->scroll_position_get(ops->context);
    start = ops->scroll_start_index_get != NULL ?
        ops->scroll_start_index_get(ops->context, count) : 0;
    if (start < 0) start = 0;
    if (start >= count) start = count - 1;
    clip_y = ops->scroll_clip_y_get != NULL ?
        ops->scroll_clip_y_get(ops->context) : 0.0f;
    for (index = start; index < count; ++index) {
        OverlayTextLine line;
        memset(&line, 0, sizeof(line));
        if (ops->entry_get == NULL ||
            !ops->entry_get(ops->context, index, &line)) continue;
        if (clip_y != 0.0f && y + line.height >= clip_y) {
            if (ops->line_focus != NULL)
                ops->line_focus(ops->context, &line);
            if (ops->scroll_limit_reached != NULL)
                ops->scroll_limit_reached(ops->context);
            break;
        }
        line.y = y;
        if (ops->line_draw != NULL &&
            !ops->line_draw(ops->context, &line, line.x, y)) {
            if (ops->line_focus != NULL)
                ops->line_focus(ops->context, &line);
            break;
        }
        y += line.height + 10.0f;
    }
    if (ops->finish != NULL) ops->finish(ops->context);
    else ui_mode_resource_positions_apply();
    return 1;
}

// FUNCTION: LEGORR 0x00418da0
int help_window_button_hit_test(uint32_t x, uint32_t y, int mode,
                                int transition_flags, int active,
                                float feedback_amount)
{
    if ((s_sel_state & 3u) != 3u) {
        return 0;
    }

    s_sel_state &= ~0x30u;
    const float pointer_x = (float)x;
    const float pointer_y = (float)y;
    if (pointer_x >= 345.0f &&
        pointer_x <= (float)display_width_get() + 345.0f &&
        pointer_y >= 350.0f &&
        pointer_y <= (float)display_height_get() + 350.0f && active != 0) {
        timed_effect_update_request_set(0x21);
        help_window_button_feedback_update(3, 5.0f);
        if (s_help_window_sound_clear != NULL) {
            s_help_window_sound_clear(0);
        }
    }

    for (int i = 0; i < LAYOUT_POSITION_ENTRIES; ++i) {
        const float* const rect = s_help_window_button_rects[i];
        if (pointer_x < rect[0] || pointer_x > rect[0] + rect[2] ||
            pointer_y < rect[1] || pointer_y > rect[1] + rect[3]) {
            continue;
        }

        s_sel_state |= 0x10u;
        s_sel_hit_index = i;
        const int transitioned = ui_mode_transition_request(
            i, mode, transition_flags);
        if (transitioned != 0 && active != 0) {
            help_window_button_feedback_update(i, feedback_amount);
            if (s_help_window_sound_clear != NULL) {
                s_help_window_sound_clear(0);
            }
        }
        if (mode == 0 && active == 0) {
            selection_mode_set_by_index(i);
        }
        return 1;
    }

    (void)ui_mode_transition_request(4, mode, transition_flags);
    return 0;
}
// FUNCTION: LEGORR 0x00418f80
void selection_mode_set_by_index(int index)
{
    switch (index) {
    case 0:
        timed_effect_update_request_set(8);
        break;
    case 1:
        timed_effect_update_request_set(9);
        break;
    case 2:
    case 3:
        break;
    default:
        break;
    }
}
// FUNCTION: LEGORR 0x00418fc0
bool ui_mode_transition_request(int index, int p2, int p3)
{
    const int mode = s_band_mode;
    if (p2 == 0) {
        s_band_mode = 4;
    } else if (s_band_mode == 4) {
        if (p3 == 0) {
            s_band_mode = index;
            s_sel_state |= 0x20u;
            return index == 4;
        }
    } else if (s_band_mode == index) {
        s_sel_state |= 0x20u;
        return s_band_mode == index;
    }
    return mode == index;
}

// FUNCTION: LEGORR 0x00419030
void help_window_button_feedback_update(int action, float amount)
{
    switch (action) {
    case 0:
        help_render_transition_cleanup_if_active();
        return;
    case 1:
        ui_mode_resource_bit40_set_clear(1, 0u);
        help_render_transition_cleanup_if_active();
        return;
    case 2:
        if (0.0f < s_help_window_feedback_position) {
            s_help_window_feedback_position -= amount * 0.2f;
            if (s_help_window_feedback_position < 0.0f) {
                s_help_window_feedback_position = 0.0f;
                return;
            }
        }
        break;
    case 3:
        if (s_help_window_feedback_position < s_help_window_feedback_limit) {
            s_help_window_feedback_position += amount * 0.2f;
            if (s_help_window_feedback_limit <= s_help_window_feedback_position) {
                s_help_window_feedback_position = s_help_window_feedback_limit;
                objective_score_text_origin_set(NULL);
            }
        }
        break;
    default:
        break;
    }
}
// FUNCTION: LEGORR 0x00419100
void ui_mode_resource_positions_apply(void)
{
    float* position_ptr = (float*)((char*)s_layout_position + sizeof(uint32_t));
    int i = 0;
    do {
        float position[2];
        uint32_t handle;
        position[0] = position_ptr[-1];
        position[1] = position_ptr[0];
        handle = selection_state_handle_get(i);
        if (handle != 0u) {
            if (s_ui_resource_reposition_hook != NULL) {
                /* SurfaceBlitRect consumes the raw pair and performs the
                 * floor/__ftol conversions itself. */
                (void)s_ui_resource_reposition_hook(handle, NULL, position,
                                                    NULL);
            }
        }
        position_ptr += LAYOUT_POSITION_STRIDE;
        ++i;
    } while ((char*)position_ptr <
             (char*)s_layout_position + sizeof(s_layout_position) +
                 sizeof(uint32_t));
}
// FUNCTION: LEGORR 0x00419150
uint32_t selection_state_handle_get(int index)
{
    if (index == 1 && (s_sel_state & 4u) == 0) {
        return s_sel_handle_extra;
    }
    if ((s_sel_state & 0x10u) != 0 && s_sel_hit_index == index) {
        if ((s_sel_state & 0x20u) != 0) {
            return s_sel_handles_b[index * SEL_HANDLE_STRIDE];
        }
        return s_sel_handles_a[index * SEL_HANDLE_STRIDE];
    }
    return 0u;
}

/* MainSimStateAdmissionPredicate (FUN_004191a0) — see frontend.hpp. The
 * corpus reads bits 1/2 of the selection word directly and the 0x40 bit
 * through its own leaf; all three reduce to the same modeled word. */
// FUNCTION: LEGORR 0x4191a0
int main_sim_state_admission_predicate(void)
{
    const uint32_t ui_mode = ui_mode_resource_bit40_get();
    const uint32_t bit1 = s_sel_state & 1u;
    const uint32_t bit2 = s_sel_state & 2u;
    const uint32_t system_state = system_state_flags_get();
    if (ui_mode != 0u && bit2 != 0u && bit1 != 0u
        && system_state == 0u) {
        return 1;
    }
    return 0;
}
// FUNCTION: LEGORR 0x004191f0
uint32_t ui_mode_resource_bit40_get(void)
{
    return s_sel_state & 0x40u;
}

// FUNCTION: LEGORR 0x00419200
void ui_mode_resource_bit40_set_clear(int probe, uint32_t value)
{
    uint32_t v;
    if (probe != 0) {
        v = s_sel_state & 0x40u;
    }
    else {
        /* FUN_00419200 fetches the value argument only after the probe test. */
        v = value;
    }
    /* Corpus layout: the nonzero answer falls through to the clear, the
     * zero answer jumps to the set tail; the word answers after the
     * store in both arms. */
    if (v != 0u) {
        s_sel_state = s_sel_state & ~0x40u; /* plain form: keeps the
            * corpus's mov/and al,0xbf/mov register round-trip (b45
            * global_flag_bit0_clear lesson) */
    } else {
        s_sel_state = s_sel_state | 0x40u;
    }
}
static int fe_parse_int_tuple(const char* text, int* out, int count)
{
    if (text == NULL || out == NULL || count <= 0) {
        return 0;
    }
    const char* p = text;
    for (int i = 0; i < count; ++i) {
        const char* separator = strchr(p, ':');
        if (p[0] == '\0' || separator == p
            || (i + 1 < count && separator == NULL)) {
            return 0;
        }
        out[i] = atoi(p);
        if (i + 1 < count) {
            p = separator + 1;
        }
    }
    return 1;
}

/* TransitionControl (FUN_00425c30): the corpus's opaque transition leaf. Like
 * flic.cpp's 0x00484c90 fallback, the port carries a file-local no-op so the
 * call site lands the corpus's two pushes; the ledger_stubs entry stays the
 * link-time no-op for the unported callers. */
static void help_transition_control_noop(int a, int b)
{
    (void)a;
    (void)b;
    /* FUN_00425c30: pure return — faithful. */
}

static int help_window_button_record_parse(const char* value, int* x, int* y,
                                           int* width, int* height,
                                           const char** resource_a,
                                           const char** resource_b,
                                           const char** text)
{
    if (value == NULL || x == NULL || y == NULL || width == NULL ||
        height == NULL || resource_a == NULL || resource_b == NULL ||
        text == NULL) {
        return 0;
    }
    const char* cursor = value;
    int* dimensions[4] = { x, y, width, height };
    for (int i = 0; i < 4; ++i) {
        const char* separator = strchr(cursor, ':');
        if (separator == NULL || separator == cursor) {
            return 0;
        }
        const int parsed = atoi(cursor);
        *dimensions[i] = parsed;
        cursor = i < 3 ? separator + 1 : separator;
    }
    if (*cursor != ':') {
        return 0;
    }
    *resource_a = ++cursor;
    const char* separator = strchr(cursor, ':');
    if (separator == NULL || separator == cursor) {
        return 0;
    }
    cursor = separator + 1;
    *resource_b = cursor;
    separator = strchr(cursor, ':');
    if (separator == NULL || separator == cursor || separator[1] == '\0') {
        return 0;
    }
    *text = separator + 1;
    return 1;
}

static void help_window_label_copy(char* out, size_t out_size,
                                   const char* value)
{
    if (out == NULL || out_size == 0) {
        return;
    }
    if (value == NULL) {
        out[0] = '\0';
        return;
    }
    const size_t copy_limit = out_size - 1;
    const char* terminator = (const char*)memchr(value, '\0', copy_limit);
    size_t length = terminator != NULL
        ? (size_t)(terminator - value) : copy_limit;
    strncpy(out, value, length);
    out[length] = '\0';
    char* underscore = out;
    while ((underscore = crt_strchr(underscore, '_')) != NULL) {
        *underscore++ = ' ';
    }
}

/* The HelpWindowInfo key has a fixed prefix/suffix and a one- or two-digit
 * line number. Build it with bounded copies so this path remains usable by
 * the VC5 toolchain as well as the native build. */
static int help_window_info_path_build(char* out, size_t out_size,
                                       const char* catalog, int line_index)
{
    static const char prefix[] = "HelpWindow::HelpWindowInfo::";
    static const char suffix[] = "L";
    size_t prefix_size = sizeof(prefix) - 1u;
    size_t catalog_size;
    size_t number_size;
    size_t needed;
    size_t cursor;
    if (out == NULL || catalog == NULL || line_index < 0 ||
        line_index > 99) return 0;
    catalog_size = strlen(catalog);
    number_size = line_index >= 10 ? 2u : 1u;
    needed = prefix_size + catalog_size + 1u + number_size + 1u;
    if (needed > out_size) return 0;
    memcpy(out, prefix, prefix_size);
    cursor = prefix_size;
    memcpy(out + cursor, catalog, catalog_size);
    cursor += catalog_size;
    memcpy(out + cursor, suffix, 1u);
    cursor += 1u;
    if (number_size == 2u) {
        out[cursor++] = (char)('0' + line_index / 10);
        line_index %= 10;
    }
    out[cursor++] = (char)('0' + line_index);
    out[cursor] = '\0';
    return 1;
}

static void help_window_info_config_strings_load_native(BlockFile* cfg)
{
    static const int catalog_kinds[3] = { 0, 1, 3 };
    int group;
    memset(s_help_info_strings, 0, sizeof(s_help_info_strings));
    for (group = 0; group < 3; ++group) {
        const int kind = catalog_kinds[group];
        const char* const* names = (const char* const*)
            catalog_name_table_peek(kind);
        uint32_t count = catalog_count_peek(kind);
        uint32_t catalog_index;
        if (count > 15u) count = 15u;
        if (names == NULL) continue;
        for (catalog_index = 0; catalog_index < count; ++catalog_index) {
            int line_index;
            for (line_index = 0; line_index < 16; ++line_index) {
                char path[1024];
                const char* value = NULL;
                const char* catalog = names[catalog_index];
                if (help_window_info_path_build(path, sizeof(path), catalog,
                                                line_index)) {
                    value = block_get_strptr(cfg, path);
                }
                if (value != NULL) {
                    char* copy = s_help_info_string_storage[group]
                        [catalog_index * 16u + (uint32_t)line_index];
                    help_window_label_copy(copy, 1024u, value);
                    s_help_info_strings[group][catalog_index * 16u +
                        (uint32_t)line_index] = copy;
                }
            }
        }
    }
}

static void help_window_button_layout_load_with_ops(
    BlockFile* cfg, const FeHelpWindowResourceOps* ops)
{
    static const char* const keys[LAYOUT_POSITION_ENTRIES] = {
        "HelpWindow::ContinueButton", "HelpWindow::CancelFutureButton"
    };
    for (int i = 0; i < LAYOUT_POSITION_ENTRIES; ++i) {
        const char* const value = block_get_strptr(cfg, keys[i]);
        int x, y, width, height;
        const char *resource_a, *resource_b, *text;
        if (!help_window_button_record_parse(value, &x, &y, &width, &height,
                                             &resource_a, &resource_b, &text)) {
            continue;
        }
        const float final_x = (float)x + s_help_background_offset_x;
        const float final_y = (float)y + s_help_background_offset_y;

        char first[256], second[256];
        const char* end = strchr(resource_a, ':');
        size_t length = end != NULL ? (size_t)(end - resource_a) : 0;
        if (length == 0 || length >= sizeof(first)) {
            continue;
        }
        memcpy(first, resource_a, length);
        first[length] = '\0';
        end = strchr(resource_b, ':');
        length = end != NULL ? (size_t)(end - resource_b) : 0;
        if (length == 0 || length >= sizeof(second)) {
            continue;
        }
        memcpy(second, resource_b, length);
        second[length] = '\0';

        help_window_button_rect_set(i, final_x, final_y,
                                    (float)width, (float)height);
        s_layout_position[i * LAYOUT_POSITION_STRIDE + 0].f = final_x;
        s_layout_position[i * LAYOUT_POSITION_STRIDE + 1].f = final_y;
        s_layout_position[i * LAYOUT_POSITION_STRIDE + 2].f = (float)width;
        s_layout_position[i * LAYOUT_POSITION_STRIDE + 3].f = (float)height;
        s_layout_position[i * LAYOUT_POSITION_STRIDE + 4].u = ops != NULL &&
            ops->resource_load != NULL
            ? ops->resource_load(ops->context, first, 0, 0) : 0u;
        s_layout_position[i * LAYOUT_POSITION_STRIDE + 5].u = ops != NULL &&
            ops->resource_load != NULL
            ? ops->resource_load(ops->context, second, 0, 0) : 0u;
        if (ops == NULL || ops->resource_load == NULL) {
            s_layout_position[i * LAYOUT_POSITION_STRIDE + 4].u =
                surface_load_handle(first);
            s_layout_position[i * LAYOUT_POSITION_STRIDE + 5].u =
                surface_load_handle(second);
        }
        help_window_label_copy(s_layout_position_text_storage[i],
                               sizeof(s_layout_position_text_storage[i]),
                               text);
        s_layout_position_text[i] = s_layout_position_text_storage[i];
        if (ops != NULL && ops->text_load != NULL) {
            ops->text_load(ops->context, i == 0 ? 8 : 9,
                           s_layout_position_text[i]);
        }
    }
}

/* Original-ABI adapter (ADR 0005 step 6) for GAME-021's lego_unit.c: HelpWindow_Initialise (004181b0) takes the config and the game name; the port's entry takes a resource-op table. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (help_window_resources_init)
extern "C" void help_window_initialise_adapter(struct BlockRec* config,
                                               const char* gameName)
{
    (void)gameName;
    help_window_resources_init(config, NULL);
}
