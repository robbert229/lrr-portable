// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef LEGORRR_GAME_POINTER_CURSOR_HPP
#define LEGORRR_GAME_POINTER_CURSOR_HPP

#include <stdint.h>

enum { POINTER_CURSOR_COUNT = 56 };
typedef struct PointerCursorSlot {
    void* resource;
    int animated;
    int hot_x;
    int hot_y;
} PointerCursorSlot;
typedef struct PointerCursorState {
    const char* names[POINTER_CURSOR_COUNT];
    PointerCursorSlot slots[POINTER_CURSOR_COUNT];
    void* default_surface;
    int selected;
    float override_timer;
} PointerCursorState;
typedef struct PointerCursorHooks {
    void* context;
    int (*entry_get)(void* context, void* section, uint32_t index,
                     const char** name_out, const char** path_out,
                     int* animated_out, int* hot_x_out, int* hot_y_out);
    uint32_t (*render_flags_get)(void* context);
    void* (*bitmap_load)(void* context, const char* path);
    void* (*animation_load)(void* context, const char* path);
    void (*bitmap_initialize)(void* context, void* bitmap);
    void (*bitmap_draw)(void* context, void* bitmap, float x, float y);
    void (*animation_draw)(void* context, void* animation, float x, float y,
                           int phase, int mode);
} PointerCursorHooks;

static inline void pointer_cursor_registry_init(PointerCursorState* state)
{
    static const char* names[POINTER_CURSOR_COUNT] = {
        "Pointer_Standard", "Pointer_Blank", "Pointer_Selected",
        "Pointer_Drill", "Pointer_CantDrill", "Pointer_Clear",
        "Pointer_Go", "Pointer_CantGo", "Pointer_Teleport",
        "Pointer_CantTeleport", "Pointer_Reinforce", "Pointer_CantReinforce",
        "Pointer_RadarPan", "Pointer_TrackObject", "Pointer_Help",
        "Pointer_CantHelp", "Pointer_PutDown", "Pointer_GetIn",
        "Pointer_GetOut", "Pointer_TutorialBlockInfo", "Pointer_Okay",
        "Pointer_NotOkay", "Pointer_CanBuild", "Pointer_CannotBuild",
        "Pointer_Dynamite", "Pointer_CantDynamite", "Pointer_PickUp",
        "Pointer_CantPickUp", "Pointer_PickUpOre", "Pointer_LegoManCantDig",
        "Pointer_VehicleCantDig", "Pointer_LegoManDig", "Pointer_VehicleDig",
        "Pointer_LegoManCantPickUp", "Pointer_VehicleCantPickUp",
        "Pointer_LegoManPickUp", "Pointer_VehiclePickUp",
        "Pointer_LegoManCantGo", "Pointer_VehicleCantGo", "Pointer_LegoManGo",
        "Pointer_VehicleGo", "Pointer_LegoManClear", "Pointer_VehicleClear",
        "Pointer_SurfaceType_Immovable", "Pointer_SurfaceType_Hard",
        "Pointer_SurfaceType_Medium", "Pointer_SurfaceType_Loose",
        "Pointer_SurfaceType_Soil", "Pointer_SurfaceType_Lava",
        "Pointer_SurfaceType_Water", "Pointer_SurfaceType_OreSeam",
        "Pointer_SurfaceType_Lake", "Pointer_SurfaceType_CrystalSeam",
        "Pointer_SurfaceType_RechargeSeam", "Pointer_CantZoom", "Pointer_Zoom"
    };
    unsigned int i;
    if (state == NULL) return;
    for (i = 0; i < POINTER_CURSOR_COUNT; ++i) {
        state->names[i] = names[i];
        state->slots[i].resource = NULL;
        state->slots[i].animated = 0;
        state->slots[i].hot_x = 0;
        state->slots[i].hot_y = 0;
    }
    state->default_surface = NULL;
    state->selected = 0;
    state->override_timer = 0.0f;
}

static inline int pointer_cursor_name_equal(const char* first,
                                            const char* second)
{
    if (first == NULL || second == NULL) return 0;
    while (*first != '\0' && *second != '\0') {
        char a = *first++;
        char b = *second++;
        if (a >= 'A' && a <= 'Z') a = (char)(a + ('a' - 'A'));
        if (b >= 'A' && b <= 'Z') b = (char)(b + ('a' - 'A'));
        if (a != b) return 0;
    }
    return *first == *second;
}

static inline int pointer_cursor_name_resolve(const PointerCursorState* state,
                                               const char* name,
                                               int* index_out)
{
    unsigned int i;
    if (state == NULL || name == NULL || index_out == NULL) return 0;
    for (i = 0; i < POINTER_CURSOR_COUNT; ++i) {
        if (pointer_cursor_name_equal(state->names[i], name)) {
            *index_out = (int)i;
            return 1;
        }
    }
    return 0;
}

static inline uint32_t pointer_cursor_config_load(
    PointerCursorState* state, void* section, const PointerCursorHooks* hooks)
{
    uint32_t loaded = 0;
    uint32_t entry_index;
    if (state == NULL || hooks == NULL || hooks->entry_get == NULL) return 0;
    if (state->names[0] == NULL) pointer_cursor_registry_init(state);
    for (entry_index = 0; entry_index < 256; ++entry_index) {
        const char* name = NULL;
        const char* path = NULL;
        int animated = 0;
        int hot_x = 0;
        int hot_y = 0;
        int slot;
        if (!hooks->entry_get(hooks->context, section, entry_index, &name,
                              &path, &animated, &hot_x, &hot_y)) break;
        if (name != NULL && name[0] == '!') {
            const uint32_t flags = hooks->render_flags_get == NULL ? 0u
                : hooks->render_flags_get(hooks->context);
            if ((flags & 0x02000000u) != 0u) continue;
            ++name;
        }
        if (!pointer_cursor_name_resolve(state, name, &slot) || path == NULL)
            continue;
        if (animated) {
            if (hooks->animation_load == NULL) continue;
            state->slots[slot].resource =
                hooks->animation_load(hooks->context, path);
            state->slots[slot].animated = 1;
            state->slots[slot].hot_x = hot_x;
            state->slots[slot].hot_y = hot_y;
        }
        else {
            if (hooks->bitmap_load == NULL) continue;
            state->slots[slot].resource = hooks->bitmap_load(hooks->context,
                                                            path);
            state->slots[slot].animated = 0;
            state->slots[slot].hot_x = 0;
            state->slots[slot].hot_y = 0;
            if (state->slots[slot].resource != NULL &&
                hooks->bitmap_initialize != NULL)
                hooks->bitmap_initialize(hooks->context,
                                         state->slots[slot].resource);
        }
        if (state->slots[slot].resource != NULL) ++loaded;
    }
    return loaded;
}

static inline void pointer_cursor_draw(PointerCursorState* state,
                                       const PointerCursorHooks* hooks,
                                       uint32_t x, uint32_t y)
{
    int selected;
    PointerCursorSlot* slot;
    if (state == NULL || hooks == NULL) return;
    selected = state->selected;
    if (selected < 0 || selected >= POINTER_CURSOR_COUNT) selected = 0;
    slot = &state->slots[selected];
    if (!slot->animated) {
        void* bitmap = slot->resource != NULL ? slot->resource
                                               : state->slots[0].resource;
        if (bitmap == NULL) bitmap = state->default_surface;
        if (bitmap != NULL && hooks->bitmap_draw != NULL)
            hooks->bitmap_draw(hooks->context, bitmap, (float)x, (float)y);
        return;
    }
    if (slot->resource != NULL && hooks->animation_draw != NULL) {
        const uint32_t flags = hooks->render_flags_get == NULL ? 0u
            : hooks->render_flags_get(hooks->context);
        /* The original draws the blank cursor surface before the animated
         * cursor so the animation has a clean background at the pointer. */
        if (state->slots[1].resource != NULL && hooks->bitmap_draw != NULL)
            hooks->bitmap_draw(hooks->context, state->slots[1].resource,
                               (float)x, (float)y);
        hooks->animation_draw(hooks->context, slot->resource,
            (float)((int)x + slot->hot_x), (float)((int)y + slot->hot_y),
            (int)((~flags >> 20) & 1u), 1);
    }
}

#endif
