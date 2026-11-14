// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-031 ObjInfo.c's port tail (finding 633): the port's models of the
 * unit's functions and the state the port's other files read. Unannotated:
 * the original-shape unit is objinfo.c (ADR 0005). In the matching build
 * the unit defines the four port names its callers use
 * (objinfo_link_names.h), so this file compiles its own out there.
 * Relocated by tools/relocate_unit.py (ADR 0006); body hashes:
 * docs/coverage/relocations/GAME-031.csv. */
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
#include "frontend_for_object_overlays.hpp"
#include <stdint.h>
#include "game.hpp"
#include "spawn.hpp"
#include "terrain_grid.hpp"
extern "C" int tutorial_script_file_load_target(const char* path);
#include "tutorial_hud_for_object_overlays.hpp"

/* --- the panel resource + scroll leaf band (see frontend.hpp) --------------
 * The corpus's unowned panel words (DAT_004ffe68 enable gate, DAT_004ffed8/
 * edc bubble offsets, DAT_00500810..81c handle/dimensions, DAT_005007d8 UI
 * record slot) land here as zero-init statics; both corpus seams (surface
 * blit FUN_0047dcb0, surface load FUN_0047d9d0) are hook boundaries with
 * no ported body — the seam calls record and the load answers NULL (the
 * view object-layer precedent). */
/* The port's models of objInfoGlobs' flags (004ffe68), hunger offset
 * (004ffebc) and hunger images (004ffec4); objinfo.c owns the original's. */
uint32_t s_panel_enable_word;

float    s_panel_position_offset_x;
float    s_panel_position_offset_y;
void*    s_panel_position_slots[5];

static void* s_legacy_hunger_images[5];
static float s_legacy_hunger_position[2];

/* Defined below (forward declarations added by tools/relocate_unit.py). */
static void legacy_color_pair(float source[3], float high[3], float low[3]);
static int legacy_config_bool_from_root(BlockFile* values, const char* config_root, const char* key);
static int legacy_config_int_from_root(BlockFile* values, const char* config_root, const char* key, int fallback);
static int legacy_config_pair_from_root(BlockFile* values, const char* config_root, const char* key, int* x, int* y);
static int legacy_config_rgb_from_root(BlockFile* values, const char* config_root, const char* key, float rgb[3]);
static void legacy_hud_resource_name(BlockFile* cfg, const char* config_root, const char* key, char* out, uint32_t out_size);
static uint32_t legacy_rgb_pack(const float rgb[3]);
static void panel_texture_draw_at_active_target(void* resource, float x, float y);

/* The port's model of 0x00459530; objinfo.c serves this name in the
 * matching build (finding 633). */
#if !(defined(_MSC_VER) && _MSC_VER < 1200)
void frontend_render_three_pass_update(BlockFile* values,
                                        const char* config_root)
{
    legacy_health_bar_config_load(values, config_root);
    legacy_hunger_images_position_config_load(values, config_root);
    (void)frontend_bubble_images_position_config_load(values, config_root);
}
#endif

/* The port's model of 0x00459560 (objinfo.c). */
int legacy_health_bar_config_load(BlockFile* values, const char* config_root)
{
    int x, y;
    int width, height;
    BlockFile* cfg = values;
    int border;
    int vertical;
    float rgb[3];
    if (cfg == NULL) return 0;
    if (!legacy_config_pair_from_root(values, config_root,
            "HealthBarPosition", &x, &y)) return 0;
    s_legacy_health_bar.x = (float)x;
    s_legacy_health_bar.y = (float)y;
    if (!legacy_config_pair_from_root(values, config_root,
            "HealthBarWidthHeight", &width, &height)) return 0;
    s_legacy_health_bar.width = (float)width;
    s_legacy_health_bar.height = (float)height;
    // The original derives this through an integer conversion of the raw
    // value string, so a missing or empty key yields 0 rather than 1.
    border = legacy_config_int_from_root(values, config_root,
            "HealthBarBorderSize", 0);
    s_legacy_health_bar.border = (float)border;
    if (!legacy_config_rgb_from_root(values, config_root,
            "HealthBarBorderRGB", rgb)) return 0;
    memcpy(s_legacy_health_bar.border_rgb, rgb, sizeof(rgb));
    legacy_color_pair(s_legacy_health_bar.border_rgb,
                      s_legacy_health_bar.border_high,
                      s_legacy_health_bar.border_low);
    if (!legacy_config_rgb_from_root(values, config_root,
            "HealthBarBackgroundRGB", rgb)) return 0;
    memcpy(s_legacy_health_bar.background_rgb, rgb, sizeof(rgb));
    if (!legacy_config_rgb_from_root(values, config_root,
            "HealthBarRGB", rgb)) return 0;
    memcpy(s_legacy_health_bar.health_rgb, rgb, sizeof(rgb));
    vertical = legacy_config_bool_from_root(values, config_root,
            "HealthBarVertical");
    /* DAT_004ffe68 is shared by the other HUD loaders. The original only
       ORs these bits: a later horizontal reload does not clear vertical. */
    if (vertical == 1) s_panel_enable_word |= 8u;
    s_panel_enable_word |= 2u;
    s_legacy_health_bar.vertical =
        (s_panel_enable_word & 8u) != 0 ? 1u : 0u;
    s_legacy_health_bar.flags |= 2u;
    if ((s_panel_enable_word & 8u) != 0)
        s_legacy_health_bar.flags |= 8u;
    return 1;
}

/* The port's model of 0x00459900 (objinfo.c). */
int legacy_hunger_images_position_config_load(BlockFile* values,
                                               const char* config_root)
{
    int x, y;
    BlockFile* cfg = values;
    legacy_hunger_images_load(values, config_root);
    if (cfg == NULL || !legacy_config_pair_from_root(values, config_root,
            "HungerImagesPosition", &x, &y)) return 0;
    s_legacy_hunger_position[0] = (float)x;
    s_legacy_hunger_position[1] = (float)y;
    panel_position_offsets_set((float)x, (float)y);
    s_panel_enable_word |= 1u;
    return 1;
}
/* The port's model of 0x004599c0 (objinfo.c). */
void legacy_hunger_images_load(BlockFile* values, const char* config_root)
{
    int i;
    char path[96];
    char resource_name[256];
    BlockFile* cfg = values;
    // The original re-reads the five ObjInfo::HungerImages::HungerImage%i
    // keys and overwrites each slot without releasing the previous surface
    // (the resource layer owns surface lifetime).
    for (i = 0; i < 5; ++i) {
        void* image = NULL;
        sprintf(path, "HungerImages::HungerImage%i", i);
        legacy_hud_resource_name(cfg, config_root, path, resource_name,
                                 (uint32_t)sizeof(resource_name));
        if (resource_name[0] != '\0' &&
            s_legacy_hud_ops.surface_load != NULL) {
            image = s_legacy_hud_ops.surface_load(
                cfg, resource_name, 0, 0, s_legacy_hud_ops.context);
            if (image != NULL && s_legacy_hud_ops.surface_initialize != NULL)
                s_legacy_hud_ops.surface_initialize(
                    image, s_legacy_hud_ops.context);
        }
        s_legacy_hunger_images[i] = image;
        /* These are the five DAT_004ffec4 slots consumed by
         * IndexedPanelResourcePositionApply. Keep the config-loaded images
         * in the same table used by the live panel drawing path. */
        panel_position_slot_set(i, image);
    }
}

/* The port's model of 0x00459a50 (objinfo.c). */
int frontend_bubble_images_position_config_load(BlockFile* values,
                                                const char* config_root)
{
    const char* const path = string_path_join(config_root, "ObjInfo",
        "BubbleImagesPosition", (char*)NULL);
    const char* value;
    char storage[128];
    char* fields[2];
    if (values == NULL) return 0;
    value = block_get_strptr(values, path);
    if (value == NULL ||
        !frontend_config_fields(value, storage, sizeof(storage), fields, 2,
                                ':')) {
        return 0;
    }
    bubble_offsets_set((float)atoi(fields[0]), (float)atoi(fields[1]));
    panel_enable_word_set(panel_enable_word_peek() | 0x10u);
    return 1;
}

/* The port's model of 0x00459b00; objinfo.c serves this name in the
 * matching build (finding 633). */
#if !(defined(_MSC_VER) && _MSC_VER < 1200)
void legacy_entity_screen_ring_render(void* entity, int x, int y)
{
    EntitySlot* slot = (EntitySlot*)entity;
    float first_xy[20];
    float second_xy[20];
    float rect[4];
    float health;
    float width, height, border;
    float health_size;
    float x0, y0;
    int i;
    int point_count;
    if (slot == NULL || (s_panel_enable_word & 2u) == 0u) return;
    if (entity_quad_mesh_render_record_allowed(slot) == 0) return;
    health = slot->field_340;
    if (health < 0.0f) health = 0.0f;
    if (health > 100.0f) health = 100.0f;
    border = s_legacy_health_bar.border;
    width = s_legacy_health_bar.width + border * 2.0f;
    height = s_legacy_health_bar.height + border * 2.0f;
    x0 = (float)x + s_legacy_health_bar.x;
    y0 = (float)y + s_legacy_health_bar.y;

    /* Match the two interleaved 2*border vertex streams built by retail.
     * Each pair walks one screen edge while stepping one pixel inward. */
    point_count = (int)border * 2;
    if (point_count > 10) point_count = 10;
    if (point_count < 0) point_count = 0;
    memset(first_xy, 0, sizeof(first_xy));
    memset(second_xy, 0, sizeof(second_xy));
    for (i = 0; i < point_count / 2; ++i) {
        float fi = (float)i;
        int at = i * 4;
        first_xy[at] = x0 + fi;
        first_xy[at + 1] = y0 + fi;
        first_xy[at + 2] = x0 + width - 1.0f - fi;
        first_xy[at + 3] = y0 + fi;
        second_xy[at] = x0 + fi;
        second_xy[at + 1] = y0 + height - 2.0f - fi;
        second_xy[at + 2] = x0 + fi + 1.0f;
        second_xy[at + 3] = y0 + fi;
    }
    {
        float opposite_a[20];
        float opposite_b[20];
        memset(opposite_a, 0, sizeof(opposite_a));
        memset(opposite_b, 0, sizeof(opposite_b));
        for (i = 0; i < point_count / 2; ++i) {
            float fi = (float)i;
            int at = i * 4;
            opposite_a[at] = x0 + fi;
            opposite_a[at + 1] = y0 + height - 1.0f - fi;
            opposite_a[at + 2] = x0 + width - 1.0f - fi;
            opposite_a[at + 3] = y0 + height - 1.0f - fi;
            opposite_b[at] = x0 + width - 2.0f - fi;
            opposite_b[at + 1] = y0 + height - 1.0f - fi;
            opposite_b[at + 2] = x0 + width - 1.0f - fi;
            opposite_b[at + 3] = y0 + fi + 1.0f;
        }
        if (s_legacy_hud_ops.ring_strip_draw != NULL && point_count > 0) {
            s_legacy_hud_ops.ring_strip_draw(first_xy, second_xy,
                (uint32_t)point_count, s_legacy_health_bar.border_high, 0,
                s_legacy_hud_ops.context);
            s_legacy_hud_ops.ring_strip_draw(opposite_a, opposite_b,
                (uint32_t)point_count, s_legacy_health_bar.border_low, 0,
                s_legacy_hud_ops.context);
        }
    }

    if (s_legacy_hud_ops.rect_draw != NULL) {
        if (s_legacy_health_bar.vertical == 0u) {
            health_size = health * width * 0.01f;
            rect[0] = x0;
            rect[1] = y0;
            rect[2] = health_size;
            rect[3] = height;
            s_legacy_hud_ops.rect_draw(rect,
                legacy_rgb_pack(s_legacy_health_bar.health_rgb),
                s_legacy_hud_ops.context);
            rect[0] = x0 + health_size;
            rect[2] = width - health_size;
            s_legacy_hud_ops.rect_draw(rect,
                legacy_rgb_pack(s_legacy_health_bar.background_rgb),
                s_legacy_hud_ops.context);
        }
        else {
            health_size = (100.0f - health) * height * 0.01f;
            rect[0] = x0;
            rect[1] = y0;
            rect[2] = width;
            rect[3] = health_size;
            s_legacy_hud_ops.rect_draw(rect,
                legacy_rgb_pack(s_legacy_health_bar.background_rgb),
                s_legacy_hud_ops.context);
            rect[1] = y0 + health_size;
            rect[3] = height - health_size;
            s_legacy_hud_ops.rect_draw(rect,
                legacy_rgb_pack(s_legacy_health_bar.health_rgb),
                s_legacy_hud_ops.context);
        }
    }
}
#endif

/* IndexedPanelResourcePositionApply (FUN_00459f50) — see frontend.hpp.
 * The corpus reads the slot selector off the record's screen-position
 * float (+0x344), multiplied by its two float constants in the x87
 * register's extended precision (modeled as exact double products) and
 * truncated toward zero, then clamped at the last slot by the unsigned
 * compare (a negative conversion clamps to 4 too); the slot table and
 * offsets are own storage (the position-config loads are recorded config
 * seams), and the position rides the surface blit seam. Deviations: the
 * port takes the record float's address instead of the corpus's blind
 * record+0x344 read, and a NULL float reads zero. */
/* The port's model of 0x00459f50 (objinfo.c). */
void indexed_panel_resource_position_apply(const float* record_position,
                                           int x, int y)
{
    if ((s_panel_enable_word & 1u) == 0u) {
        return;
    }
    const float v = (record_position != NULL ? *record_position : 0.0f);
    const int sel = (int)(((double)v * (double)0.01f) * (double)5.0f);
    uint32_t slot = (uint32_t)sel;
    if (slot > 4u) {
        slot = 4u; /* the corpus jbe: every out-of-range index, negative
                    * conversions included, rides the last slot */
    }
    void* res = s_panel_position_slots[slot];
    if (res == NULL) {
        return;
    }
    panel_blit_seam_call(res, (float)x + s_panel_position_offset_x,
                         (float)y + s_panel_position_offset_y);
}
/* The port's model of 0x00459fd0; objinfo.c serves this name in the
 * matching build (finding 633). */
#if !(defined(_MSC_VER) && _MSC_VER < 1200)
void panel_resource_position_apply_if_enabled(void* resource, int x, int y)
{
    /* The corpus tests the enable byte, then FILD/FADD-stores x before y
     * into its local coordinate pair. Its native SurfaceBlitRect path uses
     * the renderer-owned DirectDraw device; portable HUD images are
     * TextureSurface records and draw through the active software target. */
    if (((unsigned char)s_panel_enable_word & 0x10u) != 0u) {
        float position[2];
        position[0] = (float)x + s_bubble_offset_x;
        position[1] = (float)y + s_bubble_offset_y;
        panel_blit_seam_call(resource, position[0], position[1]);
        panel_texture_draw_at_active_target(resource, position[0],
                                             position[1]);
    }
}
#endif
/* FUN_0047dcb0's native device path is renderer-owned. The portable HUD
 * loader supplies TextureSurface records, so use the active software render
 * target when one is bound and retain the seam state for callers without a
 * framebuffer. */
static void panel_texture_draw_at_active_target(void* resource, float x,
                                               float y)
{
    DisplaySurface* target = surface_active_render_target_get();
    void* pixels = NULL;
    int pitch = 0;
    if (resource == NULL || target == NULL || target->w <= 0 ||
        target->h <= 0) return;
    if (surface_lock(target, &pixels, &pitch) != 0) return;
    surface_blit_texture(pixels, pitch, target->w, target->h,
        (const TextureSurface*)resource, (int)crt_float_to_long(x),
        (int)crt_float_to_long(y));
    surface_unlock(target, pixels);
}

static int legacy_config_rgb(BlockFile* cfg, const char* path, float rgb[3])
{
    if (cfg == NULL || path == NULL || rgb == NULL) return 0;
    if (s_legacy_hud_ops.config_rgb != NULL)
        return s_legacy_hud_ops.config_rgb(cfg, path, rgb,
                                           s_legacy_hud_ops.context);
    return config_rgb_vector_read(cfg, path, &rgb[0], &rgb[1], &rgb[2]);
}

/* The retail loaders resolve each setting through a key-name file first,
 * then fetch and parse the corresponding value from the value file. Most
 * production callers pass the same Lego.cfg for both, but keep the two
 * inputs distinct for callers which supply a separate key table. */
static const char* legacy_hud_config_path(const char* config_root,
                                          const char* key)
{
    return string_path_join(config_root, "ObjInfo", key, (char*)NULL);
}

static int legacy_config_pair_from_root(BlockFile* values,
                                        const char* config_root,
                                        const char* key, int* x, int* y)
{
    const char* path;
    if (values == NULL || config_root == NULL || key == NULL) return 0;
    path = legacy_hud_config_path(config_root, key);
    return legacy_config_pair(values, path, x, y);
}

static int legacy_config_rgb_from_root(BlockFile* values,
                                       const char* config_root,
                                       const char* key, float rgb[3])
{
    const char* path;
    if (values == NULL || config_root == NULL || key == NULL) return 0;
    path = legacy_hud_config_path(config_root, key);
    return legacy_config_rgb(values, path, rgb);
}

static int legacy_config_int_from_root(BlockFile* values,
                                       const char* config_root,
                                       const char* key, int fallback)
{
    const char* path;
    if (values == NULL || config_root == NULL || key == NULL) return fallback;
    path = legacy_hud_config_path(config_root, key);
    return block_get_int(values, path, fallback);
}

static int legacy_config_bool_from_root(BlockFile* values,
                                        const char* config_root,
                                        const char* key)
{
    const char* path;
    if (values == NULL || config_root == NULL || key == NULL) return 0;
    path = legacy_hud_config_path(config_root, key);
    return block_get_bool_path(values, path);
}

static float legacy_clamp_01(float value)
{
    if (value < 0.0f) return 0.0f;
    if (value > 1.0f) return 1.0f;
    return value;
}

static void legacy_color_pair(float source[3], float high[3], float low[3])
{
    int i;
    for (i = 0; i < 3; ++i) {
        /* The retail loop walks the interleaved globals in three strides,
         * clamping the source, lighter, and darker RGB channels in place. */
        high[i] = legacy_clamp_01(source[i] + source[i] * 0.4f);
        low[i] = legacy_clamp_01(source[i] - source[i] * 0.4f);
        source[i] = legacy_clamp_01(source[i]);
    }
}

static void legacy_hud_resource_name(BlockFile* cfg, const char* config_root,
                                     const char* key,
                                     char* out, uint32_t out_size)
{
    const char* path;
    out[0] = '\0';
    if (cfg == NULL || config_root == NULL || key == NULL || out_size == 0)
        return;
    path = legacy_hud_config_path(config_root, key);
    if (s_legacy_hud_ops.config_resource != NULL) {
        s_legacy_hud_ops.config_resource(cfg, path, out, out_size,
                                        s_legacy_hud_ops.context);
    } else {
        block_get_str(cfg, path, out, (size_t)out_size);
    }
}

static uint32_t legacy_rgb_pack(const float rgb[3])
{
    uint32_t r = (uint32_t)(legacy_clamp_01(rgb[0]) * 255.0f);
    uint32_t g = (uint32_t)(legacy_clamp_01(rgb[1]) * 255.0f);
    uint32_t b = (uint32_t)(legacy_clamp_01(rgb[2]) * 255.0f);
    return (r << 16) | (g << 8) | b;
}

/* ObjInfo_DrawHungerImage (0x00459f50) in the original ABI, for Bubbles.c
 * (bubbles.c, finding 598): the original reads the object's energy at
 * +0x344 itself; the port's model takes a pointer to that float. In the
 * matching build objinfo.c serves this name (finding 633). */
// PORT-ONLY: original-ABI adapter over indexed_panel_resource_position_apply
#if !(defined(_MSC_VER) && _MSC_VER < 1200)
extern "C" void objinfo_draw_hunger_image_adapter(void* liveObj, int x, int y)
{
    indexed_panel_resource_position_apply(
        (const float*)((const char*)liveObj + 0x344), x, y);
}
#endif
