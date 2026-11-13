// SPDX-License-Identifier: LGPL-3.0-or-later
#include <math.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "view.hpp"
#include "record.hpp"
#include "scene_resources.hpp"
#include "scene_pool.hpp"
#include "surface.hpp"
#include "sound.hpp"
#include "texture.hpp"
#include "heap.hpp"
#include "string.hpp"
#include "resource.hpp"
#include "crt/crt.hpp"
#include "model.hpp"
#include "../game/ai.hpp"
#include "../game/entity.hpp"
#include "../game/level.hpp"
#include "view_shared.hpp"
#include "view_for_tile_variants.hpp"
#include "view_for_camera_control.hpp"
#include "view_for_com_object_tree.hpp"
#include "view_for_viewports.hpp"
#include "view_for_lws.hpp"
#include "view_for_panel_unit.hpp"
#include "view_for_lego.hpp"
#include "view_for_images_unit.hpp"
#include "view_for_legoobject.hpp"
/* Main.c's fog method word (00505920) has C linkage, so that
 * container.c, compiled as C, reads it (finding 624). The frozen
 * header's later declaration keeps this linkage. */
extern "C" uint32_t s_scene_fog_mode;
#include "view_for_container_unit.hpp"
#include "view_for_mesh_unit.hpp"
#if defined(LEGORRR_BACKEND_MINIWIN)
#include "container_native.hpp"
#endif

/* Reconstruction-owned globals for corpus constants. */
float g_view_follow_scale = 1.0f;   /* port copy of cameraGlobs.cameraDropOff (005560a4, camera_control.c) */
int   g_view_grid_edge   = 0;       /* port copy of cameraGlobs.mouseScrollIndent (005560ac) */
int   g_view_test_blend  = 0;       /* test hook for the mode-3 EMA target */
ViewObjectLookupHook s_view_object_lookup_hook = NULL;
ViewTargetBlendHook s_view_target_blend_hook = NULL;
void* s_view_follow_target_context = NULL;

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
void view_follow_target_hooks_set(ViewObjectLookupHook lookup,
    ViewTargetBlendHook blend, void* context)
{
    s_view_object_lookup_hook = lookup;
    s_view_target_blend_hook = blend;
    s_view_follow_target_context = context;
}
#endif /* !LEGORRR_BACKEND_WINAPI */

/* CameraZoomStepAdjust reads the primary view/camera context directly from
 * the corpus's active-view global. Its owner is the unported config setup. */
// GLOBAL: LEGORR 0x005560e4
void* s_zoom_step_context;
TerrainPointPickCellCheckHook s_terrain_point_pick_hook;
void* s_terrain_point_pick_user;
ViewportPointDispatchHooks s_viewport_point_dispatch_hooks;
TexturedSceneDrawHooks s_textured_scene_draw_hooks;

SceneOwnerLinkReleaseFn s_owner_link_interface_release;
SceneOwnerLinkReleaseFn s_owner_link_child_release;
CameraViewContextReleaseHook s_view_context_release_hook;
CameraViewContextFreeHook s_view_context_free_hook;
void* s_view_context_close_context;
AnimationObjectTransformApplyHook s_animation_orientation_hook;
RenderNodePositionApplyHook s_render_node_position_hook;
void* s_animation_transform_context;
SceneMatrixApplyHook s_scene_matrix_apply_hook;
void* s_scene_matrix_apply_context;
SceneObjectChainHooks s_scene_object_chain_hooks;
SceneObjectDisposeHooks s_scene_object_dispose_hooks;
SceneObjectBoundsHooks s_scene_object_bounds_hooks;
RenderPackedRgbaHooks s_render_packed_rgba_hooks;
static RenderHierarchyFloat3Hooks s_render_hierarchy_float3_hooks;
static SceneActionNameLookupHook s_scene_action_name_lookup_hook;
static void* s_scene_action_name_lookup_context;
static SceneActionInterfaceGetHook s_scene_action_interface_get_hook;
static void* s_scene_action_interface_get_context;
static VideoAspectRatioQueryHook s_video_aspect_ratio_query_hook;
static void* s_video_aspect_ratio_query_context;
SceneContextHooks s_scene_context_hooks;
SceneActFrameScanHooks s_scene_act_frame_scan_hooks;
SceneActFrameRecordCloneHooks s_scene_act_frame_record_clone_hooks;
EntityVisualResourceLoadHooks s_entity_visual_resource_load_hooks;
DDiSceneAnimationHookRecordHooks s_ddiscene_animation_hook_record_hooks;
DDiSceneFrameRecordFindHooks s_ddiscene_frame_record_find_hooks;
static DDiSceneSampleHook s_ddiscene_sample_hook;
static void* s_ddiscene_sample_context;
int s_ddiscene_sample_enabled;
typedef struct DDiSceneSampleHookEntry {
    DDiSceneSampleHook callback;
    void* context;
} DDiSceneSampleHookEntry;
static DDiSceneSampleHookEntry s_ddiscene_additional_sample_hooks[8];
static unsigned int s_ddiscene_additional_sample_hook_count;
DDiSceneEndHook s_ddiscene_end_hook;
void* s_ddiscene_end_context;

/* Keep the portable callback fanout behind one callback-shaped dispatch.
 * The reference draw step itself invokes one registered callback. */
void ddiscene_sample_hook_dispatch(const char* sample_name,
                                          void* scene_object)
{
    unsigned int hook_index;
    if (s_ddiscene_sample_hook != NULL) {
        s_ddiscene_sample_hook(sample_name, scene_object,
                               s_ddiscene_sample_context);
    }
    for (hook_index = 0;
         hook_index < s_ddiscene_additional_sample_hook_count;
         ++hook_index) {
        DDiSceneSampleHookEntry* entry =
            &s_ddiscene_additional_sample_hooks[hook_index];
        if (entry->callback == s_ddiscene_sample_hook &&
            entry->context == s_ddiscene_sample_context) continue;
        entry->callback(sample_name, scene_object, entry->context);
    }
}
void ddiscene_activity_sample_dispatch(const char* sample, void* scene_object)
{
    if (sample != NULL && scene_object != NULL && s_ddiscene_sample_enabled)
        ddiscene_sample_hook_dispatch(sample, scene_object);
}
void ddiscene_activity_trigger_dispatch(void* scene_object)
{
    if (scene_object != NULL && s_ddiscene_end_hook != NULL)
        s_ddiscene_end_hook(scene_object, s_ddiscene_end_context);
}
static DDiSceneLightCreateHooks s_ddiscene_light_create_hooks;
static DDiSceneKindModeSetupHooks s_ddiscene_kind_mode_setup_hooks;
static DDiSceneDrawBeginHook s_ddiscene_draw_begin_hook;
static void* s_ddiscene_draw_begin_context;
static SceneHierarchyRegisterHooks s_scene_hierarchy_register_hooks;
DDiSceneObjectDestroyHook s_ddiscene_object_destroy_hook;
void* s_ddiscene_object_destroy_context;
SceneChildSearchHooks s_scene_child_search_hooks;
SceneFormattedStringHooks s_scene_formatted_string_hooks;
RenderInterfacePairHooks s_render_interface_pair_hooks;
DDiScenePayloadReleaseHooks s_ddiscene_payload_release_hooks;
RendererRecordCloneHooks s_renderer_record_clone_hooks;
static char* s_shared_object_directory_root;
SharedObjectSoundEnabledFn s_shared_object_sound_enabled;
SharedObjectSoundSubmitFn s_shared_object_sound_submit;
void* s_scene_material_manager;
SceneEntryExpandOps s_scene_entry_expand_ops;
SharedObjectCloneOps s_shared_object_clone_ops;
QuadMeshRenderHooks s_quad_mesh_render_hooks;
SinePhaseBlendState s_sine_phase_blend_state;
SinePhaseBlendHooks s_sine_phase_blend_hooks;
SceneFrameApplyHooks s_scene_frame_apply_hooks;
SceneRotationApplyHooks s_scene_rotation_apply_hooks;
SceneObjectRecordRetainHook s_scene_object_record_retain_hook;
void* s_scene_object_record_retain_context;
static RenderQualityApplyHook s_render_quality_apply_hook;
static void* s_render_quality_apply_context;

#if defined(_MSC_VER)
typedef void (__stdcall *DDiScenePayloadReleaseMethod)(void*);
#elif defined(__i386__) && (defined(__GNUC__) || defined(__clang__))
typedef void (__attribute__((stdcall)) *DDiScenePayloadReleaseMethod)(void*);
#else
typedef void (*DDiScenePayloadReleaseMethod)(void*);
#endif

void ddiscene_payload_interface_release(void* interface_object)
{
    void** vtable;
    if (interface_object == NULL) return;
    vtable = *(void***)interface_object;
    if (vtable != NULL && vtable[2] != NULL) {
        ((DDiScenePayloadReleaseMethod)vtable[2])(interface_object);
    }
}

void scene_object_bounds_hooks_set(const SceneObjectBoundsHooks* hooks)
{
    if (hooks == NULL) {
        memset(&s_scene_object_bounds_hooks, 0,
               sizeof(s_scene_object_bounds_hooks));
    }
    else {
        s_scene_object_bounds_hooks = *hooks;
    }
}

void render_packed_rgba_hooks_set(const RenderPackedRgbaHooks* hooks)
{
    if (hooks == NULL) memset(&s_render_packed_rgba_hooks, 0,
                              sizeof(s_render_packed_rgba_hooks));
    else s_render_packed_rgba_hooks = *hooks;
}

void render_hierarchy_float3_hooks_set(
    const RenderHierarchyFloat3Hooks* hooks)
{
    if (hooks == NULL) memset(&s_render_hierarchy_float3_hooks, 0,
                              sizeof(s_render_hierarchy_float3_hooks));
    else s_render_hierarchy_float3_hooks = *hooks;
}

/* RenderObjectHierarchyFloat3Set (FUN_00474e90): write the mesh-backed
 * record directly, or dispatch to the scene interface and each child. */
uint32_t render_object_hierarchy_float3_set(void* scene_object,
                                            const float xyz[3])
{
    RenderHierarchyFloat3Hooks* hooks = &s_render_hierarchy_float3_hooks;
    void* mesh;
    void* mesh_record;
    void* scene_interface;
    const void* children;
    uint32_t count = 0;
    uint32_t i;
    if (scene_object == NULL || xyz == NULL) return 0;
    memcpy(&mesh, (char*)scene_object + 0x0c, sizeof(mesh));
    if (mesh == NULL) return 0;
    memcpy(&mesh_record, (char*)mesh + 0x0c, sizeof(mesh_record));
    if (mesh_record != NULL) {
        /* FUN_00474e90 calls FUN_00482370 directly for the mesh-record
         * branch; scale the stored vertex positions in place. */
        mesh_vertices_scale_apply(mesh_record, xyz[0], xyz[1], xyz[2]);
        return 0;
    }
    memcpy(&scene_interface, (char*)mesh + 8, sizeof(scene_interface));
    if (scene_interface == NULL) return 0;
    if (hooks->scene_interface_set != NULL) {
        hooks->scene_interface_set(scene_interface, xyz, hooks->context);
    }
    if (hooks->children_get == NULL || hooks->child_get == NULL ||
        hooks->child_interface_set == NULL) return 0;
    children = hooks->children_get(scene_interface, &count, hooks->context);
    if (children == NULL) return 0;
    for (i = 0; i < count; ++i) {
        void* child = hooks->child_get(children, i, hooks->context);
        if (child != NULL) {
            hooks->child_interface_set(child, xyz, hooks->context);
        }
    }
    return 0;
}

void scene_action_name_lookup_hook_set(SceneActionNameLookupHook hook,
                                       void* context)
{
    s_scene_action_name_lookup_hook = hook;
    s_scene_action_name_lookup_context = context;
}

void* ddiscene_action_name_record_find(void* scene_object,
                                              const char* action_name,
                                              unsigned int selector)
{
    void* action_interface;
    if (scene_object == NULL || action_name == NULL || selector > 1u)
        return NULL;
    if (s_scene_action_interface_get_hook != NULL) {
        action_interface = s_scene_action_interface_get_hook(
            scene_object, (void*)action_name, selector,
            s_scene_action_interface_get_context);
    }
    else {
        memcpy(&action_interface,
               (char*)scene_object + (selector != 0u ? 8u : 4u),
               sizeof(action_interface));
    }
    if (action_interface == NULL) return NULL;
    return ddiscene_frame_record_find(action_interface, action_name,
                                      (int)selector);
}

/* SceneActionNamePresent (FUN_00473340): only kind-4 scene objects expose
 * the action-name lookup path. The backend callback models the scene's
 * action search while preserving that type gate and boolean result. */
uint32_t scene_action_name_present(void* scene_object, void* action_name)
{
    if (scene_object == NULL ||
        *(uint32_t*)((char*)scene_object + 0x10) != 4u) {
        return 0;
    }
    if (s_scene_action_name_lookup_hook != NULL) {
        return s_scene_action_name_lookup_hook(
            scene_object, action_name,
            s_scene_action_name_lookup_context) != 0;
    }
    return ddiscene_action_name_record_find(
        scene_object, (const char*)action_name, 1u) != NULL;
}

void scene_act_frame_scan_hooks_set(const SceneActFrameScanHooks* hooks)
{
    if (hooks == NULL) memset(&s_scene_act_frame_scan_hooks, 0,
                              sizeof(s_scene_act_frame_scan_hooks));
    else s_scene_act_frame_scan_hooks = *hooks;
}

void scene_act_frame_record_clone_hooks_set(
    const SceneActFrameRecordCloneHooks* hooks)
{
    if (hooks == NULL)
        memset(&s_scene_act_frame_record_clone_hooks, 0,
               sizeof(s_scene_act_frame_record_clone_hooks));
    else s_scene_act_frame_record_clone_hooks = *hooks;
}

void entity_visual_resource_load_hooks_set(
    const EntityVisualResourceLoadHooks* hooks)
{
    if (hooks == NULL) memset(&s_entity_visual_resource_load_hooks, 0,
                              sizeof(s_entity_visual_resource_load_hooks));
    else s_entity_visual_resource_load_hooks = *hooks;
}

void scene_action_interface_get_hook_set(SceneActionInterfaceGetHook hook,
                                         void* context)
{
    s_scene_action_interface_get_hook = hook;
    s_scene_action_interface_get_context = context;
}

void video_aspect_ratio_query_hook_set(VideoAspectRatioQueryHook hook,
                                       void* context)
{
    s_video_aspect_ratio_query_hook = hook;
    s_video_aspect_ratio_query_context = context;
}

float video_object_aspect_ratio_get(void* video_object)
{
    if (video_object == NULL || s_video_aspect_ratio_query_hook == NULL) {
        return 0.0f;
    }
    return s_video_aspect_ratio_query_hook(video_object,
                                           s_video_aspect_ratio_query_context);
}

void scene_context_hooks_set(const SceneContextHooks* hooks)
{
    if (hooks == NULL) {
        memset(&s_scene_context_hooks, 0, sizeof(s_scene_context_hooks));
    }
    else {
        s_scene_context_hooks = *hooks;
    }
}

/* SceneObjectOwnerLinkEnsure port hook. The original FUN_00475880 performs
 * property lookup and DDiSceneObject allocation, which is not modeled here. */
void* scene_object_owner_link_ensure(void* owner_interface)
{
    if (owner_interface == NULL ||
        s_scene_context_hooks.owner_link_ensure == NULL) {
        return NULL;
    }
    return s_scene_context_hooks.owner_link_ensure(
        owner_interface, s_scene_context_hooks.context);
}

void ddiscene_animation_hook_record_hooks_set(
    const DDiSceneAnimationHookRecordHooks* hooks)
{
    if (hooks == NULL) {
        memset(&s_ddiscene_animation_hook_record_hooks, 0,
               sizeof(s_ddiscene_animation_hook_record_hooks));
    }
    else {
        s_ddiscene_animation_hook_record_hooks = *hooks;
    }
}

/* The checked-in FUN_00475f70 body obtains the record through interface
 * slot +0x1c and installs a new record through +0x18. These are renderer-owned
 * stores; keep their calls on the supplied interface instead of a local map. */
DDiSceneAnimationHookRecord* ddiscene_animation_hook_record_get(
    void* scene_interface)
{
    void** vtable;
    typedef void* (VIEW_COM_CALL *GetProperty)(void*);
    GetProperty get_property;
    if (scene_interface == NULL) return NULL;
    vtable = *static_cast<void***>(scene_interface);
    if (vtable == NULL || vtable[0x1c / 4] == NULL) return NULL;
    get_property = reinterpret_cast<GetProperty>(vtable[0x1c / 4]);
    return static_cast<DDiSceneAnimationHookRecord*>(
        get_property(scene_interface));
}

int ddiscene_animation_hook_record_set(
    void* scene_interface, DDiSceneAnimationHookRecord* record)
{
    void** vtable;
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
    typedef long (VIEW_COM_CALL *SetProperty)(void*, void*);
#else
    typedef void (VIEW_COM_CALL *SetProperty)(void*, void*);
#endif
    SetProperty set_property;
    if (scene_interface == NULL) return 0;
    vtable = *static_cast<void***>(scene_interface);
    if (vtable == NULL || vtable[0x18 / 4] == NULL) return 0;
    set_property = reinterpret_cast<SetProperty>(vtable[0x18 / 4]);
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
    return set_property(scene_interface, record) == 0;
#else
    set_property(scene_interface, record);
    return 1;
#endif
}

namespace unported_ledger_stubs {
void* ledger_stub_00475e40_DDiSceneFrameRecordFind(
    void* action_interface, const char* action_name, int selector);
}

void ddiscene_frame_record_find_hooks_set(
    const DDiSceneFrameRecordFindHooks* hooks)
{
    if (hooks == NULL) {
        memset(&s_ddiscene_frame_record_find_hooks, 0,
               sizeof(s_ddiscene_frame_record_find_hooks));
    }
    else {
        s_ddiscene_frame_record_find_hooks = *hooks;
    }
}

void ddiscene_light_create_hooks_set(const DDiSceneLightCreateHooks* hooks)
{
    if (hooks == NULL) {
        memset(&s_ddiscene_light_create_hooks, 0,
               sizeof(s_ddiscene_light_create_hooks));
    }
    else {
        s_ddiscene_light_create_hooks = *hooks;
    }
}

/* container.c (Container.c, finding 624) serves these port names in the
 * matching build (container_link_names.h): every build but MSVC 5.0's
 * compiles the port's definitions. */
#if defined(_MSC_VER) && _MSC_VER < 1200
#define LRR_CONTAINER_PORT_NAMES 0
#else
#define LRR_CONTAINER_PORT_NAMES 1
#endif

/* DDiSceneLightCreate (FUN_004734f0): create the scene wrapper as kind 5,
 * ask the renderer to create/configure the light, and attach its animation
 * hook only after the renderer accepted the light. */
#if LRR_CONTAINER_PORT_NAMES /* container.c serves ddiscene_light_create in the matching build */
void* ddiscene_light_create(void* owner, uint32_t value_a,
                            uint32_t value_b, uint32_t value_c,
                            uint32_t value_d)
{
    void* record;
    void* light_interface = NULL;
    DDiSceneLightCreateHooks* const hooks = &s_ddiscene_light_create_hooks;
    if (hooks->record_create == NULL || hooks->record_kind_set == NULL ||
        hooks->light_create == NULL) return NULL;
    record = hooks->record_create(owner, hooks->context);
    if (record == NULL) return NULL;
    hooks->record_kind_set(record, 5u, hooks->context);
    if (hooks->light_create(record, value_a, value_b, value_c, value_d,
                            &light_interface, hooks->context) == 0) {
        if (hooks->light_configure != NULL)
            hooks->light_configure(record, value_b, hooks->context);
        if (hooks->animation_hook_set != NULL)
            hooks->animation_hook_set(record, light_interface,
                                      hooks->context);
    }
    return record;
}
#endif /* LRR_CONTAINER_PORT_NAMES */

void ddiscene_kind_mode_setup_hooks_set(
    const DDiSceneKindModeSetupHooks* hooks)
{
    if (hooks == NULL) {
        memset(&s_ddiscene_kind_mode_setup_hooks, 0,
               sizeof(s_ddiscene_kind_mode_setup_hooks));
    }
    else {
        s_ddiscene_kind_mode_setup_hooks = *hooks;
    }
}

/* DDiSceneObjectKindModeSetup (FUN_00473560): create kind-one scene state;
 * modes 2/3/4 map to renderer resources 0x2000/0x3000/0x2800, while other
 * modes enter the legacy interface path. */
void* ddiscene_object_kind_mode_setup(void* owner, int mode)
{
    unsigned int resource_kind;
    void* record;
    void* resource;
    DDiSceneKindModeSetupHooks* const hooks =
        &s_ddiscene_kind_mode_setup_hooks;
    if (hooks->record_create == NULL || hooks->record_kind_set == NULL)
        return NULL;
    record = hooks->record_create(owner, hooks->context);
    if (record == NULL) return NULL;
    hooks->record_kind_set(record, 1u, hooks->context);
    if (mode == 2) resource_kind = 0x2000u;
    else if (mode == 3) resource_kind = 0x3000u;
    else if (mode == 4) resource_kind = 0x2800u;
    else {
        if (hooks->legacy_mode_setup != NULL)
            hooks->legacy_mode_setup(record, mode, hooks->context);
        return record;
    }
    if (hooks->mode_resource_create != NULL &&
        hooks->animation_resource_set != NULL) {
        resource = hooks->mode_resource_create(record, resource_kind,
                                               hooks->context);
        hooks->animation_resource_set(record, resource, hooks->context);
    }
    return record;
}

void ddiscene_draw_begin_hook_set(DDiSceneDrawBeginHook hook, void* context)
{
    s_ddiscene_draw_begin_hook = hook;
    s_ddiscene_draw_begin_context = context;
}

/* DDiSceneDrawBegin (FUN_00477010): the device's draw-state/query sequence
 * is owned by the installed renderer interface. */
void ddiscene_draw_begin(void* render_object, int begin)
{
    if (render_object == NULL || s_ddiscene_draw_begin_hook == NULL) return;
    s_ddiscene_draw_begin_hook(render_object, begin,
                               s_ddiscene_draw_begin_context);
}

void* com_resource_enumeration_result_get(void* resource_owner)
{
    return com_property_result_get(resource_owner);
}

void scene_hierarchy_register_hooks_set(
    const SceneHierarchyRegisterHooks* hooks)
{
    if (hooks == NULL) {
        memset(&s_scene_hierarchy_register_hooks, 0,
               sizeof(s_scene_hierarchy_register_hooks));
    }
    else {
        s_scene_hierarchy_register_hooks = *hooks;
    }
}

static void scene_hierarchy_register_node(unsigned short* next_index,
                                          void* root, void* node,
                                          unsigned int depth)
{
    SceneHierarchyNodeInfo info;
    SceneHierarchyRegisterHooks* const hooks =
        &s_scene_hierarchy_register_hooks;
    char name[1028];
    char digits[10];
    size_t i;
    size_t name_length;
    unsigned int digit_count;
    unsigned int node_number;
    void* child;
    if (next_index == NULL || node == NULL || depth > 128u ||
        hooks->node_inspect == NULL) return;
    memset(&info, 0, sizeof(info));
    if (!hooks->node_inspect(node, &info, hooks->context)) return;
    if (info.scene_interface != NULL) {
        if ((info.flags & 4u) != 0u &&
            hooks->identity_transform_set != NULL) {
            hooks->identity_transform_set(info.scene_interface,
                                           hooks->context);
        }
        if (hooks->interface_store != NULL) {
            hooks->interface_store(root, *next_index, info.scene_interface,
                                   hooks->context);
        }
        if (hooks->node_index_set != NULL) {
            hooks->node_index_set(node, *next_index, hooks->context);
        }
        if (*next_index != 0xffffu) ++*next_index;
        if (info.display_name != NULL && hooks->interface_name_set != NULL) {
            name_length = strlen(info.display_name);
            if (name_length > sizeof(name) - 8u)
                name_length = sizeof(name) - 8u;
            memcpy(name, info.display_name, name_length);
            name[name_length++] = '_';
            node_number = (unsigned int)info.node_id;
            digit_count = 0u;
            do {
                digits[digit_count++] = (char)('0' + node_number % 10u);
                node_number /= 10u;
            } while (node_number != 0u && digit_count < sizeof(digits));
            if (digit_count == 1u) digits[digit_count++] = '0';
            while (digit_count != 0u && name_length < sizeof(name) - 1u)
                name[name_length++] = digits[--digit_count];
            name[name_length] = '\0';
            for (i = 0; name[i] != '\0'; ++i) {
                if (!isalnum((unsigned char)name[i]) && name[i] != '_')
                    name[i] = '_';
            }
            hooks->interface_name_set(info.scene_interface, name,
                                      hooks->context);
        }
    }
    child = info.first_child;
    while (child != NULL) {
        scene_hierarchy_register_node(next_index, root, child, depth + 1u);
        if (hooks->next_sibling == NULL) break;
        child = hooks->next_sibling(child, hooks->context);
    }
}

/* SceneHierarchyInterfaceRegisterRecursive (FUN_004887c0): register each
 * node interface in depth-first order, apply identity transforms to flagged
 * nodes, assign sequential ids, and publish sanitized interface names. */
void scene_hierarchy_interface_register_recursive(unsigned short* next_index,
                                                  void* root, void* node)
{
    scene_hierarchy_register_node(next_index, root, node, 0u);
}

int shared_object_directory_root_set(const char* directory)
{
    char* path = NULL;
    if (directory != NULL) {
        size_t length = strlen(directory);
        int has_separator = length != 0u && directory[length - 1u] == '\\';
        size_t output_length = length + (has_separator ? 0u : 1u);
        path = (char*)engine_heap_alloc((long)(output_length + 1u));
        if (path != NULL) {
            memcpy(path, directory, length);
            if (!has_separator) path[length] = '\\';
            path[output_length] = '\0';
        }
    }
    /* The model resolver borrows this owned prefix. Revoke that borrow
     * before freeing/replacing it, including failed reconfiguration. */
    model_resource_fallback_prefix_set(NULL);
    if (s_shared_object_directory_root != NULL)
        engine_heap_free(s_shared_object_directory_root);
    s_shared_object_directory_root = path;
    model_resource_fallback_prefix_set(path);
    return directory == NULL || path != NULL;
}

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
const char* shared_object_directory_root_get(void)
{
    return s_shared_object_directory_root;
}
#endif /* !LEGORRR_BACKEND_WINAPI */

void shared_object_sound_callbacks_get(
    SharedObjectSoundEnabledFn* enabled_out,
    SharedObjectSoundSubmitFn* submit_out)
{
    if (enabled_out != NULL) *enabled_out = s_shared_object_sound_enabled;
    if (submit_out != NULL) *submit_out = s_shared_object_sound_submit;
}

void scene_material_manager_set(void* manager)
{
    s_scene_material_manager = manager;
}

void scene_entry_expand_ops_set(const SceneEntryExpandOps* ops)
{
    if (ops == NULL) memset(&s_scene_entry_expand_ops, 0,
                            sizeof(s_scene_entry_expand_ops));
    else s_scene_entry_expand_ops = *ops;
}

void shared_object_clone_ops_set(const SharedObjectCloneOps* ops)
{
    if (ops == NULL) memset(&s_shared_object_clone_ops, 0,
                            sizeof(s_shared_object_clone_ops));
    else s_shared_object_clone_ops = *ops;
}

void ddiscene_object_destroy_hook_set(DDiSceneObjectDestroyHook hook,
                                      void* context)
{
    s_ddiscene_object_destroy_hook = hook;
    s_ddiscene_object_destroy_context = context;
}

/* DDiSceneObjectDestroy (FUN_00472a40) delegates to the mode-bearing
 * destructor with mode zero. */
void ddiscene_object_destroy(void* scene_object)
{
    ddiscene_object_destroy_with_mode(scene_object, 0);
}

static void* ddiscene_object_timing_interface_get(void* scene_object)
{
    uint32_t kind;
    if (scene_object == NULL) return NULL;
    kind = *(uint32_t*)((char*)scene_object + 0x10);
    if (kind == 3u) return *(void**)((char*)scene_object + 4);
    if (kind == 4u && s_scene_action_interface_get_hook != NULL) {
        void* action_slot = *(void**)((char*)scene_object + 0x0c);
        void* action_name = action_slot != NULL ? *(void**)action_slot : NULL;
        return s_scene_action_interface_get_hook(
            scene_object, action_name, 0,
            s_scene_action_interface_get_context);
    }
    return NULL;
}

static void* ddiscene_object_frame_record_get(void* scene_object)
{
    void* action_slot;
    const char* action_name;
    void* channel;
    if (scene_object == NULL) return NULL;
    if (*(uint32_t*)((char*)scene_object + 0x10) == 3u)
        return *(void**)((char*)scene_object + 4);
    if (*(uint32_t*)((char*)scene_object + 0x10) != 4u) return NULL;
    action_slot = *(void**)((char*)scene_object + 0x0c);
    action_name = action_slot != NULL ? *(const char**)action_slot : NULL;
    channel = *(void**)((char*)scene_object + 4);
    return channel != NULL && action_name != NULL
        ? ddiscene_frame_record_find(channel, action_name, 0) : NULL;
}

void ddiscene_payload_release_hooks_set(
    const DDiScenePayloadReleaseHooks* hooks)
{
    if (hooks == NULL) memset(&s_ddiscene_payload_release_hooks, 0,
                              sizeof(s_ddiscene_payload_release_hooks));
    else s_ddiscene_payload_release_hooks = *hooks;
}

void ddiscene_frame_callbacks_set(DDiSceneSampleHook sample_hook,
                                  void* sample_context, int sample_enabled,
                                  DDiSceneEndHook end_hook,
                                  void* end_context)
{
    s_ddiscene_sample_hook = sample_hook;
    s_ddiscene_sample_context = sample_context;
    s_ddiscene_sample_enabled = sample_enabled != 0;
    s_ddiscene_end_hook = end_hook;
    s_ddiscene_end_context = end_context;
}

int ddiscene_sample_hook_add(DDiSceneSampleHook sample_hook,
                             void* sample_context)
{
    unsigned int index;
    if (sample_hook == NULL) return 0;
    for (index = 0; index < s_ddiscene_additional_sample_hook_count; ++index) {
        if (s_ddiscene_additional_sample_hooks[index].callback == sample_hook &&
            s_ddiscene_additional_sample_hooks[index].context == sample_context) {
            s_ddiscene_sample_enabled = 1;
            return 1;
        }
    }
    if (s_ddiscene_additional_sample_hook_count >=
        sizeof(s_ddiscene_additional_sample_hooks) /
        sizeof(s_ddiscene_additional_sample_hooks[0])) return 0;
    s_ddiscene_additional_sample_hooks[
        s_ddiscene_additional_sample_hook_count].callback = sample_hook;
    s_ddiscene_additional_sample_hooks[
        s_ddiscene_additional_sample_hook_count].context = sample_context;
    ++s_ddiscene_additional_sample_hook_count;
    s_ddiscene_sample_enabled = 1;
    return 1;
}

float ddiscene_advance_by_time(void* scene_object, float delta)
{
    return scene_object != NULL
        ? ddiscene_advance_by_time_target(scene_object, delta) : 0.0f;
}

void scene_child_search_hooks_set(const SceneChildSearchHooks* hooks)
{
    if (hooks == NULL) {
        memset(&s_scene_child_search_hooks, 0,
               sizeof(s_scene_child_search_hooks));
    }
    else {
        s_scene_child_search_hooks = *hooks;
    }
}

void scene_formatted_string_hooks_set(const SceneFormattedStringHooks* hooks)
{
    if (hooks == NULL) {
        memset(&s_scene_formatted_string_hooks, 0,
               sizeof(s_scene_formatted_string_hooks));
    }
    else {
        s_scene_formatted_string_hooks = *hooks;
    }
}

void render_interface_pair_hooks_set(const RenderInterfacePairHooks* hooks)
{
    if (hooks == NULL) memset(&s_render_interface_pair_hooks, 0,
                              sizeof(s_render_interface_pair_hooks));
    else s_render_interface_pair_hooks = *hooks;
}

static int render_quality_string_equals(const char* left, const char* right)
{
    unsigned char a, b;
    if (left == NULL || right == NULL) return 0;
    do {
        a = (unsigned char)*left++;
        b = (unsigned char)*right++;
        if (a >= 'A' && a <= 'Z') a = (unsigned char)(a + ('a' - 'A'));
        if (b >= 'A' && b <= 'Z') b = (unsigned char)(b + ('a' - 'A'));
        if (a != b) return 0;
    } while (a != 0);
    return 1;
}

void render_quality_apply_hook_set(RenderQualityApplyHook hook,
                                   void* context)
{
    s_render_quality_apply_hook = hook;
    s_render_quality_apply_context = context;
}

int render_quality_config_apply(const RenderQualitySettings* settings)
{
    int quality;
    if (settings == NULL || settings->fp_clip_blocks == 0.0f) return 0;
    if (render_quality_string_equals(settings->quality, "Gouraud")) quality = 3;
    else if (render_quality_string_equals(settings->quality, "Flat")) quality = 2;
    else if (render_quality_string_equals(settings->quality, "Wireframe"))
        quality = 0;
    else return 0;
    if (s_render_quality_apply_hook != NULL) {
        s_render_quality_apply_hook(s_render_quality_apply_context, quality,
                                    settings->dither == 1,
                                    settings->filter == 1,
                                    settings->mipmap == 1,
                                    settings->linear_mipmap == 1,
                                    settings->blend == 1,
                                    settings->alpha == 1);
    }
    return 1;
}

void selection_rectangle_render(const SelectionRectangleRenderState* state,
                                const SelectionRectangleRenderHooks* hooks)
{
    float anchor[2];
    float points[10];
    if (state == NULL || hooks == NULL || (state->flags & 0x100u) == 0u ||
        hooks->anchor_get == NULL || hooks->polyline_draw == NULL ||
        !hooks->anchor_get(hooks->context, anchor)) return;
    points[0] = anchor[0]; points[1] = anchor[1];
    points[2] = state->mouse_x; points[3] = anchor[1];
    points[4] = state->mouse_x; points[5] = state->mouse_y;
    points[6] = anchor[0]; points[7] = state->mouse_y;
    points[8] = anchor[0]; points[9] = anchor[1];
    hooks->polyline_draw(hooks->context, points, 4, state->color, 2);
}

int camera_pointer_ray_route(const CameraPointerRayHooks* hooks,
                             float x, float y, float* out_x, float* out_y)
{
    float near_point[3], far_point[3], direction[3];
    float plane_point[3] = { 0.0f, 0.0f, 0.0f };
    float plane_normal[3] = { 0.0f, 0.0f, -1.0f };
    float intersection[3];
    int i;
    if (hooks == NULL || hooks->screen_point_to_ray == NULL ||
        hooks->plane_intersect == NULL || out_x == NULL || out_y == NULL)
        return 0;
    if (!hooks->screen_point_to_ray(hooks->context, x, y, 0.0f, near_point) ||
        !hooks->screen_point_to_ray(hooks->context, x, y, 1.0f, far_point))
        return 0;
    for (i = 0; i < 3; ++i) direction[i] = far_point[i] - near_point[i];
    if (!hooks->plane_intersect(hooks->context, near_point, direction,
                                plane_point, plane_normal, intersection))
        return 0;
    *out_x = intersection[0];
    *out_y = intersection[1];
    return 1;
}

void render_viewport_state_update(RenderViewportState* state,
                                  const RenderViewportHooks* hooks)
{
    float position[2];
    if (state == NULL || hooks == NULL) return;
    if (state->view_object == NULL) state->flags |= 0x4000u;
    if ((state->flags & 0x4000u) != 0u) {
        position[0] = state->fallback_position[0];
        position[1] = state->fallback_position[1];
    }
    else {
        position[0] = 0.0f;
        position[1] = 0.0f;
        if (hooks->view_position_get != NULL)
            hooks->view_position_get(hooks->context, state->view_object,
                                     position);
    }
    if (hooks->display_state_set != NULL)
        hooks->display_state_set(hooks->context, state->device,
                                 state->display_value);
    if (hooks->position_set != NULL)
        hooks->position_set(hooks->context, state->device, position);
}

void global_render_record_set_from_input(
    void* record, void* view, const GlobalRenderRecordHooks* hooks)
{
    float value;
    if (record == NULL || view == NULL || hooks == NULL ||
        hooks->value_get == NULL || hooks->record_set == NULL) return;
    value = hooks->value_get(hooks->context, record, 0.7f, 0.01f);
    hooks->record_set(hooks->context, view, record, 2, value);
}

static SceneUserVisualManagerHooks s_user_visual_manager_hooks;

void scene_user_visual_manager_hooks_set(
    const SceneUserVisualManagerHooks* hooks)
{
    if (hooks == NULL)
        memset(&s_user_visual_manager_hooks, 0,
               sizeof(s_user_visual_manager_hooks));
    else
        s_user_visual_manager_hooks = *hooks;
}

static int scene_user_visual_operations_present(
    const SceneUserVisualManagerHooks* hooks)
{
    return hooks->renderer_device != NULL &&
        hooks->owner_hidden != NULL && hooks->facing_update != NULL &&
        hooks->render_context_open != NULL &&
        hooks->interface_release != NULL && hooks->group_get != NULL &&
        hooks->state_begin != NULL && hooks->group_draw != NULL &&
        hooks->model_draw != NULL && hooks->state_end != NULL;
}

int scene_user_visual_manager_operations_present(void)
{
    return scene_user_visual_operations_present(&s_user_visual_manager_hooks);
}

static int scene_user_visual_defer(EngineTaskCallbackRecord* record,
                                   const float transform[16])
{
    void* previous = model_render_list_head_peek();
    uint32_t payload[16];
    /* This queue stores a target record word, not a widened host pointer.
     * Refuse an identity that the real consumer could not reconstruct. */
    if ((uintptr_t)record != (uintptr_t)(uint32_t)(uintptr_t)record) return 0;
    /* 482ce0 copies the record word and sixteen transform words into the
     * real renderer queue. It is distinct from the parsed-Model queue. */
    memcpy(payload, transform, sizeof(payload));
    renderer_command_node_prepend((uint32_t)(uintptr_t)record, payload);
    return model_render_list_head_peek() != previous;
}

int scene_user_visual_manager_dispatch(EngineTaskCallbackRecord* record,
                                        uint32_t reason, void* viewport)
{
    /* Preserve the original owner throughout this call, including cleanup
     * if a nested renderer operation replaces the installed table. */
    const SceneUserVisualManagerHooks hooks = s_user_visual_manager_hooks;
    float transform[16];
    void* parameter = NULL;
    void* acquired = NULL;
    int succeeded = 1;
    int state_started = 0;
    int deferred = 0;
    int pass;
    if (reason == SCENE_USER_VISUAL_CAN_SEE) {
        int hidden;
        if (hooks.renderer_device == NULL || record == NULL ||
            (record->words[11] & 1u) != 0u ||
            hooks.owner_hidden == NULL) return 0;
        hidden = hooks.owner_hidden(hooks.context, record);
        if (hidden != 0) return 0;
        if ((record->words[11] & 0x01000000u) != 0u &&
            (record->words[11] & 0x02000000u) == 0u) {
            int updated;
            if (hooks.facing_update == NULL) return 0;
            updated = hooks.facing_update(hooks.context, record, viewport);
            if (updated < 0) return 0;
            if (updated != 0) record->words[11] |= 0x02000000u;
        }
        return 1;
    }
    if (reason != SCENE_USER_VISUAL_RENDER) return 0;
    /* 4828bb..4828c1 returns one without accessing the record or drawing. */
    if (hooks.renderer_device == NULL) return 1;
    if (record == NULL || !scene_user_visual_operations_present(&hooks))
        return 0;
    record->words[11] &= ~0x02000000u;
    if (!hooks.render_context_open(hooks.context, hooks.renderer_device,
            viewport, transform, &parameter, &acquired) || acquired == NULL) {
        succeeded = 0;
        goto finish;
    }
    if ((record->words[11] & 2u) != 0u) {
        succeeded = scene_user_visual_defer(record, transform);
    }
    else if ((record->words[6] & 0x1c00u) != 0u) {
        /* The two scans re-read live count, eligibility and material state.
         * 482ac0 may change them; never pre-classify or cache the groups. */
        for (pass = 0; pass < 2; ++pass) {
            uint32_t index;
            for (index = 0u; index < record->words[0]; ++index) {
                void* group = hooks.group_get(hooks.context, record, index);
                uint32_t flags;
                uint32_t material;
                if (group == NULL || ((uintptr_t)group & 3u) != 0u) {
                    succeeded = 0;
                    goto finish;
                }
                if (!model_child_render_eligibility_check(group)) continue;
                memcpy(&flags, (const char*)group + 0x6c, sizeof(flags));
                if ((flags & 0x80000u) != 0u) {
                    deferred = 1;
                    continue;
                }
                memcpy(&material, (const char*)group + 0x64,
                       sizeof(material));
                if ((pass == 0 && material != 0u) ||
                    (pass != 0 && material == 0u)) continue;
                if (!state_started) {
                    state_started = 1;
                    if (!hooks.state_begin(hooks.context, record, parameter,
                                             transform)) {
                        succeeded = 0;
                        goto finish;
                    }
                }
                if (!hooks.group_draw(hooks.context, record, group,
                                       transform)) {
                    succeeded = 0;
                    goto finish;
                }
            }
        }
        if (deferred) succeeded = scene_user_visual_defer(record, transform);
    }
    else {
        state_started = 1;
        if (!hooks.state_begin(hooks.context, record, parameter, transform) ||
            !hooks.model_draw(hooks.context, record, transform))
            succeeded = 0;
    }
finish:
    /* Pair cleanup with the start attempt, including partially changed
     * state on adapter failure. Release a failed query's nonnull output too.
     * Retail assumes successful COM calls; these refusals are port policy. */
    if (state_started && !hooks.state_end(hooks.context)) succeeded = 0;
    if (acquired != NULL) hooks.interface_release(hooks.context, acquired);
    return succeeded;
}

EngineTaskCallbackHooks s_engine_task_callback_hooks;

void engine_task_callback_hooks_set(const EngineTaskCallbackHooks* hooks)
{
    if (hooks == NULL)
        memset(&s_engine_task_callback_hooks, 0,
               sizeof(s_engine_task_callback_hooks));
    else
        s_engine_task_callback_hooks = *hooks;
}

void engine_task_callback_record_release(
    EngineTaskCallbackRecord* record)
{
    if (record == NULL) return;
    render_resource_instance_record_release(record);
}

void scene_object_record_retain_hook_set(SceneObjectRecordRetainHook hook,
                                         void* context)
{
    s_scene_object_record_retain_hook = hook;
    s_scene_object_record_retain_context = context;
}

void scene_frame_apply_hooks_set(const SceneFrameApplyHooks* hooks)
{
    if (hooks == NULL) memset(&s_scene_frame_apply_hooks, 0,
                              sizeof(s_scene_frame_apply_hooks));
    else s_scene_frame_apply_hooks = *hooks;
}

int viewport_target_position_clamp(ViewportTargetClampState* state,
                                   const int position[2])
{
    int clamped_x, clamped_y;
    int maximum_x, maximum_y;
    int changed;
    if (state == NULL || position == NULL || state->edge_extent < 0 ||
        state->map_width <= 0 || state->map_height <= 0) return 0;
    clamped_x = position[0];
    clamped_y = position[1];
    maximum_x = state->map_width - state->edge_extent - 1;
    maximum_y = state->map_height - state->edge_extent - 1;
    if (clamped_x < state->edge_extent) clamped_x = state->edge_extent;
    if (clamped_x > maximum_x) clamped_x = maximum_x;
    if (clamped_y < state->edge_extent) clamped_y = state->edge_extent;
    if (clamped_y > maximum_y) clamped_y = maximum_y;
    changed = clamped_x != position[0] || clamped_y != position[1];
    state->target[0] = clamped_x;
    state->target[1] = clamped_y;
    state->global_flags |= 0x200000u;
    return changed;
}

void scene_rotation_apply_hooks_set(const SceneRotationApplyHooks* hooks)
{
    if (hooks == NULL) memset(&s_scene_rotation_apply_hooks, 0,
                              sizeof(s_scene_rotation_apply_hooks));
    else s_scene_rotation_apply_hooks = *hooks;
}

void scene_rotation_flags_apply(void* object, const float first[3],
                                const float second[3], const float third[3],
                                float scale, float uniform_scale,
                                float child_value, uint8_t flags)
{
    scene_rotation_flags_apply(object, 0.0f, first, second, third, scale,
                               uniform_scale, child_value, flags);
}
RenderResourceBindingHooks s_render_resource_binding_hooks;

void camera_view_context_close_hooks_set(CameraViewContextReleaseHook release,
                                         CameraViewContextFreeHook free_view,
                                         void* context)
{
    s_view_context_release_hook = release;
    s_view_context_free_hook = free_view;
    s_view_context_close_context = context;
}

void animation_transform_hooks_set(AnimationObjectTransformApplyHook orient,
                                   RenderNodePositionApplyHook position,
                                   void* context)
{
    s_animation_orientation_hook = orient;
    s_render_node_position_hook = position;
    s_animation_transform_context = context;
}

void scene_matrix_apply_hook_set(SceneMatrixApplyHook hook, void* context)
{
    s_scene_matrix_apply_hook = hook;
    s_scene_matrix_apply_context = context;
}

void renderer_record_clone_hooks_set(const RendererRecordCloneHooks* hooks)
{
    if (hooks == NULL) {
        memset(&s_renderer_record_clone_hooks, 0,
               sizeof(s_renderer_record_clone_hooks));
    }
    else {
        s_renderer_record_clone_hooks = *hooks;
    }
}

void quad_mesh_render_hooks_set(const QuadMeshRenderHooks* hooks)
{
    if (hooks == NULL) {
        memset(&s_quad_mesh_render_hooks, 0, sizeof(s_quad_mesh_render_hooks));
    }
    else {
        s_quad_mesh_render_hooks = *hooks;
    }
}

void sine_phase_blend_state_set(const SinePhaseBlendState* state)
{
    if (state == NULL) memset(&s_sine_phase_blend_state, 0,
                              sizeof(s_sine_phase_blend_state));
    else s_sine_phase_blend_state = *state;
}

void sine_phase_blend_hooks_set(const SinePhaseBlendHooks* hooks)
{
    if (hooks == NULL) memset(&s_sine_phase_blend_hooks, 0,
                              sizeof(s_sine_phase_blend_hooks));
    else s_sine_phase_blend_hooks = *hooks;
}

void render_resource_binding_hooks_set(
    const RenderResourceBindingHooks* hooks)
{
    if (hooks == NULL) {
        memset(&s_render_resource_binding_hooks, 0,
               sizeof(s_render_resource_binding_hooks));
    }
    else {
        s_render_resource_binding_hooks = *hooks;
    }
}

void scene_object_chain_hooks_set(const SceneObjectChainHooks* hooks)
{
    if (hooks == NULL) {
        memset(&s_scene_object_chain_hooks, 0,
               sizeof(s_scene_object_chain_hooks));
    }
    else {
        s_scene_object_chain_hooks = *hooks;
    }
}

void scene_object_chain_record_release(SceneObjectChainRecord* record)
{
    int i;
    if (record == NULL) return;
    if (s_scene_object_chain_hooks.node_release != NULL) {
        for (i = 4; i >= 0; --i) {
            if (record->node[i] != NULL) {
                s_scene_object_chain_hooks.node_release(
                    s_scene_object_chain_hooks.context, record->node[i]);
            }
        }
    }
    engine_heap_free(record);
}

void scene_object_dispose_hooks_set(const SceneObjectDisposeHooks* hooks)
{
    if (hooks == NULL) {
        memset(&s_scene_object_dispose_hooks, 0,
               sizeof(s_scene_object_dispose_hooks));
    }
    else {
        s_scene_object_dispose_hooks = *hooks;
    }
}

void scene_object_owner_link_release_hooks_set(
    SceneOwnerLinkReleaseFn interface_release,
    SceneOwnerLinkReleaseFn child_chain_release)
{
    s_owner_link_interface_release = interface_release;
    s_owner_link_child_release = child_chain_release;
}

/* ---- pure vector helpers ------------------------------------------------
 * Corpus mapping (finding 280; the portable C fallbacks preserve the
 * byte-equivalent single-precision results, while selected MSVC/i386 paths
 * mirror the corpus's x87 instruction schedule):
 *   FUN_00401720 vec_scale, FUN_00401500 vec_add,  FUN_004016c0 vec_sub,
 *   FUN_00401470 vec_cross, FUN_00401750 vec_normalize,
 *   FUN_00401420 vec_dist,  FUN_004016f0 vec_dot,  FUN_004013a0 vec_len,
 *   FUN_00401240 vec2_dist,
 *   FUN_0047c0c0 vec_exact_equal. */

#if !defined(LEGORRR_BACKEND_WINAPI)
/* D3DRM.DLL's D3DRMVectorRandom and D3DRMVectorRotate for the native port,
 * which has no D3DRM import. Maths.c's wrappers (FUN_00479500,
 * FUN_00479510) call these; the Windows builds call the DLL. */
static float d3drm_vector_modulus(const float* vector)
{
    float maximum;
    float x, y, z;
    if (fabs((double)vector[0]) >= fabs((double)vector[1]))
        maximum = (float)fabs((double)vector[0]);
    else
        maximum = (float)fabs((double)vector[1]);
    if (fabs((double)vector[2]) > (double)maximum)
        maximum = (float)fabs((double)vector[2]);
    if (maximum == 0.0f) return 0.0f;
    x = vector[0] / maximum;
    y = vector[1] / maximum;
    z = vector[2] / maximum;
    /* The DLL accumulates the scaled squares in x, y, z order. */
    return maximum * (float)sqrt((double)(x * x + y * y + z * z));
}

static void d3drm_vector_normalize(float* vector)
{
    float modulus = d3drm_vector_modulus(vector);
    if (modulus != 0.0f) {
        float scale = 1.0f / modulus;
        vector[0] = scale * vector[0];
        vector[1] = scale * vector[1];
        vector[2] = scale * vector[2];
    }
    else {
        /* D3DRMVectorNormalize maps a zero vector to its +X basis vector. */
        vector[0] = 1.0f;
        vector[1] = 0.0f;
        vector[2] = 0.0f;
    }
}

#if defined(_MSC_VER)
/* The D3DRM DLL keeps this LCG word in its per-thread context at +0x14. */
__declspec(thread) static uint32_t s_d3drm_random_state = 1u;
#elif defined(__GNUC__) || defined(__clang__)
static __thread uint32_t s_d3drm_random_state = 1u;
#else
/* This fallback is for toolchains without a native TLS storage specifier. */
static uint32_t s_d3drm_random_state = 1u;
#endif

extern "C" float* D3DRMVectorRandom(float* out)
{
    float modulus;

    /* D3DRM.DLL's TLS state starts at one. Each call applies its 0x343fd /
     * 0x269ec3 LCG, takes bits 16..30, maps through 1/32767 and subtracts
     * 0.5, then retries only if the vector modulus is zero. */
    do {
        uint32_t value;
        s_d3drm_random_state =
            s_d3drm_random_state * 0x343fdu + 0x269ec3u;
        value = (s_d3drm_random_state >> 16) & 0x7fffu;
        out[0] = (float)value * (1.0f / 32767.0f) - 0.5f;
        s_d3drm_random_state =
            s_d3drm_random_state * 0x343fdu + 0x269ec3u;
        value = (s_d3drm_random_state >> 16) & 0x7fffu;
        out[1] = (float)value * (1.0f / 32767.0f) - 0.5f;
        s_d3drm_random_state =
            s_d3drm_random_state * 0x343fdu + 0x269ec3u;
        value = (s_d3drm_random_state >> 16) & 0x7fffu;
        out[2] = (float)value * (1.0f / 32767.0f) - 0.5f;
        modulus = d3drm_vector_modulus(out);
    } while (modulus == 0.0f);
    d3drm_vector_normalize(out);
    return out;
}

extern "C" float* D3DRMVectorRotate(float* out, float* vector, float* axis,
                                    float angle)
{
    float input[3];
    float normalized_axis[3];
    float cross[3];
    float projection;
    float one_minus_cosine;
    float rotation_angle = -angle;
    float sine;
    float cosine;

    /* The import normalizes the axis in place before loading the source
     * vector. It then uses Rodrigues' formula with -theta and normalizes the
     * result before returning. Cache both vectors before writing so r may
     * alias v or axis just as in the DLL. */
    d3drm_vector_normalize(axis);
    input[0] = vector[0];
    input[1] = vector[1];
    input[2] = vector[2];
    normalized_axis[0] = axis[0];
    normalized_axis[1] = axis[1];
    normalized_axis[2] = axis[2];
    cosine = (float)cos((double)rotation_angle);
    sine = (float)sin((double)rotation_angle);
    one_minus_cosine = 1.0f - cosine;
    projection = normalized_axis[0] * input[0] +
                 normalized_axis[1] * input[1] +
                 normalized_axis[2] * input[2];
    cross[0] = normalized_axis[1] * input[2] -
               normalized_axis[2] * input[1];
    cross[1] = normalized_axis[2] * input[0] -
               normalized_axis[0] * input[2];
    cross[2] = normalized_axis[0] * input[1] -
               normalized_axis[1] * input[0];
    out[0] = input[0] * cosine + cross[0] * sine +
             normalized_axis[0] * projection * one_minus_cosine;
    out[1] = input[1] * cosine + cross[1] * sine +
             normalized_axis[1] * projection * one_minus_cosine;
    out[2] = input[2] * cosine + cross[2] * sine +
             normalized_axis[2] * projection * one_minus_cosine;
    d3drm_vector_normalize(out);
    return out;
}
#endif /* !LEGORRR_BACKEND_WINAPI */

/* ---- geometry helpers (finding 363/323) --------------------------------
 * Pure math leaves, corpus-exact operations: the corpus computes on x87
 * long doubles, the port on single precision (documented deviation). */

/* ---- offset-point rect family (findings 241/363/446) --------------------- */

/* Shared body of the portable rectangle helpers. The retail leaves obtain
 * their two offsets from x87 ST(0)/ST(1), which a normal C signature cannot
 * express. VC5's __ftol conversion rounds to nearest before subtracting. */
static int rect_contains_converted(const float rect[4], float off_x,
                                   float off_y, int x, int y)
{
    const float fx = (float)(x - crt_float_to_long(off_x));
    const float fy = (float)(y - crt_float_to_long(off_y));
    return (rect[0] <= fx && fy < rect[3]
            && rect[1] <= fy && fx < rect[2])
               ? 1 : 0;
}

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
/* Port-local adapter. Retail 0x45b590 has four stack arguments
 * (unused, bounds, x, y) and consumes its offsets from x87 state. */
int rect_contains_offset_point(const float rect_bounds[4],
                               const float offset[2], int x, int y)
{
    if (rect_bounds == NULL || offset == NULL) {
        return 0;
    }
    return rect_contains_converted(rect_bounds, offset[0], offset[1], x, y);
}

/* Port-local scalar convenience overload retained for existing callers. */
int rect_contains_offset_point(const float rect_bounds[4], float off_x,
                               float off_y, int x, int y)
{
    return rect_contains_converted(rect_bounds, off_x, off_y, x, y);
}

/* Port-local adapter. Retail 0x463c00 has the same x87-input ABI as
 * 0x45b590 and is intentionally not mapped to this ordinary C helper. */
int rect_contains_point_by_converted_coords(const float rect_bounds[4],
                                            const float offset[2],
                                            int x, int y)
{
    if (rect_bounds == NULL || offset == NULL) {
        return 0;
    }
    return rect_contains_converted(rect_bounds, offset[0], offset[1], x, y);
}

/* Port-local scalar convenience overload retained for existing callers. */
int rect_contains_point_by_converted_coords(const float rect_bounds[4],
                                            float off_x, float off_y,
                                            int x, int y)
{
    return rect_contains_converted(rect_bounds, off_x, off_y, x, y);
}

/* Port helper for an explicit bounds slot and offset pair. Retail
 * FUN_00463c90 takes (record, x, y), reads bounds at record + 0x14, and
 * obtains its two conversion values from state not exposed by this API. */
int rect_contains_point_from_object_bounds(const float* const* bounds_at_14,
                                           float off_x, float off_y,
                                           int x, int y)
{
    if (bounds_at_14 == NULL || *bounds_at_14 == NULL) {
        return 0;
    }
    return rect_contains_converted(*bounds_at_14, off_x, off_y, x, y);
}
#endif /* !LEGORRR_BACKEND_WINAPI */

/* ---- camera-handle primitives (record-backed) --------------------------- */

void cam_offset(CameraHandle* c, float x, float y, float z)
{
    if (c == NULL) {
        return;
    }
    c->inuse = 1;
    c->pos[0] += x;
    c->pos[1] += y;
    c->pos[2] += z;
}

void cam_get_pos(CameraHandle* c, float out[3])
{
    if (c != NULL && out != NULL) {
        out[0] = c->pos[0];
        out[1] = c->pos[1];
        out[2] = c->pos[2];
    }
}

void cam_set_pos(CameraHandle* c, float x, float y, float z)
{
    if (c == NULL) {
        return;
    }
    c->inuse = 1;
    c->pos[0] = x;
    c->pos[1] = y;
    c->pos[2] = z;
}

void cam_get_facing(CameraHandle* c, float out[3])
{
    if (c != NULL && out != NULL) {
        out[0] = c->rot[0];
        out[1] = c->rot[1];
        out[2] = c->rot[2];
    }
}

void cam_set_facing(CameraHandle* c, const float* r)
{
    if (c == NULL || r == NULL) {
        return;
    }
    c->inuse = 1;
    c->rot[0] = r[0];
    c->rot[1] = r[1];
    c->rot[2] = r[2];
}

/* Camera world queries cross through the bound terrain owner. In
 * particular, world X/Y are never voxel indices and +4 is not a height. */

int engine_context_grid_pick(EngineContext* ctx, const float* pos,
                                    int* gx, int* gy)
{
    if (ctx == NULL || pos == NULL || gx == NULL || gy == NULL) return 0;
    if (ctx->level_block != NULL)
        return terrain_world_xy_to_cell(ctx->level_terrain,
            pos[0], pos[1], gx, gy) &&
            *gx < engine_level_width(ctx) - 1 &&
            *gy < engine_level_height(ctx) - 1;
    *gx = (int)pos[0];
    *gy = (int)pos[1];
    return 1;
}
float terrain_height(EngineContext* ctx, float x, float y)
{
    if (ctx != NULL && ctx->level_terrain != NULL)
        return terrain_surface_height_interpolate(ctx->level_terrain, x, y);
    return 0.0f;
}

/* ---- the ported tick ---------------------------------------------------- */

float view_camera_delta_from_seconds(float seconds)
{
    float ticks;
    if (seconds <= 0.0f) return 0.0f;
    ticks = seconds * 25.0f;
    return ticks > 3.0f ? 3.0f : ticks;
}

void view_primary_target_request(EngineContext* ctx, float x, float y,
                                 int tween)
{
    ViewContext* view = ctx != NULL ? ctx->views[0] : NULL;
    if (view == NULL || view->mode != 1) return;
    view->target_xy[0] = x;
    view->target_xy[1] = y;
    view->target_pending = 1;
    view->target_tween = tween != 0;
    ctx->state |= 0x200000u;
}

/* EngineTick @ 0x4232e0, before MainSimStep. This is the one consumer of
 * the script's world target, distinct from the mode-3 eye-pose follower. */
void view_primary_target_tick(EngineContext* ctx, float reference_delta)
{
    ViewContext* view = ctx != NULL ? ctx->views[0] : NULL;
    float x, y;
    if (view == NULL || view->mode != 1 || !view->target_pending ||
        reference_delta <= 0.0f) return;
    x = view->target_xy[0];
    y = view->target_xy[1];
    if (view->target_tween) {
        float factor = 5.0f / reference_delta;
        float dx, dy;
        x = (factor * view->cam_facing.pos[0] + x) / (factor + 1.0f);
        y = (factor * view->cam_facing.pos[1] + y) / (factor + 1.0f);
        dx = x - view->cam_facing.pos[0];
        dy = y - view->cam_facing.pos[1];
        if (sqrtf(dx * dx + dy * dy) < 2.0f) view->target_tween = 0;
    }
    selection_record_point_set(view, x, y);
    view->target_pending = view->target_tween;
    ctx->state &= ~0x200000u;
}

void view_primary_level_release(EngineContext* ctx)
{
    ViewContext* view = ctx != NULL ? ctx->views[0] : NULL;
    if (view == NULL) return;
    view->target_pending = view->target_tween = 0;
    view->pose_valid = 0;
    memset(&view->displayed, 0, sizeof(view->displayed));
    ctx->state &= ~0x200000u;
}

int view_primary_level_initialize(EngineContext* ctx, float x, float y,
                                   float heading)
{
    ViewContext* view = ctx != NULL ? ctx->views[0] : NULL;
    if (view == NULL || view->mode != 1 || view->controller == NULL ||
        ctx->level_terrain == NULL) return 0;
    view_primary_level_release(ctx);
    memset(view->offset, 0, sizeof(view->offset));
    memset(view->velocity, 0, sizeof(view->velocity));
    selection_record_point_set(view, x, y);
    camera_rotation_set(view->controller, heading);
    camera_tilt_set(view->controller, 0.8f);
    /* Establish the initial ground/eye frames without advancing a tween. */
    view_tick(view, ctx, 1.0f, 0.0f);
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
    if (!scene_runtime_cameras_sync()) return 0;
    return view->pose_valid;
#else
    return 1;
#endif
}

void view_primary_projection_invalidate(ViewContext* view)
{
    if (view == NULL) return;
    /* Retain the draw ordinal; get requires the displayed level identity. */
    view->displayed.level = NULL;
    view->displayed.width = view->displayed.height = 0;
}

int view_primary_projection_publish(ViewContext* view, const void* level,
                                     int width, int height)
{
    ViewCameraProjection* projection;
    if (view == NULL || !view->pose_valid || level == NULL ||
        width <= 0 || height <= 0) {
        view_primary_projection_invalidate(view);
        return 0;
    }
    projection = &view->displayed;
    projection->pose = view->pose;
    projection->level = level;
    projection->width = width;
    projection->height = height;
    /* Shipped D3DRM viewport defaults: front=1, plane +/-0.5, uniform
     * scaling. The square-pixel viewport scale is max(width,height)/2;
     * the perspective plane contributes 2. No screenshot FOV constant. */
    projection->focal_x = (float)(width > height ? width : height);
    projection->focal_y = projection->focal_x;
    projection->center_x = (float)width * 0.5f;
    projection->center_y = (float)height * 0.5f;
    ++projection->generation;
    if (projection->generation == 0u) ++projection->generation;
    return 1;
}

int view_primary_projection_get(const ViewContext* view, const void* level,
                                 int width, int height,
                                 ViewCameraProjection* projection)
{
    if (view == NULL || projection == NULL ||
        view->displayed.generation == 0u || view->displayed.level == NULL ||
        view->displayed.level != level ||
        view->displayed.width != width || view->displayed.height != height)
        return 0;
    *projection = view->displayed;
    return 1;
}

/* The corpus ticks the three view contexts each frame (finding 03 §4). */
void view_views_tick(EngineContext* ctx, float dt, float scale)
{
    if (ctx == NULL) {
        return;
    }
    for (int i = 0; i < 3; ++i) {
        if (ctx->views[i] != NULL) {
            view_tick(ctx->views[i], ctx, dt, scale);
        }
    }
}

/* Startup CameraViewContextSet binds CameraSpeed, CameraDropOff,
 * CameraAcceleration and MouseScrollIndent in the reference caller order. */
float g_view_pan_cap   = 0.0f;  /* port copy of cameraGlobs.cameraSpeed (005560a0, camera_control.c) */
float g_view_pan_scale = 0.0f;  /* port copy of cameraGlobs.cameraAcceleration (005560a8) */

CameraPoseDispatchHook s_camera_pose_dispatch_hook;
void* s_camera_pose_dispatch_context;
SceneTimingValueGetHook s_scene_timing_value_get_hook;
SceneTimingCountGetHook s_scene_timing_count_get_hook;
void* s_scene_timing_hooks_context;
AnimationObjectTimeUpdateHook s_animation_object_time_update_hook;
AnimationObjectActionHook s_animation_object_time_get_hook;
AnimationObjectActionHook s_animation_object_value_get_hook;
void* s_animation_object_hooks_context;
static RendererSubobjectFindHook s_renderer_subobject_find_hook;
static RendererSubobjectApplyHook s_renderer_subobject_apply_hook;
static void* s_renderer_subobject_hooks_context;
RendererHandleScalarGetHook s_renderer_handle_scalar_get_hook;
void* s_renderer_handle_scalar_get_context;

void camera_pose_dispatch_hook_install(CameraPoseDispatchHook hook,
                                       void* context)
{
    s_camera_pose_dispatch_hook = hook;
    s_camera_pose_dispatch_context = context;
}

void scene_timing_value_hooks_install(SceneTimingValueGetHook value_get,
                                      SceneTimingCountGetHook count_get,
                                      void* context)
{
    s_scene_timing_value_get_hook = value_get;
    s_scene_timing_count_get_hook = count_get;
    s_scene_timing_hooks_context = context;
}

void animation_object_hooks_install(AnimationObjectTimeUpdateHook update,
                                    AnimationObjectActionHook time_get,
                                    AnimationObjectActionHook value_get,
                                    void* context)
{
    s_animation_object_time_update_hook = update;
    s_animation_object_time_get_hook = time_get;
    s_animation_object_value_get_hook = value_get;
    s_animation_object_hooks_context = context;
}

void renderer_subobject_hooks_install(RendererSubobjectFindHook find,
                                      RendererSubobjectApplyHook apply,
                                      void* context)
{
    s_renderer_subobject_find_hook = find;
    s_renderer_subobject_apply_hook = apply;
    s_renderer_subobject_hooks_context = context;
}

void renderer_handle_scalar_hook_install(RendererHandleScalarGetHook hook,
                                         void* context)
{
    s_renderer_handle_scalar_get_hook = hook;
    s_renderer_handle_scalar_get_context = context;
}

void* scene_object_subobject_resolve(void* owner, void* source,
                                            int index);
static void* scene_object_cached_subobject_get(void* owner,
                                               size_t source_offset,
                                               size_t cache_offset,
                                               int index);

static uint8_t cam_flags(const void* rec, size_t off)
{
    return *(const uint8_t*)((const char*)rec + off);
}

static void cam_or_flags(void* rec, uint8_t mask)
{
    uint8_t f = *(uint8_t*)((char*)rec + CAM_FLAGS_OFF);
    f |= mask;
    *(uint8_t*)((char*)rec + CAM_FLAGS_OFF) = f;
}

void terrain_point_pick_cell_check_hook_set(
    TerrainPointPickCellCheckHook hook, void* user)
{
    s_terrain_point_pick_hook = hook;
    s_terrain_point_pick_user = user;
}

void viewport_point_dispatch_hooks_set(const ViewportPointDispatchHooks* hooks)
{
    if (hooks == NULL)
        memset(&s_viewport_point_dispatch_hooks, 0,
               sizeof(s_viewport_point_dispatch_hooks));
    else
        s_viewport_point_dispatch_hooks = *hooks;
}

void textured_scene_draw_hooks_set(const TexturedSceneDrawHooks* hooks)
{
    if (hooks == NULL)
        memset(&s_textured_scene_draw_hooks, 0,
               sizeof(s_textured_scene_draw_hooks));
    else
        s_textured_scene_draw_hooks = *hooks;
}

/* CameraZoomStepAdjust (FUN_0045bf20) — see view.hpp. */
void camera_zoom_step_context_set(void* context)
{
    s_zoom_step_context = context;
}

/* ---- scene-root hook band (2026-10-24) ----------------------------------- *
 * SceneRootObjectGet / DDiSceneObjectArgumentsNormalize — see view.hpp. The
 * corpus's root global (DAT_0076add4) is containerGlobs.rootContainer, which
 * container.c defines (finding 624); this is the port's word. */
void* s_scene_root;

#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
#include "lwsc_winapi.hpp"
#endif

#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
static struct SceneCameraBinding {
    CameraHandle* camera;
    void* scene;
} s_scene_camera_bindings[9];
static unsigned int s_scene_camera_count;
static ViewContext* s_scene_primary_view;

static void scene_runtime_camera_pose_set(void* scene, void* relative,
    float x, float y, float z, float up_x, float up_y, float up_z, void* user)
{
    (void)user;
    (void)ddiscene_object_pose_method_dispatch(scene, relative,
        x, y, z, up_x, up_y, up_z);
}

static void scene_runtime_property_release(void* object)
{
    DDiSceneAnimationHookRecord* property =
        ddiscene_animation_hook_record_get(object);
    if (property != NULL) {
        ddiscene_animation_hook_record_set(object, NULL);
        if (property->name != NULL) engine_heap_free(property->name);
        engine_heap_free(property);
    }
}

static void scene_runtime_frame_release(void* frame)
{
    scene_runtime_property_release(frame);
    scene_winapi_frame_release(frame);
}

static void scene_runtime_record_destroy(void* record, int mode)
{
    ddiscene_object_destroy_with_mode(record, mode);
}

int scene_runtime_initialize(void)
{
    typedef long (VIEW_COM_CALL *SortModeSetFn)(void*, uint32_t);
    void* root;
    void* frame;
    void** vtable;
    if (s_scene_root != NULL) return 0;
    if (!scene_winapi_initialize()) return 0;
    lego::ddiscene_object_pool_hooks_set(scene_winapi_frame_create,
        scene_runtime_frame_release, scene_runtime_record_destroy, NULL);
    root = lego::ddiscene_object_allocate(NULL);
    if (root == NULL) {
        scene_runtime_shutdown();
        return 0;
    }
    frame = *(void**)root;
    vtable = *(void***)frame;
    /* EntityVisualTypeTablesInit @ 0x472710: SetSortMode(D3DRMSORT_NONE). */
    if (((SortModeSetFn)vtable[0xf8 / 4])(frame, 1u) != 0) {
        scene_runtime_shutdown();
        return 0;
    }
    scene_root_install(root);
    return 1;
}

void scene_runtime_shutdown(void)
{
#if defined(LWSC_WINAPI_POSE_BACKEND)
    if (!lwsc_winapi_can_shutdown()) {
        fprintf(stderr, "scene: LWSC pose contexts still own root/frame references; shutdown deferred\n");
        return;
    }
#endif
    if (!indexed_surface_resources_can_shutdown()) {
        fprintf(stderr, "scene: refusing shutdown with live roof resource\n");
        return;
    }
    if (s_scene_primary_view != NULL) {
        ViewContext* view = s_scene_primary_view;
        void* empty = NULL;
        view->scene_eye = view->scene_tilt = view->scene_follow = NULL;
        view->pose_valid = 0;
        memset(&view->displayed, 0, sizeof(view->displayed));
        if (view->controller != NULL) {
            memcpy((char*)view->controller + 0x24, &empty, sizeof(void*));
            memcpy((char*)view->controller + 0x28, &empty, sizeof(empty));
            memcpy((char*)view->controller + 0x30, &empty, sizeof(empty));
        }
    }
    s_scene_primary_view = NULL;
    camera_pose_dispatch_hook_install(NULL, NULL);
    s_scene_camera_count = 0u;
    memset(s_scene_camera_bindings, 0, sizeof(s_scene_camera_bindings));
    scene_root_install(NULL);
    lego::ddiscene_object_pool_shutdown();
    lego::ddiscene_object_pool_hooks_set(NULL, NULL, NULL, NULL);
    scene_winapi_shutdown();
}

int scene_runtime_primary_camera_bind(ViewContext* view, void* controller)
{
    if (view == NULL || controller == NULL || s_scene_root == NULL ||
        s_scene_primary_view != NULL) return 0;
    /* Constructor 0x435b20: root -> follow/yaw -> tilt -> eye. The
     * terrain-projected cam_facing remains a separate light-frame owner. */
    s_scene_primary_view = view;
    view->controller = controller;
    view->scene_follow = lego::ddiscene_object_allocate(
        (const lego::DDiSceneObjectDescriptor*)s_scene_root);
    if (view->scene_follow == NULL) return 0;
    view->scene_tilt = lego::ddiscene_object_allocate(
        (const lego::DDiSceneObjectDescriptor*)view->scene_follow);
    if (view->scene_tilt == NULL) return 0;
    view->scene_eye = lego::ddiscene_object_allocate(
        (const lego::DDiSceneObjectDescriptor*)view->scene_tilt);
    if (view->scene_eye == NULL) return 0;
    memcpy((char*)controller + 0x24, &view->scene_eye, sizeof(void*));
    memcpy((char*)controller + 0x28, &view->scene_follow, sizeof(void*));
    memcpy((char*)controller + 0x30, &view->scene_tilt, sizeof(void*));
    camera_pose_dispatch_hook_install(scene_runtime_camera_pose_set, NULL);
    if (!ddiscene_object_position_set(view->scene_eye, view->scene_tilt,
                                      0.0f, 0.0f, -200.0f) ||
        !ddiscene_object_pose_method_dispatch(view->scene_eye, view->scene_tilt,
            0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f)) return 0;
    return 1;
}

int scene_runtime_collection_pose_get(const ViewContext* view, int primary,
                                       float position[3], float forward[3])
{
    if (view == NULL || position == NULL || s_scene_root == NULL ||
        !lego::ddiscene_object_pool_owns(view->scene_follow) ||
        !ddiscene_object_position_get(view->scene_follow, NULL, position))
        return 0;
    if (!primary) return 1;
    return forward != NULL &&
        lego::ddiscene_object_pool_owns(view->scene_eye) &&
        ddiscene_object_facing_get(view->scene_eye, NULL, forward);
}

void* scene_runtime_camera_bind(CameraHandle* camera)
{
    void* scene;
    if (camera == NULL || s_scene_root == NULL ||
        s_scene_camera_count >= 9u) return NULL;
    scene = lego::ddiscene_object_allocate(
        (const lego::DDiSceneObjectDescriptor*)s_scene_root);
    if (scene == NULL) return NULL;
    s_scene_camera_bindings[s_scene_camera_count].camera = camera;
    s_scene_camera_bindings[s_scene_camera_count++].scene = scene;
    return scene;
}

int scene_runtime_cameras_sync(void)
{
    unsigned int i;
    if (s_scene_primary_view != NULL) {
        ViewContext* view = s_scene_primary_view;
        view->pose_valid = 0;
        if (!ddiscene_object_position_set(view->scene_follow, NULL,
                view->cam_follow.pos[0], view->cam_follow.pos[1],
                view->cam_follow.pos[2]) ||
            !ddiscene_object_position_set(view->scene_eye, view->scene_tilt,
                view->cam_world.pos[0], view->cam_world.pos[1],
                view->cam_world.pos[2]) ||
            !ddiscene_object_position_get(view->scene_eye, NULL,
                view->pose.eye) ||
            !ddiscene_object_facing_pair_get(view->scene_eye, NULL,
                view->pose.forward, view->pose.up)) return 0;
        view->pose_valid = 1;
    }
    for (i = 0u; i < s_scene_camera_count; ++i) {
        const CameraHandle* camera = s_scene_camera_bindings[i].camera;
        if (!ddiscene_object_position_set(s_scene_camera_bindings[i].scene,
                NULL, camera->pos[0], camera->pos[1], camera->pos[2])) return 0;
        /* cam_facing stores a forward vector. A zero vector is the
         * uninitialized portable camera's identity orientation. */
        if (camera->rot[0] != 0.0f || camera->rot[1] != 0.0f ||
            camera->rot[2] != 0.0f) {
            if (!ddiscene_object_pose_method_dispatch(
                    s_scene_camera_bindings[i].scene, NULL,
                    camera->rot[0], camera->rot[1], camera->rot[2],
                    0.0f, 1.0f, 0.0f)) return 0;
        }
    }
    return 1;
}

void* scene_runtime_light_create(void* owner, unsigned int kind,
                                  const float rgb[3])
{
    void* record = lego::ddiscene_object_allocate(
        (const lego::DDiSceneObjectDescriptor*)owner);
    void** payload;
    void* frame;
    void** vtable;
    typedef long (VIEW_COM_CALL *AddLightFn)(void*, void*);
    if (record == NULL) return NULL;
    *(uint32_t*)((char*)record + 0x10) = 5u;
    payload = (void**)engine_heap_alloc(2 * sizeof(void*));
    if (payload == NULL) {
        scene_runtime_record_destroy(record, 0);
        return NULL;
    }
    memset(payload, 0, 2 * sizeof(void*));
    *(void**)((char*)record + 0x0c) = payload;
    if (scene_winapi_light_create(kind, rgb, &payload[1]) != 0) {
        scene_runtime_record_destroy(record, 0);
        return NULL;
    }
    frame = ((void**)record)[1];
    vtable = *(void***)frame;
    if (((AddLightFn)vtable[0x30 / 4])(frame, payload[1]) != 0) {
        scene_runtime_record_destroy(record, 0);
        return NULL;
    }
    return record;
}

int scene_runtime_light_parameter_set(void* light, unsigned int offset,
                                       float value)
{
    void* object = ((void**)*(void**)((char*)light + 0x0c))[1];
    void** vtable = *(void***)object;
    typedef long (VIEW_COM_CALL *LightParameterFn)(void*, float);
    return ((LightParameterFn)vtable[offset / 4])(object, value) == 0;
}
#endif
/* The port's word for containerGlobs.fogColour (0076cd8c; finding 624). */
uint32_t s_scene_fog_color;
// GLOBAL: LEGORR 0x00505920
uint32_t s_scene_fog_mode;

/* The port's model of Viewports.c's viewport pool (viewportGlobs,
 * 0076ace0-0076ad6c), which gods98/viewports.c now defines (finding 593). */
void* s_render_surface_pool_blocks[32];
unsigned int s_render_surface_pool_level_count;
void* s_render_surface_pool_free_head;

static void* render_surface_pool_link_get(void* record)
{
    void* next;
    memcpy(&next, (unsigned char*)record + 0x1c, sizeof(next));
    return next;
}

void render_surface_pool_link_set(void* record, void* next)
{
    memcpy((unsigned char*)record + 0x1c, &next, sizeof(next));
}

void* ddiscene_render_surface_pool_acquire(void)
{
    void* record;
    if (s_render_surface_pool_free_head == NULL) {
        ddiscene_render_surface_pool_grow();
    }
    record = s_render_surface_pool_free_head;
    if (record != NULL) {
        s_render_surface_pool_free_head = render_surface_pool_link_get(record);
        render_surface_pool_link_set(record, NULL);
    }
    return record;
}

void ddiscene_render_surface_pool_release(void* record)
{
    if (record != NULL) {
        render_surface_pool_link_set(record, s_render_surface_pool_free_head);
        s_render_surface_pool_free_head = record;
    }
}

void ddiscene_render_surface_pool_head_push(void* record)
{
    ddiscene_render_surface_pool_release(record);
}

void ddiscene_render_surface_pool_initialize(void)
{
    unsigned int i;
    for (i = 0; i < 32u; ++i) s_render_surface_pool_blocks[i] = NULL;
    s_render_surface_pool_level_count = 0u;
    s_render_surface_pool_free_head = NULL;
}

unsigned int ddiscene_render_surface_pool_block_count(void)
{
    return s_render_surface_pool_level_count;
}

void* ddiscene_render_surface_pool_block_get(unsigned int index)
{
    return index < s_render_surface_pool_level_count
        ? s_render_surface_pool_blocks[index] : NULL;
}

void ddiscene_render_surface_pool_release_all(void)
{
    unsigned int i;
    for (i = 0; i < s_render_surface_pool_level_count; ++i) {
        if (s_render_surface_pool_blocks[i] != NULL)
            engine_heap_free(s_render_surface_pool_blocks[i]);
        s_render_surface_pool_blocks[i] = NULL;
    }
    s_render_surface_pool_level_count = 0u;
    s_render_surface_pool_free_head = NULL;
}

void* s_scene_registry_blocks[32];
unsigned int s_scene_registry_block_count;
SceneRegistrySurfaceReleaseFn s_scene_registry_surface_release;

void scene_object_registry_surface_release_set(
    SceneRegistrySurfaceReleaseFn release)
{
    s_scene_registry_surface_release = release;
}

void scene_object_registry_pool_block_register(void* block)
{
    if (block != NULL && s_scene_registry_block_count < 32u) {
        s_scene_registry_blocks[s_scene_registry_block_count++] = block;
    }
}

void scene_root_install(void* root) { s_scene_root = root; }
#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
void* scene_root_peek(void) { return s_scene_root; }
#endif /* !LEGORRR_BACKEND_WINAPI */

static void* scene_root_interface(void)
{
    return *(void**)s_scene_root;
}

/* DDiScene object interfaces expose the current action/property record at
 * vtable byte offset 0x1c. Keep the unrelated renderer-root vtable above
 * separate: this is the small interface used by the animation getters. */
class DDiScenePropertyInterface
{
public:
    virtual void slot_00(void) = 0;
    virtual void slot_04(void) = 0;
    virtual void slot_08(void) = 0;
    virtual void slot_0c(void) = 0;
    virtual void slot_10(void) = 0;
    virtual void slot_14(void) = 0;
    virtual void slot_18(void) = 0;
    virtual void* current_property_get(void) = 0;
};

#undef DDI_COORD_SLOT

#undef DDI_POSE_SLOT

/* FUN_004761e0's +0x18 value is passed as a sample name to the registered
 * frame callback. Preserve it as a native pointer in the portable runtime. */
const char* ddiscene_animation_sample_name_get(void* scene_interface)
{
    void* record = ((DDiScenePropertyInterface*)scene_interface)
        ->current_property_get();
    return record != NULL ? *(const char**)((char*)record + 0x18) : NULL;
}

/* Host-width view of the auxiliary pointer at +0x1c. XfTextureNameFormat
 * tests the pointed record's +0x0c state word. */
void* ddiscene_action_record_field_1c_pointer_get(void* scene_interface)
{
    void* record = ((DDiScenePropertyInterface*)scene_interface)
        ->current_property_get();
    void* value = NULL;
    if (record != NULL)
        memcpy(&value, (char*)record + 0x1c, sizeof(value));
    return value;
}

int ddiscene_object_facing_get(void* object, void* alternate_frame,
                               float facing[3])
{
    float secondary[3];
    return ddiscene_object_facing_pair_get(object, alternate_frame, facing,
                                            secondary);
}

const void* s_advisor_companion_transform;
AdvisorCompanionApplyFn s_advisor_object_apply;
AdvisorCompanionApplyFn s_advisor_scene_apply;

void advisor_companion_apply_hooks_set(const void* transform,
                                       AdvisorCompanionApplyFn object_apply,
                                       AdvisorCompanionApplyFn scene_apply)
{
    s_advisor_companion_transform = transform;
    s_advisor_object_apply = object_apply;
    s_advisor_scene_apply = scene_apply;
}

void* s_scene_value_query_context;
SceneObjectReadyQueryFn s_scene_ready_query;
SceneObjectCurrentValueFn s_scene_current_value_query;
SceneObjectValueReleaseFn s_scene_value_release;

void scene_object_value_query_hooks_set(
    void* context, SceneObjectReadyQueryFn ready_query,
    SceneObjectCurrentValueFn value_query, SceneObjectValueReleaseFn release)
{
    s_scene_value_query_context = context;
    s_scene_ready_query = ready_query;
    s_scene_current_value_query = value_query;
    s_scene_value_release = release;
}

/* --- the rotated-pairs scratch (see view.hpp) ----------------------------- */

void* rotated_pairs_scratch_peek(void) { return s_rot_pairs; }

/* --- the curve band (2026-10-25; see view.hpp) ----------------------------- */

/* Host-width view of the string carried in the second word of this property
 * result. XfTextureNameFormat uses the value as a path before lowercasing it. */
const char* scene_property_secondary_name_get(void* property)
{
    void** vtable;
    typedef void* (VIEW_COM_CALL *ScenePropertyValueRecordGetFn)(void*);
    void* record;
    const char* name = NULL;
    if (property == NULL) return NULL;
    vtable = *(void***)property;
    if (vtable == NULL || vtable[7] == NULL) return NULL;
    record = ((ScenePropertyValueRecordGetFn)vtable[7])(property);
    if (record != NULL)
        memcpy(&name, (char*)record + 4, sizeof(name));
    return name;
}

RendererSubobjectResolveFn s_renderer_subobject_resolver;

void renderer_subobject_resolver_set(RendererSubobjectResolveFn resolver)
{
    s_renderer_subobject_resolver = resolver;
}

void* scene_object_subobject_resolve(void* owner, void* source,
                                            int index)
{
    if (owner == NULL || source == NULL ||
        s_renderer_subobject_resolver == NULL) return NULL;
    return s_renderer_subobject_resolver(owner, source, index);
}

static void* scene_object_cached_subobject_get(void* owner,
                                               size_t source_offset,
                                               size_t cache_offset,
                                               int index)
{
    if (owner == NULL || index < 0 || index >= 16) {
        return NULL;
    }
    void* source = NULL;
    __builtin_memcpy(&source, (char*)owner + source_offset, sizeof(source));
    void* cached = NULL;
    const size_t slot_offset = cache_offset + (size_t)index * sizeof(void*);
    __builtin_memcpy(&cached, (char*)owner + slot_offset, sizeof(cached));
    if (cached == NULL && source != NULL) {
        cached = scene_object_subobject_resolve(owner, source, index);
        __builtin_memcpy((char*)owner + slot_offset, &cached, sizeof(cached));
    }
    return cached;
}

static SceneActivityChangeFn s_scene_activity_change;

void scene_activity_change_callback_set(SceneActivityChangeFn callback)
{
    s_scene_activity_change = callback;
}

/* FUNCTION: LEGORR 0x00408570
 * Clear the two counted handle arrays before notifying scene systems and
 * rebuilding the indexed table. Array counts are capped to their fixed
 * 16-slot layout in this portable model. */
int scene_activity_change(void* activity, void* state, void* value)
{
    if (activity == NULL) return 0;
    unsigned int count = 0;
    unsigned int i;
    __builtin_memcpy(&count, (char*)activity + 0x70u, sizeof(count));
    if (count > 16u) count = 16u;
    for (i = 0; i < count; ++i) {
        unsigned int zero = 0;
        __builtin_memcpy((char*)activity + 0x28u + i * 4u,
                         &zero, sizeof(zero));
    }
    __builtin_memcpy(&count, (char*)activity + 0x74u, sizeof(count));
    if (count > 16u) count = 16u;
    for (i = 0; i < count; ++i) {
        unsigned int zero = 0;
        __builtin_memcpy((char*)activity + 0x40u + i * 4u,
                         &zero, sizeof(zero));
    }
    unsigned int zero_count = 0;
    __builtin_memcpy((char*)activity + 0x68u, &zero_count, sizeof(zero_count));
    __builtin_memcpy((char*)activity + 0x6cu, &zero_count, sizeof(zero_count));
    int result = s_scene_activity_change != NULL
        ? s_scene_activity_change(activity, state, value) : 0;
    return result;
}

static UIRenderTreeHook s_ui_render_tree_hook;
static void* s_ui_render_tree_hook_user;

void ui_render_tree_hook_set(UIRenderTreeHook hook, void* user)
{
    s_ui_render_tree_hook = hook;
    s_ui_render_tree_hook_user = user;
}

/* FUNCTION: LEGORR 0x004889a0 */
void ui_render_entry_subrecord_apply(void* tree, void* entry,
                                     uint32_t subrecord_index)
{
    if (tree == NULL || entry == NULL) return;
    void* entries = NULL;
    __builtin_memcpy(&entries, (char*)tree + 0x14u, sizeof(entries));
    unsigned short entry_index = 0;
    __builtin_memcpy(&entry_index, (char*)entry + 6u,
                     sizeof(entry_index));
    if (entries == NULL || entry_index >= 256u) return;
    void* object = NULL;
    __builtin_memcpy(&object, (char*)entries
                     + (size_t)entry_index * sizeof(void*), sizeof(object));
    void* subrecord = NULL;
    __builtin_memcpy(&subrecord, (char*)entry + 0x18u
                     + (size_t)(subrecord_index & 0xffffu) * 0x28u,
                     sizeof(subrecord));
    if (s_ui_render_tree_hook != NULL) {
        s_ui_render_tree_hook(3, tree, entry, subrecord,
                              s_ui_render_tree_hook_user);
    }
    (void)object;
}

/* FUNCTION: LEGORR 0x00488900 */
void ui_render_tree_entry_process(void* tree, void* entry)
{
    unsigned int depth = 0;
    while (tree != NULL && entry != NULL && depth++ < 256u) {
        unsigned short entry_index = 0;
        __builtin_memcpy(&entry_index, (char*)entry + 6u,
                         sizeof(entry_index));
        unsigned short kind = 0;
        __builtin_memcpy(&kind, (char*)entry + 0x12u, sizeof(kind));
        if (kind == 1u) ui_render_entry_subrecord_apply(tree, entry, 0u);
        unsigned char flags = *((unsigned char*)entry + 0x28u);
        if ((flags & 1u) == 0 && s_ui_render_tree_hook != NULL) {
            void* node = s_ui_render_tree_hook(2, tree, entry, NULL,
                                                s_ui_render_tree_hook_user);
            if (node != NULL && (flags & 4u) != 0) {
                uint32_t node_flags = 0;
                __builtin_memcpy(&node_flags, (char*)node + 0x2cu,
                                 sizeof(node_flags));
                node_flags |= 0x01000000u;
                __builtin_memcpy((char*)node + 0x2cu, &node_flags,
                                 sizeof(node_flags));
            }
        }
        void* child = NULL;
        __builtin_memcpy(&child, (char*)entry + 0x1cu, sizeof(child));
        unsigned short child_gate = 1;
        __builtin_memcpy(&child_gate, (char*)entry + 0x26u,
                         sizeof(child_gate));
        if (child != NULL && child_gate == 0) {
            ui_render_element_children_enumerate(tree, entry);
            child = NULL;
            __builtin_memcpy((char*)entry + 0x1cu, &child, sizeof(child));
        }
        (void)entry_index;
        __builtin_memcpy(&entry, (char*)entry + 0x2cu, sizeof(entry));
    }
}

/* FUNCTION: LEGORR 0x004880d0 */
void ui_render_element_children_enumerate(void* tree, void* entry)
{
    if (s_ui_render_tree_hook != NULL && tree != NULL && entry != NULL) {
        s_ui_render_tree_hook(1, tree, entry, NULL,
                              s_ui_render_tree_hook_user);
    }
}

/* FUNCTION: LEGORR 0x00487990 */
void ui_render_tree_layout_and_release(void* tree, void* layout_context)
{
    if (tree == NULL) return;
    if (s_ui_render_tree_hook != NULL) {
        s_ui_render_tree_hook(0, tree, layout_context, NULL,
                              s_ui_render_tree_hook_user);
    }
    void* entry = NULL;
    __builtin_memcpy(&entry, (char*)tree + 0x1cu, sizeof(entry));
    unsigned int count = 0;
    while (entry != NULL && count++ < 4096u) {
        void* next = NULL;
        __builtin_memcpy(&next, (char*)entry + 0x30u, sizeof(next));
        ui_render_tree_entry_process(tree, entry);
        entry = next;
    }
    if (s_ui_render_tree_hook != NULL) {
        s_ui_render_tree_hook(4, tree, NULL, NULL,
                              s_ui_render_tree_hook_user);
    }
}

static void** s_global_entity_render_params;
static uint32_t s_global_entity_render_param_count;
static GlobalEntityRenderParamFn s_global_entity_render_param_update;
static void* s_global_entity_render_param_user;

void global_entity_render_parameters_install(void** entities, uint32_t count,
                                             GlobalEntityRenderParamFn update,
                                             void* user)
{
    s_global_entity_render_params = entities;
    s_global_entity_render_param_count = count > 65536u ? 65536u : count;
    s_global_entity_render_param_update = update;
    s_global_entity_render_param_user = user;
}

/* FUNCTION: LEGORR 0x0046e1c0 */
void global_entity_render_parameter_mode_set(int mode)
{
    if (s_global_entity_render_params == NULL
        || s_global_entity_render_param_update == NULL) return;
    const float first = mode == 0 ? 0.0f : 0.12f;
    const float second = mode == 0 ? 0.0f : 0.32f;
    for (uint32_t i = 0; i < s_global_entity_render_param_count; ++i) {
        if (s_global_entity_render_params[i] != NULL) {
            s_global_entity_render_param_update(
                s_global_entity_render_params[i], first, second,
                s_global_entity_render_param_user);
        }
    }
}

GridSurfaceLookupFn s_grid_surface_lookup;
GridSurfaceHandleReleaseFn s_grid_surface_release;
void* s_grid_surface_user;

void grid_surface_hooks_set(GridSurfaceLookupFn lookup,
                            GridSurfaceHandleReleaseFn release, void* user)
{
    s_grid_surface_lookup = lookup;
    s_grid_surface_release = release;
    s_grid_surface_user = user;
}

static WorldSpatialRecordModel* s_world_spatial_records;
static uint32_t s_world_spatial_record_count;

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
void world_spatial_records_install(WorldSpatialRecordModel* records,
                                   uint32_t count)
{
    s_world_spatial_records = records;
    s_world_spatial_record_count = count > 65536u ? 65536u : count;
}
#endif /* !LEGORRR_BACKEND_WINAPI */

/* FUNCTION: LEGORR 0x0046e8a0 */
WorldSpatialRecordModel* world_spatial_record_find_at_grid_coordinate(
    int x, int y, uint32_t* point_index)
{
    if (s_world_spatial_records == NULL) return NULL;
    for (uint32_t record = 0; record < s_world_spatial_record_count; ++record) {
        WorldSpatialRecordModel* candidate = &s_world_spatial_records[record];
        if (candidate->point_pairs == NULL) continue;
        uint32_t count = candidate->point_count > 65536u
            ? 65536u : candidate->point_count;
        for (uint32_t point = 0; point < count; ++point) {
            const int px = (int)candidate->point_pairs[point * 2u];
            const int py = (int)candidate->point_pairs[point * 2u + 1u];
            if (px == x && py == y) {
                if (point_index != NULL) *point_index = point;
                return candidate;
            }
        }
    }
    return NULL;
}


namespace unported_ledger_stubs {

// DDiSceneObjectDestroy
void ledger_stub_00472a50_DDiSceneObjectDestroy(void* scene_object,
                                                 int mode);

// DDiSceneObjectDestroy
void ledger_stub_00472a50_DDiSceneObjectDestroy(void* scene_object, int mode)
{
    ddiscene_object_destroy_with_mode(scene_object, mode);
}

// DDiSceneAdvanceByTime
void ledger_stub_00475120_DDiSceneAdvanceByTime(
    void* scene_object, float delta)
{
    ddiscene_advance_by_time(scene_object, delta);
}

// DDiSceneFrameUpdateAndCallbacks
float ledger_stub_00475140_DDiSceneFrameUpdateAndCallbacks(
    void* scene_object, float time)
{
    return ddiscene_frame_update_and_callbacks(scene_object, time);
}

// DDiSceneApplyActFrameTiming
void ledger_stub_00475300_DDiSceneApplyActFrameTiming(void* scene_object)
{
    ddiscene_apply_act_frame_timing(scene_object);
}

// DDiSceneObjectLengthGet
float ledger_stub_00475390_DDiSceneObjectLengthGet(void* scene_object)
{
    return ddiscene_object_length_get(scene_object);
}

// DDiSceneObjectFramePositionGet
uint32_t ledger_stub_004753f0_DDiSceneObjectFramePositionGet(
    void* scene_object)
{
    return ddiscene_object_frame_position_get(scene_object);
}



// DDiSceneResourceNameResolve
int ledger_stub_00476770_DDiSceneResourceNameResolve(const char* name)
{
    return ddiscene_resource_name_resolve(name);
}

// DDiSceneResourceExists
DdiSceneResourceExistsHook s_ddiscene_resource_exists_hook;
DdiSceneResourceOpenHook s_ddiscene_resource_open_hook;
void* s_ddiscene_resource_hook_context;
void ddiscene_resource_hooks_set(DdiSceneResourceExistsHook exists_hook,
                                 DdiSceneResourceOpenHook open_hook,
                                 void* context)
{
    s_ddiscene_resource_exists_hook = exists_hook;
    s_ddiscene_resource_open_hook = open_hook;
    s_ddiscene_resource_hook_context = context;
}

// SceneObjectBoundsUnion
uint32_t ledger_stub_00474f10_SceneObjectBoundsUnion(
    void* scene_object, float bounds_min_max[6])
{
    return scene_object_bounds_union(scene_object, bounds_min_max);
}

// RenderObjectSetPackedRgbaFromFloatChannels
void ledger_stub_004750b0_RenderObjectSetPackedRgbaFromFloatChannels(
    void* scene_object, float red, float green, float blue, float alpha)
{
    render_object_set_packed_rgba_from_float_channels(
        scene_object, red, green, blue, alpha);
}

// DDiSceneObjectAttachOrUnlink
void ledger_stub_00475710_DDiSceneObjectAttachOrUnlink(
    void* pair_record, void* source_pair_record)
{
    ddiscene_scene_object_attach_or_unlink(pair_record, source_pair_record);
}


// DDiSceneContextGet
void* ledger_stub_004757a0_DDiSceneContextGet(void* scene_object)
{
    return ddiscene_context_get(scene_object);
}

// TypedChildResourceScalarGet
float ledger_stub_004757f0_TypedChildResourceScalarGet(void* scene_object)
{
    return typed_child_resource_scalar_get(scene_object);
}

// SceneObjectFirstNonKind6ChildGet
void* ledger_stub_00475830_SceneObjectFirstNonKind6ChildGet(
    void* child_interface)
{
    return scene_object_first_non_kind6_child_get(child_interface);
}

// SceneObjectOwnerLinkEnsure
void* ledger_stub_00475880_SceneObjectOwnerLinkEnsure(void* owner_interface)
{
    return scene_object_owner_link_ensure(owner_interface);
}

// DDiSceneObjectArrayGrow
void ledger_stub_004759f0_DDiSceneObjectArrayGrow()
{
    lego::ddiscene_object_array_grow();
}

// SceneActionRecordActFrameScan
unsigned int ledger_stub_00475a70_SceneActionRecordActFrameScan(
    void* scene_object, void** records, int* frame_indices, char** names)
{
    return scene_action_record_act_frame_scan(scene_object, records,
                                              frame_indices, names);
}

// DDiSceneAnimationCallbacksSet
void ledger_stub_00475c00_DDiSceneAnimationCallbacksSet(
    void* scene_object, void* callback_a, void* callback_b,
    void* callback_c, void* callback_d)
{
    ddiscene_animation_callbacks_set(scene_object, callback_a, callback_b,
                                     callback_c, callback_d);
}

// DDiScenePayloadRelease
void ledger_stub_00475c80_DDiScenePayloadRelease(void* scene_object)
{
    ddiscene_payload_release(scene_object);
}

// CallbackRecordSetFlag10AndRefresh
void ledger_stub_00475e10_CallbackRecordSetFlag10AndRefresh(void* property)
{
    callback_record_set_flag10_and_refresh(property);
}

// DDiSceneFrameRecordFind
void* ledger_stub_00475e40_DDiSceneFrameRecordFind(
    void* action_interface, const char* action_name, int selector)
{
    return ddiscene_frame_record_find(action_interface, action_name, selector);
}

// DDiSceneAnimationHookCreate
void ledger_stub_00475f70_DDiSceneAnimationHookCreate(
    void* scene_object, int callback, const char* name,
    const uint32_t* value_0c, int value_08, const uint32_t* value_10,
    const uint32_t* value_14, int value_18, int value_24,
    const uint32_t* value_20)
{
    ddiscene_animation_hook_create(scene_object, callback, name, value_0c,
                                   value_08, value_10, value_14, value_18,
                                   value_24, value_20);
}

// AttachedObjectDataReleaseAndClear
void ledger_stub_004760e0_AttachedObjectDataReleaseAndClear(void* property)
{
    attached_object_data_release_and_clear(property);
}



// RenderInterfacePairUpdate
void ledger_stub_00476220_RenderInterfacePairUpdate(
    void* first, void* second)
{
    render_interface_pair_update(first, second);
}

void ledger_stub_004876c0_SharedObjectDirectoryScan(
    const char* directory, SharedObjectEntryResolveFn resolve,
    SharedObjectSoundEnabledFn sound_enabled,
    SharedObjectSoundSubmitFn sound_submit)
{
    shared_object_directory_scan_configure(directory, resolve, sound_enabled,
                                           sound_submit);
}

void ledger_stub_004877e0_SceneEntrySlotTableExpand(void* scene)
{
    shared_object_scene_entry_slot_table_expand(scene);
}

void* ledger_stub_00487a00_SharedObjectClone(void* source,
                                             void* caller_context)
{
    return shared_object_clone(source, caller_context);
}

// AnimationTrackFindKeyframeAndFraction
float ledger_stub_00487cb0_AnimationTrackFindKeyframeAndFraction(
    const void* track, float time, uint16_t* segment_out)
{
    return ::animation_track_find_keyframe_and_fraction(track, time,
                                                         segment_out);
}

#undef LEDGER_SCENE_COM_STDCALL

// SharedObjectTimedSoundEventUpdate
void ledger_stub_00487de0_SharedObjectTimedSoundEventUpdate(void* owner,
                                                            void* track)
{
    shared_object_timed_sound_event_update(owner, track);
}

// SharedObjectFloatTrackInterpolateApply
void ledger_stub_00488070_SharedObjectFloatTrackInterpolateApply(
    void* owner, const void* track, uint32_t segment, float fraction)
{
    lego::shared_object_float_track_interpolate_apply(owner, track, segment,
                                                       fraction);
}

// AnimationTrackKeysInterpolate
void ledger_stub_00488170_AnimationTrackKeysInterpolate(
    void* owner, const void* track, uint32_t segment, float fraction)
{
    ::animation_track_keys_interpolate(owner, track, segment, fraction);
}

// SceneNodeTransformMatrixApply
void ledger_stub_004882e0_SceneNodeTransformMatrixApply(void* owner,
    const void* track, const float position[3], const float rotation[3],
    const float scale[3])
{
    ::scene_node_transform_matrix_apply(owner, track, position, rotation,
                                        scale);
}


/* CursorRayToCameraRoute (FUN_00426420) performs camera ray and pan updates;
 * this call only resets the view record's +0x9c cursor-ray accumulator
 * (FUN_00435f16), so it is deliberately left unmapped here. */
void ledger_stub_00426420_CursorRayToCameraRoute(ViewContext* view)
{
    cursor_screen_ray_context_reset(view);
}


// ComInterfaceQueryOrFallback
void* ledger_stub_004894c0_ComInterfaceQueryOrFallback(
    float color0, float color1, float color2,
    float color3, float color4, float color5, float power)
{
    return scene_material_create(color0, color1, color2, color3, color4,
                                 color5, power);
}

// DDiSceneTimedHookDispatch
void ledger_stub_004897e0_DDiSceneTimedHookDispatch(
    void* hook_record, float time, void* output)
{
    ddiscene_timed_hook_dispatch(hook_record, time, output);
}

} // namespace unported_ledger_stubs

ObjectHierarchyResourceHooks s_object_hierarchy_resource_hooks;

void object_hierarchy_resource_hooks_set(
    const ObjectHierarchyResourceHooks* hooks)
{
    if (hooks == NULL)
        memset(&s_object_hierarchy_resource_hooks, 0,
               sizeof(s_object_hierarchy_resource_hooks));
    else
        s_object_hierarchy_resource_hooks = *hooks;
}

namespace unported_ledger_stubs {


void ledger_stub_00408620_SceneObjectRecordCloneRetainHandles(
    const SceneObjectRecord* source, SceneObjectRecord* destination)
{
    scene_object_record_clone_retain_handles(source, destination);
}




void ledger_stub_0040ba00_SceneRotationFlagsApply(
    void* object, float projection_scale, const float first[3],
    const float second[3],
    const float third[3], float scale, float uniform_scale,
    float child_value, uint8_t flags)
{
    scene_rotation_flags_apply(object, projection_scale, first, second,
                               third, scale, uniform_scale, child_value,
                               flags);
}

void ledger_stub_0040bb90_SceneFrameFromTripletApply(
    void* object, float projection_scale, const float first[3],
    const float second[3], const float third[3])
{
    scene_frame_from_triplet_apply(object, projection_scale, first, second,
                                   third);
}


} // namespace unported_ledger_stubs
