// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef LEGO_GAME_SIMULATION_UI_PORTS_HPP
#define LEGO_GAME_SIMULATION_UI_PORTS_HPP

#include <stdint.h>

typedef struct SimCategoryHitEntry {
    int left;
    int top;
    int right;
    int bottom;
    int category;
    int item;
} SimCategoryHitEntry;

static inline int sim_category_cell_hit_test(
    const SimCategoryHitEntry* entries, uint32_t count,
    int x, int y, int* category_out, int* item_out)
{
    uint32_t i;
    if (entries == 0) return 0;
    for (i = 0; i < count; ++i) {
        if (x >= entries[i].left && x < entries[i].right &&
            y >= entries[i].top && y < entries[i].bottom) {
            if (category_out != 0) *category_out = entries[i].category;
            if (item_out != 0) *item_out = entries[i].item;
            return 1;
        }
    }
    return 0;
}

typedef struct SimCursorInteractionHooks {
    int (*category_hit)(void* context, int x, int y,
                        int* category, int* item);
    void (*hover_update)(void* context, int category, int item);
    void (*action_dispatch)(void* context, int category, int item,
                            unsigned int action);
    int (*region_hit)(void* context, int x, int y, int* region);
    void (*region_dispatch)(void* context, int region);
    void* context;
} SimCursorInteractionHooks;

static inline int sim_cursor_interaction_tick(
    const SimCursorInteractionHooks* hooks, int x, int y,
    unsigned int action)
{
    int category = -1;
    int item = -1;
    int region = -1;
    if (hooks == 0) return 0;
    if (hooks->category_hit != 0 &&
        hooks->category_hit(hooks->context, x, y, &category, &item)) {
        if (hooks->hover_update != 0)
            hooks->hover_update(hooks->context, category, item);
        if (action != 0 && hooks->action_dispatch != 0)
            hooks->action_dispatch(hooks->context, category, item, action);
        return 1;
    }
    if (hooks->hover_update != 0)
        hooks->hover_update(hooks->context, -1, -1);
    if (hooks->region_hit != 0 &&
        hooks->region_hit(hooks->context, x, y, &region)) {
        if (hooks->region_dispatch != 0)
            hooks->region_dispatch(hooks->context, region);
        return 1;
    }
    return 0;
}

typedef struct PanelRotationControlState {
    int center_x;
    int center_y;
    int radius;
    void* images[4];
} PanelRotationControlState;

typedef struct PanelRotationControlHooks {
    int (*config_get)(void* context, const char* key, int* value);
    void* (*image_load)(void* context, const char* key);
    void* context;
} PanelRotationControlHooks;

static inline int panel_rotation_control_config_load(
    PanelRotationControlState* state, const PanelRotationControlHooks* hooks)
{
    static const char* image_names[4] = {
        "RotateLeft", "RotateRight", "RotateUp", "RotateDown"
    };
    int ok = 1;
    int i;
    if (state == 0 || hooks == 0) return 0;
    if (hooks->config_get != 0) {
        ok &= hooks->config_get(hooks->context, "CenterX", &state->center_x);
        ok &= hooks->config_get(hooks->context, "CenterY", &state->center_y);
        ok &= hooks->config_get(hooks->context, "Radius", &state->radius);
    }
    for (i = 0; i < 4; ++i)
        state->images[i] = hooks->image_load != 0
            ? hooks->image_load(hooks->context, image_names[i]) : 0;
    return ok;
}

typedef struct UiOverlayAnimationState {
    void* animation;
    int x;
    int y;
    float width;
    float height;
    float progress;
} UiOverlayAnimationState;

static inline void ui_overlay_animation_apply(
    const UiOverlayAnimationState* state,
    void (*draw)(void* context, void* animation, int frame,
                 const float* position, const float* size),
    void* context)
{
    float position[2];
    float size[2];
    if (state == 0 || state->animation == 0 || draw == 0 ||
        state->progress <= 0.0f) return;
    position[0] = (float)state->x;
    position[1] = (float)state->y;
    size[0] = state->width * state->progress;
    size[1] = state->height;
    draw(context, state->animation, 0, position, size);
}

typedef struct AdvisorAnimationRequestHooks {
    void (*request)(void* context, int slot, int animation);
    void* context;
} AdvisorAnimationRequestHooks;

static inline int advisor_animation_scene_lookup_and_request(
    int scene, int animation, const AdvisorAnimationRequestHooks* hooks)
{
    int slot = 21;
    if (scene == 0 && animation >= 0 && animation <= 4)
        slot = 6 + animation;
    else if (scene == 6 && animation >= 0 && animation <= 2)
        slot = 14 + animation;
    else if (scene == 9 && animation >= 0 && animation <= 1)
        slot = 18 + animation;
    else if (scene == 10 && animation >= 0 && animation <= 1)
        slot = 11 + animation;
    if (slot == 21) return 0;
    if (hooks != 0 && hooks->request != 0)
        hooks->request(hooks->context, slot, animation);
    return 1;
}

static inline void pair_count_histogram_build(
    const uint32_t pair_values[12], uint32_t counts[2][6],
    uint32_t dominant[2])
{
    uint32_t group;
    uint32_t i;
    if (counts == 0 || dominant == 0) return;
    for (group = 0; group < 2; ++group) {
        uint32_t best = 0;
        dominant[group] = 0;
        for (i = 0; i < 6; ++i) counts[group][i] = 0;
        if (pair_values == 0) continue;
        for (i = 0; i < 6; ++i) {
            uint32_t j;
            const uint32_t value = pair_values[group * 6 + i];
            uint32_t found = 6;
            for (j = 0; j < i; ++j) {
                if (pair_values[group * 6 + j] == value) {
                    found = j;
                    break;
                }
            }
            if (found < 6) continue;
            for (j = i; j < 6; ++j)
                if (pair_values[group * 6 + j] == value)
                    ++counts[group][i];
            if (counts[group][i] > best) {
                best = counts[group][i];
                dominant[group] = value;
            }
        }
    }
}

typedef struct RegionContainmentEntry {
    int left;
    int top;
    int right;
    int bottom;
    int region;
    int matched;
} RegionContainmentEntry;

static inline int region_containment_scan_update(
    RegionContainmentEntry* entries, uint32_t count, int x, int y,
    void (*changed)(void* context, int region, int contained),
    void* context)
{
    uint32_t i;
    int matches = 0;
    if (entries == 0) return 0;
    for (i = 0; i < count; ++i) {
        const int contained = x >= entries[i].left && x < entries[i].right &&
            y >= entries[i].top && y < entries[i].bottom;
        if (contained) ++matches;
        if (contained != entries[i].matched) {
            entries[i].matched = contained;
            if (changed != 0)
                changed(context, entries[i].region, contained);
        }
    }
    return matches;
}

#endif
