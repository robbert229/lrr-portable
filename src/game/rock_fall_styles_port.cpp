// SPDX-License-Identifier: LGPL-3.0-or-later
/* The port tail of GAME-012, the effects unit (finding 589): the port's
 * rock-fall styles, boulder and smash-path markers, fence beams, explosions
 * and MiscObjects effects, on its own records and hook tables. It was
 * relocated verbatim into rock_fall_styles.cpp by tools/relocate_unit.py
 * (ADR 0006; body hashes in docs/coverage/relocations/GAME-012.csv) and is
 * now unannotated: the original-shape unit is rock_fall_styles.c.
 *
 * Natively these definitions serve every caller. In the matching build
 * rock_fall_styles.c defines 15 of these names itself
 * (rock_fall_styles_link_names.h), so their definitions here are compiled
 * out under _MSC_VER < 1200. */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "resource.hpp"
#include "heap.hpp"
#include "record.hpp"
#include "sound.hpp"
#include "string.hpp"
#include "texture.hpp"
#include "view.hpp"
#include "scene_pool.hpp"
#include "resource_for_rock_fall_styles.hpp"
#include <math.h>
#include <float.h>
#include <ctype.h>
#include <errno.h>
#include "crt/crt.hpp"
#include "crt/crt_for_rock_fall_styles.hpp"
#include "gods98/resource_for_rock_fall_styles.hpp"
#include "crt/crt_for_rock_fall_styles.hpp"

/* The port's copy of effectGlobs.rockFallStyleIndex (004c79a4, rock_fall_styles.c). */
uint32_t s_rockfall_base = 0u;      /* DAT_004c79a4 */

static SceneResourceSlotModel s_scene_resource_slots[10];

static float s_bird_scarer_period;

static void* s_scene_resource_slot_instances[10][10];

static void* s_scene_availability_parents[2];
static void* s_scene_act_parents[SCENE_AVAILABILITY_STYLE_CAPACITY][3];

static int s_scene_pose_slot_claimed[SCENE_AVAILABILITY_STYLE_CAPACITY][3][4];

/* Defined below (forward declarations added by tools/relocate_unit.py). */
static void scene_light_pool_expire(void** pool, float delta);
static Resource* resource_load_complete(const char* path);

namespace unported_ledger_stubs {
// SetTimeoutTo10000
/* Port model of 0x0040bdc0 (GAME-012); the original-shape function is in rock_fall_styles.c. */
#if !(defined(_MSC_VER) && _MSC_VER < 1200) /* rock_fall_styles_link_names.h */
extern "C" void ledger_stub_0040bdc0_SetTimeoutTo10000(void)
{
    uint32_t* ignored_output;
    (void)scene_resource_availability_sweep(10000.0f, &ignored_output,
                                             &ignored_output);
}
#endif
} // namespace unported_ledger_stubs

/* SceneMarkerCreateAtObjectPosition (FUN_0040bde0). */
/* Port model of 0x0040bde0 (GAME-012); the original-shape function is in rock_fall_styles.c. */
void scene_marker_create_at_object_position(void* object)
{
    scene_marker_create_at_position_or_object(object, NULL);
}
/* SceneLightCreateAtPosition (FUN_0040be10): clone the scene-selected light
 * template into the first free four-entry pool slot and place it. */
/* Port model of 0x0040be10 (GAME-012); the original-shape function is in rock_fall_styles.c. */
#if !(defined(_MSC_VER) && _MSC_VER < 1200) /* rock_fall_styles_link_names.h */
void scene_light_create_at_position(const float position[3])
{
    int slot;
    int template_index = s_scene_marker_scene_variant == 2 ? 1 : 0;
    void* template_handle = s_scene_marker_templates[template_index];
    if (position == NULL || template_handle == NULL
        || s_resource_child_create_hook == NULL
        || s_scene_effect_pose_hook == NULL) {
        return;
    }
    for (slot = 0; slot < 4; ++slot) {
        if (s_scene_light_primary[slot] == NULL) {
            void* instance = s_resource_child_create_hook(
                template_handle, s_resource_instance_context);
            if (instance != NULL && s_scene_effect_pose_hook(instance,
                    position[0], position[1], position[2], 0.0f, 1.0f, 0.0f,
                    s_scene_effect_pose_context) != 0) {
                s_scene_light_primary[slot] = instance;
            } else if (instance != NULL && s_scene_release_hook != NULL) {
                s_scene_release_hook(instance, s_scene_lifecycle_context);
            }
            return;
        }
    }
}
#endif

/* SceneMarkerCreateAtPositionOrObject (FUN_0040beb0): use explicit
 * coordinates for a null object, otherwise ask the object-position seam. */
/* Port model of 0x0040beb0 (GAME-012); the original-shape function is in rock_fall_styles.c. */
void scene_marker_create_at_position_or_object(void* object,
                                               const float position[3])
{
    int slot;
    float resolved[3];
    if (object == NULL) {
        if (position == NULL) {
            return;
        }
        resolved[0] = position[0];
        resolved[1] = position[1];
        resolved[2] = position[2];
    } else if (s_scene_object_position_hook == NULL
               || s_scene_object_position_hook(object, resolved,
                        s_scene_object_position_context) == 0) {
        return;
    }
    if (s_scene_marker_templates[2] == NULL
        || s_resource_child_create_hook == NULL
        || s_scene_effect_pose_hook == NULL) {
        return;
    }
    for (slot = 0; slot < 4; ++slot) {
        if (s_scene_light_secondary[slot] == NULL) {
            void* instance = s_resource_child_create_hook(
                s_scene_marker_templates[2], s_resource_instance_context);
            if (instance != NULL && s_scene_effect_pose_hook(instance,
                    resolved[0], resolved[1], resolved[2], 0.0f, 1.0f, 0.0f,
                    s_scene_effect_pose_context) != 0) {
                s_scene_light_secondary[slot] = instance;
            } else if (instance != NULL && s_scene_release_hook != NULL) {
                s_scene_release_hook(instance, s_scene_lifecycle_context);
            }
            return;
        }
    }
}
/* RockFallStyleNameLookup (FUN_0040bf70): case-insensitive name scan over
 * the installed RockFallStyles names. The port caps the owner table and
 * guards null input/output instead of following the DAT table blindly. */
/* Port model of 0x0040bf70 (GAME-012); the original-shape function is in rock_fall_styles.c. */
#if !(defined(_MSC_VER) && _MSC_VER < 1200) /* rock_fall_styles_link_names.h */
int rock_fall_style_name_lookup(const char* name, uint32_t* index)
{
    uint32_t i;
    if (name == NULL || index == NULL) {
        return 0;
    }
    for (i = 0u; i < s_rock_fall_style_name_count; ++i) {
        if (s_rock_fall_style_names[i][0] != '\0'
            && strcasecmp_lite(s_rock_fall_style_names[i], name) == 0) {
            *index = i;
            return 1;
        }
    }
    return 0;
}
#endif

/* Port model of 0x0040bfc0 (GAME-012); the original-shape function is in rock_fall_styles.c. */
#if !(defined(_MSC_VER) && _MSC_VER < 1200) /* rock_fall_styles_link_names.h */
void rockfall_style_table_base_set(uint32_t v)
{
    s_rockfall_base = v;
}
#endif

/* RockFallStylesConfigLoad (FUN_0040bfd0): owns a bounded native copy of
 * each configured name and registers comma-delimited scene resource tokens.
 * The callback stands in for SceneActSlotLoadAndRegister, whose original
 * DirectDraw scene implementation has not been ported. */
/* Port model of 0x0040bfd0 (GAME-012); the original-shape function is in rock_fall_styles.c. */
void rock_fall_styles_config_load(const RockFallStyleConfigEntry* entries,
                                 uint32_t count,
                                 RockFallStyleRegisterHook register_hook,
                                 void* context)
{
    const char* names[ROCK_FALL_STYLE_NAME_CAPACITY];
    uint32_t i;
    if (entries == NULL) {
        rock_fall_style_names_install(NULL, 0u);
        return;
    }
    if (count > ROCK_FALL_STYLE_NAME_CAPACITY) {
        count = ROCK_FALL_STYLE_NAME_CAPACITY;
    }
    memset(s_rock_fall_style_names, 0, sizeof(s_rock_fall_style_names));
    for (i = 0u; i < count; ++i) {
        char* token;
        char* cursor;
        char* parsed[32];
        uint32_t parsed_count = 0u;
        strncpy(s_rock_fall_style_names[i],
                entries[i].name != NULL ? entries[i].name : "",
                ROCK_FALL_STYLE_NAME_LENGTH - 1);
        names[i] = s_rock_fall_style_names[i];
        if (entries[i].resources == NULL || register_hook == NULL) {
            continue;
        }
        token = (char*)malloc(strlen(entries[i].resources) + 1u);
        if (token == NULL) {
            continue;
        }
        strcpy(token, entries[i].resources);
        cursor = token;
        while (*cursor != '\0') {
            char* start = cursor;
            while (*cursor != '\0' && *cursor != ',') {
                ++cursor;
            }
            if (*cursor == ',') {
                *cursor++ = '\0';
            }
            if (*start != '\0' && parsed_count < 32u) {
                parsed[parsed_count++] = start;
            }
        }
        if (*cursor == '\0' && parsed_count == 0u && token[0] != '\0') {
            parsed[parsed_count++] = token;
        }
        if (parsed_count > 1u) {
            uint32_t resource_index;
            for (resource_index = 1u; resource_index < parsed_count;
                 ++resource_index) {
                register_hook(parsed[resource_index], i, resource_index - 1u,
                              parsed[0], context);
            }
        }
        free(token);
    }
    s_rock_fall_style_name_count = count;
}

/* SceneActSlotLoadAndRegister (FUN_0040c0d0): load an LWS typed resource,
 * retry the corpus's ANIM type fallback, then lock its parent and four
 * children into the configured style/group record. */
/* Port model of 0x0040c0d0 (GAME-012); the original-shape function is in rock_fall_styles.c. */
int scene_act_slot_load_and_register(void* scene, const char* path,
                                     int style, int group,
                                     const char* name)
{
    void* parent;
    int slot;
    if (style < 0 || style >= SCENE_AVAILABILITY_STYLE_CAPACITY
        || group < 0 || group >= 3 || path == NULL
        || s_scene_act_load_hook == NULL
        || s_resource_child_create_hook == NULL) {
        return 0;
    }
    parent = s_scene_act_load_hook(scene, path, "LWS", s_scene_act_context);
    if (parent == NULL) {
        parent = s_scene_act_load_hook(scene, path, "ANIM", s_scene_act_context);
    }
    if (parent == NULL) {
        return 0;
    }
    s_scene_act_parents[style][group] = parent;
    if (s_resource_lock_hook != NULL) {
        s_resource_lock_hook(parent, 1, s_resource_instance_context);
    }
    for (slot = 0; slot < 4; ++slot) {
        SceneAvailabilitySlot* child =
            &s_scene_availability_styles[style][group][slot];
        child->handle = s_resource_child_create_hook(
            parent, s_resource_instance_context);
        child->configured = child->handle != NULL ? 1 : 0;
        child->enabled = 0;
        child->pending = 0;
        s_scene_pose_slot_claimed[style][group][slot] = 0;
        if (child->handle != NULL && s_resource_lock_hook != NULL) {
            s_resource_lock_hook(child->handle, 1, s_resource_instance_context);
        }
    }
    if (s_scene_act_name_register_hook != NULL) {
        s_scene_act_name_register_hook(parent, name, (uint32_t)style,
                                       (uint32_t)group, s_scene_act_context);
    }
    return 1;
}
/* IndexedResourceHandleSetCreate (FUN_0040c1b0): create and lock a parent
 * resource, then populate its fixed 30-child transient group. */
/* Port model of 0x0040c1b0 (GAME-012); the original-shape function is in rock_fall_styles.c. */
#if !(defined(_MSC_VER) && _MSC_VER < 1200) /* rock_fall_styles_link_names.h */
int indexed_resource_handle_set_create(void* scene, const char* path,
                                       int group)
{
    int index;
    void* parent;
    if (group < 0 || group >= 2 || s_resource_parent_load_hook == NULL
        || s_resource_child_create_hook == NULL) {
        return 0;
    }
    parent = s_resource_parent_load_hook(scene, path,
                                         s_resource_instance_context);
    if (parent == NULL) {
        return 0;
    }
    s_scene_availability_parents[group] = parent;
    if (s_resource_lock_hook != NULL) {
        s_resource_lock_hook(parent, 1, s_resource_instance_context);
    }
    for (index = 0; index < 30; ++index) {
        SceneAvailabilitySlot* entry =
            &s_scene_availability_transients[group][index];
        entry->handle = s_resource_child_create_hook(
            parent, s_resource_instance_context);
        entry->configured = entry->handle != NULL;
        entry->enabled = 0;
        entry->pending = 0;
        if (entry->handle != NULL && s_resource_lock_hook != NULL) {
            s_resource_lock_hook(entry->handle, 1,
                                 s_resource_instance_context);
        }
    }
    return 1;
}
#endif

/* SceneSlotPoseScaleRegister (FUN_0040c230): claim the first vacant pose
 * slot in a style group, initialize its scene transform through the hook,
 * and retain the paired values used by ReadCellTablePair. */
/* Port model of 0x0040c230 (GAME-012); the original-shape function is in rock_fall_styles.c. */
#if !(defined(_MSC_VER) && _MSC_VER < 1200) /* rock_fall_styles_link_names.h */
int scene_slot_pose_scale_register(int group, uint32_t first,
                                   uint32_t second, float x, float y,
                                   float z, float heading, float pitch)
{
    int slot;
    SceneAvailabilitySlot* model;
    if (group < 0 || group >= 3 || s_scene_slot_pose_hook == NULL) {
        return 0;
    }
    for (slot = 0; slot < 4; ++slot) {
        if (s_scene_pose_slot_claimed[s_scene_availability_selected_style]
                                     [group][slot] != 0) {
            continue;
        }
        model = &s_scene_availability_styles[s_scene_availability_selected_style]
                                            [group][slot];
        if (model->handle == NULL
            || s_scene_slot_pose_hook(model->handle, group, slot, x, y, z,
                                      heading, pitch,
                                      s_scene_slot_pose_context) == 0) {
            return 0;
        }
        s_scene_pose_slot_claimed[s_scene_availability_selected_style]
                                 [group][slot] = 1;
        s_scene_cell_table_pairs[s_scene_availability_selected_style]
                                [group][slot][0] = first;
        s_scene_cell_table_pairs[s_scene_availability_selected_style]
                                [group][slot][1] = second;
        return 1;
    }
    return 0;
}
#endif

/* TransientEffectRenderSlotConfigure (FUN_0040c2f0): configure the first
 * available child in one of the two 30-slot transient groups. */
/* Port model of 0x0040c2f0 (GAME-012); the original-shape function is in rock_fall_styles.c. */
#if !(defined(_MSC_VER) && _MSC_VER < 1200) /* rock_fall_styles_link_names.h */
int transient_effect_render_slot_configure(int group, float x, float y,
                                           float size, float heading,
                                           float dx, float dy)
{
    int slot;
    if (group < 0 || group >= 2) {
        return 0;
    }
    for (slot = 0; slot < 30; ++slot) {
        SceneAvailabilitySlot* entry =
            &s_scene_availability_transients[group][slot];
        if (entry->configured != 0 || entry->pending != 0
            || entry->handle == NULL) {
            continue;
        }
        if (s_transient_effect_configure_hook != NULL) {
            if (s_transient_effect_configure_hook(entry->handle, group, x, y,
                    size, heading, dx, dy,
                    s_transient_effect_configure_context) == 0) {
                return 0;
            }
        }
        else {
            /* The retail slot configurator's four calls are already exposed
             * by the portable DDiScene wrappers. Keep a concrete path for
             * slots populated by an installed scene-resource backend. The
             * wrapper return values are intentionally ignored: the retail
             * body marks this resource pending once it has selected a slot. */
            scene_render_lock_toggle(entry->handle, 0);
            (void)ddiscene_frame_update_and_callbacks(entry->handle, 0.0f);
            (void)ddiscene_object_position_set(entry->handle, NULL,
                                               x, y, size);
            (void)ddiscene_object_pose_method_dispatch(entry->handle, NULL,
                heading, dx, dy, 0.0f, 0.0f, -1.0f);
        }
        entry->pending = 1;
        entry->enabled = 0;
        return 1;
    }
    return 0;
}
#endif
/* SceneLightPoolExpireSweep (FUN_0040c4d0). */
/* Port model of 0x0040c4d0 (GAME-012); the original-shape function is in rock_fall_styles.c. */
void scene_light_pool_primary_expire(float delta)
{
    scene_light_pool_expire(s_scene_light_primary, delta);
}

/* SceneLightSecondaryPoolExpireSweep (FUN_0040c520). */
/* Port model of 0x0040c520 (GAME-012); the original-shape function is in rock_fall_styles.c. */
void scene_light_pool_secondary_expire(float delta)
{
    scene_light_pool_expire(s_scene_light_secondary, delta);
}

/* ReadCellTablePair (FUN_0040c570): reads the two words in the selected
 * style's group slot. Invalid native indices return zero values. */
/* Port model of 0x0040c570 (GAME-012); the original-shape function is in rock_fall_styles.c. */
#if !(defined(_MSC_VER) && _MSC_VER < 1200) /* rock_fall_styles_link_names.h */
void scene_cell_table_pair_read(int group, int slot, uint32_t* first,
                                uint32_t* second)
{
    uint32_t a = 0u;
    uint32_t b = 0u;
    if (group >= 0 && group < 3 && slot >= 0 && slot < 4) {
        a = s_scene_cell_table_pairs[s_scene_availability_selected_style]
                                    [group][slot][0];
        b = s_scene_cell_table_pairs[s_scene_availability_selected_style]
                                    [group][slot][1];
    }
    if (first != NULL) {
        *first = a;
    }
    if (second != NULL) {
        *second = b;
    }
}
#endif

/* ResourceSlotRegisterName (FUN_0040c5b0): find the matching handle pair,
 * claim its next available registration bit, and delegate name formatting
 * and application through the engine-facing hooks. */
/* Port model of 0x0040c5b0 (GAME-012); the original-shape function is in rock_fall_styles.c. */
#if !(defined(_MSC_VER) && _MSC_VER < 1200) /* rock_fall_styles_link_names.h */
int scene_resource_slot_register_name(uint32_t handle_key,
                                      uint32_t secondary_key)
{
    int group;
    for (group = 0; group < 3; ++group) {
        int slot;
        for (slot = 0; slot < 4; ++slot) {
            SceneResourceNameSlot* entry =
                &s_scene_resource_name_slots[s_scene_availability_selected_style]
                                            [group][slot];
            uint32_t index;
            if (entry->handle == NULL || entry->handle_key != handle_key
                || entry->secondary_key != secondary_key) {
                continue;
            }
            for (index = 0u; index < entry->name_count && index < 32u; ++index) {
                uint32_t bit = 1u << index;
                void* formatted;
                if ((entry->registered & bit) != 0u) {
                    continue;
                }
                entry->registered |= bit;
                if (s_resource_name_format_hook == NULL
                    || s_resource_name_apply_hook == NULL) {
                    return 0;
                }
                formatted = s_resource_name_format_hook(entry->handle,
                    entry->name_base, index, s_resource_name_context);
                if (formatted == NULL) {
                    return 0;
                }
                s_resource_name_apply_hook(entry->handle, formatted,
                                           s_resource_name_context);
                return 1;
            }
            return 0;
        }
    }
    return 0;
}
#endif
/* SceneResourceSingletonPoolRelease (FUN_0040c690): release and clear the
 * four primary singleton handles. */
/* Port model of 0x0040c690 (GAME-012); the original-shape function is in rock_fall_styles.c. */
#if !(defined(_MSC_VER) && _MSC_VER < 1200) /* rock_fall_styles_link_names.h */
void scene_resource_singleton_pool_release(void)
{
    for (int i = 0; i < 4; ++i) {
        if (s_scene_light_primary[i] != NULL) {
            if (s_scene_release_hook != NULL) {
                s_scene_release_hook(s_scene_light_primary[i],
                                    s_scene_lifecycle_context);
            }
            s_scene_light_primary[i] = NULL;
        }
    }
}
#endif

/* RockFallStyleSceneResourcesRelease (FUN_0040c6c0): release the four
 * resource handles in each of three groups for the selected style. */
/* Port model of 0x0040c6c0 (GAME-012); the original-shape function is in rock_fall_styles.c. */
#if !(defined(_MSC_VER) && _MSC_VER < 1200) /* rock_fall_styles_link_names.h */
void rock_fall_style_scene_resources_release(void)
{
    int group;
    for (group = 0; group < 3; ++group) {
        int slot;
        for (slot = 0; slot < 4; ++slot) {
            SceneAvailabilitySlot* entry =
                &s_scene_availability_styles[s_scene_availability_selected_style]
                                             [group][slot];
            if (entry->handle != NULL && s_scene_release_hook != NULL) {
                s_scene_release_hook(entry->handle, s_scene_lifecycle_context);
            }
            entry->handle = NULL;
            entry->configured = 0;
            entry->enabled = 0;
            entry->pending = 0;
            s_scene_pose_slot_claimed[s_scene_availability_selected_style]
                                     [group][slot] = 0;
        }
    }
}
#endif
/* GlobalResourceHandleLoadAndEnable (FUN_0040c720): load and retain the
 * resource, with enable represented by the stored non-null global handle. */
/* Port model of 0x0040c720 (GAME-012); the original-shape function is in rock_fall_styles.c. */
int global_resource_handle_load_and_enable(const char* path)
{
    Resource* resource = resource_load_complete(path);
    if (resource == NULL) {
        return 0;
    }
    if (s_global_resource_handle != NULL) {
        global_resource_handle_release();
    }
    s_global_resource_handle = resource;
    return 1;
}
/* TerrainPointObjectCreate (FUN_0040c750): resolve the point, place it one
 * scene unit below the terrain surface, and retain it in the four-object
 * expiry pool. */
/* Port model of 0x0040c750 (GAME-012); the original-shape function is in rock_fall_styles.c. */
void terrain_point_object_create(void* object, const float* xy)
{
    float position[3];
    float height;
    uint32_t index;
    void* instance;
    if (object == NULL) {
        if (xy == NULL) {
            return;
        }
        position[0] = xy[0];
        position[1] = xy[1];
    } else if (s_scene_object_position_hook == NULL
               || s_scene_object_position_hook(object, position,
                        s_scene_object_position_context) == 0) {
        return;
    }
    if (s_terrain_height_hook == NULL || s_terrain_point_template == NULL
        || s_resource_child_create_hook == NULL
        || s_scene_effect_pose_hook == NULL
        || s_terrain_height_hook(position[0], position[1], &height,
                                 s_terrain_height_context) == 0
        || s_terrain_point_object_count >= 4u) {
        return;
    }
    position[2] = height - 1.0f;
    instance = s_resource_child_create_hook(s_terrain_point_template,
                                             s_resource_instance_context);
    if (instance == NULL || s_scene_effect_pose_hook(instance,
            position[0], position[1], position[2], 0.0f, 1.0f, 0.0f,
            s_scene_effect_pose_context) == 0) {
        if (instance != NULL && s_scene_release_hook != NULL) {
            s_scene_release_hook(instance, s_scene_lifecycle_context);
        }
        return;
    }
    index = s_terrain_point_object_count++;
    s_terrain_point_objects[index] = instance;
}

/* TerrainPointObjectsRemoveMatching (FUN_0040c830): advance each live
 * point object and remove completed objects with swap-with-last compaction. */
/* Port model of 0x0040c830 (GAME-012); the original-shape function is in rock_fall_styles.c. */
void terrain_point_objects_remove_matching(float delta)
{
    uint32_t index = 0u;
    if (s_terrain_point_object_count > 4u) {
        s_terrain_point_object_count = 4u;
    }
    while (index < s_terrain_point_object_count) {
        void* handle = s_terrain_point_objects[index];
        if (handle != NULL && s_scene_advance_hook != NULL
            && s_scene_advance_hook(handle, delta,
                                    s_scene_lifecycle_context) != 0) {
            if (s_scene_release_hook != NULL) {
                s_scene_release_hook(handle, s_scene_lifecycle_context);
            }
            --s_terrain_point_object_count;
            s_terrain_point_objects[index] =
                s_terrain_point_objects[s_terrain_point_object_count];
            s_terrain_point_objects[s_terrain_point_object_count] = NULL;
        } else {
            ++index;
        }
    }
}
/* GetResourceSlotAddressByIndex (FUN_0040c8a0): return the selected slot
 * record from the ten-entry global band; invalid indices answer NULL. */
/* Port model of 0x0040c8a0 (GAME-012); the original-shape function is in rock_fall_styles.c. */
SceneResourceSlotModel* resource_slot_address_by_index(int index)
{
    if (index < 0 || index >= 10) {
        return NULL;
    }
    return &s_scene_resource_slots[index];
}

/* ResourceHandleLoadWithFallback (FUN_0040c920): load the primary resource,
 * retry the fallback when absent, and mark the selected port slot enabled. */
/* Port model of 0x0040c920 (GAME-012); the original-shape function is in rock_fall_styles.c. */
int resource_handle_load_with_fallback(int index, const char* primary,
                                       const char* fallback)
{
    SceneResourceSlotModel* slot = resource_slot_address_by_index(index);
    Resource* resource;
    if (slot == NULL || (primary == NULL && fallback == NULL)) {
        return 0;
    }
    resource = resource_load_complete(primary);
    if (resource == NULL) {
        resource = resource_load_complete(fallback);
    }
    if (resource == NULL) {
        return 0;
    }
    if (slot->handle != NULL) {
        resource_free((Resource*)slot->handle);
    }
    slot->handle = resource;
    slot->selected = 1u;
    return 1;
}

/* MiscObjectsEffectSlotBind (FUN_0040c990): bind the fixed ten misc-effect
 * resource slots using primary/fallback resource descriptors. */
/* Port model of 0x0040c990 (GAME-012); the original-shape function is in rock_fall_styles.c. */
int misc_objects_effect_slots_bind(const MiscEffectResourceEntry* entries,
                                   uint32_t count)
{
    uint32_t index;
    int loaded = 0;
    if (entries == NULL) {
        return 0;
    }
    if (count > 10u) {
        count = 10u;
    }
    for (index = 0u; index < count; ++index) {
        if (resource_handle_load_with_fallback((int)index,
                entries[index].primary, entries[index].fallback) != 0) {
            ++loaded;
        }
    }
    return loaded;
}
/* SceneTimeoutEntrySweep (FUN_0040cb70): advance each handle and remove
 * completed entries by swapping in the last live handle. The bird-scarer
 * slot periodically invokes the original camera/effect path through a hook. */
/* Port model of 0x0040cb70 (GAME-012); the original-shape function is in rock_fall_styles.c. */
void scene_timeout_entry_sweep(SceneTimeoutEntrySet* set, float delta)
{
    uint32_t index = 0u;
    if (set == NULL) {
        return;
    }
    if (set->count > 10u) {
        set->count = 10u;
    }
    while (index < set->count) {
        void* handle = set->entries[index];
        int expired = 0;
        if (handle == NULL) {
            set->entries[index] = set->entries[set->count - 1u];
            set->entries[set->count - 1u] = NULL;
            --set->count;
            continue;
        }
        if (set == &s_terrain_marker_sets[8]) {
            s_bird_scarer_period += delta;
            if (s_bird_scarer_period > 12.5f) {
                if (s_bird_scarer_effect_hook != NULL) {
                    s_bird_scarer_effect_hook(handle,
                                              s_scene_lifecycle_context);
                }
                s_bird_scarer_period = 0.0f;
            }
        }
        if (s_scene_advance_hook != NULL) {
            expired = s_scene_advance_hook(handle, delta,
                                           s_scene_lifecycle_context);
        }
        if (expired != 0) {
            if (s_scene_release_hook != NULL) {
                s_scene_release_hook(handle, s_scene_lifecycle_context);
            }
            --set->count;
            set->entries[index] = set->entries[set->count];
            set->entries[set->count] = NULL;
            continue;
        }
        ++index;
    }
}

/* TerrainMarkerSetsClear (FUN_0040cc40): sweep all ten marker sets in table
 * order using the same timeout contract. */
/* Port model of 0x0040cc40 (GAME-012); the original-shape function is in rock_fall_styles.c. */
void terrain_marker_sets_clear(float delta)
{
    for (int i = 0; i < 10; ++i) {
        scene_timeout_entry_sweep(&s_terrain_marker_sets[i], delta);
    }
}

/* LavaEffectSelectAndPlace (FUN_0040cce0): clone a configured effect resource
 * into its per-slot ten-instance pool and place it at the requested pose. */
/* Port model of 0x0040cce0 (GAME-012); the original-shape function is in rock_fall_styles.c. */
#if !(defined(_MSC_VER) && _MSC_VER < 1200) /* rock_fall_styles_link_names.h */
int lava_effect_select_and_place(int index, const float* position,
                                 const float* rotation)
{
    SceneResourceSlotModel* slot = resource_slot_address_by_index(index);
    void* instance;
    int pose_applied;
    float rx = 0.0f;
    float ry = 1.0f;
    float rz = 0.0f;
    if (slot == NULL || position == NULL || slot->handle == NULL
        || slot->count >= 10u || s_resource_child_create_hook == NULL) {
        return 0;
    }
    /* FUN_0040cce0 clears the slot word at +0x2c before cloning. */
    slot->selected = 0u;
    if (rotation != NULL) {
        rx = rotation[0];
        ry = rotation[1];
        rz = rotation[2];
    }
    instance = s_resource_child_create_hook(slot->handle,
                                             s_resource_instance_context);
    if (instance == NULL) {
        return 0;
    }
    if (s_scene_effect_pose_hook != NULL) {
        pose_applied = s_scene_effect_pose_hook(instance, position[0],
            position[1], position[2], rx, ry, rz,
            s_scene_effect_pose_context);
    }
    else {
        /* The retail path advances the clone at time zero, sets its position,
         * then dispatches its pose vtable method with the supplied rotation
         * and terminal -1. These DDiScene leaves are available even when a
         * host-specific pose adapter is not installed. */
        (void)ddiscene_frame_update_and_callbacks(instance, 0.0f);
        pose_applied = ddiscene_object_position_set(instance, NULL,
            position[0], position[1], position[2]);
        if (pose_applied != 0) {
            pose_applied = ddiscene_object_pose_method_dispatch(instance,
                NULL, rx, ry, rz, 0.0f, 0.0f, -1.0f);
        }
    }
    if (pose_applied == 0) {
        if (s_scene_release_hook != NULL) {
            s_scene_release_hook(instance, s_scene_lifecycle_context);
        }
        return 0;
    }
    s_scene_resource_slot_instances[index][slot->count] = instance;
    ++slot->count;
    return 1;
}
#endif
static void scene_light_pool_expire(void** pool, float delta)
{
    for (int i = 0; i < 4; ++i) {
        void* handle = pool[i];
        if (handle != NULL && s_scene_advance_hook != NULL
            && s_scene_advance_hook(handle, delta,
                                    s_scene_lifecycle_context) != 0) {
            if (s_scene_release_hook != NULL) {
                s_scene_release_hook(handle, s_scene_lifecycle_context);
            }
            pool[i] = NULL;
        }
    }
}

static Resource* resource_load_complete(const char* path)
{
    Resource* resource;
    if (path == NULL || path[0] == '\0') {
        return NULL;
    }
    resource = resource_open(path, RESOURCE_AUTO);
    if (resource == NULL) {
        return NULL;
    }
    if (resource_read(resource) < 0) {
        resource_free(resource);
        return NULL;
    }
    return resource;
}

/* Original-ABI adapter (ADR 0005 step 6) for GAME-021's lego_unit.c: the
 * original FUN_0040c720 takes the root container and the path; the port's
 * entry above takes the path only. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (global_resource_handle_load_and_enable 0040c720)
#if !(defined(_MSC_VER) && _MSC_VER < 1200) /* rock_fall_styles_link_names.h */
extern "C" int global_resource_handle_load_and_enable_original(void* root,
                                                               const char* path)
{
    (void)root;
    return global_resource_handle_load_and_enable(path);
}
#endif

/* Original-ABI adapter (ADR 0005 step 6) for GAME-021's lego_unit.c: the
 * original FUN_0040c990 reads the MiscObjects entries from the config
 * itself; the port's entry above takes them parsed (lego.cpp's loader
 * does the parsing). Nothing calls this natively, so it does nothing. */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (misc_objects_effect_slots_bind 0040c990)
#if !(defined(_MSC_VER) && _MSC_VER < 1200) /* rock_fall_styles_link_names.h */
extern "C" void misc_objects_effect_slots_bind_original(BlockFile* config,
                                                        const char* gameName,
                                                        void* root)
{
    (void)config;
    (void)gameName;
    (void)root;
}
#endif
