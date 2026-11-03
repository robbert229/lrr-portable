// SPDX-License-Identifier: LGPL-3.0-or-later
#include "tutorial_world_arrows.hpp"
#include "tutorial_hud_ports.hpp"
#include "entity.hpp"
#include "frontend.hpp"
#include "level.hpp"
#include "../crt/crt.hpp"
#include "../gods98/message.hpp"
#include "../gods98/surface.hpp"
#include "../gods98/texture.hpp"
#include <math.h>
#include <stdlib.h>
#include <string.h>

float tutorial_world_arrows_delta_from_seconds(float seconds)
{
    float ticks;
    if (seconds <= 0.0f) return 0.0f;
    ticks = seconds * 25.0f;
    return ticks > 3.0f ? 3.0f : ticks;
}

void tutorial_world_arrows_discard(TutorialWorldArrows* arrows)
{
    if (arrows == NULL) return;
    arrows->level = NULL;
    arrows->level_generation = 0u;
    arrows->count = 0u;
    arrows->complete = 0;
    arrows->frame_valid = 0;
}

void tutorial_world_arrows_release(TutorialWorldArrows* arrows)
{
    if (arrows == NULL) return;
    tutorial_world_arrows_discard(arrows);
    free(arrows->intents);
    arrows->intents = NULL;
    arrows->capacity = 0u;
}

static int tutorial_world_arrows_reserve(TutorialWorldArrows* arrows,
                                         int width, int height)
{
    size_t count;
    TutorialWorldArrowIntent* intents;
    if (width <= 0 || height <= 0 ||
        (size_t)width > (((size_t)-1) - 1u) / (size_t)height) return 0;
    count = (size_t)width * (size_t)height + 1u;
    if (count <= arrows->capacity) return 1;
    if (count > ((size_t)-1) / sizeof(*intents)) return 0;
    intents = (TutorialWorldArrowIntent*)realloc(arrows->intents,
        count * sizeof(*intents));
    if (intents == NULL) return 0;
    arrows->intents = intents;
    arrows->capacity = count;
    return 1;
}

static void tutorial_world_arrow_append(TutorialWorldArrows* arrows,
                                         LevelState* level,
                                         const float xy[2], float bob)
{
    TutorialWorldArrowIntent* intent;
    if (!arrows->complete) return;
    intent = &arrows->intents[arrows->count++];
    intent->world[0] = xy[0];
    intent->world[1] = xy[1];
    intent->world[2] = terrain_surface_height_interpolate(
        level->ptr_terrain, xy[0], xy[1]);
    intent->bob = bob;
}

/* 0x435388..0x435520: use the existing terrain appearance owners. This does
 * not change geometry, materials, depth, or the active marker at voxel+4. */
static void tutorial_world_arrow_block_appearance(LevelState* level,
                                                  int x, int y,
                                                  uint8_t* voxel)
{
    uint32_t prior;
    uint32_t value;
    memcpy(&prior, voxel + 0x44, sizeof(prior));
    if (voxel[4] != 0u) {
        if (sim_status_banner_word_get() == 0u)
            terrain_cell_surface_type_reset(level->ptr_terrain, x, y);
        else
            (void)terrain_cell_surface_type_set(level->ptr_terrain, x, y, 5);
        value = 1u;
        memcpy(voxel + 0x44, &value, sizeof(value));
    }
    else if (prior == 1u) {
        value = 0u;
        memcpy(voxel + 0x44, &value, sizeof(value));
        memcpy(&value, voxel + 0x28, sizeof(value));
        if ((system_state_flags_get() & 2u) == 0u && value != 0u &&
            s_obj_sel_word == 0u)
            (void)terrain_cell_surface_type_set(level->ptr_terrain, x, y, 4);
        else
            terrain_cell_surface_type_reset(level->ptr_terrain, x, y);
    }
    else if (prior == 0u &&
        terrain_cell_surface_type_get(level->ptr_terrain, x, y) == 5) {
        terrain_cell_surface_type_reset(level->ptr_terrain, x, y);
    }
}

int tutorial_world_arrows_capture(TutorialWorldArrows* arrows,
    LevelState* level, uint32_t level_generation, uint32_t logical_frame,
    int context_mode, int time_frozen, int lists_active,
    float raw_elapsed_units)
{
    float block_bob;
    float actor_bob;
    uint32_t slot;
    EntitySlot* actor;
    int x, y;
    if (arrows == NULL) return 0;
    if (arrows->frame_valid && arrows->logical_frame == logical_frame &&
        arrows->level == level &&
        arrows->level_generation == level_generation) return arrows->complete;
    arrows->logical_frame = logical_frame;
    arrows->frame_valid = 1;
    arrows->level = level;
    arrows->level_generation = level_generation;
    arrows->count = 0u;
    arrows->complete = 0;
    if (context_mode != 1 || time_frozen) {
        arrows->complete = 1;
        return 1;
    }

    /* Separate retail globals 0x4de58c / 0x4ff9b8. Sample OLD phase; never
     * use SetGameSpeed, NEXT flash, elapsed scaled time or draw frequency. */
    block_bob = (float)sin((double)arrows->block_phase);
    arrows->block_phase += raw_elapsed_units * 25.0f * 0.013333333656191826f;
    actor_bob = (float)sin((double)arrows->actor_phase);
    arrows->actor_phase += raw_elapsed_units * 25.0f * 0.013333333656191826f;
    if (level == NULL || level->ptr_terrain == NULL ||
        level_generation == 0u || !lists_active) return 0;
    arrows->complete = tutorial_world_arrows_reserve(arrows,
        level_width(level), level_height(level));

    /* SystemState gates the block scan only; both phases already advanced.
     * Activation must clear the raw coordinate-group definitions first. */
    if (system_state_flags_get() != 0u) {
        /* TerrainCreate retains dim[0/1] = map width/height minus one;
         * the final map row/column contains vertices, not physical quads. */
        for (y = 0; y < level_height(level) - 1; ++y) {
            for (x = 0; x < level_width(level) - 1; ++x) {
                uint8_t* voxel;
                float xy[2];
                if (grid_cell_entity_eligibility_check(x, y) != 0) continue;
                voxel = level_voxel(level, x, y);
                if (voxel == NULL) continue;
                if (voxel[4] != 0u && terrain_cell_world_center_xy_get(
                        level->ptr_terrain, x, y, xy))
                    tutorial_world_arrow_append(arrows, level, xy, block_bob);
                tutorial_world_arrow_block_appearance(level, x, y, voxel);
            }
        }
    }
    /* 0x454600: one-based OList lookup with no kind/selection/route filter.
     * Keep the existing pre-input marking hook; this is its render-point peer. */
    slot = tutorial_record_object_pointer_get();
    actor = slot != 0u ? object_pointer_registry_entity_get(slot - 1u) : NULL;
    if (actor != NULL) {
        float xy[2];
        actor->tutorial_unit_at_block_3f8 = 1u;
        entity_transform_position_get(actor, xy);
        tutorial_world_arrow_append(arrows, level, xy, actor_bob);
    }
    return arrows->complete;
}

void tutorial_world_arrows_draw(const TutorialWorldArrows* arrows,
    const LevelState* level, uint32_t level_generation,
    TutorialWorldArrowProjectFn project, void* context,
    void* pixels, int pitch, int width, int height)
{
    const TextureSurface* icon;
    size_t i;
    if (arrows == NULL || !arrows->complete || arrows->level != level ||
        arrows->level_generation != level_generation || project == NULL ||
        pixels == NULL || width <= 0 || height <= 0) return;
    icon = (const TextureSurface*)advisor_tutorial_icon_get();
    if (icon == NULL || icon->width <= 0 || icon->height <= 0) return;
    for (i = 0u; i < arrows->count; ++i) {
        const TutorialWorldArrowIntent* intent = &arrows->intents[i];
        float screen[2];
        int x, y;
        if (!project(context, intent->world, screen)) continue;
        x = crt_float_to_long(screen[0] - (float)(icon->width / 2));
        y = crt_float_to_long(screen[1] -
            (2.0f * (float)icon->height + 20.0f * intent->bob));
        surface_blit_texture(pixels, pitch, width, height, icon, x, y);
    }
}
