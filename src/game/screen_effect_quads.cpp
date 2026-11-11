// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-009 GAME-009: the unit's functions in original address order and the
 * state they own, relocated verbatim by tools/relocate_unit.py (ADR 0006).
 * Body hashes: docs/coverage/relocations/GAME-009.csv. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "message.hpp"
#include "simtick.hpp"
#include "sound.hpp"
#include "heap.hpp"
#include "crt/crt.hpp"
#include "resource.hpp"
#include "game/level.hpp"
#include "game/ai.hpp"
#include "game/cmd.hpp"
#include "game/select.hpp"
#include "game/entity.hpp"
#include "game/effects.hpp"
#include "game/render.hpp"
#include "game/terrain_grid.hpp"
#include "view.hpp"
#include <float.h>
#include <ctype.h>
#include <errno.h>
#include "gods98/texture.hpp"
#include "string.hpp"
#include "crt/crt_for_screen_effect_quads.hpp"
#include <stdint.h>
#include "building_scene.hpp"
#include "actor_scene.hpp"
#include "type_config_ports.hpp"
#include "entity_pose_delta.hpp"
#include "entity_motion_ports.hpp"
#include "activity_timeout.hpp"
#include "entity_motion_probe.hpp"
#include "port_loop_ports.hpp"
#include "simulation_ui_ports.hpp"
#include "type_motion_scale.hpp"
#include "entity_quad_collision.hpp"
#include "spawn.hpp"
#include "weapon.hpp"
#include "gods98/model.hpp"
#include "gods98/engine.hpp"
#include "gods98/record.hpp"
#include "gods98/block.hpp"
#include "upgrades.hpp"
#include "frontend.hpp"
#include "util.hpp"
#include "gods98/media.hpp"
#include "entity_for_screen_effect_quads.hpp"
#include <stdarg.h>
#include <time.h>
#include "frontend.h"
#include "objective.hpp"
#include "terrain_feedback.hpp"
#include "savegame.hpp"
#include "gods98/audio.hpp"
#include "gods98/flic.hpp"
#include "gods98/surface.hpp"
#include "gods98/input.hpp"
#include "priority_config_ports.hpp"
#include "tutorial_hud_ports.hpp"
#include "ui_action_dispatch.hpp"
#include "camera_drag.hpp"
#include "screen_pick.hpp"
#include "pointer_cursor.hpp"
#include "ui_list.hpp"
#include <limits.h>
#include "game.hpp"
#include "gods98/camera.hpp"
#include "scene_render.hpp"
#include "mission.hpp"
#include "../gods98/frameloop.hpp"
#include "../platform/platform.h"
#include <locale.h>
#include "game/options.hpp"
#include "game/tutorial_world_arrows.hpp"
#include "movie.hpp"
#include "tickers.hpp"
#include "scene_pool.hpp"
#include "scene_resources.hpp"
#include "view_for_screen_effect_quads.hpp"
#include "gods98/simtick_for_screen_effect_quads.hpp"
#include "crt/crt_for_screen_effect_quads.hpp"
#include "entity_for_screen_effect_quads.hpp"
#include "frontend_for_screen_effect_quads.hpp"
#include "level_for_screen_effect_quads.hpp"
#include "mission_for_screen_effect_quads.hpp"
#include "gods98/frameloop_for_screen_effect_quads.hpp"
#include "gods98/resource_for_screen_effect_quads.hpp"
#include "gods98/view_for_screen_effect_quads.hpp"

#define GRID_REC_FIELD_WORDS   16    /* cleared by the mask sweep */

// GLOBAL: LEGORR 0x004a1d60
const char s_task_copy_format[] = "%s";

// GLOBAL: LEGORR 0x004b8bc8
uint32_t s_grid_rec[GRID_REC_REGION_DWORDS];

static uint32_t s_quad_mesh_render_records[10][8];
static void* s_quad_mesh_render_objects[10];
static void* s_quad_mesh_render_callbacks[10];

/* Defined below (forward declarations added by tools/relocate_unit.py). */
static int effect_helper(SimtickEffect* e, float dt);

/* CreditsTextFile + CreditsBackAVI: scroll centered text over the configured
 * AVI until the player presses a key or clicks. */
// FUNCTION: LEGORR 0x0040a0c0
void frameloop_credits(PlatformWindow* win, DisplaySurface* surf,
                              BlockFile* cfg, const char* wad_dir,
                              AudioPlayer* ap, int audio_status, int* quit,
                              FILE* log, long* frame_number)
{
    uint32_t scroll_clock = (uint32_t)now_ms();
    const char* text_name = block_get_strptr(cfg,
                                             "Lego*::Main::CreditsTextFile");
    char** lines = NULL;
    int line_count = 0;
    if (text_name == NULL
        || !frontend_credits_lines_load(text_name, &lines, &line_count)) {
        fprintf(log, "credits: cannot load %s\n",
                text_name != NULL ? text_name : "Lego*::Main::CreditsTextFile");
        return;
    }

    TextureSurface* font = texture_get_or_load(
        NULL, "Interface\\Fonts\\RSFont.bmp");
    Movie* movie = NULL;
    MovieInfo mi;
    MovieOut out;
    memset(&mi, 0, sizeof(mi));
    memset(&out, 0, sizeof(out));
    int video_frame_ms = FRAME_MS;
    char movie_rel[256], movie_os[512];
    if (movie_key_path(cfg, "CreditsBackAVI", movie_rel,
                       sizeof(movie_rel))
        && movie_path_resolve(wad_dir, movie_rel, movie_os,
                              sizeof(movie_os))) {
        movie = movie_open_path(movie_os);
        if (movie == NULL || movie_info(movie, &mi) != 0
            || mi.width <= 0 || mi.height <= 0) {
            fprintf(log, "credits: cannot open backdrop %s\n", movie_os);
            movie_close(movie);
            movie = NULL;
        } else if (mi.fps_num > 0 && mi.fps_den > 0) {
            video_frame_ms = 1000 * mi.fps_den / mi.fps_num;
            if (video_frame_ms < 1) {
                video_frame_ms = 1;
            }
        }
    }
    InputState input;
    input_reset(&input);
    int scroll_px = 0;
    int64_t next_video_ms = now_ms();
    int have_video_frame = 0;
    int leave = 0;
    int mouse_was_down = 0;
    while (!*quit && !leave) {
        PlatformEvent evbuf[32];
        int nev = platform_pump_events(win, evbuf, 32);
        (void)frameloop_application_events_track(&s_frameloop_application,
                                                evbuf, nev);
        input_capture_events(&input, evbuf, nev);
        renderer_word_c_set(input.mouse_down != 0 ? 1u : 0u);
        input_cursor_position_update();
        input_cursor_position_sync(&input);
        for (int i = 0; i < nev; ++i) {
            if (evbuf[i].kind == PLATFORM_EVENT_QUIT) {
                *quit = 1;
                break;
            }
            if (evbuf[i].kind == PLATFORM_EVENT_KEYDOWN) {
                leave = 1;
            }
            if (evbuf[i].kind == PLATFORM_EVENT_MOUSE) {
                if (!evbuf[i].held && mouse_was_down) {
                    leave = 1;
                }
                mouse_was_down = evbuf[i].held;
            }
        }
        if (*quit || leave) {
            break;
        }

        if (movie != NULL && now_ms() >= next_video_ms) {
            int mr = movie_next(movie, &out);
            if (mr == 1) {
                movie_close(movie);
                movie = movie_open_path(movie_os);
                if (movie != NULL && movie_info(movie, &mi) == 0) {
                    mr = movie_next(movie, &out);
                } else {
                    movie_close(movie);
                    movie = NULL;
                    mr = -1;
                }
            }
            if (mr == 0) {
                have_video_frame = 1;
            } else if (mr < 0) {
                movie_close(movie);
                movie = NULL;
                have_video_frame = 0;
            }
            next_video_ms += video_frame_ms;
            if (next_video_ms < now_ms() - video_frame_ms) {
                next_video_ms = now_ms();
            }
        }

        frontend_credits_scroll_step((uint32_t)now_ms(), &scroll_clock,
                                     &scroll_px);

        void* pixels = NULL;
        int pitch = 0;
        if (surface_lock(surf, &pixels, &pitch) == 0) {
            if (have_video_frame) {
                movie_scale_rgba(out.rgba, mi.width, mi.height, out.rgba_pitch,
                                 (uint8_t*)pixels, WINDOW_W, WINDOW_H, pitch);
            } else {
                surface_fill(pixels, pitch, WINDOW_W, WINDOW_H, 0, 0, 0);
            }
            frontend_render_credits(font,
                                    (const char* const*)lines, line_count,
                                    (int)scroll_px, WINDOW_W, WINDOW_H,
                                    pixels, pitch);
            platform_screenshot(win, pixels, pitch, WINDOW_W, WINDOW_H,
                                (int)*frame_number);
            platform_present_pixels(win, pixels, pitch, WINDOW_W, WINDOW_H);
            surface_unlock(surf, pixels);
        }
        /* ScrollingTextDisplay calls SoundSubsystemFrameTick after its
         * text/video draw and before the next frame boundary. */
        sound_subsystem_frame_tick(1.0f);
        if (audio_status == 0) {
            audio_update(ap);
        }
        if (movie == NULL) {
            Sleep(10);
        }
        ++*frame_number;
        input_begin_frame(&input);
    }

    movie_close(movie);
    texture_release(font);
    frontend_credits_lines_free(lines, line_count);
}

// FUNCTION: LEGORR 0x0040a3d0
void screen_effect_flags_clear_and_release(void)
{
    for (int i = 0; i < SCREEN_EFFECT_COUNT; ++i) {
        if ((s_screen_effect_flags[i] & 1u) != 0u) {
            s_screen_effect_flags[i] &= ~1u;
            ++s_screen_effect_seam_calls;
            s_screen_effect_seam_last_handle = s_screen_effect_handle[i];
            s_screen_effect_seam_last_flag = 1;
        }
    }
}

/* PreloadNumberedBitmaps (FUN_0040a400): format <directory>\\ followed by
 * either <index>.bmp or <prefix><index>.bmp, then get-or-create each of the
 * eleven textures. The retail function overwrites its slots without a
 * release; the texture cache owns the loaded surfaces. */
// FUNCTION: LEGORR 0x0040A400
void preload_numbered_bitmaps(const char* directory, const char* prefix)
{
    unsigned int index;
    char path[512];
    if (directory == NULL) return;
    for (index = 0u; index < 11u; ++index) {
        char suffix[24];
        size_t length;
        strncpy(path, directory, 400u);
        path[400] = '\0';
        length = strlen(path);
        if (length < sizeof(path) - 1u) {
            path[length++] = '\\';
            path[length] = '\0';
        }
        if (prefix != NULL && prefix[0] != '\0' && length < 480u) {
            size_t available = 480u - length;
            strncat(path, prefix, available);
            length = strlen(path);
        }
        sprintf(suffix, "%u.bmp", index);
        strncat(path, suffix, sizeof(path) - strlen(path) - 1u);
        if (s_numbered_bitmap_load_hook != NULL) {
            s_numbered_bitmap_slots[index] = s_numbered_bitmap_load_hook(
                path, s_numbered_bitmap_context);
        }
        else {
            s_numbered_bitmap_slots[index] = texture_get_or_load(NULL, path);
        }
    }
}

// FUNCTION: LEGORR 0x0040a4b0
void quad_mesh_render_record_create(void* entity, uint32_t count)
{
    static const int offsets[6] = { 0, 2, 1, 0, 3, 2 };
    void* slot;
    void* parent;
    void* render_object;
    void* callback_record;
    unsigned int index;
    int i;
    if (entity == NULL || count == 0u || count >= 999u ||
        s_quad_mesh_render_hooks.entity_eligible == NULL ||
        !s_quad_mesh_render_hooks.entity_eligible(
            s_quad_mesh_render_hooks.context, entity)) return;
    slot = quad_mesh_render_record_pool_slot_allocate();
    if (slot == NULL) return;
    for (index = 0; index < 10; ++index) {
        if (slot == s_quad_mesh_render_records[index]) break;
    }
    if (index >= 10) return;
    s_quad_mesh_render_records[index][3] = 0;
    s_quad_mesh_render_records[index][4] = 0;
    s_quad_mesh_render_records[index][5] = 0;
    s_quad_mesh_render_records[index][6] = 0;
    if (s_quad_mesh_render_hooks.entity_lift_get != NULL) {
        float lift = s_quad_mesh_render_hooks.entity_lift_get(
            s_quad_mesh_render_hooks.context, entity);
        float negative_lift = -lift;
        memcpy(&s_quad_mesh_render_records[index][5], &negative_lift,
               sizeof(negative_lift));
    }
    parent = s_quad_mesh_render_hooks.entity_scene_get != NULL
        ? s_quad_mesh_render_hooks.entity_scene_get(
              s_quad_mesh_render_hooks.context, entity) : NULL;
    render_object = s_quad_mesh_render_objects[index];
    if (render_object == NULL) {
        if (s_quad_mesh_render_hooks.render_object_create == NULL) return;
        render_object = s_quad_mesh_render_hooks.render_object_create(
            s_quad_mesh_render_hooks.context, parent);
        if (render_object == NULL) return;
        s_quad_mesh_render_objects[index] = render_object;
        callback_record = s_quad_mesh_render_hooks.callback_record_create != NULL
            ? s_quad_mesh_render_hooks.callback_record_create(
                  s_quad_mesh_render_hooks.context, render_object, slot,
                  0x3000u) : NULL;
        s_quad_mesh_render_callbacks[index] = callback_record;
        if (callback_record != NULL &&
            s_quad_mesh_render_hooks.digit_quad_prepare != NULL) {
            for (i = 0; i < 4; ++i) {
                s_quad_mesh_render_hooks.digit_quad_prepare(
                    s_quad_mesh_render_hooks.context, callback_record,
                    4, 2, 3, offsets);
            }
        }
    }
    else {
        if (s_quad_mesh_render_hooks.render_object_attach != NULL) {
            s_quad_mesh_render_hooks.render_object_attach(
                s_quad_mesh_render_hooks.context, render_object, parent);
        }
        if (s_quad_mesh_render_hooks.render_object_disable != NULL) {
            s_quad_mesh_render_hooks.render_object_disable(
                s_quad_mesh_render_hooks.context, render_object, 0);
        }
    }
    if (s_quad_mesh_render_hooks.digit_count_apply != NULL) {
        s_quad_mesh_render_hooks.digit_count_apply(
            s_quad_mesh_render_hooks.context, slot, count);
    }
}
// FUNCTION: LEGORR 0x0040a5c0
void* quad_mesh_render_record_pool_slot_allocate(void)
{
    unsigned int i;
    for (i = 0; i < 10; ++i) {
        if ((s_quad_mesh_render_records[i][7] & 1u) == 0u) {
            s_quad_mesh_render_records[i][7] |= 1u;
            return s_quad_mesh_render_records[i];
        }
    }
    return NULL;
}

// FUNCTION: LEGORR 0x0040a5e0
void record_count_digits_fill(const RecordCountRenderHooks* hooks, uint32_t value)
{
    unsigned int slot;
    unsigned int digits = 0;
    uint32_t probe = value;
    float red;
    float green;
    if (hooks == NULL) return;
    if (value < 5u) { red = 0.0f; green = 1.0f; }
    else if (value < 10u) { red = 1.0f; green = 1.0f; }
    else { red = 1.0f; green = 0.25f; }
    if (hooks->style_set != NULL) {
        for (slot = 0; slot < 4; ++slot) {
            hooks->style_set(hooks->context, slot, red, green, 0);
            hooks->style_set(hooks->context, slot, red, green, 3);
        }
    }
    if (probe != 0) {
        uint32_t limit = 1;
        while (limit <= probe && limit <= 100000000u) {
            ++digits;
            limit *= 10u;
        }
    }
    if (digits > 4) digits = 4;
    for (slot = 0; slot < digits; ++slot) {
        if (hooks->digit_set != NULL)
            hooks->digit_set(hooks->context, slot, value % 10u);
        if (hooks->slot_hide != NULL)
            hooks->slot_hide(hooks->context, slot, 0);
        value /= 10u;
    }
    if (hooks->blank_set != NULL) hooks->blank_set(hooks->context, digits);
    for (slot = digits + 1; slot < 4; ++slot) {
        if (hooks->slot_hide != NULL)
            hooks->slot_hide(hooks->context, slot, 1);
    }
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

/* EntityQuadMeshRenderRecordAllowed (FUN_0040aae0) — see entity.hpp. */
// FUNCTION: LEGORR 0x0040aae0
int entity_quad_mesh_render_record_allowed(const EntitySlot* e)
{
    if ((entity_state_flags_get(e) & 0x08000000u) == 0u && e->type != 8
        && e->type != 13) {
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0040ab10
void global_mode_zero_input_set(int value)
{
    s_mode_zero_flag = (value == 0) ? 1u : 0u;
}

// FUNCTION: LEGORR 0x0040ab30
void grid_record_flag_mask_clear(void)
{
    size_t k = 33; /* the corpus's field-word origin */
    do {
        for (int row = 0; row < 15; ++row) {
            for (int d = 0; d < GRID_REC_FIELD_WORDS; ++d) {
                s_grid_rec[k + (size_t)d] &= 0xfffffef3u; /* ~ 0x10c */
            }
            k += GRID_REC_STRIDE;
        }
    } while (k + 33 < GRID_REC_REGION_DWORDS);
}
