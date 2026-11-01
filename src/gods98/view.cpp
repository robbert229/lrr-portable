// SPDX-License-Identifier: LGPL-3.0-or-later
#include <math.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "view.hpp"
#include "record.hpp"
#include "texture.hpp"
#include "heap.hpp"
#include "crt/crt.hpp"

/* The shared game thunk maps to VC5 _rand on the reference toolchain and to
 * the same 15-bit stream on portable builds. */
extern uint32_t rng_thunk_draw15(void);

/* Reconstruction-owned globals for corpus constants. */
float g_view_follow_scale = 1.0f;   /* DAT_005560a4 (grid pick follow scale) */
// GLOBAL: LEGORR 0x005560ac
int   g_view_grid_edge   = 0;       /* DAT_005560ac (tile-count edge clamp) */
int   g_view_test_blend  = 0;       /* test hook for the mode-3 EMA target */
static ViewObjectLookupHook s_view_object_lookup_hook = NULL;
static ViewTargetBlendHook s_view_target_blend_hook = NULL;
static void* s_view_follow_target_context = NULL;

void view_follow_target_hooks_set(ViewObjectLookupHook lookup,
    ViewTargetBlendHook blend, void* context)
{
    s_view_object_lookup_hook = lookup;
    s_view_target_blend_hook = blend;
    s_view_follow_target_context = context;
}

/* CameraZoomStepAdjust reads the primary view/camera context directly from
 * the corpus's active-view global. Its owner is the unported config setup. */
// GLOBAL: LEGORR 0x005560e4
static void* s_zoom_step_context;
static TerrainPointPickCellCheckHook s_terrain_point_pick_hook;
static void* s_terrain_point_pick_user;
static ViewportPointDispatchHooks s_viewport_point_dispatch_hooks;
static TexturedSceneDrawHooks s_textured_scene_draw_hooks;

/* The corpus's normalize numerator is the .rdata dword 1.0 at 0x49e008
 * (objdump-verified fdivr operand in FUN_00401750); a named const static
 * keeps the fld operand resolvable by Reccmp. */
// GLOBAL: LEGORR 0x0049e008
static const float s_normalize_unit = 1.0f;   /* DAT_0049e008, verified */

/* The curve builder's divisor is the .rdata dword 1.0 at 0x49e050
 * (objdump-verified fdivr operand in FUN_00406820; a different .rdata dword
 * from 0x49e008, since Reccmp resolves constants per-address); a named
 * const static keeps the fld operand resolvable by Reccmp. */
// GLOBAL: LEGORR 0x0049e050
static const float s_curve_unit = 1.0f;   /* DAT_0049e050, verified */

static SceneOwnerLinkReleaseFn s_owner_link_interface_release;
static SceneOwnerLinkReleaseFn s_owner_link_child_release;
static ViewportDimensionsQueryFn s_viewport_dimensions_query;
static void* s_viewport_dimensions_context;
static ModelTransformReadHook s_model_transform_read_hook;
static SceneTransformWriteHook s_scene_transform_write_hook;
static void* s_model_transform_context;
static CameraViewContextReleaseHook s_view_context_release_hook;
static CameraViewContextFreeHook s_view_context_free_hook;
static void* s_view_context_close_context;
static AnimationObjectTransformApplyHook s_animation_orientation_hook;
static RenderNodePositionApplyHook s_render_node_position_hook;
static void* s_animation_transform_context;
static SceneMatrixApplyHook s_scene_matrix_apply_hook;
static void* s_scene_matrix_apply_context;
static SceneObjectChainHooks s_scene_object_chain_hooks;
static SceneObjectDisposeHooks s_scene_object_dispose_hooks;
static SceneObjectBoundsHooks s_scene_object_bounds_hooks;
static RenderPackedRgbaHooks s_render_packed_rgba_hooks;
static RenderHierarchyFloat3Hooks s_render_hierarchy_float3_hooks;
static SceneActionNameLookupHook s_scene_action_name_lookup_hook;
static void* s_scene_action_name_lookup_context;
static SceneActionInterfaceGetHook s_scene_action_interface_get_hook;
static void* s_scene_action_interface_get_context;
static VideoAspectRatioQueryHook s_video_aspect_ratio_query_hook;
static void* s_video_aspect_ratio_query_context;
static SceneContextHooks s_scene_context_hooks;
static SceneActFrameScanHooks s_scene_act_frame_scan_hooks;
static SceneActFrameRecordCloneHooks s_scene_act_frame_record_clone_hooks;
static EntityVisualResourceLoadHooks s_entity_visual_resource_load_hooks;
static DDiSceneAnimationHookRecordHooks s_ddiscene_animation_hook_record_hooks;
static DDiSceneFrameRecordFindHooks s_ddiscene_frame_record_find_hooks;
static DDiSceneSampleHook s_ddiscene_sample_hook;
static void* s_ddiscene_sample_context;
static int s_ddiscene_sample_enabled;
typedef struct DDiSceneSampleHookEntry {
    DDiSceneSampleHook callback;
    void* context;
} DDiSceneSampleHookEntry;
static DDiSceneSampleHookEntry s_ddiscene_additional_sample_hooks[8];
static unsigned int s_ddiscene_additional_sample_hook_count;
static DDiSceneEndHook s_ddiscene_end_hook;
static void* s_ddiscene_end_context;

/* Keep the portable callback fanout behind one callback-shaped dispatch.
 * The reference draw step itself invokes one registered callback. */
static void ddiscene_sample_hook_dispatch(const char* sample_name,
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
static DDiSceneLightCreateHooks s_ddiscene_light_create_hooks;
static DDiSceneKindModeSetupHooks s_ddiscene_kind_mode_setup_hooks;
static DDiSceneDrawBeginHook s_ddiscene_draw_begin_hook;
static void* s_ddiscene_draw_begin_context;
static SceneHierarchyRegisterHooks s_scene_hierarchy_register_hooks;
static DDiSceneObjectDestroyHook s_ddiscene_object_destroy_hook;
static void* s_ddiscene_object_destroy_context;
static SceneChildSearchHooks s_scene_child_search_hooks;
static SceneFormattedStringHooks s_scene_formatted_string_hooks;
static RenderInterfacePairHooks s_render_interface_pair_hooks;
static DDiScenePayloadReleaseHooks s_ddiscene_payload_release_hooks;
static RendererRecordCloneHooks s_renderer_record_clone_hooks;
static SceneObjectListApplyHook s_scene_object_list_apply_hook;
static void* s_scene_object_list_apply_context;
static uint32_t s_quad_mesh_render_records[10][8];
static void* s_quad_mesh_render_objects[10];
static void* s_quad_mesh_render_callbacks[10];
static QuadMeshRenderHooks s_quad_mesh_render_hooks;
static SinePhaseBlendState s_sine_phase_blend_state;
static SinePhaseBlendHooks s_sine_phase_blend_hooks;
static SceneFrameApplyHooks s_scene_frame_apply_hooks;
static SceneRotationApplyHooks s_scene_rotation_apply_hooks;
static SceneObjectRecordRetainHook s_scene_object_record_retain_hook;
static void* s_scene_object_record_retain_context;
static RenderQualityApplyHook s_render_quality_apply_hook;
static void* s_render_quality_apply_context;

#if defined(_MSC_VER)
typedef void (__stdcall *DDiScenePayloadReleaseMethod)(void*);
#elif defined(__i386__) && (defined(__GNUC__) || defined(__clang__))
typedef void (__attribute__((stdcall)) *DDiScenePayloadReleaseMethod)(void*);
#else
typedef void (*DDiScenePayloadReleaseMethod)(void*);
#endif

#if defined(_MSC_VER) && defined(_M_IX86)
static void ddiscene_payload_owner_detach_call(void* owner, void* payload,
                                                void* method)
{
    __asm {
        mov ecx, dword ptr [esp + 4]
        mov edx, dword ptr [esp + 8]
        mov eax, dword ptr [esp + 12]
        push edx
        call eax
    }
}
#elif defined(__i386__) && (defined(__GNUC__) || defined(__clang__))
typedef void (__attribute__((thiscall)) *DDiScenePayloadDetachMethod)(
    void*, void*);
static void ddiscene_payload_owner_detach_call(void* owner, void* payload,
                                                void* method)
{
    ((DDiScenePayloadDetachMethod)method)(owner, payload);
}
#else
typedef void (*DDiScenePayloadDetachMethod)(void*, void*);
static void ddiscene_payload_owner_detach_call(void* owner, void* payload,
                                                void* method)
{
    ((DDiScenePayloadDetachMethod)method)(owner, payload);
}
#endif

static void ddiscene_payload_interface_release(void* interface_object)
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

/* SceneObjectBoundsUnion (FUN_00474f10): the root's bounds initialize the
 * output, then each child contributes a componentwise min/max. The original
 * obtains both the root and child collection through scene-interface calls;
 * those accessors stay replaceable while the union behavior is local. */
// FUNCTION: LEGORR 0x00474f10
uint32_t scene_object_bounds_union(void* scene_object,
                                  float bounds_min_max[6])
{
    SceneObjectBoundsHooks* hooks = &s_scene_object_bounds_hooks;
    void* root;
    const void* children;
    uint32_t count = 0;
    uint32_t i;
    float child_bounds[6];

    if (scene_object == NULL || bounds_min_max == NULL ||
        hooks->root_get == NULL || hooks->bounds_query == NULL) {
        return 0;
    }
    root = hooks->root_get(scene_object, hooks->context);
    if (root == NULL) return 0;
    hooks->bounds_query(root, bounds_min_max, hooks->context);
    if (hooks->children_get == NULL || hooks->child_get == NULL) return 0;
    children = hooks->children_get(root, &count, hooks->context);
    if (children == NULL) return 0;

    for (i = 0; i < count; i++) {
        void* child = hooks->child_get(children, i, hooks->context);
        int axis;
        if (child == NULL) continue;
        hooks->bounds_query(child, child_bounds, hooks->context);
        for (axis = 0; axis < 3; axis++) {
            if (child_bounds[axis] < bounds_min_max[axis]) {
                bounds_min_max[axis] = child_bounds[axis];
            }
            if (child_bounds[axis + 3] > bounds_min_max[axis + 3]) {
                bounds_min_max[axis + 3] = child_bounds[axis + 3];
            }
        }
    }
    return 0;
}

void render_packed_rgba_hooks_set(const RenderPackedRgbaHooks* hooks)
{
    if (hooks == NULL) memset(&s_render_packed_rgba_hooks, 0,
                              sizeof(s_render_packed_rgba_hooks));
    else s_render_packed_rgba_hooks = *hooks;
}

/* RenderObjectSetPackedRgbaFromFloatChannels (FUN_004750b0): convert the
 * four caller-supplied float channels in reference order, then update the
 * render interface stored in the kind-five mesh record. */
// FUNCTION: LEGORR 0x004750b0
void render_object_set_packed_rgba_from_float_channels(
    void* scene_object, float red, float green, float blue, float alpha)
{
    RenderPackedRgbaHooks* hooks = &s_render_packed_rgba_hooks;
    void* mesh;
    void* render_object;
    uint32_t packed;
    if (scene_object == NULL ||
        *(uint32_t*)((char*)scene_object + 0x10) != 5u ||
        hooks->packed_set == NULL) return;
    mesh = *(void**)((char*)scene_object + 0x0c);
    if (mesh == NULL) return;
    render_object = *(void**)((char*)mesh + 4);
    if (render_object == NULL) return;
    packed = ((uint32_t)crt_float_to_long(red) << 24) |
             ((uint32_t)crt_float_to_long(green) << 16) |
             ((uint32_t)crt_float_to_long(blue) << 8) |
             (uint32_t)crt_float_to_long(alpha);
    hooks->packed_set(render_object, packed, hooks->context);
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

/* SceneActionNamePresent (FUN_00473340): only kind-4 scene objects expose
 * the action-name lookup path. The backend callback models the scene's
 * action search while preserving that type gate and boolean result. */
uint32_t scene_action_name_present(void* scene_object, void* action_name)
{
    if (scene_object == NULL || *(uint32_t*)((char*)scene_object + 0x10) != 4u ||
        s_scene_action_name_lookup_hook == NULL) {
        return 0;
    }
    return s_scene_action_name_lookup_hook(
        scene_object, action_name, s_scene_action_name_lookup_context) != 0;
}

void scene_act_frame_scan_hooks_set(const SceneActFrameScanHooks* hooks)
{
    if (hooks == NULL) memset(&s_scene_act_frame_scan_hooks, 0,
                              sizeof(s_scene_act_frame_scan_hooks));
    else s_scene_act_frame_scan_hooks = *hooks;
}

static int scene_act_frame_name_match(const char* name)
{
    static const char prefix[] = "ActFrame_";
    unsigned int i;
    if (name == NULL) return 0;
    for (i = 0; i < 8; ++i) {
        int a = (unsigned char)name[i];
        int b = (unsigned char)prefix[i];
        if (a == 0) return 0;
        if (a >= 'a' && a <= 'z') a -= ('a' - 'A');
        if (b >= 'a' && b <= 'z') b -= ('a' - 'A');
        if (a != b) return 0;
    }
    return 1;
}

/* SceneActionRecordActFrameScan (FUN_00475a70): enumerate both type-four
 * animation channels and fill the caller's optional record, frame-index, and
 * owned-name arrays in the same order. */
// FUNCTION: LEGORR 0x00475a70
unsigned int scene_action_record_act_frame_scan(void* scene_object,
                                                void** records,
                                                int* frame_indices,
                                                char** names)
{
#if defined(_MSC_VER)
#define SCENE_SCAN_STDCALL __stdcall
#elif defined(__i386__) && (defined(__GNUC__) || defined(__clang__))
#define SCENE_SCAN_STDCALL __attribute__((stdcall))
#else
#define SCENE_SCAN_STDCALL
#endif
    typedef unsigned int (SCENE_SCAN_STDCALL *ActionCountMethod)(void*);
    typedef void (SCENE_SCAN_STDCALL *ActionRecordMethod)(
        void*, unsigned int, void**);
    unsigned int slot, found = 0;
    SceneActFrameScanHooks* hooks = &s_scene_act_frame_scan_hooks;
    if (scene_object == NULL ||
        *(uint32_t*)((char*)scene_object + 0x10) != 4u ||
        hooks->action_name_get == NULL)
        return 0;
    for (slot = 0; slot < 2; ++slot) {
        void* animation;
        if (hooks->animation_interface_get != NULL) {
            animation = hooks->animation_interface_get(
                scene_object, slot, hooks->context);
        }
        else {
            memcpy(&animation,
                   (char*)scene_object + (slot == 0u ? 4u : 8u),
                   sizeof(animation));
        }
        unsigned int count, index;
        if (animation == NULL) continue;
        if (hooks->action_count_get != NULL) {
            count = hooks->action_count_get(animation, hooks->context);
        }
        else {
            void** vtable = *(void***)animation;
            if (vtable == NULL || vtable[3] == NULL) continue;
            count = ((ActionCountMethod)vtable[3])(animation);
        }
        for (index = 0; index < count; ++index) {
            const char* name = hooks->action_name_get(animation, index,
                                                       hooks->context);
            if (scene_act_frame_name_match(name)) {
                void* action_record = NULL;
                if (records != NULL || frame_indices != NULL) {
                    if (hooks->action_record_get != NULL) {
                        action_record = hooks->action_record_get(
                            animation, index, hooks->context);
                    }
                    else {
                        void** vtable = *(void***)animation;
                        if (vtable != NULL && vtable[4] != NULL) {
                            ((ActionRecordMethod)vtable[4])(
                                animation, index, &action_record);
                        }
                    }
                }
                if (records != NULL) {
                    records[found] = action_record;
                }
                if (frame_indices != NULL) {
                    int value = 0;
                    if (action_record != NULL) {
                        value = (int)ddiscene_action_record_field_1c_get(
                            action_record);
                    }
                    else if (hooks->action_value_get != NULL) {
                        hooks->action_value_get(animation, index, &value,
                                                hooks->context);
                    }
                    frame_indices[found] = value;
                }
                if (names != NULL) {
                    size_t length = strlen(name);
                    char* copy = (char*)engine_heap_alloc((long)(length + 1u));
                    if (copy != NULL) memcpy(copy, name, length + 1u);
                    names[found] = copy;
                }
                ++found;
            }
        }
    }
#undef SCENE_SCAN_STDCALL
    return found;
}

void scene_act_frame_record_clone_hooks_set(
    const SceneActFrameRecordCloneHooks* hooks)
{
    if (hooks == NULL)
        memset(&s_scene_act_frame_record_clone_hooks, 0,
               sizeof(s_scene_act_frame_record_clone_hooks));
    else s_scene_act_frame_record_clone_hooks = *hooks;
}

/* The `words[3] != 0` record form is a shallow record clone with a retained
 * interface in word 1. The zero form builds a fresh scene-animation record;
 * its QueryInterface and animation-list clone calls are COM-specific and
 * stay behind the typed full_record_clone seam. */
// FUNCTION: LEGORR 0x00489660
SceneActFrameRecord* scene_act_frame_record_clone_or_create(
    const SceneActFrameRecord* source, uint32_t clone_argument,
    uintptr_t* output_value)
{
    SceneActFrameRecord* copy;
    if (source == NULL) return NULL;
    copy = (SceneActFrameRecord*)engine_heap_alloc(
        (long)sizeof(SceneActFrameRecord));
    if (copy == NULL) return NULL;
    if (source->words[3] != 0u) {
        memcpy(copy, source, sizeof(*copy));
        copy->words[0] = 0u;
        if (source->words[1] != 0u) {
            if (s_scene_act_frame_record_clone_hooks.interface_retain == NULL) {
                engine_heap_free(copy);
                return NULL;
            }
            copy->words[1] = (uintptr_t)
                s_scene_act_frame_record_clone_hooks.interface_retain(
                    (void*)source->words[1],
                    s_scene_act_frame_record_clone_hooks.context);
            if (copy->words[1] == 0u) {
                engine_heap_free(copy);
                return NULL;
            }
        }
    }
    else {
        memset(copy, 0, sizeof(*copy));
        if (s_scene_act_frame_record_clone_hooks.full_record_clone == NULL ||
            !s_scene_act_frame_record_clone_hooks.full_record_clone(
                source, clone_argument, copy,
                s_scene_act_frame_record_clone_hooks.context)) {
            engine_heap_free(copy);
            return NULL;
        }
    }
    if (output_value != NULL) *output_value = source->words[7];
    return copy;
}

void entity_visual_resource_load_hooks_set(
    const EntityVisualResourceLoadHooks* hooks)
{
    if (hooks == NULL) memset(&s_entity_visual_resource_load_hooks, 0,
                              sizeof(s_entity_visual_resource_load_hooks));
    else s_entity_visual_resource_load_hooks = *hooks;
}

// FUNCTION: LEGORR 0x00473690
/* EntityVisualResourceLoad (FUN_00473690): resolve the resource key, reuse
 * an existing cache entry where available, otherwise create one, then run
 * common preparation and the kind-four act-frame build before attachment. */
void* entity_visual_resource_load(void* scene_object)
{
    EntityVisualResourceLoadHooks* hooks =
        &s_entity_visual_resource_load_hooks;
    int kind;
    const char* name;
    void* resource = NULL;
    if (scene_object == NULL || hooks->kind_get == NULL ||
        hooks->name_get == NULL) return NULL;
    kind = hooks->kind_get(scene_object, hooks->context);
    name = hooks->name_get(scene_object, hooks->context);
    if (name != NULL && hooks->resource_lookup != NULL)
        resource = hooks->resource_lookup(name, hooks->context);
    if (resource == NULL && hooks->resource_create != NULL)
        resource = hooks->resource_create(scene_object, name, hooks->context);
    if (resource == NULL) return NULL;
    if (hooks->resource_prepare != NULL)
        hooks->resource_prepare(resource, scene_object, hooks->context);
    if (kind == 4 && hooks->animation_build != NULL)
        hooks->animation_build(resource, scene_object, hooks->context);
    if (hooks->resource_attach != NULL)
        hooks->resource_attach(scene_object, resource, hooks->context);
    return resource;
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

void* ddiscene_context_get(void* scene_object)
{
    void* owner_interface = NULL;
    void* result;
    SceneContextHooks* const hooks = &s_scene_context_hooks;
    if (scene_object == NULL || scene_object == hooks->shared_scene ||
        hooks->owner_interface_get == NULL ||
        hooks->owner_link_ensure == NULL) {
        return NULL;
    }
    if (!hooks->owner_interface_get(scene_object, &owner_interface,
                                    hooks->context) ||
        owner_interface == NULL) {
        return NULL;
    }
    result = scene_object_owner_link_ensure(owner_interface);
    if (hooks->interface_release != NULL) {
        hooks->interface_release(owner_interface, hooks->context);
    }
    return result;
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
static DDiSceneAnimationHookRecord* ddiscene_animation_hook_record_get(
    void* scene_interface)
{
    void** vtable;
    typedef void* (*GetProperty)(void*);
    GetProperty get_property;
    if (scene_interface == NULL) return NULL;
    vtable = *static_cast<void***>(scene_interface);
    if (vtable == NULL || vtable[0x1c / 4] == NULL) return NULL;
    get_property = reinterpret_cast<GetProperty>(vtable[0x1c / 4]);
    return static_cast<DDiSceneAnimationHookRecord*>(
        get_property(scene_interface));
}

static int ddiscene_animation_hook_record_set(
    void* scene_interface, DDiSceneAnimationHookRecord* record)
{
    void** vtable;
    typedef void (*SetProperty)(void*, void*);
    SetProperty set_property;
    if (scene_interface == NULL) return 0;
    vtable = *static_cast<void***>(scene_interface);
    if (vtable == NULL || vtable[0x18 / 4] == NULL) return 0;
    set_property = reinterpret_cast<SetProperty>(vtable[0x18 / 4]);
    set_property(scene_interface, record);
    return 1;
}

// FUNCTION: LEGORR 0x00475f70
/* DDiSceneAnimationHookCreate (FUN_00475f70): allocate the ten-word record
 * once, update only nonzero/non-null arguments, and replace its owned name. */
void ddiscene_animation_hook_create(void* scene_object, int callback,
                                    const char* name,
                                    const uint32_t* value_0c, int value_08,
                                    const uint32_t* value_10,
                                    const uint32_t* value_14,
                                    int value_18, int value_24,
                                    const uint32_t* value_20)
{
    DDiSceneAnimationHookRecord* record;
    DDiSceneAnimationHookRecordHooks* hooks =
        &s_ddiscene_animation_hook_record_hooks;
    if (scene_object == NULL) return;
    if (hooks->record_get != NULL && hooks->record_set != NULL) {
        record = hooks->record_get(scene_object, hooks->context);
    }
    else {
        record = ddiscene_animation_hook_record_get(scene_object);
    }
    if (record == NULL) {
        record = (DDiSceneAnimationHookRecord*)engine_heap_alloc(
            (long)sizeof(DDiSceneAnimationHookRecord));
        if (record == NULL) return;
        memset(record, 0, sizeof(*record));
        record->scene_object = scene_object;
        if (value_0c != NULL) record->value_0c = *value_0c;
        record->value_08 = value_08;
        if (value_10 != NULL) record->value_10 = *value_10;
        if (value_14 != NULL) record->value_14 = *value_14;
        record->value_18 = value_18;
        record->callback = callback;
        if (value_20 != NULL) record->value_20 = *value_20;
        record->value_24 = value_24;
        if (hooks->record_get != NULL && hooks->record_set != NULL) {
            hooks->record_set(scene_object, record, hooks->context);
        }
        else {
            if (!ddiscene_animation_hook_record_set(scene_object, record)) {
                engine_heap_free(record);
                return;
            }
        }
    }
    else {
        if (callback != 0) record->callback = callback;
        if (name != NULL && record->name != NULL) {
            engine_heap_free(record->name);
            record->name = NULL;
        }
        if (value_0c != NULL) record->value_0c = *value_0c;
        if (value_08 != 0) record->value_08 = value_08;
        if (value_10 != NULL) record->value_10 = *value_10;
        if (value_14 != NULL) record->value_14 = *value_14;
        if (value_18 != 0) record->value_18 = value_18;
        if (value_24 != 0) record->value_24 = value_24;
        if (value_20 != NULL) record->value_20 = *value_20;
    }
    if (name != NULL) {
        const size_t length = strlen(name);
        record->name = (char*)engine_heap_alloc((long)(length + 1u));
        if (record->name != NULL) memcpy(record->name, name, length + 1u);
    }
}

typedef void (*DdiTimedInterfaceCall)(void*, float, void*);
typedef void (*DdiTimedInterfaceUpdate)(void*, float, void*, void*);
typedef void (*DdiTimedInterfaceRelease)(void*);

/* SharedObjectAnimationAndTimedEventUpdate is implemented in the ledger
 * translation unit because its track layout is shared with game animation. */
namespace unported_ledger_stubs {
void ledger_stub_00487ba0_SharedObjectAnimationAndTimedEventUpdate(
    void* shared_object, float time, const void* hooks);
void* ledger_stub_00475e40_DDiSceneFrameRecordFind(
    void* action_interface, const char* action_name, int selector);
}

// FUNCTION: LEGORR 0x004897e0
/* DDiSceneTimedHookDispatch (FUN_004897e0): dispatch through the retained
 * renderer interfaces in the 0x20-byte frame record. */
void ddiscene_timed_hook_dispatch(void* hook_record, float time,
                                  void* output)
{
    SceneActFrameRecord* record = (SceneActFrameRecord*)hook_record;
    SceneActFrameRecord* shared;
    void** shared_children;
    void** record_children;
    uintptr_t i;
    if (record == NULL) return;
    shared = (SceneActFrameRecord*)record->words[0];
    if (shared == NULL) {
        if (record->words[3] != 0u) {
            unported_ledger_stubs::
                ledger_stub_00487ba0_SharedObjectAnimationAndTimedEventUpdate(
                (void*)record->words[1], time, NULL);
        }
        else if (record->words[2] != 0u) {
            void* object = (void*)record->words[2];
            void** vtable = *static_cast<void***>(object);
            if (vtable != NULL && vtable[0x38 / 4] != NULL) {
                DdiTimedInterfaceCall call =
                    reinterpret_cast<DdiTimedInterfaceCall>(vtable[0x38 / 4]);
                call(object, time, output);
            }
        }
        return;
    }
    if (shared->words[3] != 0u) {
        unported_ledger_stubs::
            ledger_stub_00487ba0_SharedObjectAnimationAndTimedEventUpdate(
            (void*)shared->words[1], time, NULL);
    }
    else if (shared->words[2] != 0u) {
        void* object = (void*)shared->words[2];
        void** vtable = *static_cast<void***>(object);
        if (vtable != NULL && vtable[0x38 / 4] != NULL) {
            DdiTimedInterfaceCall call =
                reinterpret_cast<DdiTimedInterfaceCall>(vtable[0x38 / 4]);
            call(object, time, output);
        }
    }
    shared_children = (void**)shared->words[5];
    record_children = (void**)record->words[5];
    for (i = 0u; i < record->words[6]; ++i) {
        void* child = shared_children != NULL ? shared_children[i] : NULL;
        if (child != NULL) {
            void* updated_interface = NULL;
            void** vtable = *static_cast<void***>(child);
            if (vtable != NULL) {
                if (vtable[0x5c / 4] != NULL) {
                    DdiTimedInterfaceUpdate update =
                        reinterpret_cast<DdiTimedInterfaceUpdate>(
                            vtable[0x5c / 4]);
                    update(child, time, output, &updated_interface);
                }
                if (vtable[0x74 / 4] != NULL) {
                    DdiTimedInterfaceCall call =
                        reinterpret_cast<DdiTimedInterfaceCall>(
                            vtable[0x74 / 4]);
                    call(child, time, output);
                }
            }
            if (updated_interface != NULL) {
                void** updated_vtable = *static_cast<void***>(updated_interface);
                if (updated_vtable != NULL && updated_vtable[2] != NULL) {
                    DdiTimedInterfaceRelease release =
                        reinterpret_cast<DdiTimedInterfaceRelease>(
                            updated_vtable[2]);
                    release(updated_interface);
                }
            }
        }
        if (record_children != NULL && record_children[i] != NULL) {
            void* record_child = record_children[i];
            void* child_output = NULL;
            void** vtable = *static_cast<void***>(record_child);
            if (vtable != NULL && vtable[0x38 / 4] != NULL) {
                DdiTimedInterfaceCall call =
                    reinterpret_cast<DdiTimedInterfaceCall>(vtable[0x38 / 4]);
                call(record_child, 0.0f, &child_output);
            }
        }
    }
    if (output != NULL) {
        float output_time;
        memcpy(&output_time, output, sizeof(output_time));
        if (shared->words[3] != 0u) {
            unported_ledger_stubs::
                ledger_stub_00487ba0_SharedObjectAnimationAndTimedEventUpdate(
                (void*)shared->words[1], output_time, NULL);
        }
        else if (shared->words[2] != 0u) {
            void* object = (void*)shared->words[2];
            void** vtable = *static_cast<void***>(object);
            if (vtable != NULL && vtable[0x38 / 4] != NULL) {
                DdiTimedInterfaceCall call =
                    reinterpret_cast<DdiTimedInterfaceCall>(vtable[0x38 / 4]);
                call(object, output_time, NULL);
            }
        }
    }
}

// FUNCTION: LEGORR 0x00475300
/* DDiSceneApplyActFrameTiming (FUN_00475300): only records with the pending
 * timing bit dispatch their current action frame's timed hook. Kind 4 finds
 * the selected frame through channel 0; kind 3 stores that channel directly. */
void ddiscene_apply_act_frame_timing(void* scene_object)
{
    void* selected = scene_object;
    uint32_t hook_record = (uint32_t)(uintptr_t)scene_object;
    float time;
    memcpy(&time, &scene_object, sizeof(time));
    if ((*(uint32_t*)((char*)scene_object + 0x14) & 0x20u) != 0u) {
        uint32_t kind = *(uint32_t*)((char*)scene_object + 0x10);
        if (kind == 4u) {
            selected = unported_ledger_stubs::
                ledger_stub_00475e40_DDiSceneFrameRecordFind(
                    scene_object,
                    **(const char***)((char*)scene_object + 0x0c), 0);
            if (selected != NULL) {
                hook_record = ddiscene_action_record_field_1c_get(selected);
                time = ddiscene_animation_duration_get(selected);
            }
        }
        else if (kind == 3u) {
            selected = *(void**)((char*)scene_object + 4);
            hook_record = ddiscene_action_record_field_1c_get(selected);
            time = ddiscene_animation_duration_get(selected);
        }
        if (hook_record != 0u) {
            ddiscene_timed_hook_dispatch((void*)hook_record, time, NULL);
        }
    }
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

static int ddiscene_ascii_case_equal(const char* first, const char* second)
{
    if (first == NULL || second == NULL) return 0;
    while (*first != '\0' && *second != '\0') {
        if (tolower((unsigned char)*first) !=
            tolower((unsigned char)*second)) return 0;
        ++first;
        ++second;
    }
    return *first == '\0' && *second == '\0';
}

/* DDiSceneFrameRecordFind (FUN_00475e40): search the selected action-frame
 * collection for the case-insensitive `ActFrame_<name>` record. */
void* ddiscene_frame_record_find(void* action_interface,
                                 const char* action_name, int selector)
{
    char wanted[1024];
    static const char prefix[] = "ActFrame_";
    size_t action_length;
    size_t prefix_length = sizeof(prefix) - 1u;
    unsigned int count;
    unsigned int index;
    DDiSceneFrameRecordFindHooks* const hooks =
        &s_ddiscene_frame_record_find_hooks;
    if (action_interface == NULL || action_name == NULL ||
        hooks->count_get == NULL || hooks->record_get == NULL ||
        hooks->record_name_get == NULL) return NULL;
    action_length = strlen(action_name);
    if (prefix_length + action_length >= sizeof(wanted)) return NULL;
    memcpy(wanted, prefix, prefix_length);
    memcpy(wanted + prefix_length, action_name, action_length + 1u);
    count = hooks->count_get(action_interface, selector, hooks->context);
    if (count > 4096u) count = 4096u;
    for (index = 0; index < count; ++index) {
        void* record = hooks->record_get(action_interface, selector, index,
                                         hooks->context);
        const char* name = record != NULL
            ? hooks->record_name_get(record, hooks->context) : NULL;
        if (ddiscene_ascii_case_equal(name, wanted)) {
            if (hooks->record_retain != NULL)
                hooks->record_retain(record, hooks->context);
            return record;
        }
    }
    return NULL;
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

/* DDiSceneLightCreate (FUN_004734f0): create the scene wrapper as kind 5,
 * ask the renderer to create/configure the light, and attach its animation
 * hook only after the renderer accepted the light. */
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

/* ComResourceEnumerationResultGet (FUN_00477310): slot +0x64 fills two
 * interface outputs. The first is queried through +0x68 and the second is
 * the returned interface; both slot +0x08 calls are part of the recovered
 * ownership sequence. */
// FUNCTION: LEGORR 0x00477310
void* com_resource_enumeration_result_get(void* resource_owner)
{
#if defined(_MSC_VER)
#define SCENE_COM_STDCALL __stdcall
#elif defined(__i386__) && (defined(__GNUC__) || defined(__clang__))
#define SCENE_COM_STDCALL __attribute__((stdcall))
#else
#define SCENE_COM_STDCALL
#endif
    typedef void (SCENE_COM_STDCALL *ComMethodOutPair)(void*, void*);
    typedef void (SCENE_COM_STDCALL *ComMethodOutOne)(void*, void*);
    typedef void (SCENE_COM_STDCALL *ComMethodNoOut)(void*);
    void* resource_interface = NULL;
    void** vtable;
    void* outputs[2];
    void* first_result;
    void* second_result;
    if (resource_owner == NULL) return NULL;
    memcpy(&resource_interface, (char*)resource_owner + 0x10,
           sizeof(resource_interface));
    if (resource_interface == NULL) return NULL;
    vtable = *(void***)resource_interface;
    if (vtable == NULL || vtable[25] == NULL) return NULL;
    outputs[0] = NULL;
    outputs[1] = NULL;
    ((ComMethodOutPair)vtable[25])(
        resource_interface, outputs);
    first_result = outputs[0];
    if (first_result == NULL) return outputs[1];
    vtable = *(void***)first_result;
    if (vtable == NULL) return outputs[1];
    if (vtable[2] != NULL)
        ((ComMethodNoOut)vtable[2])(first_result);
    if (vtable[26] != NULL) {
        ((ComMethodOutOne)vtable[26])(
            first_result, &outputs[1]);
    }
    second_result = outputs[1];
    if (second_result != NULL) {
        vtable = *(void***)second_result;
        if (vtable != NULL && vtable[2] != NULL)
            ((ComMethodNoOut)vtable[2])(second_result);
    }
#undef SCENE_COM_STDCALL
    return second_result;
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

/* DDiSceneObjectDestroy (FUN_00472a50): renderer-specific vtable resources
 * are released by the installed destructor callback. */
void ddiscene_object_destroy_with_mode(void* scene_object, int mode)
{
    if (scene_object == NULL) return;
    if (s_ddiscene_object_destroy_hook == NULL) return;
    s_ddiscene_object_destroy_hook(scene_object, mode,
                                   s_ddiscene_object_destroy_context);
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

/* DDiSceneObjectLengthGet (FUN_00475390): kind 4 resolves its selected
 * ActFrame record through channel 0; kind 3 already carries the channel. */
// FUNCTION: LEGORR 0x00475390
float ddiscene_object_length_get(void* scene_object)
{
    uint32_t kind;
    void* interface_object;
    if (scene_object == NULL) return 0.0f;
    kind = *(uint32_t*)((char*)scene_object + 0x10);
    if (kind == 4u) {
        void* channel = *(void**)((char*)scene_object + 4);
        void* action_slot = *(void**)((char*)scene_object + 0x0c);
        const char* action_name = action_slot != NULL
            ? *(const char**)action_slot : NULL;
        interface_object = channel != NULL && action_name != NULL
            ? ddiscene_frame_record_find(channel, action_name, 0) : NULL;
        if (interface_object != NULL)
            return ddiscene_animation_duration_get(interface_object);
    }
    else if (kind == 3u) {
        interface_object = *(void**)((char*)scene_object + 4);
        if (interface_object != NULL)
            return ddiscene_animation_duration_get(interface_object);
    }
    return 0.0f;
}

/* DDiSceneObjectFramePositionGet (FUN_004753f0): follow the same kind
 * dispatch, then read the selected frame record's +0x0c field. */
// FUNCTION: LEGORR 0x004753f0
uint32_t ddiscene_object_frame_position_get(void* scene_object)
{
    uint32_t kind;
    void* interface_object = NULL;
    if (scene_object == NULL) return 0u;
    kind = *(uint32_t*)((char*)scene_object + 0x10);
    if (kind == 4u) {
        void* channel = *(void**)((char*)scene_object + 4);
        void* action_slot = *(void**)((char*)scene_object + 0x0c);
        const char* action_name = action_slot != NULL
            ? *(const char**)action_slot : NULL;
        if (channel != NULL && action_name != NULL)
            interface_object = ddiscene_frame_record_find(
                channel, action_name, 0);
    }
    else if (kind == 3u) {
        interface_object = *(void**)((char*)scene_object + 4);
    }
    return interface_object != NULL
        ? ddiscene_action_record_field_0c_get(interface_object) : 0u;
}

/* DDiSceneAnimationCallbacksSet (FUN_00475c00): allocate the four-callback
 * record once, then replace only non-null callback arguments. Native pointer
 * width keeps the record usable in the 64-bit reconstruction. */
void ddiscene_animation_callbacks_set(void* scene_object,
                                      void* callback_a, void* callback_b,
                                      void* callback_c, void* callback_d)
{
    void** callbacks = NULL;
    void* values[4];
    unsigned int i;
    if (scene_object == NULL) return;
    memcpy(&callbacks, (char*)scene_object + 0x0c, sizeof(callbacks));
    if (callbacks == NULL) {
        callbacks = (void**)engine_heap_alloc((long)(4 * sizeof(void*)));
        if (callbacks == NULL) return;
        memset(callbacks, 0, 4 * sizeof(void*));
        memcpy((char*)scene_object + 0x0c, &callbacks, sizeof(callbacks));
    }
    values[0] = callback_a;
    values[1] = callback_b;
    values[2] = callback_c;
    values[3] = callback_d;
    for (i = 0; i < 4; ++i) {
        if (values[i] != NULL) callbacks[i] = values[i];
    }
}

void ddiscene_payload_release_hooks_set(
    const DDiScenePayloadReleaseHooks* hooks)
{
    if (hooks == NULL) memset(&s_ddiscene_payload_release_hooks, 0,
                              sizeof(s_ddiscene_payload_release_hooks));
    else s_ddiscene_payload_release_hooks = *hooks;
}

/* DDiScenePayloadRelease (FUN_00475c80): release kind-specific resources,
 * free the payload, and clear the scene's +0x0c slot. */
// FUNCTION: LEGORR 0x00475c80
void ddiscene_payload_release(void* scene_object)
{
    void* payload;
    void* owner;
    uint32_t kind;
    void** fields;
    if (scene_object == NULL) return;
    memcpy(&payload, (char*)scene_object + 0x0c, sizeof(payload));
    if (payload == NULL) return;
    memcpy(&kind, (char*)scene_object + 0x10, sizeof(kind));
    memcpy(&owner, (char*)scene_object + 4, sizeof(owner));
    fields = (void**)payload;
    if (kind == 4u) {
        if (fields[0] != NULL) engine_heap_free(fields[0]);
    }
    else if (kind == 5u && fields[1] != NULL) {
        if (s_ddiscene_payload_release_hooks.interface_release != NULL) {
            s_ddiscene_payload_release_hooks.interface_release(
                fields[1], s_ddiscene_payload_release_hooks.context);
        }
        else {
            ddiscene_payload_interface_release(fields[1]);
        }
    }
    else if (kind == 1u && fields[2] != NULL) {
        if (s_ddiscene_payload_release_hooks.kind1_detach != NULL) {
            s_ddiscene_payload_release_hooks.kind1_detach(
                owner, fields[2], s_ddiscene_payload_release_hooks.context);
        }
        else if (owner != NULL) {
            void** vtable = *(void***)owner;
            if (vtable != NULL && vtable[0xa0 / 4] != NULL) {
                ddiscene_payload_owner_detach_call(
                    owner, fields[2], vtable[0xa0 / 4]);
            }
        }
        if (s_ddiscene_payload_release_hooks.interface_release != NULL) {
            s_ddiscene_payload_release_hooks.interface_release(
                fields[2], s_ddiscene_payload_release_hooks.context);
        }
        else {
            ddiscene_payload_interface_release(fields[2]);
        }
    }
    else if (kind == 8u && fields[3] != NULL &&
             s_ddiscene_payload_release_hooks.kind8_detach != NULL) {
        s_ddiscene_payload_release_hooks.kind8_detach(
            fields[3], owner, s_ddiscene_payload_release_hooks.context);
    }
    engine_heap_free(payload);
    payload = NULL;
    memcpy((char*)scene_object + 0x0c, &payload, sizeof(payload));
}

/* DDiSceneResourceNameResolve (FUN_00476770): find the DDiScene_ marker,
 * skip non-alpha bytes in the suffix, and return an owned copy. The corpus
 * uses integer 1 when the marker is absent; retain that sentinel. */
char* ddiscene_resource_name_resolve(const char* name)
{
    static const char prefix[] = "DDiScene_";
    const char* suffix;
    size_t length;
    char* resolved;
    if (name == NULL) return (char*)1;
    suffix = strstr(name, prefix);
    if (suffix == NULL) return (char*)1;
    suffix += sizeof(prefix) - 1;
    while (*suffix != '\0' && !isalpha((unsigned char)*suffix)) ++suffix;
    if (*suffix == '\0') return (char*)1;
    length = strlen(suffix);
    resolved = (char*)engine_heap_alloc((long)(length + 1));
    if (resolved == NULL) return (char*)1;
    memcpy(resolved, suffix, length + 1);
    return resolved;
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

// FUNCTION: LEGORR 0x00475140
/* DDiSceneFrameUpdateAndCallbacks (FUN_00475140): select the kind-specific
 * frame, consume the action-entry sample bit, update its timing record, run
 * timed dispatch, and signal the registered end callback on frame overflow. */
float ddiscene_frame_update_and_callbacks(void* scene_object, float time)
{
    void* frame_interface = NULL;
    void* action_slot;
    const char* action_name;
    uint32_t kind;
    uint32_t flags;
    uint32_t timing_suppressed;
    const char* sample_name;
    uint32_t frame_count;
    uint32_t last_frame;
    float length;
    float remainder = 0.0f;
    float last_frame_time;
    void* timing_record;

    if (scene_object == NULL) return 0.0f;
    kind = *(uint32_t*)((char*)scene_object + 0x10);
    flags = *(uint32_t*)((char*)scene_object + 0x14);
    if (kind == 4u) {
        action_slot = *(void**)((char*)scene_object + 0x0c);
        action_name = action_slot != NULL ? *(const char**)action_slot : NULL;
        if (action_name != NULL) {
            frame_interface = ddiscene_frame_record_find(
                *(void**)((char*)scene_object + 4), action_name, 0);
        }
        if (frame_interface != NULL) {
            timing_record = (void*)(uintptr_t)
                ddiscene_action_record_field_1c_get(frame_interface);
        }
        if (frame_interface != NULL && (flags & 2u) != 0u) {
            sample_name = ddiscene_animation_sample_name_get(frame_interface);
            if (sample_name != NULL && s_ddiscene_sample_enabled != 0) {
                ddiscene_sample_hook_dispatch(sample_name, scene_object);
            }
            *(uint32_t*)((char*)scene_object + 0x14) = flags & ~2u;
            flags &= ~2u;
        }
    }
    else if (kind == 3u) {
        frame_interface = *(void**)((char*)scene_object + 4);
        if (frame_interface != NULL) {
            timing_record = (void*)(uintptr_t)
                ddiscene_action_record_field_1c_get(frame_interface);
        }
    }
    if (frame_interface == NULL) return 0.0f;

    frame_count = ddiscene_action_record_field_0c_get(frame_interface);
    length = ddiscene_object_length_get(scene_object);

    /* FUN_00475f70 updates the frame interface's stored time value. */
    ddiscene_animation_hook_create(frame_interface, 0, NULL, NULL, 0,
                                   NULL, (const uint32_t*)&time, 0, 0, NULL);

    timing_suppressed = flags & 8u;
    if (frame_count != 0u) {
        --frame_count;
        if ((float)frame_count < time) {
            remainder = time - (float)frame_count;
            timing_suppressed = 0u;
        }
    }
    if (timing_suppressed == 0u) {
        last_frame = ddiscene_animation_last_frame_get(frame_interface);
        last_frame_time = (float)last_frame;
        if (frame_count != 1u) {
            ddiscene_timed_hook_dispatch(timing_record, time, NULL);
            if (s_ddiscene_end_hook != NULL && last_frame != 0u &&
                length < last_frame_time && last_frame_time <= time) {
                s_ddiscene_end_hook(scene_object, s_ddiscene_end_context);
                return remainder;
            }
        }
    }
    else {
        *(uint32_t*)((char*)scene_object + 0x14) = flags | 0x20u;
    }
    return remainder;
}

// FUNCTION: LEGORR 0x00475120
/* DDiSceneAdvanceByTime (FUN_00475120): add the delta to the current object
 * length and discard the frame update's remainder, as the target does. */
float ddiscene_advance_by_time(void* scene_object, float delta)
{
    float current;
    if (scene_object == NULL) return 0.0f;
    current = ddiscene_object_length_get(scene_object);
    return ddiscene_frame_update_and_callbacks(scene_object, current + delta);
}

/* TypedChildResourceScalarGet (FUN_004757f0): only type-4 scene objects with
 * a non-null resource name can resolve a child property. */
// FUNCTION: LEGORR 0x004757f0
float typed_child_resource_scalar_get(void* scene_object)
{
    void* record;
    if (scene_object == NULL ||
        *(uint32_t*)((char*)scene_object + 0x10) != 4u) {
        return 0.0f;
    }
    record = ddiscene_object_frame_record_get(scene_object);
    return record != NULL ? scene_property_float_at14_get(record) : 0.0f;
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

/* SceneObjectFirstNonKind6ChildGet (FUN_00475830): walk the child-interface
 * chain until its owner property has a kind other than six. */
// FUNCTION: LEGORR 0x00475830
void* scene_object_first_non_kind6_child_get(void* child_interface)
{
    SceneChildSearchHooks* hooks = &s_scene_child_search_hooks;
    void* current = child_interface;
    void* result = NULL;
    uint32_t guard = 0;
    while (current != NULL && result == NULL && guard < 4096u) {
        void* property;
        void* next;
        if (hooks->property_get == NULL || hooks->next_get == NULL) return NULL;
        property = hooks->property_get(current, hooks->context);
        if (property != NULL && *(uint32_t*)((char*)property + 0x10) != 6u) {
            result = property;
        }
        next = hooks->next_get(current, hooks->context);
        if (next != NULL && hooks->release != NULL) {
            hooks->release(next, hooks->context);
        }
        current = next;
        ++guard;
    }
    return result;
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

/* SceneObjectSetFormattedString (FUN_00476270): format into a bounded stack
 * scratch buffer, copy into engine-owned storage, set the interface string,
 * then publish the animation hook in the recovered order. */
void scene_object_set_formatted_string(void* scene_object,
                                       const char* format, ...)
{
    char local[1024];
    char* owned;
    size_t length;
    va_list arguments;
    if (scene_object == NULL || format == NULL) return;
    va_start(arguments, format);
#if defined(_MSC_VER)
    _vsnprintf(local, sizeof(local) - 1, format, arguments);
#else
    vsnprintf(local, sizeof(local) - 1, format, arguments);
#endif
    va_end(arguments);
    local[sizeof(local) - 1] = '\0';
    length = strlen(local);
    owned = (char*)engine_heap_alloc((long)(length + 1));
    if (owned == NULL) return;
    memcpy(owned, local, length + 1);
    if (s_scene_formatted_string_hooks.string_set != NULL) {
        s_scene_formatted_string_hooks.string_set(
            scene_object, owned, s_scene_formatted_string_hooks.context);
    }
    if (s_scene_formatted_string_hooks.animation_string_bind != NULL) {
        s_scene_formatted_string_hooks.animation_string_bind(
            scene_object, owned, s_scene_formatted_string_hooks.context);
    }
    if (s_scene_formatted_string_hooks.string_set == NULL &&
        s_scene_formatted_string_hooks.animation_string_bind == NULL) {
        engine_heap_free(owned);
    }
}

void render_interface_pair_hooks_set(const RenderInterfacePairHooks* hooks)
{
    if (hooks == NULL) memset(&s_render_interface_pair_hooks, 0,
                              sizeof(s_render_interface_pair_hooks));
    else s_render_interface_pair_hooks = *hooks;
}

/* RenderInterfacePairUpdate (FUN_00476220): preserve the two-interface
 * operation as one backend callback so it can issue the recovered query,
 * parent update, and pair-set sequence. */
void render_interface_pair_update(void* first, void* second)
{
    if (s_render_interface_pair_hooks.pair_update != NULL) {
        s_render_interface_pair_hooks.pair_update(
            first, second, s_render_interface_pair_hooks.context);
    }
}

/* DDiSceneObjectAttachOrUnlink (FUN_00475710): pairs both stored interfaces
 * against a source pair, or asks the backend to unlink each current one. */
// FUNCTION: LEGORR 0x00475710
void ddiscene_scene_object_attach_or_unlink(void* pair_record,
                                            void* source_pair_record)
{
    void* first;
    void* second;
    if (pair_record == NULL) return;
    memcpy(&first, pair_record, sizeof(first));
    memcpy(&second, (char*)pair_record + 2 * sizeof(void*), sizeof(second));
    if (source_pair_record == NULL) {
        if (first != NULL &&
            s_render_interface_pair_hooks.interface_unlink != NULL) {
            s_render_interface_pair_hooks.interface_unlink(
                first, s_render_interface_pair_hooks.context);
        }
        if (second != NULL &&
            s_render_interface_pair_hooks.interface_unlink != NULL) {
            s_render_interface_pair_hooks.interface_unlink(
                second, s_render_interface_pair_hooks.context);
        }
        return;
    }
    {
        void* source_first;
        void* source_second;
        memcpy(&source_first, source_pair_record, sizeof(source_first));
        memcpy(&source_second,
               (char*)source_pair_record + 2 * sizeof(void*),
               sizeof(source_second));
        render_interface_pair_update(source_first, first);
        render_interface_pair_update(source_second, second);
    }
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

EngineTaskCallbackRecord* engine_task_callback_record_allocate_and_initialize(
    void* manager, void* owner, void* callback, int mode,
    const EngineTaskCallbackHooks* hooks)
{
    EngineTaskCallbackRecord* record;
    void* resource_interface = NULL;
    if (hooks != NULL && hooks->record_allocate != NULL)
        record = hooks->record_allocate(hooks->context);
    else
        record = (EngineTaskCallbackRecord*)calloc(1, sizeof(*record));
    if (record == NULL) return NULL;
    memset(record, 0, sizeof(*record));
    if (hooks != NULL && hooks->global_initialize != NULL)
        hooks->global_initialize(hooks->context);
    if (hooks != NULL && hooks->manager_event != NULL)
        hooks->manager_event(hooks->context, manager);
    if (hooks != NULL && hooks->resource_interface_get != NULL)
        resource_interface = hooks->resource_interface_get(hooks->context,
                                                            record);
    if (hooks != NULL && hooks->owner_bind != NULL)
        hooks->owner_bind(hooks->context, owner, resource_interface);
    if (hooks != NULL && hooks->callback_attach != NULL)
        hooks->callback_attach(hooks->context, resource_interface, record);
    record->resource_interface = resource_interface;
    record->callback = callback;
    record->handler = callback;
    record->self = record;
    record->owner = owner;
    record->value = 1u;
    if (mode == 1) record->flags |= 2u;
    else if (mode == 2) record->flags |= 4u;
    return record;
}

void scene_object_record_retain_hook_set(SceneObjectRecordRetainHook hook,
                                         void* context)
{
    s_scene_object_record_retain_hook = hook;
    s_scene_object_record_retain_context = context;
}

// FUNCTION: LEGORR 0x00408620
void scene_object_record_clone_retain_handles(
    const SceneObjectRecord* source, SceneObjectRecord* destination)
{
    unsigned int i;
    if (source == NULL || destination == NULL) return;
    for (i = 0; i < 0x53u; ++i) destination->words[i] = source->words[i];
    destination->words[0x52] = (void*)((uintptr_t)destination->words[0x52]
                                      & ~(uintptr_t)1u);
    if (s_scene_object_record_retain_hook != NULL) {
        if (source->words[1] != NULL)
            destination->words[1] = s_scene_object_record_retain_hook(
                s_scene_object_record_retain_context, source->words[1]);
        if (source->words[0x1f] != NULL)
            destination->words[0x1f] = s_scene_object_record_retain_hook(
                s_scene_object_record_retain_context, source->words[0x1f]);
    }
}

void scene_frame_apply_hooks_set(const SceneFrameApplyHooks* hooks)
{
    if (hooks == NULL) memset(&s_scene_frame_apply_hooks, 0,
                              sizeof(s_scene_frame_apply_hooks));
    else s_scene_frame_apply_hooks = *hooks;
}

static float scene_frame_vector_length(const float vector[3])
{
    return (float)sqrt((double)(vector[0] * vector[0] +
                                 vector[1] * vector[1] +
                                 vector[2] * vector[2]));
}

// FUNCTION: LEGORR 0x0040bb90
void scene_frame_from_triplet_apply(void* object, float projection_scale,
                                   const float first[3],
                                   const float second[3],
                                   const float third[3])
{
    float forward[3];
    float normal[3];
    float transformed_second[3];
    float transformed_third[3];
    float projection[16];
    float forward_length, normal_length;
    float projection_height;
    int i;
    if (object == NULL || first == NULL || second == NULL || third == NULL)
        return;
    for (i = 0; i < 3; ++i) forward[i] = second[i] - first[i];
    normal[0] = (third[2] - second[2]) * forward[1] -
                (third[1] - second[1]) * forward[2];
    normal[1] = (third[0] - second[0]) * forward[2] -
                (third[2] - second[2]) * forward[0];
    normal[2] = (third[1] - second[1]) * forward[0] -
                (third[0] - second[0]) * forward[1];
    forward_length = scene_frame_vector_length(forward);
    normal_length = scene_frame_vector_length(normal);
    projection_height = forward[1];
    for (i = 0; i < 3; ++i) {
        forward[i] /= forward_length;
        normal[i] = -normal[i] / normal_length;
    }
    if (s_scene_frame_apply_hooks.orientation_set != NULL)
        s_scene_frame_apply_hooks.orientation_set(
            s_scene_frame_apply_hooks.context, object, 0, normal, forward);
    if (s_scene_frame_apply_hooks.position_set != NULL)
        s_scene_frame_apply_hooks.position_set(
            s_scene_frame_apply_hooks.context, object, 0, first);
    memcpy(transformed_second, second, sizeof(transformed_second));
    memcpy(transformed_third, third, sizeof(transformed_third));
    if (s_scene_frame_apply_hooks.point_transform != NULL) {
        s_scene_frame_apply_hooks.point_transform(
            s_scene_frame_apply_hooks.context, object, transformed_second,
            second);
        s_scene_frame_apply_hooks.point_transform(
            s_scene_frame_apply_hooks.context, object, transformed_third,
            third);
    }
    perspective_projection_matrix_set(
        projection, projection_scale, projection_scale, projection_height,
        transformed_third[0], transformed_third[2]);
    if (s_scene_frame_apply_hooks.projection_set != NULL)
        s_scene_frame_apply_hooks.projection_set(
            s_scene_frame_apply_hooks.context, object, 1, projection);
}

/* CellVisualChannelRelay (FUN_0040b880): resolve the two root resources into
 * the current channel slot, then apply the selected and opposite corner
 * frames. The record follows the recovered dword layout on x86 while its
 * pointer-bearing banks widen naturally in the portable build. */
// FUNCTION: LEGORR 0x0040b880
void scene_cell_visual_channels_apply(SceneCellVisualRelayRecord* record,
                                      const float corner_points[12],
                                      uint32_t lookup_selector,
                                      uint32_t corner_index, float scale,
                                      float uniform_scale)
{
    static const uint8_t corner_triplets[4][3] = {
        { 3, 0, 2 }, { 0, 1, 3 }, { 1, 2, 0 }, { 2, 3, 1 }
    };
    void* scene_objects[2];
    float child_value = 0.0f;
    uint32_t child_value_bits;
    uint32_t index;
    uint32_t first_corner;
    uint32_t opposite_corner;
    if (record == NULL || corner_points == NULL) return;
    index = record->channel_cursor;
    if (index >= 100u) return;
    scene_objects[0] = entity_visual_resource_load(record->resource_roots[0]);
    scene_objects[1] = entity_visual_resource_load(record->resource_roots[1]);
    record->channel_resources[0][index] = scene_objects[0];
    record->channel_resources[1][index] = scene_objects[1];
    if (record->lookup_table != NULL &&
        record->lookup_table->entries != NULL) {
        uint32_t lookup_index =
            (lookup_selector & 0x0fu) * record->lookup_table->stride +
            ((lookup_selector & 0xffu) >> 4);
        child_value_bits = record->lookup_table->entries[lookup_index];
        memcpy(&child_value, &child_value_bits, sizeof(child_value));
    }
    first_corner = corner_index & 3u;
    opposite_corner = (first_corner + 2u) & 3u;
    scene_rotation_flags_apply(scene_objects[0], record->projection_scale,
        corner_points + corner_triplets[first_corner][0] * 3u,
        corner_points + corner_triplets[first_corner][1] * 3u,
        corner_points + corner_triplets[first_corner][2] * 3u,
        scale, uniform_scale, child_value, 0u);
    scene_rotation_flags_apply(scene_objects[1], record->projection_scale,
        corner_points + corner_triplets[opposite_corner][0] * 3u,
        corner_points + corner_triplets[opposite_corner][1] * 3u,
        corner_points + corner_triplets[opposite_corner][2] * 3u,
        scale, uniform_scale, child_value, 0u);
    record->channel_cursor = index + 1u;
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

/* SceneRotationFlagsApply (FUN_0040ba00): argument 2 is passed through to
 * SceneFrameFromTripletApply as its projection scale; the remaining scalar
 * and bit arguments retain their original stack order. */
// FUNCTION: LEGORR 0x0040BA00
void scene_rotation_flags_apply(void* object, float projection_scale,
                                const float first[3],
                                const float second[3], const float third[3],
                                float scale, float uniform_scale,
                                float child_value, uint8_t flags)
{
    unsigned int i;
    SceneRotationApplyHooks* hooks = &s_scene_rotation_apply_hooks;
    if (object == NULL || first == NULL || second == NULL || third == NULL)
        return;
    if (hooks->frame_transform_apply != NULL) {
        hooks->frame_transform_apply(hooks->context, object, projection_scale,
                                     first, second, third);
    } else {
        /* The renderer's projection-matrix operation is not exposed by the
         * portable scene interface; preserve the available frame update. */
        scene_frame_from_triplet_apply(object, projection_scale,
                                       first, second, third);
    }
    if (hooks->apply != NULL)
        hooks->apply(hooks->context, object, 0x473b40u, 0, 0, 0, 0, 0);
    i = 0u;
    while (hooks->child_count != NULL &&
           i < hooks->child_count(hooks->context, object)) {
        if (hooks->apply != NULL)
            hooks->apply(hooks->context, object, 0x474da0u, i,
                         child_value, 0, 0, 0);
        ++i;
    }
    if (hooks->apply == NULL) return;
    hooks->apply(hooks->context, object, 0x4755e0u, 1,
                 -0.02f, -0.02f, 0, 0);
    hooks->apply(hooks->context, object, 0x4755b0u, 1,
                 1.015f, 1.015f, scale, 0);
    hooks->apply(hooks->context, object, 0x4750b0u, 0,
                 uniform_scale, uniform_scale, uniform_scale, 1.0f);
    if (flags & 1u) hooks->apply(hooks->context, object, 0x475580u, 1,
                                 1, 0, 0, 1.57079637f);
    if (flags & 2u) hooks->apply(hooks->context, object, 0x475580u, 1,
                                 1, 0, 0, -1.57079637f);
    if (flags & 4u) hooks->apply(hooks->context, object, 0x475580u, 1,
                                 0, 1, 0, 1.57079637f);
    if (flags & 8u) hooks->apply(hooks->context, object, 0x475580u, 1,
                                 0, 1, 0, -1.57079637f);
    if (flags & 0x10u) hooks->apply(hooks->context, object, 0x475580u, 1,
                                   0, 0, 1, 1.57079637f);
    if (flags & 0x20u) hooks->apply(hooks->context, object, 0x475580u, 1,
                                   0, 0, 1, -1.57079637f);
    if (flags & 0x80u) hooks->apply(hooks->context, object, 0x4755e0u, 2,
                                   0, -10, 0, 0);
    if (flags & 0x40u) hooks->apply(hooks->context, object, 0x4755e0u, 2,
                                   0, 10, 0, 0);
}

void scene_rotation_flags_apply(void* object, const float first[3],
                                const float second[3], const float third[3],
                                float scale, float uniform_scale,
                                float child_value, uint8_t flags)
{
    scene_rotation_flags_apply(object, 0.0f, first, second, third, scale,
                               uniform_scale, child_value, flags);
}
static RenderResourceBindingHooks s_render_resource_binding_hooks;
static float s_viewport_left;
static float s_viewport_top;
static float s_viewport_right;
static float s_viewport_bottom;

void viewport_rectangle_set(float left, float top, float right, float bottom)
{
    s_viewport_left = left;
    s_viewport_top = top;
    s_viewport_right = right;
    s_viewport_bottom = bottom;
}

void viewport_dimensions_query_hook_set(ViewportDimensionsQueryFn query,
                                        void* context)
{
    s_viewport_dimensions_query = query;
    s_viewport_dimensions_context = context;
}

void model_transform_copy_hooks_set(ModelTransformReadHook read_transform,
                                    SceneTransformWriteHook write_transform,
                                    void* context)
{
    s_model_transform_read_hook = read_transform;
    s_scene_transform_write_hook = write_transform;
    s_model_transform_context = context;
}

// ModelTransformCopyToSceneNode (FUN_00435e35) copies transform state only
// for mode-3 views. The callbacks bridge the native model and scene backends.
// FUNCTION: LEGORR 0x00435e35
void model_transform_copy_to_scene_node(void* view_ptr, void* model_record)
{
    ViewContext* view = (ViewContext*)view_ptr;
    void* model;
    float position[3];
    float orientation[4];
    if (view == NULL || model_record == NULL || view->mode != 3 ||
        s_model_transform_read_hook == NULL ||
        s_scene_transform_write_hook == NULL) return;
    model = *(void**)((char*)model_record + 0x24);
    if (model == NULL) return;
    if (!s_model_transform_read_hook(s_model_transform_context, model,
                                     position, orientation)) return;
    s_scene_transform_write_hook(s_model_transform_context, &view->cam_world,
                                 position, orientation);
}

void camera_view_context_close_hooks_set(CameraViewContextReleaseHook release,
                                         CameraViewContextFreeHook free_view,
                                         void* context)
{
    s_view_context_release_hook = release;
    s_view_context_free_hook = free_view;
    s_view_context_close_context = context;
}

// CameraViewContextClose (FUN_00435dc8) releases the world camera first,
// conditionally releases follow and third handles, then frees the context.
// FUNCTION: LEGORR 0x00435dc8
void camera_view_context_close(void* view)
{
    int mode;
    if (view == NULL) return;
    mode = *(int*)view;
    if (s_view_context_release_hook != NULL) {
        s_view_context_release_hook(s_view_context_close_context, view, 9);
        if (mode != 3) {
            s_view_context_release_hook(s_view_context_close_context,
                                        view, 10);
            s_view_context_release_hook(s_view_context_close_context,
                                        view, 12);
        }
    }
    if (s_view_context_free_hook != NULL) {
        s_view_context_free_hook(s_view_context_close_context, view);
    }
}

void animation_transform_hooks_set(AnimationObjectTransformApplyHook orient,
                                   RenderNodePositionApplyHook position,
                                   void* context)
{
    s_animation_orientation_hook = orient;
    s_render_node_position_hook = position;
    s_animation_transform_context = context;
}

// AnimationObjectTransformSet (FUN_00406dc0).
// FUNCTION: LEGORR 0x00406dc0
void animation_object_transform_set(void* record, float first, float second)
{
    static const float orientation[4] = { 0.0f, 0.0f, 0.0f, -1.0f };
    void* render_object;
    if (record == NULL || s_animation_orientation_hook == NULL) return;
    render_object = *(void**)((char*)record + 4);
    if (render_object == NULL) return;
    s_animation_orientation_hook(s_animation_transform_context,
                                 render_object, first, second, orientation);
}

// RenderNodeSetPositionFromCallback (FUN_00406df0).
// FUNCTION: LEGORR 0x00406df0
void render_node_position_from_callback(void* record, float x, float y,
                                        RenderNodePositionValueFn value_fn,
                                        void* context)
{
    void* render_object;
    float z;
    if (record == NULL || value_fn == NULL ||
        s_render_node_position_hook == NULL) return;
    render_object = *(void**)((char*)record + 4);
    if (render_object == NULL) return;
    z = value_fn(x, y, context);
    s_render_node_position_hook(s_animation_transform_context,
                                render_object, x, y, z);
}

void scene_matrix_apply_hook_set(SceneMatrixApplyHook hook, void* context)
{
    s_scene_matrix_apply_hook = hook;
    s_scene_matrix_apply_context = context;
}

// FUNCTION: LEGORR 0x00487cb0
/* AnimationTrackFindKeyframeAndFraction helper. The track contains
 * a count at +0x24 and 0x28-byte key records at +0x18; each key's time is at
 * +0x24. Keep the original lower-bound binary search and interpolation
 * denominator (the next key's timestamp) intact. */
float animation_track_find_keyframe_and_fraction(const void* track,
                                                 float time,
                                                 uint16_t* keyframe_out)
{
    const unsigned char* bytes =
        static_cast<const unsigned char*>(track);
    const unsigned char* keys;
    uint16_t count;
    uint32_t low = 0;
    uint32_t high;
    uint32_t middle;
    uint16_t key_time;
    uint16_t next_time;
    if (track == NULL || keyframe_out == NULL) return 0.0f;
    memcpy(&count, bytes + 0x24, sizeof(count));
    memcpy(&keys, bytes + 0x18, sizeof(keys));
    high = (uint32_t)count;
    do {
        middle = (high + low) / 2u;
        memcpy(&key_time, keys + middle * 0x28u + 0x24,
               sizeof(key_time));
        if (middle == low) break;
        if (time < (float)key_time) high = middle;
        else low = middle;
    } while (1);
    *keyframe_out = (uint16_t)middle;
    memcpy(&next_time, keys + middle * 0x28u + 0x4c,
           sizeof(next_time));
    return (time - (float)key_time) /
           ((float)next_time - (float)key_time);
}

// FUNCTION: LEGORR 0x00488170
/* AnimationTrackKeysInterpolate helper. */
void animation_track_keys_interpolate(void* owner, const void* track,
                                      uint32_t keyframe, float fraction)
{
    const unsigned char* bytes =
        static_cast<const unsigned char*>(track);
    const float* key;
    uint32_t index = keyframe & 0xffffu;
    float position[3];
    float rotation[3];
    float scale[3];
    unsigned int axis;
    if (owner == NULL || track == NULL) return;
    memcpy(&key, bytes + 0x18, sizeof(key));
    key = reinterpret_cast<const float*>(
        reinterpret_cast<const unsigned char*>(key) + index * 0x28u);
    for (axis = 0; axis < 3; ++axis) {
        position[axis] = key[axis] + (key[10 + axis] - key[axis]) * fraction;
        rotation[axis] = key[3 + axis] +
                         (key[13 + axis] - key[3 + axis]) * fraction;
        scale[axis] = key[6 + axis] + (key[16 + axis] - key[6 + axis]) * fraction;
    }
    scene_node_transform_matrix_apply(owner, track, position, rotation, scale);
}

/* SceneNodeTransformMatrixApply adapter. The normal branch uses the
 * observed owner table and +0x38 matrix setter. The retail flag-4 branch
 * first queries interfaces through vtable slots +0x5c/+0x74; their argument
 * and returned-object contracts are not represented by the current scene
 * API, so this helper leaves that branch untouched and is not the mapped
 * retail target. */
void scene_node_transform_matrix_apply(void* owner, const void* track,
                                       const float position[3],
                                       const float rotation[3],
                                       const float scale[3])
{
    const unsigned char* bytes =
        static_cast<const unsigned char*>(track);
    void** nodes;
    void* node;
    void** vtable;
    typedef void (*MatrixSetMethod)(void*, uint32_t, const float*);
    MatrixSetMethod set_matrix;
    uint16_t handle;
    float matrix[16];
    float sx, cx, sy, cy, sz, cz;
    float m3c, m40, m38, m44, m48, m10, m2c;
    float m30, m28, m1c, m20, m18;
    float offset[3];
    unsigned int i;
    if (owner == NULL || track == NULL || position == NULL ||
        rotation == NULL || scale == NULL) return;
    memcpy(&nodes, static_cast<unsigned char*>(owner) + 0x14,
           sizeof(nodes));
    memcpy(&handle, bytes + 6, sizeof(handle));
    if (nodes == NULL) return;
    node = nodes[handle];
    if (node == NULL) return;
    vtable = *static_cast<void***>(node);
    if (vtable == NULL || vtable[0x38 / 4] == NULL) return;
    set_matrix = reinterpret_cast<MatrixSetMethod>(vtable[0x38 / 4]);
    memset(matrix, 0, sizeof(matrix));
    if ((bytes[0x28] & 4u) != 0) {
        return;
    }
    else {
        memcpy(offset, bytes + 0x0c, sizeof(offset));
        sx = (float)sin((double)rotation[0]);
        cx = (float)cos((double)rotation[0]);
        sy = (float)sin((double)rotation[1]);
        cy = (float)cos((double)rotation[1]);
        sz = (float)sin((double)rotation[2]);
        cz = (float)cos((double)rotation[2]);
        m3c = scale[0] * sz;
        m40 = sx * (sy * m3c) + cx * (scale[0] * cz);
        m38 = cx * (sy * m3c) - sx * (scale[0] * cz);
        m3c = cy * m3c;
        m44 = scale[1] * cz;
        m48 = sy * m44;
        m10 = -(scale[1] * sz);
        m2c = cy * m44;
        m30 = sx * m48 + cx * m10;
        m28 = -(sx * m10) + cx * m48;
        m1c = -(scale[2] * sy);
        m20 = sx * (scale[2] * cy);
        m18 = cx * (scale[2] * cy);
        matrix[0] = m40; matrix[1] = m3c; matrix[2] = m38;
        matrix[4] = m30; matrix[5] = m2c; matrix[6] = m28;
        matrix[8] = m20; matrix[9] = m1c; matrix[10] = m18;
        matrix[12] = offset[0] * m40 + offset[1] * m30 +
                     offset[2] * m20 + position[0];
        matrix[13] = offset[0] * m3c + offset[1] * m2c +
                     offset[2] * m1c + position[1];
        matrix[14] = offset[0] * m38 + offset[1] * m28 +
                     offset[2] * m18 + position[2];
        matrix[15] = 1.0f;
    }
    set_matrix(node, 0, matrix);
}

// MatrixApplyRotationAboutZWithOffset (FUN_0044b7a0).
// FUNCTION: LEGORR 0x0044b7a0
void matrix_apply_rotation_about_z_with_offset(void* target,
                                               float matrix[16],
                                               float offset)
{
    float original[16];
    float rotated[16];
    float old_x;
    float old_y;
    float old_z;
    int column;
    if (matrix == NULL) return;
    memcpy(original, matrix, sizeof(original));
    old_x = original[12];
    old_y = original[13];
    old_z = original[14];
    for (column = 0; column < 4; ++column) {
        int base = column * 4;
        rotated[base + 0] = -original[base + 1];
        rotated[base + 1] = original[base + 0];
        rotated[base + 2] = original[base + 2];
        rotated[base + 3] = original[base + 3];
    }
    rotated[12] = old_x;
    rotated[13] = old_z;
    rotated[14] = -old_y - offset;
    memcpy(matrix, rotated, sizeof(rotated));
    if (s_scene_matrix_apply_hook != NULL) {
        s_scene_matrix_apply_hook(s_scene_matrix_apply_context,
                                  target, matrix);
    }
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

// RendererRecordClone (FUN_00406cb0).
// FUNCTION: LEGORR 0x00406cb0
void renderer_record_clone(const RendererRecord* source,
                           RendererRecord* destination)
{
    unsigned int i;
    if (source == NULL || destination == NULL) return;
    destination->first_word = source->first_word;
    destination->visual_resource = source->visual_resource;
    for (i = 0; i < 23; ++i) destination->prefix[i] = source->prefix[i];
    for (i = 0; i < 3; ++i) {
        destination->resource_lists[i] = source->resource_lists[i];
    }
    destination->flags = source->flags & ~1u;
    if (s_renderer_record_clone_hooks.linked_records_clone != NULL) {
        for (i = 0; i < 3; ++i) {
            destination->resource_lists[i] =
                s_renderer_record_clone_hooks.linked_records_clone(
                    s_renderer_record_clone_hooks.context,
                    source->resource_lists[i]);
        }
    }
    if (s_renderer_record_clone_hooks.visual_resource_load != NULL) {
        destination->visual_resource =
            s_renderer_record_clone_hooks.visual_resource_load(
                s_renderer_record_clone_hooks.context,
                source->visual_resource);
    }
}

void scene_object_list_apply_hook_set(SceneObjectListApplyHook hook,
                                      void* context)
{
    s_scene_object_list_apply_hook = hook;
    s_scene_object_list_apply_context = context;
}

static void scene_object_list_apply(void* list, uint32_t operation,
                                    void* payload)
{
    void* object;
    unsigned int count;
    if (list == NULL || s_scene_object_list_apply_hook == NULL) return;
    object = *(void**)((char*)list + 4);
    count = 0;
    while (object != NULL && count < 65536u) {
        void* next = *(void**)((char*)object + 0x9c);
        s_scene_object_list_apply_hook(s_scene_object_list_apply_context,
                                       list, object, operation, payload);
        object = next;
        ++count;
    }
}

// FUNCTION: LEGORR 0x0040fe00
void scene_object_list_update_transforms(void* list, void* payload)
{
    void* first;
    if (list == NULL) return;
    first = *(void**)list;
    if (s_scene_object_list_apply_hook != NULL) {
        s_scene_object_list_apply_hook(s_scene_object_list_apply_context,
                                       list, first, 0x0040fde0u, payload);
    }
    scene_object_list_apply(list, 0x0040fb90u, payload);
    scene_object_list_apply(list, 0x0040fd00u, payload);
}

// FUNCTION: LEGORR 0x0040ff50
void scene_object_list_apply_float_callback(void* list, float value)
{
    scene_object_list_apply(list, 0x0040fed0u, &value);
}

// FUNCTION: LEGORR 0x00410070
void scene_object_list_apply_position_callback(void* list,
                                              const float position[3])
{
    scene_object_list_apply(list, 0x00410050u, (void*)position);
}

// FUNCTION: LEGORR 0x004100b0
void scene_object_list_apply_second_position_callback(
    void* list, const float position[3])
{
    scene_object_list_apply(list, 0x00410090u, (void*)position);
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

void quad_mesh_render_hooks_set(const QuadMeshRenderHooks* hooks)
{
    if (hooks == NULL) {
        memset(&s_quad_mesh_render_hooks, 0, sizeof(s_quad_mesh_render_hooks));
    }
    else {
        s_quad_mesh_render_hooks = *hooks;
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

// FUNCTION: LEGORR 0x00424730
void sine_phase_midpoint_blend_dispatch(void* object, float delta)
{
    SinePhaseBlendState* state = &s_sine_phase_blend_state;
    SinePhaseBlendHooks* hooks = &s_sine_phase_blend_hooks;
    float color[3];
    float half_difference;
    float oscillation;
    unsigned int i;
    if ((state->flags & 0x8000u) == 0u) return;
    if (hooks->object_prepare != NULL) {
        hooks->object_prepare(hooks->context, object);
    }
    if (object == NULL || (state->flags & 0x10000u) == 0u ||
        hooks->color_apply == NULL || state->period == 0.0f) return;
    state->phase += (6.2831855f / state->period) * delta;
    oscillation = (float)sin((double)state->phase);
    for (i = 0; i < 3; ++i) {
        half_difference = (state->to[i] - state->from[i]) * 0.5f;
        color[i] = state->from[i] + half_difference +
                   half_difference * oscillation;
    }
    hooks->color_apply(hooks->context, color[0], color[1], color[2]);
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

// FUNCTION: LEGORR 0x0046c030
void render_resource_binding_initialize(RenderResourceBinding* binding,
                                        void* key, void* config_a,
                                        void* config_b)
{
    unsigned int selector;
    if (binding == NULL) return;
    binding->key = NULL;
    binding->resource = NULL;
    binding->state = 0;
    binding->flags = 0;
    if (s_render_resource_binding_hooks.resource_find != NULL) {
        for (selector = 0; selector < 3; ++selector) {
            binding->resource =
                s_render_resource_binding_hooks.resource_find(
                    s_render_resource_binding_hooks.context,
                    config_a, config_b, selector);
            if (binding->resource != NULL) break;
        }
    }
    binding->key = key;
}

// FUNCTION: LEGORR 0x0046c0b0
void render_resource_binding_copy_and_convert(
    const RenderResourceBinding* source,
    RenderResourceBinding* destination)
{
    if (source == NULL || destination == NULL) return;
    *destination = *source;
    if (s_render_resource_binding_hooks.resource_load != NULL) {
        destination->resource =
            s_render_resource_binding_hooks.resource_load(
                s_render_resource_binding_hooks.context, source->resource);
    }
}

// FUNCTION: LEGORR 0x0046c0f0
void render_resource_binding_release(RenderResourceBinding* binding)
{
    if (binding == NULL) return;
    if (s_render_resource_binding_hooks.resource_release != NULL) {
        s_render_resource_binding_hooks.resource_release(
            s_render_resource_binding_hooks.context, binding->resource);
    }
}

// FUNCTION: LEGORR 0x0046c110
float animation_resource_advance_time(RenderResourceBinding* binding,
                                     void* argument)
{
    if (binding != NULL &&
        s_render_resource_binding_hooks.animation_time_get != NULL) {
        (void)s_render_resource_binding_hooks.animation_time_get(
            s_render_resource_binding_hooks.context, binding->resource,
            argument);
    }
    return 0.0f;
}

// FUNCTION: LEGORR 0x0046c130
void scene_object_set_animation_with_fallback(
    RenderResourceBinding* binding, void* name)
{
    int result = 0;
    if (binding == NULL ||
        s_render_resource_binding_hooks.animation_set == NULL) return;
    result = s_render_resource_binding_hooks.animation_set(
        s_render_resource_binding_hooks.context, binding->resource, name);
    if (result == 0) {
        (void)s_render_resource_binding_hooks.animation_set(
            s_render_resource_binding_hooks.context, binding->resource,
            s_render_resource_binding_hooks.fallback_animation);
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

static void scene_object_chain_nodes_release(SceneObjectChainRecord* record)
{
    int i;
    if (record == NULL || s_scene_object_chain_hooks.node_release == NULL)
        return;
    for (i = 2; i >= 0; --i) {
        if (record->node[i] != NULL) {
            s_scene_object_chain_hooks.node_release(
                s_scene_object_chain_hooks.context, record->node[i]);
            record->node[i] = NULL;
        }
    }
}

// SceneObjectChainRecordCreate (FUN_00435b20).
// FUNCTION: LEGORR 0x00435b20
SceneObjectChainRecord* scene_object_chain_record_create(void* parent,
                                                         int type)
{
    static const float direction[3] = { 0.0f, 0.0f, 1.0f };
    static const float up[3] = { 0.0f, 1.0f, 0.0f };
    SceneObjectChainRecord* record;
    SceneObjectChainHooks* hooks = &s_scene_object_chain_hooks;
    if (hooks->node_create == NULL) return NULL;
    record = (SceneObjectChainRecord*)engine_heap_alloc(
        (long)sizeof(SceneObjectChainRecord));
    if (record == NULL) return NULL;
    memset(record, 0, sizeof(*record));
    record->type = type;
    record->layer_depth = 200.0f;
    record->node[0] = hooks->node_create(hooks->context, NULL);
    if (record->node[0] == NULL) goto fail;
    if (hooks->node_position_set != NULL) {
        hooks->node_position_set(hooks->context, record->node[0],
                                 0.0f, 0.0f, -record->layer_depth);
    }
    if (hooks->node_orientation_set != NULL) {
        hooks->node_orientation_set(hooks->context, record->node[0],
                                    direction, up);
    }
    if (type == 3) {
        if (hooks->node_attach != NULL)
            hooks->node_attach(hooks->context, record->node[0], parent);
        return record;
    }
    record->node[1] = hooks->node_create(hooks->context, parent);
    if (record->node[1] == NULL) goto fail;
    record->node[2] = hooks->node_create(hooks->context, parent);
    if (record->node[2] == NULL) goto fail;
    record->node[3] = hooks->node_create(hooks->context, record->node[1]);
    if (record->node[3] == NULL) goto fail;
    if (type == 1) {
        record->node[4] = hooks->node_create(hooks->context, record->node[3]);
        if (record->node[4] != NULL && hooks->node_position_set != NULL) {
            hooks->node_position_set(hooks->context, record->node[4],
                                     0.0f, 0.0f, -80.0f);
        }
    }
    if (hooks->node_attach != NULL) {
        hooks->node_attach(hooks->context, record->node[0], record->node[3]);
    }
    return record;
fail:
    scene_object_chain_nodes_release(record);
    engine_heap_free(record);
    return NULL;
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

// FUNCTION: LEGORR 0x0044bae0
void scene_object_resource_and_callback_state_dispose(void** owner)
{
    void* scene_object;
    void** resource_slot;
    void* resource;
    SceneObjectDisposeHooks* hooks = &s_scene_object_dispose_hooks;
    if (owner == NULL || *owner == NULL) return;
    if (hooks->callback_state_remove != NULL) {
        hooks->callback_state_remove(hooks->context, owner,
                                     0x0044bab0u, 0);
    }
    scene_object = *owner;
    resource_slot = (void**)((char*)scene_object + 0x98);
    resource = *resource_slot;
    if (resource != NULL) {
        if (hooks->scene_node_detach != NULL) {
            hooks->scene_node_detach(hooks->context, resource);
        }
        if (hooks->scene_node_release != NULL) {
            hooks->scene_node_release(hooks->context, resource);
        }
        *resource_slot = NULL;
    }
    if (hooks->owner_record_release != NULL) {
        hooks->owner_record_release(hooks->context, owner);
    }
}

/* FUNCTION: LEGORR 0x00485ea0 */
void viewport_subrectangle_set(const float rectangle[4])
{
    float width = 0.0f;
    float height = 0.0f;
    float left = 0.0f;
    float top = 0.0f;
    float right;
    float bottom;
    if (rectangle != NULL) {
        if (rectangle[0] > 0.0f) left = rectangle[0];
        if (rectangle[1] > 0.0f) top = rectangle[1];
    }
    if (s_viewport_dimensions_query == NULL ||
        s_viewport_dimensions_query(s_viewport_dimensions_context,
                                    &width, &height) != 0) {
        return;
    }
    right = width;
    bottom = height;
    if (rectangle != NULL) {
        if (rectangle[2] + left < right) right = rectangle[2] + left;
        if (rectangle[3] + top < bottom) bottom = rectangle[3] + top;
    }
    viewport_rectangle_set(left, top, right, bottom);
}

/* FUNCTION: LEGORR 0x00485fb0 */
void viewport_rectangle_get(float rectangle[4])
{
    if (rectangle == NULL) return;
    rectangle[0] = s_viewport_left;
    rectangle[1] = s_viewport_top;
    rectangle[2] = s_viewport_right - s_viewport_left;
    rectangle[3] = s_viewport_bottom - s_viewport_top;
}

void scene_object_owner_link_release_hooks_set(
    SceneOwnerLinkReleaseFn interface_release,
    SceneOwnerLinkReleaseFn child_chain_release)
{
    s_owner_link_interface_release = interface_release;
    s_owner_link_child_release = child_chain_release;
}

// FUNCTION: LEGORR 0x00489750
void scene_object_owner_link_release(SceneObjectOwnerLinkRecord* record)
{
    uint32_t i;
    if (record == NULL) return;
    if (record->kind == 0) {
        if (record->shared_mode == 0) {
            if (s_owner_link_interface_release != NULL) {
                s_owner_link_interface_release(record->first_handle);
                s_owner_link_interface_release(record->second_handle);
            }
        }
        else if (s_owner_link_child_release != NULL) {
            s_owner_link_child_release(record->child_chain);
        }
    }
    if (record->shared_mode == 0) {
        for (i = 0; i < record->handle_count; ++i) {
            if (record->handles != NULL &&
                s_owner_link_interface_release != NULL)
                s_owner_link_interface_release(record->handles[i]);
        }
        engine_heap_free(record->handles);
    }
    engine_heap_free(record);
}

/* ---- pure vector helpers ------------------------------------------------
 * Corpus mapping (finding 280; the portable C fallbacks preserve the
 * byte-equivalent single-precision results, while selected MSVC/i386 paths
 * mirror the corpus's x87 instruction schedule):
 *   FUN_00401720 vec_scale, FUN_00401500 vec_add,  FUN_004016c0 vec_sub,
 *   FUN_00401470 vec_cross, FUN_00401750 vec_normalize,
 *   FUN_00401420 vec_dist,  FUN_004016f0 vec_dot,  FUN_004013a0 vec_len,
 *   FUN_00401240 vec2_dist, FUN_00406730 vec2_dist_alt,
 *   FUN_0047c0c0 vec_exact_equal. */

// FUNCTION: LEGORR 0x00401720
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void vec_scale(float* dst, const float* src, float s)
{
    __asm {
        mov ecx, dword ptr [esp + 8]
        mov eax, dword ptr [esp + 4]
        fld dword ptr [esp + 0x0c]
        fmul dword ptr [ecx]
        fstp dword ptr [eax]
        fld dword ptr [ecx + 4]
        fmul dword ptr [esp + 0x0c]
        fstp dword ptr [eax + 4]
        fld dword ptr [ecx + 8]
        fmul dword ptr [esp + 0x0c]
        fstp dword ptr [eax + 8]
        ret
    }
}
#else
void vec_scale(float* dst, const float* src, float s) /* FUN_00401720 */
{
    dst[0] = s * src[0];
    dst[1] = s * src[1];
    dst[2] = s * src[2];
}
#endif

// FUNCTION: LEGORR 0x00401500
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void vec_add(float* dst, const float* a, const float* b)
{
    __asm {
        mov ecx, dword ptr [esp + 0x0c]
        mov edx, dword ptr [esp + 8]
        mov eax, dword ptr [esp + 4]
        fld dword ptr [ecx]
        fadd dword ptr [edx]
        fstp dword ptr [eax]
        fld dword ptr [edx + 4]
        fadd dword ptr [ecx + 4]
        fstp dword ptr [eax + 4]
        fld dword ptr [edx + 8]
        fadd dword ptr [ecx + 8]
        fstp dword ptr [eax + 8]
        ret
    }
}
#else
void vec_add(float* dst, const float* a, const float* b) /* FUN_00401500 */
{
    dst[0] = a[0] + b[0];
    dst[1] = a[1] + b[1];
    dst[2] = a[2] + b[2];
}
#endif

// FUNCTION: LEGORR 0x004016c0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void vec_sub(float* dst, const float* a, const float* b)
{
    __asm {
        mov ecx, dword ptr [esp + 8]
        mov edx, dword ptr [esp + 0x0c]
        mov eax, dword ptr [esp + 4]
        fld dword ptr [ecx]
        fsub dword ptr [edx]
        fstp dword ptr [eax]
        fld dword ptr [ecx + 4]
        fsub dword ptr [edx + 4]
        fstp dword ptr [eax + 4]
        fld dword ptr [ecx + 8]
        fsub dword ptr [edx + 8]
        fstp dword ptr [eax + 8]
        ret
    }
}
#else
void vec_sub(float* dst, const float* a, const float* b) /* FUN_004016c0 */
{
    dst[0] = a[0] - b[0];
    dst[1] = a[1] - b[1];
    dst[2] = a[2] - b[2];
}
#endif

// FUNCTION: LEGORR 0x00401470
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void vec_cross(float* dst, const float* b,
                                 const float* c)
{
    __asm {
        mov edx, dword ptr [esp + 8]
        mov ecx, dword ptr [esp + 0x0c]
        mov eax, dword ptr [esp + 4]
        fld dword ptr [edx + 8]
        fld dword ptr [ecx + 8]
        fmul dword ptr [edx + 4]
        fxch st(1)
        fmul dword ptr [ecx + 4]
        fsubp st(1), st
        fstp dword ptr [eax]
        fld dword ptr [ecx + 8]
        fld dword ptr [edx + 8]
        fmul dword ptr [ecx]
        fxch st(1)
        fmul dword ptr [edx]
        fsubp st(1), st
        fstp dword ptr [eax + 4]
        fld dword ptr [edx + 4]
        fld dword ptr [ecx + 4]
        fmul dword ptr [edx]
        fxch st(1)
        fmul dword ptr [ecx]
        fsubp st(1), st
        fstp dword ptr [eax + 8]
        ret
    }
}
#else
void vec_cross(float* dst, const float* b, const float* c) /* FUN_00401470 */
{
    dst[0] = b[1] * c[2] - b[2] * c[1];
    dst[1] = b[2] * c[0] - b[0] * c[2];
    dst[2] = b[0] * c[1] - b[1] * c[0];
}
#endif

// FUNCTION: LEGORR 0x00401750
/* FUN_00401750 keeps its x87 load/accumulate/store schedule; the zero-length
 * input remains unguarded and propagates NaNs, as pinned in test_view.cpp. */
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void vec_normalize(float* v) /* FUN_00401750 */
{
    __asm {
        push ecx
        mov eax, dword ptr [esp + 8]
        mov edx, dword ptr [eax + 8]
        mov ecx, dword ptr [eax + 4]
        fld dword ptr [eax]
        mov dword ptr [esp], edx
        mov dword ptr [esp + 8], ecx
        fld dword ptr [esp]
        fld dword ptr [esp + 8]
        fmul dword ptr [esp + 8]
        fxch st(1)
        fmul dword ptr [esp]
        fld st(2)
        fmul st, st(3)
        fxch st(1)
        faddp st(2), st
        faddp st(1), st
        fsqrt
        fstp st(1)
        fdivr dword ptr s_normalize_unit
        fld st(0)
        fld st(1)
        fxch st(1)
        fmul dword ptr [eax]
        fxch st(1)
        fmul dword ptr [eax + 4]
        fxch st(2)
        fmul dword ptr [eax + 8]
        fxch st(1)
        fstp dword ptr [eax]
        fxch st(1)
        fstp dword ptr [eax + 4]
        fstp dword ptr [eax + 8]
        pop ecx
        ret
    }
}
#else
void vec_normalize(float* v) /* FUN_00401750 (zero length unguarded, as corpus) */
{
    float s = s_normalize_unit / sqrtf(v[0] * v[0] + (v[1] * v[1] + v[2] * v[2]));
    v[0] = s * v[0];
    v[1] = s * v[1];
    v[2] = s * v[2];
}
#endif

// FUNCTION: LEGORR 0x00401420
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) float vec_dist(const float* a, const float* b)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        mov ecx, dword ptr [esp + 8]
        fld dword ptr [eax + 8]
        fld dword ptr [eax + 4]
        fsub dword ptr [ecx + 4]
        fxch st(1)
        fsub dword ptr [ecx + 8]
        fld dword ptr [eax]
        fsub dword ptr [ecx]
        fxch st(2)
        fst dword ptr [esp + 4]
        fld st(1)
        fxch st(1)
        fmul dword ptr [esp + 4]
        fxch st(1)
        fmul st, st(2)
        fxch st(3)
        fst dword ptr [esp + 4]
        fmul dword ptr [esp + 4]
        fxch st(3)
        faddp st(1), st
        fxch st(2)
        faddp st(2), st
        fxch st(1)
        fsqrt
        fstp st(1)
        ret
    }
}
#else
float vec_dist(const float* a, const float* b) /* FUN_00401420 */
{
    float dx = a[0] - b[0], dy = a[1] - b[1], dz = a[2] - b[2];
    return sqrtf(dx * dx + dy * dy + dz * dz);
}
#endif

// FUNCTION: LEGORR 0x004016f0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) float vec_dot(const float* a, const float* b)
{
    __asm {
        mov eax, dword ptr [esp + 8]
        mov ecx, dword ptr [esp + 4]
        fld dword ptr [eax + 4]
        fld dword ptr [ecx + 8]
        fxch st(1)
        fmul dword ptr [ecx + 4]
        fld dword ptr [eax]
        fxch st(2)
        fmul dword ptr [eax + 8]
        fxch st(2)
        fmul dword ptr [ecx]
        fxch st(2)
        faddp st(1), st
        fxch st(1)
        faddp st(1), st
        ret
    }
}
#else
float vec_dot(const float* a, const float* b) /* FUN_004016f0 */
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}
#endif

// FUNCTION: LEGORR 0x004013a0
float vec_len(const float* a) /* FUN_004013a0 */
{
    return sqrtf(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
}

// FUNCTION: LEGORR 0x00401240
float vec2_dist(const float* a, const float* b) /* FUN_00401240 */
{
    float dy = a[1] - b[1];
    float dx = a[0] - b[0];
    return sqrtf(dy * dy + dx * dx);
}

// FUNCTION: LEGORR 0x00406730
float vec2_dist_alt(const float* a, const float* b) /* FUN_00406730 */
{
    /* Corpus variant: x term first through an extra round-trip, then y —
     * opposite the primary helper's x87 arithmetic order. */
    float dx = a[0] - b[0];
    float dy = a[1] - b[1];
    return sqrtf(dx * dx + dy * dy);
}

// FUNCTION: LEGORR 0x004089d0
float segment_2d_point_distance(const float* point, const float* start,
                                const float* end)
{
    const float dx = end[0] - start[0];
    const float dy = end[1] - start[1];
    const float length_sq = dx * dx + dy * dy;
    float t;
    float nearest[2];

    if (length_sq <= 0.0f) {
        return vec2_dist(point, start);
    }

    t = ((point[0] - start[0]) * dx + (point[1] - start[1]) * dy) /
        length_sq;
    if (t < 0.0f) {
        nearest[0] = start[0];
        nearest[1] = start[1];
    }
    else if (t > 1.0f) {
        nearest[0] = end[0];
        nearest[1] = end[1];
    }
    else {
        nearest[0] = start[0] + t * dx;
        nearest[1] = start[1] + t * dy;
    }
    return vec2_dist(point, nearest);
}

// FUNCTION: LEGORR 0x00408b00
float point_to_polyline_minimum_segment_distance(const float* point,
                                                 const float* points,
                                                 unsigned int segment_count)
{
    unsigned int i;
    float minimum = segment_2d_point_distance(point, points, points + 2);
    for (i = 1; i < segment_count; ++i) {
        const float* segment_start = points + i * 2;
        const float distance = segment_2d_point_distance(point, segment_start,
                                                          segment_start + 2);
        if (distance < minimum) {
            minimum = distance;
        }
    }
    return minimum;
}

// FUNCTION: LEGORR 0x0047c0c0
int vec_exact_equal(const float* a, const float* b) /* FUN_0047c0c0 */
{
    return a[0] == b[0] && a[1] == b[1] && a[2] == b[2];
}

// FUNCTION: LEGORR 0x00477290
void camera_screen_coordinate_project(const ViewProjectionAdapter* adapter,
                                      float* output, const float* input)
{
    if (adapter == NULL || adapter->screen_project == NULL || output == NULL ||
        input == NULL) return;
    adapter->screen_project(adapter->context, output, input);
}

// FUNCTION: LEGORR 0x004772d0
void camera_project_point_to_screen(const ViewProjectionAdapter* adapter,
                                   float* output_xy, const float* input_xyz)
{
    float projected[4];
    if (adapter == NULL || adapter->transform_point == NULL ||
        output_xy == NULL || input_xyz == NULL) return;
    adapter->transform_point(adapter->context, projected, input_xyz);
    output_xy[0] = projected[0] / projected[3];
    output_xy[1] = projected[1] / projected[3];
}

static float projected_triangle_area_2d(const float* a, const float* b,
                                        const float* c)
{
    float area = (b[0] - a[0]) * (c[1] - a[1]) -
                 (b[1] - a[1]) * (c[0] - a[0]);
    return (area < 0.0f ? -area : area) * 0.5f;
}

// FUNCTION: LEGORR 0x0040b470
float projected_quad_screen_area_calculate(
    const ViewProjectionAdapter* adapter, const float* quad_xyz,
    const float bounds_xy[2], int apply_visibility_clip)
{
    float clip[4][4];
    float screen[4][3];
    float visibility = 1.0f;
    int i;
    if (adapter == NULL || adapter->transform_point == NULL ||
        quad_xyz == NULL || bounds_xy == NULL) return 0.0f;
    for (i = 0; i < 4; ++i) {
        adapter->transform_point(adapter->context, clip[i], quad_xyz + i * 3);
        screen[i][0] = clip[i][0] / clip[i][3];
        screen[i][1] = clip[i][1] / clip[i][3];
        screen[i][2] = clip[i][2] / clip[i][3];
        if (apply_visibility_clip &&
            (screen[i][0] < 0.0f || screen[i][1] < 0.0f ||
             screen[i][0] > bounds_xy[0] || screen[i][1] > bounds_xy[1] ||
             screen[i][2] < 0.0f)) {
            visibility -= 0.25f;
        }
    }
    if (visibility <= 0.0f) return 0.0f;
    return (projected_triangle_area_2d(screen[0], screen[1], screen[2]) +
            projected_triangle_area_2d(screen[2], screen[3], screen[0])) *
           visibility;
}

// FUNCTION: LEGORR 0x0040fb00
void matrix_rotation_compose_xyz(float* record)
{
    float scratch[64];
    float* rotation_x = scratch + 32;
    float* rotation_y = scratch;
    float* rotation_z = scratch + 16;
    float* intermediate = scratch + 48;

    matrix4_set_rotation_x(rotation_x, record[0x28 / 4]);
    matrix4_set_rotation_y(rotation_y, record[0x24 / 4]);
    matrix4_set_rotation_z(rotation_z, record[0x2c / 4]);
    matrix4_multiply(intermediate, rotation_x, rotation_y);
    matrix4_multiply(record + 0x58 / 4, rotation_z, intermediate);
}

// FUNCTION: LEGORR 0x00479500
void d3drm_vector_random(float* out)
{
    /* The bundled D3DRM export draws each component in [-1, 1] and normalizes
     * the resulting vector. Its internal random state belongs to D3DRM, so
     * the portable build uses the CRT stream here. */
    float x = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
    float y = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
    float z = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
    float length = sqrtf(x * x + y * y + z * z);
    if (length == 0.0f) {
        x = 1.0f;
        y = 0.0f;
        z = 0.0f;
        length = 1.0f;
    }
    out[0] = x / length;
    out[1] = y / length;
    out[2] = z / length;
}

// FUNCTION: LEGORR 0x00479510
void d3drm_vector_rotate(float* out, const float* vector, float* axis,
                         float angle)
{
    /* D3DRMVectorRotate normalizes its axis in place, then applies Rodrigues'
     * formula using the negated angle. D3DRMVectorNormalize maps a zero vector
     * to (1, 0, 0), so even a zero axis becomes a rotation about X. */
    float axis_length;
    float x;
    float y;
    float z;
    float dot;
    float c;
    float s;
    float one_minus_c;
    float nx;
    float ny;
    float nz;
    axis_length = sqrtf(axis[0] * axis[0] + axis[1] * axis[1] +
                        axis[2] * axis[2]);
    if (axis_length == 0.0f) {
        axis[0] = 1.0f;
        axis[1] = 0.0f;
        axis[2] = 0.0f;
    }
    else {
        axis[0] /= axis_length;
        axis[1] /= axis_length;
        axis[2] /= axis_length;
    }
    nx = axis[0];
    ny = axis[1];
    nz = axis[2];
    x = vector[0]; y = vector[1]; z = vector[2];
    dot = nx * x + ny * y + nz * z;
    c = cosf(angle);
    s = -sinf(angle);
    one_minus_c = 1.0f - c;
    out[0] = x * c + (ny * z - nz * y) * s + nx * dot * one_minus_c;
    out[1] = y * c + (nz * x - nx * z) * s + ny * dot * one_minus_c;
    out[2] = z * c + (nx * y - ny * x) * s + nz * dot * one_minus_c;
}

// FUNCTION: LEGORR 0x0040fcc0
void vector_scale_components_in_place(float* vector, float sx, float sy,
                                      float sz)
{
    if (vector == NULL) {
        return;
    }
    d3drm_vector_random(vector);
    vector[0] *= sx;
    vector[1] *= sy;
    vector[2] *= sz;
}

/* ---- geometry helpers (finding 363/323) --------------------------------
 * Pure math leaves, corpus-exact operations: the corpus computes on x87
 * long doubles, the port on single precision (documented deviation). */

/* PointDistanceWithinRadius (FUN_0045b8a0): true when the point
 * (x1,y1)-(x2,y2) distance is under `radius`. */
// FUNCTION: LEGORR 0x0045b8a0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) int point_distance_within_radius(int x1, int y1, int x2,
                                                   int y2, int radius)
{
    __asm {
        sub esp, 8
        mov eax, dword ptr [esp + 0x14]
        mov edx, dword ptr [esp + 0x0c]
        mov ecx, dword ptr [esp + 0x18]
        sub eax, edx
        mov edx, dword ptr [esp + 0x10]
        mov dword ptr [esp + 0x14], eax
        fild dword ptr [esp + 0x14]
        fild dword ptr [esp + 0x1c]
        sub ecx, edx
        mov dword ptr [esp + 0x18], ecx
        fild dword ptr [esp + 0x18]
        fld st(2)
        fmul st, st(3)
        fxch st(1)
        fst dword ptr [esp + 4]
        fmul dword ptr [esp + 4]
        faddp st(1), st
        fsqrt
        fst dword ptr [esp + 0x14]
        fxch st(1)
        fcompp
        fnstsw ax
        test ah, 0x41
        fstp st(0)
        jne point_distance_outside_radius
        mov eax, 1
        add esp, 8
        ret

    point_distance_outside_radius:
        xor eax, eax
        add esp, 8
        ret
    }
}
#else
int point_distance_within_radius(int x1, int y1, int x2, int y2, int radius)
{
    const float dx = (float)(x2 - x1);
    const float dy = (float)(y2 - y1);
    return sqrtf(dx * dx + dy * dy) < (float)radius;
}
#endif

/* RectContainsPointHalfOpen (FUN_0045e660): rect is the corpus record's
 * float quad {x, y, w, h} at +0x4…+0x10 — half-open in both axes. */
// FUNCTION: LEGORR 0x0045e660
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) int rect_contains_point_half_open(const float* rect_record,
                                                    int x, int y)
{
    __asm {
        sub esp, 8
        mov eax, dword ptr [esp + 0x10]
        mov ecx, dword ptr [esp + 0x0c]
        xor edx, edx
        mov dword ptr [esp], eax
        mov dword ptr [esp + 4], edx
        fild qword ptr [esp]
        fcom dword ptr [ecx + 4]
        fnstsw ax
        test ah, 1
        jne rect_contains_false_with_stack
        fld dword ptr [ecx + 0x0c]
        fadd dword ptr [ecx + 4]
        fxch st(1)
        fcompp
        fnstsw ax
        test ah, 1
        je rect_contains_false

        mov eax, dword ptr [esp + 0x14]
        mov dword ptr [esp + 4], edx
        mov dword ptr [esp], eax
        fild qword ptr [esp]
        fcom dword ptr [ecx + 8]
        fnstsw ax
        test ah, 1
        jne rect_contains_false_with_stack
        fld dword ptr [ecx + 0x10]
        fadd dword ptr [ecx + 8]
        fxch st(1)
        fcompp
        fnstsw ax
        test ah, 1
        je rect_contains_false
        mov eax, 1
        add esp, 8
        ret

    rect_contains_false_with_stack:
        fstp st(0)
    rect_contains_false:
        xor eax, eax
        add esp, 8
        ret
    }
}
#else
int rect_contains_point_half_open(const float* rect_record, int x, int y)
{
    if (((rect_record[1] <= (float)x)
         && ((float)x < rect_record[3] + rect_record[1]))
        && ((rect_record[2] <= (float)y)
            && ((float)y < rect_record[4] + rect_record[2]))) {
        return 1;
    }
    return 0;
}
#endif

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

/* The corpus's identity constants are .rdata dwords (objdump-verified
 * 1.0 at 0x49e920, 0.0 at 0x49e924); named const statics keep the fld
 * operands resolvable by Reccmp. */
// GLOBAL: LEGORR 0x0049e920
static const float s_matrix_one = 1.0f;   /* DAT_0049e920 */
// GLOBAL: LEGORR 0x0049e924
static const float s_matrix_zero = 0.0f;  /* DAT_0049e924 */

/* RayPlaneIntersectionParameter (FUN_00479a30): writes t = dot(q-p, n) /
 * dot(d, n) into `t_out` for ray (p,d) against plane (q,n); answers 1 for a
 * non-zero denominator (the corpus gates with fcomp against the .rdata 0.0
 * dword at 0x49e924 and jumps to the 0 answer when that compare flags
 * equality, test ah, 0x40), 0 otherwise. Both dot products accumulate z,
 * y, x, matching the corpus's fadd order. */
// FUNCTION: LEGORR 0x00479a30
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) int ray_plane_intersection_parameter(
    float* t_out, const float* p, const float* d, const float* q,
    const float* n)
{
    __asm {
        mov eax, dword ptr [esp + 0x0c]
        mov ecx, dword ptr [esp + 0x14]
        fld dword ptr [eax + 8]
        fld dword ptr [eax + 4]
        fmul dword ptr [ecx + 4]
        fld dword ptr [ecx]
        fxch st(2)
        fmul dword ptr [ecx + 8]
        fxch st(2)
        fmul dword ptr [eax]
        fxch st(2)
        faddp st(1), st
        fxch st(1)
        faddp st(1), st
        fld st(0)
        fcomp dword ptr [s_matrix_zero]
        fnstsw ax
        test ah, 0x40
        jne ray_plane_zero

        mov eax, dword ptr [esp + 0x10]
        mov edx, dword ptr [esp + 8]
        fld dword ptr [eax + 4]
        fld dword ptr [eax + 8]
        fsub dword ptr [edx + 8]
        fxch st(1)
        fsub dword ptr [edx + 4]
        fld dword ptr [eax]
        fsub dword ptr [edx]
        fxch st(2)
        fmul dword ptr [ecx + 8]
        fxch st(1)
        fmul dword ptr [ecx + 4]
        fxch st(2)
        fmul dword ptr [ecx]
        fxch st(2)
        faddp st(1), st
        fxch st(1)
        faddp st(1), st
        mov ecx, dword ptr [esp + 4]
        mov eax, 1
        fdiv st, st(1)
        fstp dword ptr [ecx]
        fstp st(0)
        ret

    ray_plane_zero:
        fstp st(0)
        xor eax, eax
        ret
    }
}
#else
int ray_plane_intersection_parameter(float* t_out, const float* p,
                                     const float* d, const float* q,
                                     const float* n)
{
    const float denom = d[2] * n[2] + d[1] * n[1] + d[0] * n[0];
    if (denom != s_matrix_zero) {
        *t_out = ((q[2] - p[2]) * n[2] + (q[1] - p[1]) * n[1]
                  + (q[0] - p[0]) * n[0]) / denom;
        return 1;
    }
    return 0;
}
#endif

/* LineIntersection2DCompute (FUN_00479af0): intersects line1
 * (point p1, direction d1) with line2 (point p2, direction d2); writes the
 * hit point into `out` and returns it, or NULL when parallel. */
// FUNCTION: LEGORR 0x00479af0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) float* line_intersection_2d_compute(
    float* out, const float* p1, const float* d1, const float* p2,
    const float* d2)
{
    __asm {
        mov eax, dword ptr [esp + 0x14]
        mov edx, dword ptr [esp + 0x0c]
        fld dword ptr [eax]
        fld dword ptr [eax + 4]
        fld dword ptr [edx]
        fld dword ptr [edx + 4]
        fchs
        fxch st(1)
        fchs
        fxch st(1)
        fmul st, st(3)
        fxch st(1)
        fmul st, st(2)
        fld dword ptr [s_matrix_zero]
        fxch st(1)
        fsubp st(2), st
        fcomp st(1)
        fnstsw ax
        test ah, 0x40
        jne line_intersection_parallel

        fxch st(1)
        fdiv st, st(1)
        fxch st(2)
        mov ecx, dword ptr [esp + 8]
        push esi
        mov esi, dword ptr [esp + 0x14]
        mov eax, dword ptr [esp + 8]
        fdiv st, st(1)
        fld dword ptr [ecx]
        fld dword ptr [ecx + 4]
        fsub dword ptr [esi + 4]
        fxch st(1)
        fsub dword ptr [esi]
        fxch st(4)
        fchs
        fxch st(2)
        fmulp st(1), st
        fxch st(3)
        fmulp st(1), st
        pop esi
        faddp st(2), st
        fstp st(0)
        fld st(0)
        fmul dword ptr [edx]
        fstp dword ptr [eax]
        fmul dword ptr [edx + 4]
        fstp dword ptr [eax + 4]
        fld dword ptr [ecx]
        fadd dword ptr [eax]
        fld dword ptr [eax + 4]
        fxch st(1)
        fstp dword ptr [eax]
        fadd dword ptr [ecx + 4]
        fstp dword ptr [eax + 4]
        ret

    line_intersection_parallel:
        fstp st(0)
        fstp st(0)
        fstp st(0)
        xor eax, eax
        ret
    }
}
#else
float* line_intersection_2d_compute(float* out, const float* p1,
                                    const float* d1, const float* p2,
                                    const float* d2)
{
    float cross = -d1[1] * d2[0] + d1[0] * d2[1];
    if (cross != s_matrix_zero) {
        float t = (p1[0] - p2[0]) * -(d2[1] / cross)
                  + (d2[0] / cross) * (p1[1] - p2[1]);
        out[0] = t * d1[0];
        out[1] = t * d1[1];
        out[0] = out[0] + p1[0];
        out[1] = out[1] + p1[1];
        return out;
    }
    return NULL;
}
#endif

/* The corpus's quadrupling factor is the .rdata dword 4.0 at 0x49e938
 * (objdump-verified operand of the corpus discriminant term); a named
 * const static keeps the fmul operand resolvable by Reccmp. */
// GLOBAL: LEGORR 0x0049e938
static const float s_sphere_quadruple = 4.0f;   /* DAT_0049e938 */

/* SphereRayIntersectionDiscriminantPositive (FUN_00479c10): positive-
 * discriminant test of ray `dir` against the sphere (center, radius). The
 * corpus's exact discriminant expression is reproduced verbatim; the sign
 * gate is the corpus's fcomp against the 0.0 dword (0x49e924) with
 * test ah, 0x41 — strictly positive. */
// FUNCTION: LEGORR 0x00479c10
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) int sphere_ray_intersection_discriminant_positive(
    const float* center, float radius2, const float* origin,
    const float* dir)
{
    __asm {
        sub esp, 0x0c
        mov eax, dword ptr [esp + 0x18]
        mov ecx, dword ptr [esp + 0x1c]
        push esi
        fld dword ptr [eax + 4]
        fld dword ptr [eax + 8]
        fld dword ptr [eax]
        fxch st(2)
        mov eax, dword ptr [esp + 0x14]
        mov edx, dword ptr [ecx]
        mov dword ptr [esp + 4], edx
        fsub dword ptr [eax + 4]
        fxch st(1)
        fsub dword ptr [eax + 8]
        mov esi, dword ptr [ecx + 4]
        fld dword ptr [esp + 4]
        mov dword ptr [esp + 8], esi
        mov ecx, dword ptr [ecx + 8]
        fld dword ptr [esp + 8]
        fxch st(4)
        fsub dword ptr [eax]
        fxch st(1)
        fadd st, st
        fxch st(4)
        fadd st, st
        mov dword ptr [esp + 0x0c], ecx
        fld dword ptr [esp + 0x0c]
        fadd st, st
        fxch st(4)
        fst dword ptr [esp + 8]
        fmul dword ptr [esp + 8]
        fxch st(3)
        fst dword ptr [esp + 0x0c]
        fmul dword ptr [esp + 0x0c]
        fld st(2)
        fxch st(6)
        fmul st, st(3)
        fxch st(2)
        fmul dword ptr [esp + 8]
        fxch st(5)
        fmul dword ptr [esp + 0x0c]
        fxch st(6)
        fmul st, st(3)
        fxch st(5)
        faddp st(2), st
        faddp st(3), st
        fld dword ptr [esp + 0x18]
        fmul dword ptr [esp + 0x18]
        fxch st(5)
        faddp st(1), st
        fxch st(3)
        faddp st(2), st
        fxch st(2)
        fstp dword ptr [esp + 0x18]
        fmul dword ptr [s_sphere_quadruple]
        fld dword ptr [esp + 0x18]
        fmul dword ptr [esp + 0x18]
        fxch st(3)
        fsubp st(1), st
        fsubp st(2), st
        fxch st(1)
        fstp st(1)
        fcomp dword ptr [s_matrix_zero]
        fnstsw ax
        test ah, 0x41
        jne sphere_ray_discriminant_nonpositive
        mov eax, 1
        pop esi
        add esp, 0x0c
        ret

    sphere_ray_discriminant_nonpositive:
        xor eax, eax
        pop esi
        add esp, 0x0c
        ret
    }
}
#else
int sphere_ray_intersection_discriminant_positive(const float* center,
                                                  float radius2,
                                                  const float* origin,
                                                  const float* dir)
{
    const float wy = origin[1] - center[1];
    const float wz = origin[2] - center[2];
    const float wx = origin[0] - center[0];
    const float b = (dir[2] + dir[2]) * wz + (dir[1] + dir[1]) * wy
                    + (dir[0] + dir[0]) * wx;
    if (b * b - ((wx * wx + wz * wz + wy * wy) * s_sphere_quadruple
                 - radius2 * radius2) > s_matrix_zero) {
        return 1;
    }
    return 0;
}
#endif

/* ---- 4x4 matrices + polygon parity (finding 309/388) --------------------- */

/* Matrix4SetZero (FUN_00479ea0): the corpus's 16 zero stores. */
// FUNCTION: LEGORR 0x00479ea0
void matrix4_set_zero(float* out)
{
    for (int i = 0; i < 16; ++i) {
        out[i] = 0.0f;
    }
}

/* Matrix4Multiply (FUN_00479ce0): the corpus takes (out, right, left),
 * zeroes the output, then accumulates each cell as the four-term dot
 * product out[row][col] = Σ_k left[row][k] · right[k][col] — i.e.
 * out = left × right, row-major. */
// FUNCTION: LEGORR 0x00479ce0
void matrix4_multiply(float* out, const float* right, const float* left)
{
    matrix4_set_zero(out);
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            float* dst = out + 4 * row + col;
            for (int k = 0; k < 4; ++k) {
                *dst = left[4 * row + k] * right[4 * k + col] + *dst;
            }
        }
    }
}

/* PointInPolygon2D (FUN_00479b80): even-odd crossing parity. Edge k runs
 * verts[k] → next[k]; the corpus strides its two read pointers in lockstep,
 * so the in-lockstep indexing below reads the same bytes for any caller
 * passing the shifted-ring pair. The boundary semantics, zero-span divide
 * and all, are the corpus's. */
// FUNCTION: LEGORR 0x00479b80
int point_in_polygon_2d(const float point[2], const float* verts,
                        const float* next, int count)
{
    int crossings = 0;
    for (int k = 0; k < count; ++k) {
        const float x0 = verts[2 * k];
        const float y0 = verts[2 * k + 1];
        const float x1 = next[2 * k];
        const float y1 = next[2 * k + 1];
        if (((y0 <= point[1] && point[1] <= y1)
             || (point[1] <= y0 && y1 <= point[1]))
            && point[0] < (x1 - x0) * ((point[1] - y0) / (y1 - y0)) + x0) {
            ++crossings;
        }
    }
    return crossings & 1;
}

/* ---- matrix/ray math leaves (2026-10-24) ---------------------------------
 * The x87 float10 deviation for the rotation setters is documented in the
 * header; every other body is byte-equivalent single-precision arithmetic.
 */

/* PointAlongRay (FUN_00479ab0): out = origin + t · dir. The corpus reads
 * dir[1]/dir[2] into registers before the stores; the x term is t*dir +
 * origin while y/z add to the origin first — component order mirrored. */
// FUNCTION: LEGORR 0x00479ab0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void point_along_ray(float* out, const float* origin,
                                      const float* dir, float t)
{
    __asm {
        mov eax, dword ptr [esp + 0x0c]
        mov ecx, dword ptr [esp + 8]
        fld dword ptr [esp + 0x10]
        fmul dword ptr [eax]
        fld dword ptr [eax + 4]
        fmul dword ptr [esp + 0x10]
        fxch st(1)
        fadd dword ptr [ecx]
        fld dword ptr [eax + 8]
        fmul dword ptr [esp + 0x10]
        fxch st(1)
        mov eax, dword ptr [esp + 4]
        fstp dword ptr [eax]
        fld dword ptr [ecx + 4]
        fadd st, st(2)
        fxch st(1)
        fstp st(2)
        fstp dword ptr [eax + 4]
        fld dword ptr [ecx + 8]
        fadd st, st(1)
        fstp dword ptr [eax + 8]
        fstp st(0)
        ret
    }
}
#else
void point_along_ray(float* out, const float* origin, const float* dir,
                     float t)
{
    const float dy = dir[1];
    const float dz = dir[2];
    out[0] = t * dir[0] + origin[0];
    out[1] = origin[1] + dy * t;
    out[2] = origin[2] + dz * t;
}
#endif

/* RandomFloatRange (FUN_004798b0): consume the shared 15-bit random stream
 * and scale it into the requested half-open interval. */
// FUNCTION: LEGORR 0x004798b0
float random_float_range(float minimum, float maximum)
{
    const float random_value = (float)(int16_t)rng_thunk_draw15();
    return random_value * 3.051851e-05f * (maximum - minimum) + minimum;
}

/* ViewRayIntersectionTest (FUN_004798e0): normalizes both direction
 * vectors (the corpus sums each normalization as z²+y²+x², its SQRT
 * accumulator order), runs the ray (p, dir_a) against the plane through q
 * with normal dir_b, and advances along dir_a when they meet. */
// FUNCTION: LEGORR 0x004798e0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) int view_ray_intersection_test(
    float* out, const float* p, const float* dir_a, const float* q,
    const float* dir_b)
{
    __asm {
        sub esp, 18h
        mov ecx, dword ptr [esp + 2ch]
        mov eax, dword ptr [esp + 24h]
        push ebx
        push esi
        mov ebx, dword ptr [ecx]
        push edi
        mov edi, dword ptr [eax]
        mov dword ptr [esp + 18h], ebx
        fld dword ptr [esp + 18h]
        mov ebx, dword ptr [ecx + 4]
        mov dword ptr [esp + 0ch], edi
        fld dword ptr [esp + 0ch]
        fmul dword ptr [esp + 0ch]
        mov edi, dword ptr [eax + 4]
        mov dword ptr [esp + 1ch], ebx
        fld dword ptr [esp + 1ch]
        mov eax, dword ptr [eax + 8]
        mov dword ptr [esp + 10h], edi
        fld dword ptr [esp + 10h]
        fmul dword ptr [esp + 10h]
        fxch st(3)
        fmul dword ptr [esp + 18h]
        mov ecx, dword ptr [ecx + 8]
        mov dword ptr [esp + 14h], eax
        fld dword ptr [esp + 14h]
        fxch st(2)
        fmul dword ptr [esp + 1ch]
        mov dword ptr [esp + 20h], ecx
        lea edx, [esp + 18h]
        fld dword ptr [esp + 20h]
        fxch st(3)
        fmul dword ptr [esp + 14h]
        fxch st(3)
        fmul dword ptr [esp + 20h]
        fxch st(5)
        faddp st(4), st
        push edx
        mov edx, dword ptr [esp + 38h]
        lea esi, [esp + 10h]
        push edx
        faddp st(1), st
        fxch st(1)
        faddp st(2), st
        fxch st(2)
        faddp st(2), st
        push esi
        mov esi, dword ptr [esp + 38h]
        lea eax, [esp + 3ch]
        push esi
        fsqrt
        push eax
        fxch st(1)
        fsqrt
        fst dword ptr [esp + 44h]
        fxch st(1)
        fdivr dword ptr [s_matrix_one]
        fxch st(1)
        fdivr dword ptr [s_matrix_one]
        fxch st(1)
        fst dword ptr [esp + 44h]
        fld dword ptr [esp + 44h]
        fld dword ptr [esp + 44h]
        fxch st(3)
        fst dword ptr [esp + 44h]
        fld dword ptr [esp + 44h]
        fld dword ptr [esp + 44h]
        fxch st(4)
        fmul dword ptr [esp + 20h]
        fxch st(3)
        fmul dword ptr [esp + 24h]
        fxch st(5)
        fmul dword ptr [esp + 28h]
        fxch st(3)
        fstp dword ptr [esp + 20h]
        fxch st(1)
        fmul dword ptr [esp + 2ch]
        fxch st(4)
        fstp dword ptr [esp + 24h]
        fmul dword ptr [esp + 30h]
        fxch st(1)
        fstp dword ptr [esp + 28h]
        fxch st(1)
        fmul dword ptr [esp + 34h]
        fxch st(2)
        fstp dword ptr [esp + 2ch]
        fstp dword ptr [esp + 30h]
        fstp dword ptr [esp + 34h]
        call ray_plane_intersection_parameter
        add esp, 14h
        test eax, eax
        je view_ray_intersection_failed
        mov ecx, dword ptr [esp + 30h]
        mov eax, dword ptr [esp + 28h]
        lea edx, [esp + 0ch]
        push ecx
        push edx
        push esi
        push eax
        call point_along_ray
        add esp, 10h
        mov eax, 1
        pop edi
        pop esi
        pop ebx
        add esp, 18h
        ret

    view_ray_intersection_failed:
        pop edi
        pop esi
        xor eax, eax
        pop ebx
        add esp, 18h
        ret
    }
}
#else
int view_ray_intersection_test(float* out, const float* p,
                               const float* dir_a, const float* q,
                               const float* dir_b)
{
    const float s_a = 1.0f / sqrtf(dir_a[2] * dir_a[2] + dir_a[1] * dir_a[1]
                                   + dir_a[0] * dir_a[0]);
    const float s_b = 1.0f / sqrtf(dir_b[2] * dir_b[2] + dir_b[1] * dir_b[1]
                                   + dir_b[0] * dir_b[0]);
    const float n_a[3] = { s_a * dir_a[0], s_a * dir_a[1], s_a * dir_a[2] };
    const float n_b[3] = { s_b * dir_b[0], s_b * dir_b[1], s_b * dir_b[2] };
    float t;
    if (ray_plane_intersection_parameter(&t, p, n_a, q, n_b) != 0) {
        point_along_ray(out, p, n_a, t);
        return 1;
    }
    return 0;
}
#endif

/* Matrix4SetIdentity (FUN_00479e70): row loop, column loop, store 1 when
 * the indices meet (the corpus's dword-0x3f800000 form). */
// FUNCTION: LEGORR 0x00479e70
void matrix4_set_identity(float* out)
{
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            out[4 * row + col] = (row == col) ? s_matrix_one : s_matrix_zero;
        }
    }
}

/* MatrixIdentityAndTranslationSet (FUN_00479e40): identity, then the
 * translation triad at byte offsets +0x30/+0x34/+0x38 = row 3. */
// FUNCTION: LEGORR 0x00479e40
void matrix4_identity_translation_set(float* out, const float* translation)
{
    matrix4_set_identity(out);
    out[12] = translation[0];
    out[13] = translation[1];
    out[14] = translation[2];
}

/* Matrix4Copy (FUN_00479eb0): the 16 dword copies. */
// FUNCTION: LEGORR 0x00479eb0
void matrix4_copy(float* out, const float* src)
{
    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 4; ++column) {
            const int index = row * 4 + column;
            out[index] = src[index];
        }
    }
}

/* Matrix4SetRotationX (FUN_00479d50; the reference-name table also calls
 * this Z, alongside FUN_00479df0): identity, then the rows 1–2 block with
 * the corpus's signs — m[5]=c, m[6]=−s, m[9]=s, m[10]=c. */
// FUNCTION: LEGORR 0x00479d50
void matrix4_set_rotation_x(float* out, float angle)
{
    const float c = (float)cos(angle);
    const float s = (float)sin(angle);
    matrix4_set_identity(out);
    out[5] = c;
    out[6] = -s;
    out[10] = c;
    out[9] = s;
}

/* Matrix4SetRotationY (FUN_00479da0): identity, then m[0]=c, m[2]=s,
 * m[8]=−s, m[10]=c. */
// FUNCTION: LEGORR 0x00479da0
void matrix4_set_rotation_y(float* out, float angle)
{
    const float c = (float)cos(angle);
    const float s = (float)sin(angle);
    matrix4_set_identity(out);
    out[0] = c;
    out[8] = -s;
    out[10] = c;
    out[2] = s;
}

/* Matrix4SetRotationZ (FUN_00479df0): identity, then m[0]=c, m[1]=−s,
 * m[4]=s, m[5]=c. */
// FUNCTION: LEGORR 0x00479df0
void matrix4_set_rotation_z(float* out, float angle)
{
    const float c = (float)cos(angle);
    const float s = (float)sin(angle);
    matrix4_set_identity(out);
    out[0] = c;
    out[1] = -s;
    out[5] = c;
    out[4] = s;
}

/* BuildPerspectiveProjectionMatrix (FUN_0040bd60): zero the matrix, raise
 * m[10]/m[15], then the three quotients — the corpus's m[1] multiplies
 * p5/p4 by p4/p1, not p5/p1. */
// FUNCTION: LEGORR 0x0040bd60
void perspective_projection_matrix_set(float* out, float p1, float p2,
                                       float p3, float p4, float p5)
{
    out[3] = 0.0f;
    out[2] = 0.0f;
    out[7] = 0.0f;
    out[6] = 0.0f;
    out[4] = 0.0f;
    out[11] = 0.0f;
    out[9] = 0.0f;
    out[8] = 0.0f;
    out[14] = 0.0f;
    out[13] = 0.0f;
    out[12] = 0.0f;
    out[15] = 1.0f;
    out[10] = 1.0f;
    out[0] = p4 / p1;
    out[5] = p3 / p2;
    out[1] = (p5 / p4) * (p4 / p1);
}

/* ---- camera-handle primitives (record-backed) --------------------------- */

// FUNCTION: LEGORR 0x004755e0
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

/* ---- world/stub boundaries ---------------------------------------------- */
/* EngineContext grid lookup -> bounded voxel-grid coordinates. This adapter
 * is used where only the widened LevelState block is available; the actual
 * descriptor-based TerrainGridPick lives in record.cpp.
 *  FUN_0044f940 / FUN_0044fe70 terrain heights -> sampled voxel byte, not
 *  the original selected-triangle corner-height interpolation.
 *  FUN_00431570 returns the current level's +4 terrain handle despite its
 *  Ghidra label CurrentLevelHeightGet.
 *  FUN_0044a200 object lookup -> 0 (no object layer yet; mode 2 idles).
 *  FUN_0044a070 velocity blend-> returns 0 (targeted EMA deferred).
 *  FUN_004369f4 is the camera tilt-axis setter; main_view_key_step groups it
 *  with separate pan/zoom operations and has no one-to-one source boundary.
 *  FUN_004798b0 is RandomFloatRange, not a camera helper; the port models the
 *  shared RNG draw but has no float-range wrapper.
 *  FUN_0041f940 is the engine-state bit-0x20000 setter, now modeled through
 *  EngineContext::state in the scripted-follow branch below. */

static int engine_context_grid_pick(EngineContext* ctx, const float* pos,
                                    int* gx, int* gy)
{
    /* with a bound level, cells come from the real voxel grid; terrain uses
     * the voxel surface byte (+0x04). */
    if (ctx->level_block != NULL) {
        int w = engine_level_width(ctx);
        int h = engine_level_height(ctx);
        int x = (int)pos[0];
        int y = (int)pos[1];
        if (w > 0 && h > 0 && x >= 0 && y >= 0 && x < w && y < h) {
            *gx = x;
            *gy = y;
            return 1;
        }
        return 0;
    }
    *gx = (int)pos[0];
    *gy = (int)pos[1];
    return 1;
}
static float terrain_height(EngineContext* ctx, float x, float y)
{
    if (ctx->level_block != NULL) {
        return (float)engine_voxel_byte(ctx, (int)x, (int)y, 0x04);
    }
    return 0.0f;
}

/* RandomReachableGridPointSelect (FUN_00450560): the ray-descending
 * grid-point pick — see view.hpp for the plane geometry. Both float*
 * direction arguments ride the corpus's forwarded-dword convention:
 * the point and ray direction are vectors, and Ghidra typed them as
 * plain dwords because FUN_004798e0 only forwards them. The plane-point
 * z quirk is faithful: FUN_004798e0 answers into `out` while the height
 * store lands on the plane quad's z-slot, so the corpus effectively
 * intersects against successively lower planes. */
// FUNCTION: LEGORR 0x00450560
void random_reachable_grid_point_select(EngineContext* ctx, const float* p,
                                        const float* dir, float* out,
                                        int* gx, int* gy, int rounds)
{
    float plane[3] = { 0.0f, 0.0f, -50.0f };
    const float normal[3] = { 0.0f, 0.0f, -1.0f };
    for (int i = rounds; i != 0; --i) {
        view_ray_intersection_test(out, p, dir, plane, normal);
        plane[2] = terrain_height(ctx, out[0], out[1]);
    }
    (void)engine_context_grid_pick(ctx, out, gx, gy);
}

static int object_lookup(int id)
{
    return s_view_object_lookup_hook != NULL
        ? s_view_object_lookup_hook(id, s_view_follow_target_context) : 0;
}
// FUNCTION: LEGORR 0x0044a070
static int target_blend(int id, int mode, float* blend,
                        float* facing)
{
    if (s_view_target_blend_hook != NULL)
        return s_view_target_blend_hook(id, mode, blend, facing,
                                        s_view_follow_target_context);
    if (blend != NULL) {
        blend[0] = blend[1] = blend[2] = 0.0f;
    }
    if (facing != NULL) {
        facing[0] = facing[1] = facing[2] = 0.0f;
    }
    return g_view_test_blend; /* 0 = deferred (TODO: FUN_0044a070) */
}

/* ---- the ported tick ---------------------------------------------------- */

static void view_tail(ViewContext* v, float scale)
{
    /* Corpus tail: when the motion timer has expired, velocity snaps to 0;
     * otherwise it decays toward the step size and the camera is shifted by
     * the negative velocity (glide-to-stop). */
    if (v->timer_num <= v->timer_acc) {
        v->velocity[0] = v->velocity[1] = v->velocity[2] = 0.0f;
    } else {
        float decay = ((float)(v->timer_num - v->timer_acc) / (float)v->timer_num)
                    * v->step;
        vec_scale(v->velocity, v->velocity, decay);
        v->timer_acc += (int)scale;
        cam_offset(&v->cam_world, -v->velocity[0], -v->velocity[1],
                   -v->velocity[2]);
    }
}

// FUNCTION: LEGORR 0x00435f5c
void view_tick(ViewContext* v, EngineContext* ctx, float dt, float scale)
{
    if (v == NULL) {
        return;
    }
    EngineState* st = ctx != NULL ? ctx->state_block : NULL;
    (void)st;
    cam_offset(&v->cam_world, -v->velocity[0], -v->velocity[1],
               -v->velocity[2]);

    if (v->mode == 1) {
        /* Follow: keep the follow camera at the offset, then clamp it at the
         * world grid edges (corpus: local_8/local_c vs DAT_005560ac). */
        vec_scale(v->offset, v->offset, g_view_follow_scale);
        cam_offset(&v->cam_follow, v->offset[0], v->offset[1], v->offset[2]);
        float pos[3], fac[3];
        cam_get_pos(&v->cam_follow, pos);
        cam_get_facing(&v->cam_follow, fac);
        (void)fac;
        int gx = 0, gy = 0;
        if (!engine_context_grid_pick(ctx, pos, &gx, &gy)) {
            cam_offset(&v->cam_follow, -v->offset[0], -v->offset[1],
                       -v->offset[2]);
        } else {
            const int edge = g_view_grid_edge;
            if (gx < edge || engine_level_width(ctx) - edge <= gx) {
                cam_offset(&v->cam_follow, -v->offset[0], 0, 0);
            }
            if (gy < edge || engine_level_height(ctx) - edge <= gy) {
                cam_offset(&v->cam_follow, 0, -v->offset[1], 0);
            }
            float h = terrain_height(ctx, pos[0], pos[1]);
            cam_set_pos(&v->cam_facing, pos[0], pos[1], h);
            cam_set_facing(&v->cam_facing, fac);
            /* The corpus subtracts the sampled terrain surface from the
             * follow span before positioning the world camera. */
            float z = v->follow_span - h;
            cam_set_pos(&v->cam_world, 0.0f, 0.0f, -z);
        }
        view_tail(v, scale);
        return;
    }

    if (v->mode == 2) {
        /* Scripted / lagged follow: interpolate toward the target with
         * k = |d|/(|d|+A) once an object target exists (object_lookup). */
        if (v->follow_id == 0) {
            if (ctx != NULL) {
                ctx->state &= ~0x20000u;
            }
            view_tail(v, scale);
            return;
        }
        int target = object_lookup(v->follow_id);
        if (target == 0) {
            view_tail(v, scale);
            return;
        }
        if (ctx != NULL) {
            ctx->state |= 0x20000u;
        }
        float goal3[3], facing[3];
        cam_get_pos(&v->cam_world, goal3);
        cam_get_facing(&v->cam_world, facing);
        float dist = vec_len(goal3);
        float a = (float)v->a3;
        float k = a <= 0.0f ? 0.0f : dist / (dist + a); (void)k;
        /* ...full corpus interpolations follow when the object layer lands */
        view_tail(v, scale);
        return;
    }

    if (v->mode == 3) {
        /* Free/cruise: ease the camera position and facing toward the
         * velocity target.  The factor is 4 / dt, so smaller frame deltas
         * keep the camera closer to the target while larger deltas retain
         * more of the current camera state. */
        float pos[3], facing[3];
        float target_pos[3], target_facing[3];
        float mixed_pos[3], mixed_facing[3];
        float distance, facing_dot, factor, inverse_factor;
        int target_dirty;
        cam_get_pos(&v->cam_world, pos);
        cam_get_facing(&v->cam_world, facing);
        if (v->follow_id == 0 || dt == 0.0f ||
            target_blend(v->follow_id, v->a5, target_pos,
                         target_facing) == 0) {
            view_tail(v, scale);
            return;
        }

        target_dirty = (v->flags & 0x10u) != 0;
        distance = vec_dist(target_pos, pos);
        facing_dot = vec_dot(target_facing, facing);
        if (!target_dirty || distance > 100.0f || facing_dot < 0.0f) {
            memcpy(pos, target_pos, sizeof(pos));
            memcpy(facing, target_facing, sizeof(facing));
        }

        /* The reference normalizes both facing vectors after applying its
         * dirty-target fallback, then blends the current vector toward the
         * target vector. */
        vec_normalize(facing);
        vec_normalize(target_facing);
        factor = 4.0f / dt;
        inverse_factor = 1.0f / (factor + 1.0f);
        vec_scale(mixed_facing, facing, factor);
        vec_add(mixed_facing, mixed_facing, target_facing);
        vec_scale(mixed_facing, mixed_facing, inverse_factor);
        vec_scale(mixed_pos, pos, factor);
        vec_add(mixed_pos, mixed_pos, target_pos);
        vec_scale(mixed_pos, mixed_pos, inverse_factor);

        if (distance > 0.01f)
            cam_set_pos(&v->cam_world, mixed_pos[0], mixed_pos[1],
                        mixed_pos[2]);
        if (facing_dot < 0.99999f)
            cam_set_facing(&v->cam_world, mixed_facing);
        v->flags |= 0x10;
        view_tail(v, scale);
        return;
    }

    view_tail(v, scale);
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
/* MenuPointerRectangleHitTest (FUN_004112e0): the corpus's widget test,
 * reproduced clause-for-clause — inclusive lower corners, inclusive
 * upper corners via origin+size (finding 23: the widget rectangle is
 * the 100x20 row band this routine hit-tests). */
// FUNCTION: LEGORR 0x004112e0
#if defined(LEGORRR_BACKEND_WINAPI)
__declspec(naked) int menu_pointer_rectangle_hit_test(
    int x, int y, int rect_x, int rect_y, int rect_w, int rect_h)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        mov ecx, dword ptr [esp + 0x0c]
        cmp eax, ecx
        jl menu_pointer_rectangle_hit_test_miss
        mov edx, dword ptr [esp + 0x14]
        add edx, ecx
        cmp eax, edx
        jg menu_pointer_rectangle_hit_test_miss
        mov eax, dword ptr [esp + 8]
        mov ecx, dword ptr [esp + 0x10]
        cmp eax, ecx
        jl menu_pointer_rectangle_hit_test_miss
        mov edx, dword ptr [esp + 0x18]
        add edx, ecx
        cmp eax, edx
        jg menu_pointer_rectangle_hit_test_miss
        mov eax, 1
        ret
    menu_pointer_rectangle_hit_test_miss:
        xor eax, eax
        ret
    }
}
#else
int menu_pointer_rectangle_hit_test(int x, int y, int rect_x, int rect_y,
                                    int rect_w, int rect_h)
{
    if (x < rect_x || x > rect_w + rect_x) return 0;
    if (y < rect_y || y > rect_h + rect_y) return 0;
    return 1;
}
#endif

/* MenuOverlayListCount (FUN_00412170): head slot + record+0x40, next
 * slot + node+0x18, both native-size (pointer-slot deviation; see the
 * header). */
// FUNCTION: LEGORR 0x00412170
int menu_overlay_list_count(const void* menu)
{
    int count = 0;
    void* head = *(void* const*)((const char*)menu + 0x40);
    while (head != NULL) {
        ++count;
        head = *(void**)((char*)head + 0x18);
    }
    return count;
}

/* TerrainGridBorderInsetGet (FUN_00435f28): DAT_005560ac's getter. */
// FUNCTION: LEGORR 0x00435f28
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) int view_grid_edge_get(void)
{
    __asm {
        push ebp
        mov ebp, esp
        mov eax, dword ptr [g_view_grid_edge]
        pop ebp
        ret
    }
}
#else
int view_grid_edge_get(void)
{
    return g_view_grid_edge;
}
#endif

/* --- camera-selection cluster (see view.hpp) -------------------------------
 * DAT_005560a0 / DAT_005560a8: zero-initialized until camera_view_context_set
 * configures them (the corpus loads them from the camera config records at
 * startup — that loader seam is unported, so the pan stays inert out of the
 * box, like g_view_grid_edge). */
// GLOBAL: LEGORR 0x005560a0
float g_view_pan_cap   = 0.0f;  /* DAT_005560a0 (CameraAcceleration) */
// GLOBAL: LEGORR 0x005560a8
float g_view_pan_scale = 0.0f;  /* DAT_005560a8 (CameraSpeed) */

// FUNCTION: LEGORR 0x00435e0e
void camera_view_context_set(float cap, float follow_scale,
                             float pan_scale, int edge)
{
    g_view_pan_cap = cap;
    g_view_follow_scale = follow_scale;
    g_view_pan_scale = pan_scale;
    g_view_grid_edge = edge;
}

// FUNCTION: LEGORR 0x00436b0b
void selection_record_point_set(ViewContext* v, float x, float y)
{
    if (v != NULL && v->mode == 1) {
        /* corpus: the view record's dword [10] camera (cam_follow), z=0 */
        cam_set_pos(&v->cam_follow, x, y, 0.0f);
    }
}

// FUNCTION: LEGORR 0x00436b33
int camera_get_position_with_terrain_height(ViewContext* v,
                                            EngineContext* ctx,
                                            float out[3])
{
    if (v != NULL && ctx != NULL && out != NULL && v->mode == 1) {
        cam_get_pos(&v->cam_follow, out);
        /* corpus: the terrain leaf takes the world handle; the port routes
         * it through the engine context's bound level block (deviation). */
        out[2] = terrain_height(ctx, out[0], out[1]);
    }
    return 1; /* corpus: constant success, untouched output for mode != 1 */
}

// FUNCTION: LEGORR 0x00436b87
void camera_pointer_pan_update(ViewContext* v, float vec[3], float amount)
{
    if (v == NULL || vec == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    vec_normalize(vec);
    v->pan_accum = amount * g_view_pan_scale + v->pan_accum;
    if (g_view_pan_cap < v->pan_accum) {
        v->pan_accum = g_view_pan_cap;
    }
    vec_scale(vec, vec, v->pan_accum * amount);
    vec_add(v->offset, v->offset, vec);
}

/* --- the camera-controller bounds band (see view.hpp) -----------------------
 * Fixed-offset record operations; the corpus's controller record is not
 * modeled as a struct (its owners are the per-view camera objects, and
 * the pose-dispatch half of the keys' logic stays at the unported scene
 * seam). Only the record-field shape these five leaves need is written. */
#define CAM_FLAGS_OFF 0xb8  /* gate-bit byte */

static CameraPoseDispatchHook s_camera_pose_dispatch_hook;
static void* s_camera_pose_dispatch_context;
static SceneTimingValueGetHook s_scene_timing_value_get_hook;
static SceneTimingCountGetHook s_scene_timing_count_get_hook;
static void* s_scene_timing_hooks_context;
static AnimationObjectTimeUpdateHook s_animation_object_time_update_hook;
static AnimationObjectActionHook s_animation_object_time_get_hook;
static AnimationObjectActionHook s_animation_object_value_get_hook;
static void* s_animation_object_hooks_context;
static RendererSubobjectFindHook s_renderer_subobject_find_hook;
static RendererSubobjectApplyHook s_renderer_subobject_apply_hook;
static void* s_renderer_subobject_hooks_context;
static RendererHandleScalarGetHook s_renderer_handle_scalar_get_hook;
static void* s_renderer_handle_scalar_get_context;

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

/* RendererObjectTimingValueAdjustByCount (FUN_00406d30): read the base
 * timing value from the frame interface and subtract one frame duration for
 * each additional instance when the value is nonzero. */
// FUNCTION: LEGORR 0x00406D30
float renderer_object_timing_value_adjust_by_count(void* record,
                                                   void* argument,
                                                   uint32_t count)
{
    void* frame;
    float value;
    int frame_duration;
    if (record == NULL || s_scene_timing_value_get_hook == NULL) {
        return 0.0f;
    }
    frame = *(void**)((char*)record + 4);
    if (frame == NULL) {
        return 0.0f;
    }
    value = s_scene_timing_value_get_hook(frame, argument,
                                         s_scene_timing_hooks_context);
    if (count > 1u && value != 0.0f &&
        s_scene_timing_count_get_hook != NULL) {
        frame_duration = s_scene_timing_count_get_hook(
            frame, s_scene_timing_hooks_context);
        value -= (float)frame_duration * (float)(count - 1u);
    }
    return value;
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

/* AnimationObjectTimeUpdate (FUN_00406d10): forward the supplied time to
 * the scene frame stored in the animation object's +4 slot. */
// FUNCTION: LEGORR 0x00406D10
void animation_object_time_update(void* record, float time)
{
    void* frame;
    if (record == NULL || s_animation_object_time_update_hook == NULL) return;
    frame = *(void**)((char*)record + 4);
    if (frame != NULL) {
        s_animation_object_time_update_hook(
            frame, time, s_animation_object_hooks_context);
    }
}

/* AnimationObjectTimeGet (FUN_00406da0): query the object's current scene
 * frame through the installed backend callback. */
// FUNCTION: LEGORR 0x00406DA0
void animation_object_time_get(void* record)
{
    void* frame;
    if (record == NULL || s_animation_object_time_get_hook == NULL) return;
    frame = *(void**)((char*)record + 4);
    if (frame != NULL) {
        s_animation_object_time_get_hook(frame,
                                         s_animation_object_hooks_context);
    }
}

/* AnimationObjectValueGet (FUN_00407090): run the value-query method on
 * the scene frame stored at +4. */
// FUNCTION: LEGORR 0x00407090
void animation_object_value_get(void* record)
{
    void* frame;
    if (record == NULL || s_animation_object_value_get_hook == NULL) return;
    frame = *(void**)((char*)record + 4);
    if (frame != NULL) {
        s_animation_object_value_get_hook(
            frame, s_animation_object_hooks_context);
    }
}

void renderer_subobject_hooks_install(RendererSubobjectFindHook find,
                                      RendererSubobjectApplyHook apply,
                                      void* context)
{
    s_renderer_subobject_find_hook = find;
    s_renderer_subobject_apply_hook = apply;
    s_renderer_subobject_hooks_context = context;
}

/* RendererObjectSubobjectDispatch (FUN_00406f50): resolve the requested
 * child from the frame interface, then apply it in the default mode. */
void* renderer_object_subobject_find_apply(void* record, void* key)
{
    void* frame;
    void* details = NULL;
    void* subobject;
    if (record == NULL || s_renderer_subobject_find_hook == NULL ||
        s_renderer_subobject_apply_hook == NULL) {
        return NULL;
    }
    frame = *(void**)((char*)record + 4);
    if (frame == NULL) {
        return NULL;
    }
    subobject = s_renderer_subobject_find_hook(
        frame, key, &details, s_renderer_subobject_hooks_context);
    s_renderer_subobject_apply_hook(frame, subobject, 0, 0,
                                    s_renderer_subobject_hooks_context);
    return subobject;
}

/* Portable adapter for FUN_00406f50. The retail resolver calls the resource
 * tree helpers FUN_00473c60/FUN_00473bc0; no production owner currently
 * supplies that tree through these optional callbacks. */
void renderer_object_subobject_dispatch(void* record, void* key)
{
    (void)renderer_object_subobject_find_apply(record, key);
}

/* RendererCachedHandle48GetOrCreate (FUN_00406fe0): lazily resolve the
 * resource at +0x34 and cache its scene subobject at +0x48. */
// FUNCTION: LEGORR 0x00406FE0
void* renderer_cached_handle_48_get_or_create(void* record)
{
    void** source;
    void** cached;
    if (record == NULL) return NULL;
    source = (void**)((char*)record + 0x34);
    cached = (void**)((char*)record + 0x48);
    if (*source == NULL) return NULL;
    if (*cached == NULL) {
        *cached = renderer_object_subobject_find_apply(record, *source);
    }
    return *cached;
}

/* RendererCachedHandle50GetOrCreate (FUN_00407010): sibling cache with
 * source at +0x3c and cached handle at +0x50. */
// FUNCTION: LEGORR 0x00407010
void* renderer_cached_handle_50_get_or_create(void* record)
{
    void** source;
    void** cached;
    if (record == NULL) return NULL;
    source = (void**)((char*)record + 0x3c);
    cached = (void**)((char*)record + 0x50);
    if (*source == NULL) return NULL;
    if (*cached == NULL) {
        *cached = renderer_object_subobject_find_apply(record, *source);
    }
    return *cached;
}

void renderer_handle_scalar_hook_install(RendererHandleScalarGetHook hook,
                                         void* context)
{
    s_renderer_handle_scalar_get_hook = hook;
    s_renderer_handle_scalar_get_context = context;
}

/* RendererCachedHandle4cIsNotOne (FUN_00407040): lazily resolve +0x38 into
 * +0x4c, then answer whether the handle's scalar property differs from 1. */
// FUNCTION: LEGORR 0x00407040
int renderer_cached_handle_4c_is_not_one(void* record)
{
    void** source;
    void** cached;
    float value;
    if (record == NULL) return 0;
    source = (void**)((char*)record + 0x38);
    cached = (void**)((char*)record + 0x4c);
    if (*cached == NULL && *source != NULL) {
        *cached = renderer_object_subobject_find_apply(record, *source);
    }
    if (*cached == NULL || s_renderer_handle_scalar_get_hook == NULL) {
        return 0;
    }
    value = s_renderer_handle_scalar_get_hook(
        *cached, s_renderer_handle_scalar_get_context);
    return value != 1.0f;
}

static void* scene_object_subobject_resolve(void* owner, void* source,
                                            int index);
static void* scene_object_cached_subobject_get(void* owner,
                                               size_t source_offset,
                                               size_t cache_offset,
                                               int index);

/* SceneObjectIndexedHandleGetOrCreate (FUN_00408770): resolve the indexed
 * child from +8 and cache it in the corresponding +0x28 slot. The installed
 * resolver models the retail resource-tree lookup/apply sequence. */
// FUNCTION: LEGORR 0x00408770
void* scene_object_indexed_handle_get_or_create(void* record, int index)
{
    void** source;
    void** cached;
    if (record == NULL) return NULL;
    source = (void**)((char*)record + 8);
    if (*source == NULL) return NULL;
    cached = (void**)((char*)record + 0x28) + index;
    if (*cached == NULL)
        *cached = scene_object_subobject_resolve(record, *source, index);
    return *cached;
}

/* SceneObjectPrimaryHandleGetOrCreate (FUN_004087b0). */
// FUNCTION: LEGORR 0x004087b0
void* scene_object_primary_handle_get_or_create(void* record)
{
    void** source;
    void** cached;
    if (record == NULL) return NULL;
    source = (void**)((char*)record + 0x10);
    if (*source == NULL) return NULL;
    cached = (void**)((char*)record + 0x68);
    if (*cached == NULL) {
        *cached = scene_object_subobject_resolve(record, *source, 0);
        return *cached;
    }
    return *cached;
}

/* SceneObjectSecondaryHandleGetOrCreate (FUN_004087e0). */
// FUNCTION: LEGORR 0x004087e0
void* scene_object_secondary_handle_get_or_create(void* record)
{
    void** source;
    void** cached;
    if (record == NULL) return NULL;
    source = (void**)((char*)record + 0x14);
    if (*source == NULL) return NULL;
    cached = (void**)((char*)record + 0x6c);
    if (*cached == NULL) {
        *cached = scene_object_subobject_resolve(record, *source, 0);
        return *cached;
    }
    return *cached;
}

/* SceneObjectIndexedSecondaryHandleGetOrCreate (FUN_00408810). */
// FUNCTION: LEGORR 0x00408810
void* scene_object_indexed_secondary_handle_get_or_create(void* record,
                                                           int index)
{
    void** source;
    void** cached;
    if (record == NULL) return NULL;
    source = (void**)((char*)record + 0x18);
    if (*source == NULL) return NULL;
    cached = (void**)((char*)record + 0x50) + index;
    if (*cached == NULL)
        *cached = scene_object_subobject_resolve(record, *source, index);
    return *cached;
}

// AnimationLoopTimeAdjust (FUN_00408860) shares the counted scene-time
// adjustment, then refreshes and clears the optional loop-time latch.
// FUNCTION: LEGORR 0x00408860
float animation_loop_time_adjust(void* record, void* argument,
                                 uint32_t count)
{
    uint32_t* flags;
    void* loop_frame;
    float value;
    if (record == NULL) return 0.0f;
    value = renderer_object_timing_value_adjust_by_count(record, argument,
                                                          count);
    flags = (uint32_t*)((char*)record + 0x148);
    if ((*flags & 2u) != 0u) {
        loop_frame = *(void**)((char*)record + 0x7c);
        if (loop_frame != NULL && s_scene_timing_count_get_hook != NULL &&
            s_animation_object_time_update_hook != NULL) {
            float scale = *(float*)((char*)record + 0x80);
            int duration = s_scene_timing_count_get_hook(
                loop_frame, s_scene_timing_hooks_context);
            s_animation_object_time_update_hook(
                loop_frame, (float)duration * scale,
                s_animation_object_hooks_context);
        }
        *flags &= ~2u;
    }
    return value;
}

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

// FUNCTION: LEGORR 0x00436831
void camera_tilt_bounds_set(void* camera, float lo, float hi)
{
    if (camera == NULL) {
        return; /* deviation: the corpus writes blindly */
    }
    uint8_t* flags = (uint8_t*)((char*)camera + CAM_FLAGS_OFF);
    *flags = (uint8_t)(*flags | 1u);
    __builtin_memcpy((char*)camera + 0x7c, &lo, sizeof(lo));
    __builtin_memcpy((char*)camera + 0x80, &hi, sizeof(hi));
}

/* CameraSetTilt (FUN_00436860): honor the optional bound unless override bit
 * 8 is set, store +0x78, reset the scene object's pose, then apply the
 * negative X-axis rotation. The DDiSceneObjectPoseMethodDispatch leaf is an
 * injectable renderer seam because its owning scene interface varies by
 * backend. */
// FUNCTION: LEGORR 0x00436860
void camera_tilt_set(void* camera, float value)
{
    uint8_t flags;
    float lo, hi;
    void* scene_object;
    void* pose_context;
    if (camera == NULL) {
        return;
    }
    flags = *(uint8_t*)((char*)camera + CAM_FLAGS_OFF);
    __builtin_memcpy(&lo, (char*)camera + 0x7c, sizeof(lo));
    __builtin_memcpy(&hi, (char*)camera + 0x80, sizeof(hi));
    if ((flags & 1u) != 0u && (flags & 8u) == 0u &&
        (value < lo || value > hi)) {
        return;
    }
    __builtin_memcpy((char*)camera + 0x78, &value, sizeof(value));
    scene_object = *(void**)((char*)camera + 0x30);
    pose_context = *(void**)((char*)camera + 0x28);
    if (scene_object == NULL) {
        return;
    }
    if (s_camera_pose_dispatch_hook != NULL) {
        s_camera_pose_dispatch_hook(scene_object, pose_context,
                                    0.0f, 0.0f, 1.0f,
                                    0.0f, 1.0f, 0.0f,
                                    s_camera_pose_dispatch_context);
    }
    render_object_rotate_axis(scene_object, 1u, 1.0f, 0.0f, 0.0f, -value);
}

/* CameraPanAxisUpdate (FUN_00436900): add the delta to the controller's
 * current +0x78 angle and route through CameraSetTilt for the same bounds and
 * scene-transform update. */
// FUNCTION: LEGORR 0x00436900
void camera_pan_axis_update(void* camera, float delta)
{
    camera_tilt_set(camera,
                    delta + *(float*)((char*)camera + 0x78));
}

/* ScreenEffectRecordSet (FUN_00435ebb): copy the four effect parameters
 * around the camera tilt operation in the original record. */
// FUNCTION: LEGORR 0x00435ebb
void screen_effect_record_set(void* camera_record,
                              uint32_t field4, uint32_t field8,
                              uint32_t fieldc, float tilt,
                              uint32_t field10)
{
    if (camera_record == NULL) return;
    memcpy((char*)camera_record + 4, &field4, sizeof(field4));
    memcpy((char*)camera_record + 8, &field8, sizeof(field8));
    memcpy((char*)camera_record + 0x0c, &fieldc, sizeof(fieldc));
    camera_tilt_set(camera_record, tilt);
    memcpy((char*)camera_record + 0x10, &field10, sizeof(field10));
}

// FUNCTION: LEGORR 0x00436921
void camera_scalar_bounds_set(void* camera, float lo, float hi)
{
    if (camera == NULL) {
        return; /* deviation: the corpus writes blindly */
    }
    uint8_t* flags = (uint8_t*)((char*)camera + CAM_FLAGS_OFF);
    *flags = (uint8_t)(*flags | 2u);
    __builtin_memcpy((char*)camera + 0x88, &lo, sizeof(lo));
    __builtin_memcpy((char*)camera + 0x8c, &hi, sizeof(hi));
}

/* CameraSetBoundedRotationValue (FUN_00436953): same optional-range and
 * override gates as the tilt setter, with a reset pose followed by a
 * negative Z-axis rotation. */
// FUNCTION: LEGORR 0x00436953
void camera_rotation_set(void* camera, float value)
{
    uint8_t flags;
    float lo, hi;
    void* scene_object;
    if (camera == NULL) {
        return;
    }
    flags = *(uint8_t*)((char*)camera + CAM_FLAGS_OFF);
    __builtin_memcpy(&lo, (char*)camera + 0x88, sizeof(lo));
    __builtin_memcpy(&hi, (char*)camera + 0x8c, sizeof(hi));
    if ((flags & 2u) != 0u && (flags & 8u) == 0u &&
        (value < lo || value > hi)) {
        return;
    }
    __builtin_memcpy((char*)camera + 0x84, &value, sizeof(value));
    scene_object = *(void**)((char*)camera + 0x28);
    if (scene_object == NULL) {
        return;
    }
    if (s_camera_pose_dispatch_hook != NULL) {
        s_camera_pose_dispatch_hook(scene_object, NULL,
                                    0.0f, 0.0f, 1.0f,
                                    0.0f, 1.0f, 0.0f,
                                    s_camera_pose_dispatch_context);
    }
    render_object_rotate_axis(scene_object, 1u, 0.0f, 0.0f, 1.0f, -value);
}

// FUNCTION: LEGORR 0x00436a18
void camera_distance_bounds_set(void* camera, float lo, float hi)
{
    if (camera == NULL) {
        return; /* deviation: the corpus writes blindly */
    }
    uint8_t* flags = (uint8_t*)((char*)camera + CAM_FLAGS_OFF);
    *flags = (uint8_t)(*flags | 4u);
    __builtin_memcpy((char*)camera + 0x94, &lo, sizeof(lo));
    __builtin_memcpy((char*)camera + 0x98, &hi, sizeof(hi));
}

// FUNCTION: LEGORR 0x00436a4a
void camera_distance_set(void* camera, float value)
{
    if (camera == NULL) {
        return; /* deviation: the corpus writes blindly */
    }
    const uint8_t f = *(const uint8_t*)((const char*)camera + CAM_FLAGS_OFF);
    float lo = 0.0f, hi = 0.0f;
    __builtin_memcpy(&lo, (const char*)camera + 0x94, sizeof(lo));
    __builtin_memcpy(&hi, (const char*)camera + 0x98, sizeof(hi));
    if ((f & 4u) == 0u || (f & 8u) != 0u || (lo <= value && value <= hi)) {
        __builtin_memcpy((char*)camera + 0x90, &value, sizeof(value));
    }
}

// FUNCTION: LEGORR 0x00436aa5
void camera_zoom_update(void* camera, float delta)
{
    if (camera == NULL) {
        return; /* deviation: the corpus reads blindly */
    }
    float cur = 0.0f;
    __builtin_memcpy(&cur, (const char*)camera + 0x90, sizeof(cur));
    camera_distance_set(camera, delta + cur);
}

/* CursorScreenRayContextReset (FUN_00435f16) — see view.hpp. */
// FUNCTION: LEGORR 0x00435f16
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void cursor_screen_ray_context_reset(ViewContext* v)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        test eax, eax
        je cursor_screen_ray_reset_null
        mov dword ptr [eax + 0x9c], 0
    cursor_screen_ray_reset_null:
        ret
    }
}
#else
void cursor_screen_ray_context_reset(ViewContext* v)
{
    if (v == NULL) {
        return; /* deviation: the corpus stores blindly */
    }
    v->pan_accum = 0.0f; /* the corpus's view-record +0x9c slot */
}
#endif

/* CurrentLevelTerrainGridPick (FUN_00433be0) — see view.hpp. */
// FUNCTION: LEGORR 0x00433be0
void current_level_terrain_grid_pick(EngineContext* ctx, float x, float y,
                                     int* gx, int* gy)
{
    const float pos[2] = { x, y };
    int tx = 0, ty = 0;
    int* px = gx != NULL ? gx : &tx;
    int* py = gy != NULL ? gy : &ty;
    (void)engine_context_grid_pick(ctx, pos, px, py);
}

void terrain_point_pick_cell_check_hook_set(
    TerrainPointPickCellCheckHook hook, void* user)
{
    s_terrain_point_pick_hook = hook;
    s_terrain_point_pick_user = user;
}

// FUNCTION: LEGORR 0x0045e6d0
int terrain_point_pick_resolve(const TerrainPointPickRegion* region,
                               uint32_t screen_x, uint32_t screen_y,
                               float out_world_xy[2],
                               uint32_t argument_a,
                               uint32_t argument_b)
{
    float world_x;
    float world_y;
    float x = (float)screen_x;
    float y = (float)screen_y;
    if (region == NULL || s_terrain_point_pick_hook == NULL ||
        region->tile_size == 0.0f ||
        x < region->screen_left ||
        x >= region->screen_left + region->screen_width ||
        y < region->screen_top ||
        y >= region->screen_top + region->screen_height) {
        return 0;
    }
    world_y = region->scale *
        ((region->screen_top + region->screen_width * 0.5f) - y) /
        region->tile_size + region->world_origin_y;
    world_x = region->world_origin_x - region->scale *
        ((region->screen_left + region->screen_width * 0.5f) - x) /
        region->tile_size;
    if (!s_terrain_point_pick_hook(region->grid_token, world_x, world_y,
                                   argument_a, argument_b,
                                   s_terrain_point_pick_user)) {
        return 0;
    }
    if (out_world_xy != NULL) {
        out_world_xy[0] = world_x;
        out_world_xy[1] = world_y;
    }
    return 1;
}

void viewport_point_dispatch_hooks_set(const ViewportPointDispatchHooks* hooks)
{
    if (hooks == NULL)
        memset(&s_viewport_point_dispatch_hooks, 0,
               sizeof(s_viewport_point_dispatch_hooks));
    else
        s_viewport_point_dispatch_hooks = *hooks;
}

// FUNCTION: LEGORR 0x0045e820
int viewport_point_callback_dispatch(const TerrainPointPickRegion* region,
                                     uint32_t screen_x, uint32_t screen_y,
                                     uint32_t* object_handle,
                                     float out_position_xy[2])
{
    float world_xy[2];
    float position[3];
    uint32_t handle = 0u;
    if (object_handle != NULL) *object_handle = 0u;
    if (region == NULL || object_handle == NULL ||
        s_viewport_point_dispatch_hooks.resolve == NULL ||
        !terrain_point_pick_resolve(region, screen_x, screen_y, world_xy,
                                    0u, 0u)) {
        return 0;
    }
    if (!s_viewport_point_dispatch_hooks.resolve(region->grid_token,
            world_xy[0], world_xy[1], region->scale, &handle,
            s_viewport_point_dispatch_hooks.user) || handle == 0u) {
        return 0;
    }
    *object_handle = handle;
    if (out_position_xy != NULL &&
        s_viewport_point_dispatch_hooks.position_get != NULL &&
        s_viewport_point_dispatch_hooks.position_get(handle, position,
            s_viewport_point_dispatch_hooks.user)) {
        out_position_xy[0] = position[0];
        out_position_xy[1] = position[1];
    }
    return 1;
}

void textured_scene_draw_hooks_set(const TexturedSceneDrawHooks* hooks)
{
    if (hooks == NULL)
        memset(&s_textured_scene_draw_hooks, 0,
               sizeof(s_textured_scene_draw_hooks));
    else
        s_textured_scene_draw_hooks = *hooks;
}

// FUNCTION: LEGORR 0x0045db00
void textured_scene_object_draw_at_position(const void* view,
                                            const float position_xy[2],
                                            float z, float size)
{
    float saved_matrix[16];
    float position[4];
    float scale;
    int width;
    int saved = 0;
    if (view == NULL || position_xy == NULL ||
        s_textured_scene_draw_hooks.draw == NULL) return;
    if (s_textured_scene_draw_hooks.state_save != NULL)
        saved = s_textured_scene_draw_hooks.state_save(
            s_textured_scene_draw_hooks.user, saved_matrix);
    if (s_textured_scene_draw_hooks.view_transform_set != NULL)
        s_textured_scene_draw_hooks.view_transform_set(
            s_textured_scene_draw_hooks.user, (const char*)view + 4);
    position[0] = position_xy[0];
    position[1] = position_xy[1];
    position[2] = z;
    position[3] = 0.0f;
    view_point_to_screen_geometry_transform(view, position);
    scale = size * 0.7f + 0.3f;
    width = (int)scale;
    s_textured_scene_draw_hooks.draw(s_textured_scene_draw_hooks.user,
        position, width, width * 2, scale * 0.7f, scale * 0.9f, scale);
    if (saved && s_textured_scene_draw_hooks.state_restore != NULL)
        s_textured_scene_draw_hooks.state_restore(
            s_textured_scene_draw_hooks.user, saved_matrix);
}

/* GridOperationArgumentAdapter (FUN_00442540) — see view.hpp. */
// FUNCTION: LEGORR 0x00442540
void grid_operation_argument_adapter(float x, float y, EngineContext* ctx)
{
    if (ctx == NULL) {
        return; /* deviation: the corpus reads its own world blindly */
    }
    (void)terrain_height(ctx, x, y);
}

/* SetGlobalScalarToOne (FUN_0040fec0) — see view.hpp. */
// FUNCTION: LEGORR 0x0040fec0
void set_global_scalar_to_one(void)
{
    (void)random_float_range(0.0f, 1.0f);
}

/* FloatNegateAbsoluteValue (FUN_0044d250) — see view.hpp. */
// FUNCTION: LEGORR 0x0044d250
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void float_negate_absolute_value(float* value)
{
    __asm {
        mov ecx, dword ptr [esp + 4]
        test ecx, ecx
        je float_negate_null
        fld dword ptr [ecx]
        fcomp dword ptr ds:[0049e4b0h]
        fld dword ptr [ecx]
        fnstsw ax
        test ah, 1
        je float_negate_nonnegative
        fmul dword ptr ds:[0049e4c8h]
        fstp dword ptr [ecx]
        ret

    float_negate_nonnegative:
        fchs
        fstp dword ptr [ecx]
    float_negate_null:
        ret
    }
}
#else
void float_negate_absolute_value(float* value)
{
    if (value == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    *value = -*value; /* both corpus branches negate */
}
#endif

/* Vector2SetLength (FUN_00406760) — see view.hpp. */
// FUNCTION: LEGORR 0x00406760
#if defined(_MSC_VER) && defined(_M_IX86)
/* Preserve the port's NULL and zero-length guards, then mirror the corpus's
 * x87 body and its repeated stack-slot operands. */
__declspec(naked) void vector2_set_length(float v[2], float length)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        test eax, eax
        je vector2_length_return
        fld dword ptr [esp + 8]
        fldz
        fcompp
        fstsw ax
        and ah, 0x45
        cmp ah, 0x40
        je vector2_length_return

        mov eax, dword ptr [esp + 4]
        mov ecx, dword ptr [eax]
        fld dword ptr [eax + 4]
        mov dword ptr [esp + 4], ecx
        fld dword ptr [esp + 4]
        fld st(1)
        fmul st, st(2)
        fld dword ptr [eax]
        fxch st(2)
        fmul dword ptr [esp + 4]
        fld dword ptr [eax + 4]
        fxch st(1)
        faddp st(2), st
        fxch st(1)
        fsqrt
        fst dword ptr [esp + 4]
        fdiv dword ptr [esp + 8]
        fstp dword ptr [esp + 4]
        fxch st(1)
        fdiv dword ptr [esp + 4]
        fxch st(1)
        fdiv dword ptr [esp + 4]
        fxch st(1)
        fstp dword ptr [eax]
        fstp dword ptr [eax + 4]
        fstp st(0)
        ret

    vector2_length_return:
        ret
    }
}
#else
void vector2_set_length(float v[2], float length)
{
    if (v == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    if (length == 0.0f) {
        return; /* deviation: the corpus divides blindly */
    }
    const float s = sqrtf(v[0] * v[0] + v[1] * v[1]) / length;
    v[0] = v[0] / s;
    v[1] = v[1] / s;
}
#endif

/* CameraZoomStepAdjust (FUN_0045bf20) — see view.hpp. */
void camera_zoom_step_context_set(void* context)
{
    s_zoom_step_context = context;
}

// FUNCTION: LEGORR 0x0045bf20
void camera_zoom_step_adjust(int direction, float step)
{
    if (direction == 0) {
        camera_zoom_update(s_zoom_step_context, step * -3.0f);
        return; /* layout: the corpus's arm0 ends at its own ret (0x45bf44) */
    }
    if (direction == 1) {
        camera_zoom_update(s_zoom_step_context, step * 3.0f);
    }
    /* other directions: the corpus's early exit — inert */
}

/* SelectionRecordPointGet (FUN_00436ae9) — see view.hpp. */
// FUNCTION: LEGORR 0x00436ae9
void selection_record_point_get(ViewContext* v, float out[3])
{
    if (v != NULL && out != NULL && v->mode == 1) {
        /* corpus: the view record's dword [10] camera (cam_follow) */
        cam_get_pos(&v->cam_follow, out);
    }
}

/* SelectedGlobalPointTransformAndReturnX (FUN_0046a420) — see view.hpp. */
// FUNCTION: LEGORR 0x0046a420
float selected_global_point_transform_and_return_x(EngineContext* ctx)
{
    if (ctx == NULL) {
        return 0.0f; /* deviation: the corpus strides the globals blindly */
    }
    ViewContext* v = (ctx->context_mode == 1) ? ctx->views[0] : ctx->views[2];
    float frame[3] = { 0.0f, 0.0f, 0.0f };
    (void)camera_get_position_with_terrain_height(v, ctx, frame);
    return frame[0];
}

/* RendererNodeRelativePositionSet (FUN_00436ac9) — see view.hpp. */
// FUNCTION: LEGORR 0x00436ac9
void renderer_node_relative_position_set(const void* node, float x, float y)
{
    uint32_t handle = 0u;
    if (node != NULL) {
        __builtin_memcpy(&handle, (const char*)node + 0x28, sizeof handle);
    }
    cam_offset((CameraHandle*)handle, x, y, 0.0f);
}

/* ---- scene-root hook band (2026-10-24) ----------------------------------- *
 * SceneRootObjectGet / DDiSceneObjectArgumentsNormalize — see view.hpp. The
 * corpus's root global (DAT_0076add4) is written only by the unported
 * renderer-side initializer, so the port carries it as a hooked static. */
// GLOBAL: LEGORR 0x0076add4
static void* s_scene_root;
// GLOBAL: LEGORR 0x0076cd8c
static uint32_t s_scene_fog_color;
// GLOBAL: LEGORR 0x00505920
static uint32_t s_scene_fog_mode;

/* Reconstruction owner for the corpus DDiScene render-surface record pool. */
// GLOBAL: LEGORR 0x0076ace0
void* s_render_surface_pool_blocks[32];
// GLOBAL: LEGORR 0x0076ad64
unsigned int s_render_surface_pool_level_count;
// GLOBAL: LEGORR 0x0076ad60
void* s_render_surface_pool_free_head;

static size_t render_surface_pool_stride(void)
{
    return (size_t)0x20u + (sizeof(void*) - sizeof(uint32_t));
}

static void* render_surface_pool_link_get(void* record)
{
    void* next;
    memcpy(&next, (unsigned char*)record + 0x1c, sizeof(next));
    return next;
}

static void render_surface_pool_link_set(void* record, void* next)
{
    memcpy((unsigned char*)record + 0x1c, &next, sizeof(next));
}

// FUNCTION: LEGORR 0x00477370
void ddiscene_render_surface_pool_grow(void)
{
    unsigned int count;
    unsigned int i;
    size_t stride;
    unsigned char* block;
    if (s_render_surface_pool_level_count >= 32u) {
        return;
    }
    count = 1u << s_render_surface_pool_level_count;
    stride = render_surface_pool_stride();
    if ((size_t)count > (size_t)0x7fffffff / stride) {
        return;
    }
    block = (unsigned char*)engine_heap_alloc((long)((size_t)count * stride));
    if (block == NULL) {
        return;
    }
    s_render_surface_pool_blocks[s_render_surface_pool_level_count] = block;
    ++s_render_surface_pool_level_count;
    for (i = 0; i + 1u < count; ++i) {
        render_surface_pool_link_set(block + (size_t)i * stride,
                                     block + (size_t)(i + 1u) * stride);
    }
    render_surface_pool_link_set(block + (size_t)(count - 1u) * stride,
                                 s_render_surface_pool_free_head);
    s_render_surface_pool_free_head = block;
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

static void* s_scene_registry_blocks[32];
static unsigned int s_scene_registry_block_count;
static void* s_scene_registry_free_head;
static SceneRegistrySurfaceReleaseFn s_scene_registry_surface_release;

static size_t scene_registry_record_stride(void)
{
    return (size_t)0x1cu + sizeof(void*);
}

static void* scene_registry_record_next_get(void* record)
{
    void* next;
    memcpy(&next, (unsigned char*)record + 0x1c, sizeof(next));
    return next;
}

static void scene_registry_record_next_set(void* record, void* next)
{
    memcpy((unsigned char*)record + 0x1c, &next, sizeof(next));
}

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

// FUNCTION: LEGORR 0x0047e130
void scene_object_registry_release_all(void)
{
    unsigned int level;
    size_t stride = scene_registry_record_stride();
    for (level = 0u; level < s_scene_registry_block_count; ++level) {
        unsigned int count = 1u << level;
        unsigned int i;
        unsigned char* block =
            (unsigned char*)s_scene_registry_blocks[level];
        for (i = 0u; i < count; ++i) {
            void* record = block + (size_t)i * stride;
            if (scene_registry_record_next_get(record) == record) {
                void* surface;
                memcpy(&surface, record, sizeof(surface));
                if (s_scene_registry_surface_release != NULL) {
                    s_scene_registry_surface_release(surface);
                } else {
                    texture_release((TextureSurface*)surface);
                }
                scene_registry_record_next_set(record,
                                                s_scene_registry_free_head);
                s_scene_registry_free_head = record;
            }
        }
    }
}

// FUNCTION: LEGORR 0x0047d430
void scene_object_registry_teardown(void)
{
    unsigned int i;
    scene_object_registry_release_all();
    fixed_size_small_record_pool_teardown();
    for (i = 0u; i < s_scene_registry_block_count; ++i) {
        s_scene_registry_blocks[i] = NULL;
    }
    s_scene_registry_block_count = 0u;
    s_scene_registry_free_head = NULL;
}

void scene_root_install(void* root) { s_scene_root = root; }
void* scene_root_peek(void) { return s_scene_root; }

/* SceneRootObjectGet (FUN_00472940) — the corpus's root-object read. */
// FUNCTION: LEGORR 0x00472940
void* scene_root_object_get(void)
{
    return s_scene_root;
}

/* The scene object wrapper stores its interface in its first word. These
 * renderer leaves dispatch through fixed vtable slots on that interface. */
class SceneRootInterface
{
public:
#define SCENE_INTERFACE_SLOT(n) virtual void slot_##n(void) = 0
    SCENE_INTERFACE_SLOT(0); SCENE_INTERFACE_SLOT(1);
    SCENE_INTERFACE_SLOT(2); SCENE_INTERFACE_SLOT(3);
    SCENE_INTERFACE_SLOT(4); SCENE_INTERFACE_SLOT(5);
    SCENE_INTERFACE_SLOT(6); SCENE_INTERFACE_SLOT(7);
    SCENE_INTERFACE_SLOT(8); SCENE_INTERFACE_SLOT(9);
    SCENE_INTERFACE_SLOT(10); SCENE_INTERFACE_SLOT(11);
    SCENE_INTERFACE_SLOT(12); SCENE_INTERFACE_SLOT(13);
    SCENE_INTERFACE_SLOT(14);
    virtual void clip_distance_set(float distance) = 0;
    SCENE_INTERFACE_SLOT(16); SCENE_INTERFACE_SLOT(17);
    SCENE_INTERFACE_SLOT(18); SCENE_INTERFACE_SLOT(19);
    virtual void transform_point(void* output, const void* input) = 0;
    SCENE_INTERFACE_SLOT(21);
    SCENE_INTERFACE_SLOT(22); SCENE_INTERFACE_SLOT(23);
    SCENE_INTERFACE_SLOT(24); SCENE_INTERFACE_SLOT(25);
    SCENE_INTERFACE_SLOT(26); SCENE_INTERFACE_SLOT(27);
    SCENE_INTERFACE_SLOT(28); SCENE_INTERFACE_SLOT(29);
    SCENE_INTERFACE_SLOT(30); SCENE_INTERFACE_SLOT(31);
    virtual unsigned int render_surface_value_pair_80_get(void) = 0;
    virtual unsigned int render_surface_value_pair_84_get(void) = 0;
    SCENE_INTERFACE_SLOT(34); SCENE_INTERFACE_SLOT(35);
    SCENE_INTERFACE_SLOT(36); SCENE_INTERFACE_SLOT(37);
    SCENE_INTERFACE_SLOT(38); SCENE_INTERFACE_SLOT(39);
    SCENE_INTERFACE_SLOT(40); SCENE_INTERFACE_SLOT(41);
    SCENE_INTERFACE_SLOT(42); SCENE_INTERFACE_SLOT(43);
    SCENE_INTERFACE_SLOT(44); SCENE_INTERFACE_SLOT(45);
    SCENE_INTERFACE_SLOT(46);
    virtual void fog_color_enable(unsigned int color) = 0;
    SCENE_INTERFACE_SLOT(48); SCENE_INTERFACE_SLOT(49);
    SCENE_INTERFACE_SLOT(50);
    virtual void fog_enable(int enabled) = 0;
    virtual void fog_color_set(unsigned int color) = 0;
    virtual void fog_mode_set(unsigned int mode) = 0;
    virtual void fog_parameters_set(unsigned int start, unsigned int end,
                                    unsigned int density) = 0;
    SCENE_INTERFACE_SLOT(55); SCENE_INTERFACE_SLOT(56);
    SCENE_INTERFACE_SLOT(57); SCENE_INTERFACE_SLOT(58);
    SCENE_INTERFACE_SLOT(59); SCENE_INTERFACE_SLOT(60);
    SCENE_INTERFACE_SLOT(61); SCENE_INTERFACE_SLOT(62);
    SCENE_INTERFACE_SLOT(63); SCENE_INTERFACE_SLOT(64);
    SCENE_INTERFACE_SLOT(65); SCENE_INTERFACE_SLOT(66);
    SCENE_INTERFACE_SLOT(67); SCENE_INTERFACE_SLOT(68);
    SCENE_INTERFACE_SLOT(69); SCENE_INTERFACE_SLOT(70);
    SCENE_INTERFACE_SLOT(71); SCENE_INTERFACE_SLOT(72);
    SCENE_INTERFACE_SLOT(73); SCENE_INTERFACE_SLOT(74);
    SCENE_INTERFACE_SLOT(75); SCENE_INTERFACE_SLOT(76);
    SCENE_INTERFACE_SLOT(77); SCENE_INTERFACE_SLOT(78);
    SCENE_INTERFACE_SLOT(79); SCENE_INTERFACE_SLOT(80);
    SCENE_INTERFACE_SLOT(81); SCENE_INTERFACE_SLOT(82);
    SCENE_INTERFACE_SLOT(83); SCENE_INTERFACE_SLOT(84);
    virtual void fog_render_mode_set(unsigned int mode) = 0;
#undef SCENE_INTERFACE_SLOT
};

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

class SceneObjectTransformInterface
{
public:
    virtual void slot_00(void) = 0; virtual void slot_04(void) = 0;
    virtual void slot_08(void) = 0; virtual void slot_0c(void) = 0;
    virtual void slot_10(void) = 0; virtual void slot_14(void) = 0;
    virtual void slot_18(void) = 0; virtual void slot_1c(void) = 0;
    virtual void slot_20(void) = 0; virtual void slot_24(void) = 0;
    virtual void slot_28(void) = 0; virtual void slot_2c(void) = 0;
    virtual void slot_30(void) = 0; virtual void slot_34(void) = 0;
    virtual void transform_matrix_set(uint32_t mode,
                                      const float* matrix) = 0;
    virtual void slot_3c(uint32_t mode, float x, float y, float z) = 0;
    virtual void frame_scale_set(uint32_t mode, float x, float y,
                                 float z) = 0;
    virtual void rotate_axis(uint32_t mode, float x, float y, float z,
                             float angle) = 0;
};

#define DDI_COORD_SLOT(n) virtual void slot_##n(void) = 0
class SceneObjectCoordinateInterface
{
public:
    DDI_COORD_SLOT(00); DDI_COORD_SLOT(01); DDI_COORD_SLOT(02);
    DDI_COORD_SLOT(03); DDI_COORD_SLOT(04); DDI_COORD_SLOT(05);
    DDI_COORD_SLOT(06); DDI_COORD_SLOT(07); DDI_COORD_SLOT(08);
    DDI_COORD_SLOT(09); DDI_COORD_SLOT(10); DDI_COORD_SLOT(11);
    DDI_COORD_SLOT(12); DDI_COORD_SLOT(13); DDI_COORD_SLOT(14);
    DDI_COORD_SLOT(15); DDI_COORD_SLOT(16); DDI_COORD_SLOT(17);
    DDI_COORD_SLOT(18); DDI_COORD_SLOT(19); DDI_COORD_SLOT(20);
    DDI_COORD_SLOT(21); DDI_COORD_SLOT(22); DDI_COORD_SLOT(23);
    DDI_COORD_SLOT(24); DDI_COORD_SLOT(25); DDI_COORD_SLOT(26);
    DDI_COORD_SLOT(27); DDI_COORD_SLOT(28); DDI_COORD_SLOT(29);
    DDI_COORD_SLOT(30); DDI_COORD_SLOT(31); DDI_COORD_SLOT(32);
    virtual void coordinate_transform_to_output(void* input,
                                               void* output) = 0;
    DDI_COORD_SLOT(34); DDI_COORD_SLOT(35); DDI_COORD_SLOT(36);
    DDI_COORD_SLOT(37); DDI_COORD_SLOT(38); DDI_COORD_SLOT(39);
    DDI_COORD_SLOT(40); DDI_COORD_SLOT(41); DDI_COORD_SLOT(42);
    DDI_COORD_SLOT(43); DDI_COORD_SLOT(44); DDI_COORD_SLOT(45);
    DDI_COORD_SLOT(46); DDI_COORD_SLOT(47); DDI_COORD_SLOT(48);
    DDI_COORD_SLOT(49); DDI_COORD_SLOT(50); DDI_COORD_SLOT(51);
    DDI_COORD_SLOT(52); DDI_COORD_SLOT(53); DDI_COORD_SLOT(54);
    DDI_COORD_SLOT(55); DDI_COORD_SLOT(56); DDI_COORD_SLOT(57);
    DDI_COORD_SLOT(58); DDI_COORD_SLOT(59); DDI_COORD_SLOT(60);
    DDI_COORD_SLOT(61); DDI_COORD_SLOT(62); DDI_COORD_SLOT(63);
    DDI_COORD_SLOT(64); DDI_COORD_SLOT(65);
    virtual void coordinate_transform_in_place(void* coordinates,
                                              void* transform) = 0;
};
#undef DDI_COORD_SLOT

/* DDiSceneObject pose interface. Keep the vtable ordinals explicit: world
 * position get/set and pose dispatch live at byte offsets 0x60, 0xf0, and
 * 0xec in the reference interface. */
#define DDI_POSE_SLOT(n) virtual void slot_##n(void) = 0
class DDiSceneObjectPoseInterface
{
public:
    DDI_POSE_SLOT(00); DDI_POSE_SLOT(04); DDI_POSE_SLOT(08);
    DDI_POSE_SLOT(0c); DDI_POSE_SLOT(10); DDI_POSE_SLOT(14);
    DDI_POSE_SLOT(18); DDI_POSE_SLOT(1c); DDI_POSE_SLOT(20);
    DDI_POSE_SLOT(24); DDI_POSE_SLOT(28); DDI_POSE_SLOT(2c);
    DDI_POSE_SLOT(30); DDI_POSE_SLOT(34); DDI_POSE_SLOT(38);
    DDI_POSE_SLOT(3c); DDI_POSE_SLOT(40); DDI_POSE_SLOT(44);
    DDI_POSE_SLOT(48); DDI_POSE_SLOT(4c); DDI_POSE_SLOT(50);
    DDI_POSE_SLOT(54); DDI_POSE_SLOT(58); DDI_POSE_SLOT(5c);
    virtual void position_get(void* alternate_frame, float position[3]) = 0;
    DDI_POSE_SLOT(64); DDI_POSE_SLOT(68); DDI_POSE_SLOT(6c);
    DDI_POSE_SLOT(70); DDI_POSE_SLOT(74); DDI_POSE_SLOT(78);
    DDI_POSE_SLOT(7c); DDI_POSE_SLOT(80); DDI_POSE_SLOT(84);
    DDI_POSE_SLOT(88); DDI_POSE_SLOT(8c); DDI_POSE_SLOT(90);
    DDI_POSE_SLOT(94); DDI_POSE_SLOT(98); DDI_POSE_SLOT(9c);
    DDI_POSE_SLOT(a0); DDI_POSE_SLOT(a4); DDI_POSE_SLOT(a8);
    DDI_POSE_SLOT(ac); DDI_POSE_SLOT(b0); DDI_POSE_SLOT(b4);
    DDI_POSE_SLOT(b8); DDI_POSE_SLOT(bc); DDI_POSE_SLOT(c0);
    DDI_POSE_SLOT(c4); DDI_POSE_SLOT(c8); DDI_POSE_SLOT(cc);
    DDI_POSE_SLOT(d0); DDI_POSE_SLOT(d4); DDI_POSE_SLOT(d8);
    DDI_POSE_SLOT(dc); DDI_POSE_SLOT(e0); DDI_POSE_SLOT(e4);
    DDI_POSE_SLOT(e8);
    virtual void pose_method_dispatch(void* alternate_frame,
                                     float parameter_3,
                                     float parameter_4,
                                     float parameter_5,
                                     float parameter_6,
                                     float parameter_7,
                                     float parameter_8) = 0;
    virtual void position_set(void* alternate_frame,
                              float x, float y, float z) = 0;
};
#undef DDI_POSE_SLOT

class SceneLightInterface
{
public:
    virtual void slot_00(void) = 0; virtual void slot_04(void) = 0;
    virtual void slot_08(void) = 0; virtual void slot_0c(void) = 0;
    virtual void slot_10(void) = 0; virtual void slot_14(void) = 0;
    virtual void slot_18(void) = 0; virtual void slot_1c(void) = 0;
    virtual void slot_20(void) = 0; virtual void slot_24(void) = 0;
    virtual void slot_28(void) = 0; virtual void slot_2c(void) = 0;
    virtual void slot_30(void) = 0; virtual void slot_34(void) = 0;
    virtual int slot_38(uint32_t value) = 0;
    virtual int slot_3c(uint32_t value) = 0;
    virtual int slot_40(uint32_t value) = 0;
};

static SceneLightInterface* scene_light_interface(void* record)
{
    void* const light = *(void**)((char*)record + 0x0c);
    return (SceneLightInterface*)*(void**)((char*)light + 4);
}

/* DDiSceneAnimationDurationGet (FUN_004761a0). */
// FUNCTION: LEGORR 0x004761a0
float ddiscene_animation_duration_get(void* scene_interface)
{
    void* record = ((DDiScenePropertyInterface*)scene_interface)
        ->current_property_get();
    return record != NULL ? *(float*)((char*)record + 0x10) : 0.0f;
}

/* DDiSceneAnimationStateGet (FUN_004761e0). */
// FUNCTION: LEGORR 0x004761e0
uint32_t ddiscene_animation_state_get(void* scene_interface)
{
    void* record = ((DDiScenePropertyInterface*)scene_interface)
        ->current_property_get();
    return record != NULL ? *(uint32_t*)((char*)record + 0x18) : 0u;
}

/* FUN_004761e0's +0x18 value is passed as a sample name to the registered
 * frame callback. Preserve it as a native pointer in the portable runtime. */
const char* ddiscene_animation_sample_name_get(void* scene_interface)
{
    void* record = ((DDiScenePropertyInterface*)scene_interface)
        ->current_property_get();
    return record != NULL ? *(const char**)((char*)record + 0x18) : NULL;
}

/* DDiSceneAnimationLastFrameGet (FUN_00476200). */
// FUNCTION: LEGORR 0x00476200
uint32_t ddiscene_animation_last_frame_get(void* scene_interface)
{
    void* record = ((DDiScenePropertyInterface*)scene_interface)
        ->current_property_get();
    return record != NULL ? *(uint32_t*)((char*)record + 0x20) : 0u;
}

/* DDiSceneActionRecordField1CGet (FUN_00476160). */
// FUNCTION: LEGORR 0x00476160
uint32_t ddiscene_action_record_field_1c_get(void* scene_interface)
{
    void* record = ((DDiScenePropertyInterface*)scene_interface)
        ->current_property_get();
    return record != NULL ? *(uint32_t*)((char*)record + 0x1c) : 0u;
}

/* DDiSceneActionRecordField0CGet (FUN_00476180). */
// FUNCTION: LEGORR 0x00476180
uint32_t ddiscene_action_record_field_0c_get(void* scene_interface)
{
    void* record = ((DDiScenePropertyInterface*)scene_interface)
        ->current_property_get();
    return record != NULL ? *(uint32_t*)((char*)record + 0x0c) : 0u;
}

/* ScenePropertyValueAt08Get (FUN_00476310). */
// FUNCTION: LEGORR 0x00476310
uint32_t scene_property_value_at08_get(void* scene_interface)
{
    void* record = ((DDiScenePropertyInterface*)scene_interface)
        ->current_property_get();
    return record != NULL ? *(uint32_t*)((char*)record + 0x08) : 0u;
}

/* SceneObjectTransformMatrixSet (FUN_004756b0). The wrapper stores the
 * renderer interface pointer in its first word; the corpus dispatches slot
 * 0x38 with the caller's mode and matrix. */
// FUNCTION: LEGORR 0x004756b0
void scene_object_transform_matrix_set(void* object, uint32_t mode,
                                       const float* matrix)
{
    SceneObjectTransformInterface* const scene =
        (SceneObjectTransformInterface*)*(void**)object;
    scene->transform_matrix_set(mode, matrix);
}

/* RenderObjectSetIdentityTransform (FUN_00475610). */
// FUNCTION: LEGORR 0x00475610
void render_object_set_identity_transform(void* object)
{
    float matrix[16];
    matrix4_set_identity(matrix);
    scene_object_transform_matrix_set(object, 0u, matrix);
}

/* RenderObjectRotateAxis (FUN_00475580), vtable slot +0x44. */
// FUNCTION: LEGORR 0x00475580
void render_object_rotate_axis(void* object, uint32_t mode, float x, float y,
                               float z, float angle)
{
    SceneObjectTransformInterface* const scene =
        (SceneObjectTransformInterface*)*(void**)object;
    scene->rotate_axis(mode, x, y, z, angle);
}

/* SceneFrameScaleSet (FUN_004755b0), vtable slot +0x40. */
// FUNCTION: LEGORR 0x004755b0
void scene_frame_scale_set(void* object, uint32_t mode, float x, float y,
                           float z)
{
    SceneObjectTransformInterface* const scene =
        (SceneObjectTransformInterface*)*(void**)object;
    scene->frame_scale_set(mode, x, y, z);
}

/* SceneObjectCoordinateTransformToOutput (FUN_00475090), interface slot
 * +0x84 as recovered in Finding 337. */
// FUNCTION: LEGORR 0x00475090
void scene_object_coordinate_transform_to_output(void* object,
                                                  void* input, void* output)
{
    SceneObjectCoordinateInterface* const scene =
        (SceneObjectCoordinateInterface*)*(void**)object;
    scene->coordinate_transform_to_output(input, output);
}

/* SceneObjectCoordinateTransformInPlace (FUN_00475070), interface slot
 * +0x108 as recovered in Finding 337. */
// FUNCTION: LEGORR 0x00475070
void scene_object_coordinate_transform_in_place(void* object,
                                                void* coordinates,
                                                void* transform)
{
    SceneObjectCoordinateInterface* const scene =
        (SceneObjectCoordinateInterface*)*(void**)object;
    scene->coordinate_transform_in_place(coordinates, transform);
}

static DDiSceneObjectPoseInterface* ddiscene_pose_interface_resolve(
    void* object, void* alternate_frame, void** normalized_frame)
{
    const void* object_argument;
    const void* alternate_argument;
    const void* normalized_object = NULL;
    const void* resolved_frame = NULL;
    void* normalized_object_mutable = NULL;
    if (object == NULL || normalized_frame == NULL) return NULL;
    object_argument = object;
    alternate_argument = alternate_frame;
    scene_object_arguments_normalize(&object_argument,
        alternate_frame != NULL ? &alternate_argument : NULL,
        &normalized_object_mutable, &resolved_frame);
    normalized_object = normalized_object_mutable;
    if (normalized_object == NULL) return NULL;
    *normalized_frame = (void*)resolved_frame;
    return (DDiSceneObjectPoseInterface*)normalized_object;
}

// FUNCTION: LEGORR 0x004754c0
int ddiscene_object_position_get(void* object, void* alternate_frame,
                                 float position[3])
{
    void* normalized_frame = NULL;
    DDiSceneObjectPoseInterface* scene;
    if (position == NULL) return 0;
    scene = ddiscene_pose_interface_resolve(object, alternate_frame,
                                            &normalized_frame);
    if (scene == NULL) return 0;
    scene->position_get(normalized_frame, position);
    return 1;
}

// FUNCTION: LEGORR 0x00475430
int ddiscene_object_position_set(void* object, void* alternate_frame,
                                 float x, float y, float z)
{
    void* normalized_frame = NULL;
    DDiSceneObjectPoseInterface* scene =
        ddiscene_pose_interface_resolve(object, alternate_frame,
                                        &normalized_frame);
    if (scene == NULL) return 0;
    scene->position_set(normalized_frame, x, y, z);
    return 1;
}

// FUNCTION: LEGORR 0x00475470
int ddiscene_object_pose_method_dispatch(void* object,
                                         void* alternate_frame,
                                         float parameter_3,
                                         float parameter_4,
                                         float parameter_5,
                                         float parameter_6,
                                         float parameter_7,
                                         float parameter_8)
{
    void* normalized_frame = NULL;
    DDiSceneObjectPoseInterface* scene =
        ddiscene_pose_interface_resolve(object, alternate_frame,
                                        &normalized_frame);
    if (scene == NULL) return 0;
    scene->pose_method_dispatch(normalized_frame, parameter_3, parameter_4,
        parameter_5, parameter_6, parameter_7, parameter_8);
    return 1;
}

/* Light interface forwarding leaves (Finding 430). The reference treats
 * zero from each vtable call as success. */
// FUNCTION: LEGORR 0x004734a0
int light_interface_call_slot_38(void* record, uint32_t value)
{
    return scene_light_interface(record)->slot_38(value) == 0;
}

// FUNCTION: LEGORR 0x00473480
int light_interface_call_slot_3c(void* record, uint32_t value)
{
    return scene_light_interface(record)->slot_3c(value) == 0;
}

// FUNCTION: LEGORR 0x00473460
int light_interface_call_slot_40(void* record, uint32_t value)
{
    return scene_light_interface(record)->slot_40(value) == 0;
}

static const void* s_advisor_companion_transform;
static AdvisorCompanionApplyFn s_advisor_object_apply;
static AdvisorCompanionApplyFn s_advisor_scene_apply;

void advisor_companion_apply_hooks_set(const void* transform,
                                       AdvisorCompanionApplyFn object_apply,
                                       AdvisorCompanionApplyFn scene_apply)
{
    s_advisor_companion_transform = transform;
    s_advisor_object_apply = object_apply;
    s_advisor_scene_apply = scene_apply;
}

// FUNCTION: LEGORR 0x004734c0
void advisor_companion_transform_apply(void* unused, void** object_reference)
{
    void* object;
    (void)unused;
    if (object_reference == NULL) {
        return;
    }
    object = *object_reference;
    if (s_advisor_object_apply != NULL) {
        s_advisor_object_apply(object, s_advisor_companion_transform,
                               &object_reference);
    }
    if (s_advisor_scene_apply != NULL) {
        s_advisor_scene_apply(object, s_advisor_companion_transform,
                              &object_reference);
    }
}

static void* s_scene_value_query_context;
static SceneObjectReadyQueryFn s_scene_ready_query;
static SceneObjectCurrentValueFn s_scene_current_value_query;
static SceneObjectValueReleaseFn s_scene_value_release;

void scene_object_value_query_hooks_set(
    void* context, SceneObjectReadyQueryFn ready_query,
    SceneObjectCurrentValueFn value_query, SceneObjectValueReleaseFn release)
{
    s_scene_value_query_context = context;
    s_scene_ready_query = ready_query;
    s_scene_current_value_query = value_query;
    s_scene_value_release = release;
}

// FUNCTION: LEGORR 0x00476f70
uint32_t scene_object_query_value_if_ready(void* object)
{
    void* ready_interface;
    void* owner;
    void* object_local;
    uint32_t value = 0u;
    const uint32_t* current;
    if (object == NULL) {
        return 0u;
    }
    ready_interface = *(void**)((char*)object + 0x10);
    if (s_scene_ready_query == NULL ||
        s_scene_ready_query(ready_interface) != 0) {
        return 0u;
    }
    if (s_scene_current_value_query != NULL) {
        current = s_scene_current_value_query(s_scene_value_query_context);
        if (current != NULL) {
            value = *current;
        }
    }
    owner = *(void**)((char*)object + 8);
    object_local = object;
    if (s_scene_value_release != NULL) {
        s_scene_value_release(owner, &object_local);
    }
    return value;
}

// FUNCTION: LEGORR 0x00473dc0
void renderer_fog_enable_and_color_apply(int enabled)
{
    SceneRootInterface* const scene =
        (SceneRootInterface*)*(void**)s_scene_root;
    scene->fog_enable(enabled);
    scene->fog_color_enable(enabled != 0 ? s_scene_fog_color : 0u);
}

// FUNCTION: LEGORR 0x00473e70
void renderer_fog_mode_set(unsigned int mode)
{
    SceneRootInterface* const scene =
        (SceneRootInterface*)*(void**)s_scene_root;
    scene->fog_mode_set(mode);
    scene->fog_render_mode_set(s_scene_fog_mode);
}

// FUNCTION: LEGORR 0x00473e10
void renderer_fog_color_set(float red, float green, float blue)
{
    (void)green; /* reference code converts the first argument twice */
    const unsigned int first = (unsigned int)float_to_long(red * 255.0f);
    const unsigned int second = (unsigned int)float_to_long(red * 255.0f);
    const unsigned int third = (unsigned int)float_to_long(blue * 255.0f);
    const unsigned int packed =
        ((first | ((second | 0xffffff00u) << 8)) << 8) | third;
    SceneRootInterface* const scene =
        (SceneRootInterface*)*(void**)s_scene_root;
    s_scene_fog_color = packed;
    scene->fog_color_set(packed);
}

// FUNCTION: LEGORR 0x00473ea0
void renderer_fog_parameters_set(unsigned int start, unsigned int end,
                                 unsigned int density)
{
    SceneRootInterface* const scene =
        (SceneRootInterface*)*(void**)s_scene_root;
    scene->fog_parameters_set(start, end, density);
}

// FUNCTION: LEGORR 0x00476fb0
void renderer_object_set_clip_distance(void* object, float distance)
{
    SceneRootInterface* const scene =
        (SceneRootInterface*)*(void**)((char*)object + 0x10);
    scene->clip_distance_set(distance);
}

// FUNCTION: LEGORR 0x004772b0
void render_interface_transform_point(void* object, void* output,
                                      const void* input)
{
    SceneRootInterface* const scene =
        (SceneRootInterface*)*(void**)((char*)object + 0x10);
    scene->transform_point(output, input);
}

// FUNCTION: LEGORR 0x00476f10
void render_surface_get_value_pair_80_and_84(void* surface,
                                             uint32_t* out_80,
                                             uint32_t* out_84)
{
    if (out_80 != NULL) {
        void* const scene = *(void**)((char*)surface + 0x10);
        *out_80 = ((SceneRootInterface*)scene)
            ->render_surface_value_pair_80_get();
    }
    if (out_84 != NULL) {
        void* const scene = *(void**)((char*)surface + 0x10);
        *out_84 = ((SceneRootInterface*)scene)
            ->render_surface_value_pair_84_get();
    }
}

/* SceneNodePositionSet (FUN_00410020) — see view.hpp. The inner object
 * rides the node's widened slot at +0; the corpus strides a dword. */
// FUNCTION: LEGORR 0x00410020
void scene_node_position_set(void* node, const float pos[3])
{
    if (node == NULL || pos == NULL) {
        return;
    }
    void* inner = NULL;
    const uint32_t valid = 1u;
    __builtin_memcpy((char*)node + 0x0c, &valid, sizeof valid);
    __builtin_memcpy(&inner, (const char*)node, sizeof inner);
    if (inner == NULL) {
        return; /* deviation: the corpus strides the handle blindly */
    }
    __builtin_memcpy((char*)inner + 0x0c, &pos[0], sizeof pos[0]);
    __builtin_memcpy((char*)inner + 0x10, &pos[1], sizeof pos[1]);
    __builtin_memcpy((char*)inner + 0x14, &pos[2], sizeof pos[2]);
}

/* DDiSceneObjectArgumentsNormalize (FUN_00475900) — resolve the dispatch
 * pair: the object slot rides through, and the second slot falls back to
 * the root object's first slot when the caller passes NULL. */
// FUNCTION: LEGORR 0x00475900
void scene_object_arguments_normalize(const void** obj, const void** alt,
                                      void** out_obj, const void** out_alt)
{
    if (obj != NULL && out_obj != NULL) {
        *out_obj = (void*)*obj; /* corpus: blind dereference */
    }
    if (out_alt == NULL) {
        return;
    }
    if (alt != NULL) {
        *out_alt = *alt; /* corpus: blind dereference */
        return;
    }
    /* Deviation: a missing root answers NULL; the corpus strides it. */
    const void* root_slot = NULL;
    if (s_scene_root != NULL) {
        __builtin_memcpy((void*)&root_slot, s_scene_root, sizeof root_slot);
    }
    *out_alt = root_slot;
}

/* Matrix4IdentityInitialize (FUN_00483540): spell the float bit patterns as
 * dword stores so MSVC 5 /Od uses the corpus's shared zero/one registers.
 * Preserve its non-monotonic order: the twelve off-diagonal words are zeroed
 * high-to-low, then the diagonal words receive 1.0f's IEEE-754 pattern. */
// FUNCTION: LEGORR 0x00483540
void matrix4_identity_initialize(float* out)
{
    uint32_t* words = (uint32_t*)out;
    const uint32_t zero = 0u;
    words[7] = zero;
    words[6] = zero;
    words[4] = zero;
    words[3] = zero;
    words[2] = zero;
    words[1] = zero;
    words[14] = zero;
    words[13] = zero;
    words[12] = zero;
    words[11] = zero;
    words[9] = zero;
    words[8] = zero;
    const uint32_t one = 0x3f800000u;
    words[15] = one;
    words[10] = one;
    words[5] = one;
    words[0] = one;
}

/* SurfaceTriangleNormalCalculate (FUN_00479530): the two edges
 * e1 = p4 − p3 and e2 = p3 − p2; the three products recombine as the
 * cross product cross(e2, e1), so the normal is that cross normalized.
 * The corpus divides 1.0 by SQRT with no zero-length guard (finding 388
 * pinned); the port keeps the fold exactly. */
// FUNCTION: LEGORR 0x00479530
#if defined(_MSC_VER) && defined(_M_IX86)
/* FUN_00479530's self-contained x87 schedule is small enough to preserve
 * directly. It deliberately has no zero-area guard: the reciprocal of the
 * zero cross length propagates NaNs, as the corpus does. */
__declspec(naked) void surface_triangle_normal_calculate(
    float* out, const float* p2, const float* p3, const float* p4)
{
    __asm {
        sub esp, 0x18
        mov eax, dword ptr [esp + 0x28]
        mov ecx, dword ptr [esp + 0x20]
        fld dword ptr [eax]
        fld dword ptr [eax + 4]
        fld dword ptr [eax + 8]
        mov eax, dword ptr [esp + 0x24]
        fld dword ptr [eax]
        fxch st(3)
        fsub dword ptr [eax]
        fxch st(3)
        fsub dword ptr [ecx]
        fld dword ptr [eax + 4]
        fld dword ptr [eax + 8]
        fxch st(4)
        fsub dword ptr [eax + 4]
        fxch st(3)
        fsub dword ptr [eax + 8]
        fxch st(1)
        fsub dword ptr [ecx + 4]
        fxch st(4)
        fsub dword ptr [ecx + 8]
        fxch st(5)
        fst dword ptr [esp]
        fld dword ptr [esp]
        fxch st(4)
        fst dword ptr [esp + 4]
        fxch st(2)
        fst dword ptr [esp + 8]
        fld dword ptr [esp + 8]
        fxch st(4)
        fstp dword ptr [esp + 0x0c]
        mov eax, dword ptr [esp + 0x1c]
        fmul dword ptr [esp + 0x0c]
        fld dword ptr [esp + 4]
        fxch st(3)
        fmul dword ptr [esp + 0x0c]
        fxch st(6)
        fstp dword ptr [esp + 0x10]
        fxch st(4)
        fmul dword ptr [esp + 0x10]
        fxch st(6)
        fstp dword ptr [esp + 0x14]
        fmul dword ptr [esp + 0x14]
        fxch st(2)
        fmul dword ptr [esp + 0x10]
        fxch st(1)
        fmul dword ptr [esp + 0x14]
        fxch st(3)
        fsubp st(2), st
        fxch st(4)
        fsubp st(3), st
        fst dword ptr [esp + 0x24]
        fxch st(2)
        fst dword ptr [esp + 0x20]
        fxch st(1)
        fsubp st(3), st
        fxch st(1)
        fmul dword ptr [esp + 0x24]
        fxch st(1)
        fmul dword ptr [esp + 0x20]
        fxch st(2)
        fst dword ptr [esp + 0x28]
        fmul dword ptr [esp + 0x28]
        fxch st(2)
        faddp st(1), st
        fxch st(1)
        faddp st(1), st
        fsqrt
        fdivr dword ptr s_matrix_one
        fld st(0)
        fld st(1)
        fxch st(1)
        fmul dword ptr [esp + 0x28]
        fxch st(1)
        fmul dword ptr [esp + 0x24]
        fxch st(2)
        fmul dword ptr [esp + 0x20]
        fxch st(1)
        fstp dword ptr [eax]
        fxch st(1)
        fstp dword ptr [eax + 4]
        fstp dword ptr [eax + 8]
        add esp, 0x18
        ret
    }
}
#else
void surface_triangle_normal_calculate(float* out, const float* p2,
                                       const float* p3, const float* p4)
{
    const float e1[3] = {p4[0] - p3[0], p4[1] - p3[1], p4[2] - p3[2]};
    const float e2[3] = {p3[0] - p2[0], p3[1] - p2[1], p3[2] - p2[2]};
    const float cx = e2[1] * e1[2] - e2[2] * e1[1];
    const float cy = e2[2] * e1[0] - e2[0] * e1[2];
    const float cz = e2[0] * e1[1] - e2[1] * e1[0];
    const float scale = 1.0f / (float)sqrt(cz * cz + cy * cy + cx * cx);
    out[0] = scale * cx;
    out[1] = scale * cy;
    out[2] = scale * cz;
}
#endif

/* FUNCTION: LEGORR 0x00479630
 * Deterministic port-side area: the corpus applies a random angular
 * perturbation to the longest edge. This keeps the triangle geometry and
 * downward-facing gate while omitting that unknown random-angle schedule. */
float surface_triangle_area_calculate(const float* p1, const float* p2,
                                      const float* p3,
                                      int only_downward)
{
    float e1[3], e2[3], normal[3];
    float cx, cy, cz;
    if (p1 == NULL || p2 == NULL || p3 == NULL) return 0.0f;
    e1[0] = p2[0] - p1[0]; e1[1] = p2[1] - p1[1]; e1[2] = p2[2] - p1[2];
    e2[0] = p3[0] - p1[0]; e2[1] = p3[1] - p1[1]; e2[2] = p3[2] - p1[2];
    cx = e1[1] * e2[2] - e1[2] * e2[1];
    cy = e1[2] * e2[0] - e1[0] * e2[2];
    cz = e1[0] * e2[1] - e1[1] * e2[0];
    if (only_downward) {
        surface_triangle_normal_calculate(normal, p1, p2, p3);
        if (normal[2] >= 0.0f) return 0.0f;
    }
    return 0.5f * (float)sqrt(cx * cx + cy * cy + cz * cz);
}

/* SceneMeshColorAndAlphaSet (FUN_00475020) — see view.hpp. */
// FUNCTION: LEGORR 0x00475020
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void scene_mesh_color_and_alpha_set(void* object, int block,
                                                       float red, float green,
                                                       float blue, float alpha)
{
    __asm {
        /* These two checks preserve the port's test-pinned null behavior;
         * the corpus blindly follows both links. */
        mov eax, dword ptr [esp + 4]
        push esi
        test eax, eax
        je scene_null_return
        mov ecx, dword ptr [eax + 0x0c]
        test ecx, ecx
        je scene_null_return
        mov esi, dword ptr [ecx + 0x0c]
        test esi, esi
        je scene_null_return
        mov edx, dword ptr [esp + 0x18]
        mov eax, dword ptr [esp + 0x14]
        mov ecx, dword ptr [esp + 0x10]
        push edi
        mov edi, dword ptr [esp + 0x10]
        push 0
        push edx
        push eax
        push ecx
        push edi
        push esi
        call render_object_color_vector_set
        mov edx, dword ptr [esp + 0x38]
        add esp, 0x18
        push 4
        push edx
        push edi
        push esi
        call screen_effect_item_opacity_set
        add esp, 0x10
        pop edi
        pop esi
        ret
    scene_null_return:
        pop esi
        ret
    }
}
#else
void scene_mesh_color_and_alpha_set(void* object, int block, float red,
                                    float green, float blue, float alpha)
{
    if (object == NULL) {
        return; /* pinned deviation: the corpus strides the object blindly */
    }
    void* mesh = *(void**)((char*)object + 0xc);
    if (mesh == NULL) {
        return; /* pinned deviation: the corpus strides the mesh link blindly */
    }
    void* record = *(void**)((char*)mesh + 0xc);
    if (record == NULL) {
        return; /* corpus: the single gate, the record test at 0x47502b */
    }
    render_object_color_vector_set(record, block, red, green, blue, 0);
    screen_effect_item_opacity_set(record, block, alpha, 4);
}
#endif

/* SceneMeshColorSet (FUN_00474ff0) — see view.hpp. Same inner-record ride
 * as the alpha sibling; mode three lands the triple in the block's +0x34
 * slot through the ported color-vector setter. */
// FUNCTION: LEGORR 0x00474ff0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void scene_mesh_color_set(void* object, int block, float red,
                                             float green, float blue)
{
    __asm {
        /* These checks retain the port's test-pinned null-chain behavior. */
        mov eax, dword ptr [esp + 4]
        test eax, eax
        je scene_mesh_color_null_return
        mov ecx, dword ptr [eax + 0x0c]
        test ecx, ecx
        je scene_mesh_color_null_return
        mov eax, dword ptr [ecx + 0x0c]
        test eax, eax
        je scene_mesh_color_null_return

        /* Corpus call/stack schedule from FUN_00474ff0. */
        mov edx, dword ptr [esp + 0x14]
        mov ecx, dword ptr [esp + 0x10]
        push 3
        push edx
        mov edx, dword ptr [esp + 0x14]
        push ecx
        mov ecx, dword ptr [esp + 0x14]
        push edx
        push ecx
        push eax
        call render_object_color_vector_set
        add esp, 0x18
    scene_mesh_color_null_return:
        ret
    }
}
#else
void scene_mesh_color_set(void* object, int block, float red, float green,
                          float blue)
{
    void* mesh = NULL;
    void* record = NULL;
    if (object != NULL) {
        mesh = *(void* const*)((const char*)object + 0x0c);
    }
    if (mesh != NULL) {
        record = *(void* const*)((const char*)mesh + 0x0c);
    }
    if (record == NULL) {
        return; /* pinned deviation: the corpus strides both links blindly */
    }
    render_object_color_vector_set(record, block, red, green, blue, 3);
}
#endif

/* --- the rotated-pairs scratch (see view.hpp) ----------------------------- */

#define ROT_PAIR_SLOTS 256 /* deviation: bounded scratch (blind DAT stride) */
// GLOBAL: LEGORR 0x005013c8
static int s_rot_pairs[ROT_PAIR_SLOTS * 2];

void* rotated_pairs_scratch_peek(void) { return s_rot_pairs; }

// FUNCTION: LEGORR 0x00464110
void* rotated_pairs_scratch_fill(const int* offset_pair, const int* source,
                                 int count, int rotation)
{
    if (count > ROT_PAIR_SLOTS) {
        count = ROT_PAIR_SLOTS; /* deviation: the corpus fills blindly */
    }
    for (int n = 0; n < count; ++n) {
        const int t = rotation + 0x8000; /* the corpus's sign fold */
        const int u = t >> 31;
        const int sel = (((t ^ u) - u & 3) ^ u) - u;
        int x = source[2 * n];
        int y = source[2 * n + 1];
        switch (sel) {
        case 1: { const int h = x; x = -y; y = -h; } break;
        case 2:  x = -x; y = -y; break;
        case 3: { const int h = x; x = y; y = -h; } break;
        default: break; /* identity (negative folds land here) */
        }
        s_rot_pairs[2 * n]     = x + offset_pair[0];
        s_rot_pairs[2 * n + 1] = y + offset_pair[1];
    }
    return s_rot_pairs;
}

/* --- the curve band (2026-10-25; see view.hpp) ----------------------------- */

/* CubicBezierPointEvaluate (FUN_004065f0) — see view.hpp. Its portable
 * fallback keeps the corpus's polynomial term shapes. */
// FUNCTION: LEGORR 0x004065f0
#if defined(_MSC_VER) && defined(_M_IX86)
/* The five NULL checks preserve the port's tested guard behavior; after them
 * the x87 body follows FUN_004065f0 instruction-for-instruction. The repeated
 * dword operand is the corpus's .rdata 3.0f at 0x49e048 (bytes 00 00 40 40). */
__declspec(naked) void cubic_bezier_point_evaluate(float out[2],
                                                   const float* p0,
                                                   const float* p1,
                                                   const float* p2,
                                                   const float* p3, float t)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        test eax, eax
        je bezier_point_null_return
        mov eax, dword ptr [esp + 8]
        test eax, eax
        je bezier_point_null_return
        mov eax, dword ptr [esp + 0x0c]
        test eax, eax
        je bezier_point_null_return
        mov eax, dword ptr [esp + 0x10]
        test eax, eax
        je bezier_point_null_return
        mov eax, dword ptr [esp + 0x14]
        test eax, eax
        je bezier_point_null_return

        sub esp, 0x10
        mov edx, dword ptr [esp + 0x1c]
        mov eax, dword ptr [esp + 0x18]
        push esi
        mov esi, dword ptr [esp + 0x28]
        fld dword ptr [edx]
        fmul dword ptr ds:[0049e048h]
        fld dword ptr [esi]
        fmul dword ptr ds:[0049e048h]
        fxch st(1)
        fadd dword ptr [eax]
        fxch st(1)
        mov ecx, dword ptr [esp + 0x24]
        fsubr dword ptr [ecx]
        fld dword ptr [esp + 0x2c]
        fld st(2)
        fsub dword ptr [eax]
        fxch st(2)
        fsub st, st(3)
        fxch st(1)
        fmul dword ptr [esp + 0x2c]
        fxch st(2)
        fmul dword ptr ds:[0049e048h]
        fld dword ptr [ecx]
        fxch st(2)
        fmul dword ptr ds:[0049e048h]
        fxch st(2)
        fsub dword ptr [eax]
        fxch st(1)
        fstp dword ptr [esp + 0x20]
        fxch st(1)
        fsub dword ptr [esp + 0x20]
        fxch st(1)
        fsub dword ptr [esp + 0x20]
        fxch st(2)
        fst dword ptr [esp + 0x28]
        fmul dword ptr [esp + 0x2c]
        fxch st(1)
        fstp dword ptr [esp + 0x1c]
        fxch st(1)
        fsub dword ptr [esp + 0x1c]
        fld dword ptr [esp + 0x1c]
        fmul dword ptr [esp + 0x28]
        fxch st(2)
        fstp dword ptr [esp + 0x1c]
        fld dword ptr [esp + 0x20]
        fld dword ptr [edx + 4]
        fxch st(2)
        fmul dword ptr [esp + 0x1c]
        fld dword ptr [esi + 4]
        fxch st(2)
        fmul dword ptr [esp + 0x2c]
        fxch st(3)
        fmul dword ptr ds:[0049e048h]
        fxch st(2)
        fmul dword ptr ds:[0049e048h]
        fxch st(4)
        faddp st(1), st
        fxch st(1)
        fadd dword ptr [eax + 4]
        fxch st(2)
        faddp st(1), st
        fxch st(2)
        fsubr dword ptr [ecx + 4]
        fxch st(2)
        fadd dword ptr [eax]
        fxch st(1)
        fst dword ptr [esp + 8]
        fxch st(2)
        fst dword ptr [esp + 0x10]
        fsub dword ptr [esp + 8]
        fxch st(1)
        mov edx, dword ptr [esp + 0x18]
        pop esi
        fstp dword ptr [edx]
        fxch st(1)
        fsub dword ptr [eax + 4]
        fld dword ptr [ecx + 4]
        fxch st(2)
        fmul dword ptr ds:[0049e048h]
        fxch st(1)
        fmul dword ptr ds:[0049e048h]
        fxch st(2)
        fsub dword ptr [eax + 4]
        fxch st(2)
        fstp dword ptr [esp + 0x1c]
        fsub dword ptr [esp + 0x1c]
        fxch st(1)
        fsub dword ptr [esp + 0x1c]
        fld dword ptr [esp + 0x1c]
        fmul dword ptr [esp + 0x28]
        fxch st(2)
        fstp dword ptr [esp + 0x28]
        fsub dword ptr [esp + 0x28]
        fld dword ptr [esp + 0x28]
        fmul dword ptr [esp + 0x24]
        fxch st(1)
        fmul dword ptr [esp + 0x18]
        fxch st(2)
        faddp st(2), st
        faddp st(1), st
        fadd dword ptr [eax + 4]
        fstp dword ptr [edx + 4]
        fstp st(0)
        add esp, 0x10
        ret

    bezier_point_null_return:
        ret
    }
}
#else
void cubic_bezier_point_evaluate(float out[2], const float* p0, const float* p1,
                                 const float* p2, const float* p3, float t)
{
    if (out == NULL || p0 == NULL || p1 == NULL || p2 == NULL || p3 == NULL) {
        return; /* deviation: the corpus strides the points blindly */
    }
    const double tt = (double)t * (double)t; /* the x87 t-squared word */
    const double q = (double)p1[0] * 3.0 + (double)p0[0]; /* x: P1.x*3 + P0.x */
    const double s4 = (q - (double)p0[0]) * 3.0;
    const double s3 = (((double)p2[0] - (double)p3[0] * 3.0) - q) * 3.0 - s4;
    const double s1 = ((double)p2[0] - (double)p0[0]) - s4 - s3;
    const double r6 = (double)p1[1] * 3.0 + (double)p0[1];
    const double t4 = (r6 - (double)p0[1]) * 3.0;
    const double t3 = (((double)p2[1] - (double)p3[1] * 3.0) - r6) * 3.0 - t4;
    const double t1 = ((double)p2[1] - (double)p0[1]) - t4 - t3;

    double x = (double)s4 * (double)t;
    x += (double)s3 * tt;
    x += (double)s1 * tt * (double)t;
    x += (double)p0[0];
    const float fx = (float)x;
    double y = (double)t3 * tt;
    y += (double)t4 * (double)t;
    y += (double)t1 * tt * (double)t;
    y += (double)p0[1];
    const float fy = (float)y;

    __builtin_memcpy(&out[0], &fx, sizeof fx);
    __builtin_memcpy(&out[1], &fy, sizeof fy);
}
#endif

/* BezierCurveSamplesBuild (FUN_00406820) — see view.hpp. */
// FUNCTION: LEGORR 0x00406820
void bezier_curve_samples_build(uint32_t* dest, const uint32_t* control,
                                const uint32_t* p3, const uint32_t* p4,
                                const uint32_t* p5, uint32_t count)
{
    if (dest == NULL || control == NULL) {
        return; /* deviation: the corpus strides the record blindly */
    }
    /* `cmp ebx,0x32` / `jb` leaves counts below 50 untouched. */
    if (count >= 0x32u) {
        count = 0x32u; /* the corpus's overflow clamp, faithful */
    }
    uint32_t i = 1u;
    dest[0] = count;
    dest[1] = control[0];
    dest[2] = control[1];
    if (count <= i) {
        return;
    }
    const uint32_t denominator = count - 1u;
    uint32_t* slot = dest + 3;
    /* The corpus forms `lea edx,[ebx-1]`, advances the output, then converts
     * the denominator with `fild QWORD PTR` before dividing the 1.0 constant. */
    const float inv = s_curve_unit / (float)denominator;
    do {
        const float t = (float)i * inv;
        cubic_bezier_point_evaluate((float*)slot, (const float*)control,
                                    (const float*)p3, (const float*)p4,
                                    (const float*)p5, t);
        i = i + 1u;
        slot += 2;
    } while (i < count);
}

/* PathArcLengthInterpolate (FUN_004068c0) — see view.hpp. The record layout
 * is the curve band's: count at dword 0, point i (1-based) floats at dwords
 * 2i-1/2i, segment lengths at dword 0x65+k — the same window the length
 * builder stores through +0x198. */
// FUNCTION: LEGORR 0x004068c0
uint32_t path_arc_length_interpolate(const uint32_t* curve, float travel,
                                     float* out)
{
    if (curve == NULL || out == NULL) {
        return 0u; /* deviation: the corpus strides both blindly */
    }
    const uint32_t count = curve[0];
    float accumulated = 0.0f;
    uint32_t segment = 1u;
    if (count > 1u) {
        for (;;) {
            float len = 0.0f;
            __builtin_memcpy(&len, (const char*)curve + 4u * (0x65u + segment),
                             sizeof len);
            accumulated = accumulated + len;
            if (travel < accumulated) {
                break;
            }
            segment = segment + 1u;
            if (segment >= count) {
                break;
            }
        }
    }

    float sx = 0.0f, sy = 0.0f;
    __builtin_memcpy(&sx, (const char*)curve + 4u * (2u * segment - 1u),
                     sizeof sx);
    __builtin_memcpy(&sy, (const char*)curve + 4u * (2u * segment), sizeof sy);

    if (segment < count) {
        float len = 0.0f;
        __builtin_memcpy(&len, (const char*)curve + 4u * (0x65u + segment),
                         sizeof len);
        const float frac =
            (travel - (accumulated - len)) / len; /* corpus ratio shape */
        float ax = sx, ay = sy, bx = 0.0f, by = 0.0f;
        __builtin_memcpy(&bx, (const char*)curve + 4u * (2u * segment + 1u),
                         sizeof bx);
        __builtin_memcpy(&by, (const char*)curve + 4u * (2u * segment + 2u),
                         sizeof by);
        out[0] = (bx - ax) * frac + ax;
        out[1] = (by - ay) * frac + ay;
    } else {
        out[0] = sx;
        out[1] = sy;
    }
    return segment;
}

/* ViewPointToScreenGeometryTransform (FUN_0045e940) — see view.hpp. The
 * record window is the projection block: center +4/+8, half-size +c/+10,
 * scale pair +14/+18, screen offsets +1c/+20. Deviations: NULL guards on
 * both inputs (the corpus dereferences blindly). The scale ratio is
 * recomputed at each output as in the corpus. */
// FUNCTION: LEGORR 0x0045e940
void view_point_to_screen_geometry_transform(const void* view, float p[4])
{
    if (view == NULL || p == NULL) {
        return; /* deviation: the corpus strides both blindly */
    }
    const char* w = (const char*)view;
    float cx = 0.0f, cy = 0.0f, half_x = 0.0f, half_y = 0.0f;
    float scale_a = 0.0f, scale_b = 0.0f, off_x = 0.0f, off_y = 0.0f;
    __builtin_memcpy(&cx, w + 0x04, sizeof cx);
    __builtin_memcpy(&cy, w + 0x08, sizeof cy);
    __builtin_memcpy(&half_x, w + 0x0c, sizeof half_x);
    __builtin_memcpy(&half_y, w + 0x10, sizeof half_y);
    __builtin_memcpy(&scale_a, w + 0x14, sizeof scale_a);
    __builtin_memcpy(&scale_b, w + 0x18, sizeof scale_b);
    __builtin_memcpy(&off_x, w + 0x1c, sizeof off_x);
    __builtin_memcpy(&off_y, w + 0x20, sizeof off_y);

    p[0] = p[0] - off_x;
    p[1] = p[1] - off_y;
    p[0] = (scale_a / scale_b) * p[0];
    p[1] = (scale_a / scale_b) * p[1];
    p[2] = (scale_a / scale_b) * p[2];
    p[0] = p[0] + half_x * 0.5f + cx;
    p[1] = (half_y * 0.5f + cy) - p[1];
    p[3] = (scale_a / scale_b) * p[3];
}

/* PolylineSegmentLengthsAndTotalCalculate (FUN_004067b0) — see view.hpp. */
// FUNCTION: LEGORR 0x004067b0
float polyline_segment_lengths_total(uint32_t* curve)
{
    if (curve == NULL) {
        return 0.0f; /* deviation: the corpus strides the record blindly */
    }
    const uint32_t count = curve[0];
    float zero = 0.0f;
    __builtin_memcpy((char*)curve + 0x194, &zero, sizeof zero);
    double total = 0.0;
    if (count > 1u) {
        for (uint32_t i = 1u; i < count; ++i) {
            /* Corpus point pairs ride adjacent floats: x at +4+8i, y at
             * +8+8i (the walker steps two dwords a hop). */
            const size_t base = 4u + (size_t)(i - 1u) * 8u;
            float ax = 0.0f, ay = 0.0f, bx = 0.0f, by = 0.0f;
            __builtin_memcpy(&ax, (char*)curve + base, sizeof ax);
            __builtin_memcpy(&ay, (char*)curve + base + 4, sizeof ay);
            __builtin_memcpy(&bx, (char*)curve + base + 8, sizeof bx);
            __builtin_memcpy(&by, (char*)curve + base + 12, sizeof by);
            const float a[2] = { ax, ay };
            const float b[2] = { bx, by };
            const float len = vec2_dist_alt(a, b);
            __builtin_memcpy((char*)curve + 0x198 + (size_t)(i - 1) * 4u,
                             &len, sizeof len);
            total += (double)len;
        }
    }
    return (float)total;
}

// FUNCTION: LEGORR 0x00408b60
void point_project_onto_line2d(const float* line_start, const float* line_end,
                               const float* point, float* out)
{
    float dx = line_end[0] - line_start[0];
    float dy = line_end[1] - line_start[1];
    float inverse_length = 1.0f / sqrt(dy * dy + dx * dx);
    float unit_x = inverse_length * dx;
    float unit_y = inverse_length * dy;
    float projection = (point[1] - line_start[1]) * unit_y +
                       (point[0] - line_start[0]) * unit_x;
    out[0] = projection * unit_x + line_start[0];
    out[1] = projection * unit_y + line_start[1];
}

// FUNCTION: LEGORR 0x00408bf0
void point2d_project_onto_direction(const float* origin, const float* direction,
                                    const float* point, float* out)
{
    float direction_x = direction[0];
    float direction_y = direction[1];
    float inverse_length = 1.0f / sqrt(direction_y * direction_y +
                                       direction_x * direction_x);
    float unit_x = inverse_length * direction_x;
    float unit_y = inverse_length * direction_y;
    float projection = (point[1] - origin[1]) * unit_y +
                       (point[0] - origin[0]) * unit_x;
    out[0] = projection * unit_x + origin[0];
    out[1] = projection * unit_y + origin[1];
}

/* The callback is vtable slot 7 (byte offset 0x1c on x86). Express it as a
 * virtual method so older MSVC compilers use their implicit thiscall ABI. */
class ScenePropertyValueInterface
{
public:
    virtual void* slot_0(void) = 0;
    virtual void* slot_1(void) = 0;
    virtual void* slot_2(void) = 0;
    virtual void* slot_3(void) = 0;
    virtual void* slot_4(void) = 0;
    virtual void* slot_5(void) = 0;
    virtual void* value_record_set(void* value) = 0;
    virtual void* value_record_get(void) = 0;
};

static void* scene_property_value_record_get(void* property)
{
    void** vtable = NULL;
    if (property == NULL) {
        return NULL;
    }
    __builtin_memcpy(&vtable, property, sizeof vtable);
    if (vtable == NULL) {
        return NULL;
    }
    if (vtable[7] == NULL) {
        return NULL;
    }
    return ((ScenePropertyValueInterface*)property)->value_record_get();
}

/* AttachedObjectDataReleaseAndClear (FUN_004760e0): release the optional
 * record-owned buffer and record, then clear the interface's value slot. */
// FUNCTION: LEGORR 0x004760e0
void attached_object_data_release_and_clear(void* property)
{
    void* record = scene_property_value_record_get(property);
    void* buffer = NULL;
    if (record != NULL) {
        memcpy(&buffer, (char*)record + 4, sizeof(buffer));
        if (buffer != NULL) engine_heap_free(buffer);
        engine_heap_free(record);
    }
    if (property != NULL) {
        ((ScenePropertyValueInterface*)property)->value_record_set(NULL);
    }
}

/* CallbackRecordSetFlag10AndRefresh (FUN_00475e10): set the record's flag
 * before invalidating its cached property payload. */
void callback_record_set_flag10_and_refresh(void* property)
{
    void* record = scene_property_value_record_get(property);
    if (record == NULL) return;
    *(uint32_t*)((char*)record + 0x14) |= 0x10u;
    attached_object_data_release_and_clear(property);
}

/* ScenePropertyFloatAt14Get (FUN_004761c0): read the float at +0x14 in the
 * property record returned by the interface's slot +0x1c. */
float scene_property_float_at14_get(void* property)
{
    void* record = scene_property_value_record_get(property);
    return record != NULL ? *(float*)((char*)record + 0x14) : 0.0f;
}

/* ScenePropertyPrimaryValueGet (FUN_00476120): first dword in the value
 * record returned by the object's virtual callback. */
// FUNCTION: LEGORR 0x00476120
uint32_t scene_property_primary_value_get(void* property)
{
    void** vtable = *(void***)property;
    typedef void* (*ScenePropertyValueRecordGetFn)(void*);
    void* record = ((ScenePropertyValueRecordGetFn)vtable[7])(property);
    if (record != NULL) {
        return *(uint32_t*)record;
    }
    return 0;
}

/* ScenePropertySecondaryValueGet (FUN_00476140): second dword in the same
 * callback result. */
// FUNCTION: LEGORR 0x00476140
uint32_t scene_property_secondary_value_get(void* property)
{
    void** vtable = *(void***)property;
    typedef void* (*ScenePropertyValueRecordGetFn)(void*);
    void* record = ((ScenePropertyValueRecordGetFn)vtable[7])(property);
    if (record != NULL) {
        return *(uint32_t*)((char*)record + 4);
    }
    return 0;
}

/* Row-span staging records from FUN_0046eb30. The corpus has a 0x320-byte
 * count slot and an adjacent x/y float-pair window; dimensions inferred
 * from that window bound the port's rough model to 96 points per row and
 * 64 staged rows. */
#define MAP_ROW_SPAN_RECORDS_MAX 64
#define MAP_ROW_SPAN_POINTS_MAX 96
#define MAP_ROW_SPAN_COUNT_OFFSET 0x320
#define MAP_ROW_SPAN_RECORD_SIZE 0x324
static unsigned char s_map_row_span_records[MAP_ROW_SPAN_RECORDS_MAX]
                                           [MAP_ROW_SPAN_RECORD_SIZE];
static uint32_t s_map_row_span_records_count;

/* AppendMapRowSpanToBuffer (FUN_0046ead0): append each x in the half-open
 * interval [x_begin, x_end), pairing it with y. */
// FUNCTION: LEGORR 0x0046ead0
void map_row_span_append(void* buffer, uint32_t y, uint32_t x_begin,
                         uint32_t x_end)
{
    if (buffer == NULL) {
        return;
    }
    uint32_t count = 0u;
    __builtin_memcpy(&count, (char*)buffer + MAP_ROW_SPAN_COUNT_OFFSET,
                     sizeof(count));
    while (x_begin < x_end && count < MAP_ROW_SPAN_POINTS_MAX) {
        const float x = (float)x_begin;
        const float fy = (float)y;
        __builtin_memcpy((char*)buffer + (size_t)count * 8u, &x, sizeof(x));
        __builtin_memcpy((char*)buffer + (size_t)count * 8u + 4u,
                         &fy, sizeof(fy));
        ++x_begin;
        ++count;
    }
    __builtin_memcpy((char*)buffer + MAP_ROW_SPAN_COUNT_OFFSET, &count,
                     sizeof(count));
}

/* StartMapRowSpanBufferRecord (FUN_0046eb30): clear a new row record,
 * append the supplied span, then advance the port-owned staging count. */
// FUNCTION: LEGORR 0x0046eb30
void map_row_span_record_start(uint32_t y, uint32_t x_begin,
                               uint32_t x_end)
{
    if (s_map_row_span_records_count >= MAP_ROW_SPAN_RECORDS_MAX) {
        return;
    }
    unsigned char* record =
        s_map_row_span_records[s_map_row_span_records_count];
    memset(record, 0, MAP_ROW_SPAN_RECORD_SIZE);
    map_row_span_append(record, y, x_begin, x_end);
    ++s_map_row_span_records_count;
}

uint32_t map_row_span_record_count(void)
{
    return s_map_row_span_records_count;
}

const void* map_row_span_record_get(uint32_t index)
{
    if (index >= s_map_row_span_records_count) {
        return NULL;
    }
    return s_map_row_span_records[index];
}

/* FloatRoundToNearestInt (FUN_00463e40): the caller uses the returned
 * integer as a row index. The corpus compares the fractional part against
 * 0.5, choosing floor below it and ceil at/above it (ties toward +infinity). */
// FUNCTION: LEGORR 0x00463e40
int float_round_to_nearest_int(float value)
{
    const double lower = floor((double)value);
    const double rounded = ((double)value - lower < 0.5)
        ? lower : ceil((double)value);
    return crt_float_to_long((float)rounded);
}

static RendererSubobjectResolveFn s_renderer_subobject_resolver;

void renderer_subobject_resolver_set(RendererSubobjectResolveFn resolver)
{
    s_renderer_subobject_resolver = resolver;
}

static void* scene_object_subobject_resolve(void* owner, void* source,
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

/* ActivityRunnerStateRead (FUN_00406f80): lazily resolve and cache the
 * selected subobject from the runner's +0x20 source into its +0x54 table. */
// FUNCTION: LEGORR 0x00406f80
void* activity_runner_state_read(void* runner, int index)
{
    void* source = NULL;
    void* cached = NULL;
    size_t slot_offset;

    if (runner == NULL) return NULL;
    __builtin_memcpy(&source, (char*)runner + 0x20u, sizeof(source));
    if (source == NULL) return NULL;
    slot_offset = 0x54u + (size_t)index * sizeof(void*);
    __builtin_memcpy(&cached, (char*)runner + slot_offset, sizeof(cached));
    if (cached == NULL && s_renderer_subobject_resolver != NULL) {
        cached = s_renderer_subobject_resolver(runner, source, index);
        __builtin_memcpy((char*)runner + slot_offset, &cached,
                         sizeof(cached));
    }
    __builtin_memcpy(&cached, (char*)runner + slot_offset, sizeof(cached));
    return cached;
}

/* SceneActivityChangeSecondary (FUN_004085f0): resolve the requested indexed
 * child from +0x0c and cache it in the +0x40 handle table. */
// FUNCTION: LEGORR 0x004085f0
void* scene_activity_change_secondary(void* activity, int index)
{
    void** cached;
    void* result = NULL;
    if (activity == NULL) return NULL;
    if (*(void**)((char*)activity + 0x0c) != NULL) {
        cached = (void**)((char*)activity + 0x40) + index;
        if (*cached == NULL) {
            *cached = scene_object_subobject_resolve(
                activity, *(void**)((char*)activity + 0x0c), index);
        }
        result = *cached;
    }
    return result;
}

/* FUNCTION: LEGORR 0x00406fb0 */
void* renderer_cached_handle_40_get_or_create(void* renderer)
{
    return scene_object_cached_subobject_get(renderer, 0x2cu, 0x40u, 0);
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

static uint32_t s_global_vector_values[4];
static GlobalVectorNotifyFn s_global_vector_notify;

void global_vector_notify_callback_set(GlobalVectorNotifyFn callback)
{
    s_global_vector_notify = callback;
}

void global_vector_get(uint32_t values[4])
{
    if (values != NULL) {
        __builtin_memcpy(values, s_global_vector_values,
                         sizeof(s_global_vector_values));
    }
}

/* FUNCTION: LEGORR 0x0040ffe0. The corpus stores four global dwords and
 * broadcasts the new tuple to a callback walk rooted at context. */
void global_vector_set_and_notify(void* context, const uint32_t values[4])
{
    if (values == NULL) return;
    __builtin_memcpy(s_global_vector_values, values,
                     sizeof(s_global_vector_values));
    if (s_global_vector_notify != NULL) {
        s_global_vector_notify(context, s_global_vector_values);
    }
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

static GridSurfaceLookupFn s_grid_surface_lookup;
static GridSurfaceHandleReleaseFn s_grid_surface_release;
static void* s_grid_surface_user;

void grid_surface_hooks_set(GridSurfaceLookupFn lookup,
                            GridSurfaceHandleReleaseFn release, void* user)
{
    s_grid_surface_lookup = lookup;
    s_grid_surface_release = release;
    s_grid_surface_user = user;
}

/* FUNCTION: LEGORR 0x0046e330 */
void grid_surface_entry_reset(void* surface, int key)
{
    if (surface == NULL || s_grid_surface_lookup == NULL) return;
    int index = key;
    void* entry = s_grid_surface_lookup(surface, key, &index,
                                        s_grid_surface_user);
    if (entry == NULL || index < 0 || index > 4096) return;
    uint32_t flags = 0;
    uint32_t handle = 0u;
    __builtin_memcpy(&flags, (char*)entry + 0x424u, sizeof(flags));
    flags = (flags & ~2u) | 1u;
    __builtin_memcpy((char*)entry + 0x424u, &flags, sizeof(flags));
    __builtin_memcpy(&handle, (char*)entry + 0x420u, sizeof(handle));
    if (s_grid_surface_release != NULL) {
        s_grid_surface_release(handle, s_grid_surface_user);
    }
    uint32_t zero = 0;
    __builtin_memcpy((char*)entry + 0x338u + (size_t)index * 0x18u,
                     &zero, sizeof(zero));
}

static WorldSpatialRecordModel* s_world_spatial_records;
static uint32_t s_world_spatial_record_count;

void world_spatial_records_install(WorldSpatialRecordModel* records,
                                   uint32_t count)
{
    s_world_spatial_records = records;
    s_world_spatial_record_count = count > 65536u ? 65536u : count;
}

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
