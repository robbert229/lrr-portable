// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#endif
#else
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "resource.hpp"
#include "heap.hpp"
#include "record.hpp"
#include "sound.hpp"
#include "string.hpp"
#include "texture.hpp"
#include "view.hpp"
#include "scene_pool.hpp"
#include "resource_shared.hpp"
#include "resource_for_com_object_tree.hpp"
#include "resource_for_wad.hpp"
#include "resource_from_wad.hpp"

// GLOBAL: LEGORR 0x0049f988
const unsigned char s_resource_tree_child_iid[16] = {
    0x70, 0x7f, 0x6b, 0xff, 0x0e, 0xa4, 0xd1, 0x11,
    0x91, 0xf9, 0x00, 0x00, 0xf8, 0x75, 0x8e, 0x66
};

/* Shared file-layer diagnostics used by the reference dispatchers. */
// GLOBAL: LEGORR 0x004ab028
static const char s_game_file_unknown_type_format[] =
    "%s(%i) : Unknown file system in call to %s";
// GLOBAL: LEGORR 0x004aafd4
static const char s_game_file_source_path[] =
    "C:\\Dev\\SourceSafe\\gods98_dx6\\gods98\\src\\Files.c";
// GLOBAL: LEGORR 0x004ab054
static const char s_game_file_seek_name[] = "File_Seek";
// GLOBAL: LEGORR 0x004ab010
static const char s_game_file_seek_mode_format[] = "Uknown seek mode (%i)";
// GLOBAL: LEGORR 0x004ab0a0
static const char s_game_file_tell_name[] = "File_Tell";
static const char s_game_file_open_name[] = "File_Open";
static const char s_game_file_close_name[] = "File_Close";
static const char s_game_file_write_name[] = "File_Write";
static const char s_game_file_write_wad_error[] =
    "Cannot write to a file stored in a WAD";
// GLOBAL: LEGORR 0x004ab0b8
static const char s_game_file_gets_name[] = "File_GetS";
// GLOBAL: LEGORR 0x004ab0ac
static const char s_game_file_getc_name[] = "File_GetC";
// GLOBAL: LEGORR 0x004ab0c4
static const char s_game_file_printf_wad_error[] =
    "\"fprintf\" is unsupprted for wad files";
// GLOBAL: LEGORR 0x004ab0ec
static const char s_game_file_printf_name[] = "File_PrintF";

/* Explicit vtable layouts used by the three small resource wrappers below.
 * The corpus uses 32-bit COM-style interfaces; these declarations preserve
 * their method slots without importing a platform SDK. */
class ResourcePropertyInterface
{
public:
    virtual void slot_00(void) = 0;
    virtual void slot_04(void) = 0;
    virtual void slot_08(void) = 0;
    virtual void slot_0c(void) = 0;
    virtual void slot_10(void) = 0;
    virtual void slot_14(void) = 0;
    virtual void slot_18(void) = 0;
    virtual void* property_get(void) = 0;
};

char s_rock_fall_style_names[ROCK_FALL_STYLE_NAME_CAPACITY]
                                    [ROCK_FALL_STYLE_NAME_LENGTH];
uint32_t s_rock_fall_style_name_count;
SceneTimeoutEntrySet s_terrain_marker_sets[10];
void* s_scene_light_primary[4];
void* s_scene_light_secondary[4];
void* s_scene_marker_templates[3];
int s_scene_marker_scene_variant;
SceneObjectPositionHook s_scene_object_position_hook;
void* s_scene_object_position_context;
void* s_terrain_point_template;
TerrainHeightAtPointHook s_terrain_height_hook;
void* s_terrain_height_context;
SceneResourceAdvanceHook s_scene_advance_hook;
SceneResourceReleaseHook s_scene_release_hook;
SceneTimeoutEffectHook s_bird_scarer_effect_hook;
static SceneTimeoutSetCallback s_scene_timeout_set_callback;
void* s_global_resource_handle;
static GlobalResourceReleaseFn s_global_resource_release;
static void* s_global_resource_user;
static GlobalResourceCleanupFn s_global_resource_cleanup;
static void* s_global_resource_cleanup_user;
static void* s_scroll_rect_payload_records[10];
static PanelResourceLoadFn s_panel_resource_loader;
static void* s_panel_resource_loader_user;
static ObjectListCountFn s_object_list_count;
static ObjectListSetFn s_object_list_set;
static void* s_object_list_user;
static ResourceListCreateFn s_resource_list_create;
static ResourceListUpdateFn s_resource_list_update;
static void* s_resource_list_user;
static OwnedObjectReleaseFn s_owned_object_release;
static OwnedObjectReleaseFn s_owned_scene_destroy;
static OwnedObjectReleaseFn s_owned_node_free;
static void* s_owned_release_user;
static char* s_string_table_names[50];
static int s_string_table_current = -1;
static int s_string_table_enabled;
static const char* s_message_strings[65];
static int s_loading_progress_enabled;
static int s_loading_progress_total;
static int s_loading_progress_units;
static int s_loading_progress_mode;
static float s_loading_progress_last = -1.0f;
static float s_loading_progress_rect[4];
static FileLoadingProgressDrawFn s_loading_progress_draw;
static void* s_loading_progress_user;
static void* s_indexed_surface;
static void* s_indexed_surface_texture;
static uint32_t* s_indexed_surface_entries;
static int s_indexed_surface_width;
static int s_indexed_surface_height;
static IndexedSurfaceCreateFn s_indexed_surface_create;
static IndexedTextureLoadFn s_indexed_texture_load;
static IndexedSurfaceReleaseFn s_indexed_surface_release;
static IndexedSurfaceTextureSetFn s_indexed_surface_texture_set;
static void* s_indexed_surface_user;
static IndexedSurfaceReleaseFn s_indexed_surface_owner_release;
static void* s_indexed_surface_owner_user;
static int s_indexed_surface_native_frame;

#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
static void indexed_surface_native_release(void* object, void* user)
{
    (void)user;
    ddiscene_object_destroy_with_mode(object, 0);
}

static void* indexed_surface_native_create(void* owner)
{
    lego::DDiSceneObjectDescriptor parents;
    if (owner == NULL || owner != scene_root_object_get() ||
        !lego::ddiscene_object_pool_owns(owner)) return NULL;
    memcpy(parents.channels, owner, sizeof(parents.channels));
    /* The software mesh has its own CPU geometry. This pooled object owns
     * the three genuine native frames and their visibility/parent path;
     * neither a software quad index nor a fake mesh interface is used. */
    return lego::ddiscene_object_allocate(&parents);
}
#endif
static VisualModelResolveFn s_visual_model_resolver;
static void* s_visual_model_resolver_user;
void* s_scene_lifecycle_context;
void* s_terrain_point_objects[4];
uint32_t s_terrain_point_object_count;
SceneEffectPoseHook s_scene_effect_pose_hook;
void* s_scene_effect_pose_context;
void* s_numbered_bitmap_slots[11];
NumberedBitmapLoadHook s_numbered_bitmap_load_hook;
void* s_numbered_bitmap_context;
SceneAnimationDurationHook s_animation_duration_hook;
SceneAnimationLastFrameHook s_animation_last_frame_hook;
void* s_animation_duration_context;
SceneResourceNamedResolveHook s_scene_resource_named_resolve_hook;
static SceneObjectResourceReleaseHook s_scene_object_resource_release_hook;
void* s_scene_resource_record_context;
SceneAnimationTrackApplyHook s_animation_track_apply_hook;
void* s_animation_track_context;
SceneResourceOwnerApplyHook s_scene_resource_owner_apply_hook;
static SceneAttachedResourceReleaseHook s_scene_attached_resource_release_hook;
void* s_scene_resource_owner_context;
SceneOwnedMemoryReleaseHook s_scene_owned_memory_release_hook;
void* s_scene_owned_memory_context;
SceneAnimationChannelUpdateHook s_animation_channel_update_hook;
SceneAnimationChannelQueryHook s_animation_channel_query_hook;
SceneAnimationVisibilityHook s_animation_visibility_hook;
void* s_animation_channel_context;
SceneIndexedRecordNodeReleaseHook s_scene_indexed_record_release_hook;
void* s_scene_indexed_record_context;
SceneChannelEntryProcessHook s_scene_channel_entry_process_hook;
void* s_scene_channel_entry_context;
SceneAvailabilitySlot
    s_scene_availability_styles[SCENE_AVAILABILITY_STYLE_CAPACITY][3][4];
uint32_t s_scene_cell_table_pairs[SCENE_AVAILABILITY_STYLE_CAPACITY]
                                         [3][4][2];
SceneResourceNameSlot
    s_scene_resource_name_slots[SCENE_AVAILABILITY_STYLE_CAPACITY][3][4];
SceneResourceNameFormatHook s_resource_name_format_hook;
SceneResourceNameApplyHook s_resource_name_apply_hook;
void* s_resource_name_context;
SceneResourceParentLoadHook s_resource_parent_load_hook;
SceneResourceChildCreateHook s_resource_child_create_hook;
SceneResourceLockHook s_resource_lock_hook;
void* s_resource_instance_context;
SceneAvailabilitySlot s_scene_availability_transients[2][30];
SceneActLoadHook s_scene_act_load_hook;
SceneActNameRegisterHook s_scene_act_name_register_hook;
void* s_scene_act_context;
SceneSlotPoseHook s_scene_slot_pose_hook;
void* s_scene_slot_pose_context;
TransientEffectConfigureHook s_transient_effect_configure_hook;
void* s_transient_effect_configure_context;
static uint32_t s_scene_availability_groups[12];
static uint32_t s_scene_availability_slots[12];
static SceneAvailabilityAdvanceHook s_availability_advance_hook;
static SceneAvailabilityEnableHook s_availability_enable_hook;
static void* s_availability_context;
int s_scene_availability_selected_style;

void rock_fall_style_names_install(const char* const* names, uint32_t count)
{
    uint32_t i;
    if (count > ROCK_FALL_STYLE_NAME_CAPACITY) {
        count = ROCK_FALL_STYLE_NAME_CAPACITY;
    }
    memset(s_rock_fall_style_names, 0, sizeof(s_rock_fall_style_names));
    s_rock_fall_style_name_count = 0u;
    if (names == NULL) {
        return;
    }
    for (i = 0u; i < count; ++i) {
        if (names[i] != NULL) {
            strncpy(s_rock_fall_style_names[i], names[i],
                    ROCK_FALL_STYLE_NAME_LENGTH - 1);
        }
    }
    s_rock_fall_style_name_count = count;
}

void scene_resource_lifecycle_hooks_install(
    SceneResourceAdvanceHook advance, SceneResourceReleaseHook release,
    SceneTimeoutEffectHook bird_scarer_effect, void* context)
{
    s_scene_advance_hook = advance;
    s_scene_release_hook = release;
    s_bird_scarer_effect_hook = bird_scarer_effect;
    s_scene_lifecycle_context = context;
}

void scene_light_pool_slot_install(int secondary, int index, void* handle)
{
    if (index < 0 || index >= 4) {
        return;
    }
    if (secondary != 0) {
        s_scene_light_secondary[index] = handle;
    } else {
        s_scene_light_primary[index] = handle;
    }
}

void scene_timeout_entry_install(int set_index, int entry_index, void* handle)
{
    if (set_index < 0 || set_index >= 10 || entry_index < 0
        || entry_index >= 10) {
        return;
    }
    s_terrain_marker_sets[set_index].entries[entry_index] = handle;
    if ((uint32_t)(entry_index + 1) > s_terrain_marker_sets[set_index].count) {
        s_terrain_marker_sets[set_index].count = (uint32_t)(entry_index + 1);
    }
}

void scene_timeout_set_count_install(int set_index, uint32_t count)
{
    if (set_index < 0 || set_index >= 10) {
        return;
    }
    if (count > 10u) {
        count = 10u;
    }
    s_terrain_marker_sets[set_index].count = count;
}

void scene_timeout_set_callback_set(SceneTimeoutSetCallback callback)
{
    s_scene_timeout_set_callback = callback;
}

void scene_timeout_set_10000(void)
{
    unsigned char output_a[4] = { 0, 0, 0, 0 };
    unsigned char output_b[4] = { 0, 0, 0, 0 };
    if (s_scene_timeout_set_callback != NULL) {
        s_scene_timeout_set_callback(10000.0f, output_a, output_b);
    }
}

void global_resource_handle_set(void* handle)
{
    s_global_resource_handle = handle;
}

void global_resource_release_callback_set(GlobalResourceReleaseFn callback,
                                          void* user)
{
    s_global_resource_release = callback;
    s_global_resource_user = user;
}

void global_resource_cleanup_callback_set(GlobalResourceCleanupFn callback,
                                          void* user)
{
    s_global_resource_cleanup = callback;
    s_global_resource_cleanup_user = user;
}

/* FUNCTION: LEGORR 0x0044e0a0 */
void global_resource_handle_release(void)
{
    if (s_global_resource_handle != NULL) {
        if (s_global_resource_release != NULL) {
            s_global_resource_release(s_global_resource_handle,
                                      s_global_resource_user);
        }
        if (s_global_resource_cleanup != NULL) {
            s_global_resource_cleanup(s_global_resource_cleanup_user);
        }
        s_global_resource_handle = NULL;
    }
}

/* FUNCTION: LEGORR 0x004629d0 */
void* panel_event_entry_append(void* panel, const char* name,
                               uint32_t payload, uint32_t value_a,
                               uint32_t value_b, uint32_t flags)
{
    if (panel == NULL || name == NULL || payload == 0u) return NULL;
    uint32_t count = 0;
    void* entries = NULL;
    __builtin_memcpy(&count, (char*)panel + 0x1cu, sizeof(count));
    __builtin_memcpy(&entries, (char*)panel + 0x20u, sizeof(entries));
    if (count >= 4096u) return NULL;
    void* resized = realloc(entries, (size_t)(count + 1u) * sizeof(void*));
    if (resized == NULL) return NULL;
    entries = resized;
    __builtin_memcpy((char*)panel + 0x20u, &entries, sizeof(entries));
    void* record = calloc(1u, 40u);
    if (record == NULL) return NULL;
    size_t length = strlen(name) + 1u;
    char* name_copy = (char*)malloc(length);
    if (name_copy == NULL) {
        free(record);
        return NULL;
    }
    memcpy(name_copy, name, length);
    uint32_t type = 3u;
    __builtin_memcpy((char*)record + 0u, &type, sizeof(type));
    __builtin_memcpy((char*)record + 4u, &value_a, sizeof(value_a));
    __builtin_memcpy((char*)record + 8u, &value_b, sizeof(value_b));
    __builtin_memcpy((char*)record + 0x14u, &name_copy, sizeof(name_copy));
    __builtin_memcpy((char*)record + 0x18u, &payload, sizeof(payload));
    __builtin_memcpy((char*)record + 0x1cu, &flags, sizeof(flags));
    ((void**)entries)[count] = record;
    ++count;
    __builtin_memcpy((char*)panel + 0x20u, &entries, sizeof(entries));
    __builtin_memcpy((char*)panel + 0x1cu, &count, sizeof(count));
    return record;
}

void panel_resource_loader_set(PanelResourceLoadFn callback, void* user)
{
    s_panel_resource_loader = callback;
    s_panel_resource_loader_user = user;
}

/* FUNCTION: LEGORR 0x00463810 */
void scroll_rect_payload_record_create(int slot, const float rect[4],
                                       int count, uint32_t value,
                                       const char* resource_name)
{
    if (slot < 0 || slot >= 10 || rect == NULL) return;
    struct ScrollRectRecord {
        float left, top, right, bottom;
        uint32_t field10, field14;
        void* resource;
        int field1c;
        uint32_t field20;
        uint32_t field24;
    };
    ScrollRectRecord* record = (ScrollRectRecord*)calloc(1u,
                                               sizeof(ScrollRectRecord));
    if (record == NULL) return;
    record->left = rect[0];
    record->top = rect[1];
    record->right = rect[2] + rect[0];
    record->bottom = rect[3] + rect[1];
    record->field1c = count - 1;
    record->field20 = value;
    if (s_panel_resource_loader != NULL && resource_name != NULL) {
        record->resource = s_panel_resource_loader(resource_name,
                                                   s_panel_resource_loader_user);
    }
    s_scroll_rect_payload_records[slot] = record;
}

void* scroll_rect_payload_record_get(int slot)
{
    return slot >= 0 && slot < 10 ? s_scroll_rect_payload_records[slot] : NULL;
}

void object_list_field_callbacks_set(ObjectListCountFn count,
                                     ObjectListSetFn set, void* user)
{
    s_object_list_count = count;
    s_object_list_set = set;
    s_object_list_user = user;
}

/* FUNCTION: LEGORR 0x00440bf0 */
void object_list_field_assign_all(void* record, void* value)
{
    if (record == NULL || s_object_list_count == NULL
        || s_object_list_set == NULL) return;
    uint32_t type = 0;
    void* list = NULL;
    __builtin_memcpy(&type, record, sizeof(type));
    if (type != 5u) return;
    __builtin_memcpy(&list, (char*)record + 0x1cu, sizeof(list));
    uint32_t count = s_object_list_count(list, s_object_list_user);
    if (count > 65536u) count = 65536u;
    for (uint32_t i = 0; i < count; ++i) {
        s_object_list_set(list, i, value, s_object_list_user);
    }
}

void resource_list_update_hooks_set(ResourceListCreateFn create,
                                    ResourceListUpdateFn update, void* user)
{
    s_resource_list_create = create;
    s_resource_list_update = update;
    s_resource_list_user = user;
}

/* FUNCTION: LEGORR 0x00451bc0 */
void resource_list_update_matching_key(void* list, void* library,
                                      void* update_value, uint32_t key)
{
    struct ResourceListNodeModel {
        void* object;
        void* handle;
        void* resource_key;
        uint32_t key;
        uint32_t padding;
        ResourceListNodeModel* next;
    };
    unsigned int count = 0;
    while (list != NULL && count++ < 4096u) {
        ResourceListNodeModel* node = (ResourceListNodeModel*)list;
        if (node->key == key) {
            if (node->handle == NULL && s_resource_list_create != NULL) {
                node->handle = s_resource_list_create(library,
                                                node->resource_key,
                                                s_resource_list_user);
            }
            if (node->handle != NULL && s_resource_list_update != NULL) {
                s_resource_list_update(node->handle, node->object, update_value,
                                       s_resource_list_user);
            }
        }
        list = node->next;
    }
}

void owned_object_release_hooks_set(OwnedObjectReleaseFn object_release,
                                    OwnedObjectReleaseFn scene_destroy,
                                    OwnedObjectReleaseFn node_free,
                                    void* user)
{
    s_owned_object_release = object_release;
    s_owned_scene_destroy = scene_destroy;
    s_owned_node_free = node_free;
    s_owned_release_user = user;
}

/* FUNCTION: LEGORR 0x00451c50. The reference chain is represented with
 * host-sized links; ownership bits keep the original two release gates. */
void owned_object_reference_chain_release(void* head)
{
    struct OwnedObjectNodeModel {
        void* primary;
        void* field1;
        void* secondary;
        void* field3;
        uint32_t flags;
        uint32_t padding;
        OwnedObjectNodeModel* next;
    };
    OwnedObjectNodeModel* node = (OwnedObjectNodeModel*)head;
    unsigned int count = 0;
    while (node != NULL && count++ < 4096u) {
        OwnedObjectNodeModel* next = node->next;
        if ((node->flags & 1u) == 0u) {
            if (node->secondary != NULL && s_owned_object_release != NULL) {
                s_owned_object_release(node->secondary, s_owned_release_user);
            }
            if (node->primary != NULL && s_owned_scene_destroy != NULL) {
                s_owned_scene_destroy(node->primary, s_owned_release_user);
            }
        }
        if ((node->flags & 2u) == 0u) {
            if (s_owned_node_free != NULL) {
                s_owned_node_free(node, s_owned_release_user);
            } else {
                engine_heap_free(node);
            }
        }
        node = next;
    }
}

int string_table_entry_install(const char* name)
{
    if (name == NULL) return -1;
    for (int i = 0; i < 50; ++i) {
        if (s_string_table_names[i] == NULL) {
            size_t length = strlen(name) + 1u;
            char* copy = (char*)malloc(length);
            if (copy == NULL) return -1;
            memcpy(copy, name, length);
            s_string_table_names[i] = copy;
            return i;
        }
    }
    return -1;
}

/* FUNCTION: LEGORR 0x0044dde0 */
void string_table_entry_set_current(const char* name)
{
    if (name == NULL) {
        s_string_table_enabled = 0;
        s_string_table_current = -1;
        return;
    }
    int found = -1;
    for (int i = 0; i < 50 && s_string_table_names[i] != NULL; ++i) {
        const unsigned char* a = (const unsigned char*)s_string_table_names[i];
        const unsigned char* b = (const unsigned char*)name;
        while (*a != 0 && *b != 0) {
            unsigned char ca = *a++;
            unsigned char cb = *b++;
            if (ca >= 'A' && ca <= 'Z') ca = (unsigned char)(ca + ('a' - 'A'));
            if (cb >= 'A' && cb <= 'Z') cb = (unsigned char)(cb + ('a' - 'A'));
            if (ca != cb) break;
        }
        if (*a == 0 && *b == 0) { found = i; break; }
    }
    if (found < 0) found = string_table_entry_install(name);
    s_string_table_current = found;
    s_string_table_enabled = 1;
}

const char* string_table_entry_current(void)
{
    return s_string_table_current >= 0 && s_string_table_current < 50
        ? s_string_table_names[s_string_table_current] : NULL;
}

/* FUNCTION: LEGORR 0x00451cd0 */
void message_string_table_initialize(void)
{
    static const char* names[65] = {
        "Message_Null", "Message_Select", "Message_Selected",
        "Message_ClearSelection", "Message_Deselect", "Message_Goto",
        "Message_RockMonsterGoto", "Message_RockMonsterGotoComplete",
        "Message_UserGoto", "Message_FirstPerson", "Message_TrackObject",
        "Message_TopView", "Message_PlaySample", "Message_Dig",
        "Message_DigComplete", "Message_Repair", "Message_RepairComplete",
        "Message_Reinforce", "Message_ReinforceComplete", "Message_RockFall",
        "Message_RockFallComplete", "Message_GenerateCrystal",
        "Message_GenerateCrystalComplete", "Message_CollectCrystal",
        "Message_CollectCrystalComplete", "Message_CrystalToRefinery",
        "Message_CrystalToRefineryComplete", "Message_GenerateOre",
        "Message_GenerateOreComplete", "Message_CollectOre",
        "Message_CollectOreComplete", "Message_GenerateRockMonster",
        "Message_GenerateRockMonsterComplete", "Message_GatherRock",
        "Message_GatherRockComplete", "Message_PickRandomFloor",
        "Message_PickRandomFloorComplete", "Message_AttackBuilding",
        "Message_AttackBuildingComplete", "Message_Clear",
        "Message_ClearComplete", "Message_GetIn",
        "Message_ManSelectedAndMonsterClicked", "Message_FollowAttack",
        "Message_CollectTool", "Message_ReduceSelection", "Message_ClearFallIn",
        "Message_ClearFallInComplete", "Message_BuildPath",
        "Message_BuildPathComplete", "Message_Train", "Message_TrainComplete",
        "Message_GenerateCrystalAndOre", "Message_GenerateCrystalAndOreComplete",
        "Message_GenerateFromCryOre", "Message_GenerateFromCryOreComplete",
        "Message_Upgrade", "Message_UpgradeComplete", "Message_ClearBuilding",
        "Message_ClearBuildingComplete", "Message_ClearInitial",
        "Message_ClearInitialComplete", "Message_ClearRemovePath",
        "Message_ClearRemovePathComplete", "Message_Debug_DestroyAll"
    };
    memcpy(s_message_strings, names, sizeof(s_message_strings));
}

const char* message_string_table_get(uint32_t index)
{
    return index < 65u ? s_message_strings[index] : NULL;
}

void file_loading_progress_state_install(int enabled, int total, int mode,
                                         const float rect[4],
                                         FileLoadingProgressDrawFn draw,
                                         void* user)
{
    s_loading_progress_enabled = enabled != 0;
    s_loading_progress_total = total;
    s_loading_progress_units = 0;
    s_loading_progress_mode = mode;
    s_loading_progress_last = -1.0f;
    s_loading_progress_draw = draw;
    s_loading_progress_user = user;
    if (rect != NULL) memcpy(s_loading_progress_rect, rect,
                             sizeof(s_loading_progress_rect));
}

/* FUNCTION: LEGORR 0x0044dec0 */
void file_loading_progress_update(uint32_t unused, float amount)
{
    (void)unused;
    if (!s_loading_progress_enabled) return;
    s_loading_progress_units += (int)amount;
    float progress = s_loading_progress_total > 0
        ? (float)s_loading_progress_units / (float)s_loading_progress_total
        : 0.0f;
    if (s_loading_progress_last + 0.002f >= progress) return;
    if (progress > 1.0f) progress = 1.0f;
    float rect[4];
    memcpy(rect, s_loading_progress_rect, sizeof(rect));
    switch (s_loading_progress_mode) {
    case 0:
        rect[3] = rect[3] + rect[2] - rect[2] * progress;
        break;
    case 1:
        rect[0] *= progress;
        break;
    case 2:
        rect[2] *= progress;
        break;
    case 3:
        rect[1] = rect[1] + rect[0] - rect[0] * progress;
        break;
    default:
        break;
    }
    if (s_loading_progress_draw != NULL) {
        s_loading_progress_draw(progress, rect, 1,
                                s_loading_progress_user);
        s_loading_progress_draw(progress, rect, 0,
                                s_loading_progress_user);
    }
    s_loading_progress_last = progress;
}

void indexed_surface_resource_hooks_set(IndexedSurfaceCreateFn create,
                                        IndexedTextureLoadFn load_texture,
                                        IndexedSurfaceReleaseFn release,
                                        IndexedSurfaceTextureSetFn set_texture,
                                        void* user)
{
    s_indexed_surface_create = create;
    s_indexed_texture_load = load_texture;
    s_indexed_surface_release = release;
    s_indexed_surface_texture_set = set_texture;
    s_indexed_surface_user = user;
}

/* FUNCTION: LEGORR 0x00462da0 */
int indexed_surface_resource_allocate(void* owner, int width, int height)
{
    if (width <= 0 || height <= 0 || width > 4096 || height > 4096)
        return 0;
    void* surface = NULL;
    IndexedSurfaceReleaseFn release = s_indexed_surface_release;
    void* release_user = s_indexed_surface_user;
    int native_frame = 0;
    if (s_indexed_surface_create != NULL)
        surface = s_indexed_surface_create(owner, 2, s_indexed_surface_user);
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
    else {
        surface = indexed_surface_native_create(owner);
        release = indexed_surface_native_release;
        release_user = NULL;
        native_frame = surface != NULL;
    }
#endif
    if (surface == NULL) return 0;
    size_t count = (size_t)width * (size_t)height;
    if (count > 16777216u) {
        if (release != NULL) {
            release(surface, release_user);
        }
        return 0;
    }
    uint32_t* entries = (uint32_t*)calloc(count * 2u, sizeof(uint32_t));
    if (entries == NULL) {
        if (release != NULL) {
            release(surface, release_user);
        }
        return 0;
    }
    for (size_t i = 0; i < count; ++i) entries[i * 2u + 1u] = 1u;
    free(s_indexed_surface_entries);
    if (s_indexed_surface != NULL && s_indexed_surface_owner_release != NULL) {
        if (s_indexed_surface_native_frame) graphics_overlay_word_set(0u);
        s_indexed_surface_owner_release(s_indexed_surface, s_indexed_surface_owner_user);
    }
    s_indexed_surface = surface;
    s_indexed_surface_owner_release = release;
    s_indexed_surface_owner_user = release_user;
    s_indexed_surface_native_frame = native_frame;
    if (native_frame) graphics_overlay_word_set((uint32_t)(uintptr_t)surface);
    s_indexed_surface_entries = entries;
    s_indexed_surface_width = width;
    s_indexed_surface_height = height;
    if (s_indexed_surface_texture != NULL && s_indexed_surface_release != NULL) {
        s_indexed_surface_release(s_indexed_surface_texture,
                                  s_indexed_surface_user);
    }
    s_indexed_surface_texture = NULL;
    return 1;
}

/* FUNCTION: LEGORR 0x00462e60 */
int animated_texture_replace(const char* name)
{
    if (s_indexed_surface_texture != NULL && s_indexed_surface_release != NULL) {
        s_indexed_surface_release(s_indexed_surface_texture,
                                  s_indexed_surface_user);
    }
    s_indexed_surface_texture = NULL;
    if (s_indexed_texture_load == NULL || name == NULL) return 0;
    void* texture = s_indexed_texture_load(name, s_indexed_surface_user);
    if (texture == NULL) return 0;
    s_indexed_surface_texture = texture;
    size_t count = (size_t)s_indexed_surface_width
        * (size_t)s_indexed_surface_height;
    if (s_indexed_surface_texture_set != NULL && s_indexed_surface != NULL) {
        for (size_t i = 0; i < count; ++i) {
            s_indexed_surface_texture_set(s_indexed_surface, (uint32_t)i,
                                          texture, s_indexed_surface_user);
        }
    }
    return 1;
}

/* FUNCTION: LEGORR 0x00462ed0 */
void graphics_overlay_resources_release(void)
{
    free(s_indexed_surface_entries);
    s_indexed_surface_entries = NULL;
    if (s_indexed_surface_native_frame) graphics_overlay_word_set(0u);
    if (s_indexed_surface != NULL && s_indexed_surface_owner_release != NULL) {
        s_indexed_surface_owner_release(s_indexed_surface, s_indexed_surface_owner_user);
    }
    s_indexed_surface = NULL;
    s_indexed_surface_owner_release = NULL;
    s_indexed_surface_owner_user = NULL;
    s_indexed_surface_native_frame = 0;
    if (s_indexed_surface_texture != NULL && s_indexed_surface_release != NULL) {
        s_indexed_surface_release(s_indexed_surface_texture,
                                  s_indexed_surface_user);
    }
    s_indexed_surface_texture = NULL;
    s_indexed_surface_width = 0;
    s_indexed_surface_height = 0;
}

void* indexed_surface_scene_owner_get(void)
{
    return s_indexed_surface_native_frame ? s_indexed_surface : NULL;
}

uint8_t* indexed_surface_geometry_records_get(void)
{
    return (uint8_t*)s_indexed_surface_entries;
}

int indexed_surface_resources_can_shutdown(void)
{
    return s_indexed_surface == NULL;
}

void visual_model_resolver_set(VisualModelResolveFn resolver, void* user)
{
    s_visual_model_resolver = resolver;
    s_visual_model_resolver_user = user;
}

/* FUNCTION: LEGORR 0x004519b0 */
void* visual_model_record_create(void* parent, const char* name,
                                 const char* path_a, const char* path_b,
                                 int flags)
{
    if (name == NULL || path_a == NULL || path_b == NULL
        || s_visual_model_resolver == NULL) return NULL;
    char path[512];
    size_t a = strlen(path_a);
    size_t b = strlen(path_b);
    if (a + b + 2u > sizeof(path)) return NULL;
    memcpy(path, path_a, a);
    path[a] = '\\';
    memcpy(path + a + 1u, path_b, b + 1u);
    void* resource = s_visual_model_resolver(path, 1,
                                             s_visual_model_resolver_user);
    if (resource == NULL) {
        resource = s_visual_model_resolver(path, 2,
                                           s_visual_model_resolver_user);
    }
    if (resource == NULL) return NULL;
    struct VisualModelRecordModel {
        void* resource;
        uint32_t flags;
        char* name;
        int extra;
        uint32_t kind;
        void* next;
    };
    VisualModelRecordModel* record = (VisualModelRecordModel*)calloc(
        1u, sizeof(VisualModelRecordModel));
    if (record == NULL) return NULL;
    size_t name_length = strlen(name) + 1u;
    record->name = (char*)malloc(name_length);
    if (record->name == NULL) { free(record); return NULL; }
    memcpy(record->name, name, name_length);
    record->resource = resource;
    record->flags = 0u;
    record->extra = flags;
    record->kind = 2u;
    if (parent != NULL) {
        __builtin_memcpy((char*)parent + 0x14u, &record, sizeof(record));
    }
    return record;
}

void resource_slot_model_install(int index, void* handle, uint32_t count)
{
    SceneResourceSlotModel* slot = resource_slot_address_by_index(index);
    if (slot == NULL) {
        return;
    }
    slot->handle = handle;
    slot->count = count;
}

void numbered_bitmap_load_hook_install(NumberedBitmapLoadHook hook,
                                        void* context)
{
    s_numbered_bitmap_load_hook = hook;
    s_numbered_bitmap_context = context;
}

void* numbered_bitmap_slot_peek(int index)
{
    return (index >= 0 && index < 11) ? s_numbered_bitmap_slots[index] : NULL;
}

void scene_animation_duration_hooks_install(
    SceneAnimationDurationHook duration,
    SceneAnimationLastFrameHook last_frame, void* context)
{
    s_animation_duration_hook = duration;
    s_animation_last_frame_hook = last_frame;
    s_animation_duration_context = context;
}

void scene_resource_record_hooks_install(
    SceneResourceNamedResolveHook resolve,
    SceneObjectResourceReleaseHook release, void* context)
{
    s_scene_resource_named_resolve_hook = resolve;
    s_scene_object_resource_release_hook = release;
    s_scene_resource_record_context = context;
}

void scene_animation_track_hook_install(SceneAnimationTrackApplyHook hook,
                                        void* context)
{
    s_animation_track_apply_hook = hook;
    s_animation_track_context = context;
}

void scene_resource_owner_hook_install(SceneResourceOwnerApplyHook apply,
                                       SceneAttachedResourceReleaseHook release,
                                       void* context)
{
    s_scene_resource_owner_apply_hook = apply;
    s_scene_attached_resource_release_hook = release;
    s_scene_resource_owner_context = context;
}

void scene_payload_memory_release_hook_install(
    SceneOwnedMemoryReleaseHook release, void* context)
{
    s_scene_owned_memory_release_hook = release;
    s_scene_owned_memory_context = context;
}

void scene_animation_channel_hooks_install(
    SceneAnimationChannelUpdateHook update,
    SceneAnimationChannelQueryHook query,
    SceneAnimationVisibilityHook visibility, void* context)
{
    s_animation_channel_update_hook = update;
    s_animation_channel_query_hook = query;
    s_animation_visibility_hook = visibility;
    s_animation_channel_context = context;
}

void scene_indexed_record_release_hook_install(
    SceneIndexedRecordNodeReleaseHook release, void* context)
{
    s_scene_indexed_record_release_hook = release;
    s_scene_indexed_record_context = context;
}

void scene_channel_entry_process_hook_install(
    SceneChannelEntryProcessHook hook, void* context)
{
    s_scene_channel_entry_process_hook = hook;
    s_scene_channel_entry_context = context;
}

void scene_effect_pose_hook_install(SceneEffectPoseHook hook, void* context)
{
    s_scene_effect_pose_hook = hook;
    s_scene_effect_pose_context = context;
}

void scene_marker_templates_install(void* light_default, void* light_variant,
                                    void* marker, int scene_variant)
{
    s_scene_marker_templates[0] = light_default;
    s_scene_marker_templates[1] = light_variant;
    s_scene_marker_templates[2] = marker;
    s_scene_marker_scene_variant = scene_variant;
}

void scene_object_position_hook_install(SceneObjectPositionHook hook,
                                        void* context)
{
    s_scene_object_position_hook = hook;
    s_scene_object_position_context = context;
}

void terrain_point_template_install(void* handle)
{
    s_terrain_point_template = handle;
}

void terrain_height_hook_install(TerrainHeightAtPointHook hook,
                                void* context)
{
    s_terrain_height_hook = hook;
    s_terrain_height_context = context;
}

Resource* global_resource_handle_peek(void)
{
    return (Resource*)s_global_resource_handle;
}

void scene_availability_hooks_install(SceneAvailabilityAdvanceHook advance,
                                      SceneAvailabilityEnableHook enable,
                                      void* context)
{
    s_availability_advance_hook = advance;
    s_availability_enable_hook = enable;
    s_availability_context = context;
}

void scene_availability_style_select(int style)
{
    if (style >= 0 && style < SCENE_AVAILABILITY_STYLE_CAPACITY) {
        s_scene_availability_selected_style = style;
    }
}

void scene_availability_style_slot_install(int style, int group, int slot,
                                           void* handle, int configured)
{
    if (style < 0 || style >= SCENE_AVAILABILITY_STYLE_CAPACITY
        || group < 0 || group >= 3
        || slot < 0 || slot >= 4) {
        return;
    }
    s_scene_availability_styles[style][group][slot].handle = handle;
    s_scene_availability_styles[style][group][slot].configured = configured;
    s_scene_availability_styles[style][group][slot].enabled = 0;
}

void scene_availability_transient_slot_install(int group, int slot,
                                               void* handle, int configured)
{
    if (group < 0 || group >= 2 || slot < 0 || slot >= 30) {
        return;
    }
    s_scene_availability_transients[group][slot].handle = handle;
    s_scene_availability_transients[group][slot].configured = configured;
    s_scene_availability_transients[group][slot].enabled = 0;
    s_scene_availability_transients[group][slot].pending = 0;
}

void scene_cell_table_pair_install(int style, int group, int slot,
                                   uint32_t first, uint32_t second)
{
    if (style < 0 || style >= SCENE_AVAILABILITY_STYLE_CAPACITY
        || group < 0 || group >= 3 || slot < 0 || slot >= 4) {
        return;
    }
    s_scene_cell_table_pairs[style][group][slot][0] = first;
    s_scene_cell_table_pairs[style][group][slot][1] = second;
}

void scene_resource_name_hooks_install(SceneResourceNameFormatHook format,
                                       SceneResourceNameApplyHook apply,
                                       void* context)
{
    s_resource_name_format_hook = format;
    s_resource_name_apply_hook = apply;
    s_resource_name_context = context;
}

void scene_resource_name_slot_install(int style, int group, int slot,
                                      uint32_t handle_key,
                                      uint32_t secondary_key, void* handle,
                                      uint32_t name_base, uint32_t name_count)
{
    SceneResourceNameSlot* entry;
    if (style < 0 || style >= SCENE_AVAILABILITY_STYLE_CAPACITY
        || group < 0 || group >= 3 || slot < 0 || slot >= 4) {
        return;
    }
    entry = &s_scene_resource_name_slots[style][group][slot];
    entry->handle_key = handle_key;
    entry->secondary_key = secondary_key;
    entry->handle = handle;
    entry->name_base = name_base;
    entry->name_count = name_count;
}

void scene_resource_instance_hooks_install(SceneResourceParentLoadHook load,
                                           SceneResourceChildCreateHook clone,
                                           SceneResourceLockHook lock,
                                           void* context)
{
    s_resource_parent_load_hook = load;
    s_resource_child_create_hook = clone;
    s_resource_lock_hook = lock;
    s_resource_instance_context = context;
}

void scene_act_hooks_install(SceneActLoadHook load,
                             SceneActNameRegisterHook register_name,
                             void* context)
{
    s_scene_act_load_hook = load;
    s_scene_act_name_register_hook = register_name;
    s_scene_act_context = context;
}

void scene_slot_pose_hook_install(SceneSlotPoseHook hook, void* context)
{
    s_scene_slot_pose_hook = hook;
    s_scene_slot_pose_context = context;
}

void terrain_point_object_install(int slot, void* handle)
{
    if (slot < 0 || slot >= 4) {
        return;
    }
    if (slot >= (int)s_terrain_point_object_count) {
        s_terrain_point_object_count = (uint32_t)(slot + 1);
    }
    s_terrain_point_objects[slot] = handle;
}

void transient_effect_configure_hook_install(
    TransientEffectConfigureHook hook, void* context)
{
    s_transient_effect_configure_hook = hook;
    s_transient_effect_configure_context = context;
}

static int scene_availability_advance(SceneAvailabilitySlot* slot, float delta)
{
    if ((slot->configured != 0 && slot->pending == 0)
        || slot->enabled != 0 || slot->handle == NULL
        || s_availability_advance_hook == NULL
        || s_availability_advance_hook(slot->handle, delta,
                                       s_availability_context) <= 0) {
        return 0;
    }
    if (s_availability_enable_hook != NULL) {
        s_availability_enable_hook(slot->handle, s_availability_context);
    }
    slot->configured = 1;
    slot->pending = 0;
    slot->enabled = 1;
    return 1;
}

/* FUNCTION: LEGORR 0x0040C3A0 */
int scene_resource_availability_sweep(float delta, uint32_t** groups_out,
                                      uint32_t** slots_out)
{
    int count = 0;
    int group;
    int slot;
    terrain_point_objects_remove_matching(delta);
    scene_light_pool_primary_expire(delta);
    scene_light_pool_secondary_expire(delta);
    terrain_marker_sets_clear(delta);
    for (group = 0; group < 3; ++group) {
        for (slot = 0; slot < 4; ++slot) {
            SceneAvailabilitySlot* entry =
                &s_scene_availability_styles[s_scene_availability_selected_style]
                                             [group][slot];
            if (scene_availability_advance(entry, delta) != 0 && count < 12) {
                s_scene_availability_groups[count] = (uint32_t)group;
                s_scene_availability_slots[count] = (uint32_t)slot;
                ++count;
            }
        }
    }
    for (group = 0; group < 2; ++group) {
        for (slot = 0; slot < 30; ++slot) {
            scene_availability_advance(&s_scene_availability_transients[group][slot],
                                       delta);
        }
    }
    if (groups_out != NULL) {
        *groups_out = s_scene_availability_groups;
    }
    if (slots_out != NULL) {
        *slots_out = s_scene_availability_slots;
    }
    return count;
}

// FUNCTION: LEGORR 0x00476d10
void render_resource_pair_release_and_clear(void** pair)
{
    void* property;
    if (pair == NULL) {
        return;
    }
    property = pair[1] != NULL
        ? ((ResourcePropertyInterface*)pair[1])->property_get() : NULL;
    engine_heap_free(property);
    engine_heap_free(pair[0]);
    pair[0] = NULL;
    pair[1] = NULL;
}

// FUNCTION: LEGORR 0x004762f0
void scene_property_owned_buffer_release(void* property)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    void* owned_buffer = (void*)scene_property_value_at08_get(property);
    if (owned_buffer != NULL) {
        engine_heap_free(owned_buffer);
    }
#elif defined(__i386__)
    void* owned_buffer = (void*)scene_property_value_at08_get(property);
    if (owned_buffer != NULL) {
        engine_heap_free(owned_buffer);
    }
#else
    void* record;
    void* owned_buffer;
    if (property == NULL) {
        return;
    }
    record = ((ResourcePropertyInterface*)property)->property_get();
    owned_buffer = record != NULL ? *(void**)((char*)record + 0x08) : NULL;
    engine_heap_free(owned_buffer);
#endif
}

#define WAD_MAGIC         "WWAD"
#define ENTRY_FLAG_EXPANDED 0x2u
#define EXPAND_HEADER_SZ  0x12u

static void wad_huffman_canonical_codes_build(void* table, uint8_t count);

/* The corpus's 16-byte Huffman decode entry. The roles come from the corpus
 * bytes, not from the port's earlier guess: WadHuffmanTableRead (0x0049cd90)
 * stores the read_bits(4) verdict as the WORD at +12 (`lea esi,[ebp+0xc]` /
 * `mov [esi],ax`, `add esi,0x10`), WadHuffmanSymbolDecode (0x0049ce00) tests
 * the WORD at +12 and compares `(1<<len)-1 & lsb` against the DWORD at +8
 * (`mov cx,[eax+0xc]` / `mov ecx,[eax+0x8]` / `add eax,0x10`), and the
 * canonical builder (0x0049cf90) matches the WORD at +12 and stores the
 * reversed code into the DWORD at +8. WadHuffmanTableClear (0x0049cf50)
 * zeroes the DWORD at +0, writes 0xffff into the WORD at +4, zeroes the
 * DWORD at +8 and the WORD at +12; the +0/+4 pair is untouched by this band,
 * so those two fields keep their corpus offsets and an inert role. */
typedef struct WadCodecTableEntry {
    uint32_t reserved; /* +0  DWORD: cleared to 0, unread by the decode band */
    uint16_t max;      /* +4  WORD: cleared to 0xffff, unread by that band */
    uint16_t pad;      /* +6  padding */
    uint32_t code;     /* +8  DWORD: the canonical code for this length */
    uint16_t len;      /* +12 WORD: the code's bit length */
    uint16_t pad2;     /* +14 padding */
} WadCodecTableEntry;

// FUNCTION: LEGORR 0x0049d000
static unsigned int wad_reverse_low_bits(unsigned int v, uint8_t n)
{
    unsigned int out = 0;
    while (n--) {
        out = out << 1;
        if ((v & 1u) != 0u) out = out | 1u;
        v >>= 1;
    }
    return out;
}

// FUNCTION: LEGORR 0x0049cf90
static void wad_huffman_canonical_codes_build(void* table, uint8_t count)
{
    WadCodecTableEntry* e;
    unsigned int next, unit, len;
    unsigned int i;
    e = (WadCodecTableEntry*)table;
    next = 0;
    unit = 0x80000000u;
    for (len=1; len<=16; ++len, unit >>= 1) {
        for (i=0; i<count; ++i) if (e[i].len == len) {
            e[i].code = wad_reverse_low_bits(next / unit, len);
            next += unit;
        }
    }
}

/* WAD codec state mirrors the executable's shared, non-reentrant decoder. */
// GLOBAL: LEGORR 0x00558080
static const unsigned char* wad_codec_in;
// GLOBAL: LEGORR 0x00558084
static unsigned char* wad_codec_out;
// GLOBAL: LEGORR 0x00558090
static unsigned char* wad_codec_end;
// GLOBAL: LEGORR 0x00557d68
static unsigned char wad_codec_bits;
// GLOBAL: LEGORR 0x0055808c
static unsigned int wad_codec_lsb;
// GLOBAL: LEGORR 0x00558088
static unsigned char wad_codec_msb;

// FUNCTION: LEGORR 0x0049cc70
static void wad_bitstream_output_initialize(const void* src, void* dst)
{
    const unsigned char* p;
    uint32_t n, raw;
    p = (const unsigned char*)src;
    wad_codec_in = p ? p + 0x12 : NULL;
    wad_codec_out = (unsigned char*)dst;
    raw = p ? *(const uint32_t*)(p + 4) : 0;
    n = ((raw << 24) & 0xff000000u) | ((raw << 8) & 0x00ff0000u)
      | ((raw >> 8) & 0x0000ff00u) | (raw >> 24);
    wad_codec_end = dst ? (unsigned char*)dst + n : NULL;
    wad_codec_bits = wad_codec_lsb = wad_codec_msb = 0;
}
// FUNCTION: LEGORR 0x0049ccc0
static unsigned int wad_read_bits_lsb_first(unsigned char n)
{
    unsigned int v, mask;
    v = 0;
    mask = 1;
    while(n--) {
        if(!wad_codec_bits) {
            wad_codec_lsb=wad_codec_in[0]|((unsigned int)wad_codec_in[1]<<8);
            wad_codec_in+=2;
            wad_codec_bits=16;
        }
        if(wad_codec_lsb&1) v|=mask;
        mask<<=1;
        wad_codec_lsb>>=1;
        --wad_codec_bits;
    }
    return v;
}
// FUNCTION: LEGORR 0x0049cd30
static unsigned int wad_read_bits_msb_first(unsigned char n)
{
    unsigned int v;
    v=0;
    while(n--) {
        if(!wad_codec_bits) { wad_codec_msb=*wad_codec_in++; wad_codec_bits=8; }
        v=(v<<1)|((wad_codec_msb>>7)&1);
        wad_codec_msb=(wad_codec_msb<<1)&255;
        --wad_codec_bits;
    }
    return v;
}
// FUNCTION: LEGORR 0x0049ce70
static short wad_run_length_decode(void)
{
    short a;
    short b;

    a = (short)wad_read_bits_msb_first(1);
    a += 4; /* 0x49ce7b/0x49ce7e: mov si,ax; add esi,4 */
    b = (short)wad_read_bits_msb_first(1);
    if (!b) {
        return a; /* 0x49ce90: mov ax,si */
    }
    return (short)(wad_read_bits_msb_first(1) - 2 + a * 2);
}
// FUNCTION: LEGORR 0x0049ceb0
static int wad_backreference_offset_decode(void)
{
    unsigned int v;

    v = 0;
    if ((unsigned short)wad_read_bits_msb_first(1) != 0) {
        v = wad_read_bits_msb_first(1);
        if ((unsigned short)wad_read_bits_msb_first(1) != 0) {
            v = wad_read_bits_msb_first(1) + v * 2;
            v |= 4u; /* 0x49cee4/0x49ceea/0x49ceec: double, add, OR low byte */
            if ((unsigned short)wad_read_bits_msb_first(1) == 0) {
                v = wad_read_bits_msb_first(1) + v * 2;
            }
        }
        else if ((unsigned short)v == 0) {
            v = (unsigned short)wad_read_bits_msb_first(1);
            v += 2; /* 0x49cf1e/0x49cf24: mov si,ax; add esi,2 */
        }
    }
    /* 0x49cf27-0x49cf35: scale by 256, add the next byte, advance input. */
    return v * 256u + 1u + *wad_codec_in++;
}

// FUNCTION: LEGORR 0x0049cd90
static void wad_huffman_table_read(void* table, unsigned int count)
{
    unsigned int n, i;
    WadCodecTableEntry* e;
    wad_huffman_table_clear(table, (int)count);
    n=wad_read_bits_lsb_first(5);
    /* 0x49cdb1 skips the rest after a zero count; every positive count
     * reaches the canonical builder with the capped count. */
    if(n == 0) return;
    if(n>16)n=16;
    e=(WadCodecTableEntry*)table;
    for(i=0;i<n;i++) e[i].len=(uint16_t)wad_read_bits_lsb_first(4);
    wad_huffman_canonical_codes_build(table,(int)n);
}
// FUNCTION: LEGORR 0x0049ce00
static unsigned int wad_huffman_symbol_decode(void* table)
{
    WadCodecTableEntry* e;
    unsigned int i, v;
    e=(WadCodecTableEntry*)table;
    i=0;
    while(e[i].len==0 || (((1u<< (e[i].len&31))-1u & wad_codec_lsb)!=e[i].code)) ++i;
    v=wad_read_bits_lsb_first(e[i].len);
    return i<2 ? i : (wad_read_bits_lsb_first(i-1)|(1u<<(i-1)));
}

struct ResourceStream {
    FILE* file;
    uint32_t data_offset;
    uint32_t size;
    uint8_t* expanded_data;
};

ResourceWad s_wads[RESOURCE_MAX_WADS];
int       s_wad_count = 0;
static char      s_wad_prefix[RESOURCE_PATH_MAX] = "LegoRR";
static ResourceFileTokenList s_resource_file_tokens;
static char s_resource_directory_root[RESOURCE_PATH_MAX];
// GLOBAL: LEGORR 0x005339a0
static char      s_wad_mount_path_cache[0x400];
// GLOBAL: LEGORR 0x005343b8
static int       s_wad_mount_path_cache_valid;
// GLOBAL: LEGORR 0x00533da0
static char      s_wad_mount_path_rewrite[0x400];

/* CD/STD fallback state (finding 01). */
#ifdef LEGORRR_BACKEND_WINAPI
#  define RES_PATH_SEP '\\' /* the corpus's "%c:\%s\%s" uses DOS separators */
#else
#  define RES_PATH_SEP '/'  /* host-native on the POSIX port */
#endif
static char  s_cd_drive    = 'A'; /* DAT_005343b4 analog; default mirrors .rdata */
static char  s_cd_data_dir[RESOURCE_PATH_MAX] = "Data";
static char  s_cd_root[RESOURCE_PATH_MAX] = ""; /* "" = "%c:\" default */
static ResourceDriveTypeQueryFn s_cd_drive_type_query;
static ResourcePathExistsFn s_cd_path_exists;
static void* s_cd_probe_context;
static ResourceStartupOps s_resource_startup_ops;
static int s_resource_startup_result = RESOURCE_STARTUP_OK;
static int s_resource_startup_require_cd;

void resource_startup_require_cd_set(int require_cd)
{
    s_resource_startup_require_cd = require_cd != 0;
}

int resource_startup_require_cd_get(void)
{
    return s_resource_startup_require_cd;
}

void resource_directory_scan_root_set(const char* root)
{
    size_t length;
    if (root == NULL) {
        s_resource_directory_root[0] = '\0';
        return;
    }
    length = strlen(root);
    if (length >= sizeof(s_resource_directory_root)) {
        s_resource_directory_root[0] = '\0';
        return;
    }
    memcpy(s_resource_directory_root, root, length + 1u);
}

const char* resource_directory_scan_root_get(void)
{
    return s_resource_directory_root;
}

static const char* ares_cd_path(const char* name, char* out, size_t out_sz)
{
    size_t off;
    if (s_cd_root[0] != '\0') {
        off = (size_t)snprintf(out, out_sz, "%s%c%s%c", s_cd_root,
                               RES_PATH_SEP, s_cd_data_dir, RES_PATH_SEP);
    } else {
        off = (size_t)snprintf(out, out_sz, "%c:%c%s%c", s_cd_drive,
                               RES_PATH_SEP, s_cd_data_dir, RES_PATH_SEP);
    }
    if (off >= out_sz) {
        return out;
    }
    size_t copy_capacity = out_sz - off - 1;
    strncpy(out + off, name, copy_capacity);
    out[off + copy_capacity] = '\0';
#if !defined(LEGORRR_BACKEND_WINAPI)
    char* separator = strchr(out + off, '\\');
    while (separator != NULL) {
        *separator++ = RES_PATH_SEP;
        separator = strchr(separator, '\\');
    }
#endif
    return out;
}

/* PathResolveAgainstBase (FUN_00480220): form a path under the configured
 * data root and reject parent-directory components that escape that root. */
// FUNCTION: LEGORR 0x00480220
const char* resource_path_resolve_against_base(const char* path)
{
    static char resolved[RESOURCE_PATH_MAX];
    char base[RESOURCE_PATH_MAX];
    const char* part;
    size_t length;
    if (path == NULL) return NULL;
    if (*path == '\\') ++path;
    length = strlen(path);
    if (length >= sizeof(resolved)) return NULL;
    if (*path == '/' || strchr(path, ':') != NULL) return NULL;
    part = path;
    while (*part != '\0') {
        const char* end = part;
        size_t segment_length;
        while (*end != '\0' && *end != '/' && *end != '\\') ++end;
        segment_length = (size_t)(end - part);
        if (segment_length == 2 && part[0] == '.' && part[1] == '.') {
            return NULL;
        }
        part = (*end == '\0') ? end : end + 1;
    }
    ares_cd_path("", base, sizeof(base));
    if (strlen(base) + length >= sizeof(resolved)) return NULL;
    snprintf(resolved, sizeof(resolved), "%s%s", base, path);
    return resolved;
}

/* SoundPathFallbackFormat (FUN_0047f660): only format the fallback when a
 * drive is configured. The port permits a mounted-root override and applies
 * host separators/bounds checks inside ares_cd_path. */
// FUNCTION: LEGORR 0x0047f660
static int ares_cd_fallback_path(const char* name, char* out, size_t out_sz)
{
    if (s_cd_drive == 0) {
        return 0;
    }
    ares_cd_path(name, out, out_sz);
    return 1;
}

void resource_set_cd_drive(char drive)
{
    s_cd_drive = drive;
}

void resource_set_cd_data_dir(const char* dir)
{
    if (dir != NULL && dir[0] != '\0') {
        snprintf(s_cd_data_dir, sizeof(s_cd_data_dir), "%s", dir);
    }
}

void resource_set_cd_root(const char* root)
{
    if (root == NULL) {
        s_cd_root[0] = '\0';
    } else {
        snprintf(s_cd_root, sizeof(s_cd_root), "%s", root);
    }
}

static const char* ares_cd_root_path(const char* name, char* out, size_t out_sz)
{
    if (s_cd_root[0] != '\0') {
        snprintf(out, out_sz, "%s%c%s", s_cd_root, RES_PATH_SEP, name);
    } else {
        snprintf(out, out_sz, "%c:%c%s", s_cd_drive, RES_PATH_SEP, name);
    }
    return out;
}

void resource_cd_probe_hooks_set(ResourceDriveTypeQueryFn drive_type,
                                 ResourcePathExistsFn path_exists,
                                 void* context)
{
    s_cd_drive_type_query = drive_type;
    s_cd_path_exists = path_exists;
    s_cd_probe_context = context;
}

static int resource_cd_key_path_exists(const char* path)
{
    FILE* fp;
    if (s_cd_path_exists != NULL) {
        return s_cd_path_exists(path, s_cd_probe_context);
    }
    fp = fopen(path, "rb");
    if (fp == NULL) return 0;
    fclose(fp);
    return 1;
}

/* FindCdKeyDrive (FUN_0047f4f0): scan optical drive letters C through Z,
 * retaining the first drive with a readable cd.key marker. Platform hooks
 * supply drive type and file probes; a mounted root uses its file probe. */
// FUNCTION: LEGORR 0x0047f4f0
int resource_find_cd_key_drive(void)
{
    char path[RESOURCE_PATH_MAX];
    char drive;
    if (s_cd_root[0] != '\0') {
        ares_cd_root_path("cd.key", path, sizeof(path));
        return resource_cd_key_path_exists(path);
    }
    for (drive = 'C'; drive <= 'Z'; ++drive) {
        int drive_type;
        if (s_cd_drive_type_query != NULL) {
            drive_type = s_cd_drive_type_query(drive, s_cd_probe_context);
        }
#if defined(_WIN32)
        else {
            char drive_root[4];
            drive_root[0] = drive;
            drive_root[1] = ':';
            drive_root[2] = '\\';
            drive_root[3] = '\0';
            drive_type = (int)GetDriveTypeA(drive_root);
        }
#else
        else {
            drive_type = 0;
        }
#endif
        if (drive_type != 5) {
            continue;
        }
        snprintf(path, sizeof(path), "%c:%c%s%ccd.key", drive,
                 RES_PATH_SEP, s_cd_data_dir, RES_PATH_SEP);
        if (resource_cd_key_path_exists(path)) {
            s_cd_drive = drive;
            return 1;
        }
    }
    return 0;
}

int resource_cd_present(void)
{
    char path[RESOURCE_PATH_MAX];
    ares_cd_root_path("cd.key", path, sizeof(path));
    FILE* fp = fopen(path, "rb");
    if (fp == NULL) {
        return 0;
    }
    fclose(fp);
    return 1;
}

int resource_data_present(void)
{
    char path[RESOURCE_PATH_MAX];
    ares_cd_root_path("delme.dat", path, sizeof(path));
    FILE* fp = fopen(path, "rb");
    if (fp == NULL) {
        return 0;
    }
    fclose(fp);
    return 1;
}

static uint32_t ares_le32(const uint8_t* p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

char* ares_strdup(const char* s)
{
#if defined(_MSC_VER)
    /* The archive-owned names are released with free(), so use the matching
     * VC5 CRT allocator and copy routine on the reference build. */
    return _strdup(s);
#else
    size_t n = strlen(s);
    char*  d = (char*)malloc(n + 1);
    if (d == NULL) {
        return NULL;
    }
    memcpy(d, s, n);
    d[n] = '\0';
    return d;
#endif
}

/* ------------------------------------------------------------------ *
 *  WWAD header + TOC parse (finding 01):
 *    +0x00 "WWAD"
 *    +0x04 u32 entry_count
 *    names    : entry_count x NUL-terminated archive-relative paths
 *    unc      : entry_count x NUL-terminated dev-tree paths (unused)
 *    toc      : entry_count x 16-byte { flags, orig, stored, off }
 *    then the data area at the recorded (absolute) offsets.
 * ------------------------------------------------------------------ */

static uint32_t ares_count_names(FILE* fp, uint32_t n)
{
    /* Returns the byte size of the two NUL-terminated name blocks
     * (archive-relative paths + UNC paths) read sequentially from the
     * current position. Leaves the stream positioned at the TOC. */
    uint32_t total = 0;
    for (int pass = 0; pass < 2; ++pass) {
        for (uint32_t i = 0; i < n; ++i) {
            uint32_t len = 0;
            int c;
            do {
                c = fgetc(fp);
                if (c == EOF) {
                    ferror(fp); /* clear error */
                    return total; /* truncated file */
                }
                ++len;
            } while (c != 0);
            total += len;
        }
    }
    return total;
}

int ares_parse(FILE* fp, ResourceWad* out)
{
    uint8_t hdr[8];
    if (fseek(fp, 0, SEEK_END) != 0) {
        return 0;
    }
    long size = ftell(fp);
    if (size < 0 || (unsigned long)size > 0xfffffffful) {
        return 0;
    }
    out->file_size = (uint32_t)size;

    if (fseek(fp, 0, SEEK_SET) != 0 || fread(hdr, 1, 8, fp) != 8) {
        return 0;
    }
    if (memcmp(hdr, WAD_MAGIC, 4) != 0) {
        return 0;
    }
    uint32_t n = ares_le32(hdr + 4);
    if (n == 0 || n > 0x100000) {
        return 0; /* entry-count sanity bound */
    }

    /* Count the name blocks (archive-relative + UNC) so we can slurp the
     * whole region in one allocation. */
    uint32_t names_bytes = ares_count_names(fp, n);

    /* Now slurp the name blob and record per-entry name offsets. */
    out->entry_count = n;
    out->entries = (WadEntry*)calloc(n, sizeof(WadEntry));
    out->names = (char*)malloc(names_bytes > 0 ? names_bytes : 1);
    if (out->entries == NULL || out->names == NULL) {
        free(out->entries);
        free(out->names);
        out->entries = NULL;
        out->names = NULL;
        return 0;
    }
    if (names_bytes > 0) {
        if (fseek(fp, 8, SEEK_SET) != 0
            || fread(out->names, 1, names_bytes, fp) != names_bytes) {
            return 0;
        }
    }
    uint32_t blob_pos = 0;
    for (int pass = 0; pass < 2; ++pass) {
        for (uint32_t i = 0; i < n; ++i) {
            if (pass == 0) {
                out->entries[i].name_off = blob_pos;
            }
            if (blob_pos >= names_bytes) {
                return 0;
            }
            const char* terminator = (const char*)memchr(
                out->names + blob_pos, '\0', names_bytes - blob_pos);
            if (terminator == NULL) {
                return 0;
            }
            blob_pos = (uint32_t)(terminator - out->names) + 1;
        }
    }
    /* names[] positions now point into the blob at the archive-relative
     * name for each entry (pass 0 recorded them). */

    /* TOC: 16 bytes per entry. */
    uint32_t toc_off = 8 + names_bytes;
    for (uint32_t i = 0; i < n; ++i) {
        uint8_t row[16];
        if (fseek(fp, toc_off + (long)i * 16, SEEK_SET) != 0
            || fread(row, 1, 16, fp) != 16) {
            return 0;
        }
        out->entries[i].flags  = ares_le32(row + 0);
        out->entries[i].orig   = ares_le32(row + 4);
        out->entries[i].stored = ares_le32(row + 8);
        out->entries[i].off    = ares_le32(row + 12);
    }
    return 1;
}

/* WadMountPathCacheStore (FUN_0047f590): keep a nonempty path shorter than
 * 0x401 bytes; invalid input clears the cache and its validity flag. */
// FUNCTION: LEGORR 0x0047f590
void wad_mount_path_cache_store(const char* path)
{
    size_t length;
    if (path != NULL) {
        length = strlen(path);
        if (length != 0 && length < sizeof(s_wad_mount_path_cache)) {
            strncpy(s_wad_mount_path_cache, path, length + 1);
            s_wad_mount_path_cache_valid = 1;
            return;
        }
    }
    s_wad_mount_path_cache[0] = '\0';
    s_wad_mount_path_cache_valid = 0;
}

/* MountWadFromPath (FUN_0047f640): reset mounted archive state and load the
 * requested archive path. */
// FUNCTION: LEGORR 0x0047f640
void mount_wad_from_path(const char* path)
{
    resource_unmount_all();
    if (path != NULL) {
        char root[RESOURCE_PATH_MAX];
        char* slash;
        snprintf(root, sizeof(root), "%s", path);
        slash = strrchr(root, '/');
        {
            char* backslash = strrchr(root, '\\');
            if (backslash != NULL && (slash == NULL || backslash > slash))
                slash = backslash;
        }
        if (slash == NULL) {
            snprintf(root, sizeof(root), ".");
        }
        else if (slash == root) {
            root[1] = '\0';
        }
        else {
            *slash = '\0';
        }
        resource_directory_scan_root_set(root);
        (void)wwad_archive_index_load(path);
    }
}

/* ------------------------------------------------------------------ *
 *  Public API
 * ------------------------------------------------------------------ */

/* Explicit-root mount API used by tests and portable hosts. Retail startup
 * behavior is kept in resource_mount_startup_target below. */
int resource_mount(const char* dir, const char* prefix)
{
    char mount_root[RESOURCE_PATH_MAX];
    if (dir == NULL || prefix == NULL) return s_wad_count;
    if (dir != NULL) resource_directory_scan_root_set(dir);
    ares_cd_path("", mount_root, sizeof(mount_root));
    wad_mount_path_cache_store(mount_root);
    for (int i = 0; i < RESOURCE_MAX_WADS; ++i) {
        char path[RESOURCE_PATH_MAX];
        if (s_wad_count >= RESOURCE_MAX_WADS) break;
        snprintf(path, sizeof(path), "%s/%s%u.wad", dir, prefix,
                 (unsigned)i);
        FILE* fp = fopen(path, "rb");
        if (fp == NULL) {
            continue;
        }
        ResourceWad* w = &s_wads[s_wad_count];
        memset(w, 0, sizeof(*w));
        w->fp = fp;
        if (!ares_parse(fp, w)) {
            fprintf(stderr, "resource_mount: bad or truncated wad: %s\n",
                    path);
            free(w->names);
            free(w->entries);
            memset(w, 0, sizeof(*w));
            fclose(fp);
            continue;
        }
        snprintf(w->path, sizeof(w->path), "%s", path);
        ++s_wad_count;
    }
    return s_wad_count;
}

void resource_startup_ops_set(const ResourceStartupOps* ops)
{
    if (ops == NULL) {
        memset(&s_resource_startup_ops, 0, sizeof(s_resource_startup_ops));
    } else {
        s_resource_startup_ops = *ops;
    }
}

int resource_startup_result_get(void)
{
    return s_resource_startup_result;
}

static int resource_startup_current_directory(char* path, size_t capacity)
{
    if (path == NULL || capacity == 0u) return 0;
#if defined(_WIN32)
    {
        DWORD length = GetCurrentDirectoryA((DWORD)capacity, path);
        return length != 0u && length < capacity;
    }
#else
    return getcwd(path, capacity) != NULL;
#endif
}

static int resource_startup_directory_exists(const char* path)
{
#if defined(_WIN32)
    DWORD attributes = GetFileAttributesA(path);
    return attributes != INVALID_FILE_ATTRIBUTES
        && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0u;
#else
    struct stat status;
    return stat(path, &status) == 0 && S_ISDIR(status.st_mode);
#endif
}

static void resource_startup_directory_create(const char* path)
{
#if defined(_WIN32)
    (void)CreateDirectoryA(path, NULL);
#else
    (void)mkdir(path, 0755);
#endif
}

static int resource_startup_localized_text(const char* registry_key,
                                           const char* text_key,
                                           char* output, size_t capacity)
{
    int loaded;
    if (output == NULL || capacity == 0u) return 0;
    output[0] = '\0';
    if (s_resource_startup_ops.text_lookup == NULL) return 0;
    loaded = s_resource_startup_ops.text_lookup(
        s_resource_startup_ops.context, registry_key, text_key,
        output, capacity);
    output[capacity - 1u] = '\0';
    return loaded != 0 && output[0] != '\0';
}

static void resource_startup_exit(int result)
{
    s_resource_startup_result = result;
    if (s_resource_startup_ops.exit_process != NULL) {
        s_resource_startup_ops.exit_process(s_resource_startup_ops.context, 0);
    }
}

static int resource_startup_missing_data_dialog(const char* registry_key)
{
    char message[1024];
    char caption[128];
    if (s_resource_startup_ops.message_box == NULL ||
        !resource_startup_localized_text(registry_key, "DataMissing",
                                         message, sizeof(message)) ||
        !resource_startup_localized_text(registry_key, "SetupError",
                                         caption, sizeof(caption))) {
        s_resource_startup_result = RESOURCE_STARTUP_DEPENDENCY_MISSING;
        return 0;
    }
    (void)s_resource_startup_ops.message_box(s_resource_startup_ops.context,
                                             message, caption, 0u);
    resource_startup_exit(RESOURCE_STARTUP_MISSING_DATA);
    return 1;
}

static int resource_startup_cd_retry(const char* registry_key)
{
    char message[1024];
    char caption[128];
    if (s_resource_startup_ops.message_box == NULL ||
        !resource_startup_localized_text(registry_key, "CDMissing",
                                         message, sizeof(message)) ||
        !resource_startup_localized_text(registry_key, "SetupError",
                                         caption, sizeof(caption))) {
        s_resource_startup_result = RESOURCE_STARTUP_DEPENDENCY_MISSING;
        return 0;
    }
    if (s_resource_startup_ops.message_box(s_resource_startup_ops.context,
                                           message, caption, 1u) == 2) {
        resource_startup_exit(RESOURCE_STARTUP_CD_CANCELLED);
        return 0;
    }
    return 1;
}

static void resource_startup_delme_marker_remove(const char* path)
{
#if defined(_WIN32)
    DWORD attributes = GetFileAttributesA(path);
    if (attributes != INVALID_FILE_ATTRIBUTES) {
        (void)SetFileAttributesA(path, attributes & ~FILE_ATTRIBUTE_READONLY);
        (void)DeleteFileA(path);
    }
#else
    struct stat status;
    if (stat(path, &status) == 0) {
        (void)chmod(path, status.st_mode | S_IWUSR);
        (void)remove(path);
    }
#endif
}

/* WadMountStartup @ 0x0047f130. The startup owner supplies the executable
 * prefix and registry key; the resource module owns numbered archive
 * mounting, Data-root derivation, CD-marker probing, and mount-path caching.
 * Localized prompts and modal UI are host operations; process exit remains
 * optional so portable callers can inspect the startup result. */
// FUNCTION: LEGORR 0x0047f130
void resource_mount_startup_target(const char* prefix, int require_cd,
                                   const char* registry_key)
{
    char working_directory[260];
    char data_directory[RESOURCE_PATH_MAX];
    char marker_path[RESOURCE_PATH_MAX];
    size_t length;
    int mounted;
    int cd_found;
    int data_directory_found;
    int data_missing;

    s_resource_startup_result = RESOURCE_STARTUP_OK;
    if (prefix == NULL || prefix[0] == '\0') prefix = s_wad_prefix;
    if (!resource_startup_current_directory(working_directory,
                                            sizeof(working_directory))) {
        s_resource_startup_result = RESOURCE_STARTUP_PATH_ERROR;
        return;
    }
    length = strlen(working_directory);
    if (length > 1u &&
        (working_directory[length - 1u] == '/' ||
         working_directory[length - 1u] == '\\')) {
        working_directory[length - 1u] = '\0';
    }
    if (snprintf(data_directory, sizeof(data_directory), "%s%cData",
                 working_directory, RES_PATH_SEP) < 0 ||
        strlen(data_directory) >= sizeof(data_directory)) {
        s_resource_startup_result = RESOURCE_STARTUP_PATH_ERROR;
        return;
    }

    resource_unmount_all();
    s_cd_root[0] = '\0';
    s_cd_drive = 'A';
    snprintf(s_cd_data_dir, sizeof(s_cd_data_dir), "%s", "Data");
    resource_set_wad_prefix(prefix);
    mounted = resource_mount(working_directory, prefix);
    /* resource_mount is also the explicit-root test API; restore the retail
     * startup Data directory after its modern-root cache setup. */
    resource_directory_scan_root_set(data_directory);
    wad_mount_path_cache_store(data_directory);

    cd_found = resource_find_cd_key_drive();
    while (!cd_found && require_cd != 0) {
        if (!resource_startup_cd_retry(registry_key)) return;
        cd_found = resource_find_cd_key_drive();
    }

    data_directory_found = resource_startup_directory_exists(data_directory);
    if (!cd_found && data_directory_found) {
        /* A loose Data tree is the retail fallback when neither a WAD nor a
         * CD key drive is available. Mounted CD paths remain drive-rooted. */
        snprintf(s_cd_root, sizeof(s_cd_root), "%s", working_directory);
    }
    data_missing = (!cd_found && mounted == 0 && !data_directory_found);
    if (require_cd != 0 &&
        snprintf(marker_path, sizeof(marker_path), "%s%cData%ccd.key",
                 working_directory, RES_PATH_SEP, RES_PATH_SEP) >= 0 &&
        resource_cd_key_path_exists(marker_path)) {
        data_missing = 1;
    }
    if (data_missing) {
        (void)resource_startup_missing_data_dialog(registry_key);
        return;
    }

    resource_startup_directory_create(data_directory);

    if (snprintf(marker_path, sizeof(marker_path), "%s%cData%cdelme.dat",
                 working_directory, RES_PATH_SEP, RES_PATH_SEP) >= 0 &&
        resource_cd_key_path_exists(marker_path)) {
        resource_startup_delme_marker_remove(marker_path);
    }
}

int resource_mount_startup(const char* prefix, int require_cd,
                           const char* registry_key)
{
    resource_mount_startup_target(prefix, require_cd, registry_key);
    return resource_startup_result_get();
}

void resource_set_wad_prefix(const char* prefix)
{
    if (prefix == NULL || prefix[0] == '\0') {
        return;
    }
    snprintf(s_wad_prefix, sizeof(s_wad_prefix), "%s", prefix);
}

const char* resource_get_wad_prefix(void)
{
    return s_wad_prefix;
}

int resource_mounted_count(void)
{
    return s_wad_count;
}

void resource_unmount_all(void)
{
    for (int i = 0; i < s_wad_count; ++i) {
        ResourceWad* w = &s_wads[i];
        if (w->fp != NULL) {
            fclose(w->fp);
            w->fp = NULL;
        }
        free(w->names);
        free(w->entries);
        w->names = NULL;
        w->entries = NULL;
    }
    s_wad_count = 0;
}

uint32_t resource_wad_entry_count(int wad_idx)
{
    if (wad_idx < 0 || wad_idx >= s_wad_count) {
        return 0;
    }
    return s_wads[wad_idx].entry_count;
}

uint32_t resource_wad_entry_stored(int wad_idx, int entry_idx)
{
    if (wad_idx < 0 || wad_idx >= s_wad_count
        || entry_idx < 0
        || (uint32_t)entry_idx >= s_wads[wad_idx].entry_count) {
        return 0;
    }
    return s_wads[wad_idx].entries[entry_idx].stored;
}

const char* resource_wad_entry_name(int wad_idx, int entry_idx)
{
    if (wad_idx < 0 || wad_idx >= s_wad_count
        || entry_idx < 0
        || (uint32_t)entry_idx >= s_wads[wad_idx].entry_count) {
        return NULL;
    }
    ResourceWad* w = &s_wads[wad_idx];
    return w->names + w->entries[entry_idx].name_off;
}

/* FUNCTION: LEGORR 0x0048bce0 */
void* wad_archive_record_at_index(int index)
{
    if (index < 0 || index >= s_wad_count) {
        return NULL;
    }
    return &s_wads[index];
}

/* FUNCTION: LEGORR 0x0048bd10 */
uint32_t wad_entry_field4_get(int wad_idx, int entry_idx)
{
    if (wad_idx < 0 || wad_idx >= s_wad_count || entry_idx < 0
        || (uint32_t)entry_idx >= s_wads[wad_idx].entry_count) {
        return 0;
    }
    return s_wads[wad_idx].entries[entry_idx].orig;
}

/* FUNCTION: LEGORR 0x0048bd50 */
int wad_entry_find_across_mounted_archives(const char* name, int wad_idx)
{
    int i;
    if (name == NULL) {
        return -1;
    }
    if (wad_idx != -1) {
        return resource_wad_lookup(wad_idx, name);
    }
    for (i = s_wad_count - 1; i >= 0; --i) {
        int entry = resource_wad_lookup(i, name);
        if (entry != -1) {
            return entry;
        }
    }
    return -1;
}

// FUNCTION: LEGORR 0x0047ffc0
char* game_file_mounted_path_prefix_rewrite(const char* path)
{
    size_t path_length;
    size_t prefix_length;
    const char* relative_path;
    char prefix[0x400];
    if (!s_wad_mount_path_cache_valid || path == NULL) return (char*)path;
    path_length = strlen(path);
    prefix_length = strlen(s_wad_mount_path_cache);
    if (path_length <= prefix_length || prefix_length == 0u
        || prefix_length >= sizeof(prefix)) return (char*)path;
    memcpy(prefix, path, prefix_length);
    prefix[prefix_length] = '\0';
    if (strcasecmp_lite(prefix, s_wad_mount_path_cache) != 0)
        return (char*)path;
    relative_path = path + prefix_length;
    /* PathResolveAgainstBase forms `<Data root><separator><name>`, while the
     * WAD directory stores names without a leading separator. Strip that
     * join separator at the root boundary before looking up the archive
     * entry; otherwise a valid `Lego.cfg` is treated as a loose file. */
    while (*relative_path == '/' || *relative_path == '\\') {
        ++relative_path;
    }
    snprintf(s_wad_mount_path_rewrite, sizeof(s_wad_mount_path_rewrite),
             "%s", relative_path);
    return s_wad_mount_path_rewrite;
}

/* FileOpenRouteSelect (FUN_0047fe30): choose loose-file handling for
 * non-write modes only when the rewritten path is not present in a mounted
 * archive. */
// FUNCTION: LEGORR 0x0047fe30
int game_file_open_route_select(const char* path, const char* mode)
{
    char* rewritten;
    if (path == NULL || path[0] == '\0' || mode == NULL || mode[0] == '\0')
        return 2;
    if (mode[0] == 'w' || mode[0] == 'W') return 1;
    rewritten = game_file_mounted_path_prefix_rewrite(path);
    return wad_entry_find_across_mounted_archives(rewritten, -1) == -1;
}

/* Open a loose file (STD branch) — payload read lazily by resource_read. */
static Resource* ares_open_std(const char* name, int mode)
{
    FILE* fp = fopen(name, "rb");
    if (fp == NULL) {
        return NULL;
    }
    long size = -1;
    if (fseek(fp, 0, SEEK_END) == 0) {
        size = ftell(fp);
    }
    fclose(fp);
    Resource* r = (Resource*)calloc(1, sizeof(Resource));
    if (r == NULL) {
        return NULL;
    }
    r->name = ares_strdup(name);
    if (r->name == NULL) {
        free(r);
        return NULL;
    }
    r->branch    = RESOURCE_STD_BRANCH;
    r->wad_idx   = -1;
    r->entry_idx = -1;
    r->mode      = mode;
    r->size      = (int32_t)size;
    return r;
}

/* Open a resource by name. This portable API returns a Resource handle;
 * GameFileOpenDispatch has the separate GameFileHandle contract below. */
Resource* resource_open(const char* name, int mode)
{
    if (name == NULL || name[0] == '\0') {
        return NULL;
    }

    /* STD branch first for AUTO when the loose file exists. */
    if (mode == RESOURCE_AUTO || mode == RESOURCE_STD_ONLY) {
        Resource* r = ares_open_std(name, mode);
        if (r != NULL) {
            return r;
        }
        if (mode == RESOURCE_STD_ONLY) {
            return NULL;
        }
    }

    /* WAD branch: later mounts shadow earlier ones. */
    Resource* wad = ares_open_wad_path(name, mode);
    if (wad != NULL) {
        return wad;
    }

    /* CD/STD fallback (AUTO only): "%c:\%s\%s" with the drive letter +
     * data dir (finding 01, FUN_0047f660). */
    if (mode == RESOURCE_AUTO) {
        char path[RESOURCE_PATH_MAX];
        if (ares_cd_fallback_path(name, path, sizeof(path))) {
            Resource* r = ares_open_std(path, mode);
            if (r != NULL) {
                return r;
            }
        }
    }
    return NULL;
}

/* A ResourceStream owns an independent FILE cursor. WAD streams reopen the
 * archive instead of sharing ResourceWad::fp, so read_at calls do not disturb
 * concurrent Resource::resource_read operations. */
ResourceStream* resource_stream_open(const char* name, int mode,
                                     uint32_t* size_out)
{
    Resource* resource;
    ResourceStream* stream = NULL;
    if (size_out != NULL) *size_out = 0u;
    resource = resource_open(name, mode);
    if (resource == NULL) return NULL;
    stream = (ResourceStream*)calloc(1, sizeof(*stream));
    if (stream == NULL) {
        resource_free(resource);
        return NULL;
    }
    if (resource->branch == RESOURCE_STD_BRANCH) {
        long file_size;
        stream->file = fopen(resource->name, "rb");
        if (stream->file == NULL || fseek(stream->file, 0, SEEK_END) != 0 ||
            (file_size = ftell(stream->file)) < 0 ||
            (unsigned long)file_size > 0x7ffffffful ||
            fseek(stream->file, 0, SEEK_SET) != 0) {
            resource_stream_close(stream);
            resource_free(resource);
            return NULL;
        }
        stream->size = (uint32_t)file_size;
    }
    else if (resource->branch == RESOURCE_WAD_BRANCH &&
             resource->wad_idx >= 0 && resource->wad_idx < s_wad_count &&
             resource->entry_idx >= 0 &&
             (uint32_t)resource->entry_idx <
                 s_wads[resource->wad_idx].entry_count) {
        ResourceWad* wad = &s_wads[resource->wad_idx];
        WadEntry* entry = &wad->entries[resource->entry_idx];
        uint32_t stored_end = entry->off + entry->stored;
        if (stored_end < entry->off || stored_end > wad->file_size ||
            stored_end > 0x7fffffffu ||
            entry->off > 0x7fffffffu) {
            resource_stream_close(stream);
            resource_free(resource);
            return NULL;
        }
        stream->file = fopen(wad->path, "rb");
        if (stream->file == NULL) {
            resource_stream_close(stream);
            resource_free(resource);
            return NULL;
        }
        if ((entry->flags & ENTRY_FLAG_EXPANDED) == 0u) {
            stream->data_offset = entry->off;
            stream->size = entry->stored;
        }
        else {
            uint8_t expand_header[EXPAND_HEADER_SZ];
            uint32_t expanded_size;
            uint8_t type;
            if (entry->stored < EXPAND_HEADER_SZ ||
                fseek(stream->file, (long)entry->off, SEEK_SET) != 0 ||
                fread(expand_header, 1, sizeof(expand_header), stream->file) !=
                    sizeof(expand_header) ||
                memcmp(expand_header, "RNC", 3) != 0) {
                resource_stream_close(stream);
                resource_free(resource);
                return NULL;
            }
            type = expand_header[3];
            expanded_size = ((uint32_t)expand_header[4] << 24) |
                            ((uint32_t)expand_header[5] << 16) |
                            ((uint32_t)expand_header[6] << 8) |
                            (uint32_t)expand_header[7];
            if (expanded_size > 0x7fffffffu) {
                resource_stream_close(stream);
                resource_free(resource);
                return NULL;
            }
            stream->size = expanded_size;
            if (type == 0u) {
                if (expanded_size > entry->stored - EXPAND_HEADER_SZ ||
                    entry->off + EXPAND_HEADER_SZ > 0x7fffffffu) {
                    resource_stream_close(stream);
                    resource_free(resource);
                    return NULL;
                }
                stream->data_offset = entry->off + EXPAND_HEADER_SZ;
            }
            else if (type == 1u || type == 2u) {
                uint8_t* stored;
                stored = (uint8_t*)malloc(entry->stored);
                stream->expanded_data = (uint8_t*)malloc(
                    expanded_size != 0u ? expanded_size : 1u);
                if (stored == NULL || stream->expanded_data == NULL ||
                    fseek(stream->file, (long)entry->off, SEEK_SET) != 0 ||
                    fread(stored, 1, entry->stored, stream->file) !=
                        entry->stored ||
                    (((uint32_t)wad_payload_decode_dispatch(
                        stored, stream->expanded_data) & 0xffffu) != 0u)) {
                    free(stored);
                    resource_stream_close(stream);
                    resource_free(resource);
                    return NULL;
                }
                free(stored);
            }
            else {
                resource_stream_close(stream);
                resource_free(resource);
                return NULL;
            }
        }
    }
    else {
        resource_stream_close(stream);
        resource_free(resource);
        return NULL;
    }
    resource_free(resource);
    if (size_out != NULL) *size_out = stream->size;
    return stream;
}

int resource_stream_read_at(ResourceStream* stream, uint32_t offset,
                            void* bytes, uint32_t length)
{
    uint32_t file_offset;
    if (stream == NULL || stream->file == NULL ||
        (length != 0u && bytes == NULL) || offset > stream->size ||
        length > stream->size - offset ||
        (stream->expanded_data == NULL &&
         stream->data_offset > 0x7fffffffu - offset)) return 0;
    if (stream->expanded_data != NULL) {
        if (length != 0u)
            memcpy(bytes, stream->expanded_data + offset, length);
        return 1;
    }
    file_offset = stream->data_offset + offset;
    if (fseek(stream->file, (long)file_offset, SEEK_SET) != 0) return 0;
    if (length == 0u) return 1;
    return fread(bytes, 1, length, stream->file) == length;
}

void resource_stream_close(ResourceStream* stream)
{
    if (stream == NULL) return;
    if (stream->file != NULL) fclose(stream->file);
    free(stream->expanded_data);
    free(stream);
}

static int resource_file_token_list_parse(const char* token_file,
                                          ResourceFileTokenList* list,
                                          int clear_before_open)
{
    Resource* resource;
    int32_t size;
    int32_t position = 0;
    if (list == NULL || token_file == NULL) return -1;
    if (clear_before_open) list->count = 0u;
    resource = resource_open(token_file, RESOURCE_AUTO);
    if (resource == NULL) return -1;
    list->count = 0u;
    size = resource_read(resource);
    if (size < 0 || resource->data == NULL) {
        resource_free(resource);
        return -1;
    }
    while (position < size && list->count < 512u) {
        uint32_t length = 0u;
        char* token;
        while (position < size &&
               (resource->data[position] == ' ' ||
                resource->data[position] == '\t' ||
                resource->data[position] == '\r' ||
                resource->data[position] == '\n')) ++position;
        if (position >= size) break;
        token = list->tokens[list->count];
        while (position < size && resource->data[position] != ' ' &&
               resource->data[position] != '\t' &&
               resource->data[position] != '\r' &&
               resource->data[position] != '\n') {
            if (length < 255u) token[length++] = (char)resource->data[position];
            ++position;
        }
        token[length] = '\0';
        if (length != 0u) list->count++;
    }
    resource_free(resource);
    return (int)list->count;
}

/* Portable adapter: tokenize a named file into an explicit list and invoke
 * the optional caller-owned directory callback. */
int resource_file_token_list_load_and_tree_scan(
    const char* token_file, const char* root, ResourceFileTokenList* list,
    const ResourceFileTokenListOps* ops)
{
    int count = resource_file_token_list_parse(token_file, list, 1);
    if (count < 0) return count;
    if (ops != NULL && ops->directory_tree_scan != NULL) {
        ops->directory_tree_scan(ops->context, root, list);
    }
    return count;
}

/* FileTokenListLoadAndTreeScan (FUN_004802d0): retail ABI takes only the
 * token filename; root and token table are module-owned loader state. */
// FUNCTION: LEGORR 0x004802d0
void resource_file_token_list_load_and_tree_scan_target(
    const char* token_file)
{
    int result;
    if (token_file == NULL) return;
    result = resource_file_token_list_parse(token_file,
                                            &s_resource_file_tokens, 0);
    if (result < 0) return;
    if (s_resource_directory_root[0] != '\0')
        resource_directory_tree_scan_target(s_resource_directory_root);
}

/* DirectoryTokenListCompare (FUN_00480570): find a token with the same
 * ASCII case-folded spelling. The original discards the comparison result;
 * this API exposes it to the reconstructed directory walker. */
// FUNCTION: LEGORR 0x00480570
int resource_directory_token_list_contains(
    const ResourceFileTokenList* list, const char* token)
{
    uint32_t index;
    if (list == NULL || token == NULL) return 0;
    for (index = 0u; index < list->count && index < 512u; ++index) {
        if (strcasecmp_lite(token, list->tokens[index]) == 0) return 1;
    }
    return 0;
}

typedef struct ResourceDirectoryScanHandle {
#if defined(_WIN32)
    HANDLE search;
    WIN32_FIND_DATAA data;
    int first;
#else
    DIR* directory;
    char root[1024];
#endif
} ResourceDirectoryScanHandle;

static int resource_directory_scan_default_open(void*, const char* pattern,
                                               void** scan_out)
{
    ResourceDirectoryScanHandle* scan;
    if (pattern == NULL || scan_out == NULL) return 0;
    *scan_out = NULL;
    scan = (ResourceDirectoryScanHandle*)calloc(1, sizeof(*scan));
    if (scan == NULL) return 0;
#if defined(_WIN32)
    scan->search = FindFirstFileA(pattern, &scan->data);
    if (scan->search == INVALID_HANDLE_VALUE) {
        free(scan);
        return 0;
    }
    scan->first = 1;
#else
    {
        size_t length = strlen(pattern);
        size_t root_length;
        char root[1024];
        if (length < 3u || strcmp(pattern + length - 3u, "*.*") != 0) {
            free(scan);
            return 0;
        }
        root_length = length - 3u;
        if (root_length != 0u &&
            (pattern[root_length - 1u] == '/' ||
             pattern[root_length - 1u] == '\\')) --root_length;
        if (root_length >= sizeof(root)) {
            free(scan);
            return 0;
        }
        memcpy(root, pattern, root_length);
        root[root_length] = '\0';
        {
            size_t i;
            for (i = 0u; i < root_length; ++i)
                if (root[i] == '\\') root[i] = '/';
        }
        scan->directory = opendir(root_length != 0u ? root : ".");
        if (scan->directory == NULL) {
            free(scan);
            return 0;
        }
        memcpy(scan->root, root, root_length + 1u);
    }
#endif
    *scan_out = scan;
    return 1;
}

static int resource_directory_scan_default_next(void*, void* handle,
                                                char name_out[260],
                                                int* is_directory_out)
{
    ResourceDirectoryScanHandle* scan =
        (ResourceDirectoryScanHandle*)handle;
    if (scan == NULL || name_out == NULL || is_directory_out == NULL)
        return 0;
#if defined(_WIN32)
    if (scan->first) {
        scan->first = 0;
    }
    else if (!FindNextFileA(scan->search, &scan->data)) {
        return 0;
    }
    if (strlen(scan->data.cFileName) >= 260u) return 1;
    strcpy(name_out, scan->data.cFileName);
    *is_directory_out =
        (scan->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
    for (;;) {
        struct dirent* entry = readdir(scan->directory);
        struct stat info;
        char path[1280];
        size_t root_length;
        size_t name_length;
        if (entry == NULL) return 0;
        name_length = strlen(entry->d_name);
        if (name_length >= 260u) continue;
        root_length = strlen(scan->root);
        if (root_length + name_length + 2u > sizeof(path)) continue;
        if (root_length != 0u) {
            memcpy(path, scan->root, root_length);
            path[root_length] = '/';
            memcpy(path + root_length + 1u, entry->d_name,
                   name_length + 1u);
        }
        else {
            memcpy(path, entry->d_name, name_length + 1u);
        }
        if (stat(path, &info) != 0) continue;
        strcpy(name_out, entry->d_name);
        *is_directory_out = S_ISDIR(info.st_mode) != 0;
        return 1;
    }
#endif
    return 1;
}

static void resource_directory_scan_default_close(void*, void* handle)
{
    ResourceDirectoryScanHandle* scan =
        (ResourceDirectoryScanHandle*)handle;
    if (scan == NULL) return;
#if defined(_WIN32)
    if (scan->search != INVALID_HANDLE_VALUE) FindClose(scan->search);
#else
    if (scan->directory != NULL) closedir(scan->directory);
#endif
    free(scan);
}

static void resource_directory_tree_scan_walk(
    const char* root, const ResourceFileTokenList* tokens,
    const ResourceDirectoryScanOps* ops)
{
    void* scan = NULL;
    char name[260];
    char pattern[1024];
    unsigned int root_length;
    unsigned int offset;
    ResourceDirectoryScanOps default_ops;
    if (root == NULL) return;
    if (ops == NULL) {
        default_ops.context = NULL;
        default_ops.scan_open = resource_directory_scan_default_open;
        default_ops.scan_next = resource_directory_scan_default_next;
        default_ops.scan_close = resource_directory_scan_default_close;
        ops = &default_ops;
    }
    if (ops->scan_open == NULL || ops->scan_next == NULL) return;
    root_length = (unsigned int)strlen(root);
    if (root_length + 5u >= sizeof(pattern)) return;
    memcpy(pattern, root, root_length);
    offset = root_length;
    if (offset != 0u && pattern[offset - 1u] != '/' &&
        pattern[offset - 1u] != '\\') {
        pattern[offset++] = '\\';
    }
    memcpy(pattern + offset, "*.*", 4u);
    if (!ops->scan_open(ops->context, pattern, &scan) || scan == NULL) return;
    for (;;) {
        int is_directory = 0;
        unsigned int name_length;
        char child_path[1024];
        int next;
        name[0] = '\0';
        next = ops->scan_next(ops->context, scan, name, &is_directory);
        if (next <= 0) break;
        name[259] = '\0';
        if (name[0] == '\0' || strcmp(name, ".") == 0 ||
            strcmp(name, "..") == 0) continue;
        root_length = (unsigned int)strlen(root);
        name_length = (unsigned int)strlen(name);
        if (root_length + name_length + 2u >= sizeof(child_path)) continue;
        memcpy(child_path, root, root_length);
        offset = root_length;
        if (offset != 0u && child_path[offset - 1u] != '/' &&
            child_path[offset - 1u] != '\\') {
#if defined(_WIN32)
            child_path[offset++] = '\\';
#else
            child_path[offset++] = '/';
#endif
        }
        memcpy(child_path + offset, name, name_length + 1u);
        if (is_directory != 0) {
            resource_directory_tree_scan_walk(child_path, tokens, ops);
        }
        else {
            /* The reference performs this comparison for side effects only;
             * its helper does not return a membership result. */
            resource_directory_token_list_contains(tokens, child_path);
        }
    }
    if (ops->scan_close != NULL) ops->scan_close(ops->context, scan);
}

/* Portable adapter: search an explicit root with an explicit token list and
 * optional host scan operations. */
void resource_directory_tree_scan(const char* root,
                                  const ResourceFileTokenList* tokens,
                                  const ResourceDirectoryScanOps* ops)
{
    resource_directory_tree_scan_walk(root, tokens, ops);
}

/* DirectoryTreeScan (FUN_00480390): retail ABI takes one root and reads the
 * process-wide token state populated by FileTokenListLoadAndTreeScan. */
// FUNCTION: LEGORR 0x00480390
void resource_directory_tree_scan_target(const char* root)
{
    ResourceDirectoryScanHandle* scan = NULL;
    char pattern[1024];
    char name[260];
    char child_path[1024];
    unsigned int root_length;
    unsigned int offset;
    if (root == NULL) return;

    /* Keep the target's local root\*.* pattern construction here instead
     * of forwarding through the portable explicit-state walker. */
    root_length = (unsigned int)strlen(root);
    if (root_length + 5u >= sizeof(pattern)) return;
    memcpy(pattern, root, root_length);
    offset = root_length;
    if (offset != 0u && pattern[offset - 1u] != '/' &&
        pattern[offset - 1u] != '\\') {
        pattern[offset++] = '\\';
    }
    memcpy(pattern + offset, "*.*", 4u);
    if (!resource_directory_scan_default_open(NULL, pattern,
                                               (void**)&scan) ||
        scan == NULL) return;

    for (;;) {
        int is_directory = 0;
        unsigned int name_length;
        int next;
        name[0] = '\0';
        next = resource_directory_scan_default_next(NULL, scan, name,
                                                     &is_directory);
        if (next <= 0) break;
        name[259] = '\0';
        if (name[0] == '\0' || strcmp(name, ".") == 0 ||
            strcmp(name, "..") == 0) continue;
        root_length = (unsigned int)strlen(root);
        name_length = (unsigned int)strlen(name);
        if (root_length + name_length + 2u >= sizeof(child_path)) continue;
        memcpy(child_path, root, root_length);
        offset = root_length;
        if (offset != 0u && child_path[offset - 1u] != '/' &&
            child_path[offset - 1u] != '\\') {
#if defined(_WIN32)
            child_path[offset++] = '\\';
#else
            child_path[offset++] = '/';
#endif
        }
        memcpy(child_path + offset, name, name_length + 1u);
        if (is_directory != 0) {
            resource_directory_tree_scan_target(child_path);
        }
        else {
            /* Retail calls the comparator for its side effect and ignores
             * the membership result. */
            (void)resource_directory_token_list_contains(
                &s_resource_file_tokens, child_path);
        }
    }
    resource_directory_scan_default_close(NULL, scan);
}

int32_t resource_read(Resource* r)
{
    if (r == NULL || r->data != NULL) {
        return r != NULL ? r->size : -1;
    }
    if (r->branch == RESOURCE_STD_BRANCH) {
        FILE* fp = fopen(r->name, "rb");
        if (fp == NULL || r->size < 0) {
            if (fp != NULL) {
                fclose(fp);
            }
            return -1;
        }
        uint8_t* buf = (uint8_t*)malloc((size_t)r->size > 0 ? (size_t)r->size : 1u);
        if (buf == NULL || fread(buf, 1, (size_t)r->size, fp) != (size_t)r->size) {
            free(buf);
            fclose(fp);
            return -1;
        }
        fclose(fp);
        r->data = buf;
        return r->size;
    }

    ResourceWad* w = &s_wads[r->wad_idx];
    WadEntry*    e = &w->entries[r->entry_idx];

    /* Read the stored bytes. */
    long payload_off = (long)e->off;
    if (payload_off < 0 || (uint32_t)payload_off + e->stored > w->file_size) {
        return -1;
    }
    uint8_t* stored = (uint8_t*)malloc(e->stored > 0 ? e->stored : 1u);
    if (stored == NULL || fseek(w->fp, payload_off, SEEK_SET) != 0
        || fread(stored, 1, e->stored, w->fp) != e->stored) {
        free(stored);
        return -1;
    }

    if ((e->flags & ENTRY_FLAG_EXPANDED) == 0) {
        r->data = stored;
        r->size = (int32_t)e->stored;
        return r->size;
    }

    /* Expanded entry (finding 01): 4-byte magic, type byte at [3],
     * big-endian ("byte-swapped") length at [4], payload from +0x12. */
    if (e->stored < EXPAND_HEADER_SZ) {
        free(stored);
        return -1;
    }
    uint32_t type = stored[3];
    uint32_t len  = ((uint32_t)stored[4] << 24) | ((uint32_t)stored[5] << 16)
                  | ((uint32_t)stored[6] << 8) | (uint32_t)stored[7];
    if (len > 0x7fffffffu ||
        (type == 0u && len > e->stored - EXPAND_HEADER_SZ) ||
        (type > 2u) || memcmp(stored, "RNC", 3) != 0) {
        free(stored);
        return -1;
    }
    uint8_t* buf = (uint8_t*)malloc(len > 0 ? len : 1u);
    if (buf == NULL) {
        free(stored);
        return -1;
    }
    if (((uint32_t)wad_payload_decode_dispatch(stored, buf) & 0xffffu) != 0u) {
        free(stored);
        free(buf);
        return -1;
    }
    free(stored);
    r->data = buf;
    r->size = (int32_t)len;
    return r->size;
}

void resource_free(Resource* r)
{
    if (r == NULL) {
        return;
    }
    free(r->data);
    free(r->name);
    free(r);
}

static WholeFileProgressHook s_whole_file_progress_hook;
static void* s_whole_file_progress_user;

/* FUNCTION: LEGORR 0x004800C0 */
void* read_whole_file_with_progress(const char* path, int32_t* size_out,
                                    int binary_mode)
{
    GameFileHandle* handle;
    int32_t length;
    void* buffer;
    int read_count;
    const char* mode = binary_mode != 0 ? "rb" : "r";
    if (size_out != NULL) {
        *size_out = 0;
    }
    if (path == NULL) {
        return NULL;
    }
    handle = game_file_open_dispatch(path, mode);
    if (handle == NULL) {
        return NULL;
    }
    (void)wad_file_seek(handle, 0, 2);
    length = wad_file_cursor_get(handle);
    if (length < 0) {
        game_file_close(handle);
        return NULL;
    }
    if (s_whole_file_progress_hook != NULL) {
        s_whole_file_progress_hook(path, (uint32_t)length,
                                   s_whole_file_progress_user);
    }
    buffer = engine_heap_alloc(length > 0 ? (long)length : 1L);
    if (buffer == NULL) {
        game_file_close(handle);
        return NULL;
    }
    (void)wad_file_seek(handle, 0, 0);
    read_count = wad_file_read(buffer, 1, length, handle);
    game_file_close(handle);
    if (read_count != length) {
        engine_heap_free(buffer);
        return NULL;
    }
    if (size_out != NULL) {
        *size_out = length;
    }
    return buffer;
}

/* FUNCTION: LEGORR 0x004800A0 */
void* read_whole_file(const char* path, int32_t* size_out)
{
    return read_whole_file_with_progress(path, size_out, 1);
}

void whole_file_progress_hook_set(WholeFileProgressHook hook, void* user)
{
    s_whole_file_progress_hook = hook;
    s_whole_file_progress_user = user;
}

/* FixedRecordSlotAllocate (FUN_0048be00): the file-resource record band is
 * separate from the 16-entry WAD stream-slot table below. The reference
 * scans ten 0x24-byte records by their +4 occupancy dword, zeros all nine
 * dwords in the first free record, and returns its index. The port stores
 * those exact 32-bit words; pointer-bearing users are represented by the
 * surrounding resource-layer APIs rather than widening this corpus record. */
#define FILE_RESOURCE_RECORD_SLOTS 10
typedef struct FileResourceRecordSlot {
    uint32_t words[9];
} FileResourceRecordSlot;
static FileResourceRecordSlot s_file_resource_record_slots[
    FILE_RESOURCE_RECORD_SLOTS];

/* FUNCTION: LEGORR 0x0048BE00 */
int file_resource_record_slot_allocate(void)
{
    int i;
    for (i = 0; i < FILE_RESOURCE_RECORD_SLOTS; ++i) {
        if (s_file_resource_record_slots[i].words[1] == 0u) {
            memset(&s_file_resource_record_slots[i], 0,
                   sizeof(s_file_resource_record_slots[i]));
            return i;
        }
    }
    return -1;
}

typedef char fixed_record_slot_stride_check[sizeof(FixedRecordSlot) == 16u ? 1 : -1];

// GLOBAL: LEGORR 0x005756a0
FixedRecordSlot s_fixed_slots[FIXED_RECORD_SLOTS];
#if defined(_WIN64) || defined(__x86_64__) || defined(__aarch64__)
/* The retail record stores a 32-bit pointer. Keep its 16-byte layout and
 * retain native pointers separately on wider hosts. */
void* s_fixed_slot_resources[FIXED_RECORD_SLOTS];
#endif

typedef struct WadFileStreamState {
    int slot;
    int32_t cursor;
} WadFileStreamState;

/* FUNCTION: LEGORR 0x0048bd30 */
int wad_entry_slot_allocate(void)
{
    int i;
    for (i = 0; i < FIXED_RECORD_SLOTS; ++i) {
        if (s_fixed_slots[i].in_use == 0) {
            return i;
        }
    }
    return -1;
}

void* fixed_record_slot_resource_get(int slot)
{
    if ((unsigned int)slot >= FIXED_RECORD_SLOTS) return NULL;
    return fixed_record_slot_resource_get_target(slot);
}

/* Port-side payload length used by the file seek implementation. */
static int resource_slot_length_get(int slot)
{
    Resource* resource;
    int32_t length;
    resource = (Resource*)fixed_record_slot_resource_get(slot);
    if (resource == NULL) {
        return 0;
    }
    length = resource_read(resource);
    return length > 0 ? (int)length : 0;
}

/* WadFileSeek (FUN_0047f850): update the WAD stream cursor for SET/CUR/END.
 * The END case preserves the corpus's length-plus-offset behavior. */
// FUNCTION: LEGORR 0x0047f850
int wad_file_seek(GameFileHandle* handle, int offset, int origin)
{
    WadFileStreamState* state;
    int length;
    uint32_t type;
    if (handle == NULL) return 0;
    type = game_file_system_type_get(handle);
    if (type == 1u) {
        if (handle->stream == NULL) return 0;
        return fseek((FILE*)handle->stream, offset, origin);
    }
    if (type != 0u) {
        resource_debug_output_format(s_game_file_unknown_type_format,
            s_game_file_source_path, 0x170, s_game_file_seek_name);
        return 0;
    }
    if (handle->stream == NULL) return 0;
    state = (WadFileStreamState*)handle->stream;
    if (origin == 0) {
        state->cursor = offset;
        length = resource_slot_length_get(state->slot);
        if (length < state->cursor) state->cursor = length;
        if (state->cursor < 0) state->cursor = 0;
    }
    else if (origin == 1) {
        state->cursor += offset;
        length = resource_slot_length_get(state->slot);
        if (length < state->cursor) state->cursor = length;
        if (state->cursor < 0) state->cursor = 0;
    }
    else if (origin == 2) {
        state->cursor = resource_slot_length_get(state->slot) + offset;
    }
    else {
        resource_debug_output_format(s_game_file_seek_mode_format, origin);
    }
    return 0;
}

/* WadFileRead (FUN_0047f980): read complete elements from the slot resource,
 * clipping the byte count at its end and advancing the cursor. */
// FUNCTION: LEGORR 0x0047f980
int wad_file_read(void* destination, int element_size, int element_count,
                  GameFileHandle* handle)
{
    WadFileStreamState* state;
    Resource* resource;
    int length;
    unsigned int bytes;
    uint32_t type;
    if (handle == NULL) {
        return 0;
    }
    type = game_file_system_type_get(handle);
    if (type == 1u) {
        if (destination == NULL || handle->stream == NULL || element_size <= 0
            || element_count <= 0) return 0;
        return (int)fread(destination, (size_t)element_size,
                          (size_t)element_count, (FILE*)handle->stream);
    }
    if (type != 0u) {
        resource_debug_output_format(s_game_file_unknown_type_format,
            s_game_file_source_path, 0x18b, "WadFileRead");
        return 0;
    }
    if (destination == NULL || handle->stream == NULL || element_size <= 0
        || element_count <= 0) return 0;
    state = (WadFileStreamState*)handle->stream;
    resource = (Resource*)fixed_record_slot_resource_get(state->slot);
    if (resource == NULL) return 0;
    length = resource_read(resource);
    if (length <= 0 || state->cursor < 0 || state->cursor >= length) return 0;
    bytes = (unsigned int)element_size * (unsigned int)element_count;
    if (bytes > (unsigned int)(length - state->cursor)) {
        bytes = (unsigned int)(length - state->cursor);
    }
    memcpy(destination, resource->data + state->cursor, bytes);
    state->cursor += (int32_t)bytes;
    return (int)(bytes / (unsigned int)element_size);
}

/* WadFileCursorGet (FUN_0047fb10): return the current WAD stream cursor. */
// FUNCTION: LEGORR 0x0047fb10
int wad_file_cursor_get(GameFileHandle* handle)
{
    WadFileStreamState* state;
    uint32_t type;
    if (handle == NULL) return 0;
    type = game_file_system_type_get(handle);
    if (type == 0u) {
        if (handle->stream == NULL) return 0;
        state = (WadFileStreamState*)handle->stream;
        return state->cursor;
    }
    if (type == 1u) {
        if (handle->stream == NULL) return 0;
        return (int)ftell((FILE*)handle->stream);
    }
    resource_debug_output_format(s_game_file_unknown_type_format,
        s_game_file_source_path, 0x1d5, s_game_file_tell_name);
    return 0;
}

/* GameFileReadByte (FUN_0047fc20) and its line readers (FUN_0047fce0,
 * FUN_0047fd40): type 0 dispatches to the WAD cursor buffer and type 1 to
 * the FILE-backed CRT stream. */
// FUNCTION: LEGORR 0x0047fc20
int game_file_read_byte(GameFileHandle* handle)
{
    WadFileStreamState* state;
    Resource* resource;
    int length;
    int value;
    uint32_t type;
    if (handle == NULL) return -1;
    type = game_file_system_type_get(handle);
    if (type == 1u) {
        return handle->stream != NULL ? fgetc((FILE*)handle->stream) : -1;
    }
    if (type != 0u) {
        resource_debug_output_format(s_game_file_unknown_type_format,
            s_game_file_source_path, 0x221, s_game_file_getc_name);
        return 0;
    }
    if (handle->stream == NULL) return -1;
    state = (WadFileStreamState*)handle->stream;
    resource = (Resource*)fixed_record_slot_resource_get(state->slot);
    if (resource == NULL) return -1;
    length = resource_read(resource);
    if (length <= 0 || state->cursor < 0 || state->cursor >= length) return -1;
    value = resource->data[state->cursor++];
    return value;
}

// FUNCTION: LEGORR 0x0047fce0
char* game_file_read_line(char* buffer, int capacity,
                          GameFileHandle* handle)
{
    int count = 0;
    int c;
    if (buffer == NULL || capacity <= 1) return NULL;
    while (count < capacity - 1) {
        c = game_file_read_byte(handle);
        if (c < 0) break;
        buffer[count++] = (char)c;
        if (c == '\0' || c == '\n') break;
    }
    if (count == 0) return NULL;
    buffer[count] = '\0';
    return buffer;
}

// FUNCTION: LEGORR 0x0047fd40
char* game_file_read_line_dispatch(char* buffer, int capacity,
                                   GameFileHandle* handle)
{
    uint32_t type;
    if (handle == NULL) return NULL;
    type = game_file_system_type_get(handle);
    if (type == 0u) {
        return game_file_read_line(buffer, capacity, handle);
    }
    if (type == 1u) {
        if (buffer == NULL || capacity <= 0 || handle->stream == NULL) {
            return NULL;
        }
        return fgets(buffer, capacity, (FILE*)handle->stream);
    }
    resource_debug_output_format(s_game_file_unknown_type_format,
        s_game_file_source_path, 0x24b, s_game_file_gets_name);
    return NULL;
}

// FUNCTION: LEGORR 0x00480050
char* game_file_read_crlf_line(char* buffer, int capacity,
                               GameFileHandle* handle)
{
    char* result = game_file_read_line_dispatch(buffer, capacity, handle);
    size_t length;
    if (result == NULL) return NULL;
    length = strlen(buffer);
    if (length != 0u && buffer[length - 1u] == '\n') {
        buffer[--length] = '\0';
    }
    if (length != 0u && buffer[length - 1u] == '\r') {
        buffer[length - 1u] = '\0';
    }
    return result;
}

void fixed_record_slot_install(int slot, void* resource)
{
    if (slot >= 0 && slot < FIXED_RECORD_SLOTS) {
#if defined(_WIN64) || defined(__x86_64__) || defined(__aarch64__)
        s_fixed_slot_resources[slot] = resource;
        s_fixed_slots[slot].resource = 0u;
#else
        s_fixed_slots[slot].resource = (uint32_t)(uintptr_t)resource;
#endif
        s_fixed_slots[slot].in_use = 1;
    }
}

int fixed_record_slot_flag_peek(int slot)
{
    return (slot >= 0 && slot < FIXED_RECORD_SLOTS) ? (int)s_fixed_slots[slot].in_use : 0;
}

static void fixed_record_slot_release(int slot)
{
    if (slot < 0 || slot >= FIXED_RECORD_SLOTS) return;
    if (fixed_record_slot_pointer_get(slot) != NULL) {
        resource_free((Resource*)fixed_record_slot_pointer_get(slot));
    }
#if defined(_WIN64) || defined(__x86_64__) || defined(__aarch64__)
    s_fixed_slot_resources[slot] = NULL;
#endif
    s_fixed_slots[slot].resource = 0u;
    s_fixed_slots[slot].in_use = 0;
}
#undef fixed_record_slot_pointer_get

// FUNCTION: LEGORR 0x0047ff50
void game_file_handle_release(GameFileHandle* handle)
{
    if (handle == NULL) return;
    if (handle->flag == 1) {
        if (handle->stream != NULL) fclose((FILE*)handle->stream);
        engine_heap_free_wrapper(handle);
        return;
    }
    else if (handle->flag == 0) {
        WadFileStreamState* state = (WadFileStreamState*)handle->stream;
        if (state != NULL) {
            fixed_record_slot_release(state->slot);
            engine_heap_free_wrapper(state);
        }
    }
    else {
        return;
    }
    engine_heap_free_wrapper(handle);
}

// FUNCTION: LEGORR 0x0047fac0
int game_file_close(GameFileHandle* handle)
{
    if (handle == NULL) return 0;
    if (handle->flag == 0 || handle->flag == 1) {
        game_file_handle_release(handle);
        return 0;
    }
    resource_debug_output_format(s_game_file_unknown_type_format,
        s_game_file_source_path, 0x1af, s_game_file_close_name);
    return 0;
}

/* GameFileWrite (FUN_0047fa50): writes are supported by loose-file streams;
 * WAD entries are immutable and report a zero item count. */
// FUNCTION: LEGORR 0x0047fa50
uint32_t game_file_write(const void* data, uint32_t size,
                         uint32_t count, GameFileHandle* handle)
{
    uint32_t type;
    if (handle == NULL) {
        return 0u;
    }
    type = game_file_system_type_get(handle);
    if (type == 0u) {
        resource_debug_output_format(s_game_file_write_wad_error);
        return 0u;
    }
    if (type != 1u) {
        resource_debug_output_format(s_game_file_unknown_type_format,
            s_game_file_source_path, 0x19e, s_game_file_write_name);
        return 0u;
    }
    if (handle->stream == NULL || data == NULL || size == 0u || count == 0u)
        return 0u;
    return (uint32_t)fwrite(data, (size_t)size, (size_t)count,
                            (FILE*)handle->stream);
}

/* GameFilePrintFormat (FUN_0047fdb0): forward formatted text to a loose
 * stream, returning the corpus's fixed zero status. */
// FUNCTION: LEGORR 0x0047fdb0
int game_file_print_format(GameFileHandle* handle, const char* format, ...)
{
    uint32_t type;
    va_list args;
    if (handle == NULL) return 0;
    type = game_file_system_type_get(handle);
    if (type == 0u) {
        resource_debug_output_format(s_game_file_printf_wad_error);
        return 0;
    }
    if (type != 1u) {
        resource_debug_output_format(s_game_file_unknown_type_format,
            s_game_file_source_path, 0x264, s_game_file_printf_name);
        return 0;
    }
    if (handle->stream == NULL || format == NULL) return 0;
    va_start(args, format);
    (void)vfprintf((FILE*)handle->stream, format, args);
    va_end(args);
    return 0;
}

/* DebugOutputFormat (FUN_0047f600) formats one variadic message into the
 * same 1024-byte scratch size as the reference and emits it to the process
 * debug stream. The portable backend uses stderr for that stream. */
// FUNCTION: LEGORR 0x0047f600
void resource_debug_output_format(const char* format, ...)
{
    char buffer[1024];
    va_list args;
    if (format == NULL) {
        return;
    }
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
    fputs(buffer, stderr);
}

/* --- wad payload band (2026-10-25; finding 01 codec story) ---------------
 * The corpus decodes expanded wad entries through the RNC-header dispatch
 * (the DAT_004b2d08 global holds the three-byte magic "RNC"), builds the
 * stream-buffer handle pair, and clears the type-1 Huffman table between
 * streams. The codecs themselves are ported above the dispatch (the
 * FUN_0049c8e0 / FUN_0049ca60 pair); no shipped entry sets the expanded
 * flag, so their live callers stay the dispatch and the tests. */

#define WAD_DECODE_MAGIC_FAIL 0xffff
#define WAD_DECODE_TYPE_FAIL  0xfffe

/* DAT_00557d80 / DAT_00557e80 / DAT_00557f80: the type-1 codec's three
 * 16-symbol Huffman tables (literal counts, back-reference offsets, match
 * lengths). They occupy 0x100-byte bands: sixteen 16-byte entries, cleared
 * and filled by WadHuffmanTableRead with count 16. The symbol decoder keeps
 * the retail unbounded scan when no entry matches. */
// GLOBAL: LEGORR 0x00557d80
static WadCodecTableEntry s_wad_huff_literal[16];
// GLOBAL: LEGORR 0x00557e80
static WadCodecTableEntry s_wad_huff_offset[16];
// GLOBAL: LEGORR 0x00557f80
static WadCodecTableEntry s_wad_huff_length[16];

/* Both RNC codec entries return the output-end pointer with its low word
 * cleared. ExpandWadPayload tests AX, so that nonzero upper word is success. */
static uint32_t wad_codec_result_word(const void* pointer)
{
    return (uint32_t)(uintptr_t)pointer & 0xffff0000u;
}

/* DecodeWadPayloadType1 (FUN_0049c8e0): the RNC type-1 stream. One or more
 * outer passes reload the three tables, read the 16-bit block total
 * (LSB-first), then alternate literal runs (count from the literal table,
 * bytes copied straight from the input cursor) and back-reference matches
 * (out - u16(offset) - 1, length u16(sym + 2)); after every literal the
 * three bytes at the input cursor inject themselves into the LSB accumulator
 * at the live bit position without advancing it. It returns the output-end
 * pointer masked with 0xffff0000; its caller tests only AX. The output cursor
 * stores are blind, as in the corpus. */
// FUNCTION: LEGORR 0x0049c8e0
static int wad_payload_decode_type1(const void* src, void* dst)
{
    unsigned short total;
    unsigned int sym;
    unsigned short count;
    unsigned int i;
    unsigned int bits;
    unsigned int peek;
    const unsigned char* back;

    wad_bitstream_output_initialize(src, dst);
    (void)wad_read_bits_lsb_first(2u);            /* 0x49c8f2: value dropped */
    if (wad_codec_out >= wad_codec_end) {         /* 0x49c907: unsigned jae */
        return wad_codec_result_word(wad_codec_end);
    }
    for (;;) {                                    /* outer_loop 0x49c912 */
        wad_huffman_table_read(s_wad_huff_literal, 16u);
        wad_huffman_table_read(s_wad_huff_offset, 16u);
        wad_huffman_table_read(s_wad_huff_length, 16u);
        total = (unsigned short)wad_read_bits_lsb_first(16u); /* 0x49c949: edi */
        for (;;) {                                /* loopA 0x49c94b */
            sym = wad_huffman_symbol_decode(s_wad_huff_literal);
            count = (unsigned short)sym;          /* 0x49c965: u16(sym) */
            for (i = 0; i < count; ++i) {         /* copyA 0x49c972 */
                *wad_codec_out++ = *wad_codec_in++;
            }
            /* 0x49c99d: three-byte peek at the input cursor (never
             * advanced), merged into the LSB accumulator at the live bit
             * count, which the corpus leaves untouched. */
            bits = wad_codec_bits;
            peek = (unsigned int)wad_codec_in[0]
                 | ((unsigned int)wad_codec_in[1] << 8)
                 | ((unsigned int)wad_codec_in[2] << 16);
            wad_codec_lsb = (peek << bits)
                          | (wad_codec_lsb & ((1u << bits) - 1u));
            total += 0xffffu;                     /* 0x49c9cc: countdown */
            if (total == 0u) {                    /* 0x49c9d7: test di,di */
                break;
            }
            sym = wad_huffman_symbol_decode(s_wad_huff_offset);
            back = wad_codec_out - (unsigned short)sym - 1u; /* 0x49c9ec..fe */
            sym = wad_huffman_symbol_decode(s_wad_huff_length);
            count = (unsigned short)(sym + 2u);   /* 0x49ca07/0x49ca1e */
            for (i = 0; i < count; ++i) {         /* copyB 0x49ca24 */
                *wad_codec_out++ = *back++;
            }
        }
        if (wad_codec_out >= wad_codec_end) {     /* 0x49ca44: jb outer_loop */
            break;
        }
    }
    return wad_codec_result_word(wad_codec_end);   /* end & 0xffff0000 */
}

/* DecodeWadPayloadType2 (FUN_0049ca60): the RNC type-2 stream, MSB-first.
 * Each top-level bit opens a literal run (bytes copied from the input
 * cursor while the follow-up bits stay 0) or hands over to the match
 * gate: 0-run-length block, 10+10 short match (offset byte + 1, two bytes),
 * 110 match (offset from the back-reference decoder, three bytes) or 111
 * match (length byte + 9, offset from the decoder; a zero length byte is
 * the end marker). The target returns its local copy-source pointer masked
 * with 0xffff0000, or the output cursor at the end marker; callers test only
 * AX. The output cursor stores are blind, as in the corpus. */
// FUNCTION: LEGORR 0x0049ca60
static int wad_payload_decode_type2(const void* src, void* dst)
{
    unsigned int run;
    unsigned int off;
    unsigned int count;
    unsigned int i;
    const unsigned char* back;
    const unsigned char* result_pointer;

    wad_bitstream_output_initialize(src, dst);
    result_pointer = wad_codec_end;
    (void)wad_read_bits_msb_first(2u);            /* 0x49ca72: value dropped */
    if (wad_codec_out >= wad_codec_end) {         /* 0x49ca87: unsigned jae */
        return wad_codec_result_word(result_pointer);
    }
    for (;;) {                                    /* loop_top 0x49ca90 */
        if (wad_read_bits_msb_first(1u) == 0u) {  /* 0x49ca9a */
            /* 0x49ca9f: raw literal run, one byte per zero follow-up bit */
            for (;;) {
                *wad_codec_out++ = *wad_codec_in++;
                if (wad_read_bits_msb_first(1u) != 0u) {
                    break;
                }
            }
        }
        if (wad_read_bits_msb_first(1u) == 0u) {  /* 0x49cadf: raw block */
            run = (unsigned int)wad_run_length_decode();   /* 0x49cb8a */
            if ((unsigned short)run == 9u) {      /* 0x49cb91: extended run */
                count = (wad_read_bits_msb_first(4u) * 4u + 0xcu) & 0xffffu;
                for (i = 0; i < count; ++i) {     /* copyD 0x49cbbf */
                    *wad_codec_out++ = *wad_codec_in++;
                }
                if (wad_codec_out >= wad_codec_end) break;
                continue;
            }
            off = (unsigned int)wad_backreference_offset_decode(); /* 0x49cbf1 */
            back = wad_codec_out - (unsigned short)off; /* 0x49cbf9/0x49cc07 */
            result_pointer = back;
            if ((unsigned short)run == 0u) {      /* 0x49cc0f/0x49cc12 */
                continue;
            }
            count = (unsigned short)run;          /* 0x49cc18: u16(si) */
            for (i = 0; i < count; ++i) {         /* copyE 0x49cc1f */
                *wad_codec_out++ = *back++;
            }
            if (wad_codec_out >= wad_codec_end) break;
            continue;
        }
        if (wad_read_bits_msb_first(1u) == 0u) {  /* 0x49caf2: short match */
            off = (unsigned int)*wad_codec_in + 1u;        /* 0x49cb02 */
            ++wad_codec_in;
            back = wad_codec_out - (unsigned short)off;
            result_pointer = back;
            count = 2u;                           /* esi = 2 at 0x49caf9 */
            for (i = 0; i < count; ++i) {         /* copyC 0x49cb6d */
                *wad_codec_out++ = *back++;
            }
            if (wad_codec_out >= wad_codec_end) break;
            continue;
        }
        if (wad_read_bits_msb_first(1u) == 0u) {  /* 0x49cb10/0x49cb1d */
            off = (unsigned int)wad_backreference_offset_decode(); /* 0x49cb42 */
            back = wad_codec_out - (unsigned short)off;
            result_pointer = back;
            count = 3u;                           /* esi = 3 at 0x49cb1f */
        } else {
            count = ((unsigned int)*wad_codec_in + 8u) & 0xffffu; /* 0x49cb2b */
            ++wad_codec_in;
            if (count == 8u) {                    /* 0x49cb3c: end marker */
                (void)wad_read_bits_msb_first(1u);/* 0x49cc3f: value dropped */
                result_pointer = wad_codec_out;
                if (wad_codec_out < wad_codec_end) { /* 0x49cc56: jb loop_top */
                    continue;
                }
                break;                            /* falls into the exit */
            }
            off = (unsigned int)wad_backreference_offset_decode(); /* 0x49cb42 */
            back = wad_codec_out - (unsigned short)off;
            result_pointer = back;
        }
        if (count == 0u) {                        /* 0x49cb5d: test ax,ax */
            continue;
        }
        for (i = 0; i < count; ++i) {             /* copyC 0x49cb6d */
            *wad_codec_out++ = *back++;
        }
        if (wad_codec_out >= wad_codec_end) break;
    }
    return wad_codec_result_word(result_pointer);  /* pbVar4 & 0xffff0000 */
}

/* WadPayloadDecodeDispatch (FUN_0049c840): verify the "RNC" magic, then
 * branch on the type byte at [3]. Type 0 is a raw pass-through: the size
 * dword at +4 read big-endian, payload from +0x12, copied in DWORD chunks
 * plus the trailing (size & 3) bytes. Types 1/2 hand over to the RNC codecs;
 * any other byte answers the unknown-type verdict. */
// FUNCTION: LEGORR 0x0049c840
int wad_payload_decode_dispatch(const void* header, void* dest)
{
    if (header == NULL || dest == NULL) {
        /* deviation: the corpus strides both operands blindly */
        return WAD_DECODE_MAGIC_FAIL;
    }
    const unsigned char* h = (const unsigned char*)header;
    if (memcmp(h, "RNC", 3) != 0) {
        return WAD_DECODE_MAGIC_FAIL;
    }
    const unsigned char type = h[3];
    if (type == 0u) {
        /* (uVar >> 8 & 0xff + uVar * 0x100) * 0x100 + ... = big-endian read */
        const uint32_t size = ((uint32_t)h[4] << 24) | ((uint32_t)h[5] << 16)
                            | ((uint32_t)h[6] << 8) | (uint32_t)h[7];
        /* deviation: the corpus copies in dword+byte stages; the whole-span
         * memcpy is byte-identical and bounds-safe on both sides. */
        memcpy(dest, h + EXPAND_HEADER_SZ, size);
        return 0;
    }
    if (type == 1u) {
        return wad_payload_decode_type1(h, dest); /* 0x49c889: push/call */
    }
    if (type == 2u) {
        return wad_payload_decode_type2(h, dest); /* 0x49c878: push/call */
    }
    return WAD_DECODE_TYPE_FAIL;
}

/* ExpandWadPayload (FUN_0049c7c0): probe or materialize an expanded entry
 * payload. The size is the big-endian length dword at +4; a NULL out-pointer
 * is the corpus's size probe (the size alone answers). Otherwise the engine
 * heap allocates, the dispatch decodes, and a failed dispatch frees. */
// FUNCTION: LEGORR 0x0049c7c0
long wad_expand_payload(const void* header, void** out_payload)
{
    if (header == NULL) {
        return 0; /* deviation: the corpus strides the payload blindly */
    }
    const unsigned char* h = (const unsigned char*)header;
    /* corpus mov eax,[ebx+4]: one dword load, and the byteswap rides
     * arithmetically off the register so MSVC5 emits its and/shl/add
     * expansion rather than four movzx byte loads */
    const uint32_t raw = *(const uint32_t*)(h + 4);
    const uint32_t swapped = ((raw << 24) & 0xff000000u)
                           | ((raw << 8) & 0x00ff0000u)
                           | ((raw >> 8) & 0x0000ff00u) | (raw >> 24);
    const long size = (long)swapped;
    if (out_payload == NULL) {
        return size;
    }
    void* buf = engine_heap_alloc(size); /* FUN_0048dbd0 */
    if (buf == NULL) {
        return 0;
    }
    if (((uint32_t)wad_payload_decode_dispatch(h, buf) & 0xffffu) != 0u) {
        /* The retail caller tests AX, not the full EAX masked-pointer return. */
        engine_heap_free(buf); /* FUN_0048db80 */
        return 0;
    }
    *out_payload = buf;
    return size;
}

/* WadHuffmanTableClear (FUN_0049cf50): the count is an unsigned byte. Clear
 * each 16-byte entry as DWORD 0, WORD 0xffff, DWORD 0, WORD 0. */
// FUNCTION: LEGORR 0x0049cf50
void wad_huffman_table_clear(void* table, uint8_t count)
{
    uint32_t* entry = (uint32_t*)table;
    if (table == NULL || count == 0u) return;
    do {
        entry[0] = 0u;
        *(uint16_t*)(entry + 1) = 0xffffu;
        entry[2] = 0u;
        *(uint16_t*)(entry + 3) = 0u;
        entry += 4;
        --count;
    } while (count != 0u);
}

/* GameFileHandleAllocate (FUN_0047fed0): the file-layer handle pair. Kind 1
 * (wads) is the 8-byte record flagged 1 with no stream buffer; kind 0 (the
 * loose-file route) flags 0 and carries the 12-byte stream buffer behind,
 * releasing the record if the buffer alloc fails. Deviations: the record
 * fields widen to (int32_t flag; void* stream) for LP64 (the corpus's
 * dwords, 8 bytes, alias this record's front half) and the corpus's
 * blind-first store for kind 1 is guarded. The corpus's wrapper chain
 * 0x47ff30 is a bare call into the heap's alloc entry. */
// FUNCTION: LEGORR 0x0047fed0
void* game_file_handle_allocate(int kind)
{
    GameFileHandle* h;
    if (kind == 1) {
        h = (GameFileHandle*)engine_heap_alloc((long)sizeof(GameFileHandle));
        if (h == NULL) {
            return NULL;
        }
        h->flag = 1;
        /* Test-pinned deviation: unlike the corpus, the port leaves the
         * kind-1 stream slot deterministically NULL. */
        h->stream = NULL;
        return h;
    }
    if (kind != 0) {
        return NULL;
    }
    h = (GameFileHandle*)engine_heap_alloc((long)sizeof(GameFileHandle));
    if (h == NULL) {
        return NULL;
    }
    h->flag = 0;
    h->stream = engine_heap_alloc(12);
    if (h->stream != NULL) {
        return h;
    }
    engine_heap_free_wrapper(h);
    return NULL;
}

/* GameFileOpenDispatch (FUN_0047f6e0): resolve the path, select WAD versus
 * loose-file storage, then return the corresponding GameFileHandle. The WAD
 * stream owns a fixed resource slot plus a cursor record; loose streams keep
 * their CRT FILE handle directly. */
// FUNCTION: LEGORR 0x0047f6e0
GameFileHandle* game_file_open_dispatch(const char* path, const char* mode)
{
    const char* resolved;
    GameFileHandle* handle;
    int route;
    if (path == NULL || mode == NULL) return NULL;
    resolved = resource_path_resolve_against_base(path);
    if (resolved == NULL) return NULL;
    route = game_file_open_route_select(resolved, mode);
    if (route == 0) {
        char* wad_name = game_file_mounted_path_prefix_rewrite(resolved);
        Resource* resource = resource_open(wad_name, RESOURCE_WAD_ONLY);
        WadFileStreamState* state;
        int slot;
        if (resource == NULL) return NULL;
        slot = wad_entry_slot_allocate();
        if (slot < 0) {
            resource_free(resource);
            return NULL;
        }
        handle = (GameFileHandle*)game_file_handle_allocate(0);
        if (handle == NULL) {
            resource_free(resource);
            return NULL;
        }
        state = (WadFileStreamState*)handle->stream;
        state->slot = slot;
        state->cursor = 0;
        fixed_record_slot_install(slot, resource);
        return handle;
    }
    if (route != 1) {
        resource_debug_output_format(s_game_file_unknown_type_format,
            s_game_file_source_path, 0x14a, s_game_file_open_name);
        return NULL;
    }

    handle = (GameFileHandle*)game_file_handle_allocate(1);
    if (handle == NULL) return NULL;
    handle->stream = fopen(resolved, mode);
    /* The corpus gates its CD fallback on CaseInsensitiveSubstringFind(mode,
     * "w"), rather than checking the first byte. That distinction matters
     * for update modes such as "r+w": fopen may accept the path, but the
     * fallback must not redirect that writable stream to the data directory. */
    if (handle->stream == NULL &&
        case_insensitive_substring_find(mode, "w") == NULL) {
        char fallback[RESOURCE_PATH_MAX];
        if (ares_cd_fallback_path(path, fallback, sizeof(fallback)))
            handle->stream = fopen(fallback, mode);
    }
    if (handle->stream == NULL) {
        game_file_close(handle);
        return NULL;
    }
    return handle;
}

/* Narrow startup adapter retained for callers that explicitly require a
 * loose file (including output paths); the retail dispatch handles both
 * storage branches. */
GameFileHandle* game_file_open_loose_path(const char* path,
                                          const char* mode)
{
    const char* resolved;
    GameFileHandle* handle;
    FILE* stream;
    if (path == NULL || mode == NULL) return NULL;
    resolved = resource_path_resolve_against_base(path);
    if (resolved == NULL || game_file_open_route_select(resolved, mode) != 1)
        return NULL;
    handle = (GameFileHandle*)game_file_handle_allocate(1);
    if (handle == NULL) return NULL;
    stream = fopen(resolved, mode);
    if (stream == NULL) {
        game_file_close(handle);
        return NULL;
    }
    handle->stream = stream;
    return handle;
}

/* WadEntryReadIntoStreamSlot (FUN_0047fea0): zero the slot pair behind the
 * handle, resolve the entry through the corpus's wad path opener with the
 * search-all selector, and answer whether the entry exists. Deviations:
 * the opener's record-slot handle (its slot table is modeled in
 * fixed_record_slot_resource_get) rides as the opened resource pointer and
 * the corpus's -1 sentinel becomes NULL; the search-all index arrives as
 * the wad-only open mode with the corpus's global at -1 inlined. */
// FUNCTION: LEGORR 0x0047fea0
int wad_entry_read_into_stream_slot(WadStreamSlots* slots, const char* name)
{
    if (slots == NULL) {
        return 0; /* deviation: the corpus strides the slots blindly */
    }
    slots->word2 = 0u;
    slots->stream = NULL;
    const Resource* handle = resource_open(name, RESOURCE_WAD_ONLY);
    slots->handle = handle;
    return handle != NULL;
}

/* --- resource-handle release band (2026-10-25; corpus .data band) ---------
 * ResourceHandleSetRelease (FUN_0041a850) frees the corpus's global handle
 * set: the DAT_004ddc04 stride-8 record band (the first DWORD of each pair,
 * while the walker stays below DAT_004ddcc4) followed by twelve named
 * globals, freed in corpus order. Deviations: the band lands as
 * own storage (install/peek hooks for the named globals), the
 * corpus frees without clearing, and the port nulls each freed slot. */
// GLOBAL: LEGORR 0x004ddc04
static struct {
    void* handle;      /* corpus band dword (stride 8; the word behind rides) */
    uint32_t word2;
} s_handle_set_band[24]; /* DAT_004ddc04..DAT_004ddcc4, stride 8 */
static void* s_handle_set_named[12]; /* DAT_004dd734..9a8/DAT_004ddad4/d8 */

/* Port hook: slots 0..23 are the band (stride-8 units), 24..35 the named
 * globals in address order: 734, 738, 73c, 740, 744, 748, 99c, 9a0, 9a4,
 * 9a8, ad4, ad8. */
void resource_handle_set_slot_install(int slot, void* res)
{
    if (slot < 24) {
        s_handle_set_band[slot].handle = res;
    } else if (slot >= 24 && slot < 36) {
        s_handle_set_named[slot - 24] = res;
    }
}

void* resource_handle_set_slot_peek(int slot)
{
    if (slot < 24) {
        return s_handle_set_band[slot].handle;
    }
    if (slot >= 24 && slot < 36) {
        return s_handle_set_named[slot - 24];
    }
    return NULL;
}

// FUNCTION: LEGORR 0x0041a850
void resource_handle_set_release(void)
{
    for (int i = 0; i != 24; ++i) {
        /* corpus: stride the band, free the non-zero dwords under cc4 */
        if (s_handle_set_band[i].handle != NULL) {
            engine_heap_free(s_handle_set_band[i].handle); /* FUN_0048db80 */
            s_handle_set_band[i].handle = NULL; /* deviation: the corpus dangles */
        }
    }
    /* corpus free order: 734 73c 744 ad4 99c 9a4 738 740 748 ad8 9a0 9a8 */
    static const size_t order[12] = { 0, 2, 4, 10, 6, 8, 1, 3, 5, 11, 7, 9 };
    for (size_t k = 0; k != 12; ++k) {
        void** p = &s_handle_set_named[order[k]];
        if (*p != NULL) {
            engine_heap_free(*p);
            *p = NULL;
        }
    }
}

/* --- the key-group action-code band (see resource.hpp) ---------------------
 * InputKeyGroupRegister (FUN_0041a7d0) stages one interaction-action group:
 * the count word rides in at the pair's +4 slot FIRST (corpus order), then
 * a count*4 heap array takes the code dwords. The codes ride the stack as
 * the corpus's own cdecl varargs and are read out with va_arg in index
 * order. Deviations: the staging copy of the variadic window into locals
 * (and the always-zero byte tail) fold into a direct dword copy, and the
 * group guard answers where the corpus strides the band blindly.
 * Registering the same group twice re-stages in place; the corpus leaks the
 * superseded array, and so does the port. */
// FUNCTION: LEGORR 0x0041a7d0
void resource_input_key_group_register(int group, int count, ...)
{
    va_list ap;

    if (group < 0 || group >= 24) {
        return; /* deviation: the corpus strides the band blindly */
    }
    s_handle_set_band[group].word2 = (uint32_t)count;
    void* array = engine_heap_alloc((long)count * 4); /* FUN_0048dbd0 */
    s_handle_set_band[group].handle = array;
    va_start(ap, count);
    if (array != NULL) {
        uint32_t* dst = (uint32_t*)array;
        const uint32_t n = (uint32_t)count & 0x3fffffffu;
        for (uint32_t i = 0; i != n; ++i) {
            dst[i] = (uint32_t)va_arg(ap, int);
        }
    }
    va_end(ap);
}

/* InteractionActionCodeMapInitialize (FUN_0041a660): registers the fixed
 * eighteen interaction-action groups in the corpus's call order — groups
 * 0..5, 7, 8, then 0xa, 0xb, 0xc, with group 9 landing between 0xc and
 * 0xd, and group 6 left unregistered (the corpus skips it). The action
 * codes ride as the corpus's own pushed constants, one push per code. */
// FUNCTION: LEGORR 0x0041a660
void interaction_action_code_map_initialize(void)
{
    resource_input_key_group_register(0, 4, 1, 2, 3, 4);
    resource_input_key_group_register(1, 3, 5, 6, 0xe);
    resource_input_key_group_register(2, 1, 7);
    resource_input_key_group_register(3, 1, 0xe);
    resource_input_key_group_register(4, 1, 0x10);
    resource_input_key_group_register(5, 2, 9, 0xe);
    resource_input_key_group_register(7, 4, 0xb, 0xc, 0xd, 0xf);
    resource_input_key_group_register(8, 2, 0x2b, 0x2f);
    resource_input_key_group_register(0xa, 7, 0x16, 0x14, 0x42, 0x19,
                                      0x2d, 0x2e, 0x29);
    resource_input_key_group_register(0xb, 6, 0x16, 0x14, 0x19, 0x2d, 0x2e,
                                      0x1a);
    resource_input_key_group_register(0xc, 2, 0x18, 0x29);
    resource_input_key_group_register(9, 10, 0x1f, 0x17, 0x15, 0x30, 0x21,
                                      0x40, 0x39, 0x2d, 0x2e, 0x20);
    resource_input_key_group_register(0xd, 4, 0x24, 0x22, 0x41, 0x28);
    resource_input_key_group_register(0xe, 1, 0x2a);
    resource_input_key_group_register(0xf, 3, 0x2c, 0x2e, 0x2d);
    resource_input_key_group_register(0x10, 8, 0x31, 0x32, 0x33, 0x34, 0x35,
                                      0x36, 0x37, 0x38);
    resource_input_key_group_register(0x11, 6, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e,
                                      0x3f);
    resource_input_key_group_register(0x12, 4, 0x43, 0x44, 0x45, 0x46);
}

void resource_handle_set_band_word_install(int unit, uint32_t value)
{
    if (unit >= 0 && unit < 24) {
        s_handle_set_band[unit].word2 = value;
    }
}

uint32_t resource_handle_set_band_word_peek(int unit)
{
    return (unit >= 0 && unit < 24) ? s_handle_set_band[unit].word2 : 0u;
}

/* ResourceHandleListDispose (FUN_00410ae0): free the list's non-zero
 * handles (the count lives behind), then the array, then the record — the
 * record is freed even when its array word is zero. Deviations: the record
 * fields widen LP64-safe and the NULL record is guarded (the corpus strides
 * blindly). */
// FUNCTION: LEGORR 0x00410ae0
void resource_handle_list_dispose(ResourceHandleList* list)
{
    if (list == NULL) {
        return; /* deviation: the corpus derefs blindly */
    }
    if (list->handles != NULL) {
        for (int32_t i = 0; i < list->count; ++i) {
            if (list->handles[i] != NULL) {
                engine_heap_free(list->handles[i]); /* FUN_0048db80 */
            }
        }
        engine_heap_free(list->handles);
    }
    engine_heap_free(list);
}

/* ResourceHandleListAppend (FUN_00410aa0): append a heap-owned string only
 * while the preallocated handle array has capacity. */
// FUNCTION: LEGORR 0x00410aa0
void resource_handle_list_append(ResourceHandleList* list, const char* value)
{
    if (list == NULL || list->handles == NULL
        || list->count >= (int32_t)(uint32_t)list->word1 || value == NULL) {
        return;
    }
    const size_t length = strlen(value);
    if (length >= 0x7fffffffU) return;
    char* copy = (char*)engine_heap_alloc((long)length + 1);
    if (copy == NULL) return;
    strncpy(copy, value, length + 1);
    list->handles[list->count] = copy;
    ++list->count;
}

namespace unported_ledger_stubs {

static MiscObjectResourceHooks s_misc_resource_hooks;
static void* s_misc_resource_handles[22];

void misc_object_resource_hooks_set(const MiscObjectResourceHooks* hooks)
{
    if (hooks == NULL) memset(&s_misc_resource_hooks, 0,
                              sizeof(s_misc_resource_hooks));
    else s_misc_resource_hooks = *hooks;
}

void* misc_object_resource_peek(unsigned int slot)
{
    return slot < 22u ? s_misc_resource_handles[slot] : NULL;
}

typedef struct MiscResourceSpec {
    const char* name;
    int kind;
    int fallback_kind;
    int load_flag;
} MiscResourceSpec;

// FUNCTION: LEGORR 0x00422850
void misc_object_resources_load(void* config)
{
    static const MiscResourceSpec specs[22] = {
        { "Boulder", 1, 2, 1 },
        { "BoulderExplodeIce", 3, -1, 1 },
        { "BoulderExplode", 3, -1, 1 },
        { "SmashPath", 3, -1, 1 },
        { "Explosion", 5, -1, 1 },
        { "Crystal", 1, 2, 1 },
        { "Dynamite", 4, -1, 1 },
        { "OohScary", 4, -1, 1 },
        { "Barrier", 4, -1, 1 },
        { "Ore", 1, 2, 1 },
        { "ProcessedOre", 1, 2, 1 },
        { "ElectricFence", 1, -1, 1 },
        { "ElectricFenceStud", 3, -1, 1 },
        { "ShortElectricFenceBeam", 6, -1, 0 },
        { "LongElectricFenceBeam", 7, -1, 1 },
        { "SpiderWeb", 4, -1, 1 },
        { "RechargeSparkle", 3, -1, 1 },
        { "MiniTeleportUp", 3, -1, 0 },
        { "IceCube", 4, -1, 1 },
        { "Pusher", 3, -1, 1 },
        { "Freezer", 3, -1, 1 },
        { "LaserShot", 3, -1, 1 }
    };
    unsigned int i;
    memset(s_misc_resource_handles, 0, sizeof(s_misc_resource_handles));
    if (s_misc_resource_hooks.load != NULL) {
        for (i = 0u; i < 22u; ++i) {
            void* handle = s_misc_resource_hooks.load(config, "MiscObjects",
                specs[i].name, specs[i].kind, specs[i].load_flag,
                s_misc_resource_hooks.user);
            if (handle == NULL && specs[i].fallback_kind >= 0) {
                handle = s_misc_resource_hooks.load(config, "MiscObjects",
                    specs[i].name, specs[i].fallback_kind,
                    specs[i].load_flag, s_misc_resource_hooks.user);
            }
            s_misc_resource_handles[i] = handle;
            if (handle != NULL && s_misc_resource_hooks.enable != NULL)
                s_misc_resource_hooks.enable(handle,
                                             s_misc_resource_hooks.user);
        }
    }
    if (s_misc_resource_hooks.post_load != NULL)
        s_misc_resource_hooks.post_load(config, s_misc_resource_hooks.user);
}

typedef struct PortableResourceNameQuery {
    const char* name;
    void* resource;
    uint32_t match_count;
    uint32_t selected_index;
    int search_mode;
    int case_sensitive;
} PortableResourceNameQuery;

int resource_tree_callback_walk(ResourceTreeNode* root, int depth,
                                ResourceTreeNodeCallback callback,
                                void* context)
{
    uint32_t i;
    int result;
    if (root == NULL || callback == NULL) return 0;
    if (callback(root, context) != 0) return 1;
    for (i = 0u; i < root->child_count; ++i) {
        result = unported_ledger_stubs::resource_tree_callback_walk(
            root->children[i], depth + 1, callback, context);
        if (result != 0) return 1;
    }
    return 0;
}

} // namespace unported_ledger_stubs

/* Retail ResourceTreeCallbackWalk (FUN_00476330) operates on Direct3DRM
 * interfaces, not ResourceTreeNode. Keep this target entry separate from the
 * portable test/runtime tree API above. Its traversal mirrors the COM slots
 * and reference ownership visible in the retail body. */
// FUNCTION: LEGORR 0x00476330
int resource_tree_callback_walk_target(void* object, int depth,
                                       ResourceTreeCallback callback,
                                       void* context)
{
    void** object_vtable;
    ResourceTreeGetChildrenFn get_children;
    void* child_array;
    void** array_vtable;
    ResourceTreeArraySizeFn get_size;
    uint32_t child_count;
    uint32_t index;
    int found = 0;
    if (callback(object, context) != 0) return 1;

    object_vtable = *(void***)object;
    get_children = (ResourceTreeGetChildrenFn)object_vtable[0x4c / 4];
    child_array = NULL;
    if (get_children(object, &child_array) != 0) return 0;

    array_vtable = *(void***)child_array;
    get_size = (ResourceTreeArraySizeFn)array_vtable[0x0c / 4];
    child_count = get_size(child_array);
    for (index = 0u; index < child_count; ++index) {
        void* child;
        void* child_interface;
        void** child_vtable;
        void** current_array_vtable;
        ResourceTreeArrayElementFn get_element;
        ResourceTreeQueryInterfaceFn query_interface;
        ResourceTreeReferenceFn release_child;
        ResourceTreeReferenceFn release_interface;

        child = NULL;
        child_interface = NULL;
        current_array_vtable = *(void***)child_array;
        get_element = (ResourceTreeArrayElementFn)
            current_array_vtable[0x10 / 4];
        (void)get_element(child_array, index, &child);

        child_vtable = *(void***)child;
        query_interface = (ResourceTreeQueryInterfaceFn)child_vtable[0];
        (void)query_interface(child, s_resource_tree_child_iid,
                              &child_interface);
        release_child = (ResourceTreeReferenceFn)child_vtable[2];
        (void)release_child(child);

        if (resource_tree_callback_walk_target(child_interface, depth + 1,
                                               callback, context) != 0) {
            void** child_interface_vtable = *(void***)child_interface;
            release_interface = (ResourceTreeReferenceFn)
                child_interface_vtable[2];
            (void)release_interface(child_interface);
            found = 1;
            break;
        }
        {
            void** child_interface_vtable = *(void***)child_interface;
            release_interface = (ResourceTreeReferenceFn)
                child_interface_vtable[2];
            (void)release_interface(child_interface);
        }
    }
    {
        void** current_array_vtable = *(void***)child_array;
        ResourceTreeReferenceFn release_array =
            (ResourceTreeReferenceFn)current_array_vtable[2];
        (void)release_array(child_array);
    }
    return found;
}

static int resource_name_match_callback(ResourceTreeNode* node,
                                        void* context)
{
    unported_ledger_stubs::PortableResourceNameQuery* query =
        (unported_ledger_stubs::PortableResourceNameQuery*)context;
    const char* a;
    const char* b;
    int matched = 1;
    if (node == NULL || query == NULL || node->resource == NULL ||
        node->name == NULL ||
        query->name == NULL) return 0;
    if (strlen(node->name) != strlen(query->name)) return 0;
    a = node->name;
    b = query->name;
    while (*a != '\0' && *b != '\0') {
        int ca = (unsigned char)*a;
        int cb = (unsigned char)*b;
        if (!query->case_sensitive) {
            if (ca >= 'A' && ca <= 'Z') ca += 'a' - 'A';
            if (cb >= 'A' && cb <= 'Z') cb += 'a' - 'A';
        }
        if (ca != cb) {
            matched = 0;
            break;
        }
        ++a;
        ++b;
    }
    if (*a != '\0' || *b != '\0') matched = 0;
    if (!matched) return 0;
    if (query->search_mode == 1) {
        ++query->match_count;
        return 0;
    }
    if (query->search_mode == 2) {
        ++query->match_count;
        if (query->match_count != query->selected_index + 1u) return 0;
    }
    else if (query->search_mode != 0) return 0;
    query->resource = node->resource;
    return 1;
}

typedef uint32_t (RESOURCE_TREE_STDCALL *ResourceTreeGetNameFn)(
    void*, uint32_t*, char*);

static int resource_name_compare_bytes(const char* left, const char* right,
                                       int case_sensitive)
{
    while (*left != '\0' && *right != '\0') {
        unsigned char a = (unsigned char)*left;
        unsigned char b = (unsigned char)*right;
        if (case_sensitive == 0) {
            if (a >= 'A' && a <= 'Z') a = (unsigned char)(a + ('a' - 'A'));
            if (b >= 'A' && b <= 'Z') b = (unsigned char)(b + ('a' - 'A'));
        }
        if (a != b) return 0;
        ++left;
        ++right;
    }
    return *left == *right;
}

/* ResourceNameMatchCallback @ 0x00476410. The retail callback receives a
 * Direct3DRM object and the seven-word query record built by
 * ResourceNameRegisterOrQuery. Name retrieval uses the object's +0x24 slot;
 * the portable ResourceTreeNode predicate above remains for portable callers. */
// FUNCTION: LEGORR 0x00476410
int resource_name_match_callback_target(void* object, void* context)
{
    ResourceNameMatchCallbackContext* query =
        (ResourceNameMatchCallbackContext*)context;
    void** object_vtable;
    ResourceTreeGetNameFn get_name;
    uint32_t object_name_bytes = 0u;
    char* object_name;
    uint32_t mode;
    int equal;

    object_vtable = *(void***)object;
    get_name = (ResourceTreeGetNameFn)object_vtable[0x24 / 4];
    if (get_name(object, &object_name_bytes, NULL) != 0u ||
        object_name_bytes == 0u ||
        object_name_bytes - 1u != query->name_length) {
        return 0;
    }
    object_name = (char*)engine_heap_alloc((long)object_name_bytes);
    if (object_name == NULL) return 0;
    object_name[0] = '\0';
    if (get_name(object, &object_name_bytes, object_name) != 0u) {
        engine_heap_free(object_name);
        return 0;
    }

    mode = (uint32_t)query->mode;
    if (mode == 0u || mode == 2u) query->matched_object = NULL;
    equal = resource_name_compare_bytes(object_name, query->name,
                                        query->case_sensitive != 0u);
    if (equal != 0) {
        if (mode == 0u) {
            query->matched_object = object;
        } else if (mode == 1u) {
            ++query->match_count;
        } else if (mode == 2u) {
            ++query->match_count;
            if (query->match_count == query->selected_index + 1u) {
                query->matched_object = object;
            }
        }
    }
    engine_heap_free(object_name);
    return query->matched_object != NULL;
}

/* ResourceNameRegisterOrQuery (FUN_00473bc0) builds the seven-word callback
 * record, walks the interface root, and resolves the selected object through
 * SceneObjectOwnerLinkEnsure. The portable ResourceTreeNode helper below is
 * kept for host-side callers; this is the retail pointer-to-root ABI. */
// FUNCTION: LEGORR 0x00473bc0
void* resource_name_register_or_query_target(
    void** root, const char* name, int mode,
    uint32_t* match_count_or_index)
{
    const char* cursor = name;
    uint32_t remaining = 0xffffffffu;
    ResourceNameMatchCallbackContext query;

    do {
        char value;
        if (remaining == 0u) break;
        --remaining;
        value = *cursor;
        ++cursor;
        if (value == '\0') break;
    } while (1);

    query.name = name;
    query.name_length = ~remaining - 1u;
    query.case_sensitive = 0u;
    query.matched_object = NULL;
    query.match_count = 0u;
    query.mode = mode;
    query.selected_index = match_count_or_index != NULL
        ? *match_count_or_index : 0u;

    if (root != NULL && *root != NULL) {
        (void)resource_tree_callback_walk_target(*root, 0,
            resource_name_match_callback_target, &query);
    }

    if (mode == 0 || mode == 2) {
        return query.matched_object != NULL
            ? scene_object_owner_link_ensure(query.matched_object) : NULL;
    }
    if (mode == 1) {
        if (match_count_or_index != NULL)
            *match_count_or_index = query.match_count;
    }
    return NULL;
}

namespace unported_ledger_stubs {

void* resource_name_register_or_query(ResourceTreeNode* root,
                                      const char* name, int mode,
                                      uint32_t* match_count_or_index)
{
    PortableResourceNameQuery query;
    if (mode == 1 && match_count_or_index != NULL)
        *match_count_or_index = 0u;
    if (root == NULL || name == NULL ||
        (mode != 0 && mode != 1 && mode != 2)) return NULL;
    query.name = name;
    query.resource = NULL;
    query.match_count = 0u;
    query.selected_index = mode == 2 && match_count_or_index != NULL
        ? *match_count_or_index : 0u;
    query.search_mode = mode;
    query.case_sensitive = 0;
    (void)unported_ledger_stubs::resource_tree_callback_walk(root, 0,
        (ResourceTreeNodeCallback)resource_name_match_callback, &query);
    if (mode == 1) {
        if (match_count_or_index != NULL)
            *match_count_or_index = query.match_count;
        return NULL;
    }
    return query.resource != NULL
        ? scene_object_owner_link_ensure(query.resource) : NULL;
}


// ResourceNameRegisterOrQuery
void* ledger_stub_00473bc0_ResourceNameRegisterOrQuery(
    ResourceTreeNode* root, const char* name, int mode,
    uint32_t* match_count_or_index)
{
    return unported_ledger_stubs::resource_name_register_or_query(
        root, name, mode, match_count_or_index);
}

// ResourceTreeCallbackWalk
int ledger_stub_00476330_ResourceTreeCallbackWalk(
    ResourceTreeNode* root, int depth,
    ResourceTreeNodeCallback callback, void* context)
{
    return unported_ledger_stubs::resource_tree_callback_walk(
        root, depth, callback, context);
}

// NestedRecordListRemoveMatchingKey
typedef struct NestedResourceRecord {
    void* key;
    void* reserved[3];
    struct NestedResourceRecord* child_or_next;
} NestedResourceRecord;
// FUNCTION: LEGORR 0x0047b330
static int nested_record_list_remove_matching_key_impl(
    void* owner, void* key, void* opaque_record)
{
    NestedResourceRecord* record = (NestedResourceRecord*)opaque_record;
    NestedResourceRecord* child;
    if (record == NULL) return 0;
    child = record->child_or_next;
    if (nested_record_list_remove_matching_key_impl(owner, key, child)) {
        /* The retail call removes the matching key from DAT_00506684;
         * its owner argument is forwarded but ignored by that helper. */
        (void)owner;
        sound_buffer_association_remove_instance(key);
        record->child_or_next = child->child_or_next;
        engine_heap_free(child);
    }
    return record->key == key ? 1 : 0;
}


} // namespace unported_ledger_stubs

int resource_tree_callback_walk(ResourceTreeNode* root, int depth,
                                ResourceTreeNodeCallback callback,
                                void* context)
{
    return unported_ledger_stubs::resource_tree_callback_walk(
        root, depth, callback, context);
}

void* resource_name_register_or_query(ResourceTreeNode* root,
                                      const char* name, int mode,
                                      uint32_t* match_count_or_index)
{
    return unported_ledger_stubs::resource_name_register_or_query(
        root, name, mode, match_count_or_index);
}

int nested_record_list_remove_matching_key(void* owner, void* key,
                                           void* record)
{
    return unported_ledger_stubs::nested_record_list_remove_matching_key_impl(
        owner, key, record);
}
