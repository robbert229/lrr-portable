// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef LEGORRR_TUTORIAL_WORLD_ARROWS_HPP
#define LEGORRR_TUTORIAL_WORLD_ARROWS_HPP

#include <stddef.h>
#include <stdint.h>

struct LevelState;

/* Numeric intent only: never retain an entity, viewport or voxel pointer. */
typedef struct TutorialWorldArrowIntent {
    float world[3]; /* horizontal X/Y, live terrain height */
    float bob;
} TutorialWorldArrowIntent;

/* Zero-initialize once at application startup. Discard/release preserve phases:
 * the retail pointer setter and NERP level reset do not reset these globals. */
typedef struct TutorialWorldArrows {
    float block_phase;
    float actor_phase;
    TutorialWorldArrowIntent* intents;
    size_t count;
    size_t capacity;
    const struct LevelState* level;
    uint32_t level_generation;
    uint32_t logical_frame;
    int frame_valid;
    int complete;
} TutorialWorldArrows;

/* Production seconds -> WinMain's raw 25-Hz units, capped at three. Fixed
 * reference-time modes may instead supply their exact raw units to capture. */
float tutorial_world_arrows_delta_from_seconds(float seconds);

/* Exactly once at the logical render point BEFORE NERP dispatch, even if
 * software surface locking later fails. context_mode==1 and !time_frozen are
 * the retail outer gates. lists_active certifies activation consumed the raw
 * map group bytes; it must come from the real level lifecycle owner.
 * A duplicate ordinal is inert. Zero return means incomplete/unready intent;
 * allocation failure still advances phases and applies gameplay side effects. */
int tutorial_world_arrows_capture(TutorialWorldArrows* arrows,
    struct LevelState* level, uint32_t level_generation,
    uint32_t logical_frame, int context_mode, int time_frozen,
    int lists_active, float raw_elapsed_units);

/* The renderer callback must use camera v2's current displayed projection,
 * validate level/dimensions/nonzero generation, and share scene_project's
 * rejection policy. It must never publish a projection or choose a camera. */
typedef int (*TutorialWorldArrowProjectFn)(void* context,
    const float world[3], float screen[2]);
void tutorial_world_arrows_draw(const TutorialWorldArrows* arrows,
    const struct LevelState* level, uint32_t level_generation,
    TutorialWorldArrowProjectFn project, void* context,
    void* pixels, int pitch, int width, int height);

/* Call before level destruction/replacement, including same-address reuse. */
void tutorial_world_arrows_discard(TutorialWorldArrows* arrows);
void tutorial_world_arrows_release(TutorialWorldArrows* arrows);

#endif
