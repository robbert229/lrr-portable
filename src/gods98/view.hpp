// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — view context tick (corpus ViewContextTick @ 0x435f5c,
 * finding 04 §ViewContextTick). Ticked three times per frame (the three view
 * objects DAT_005560e4 / e8 / ec; finding 03).
 *
 * Modes: 1 = follow (edge-locked camera), 2 = scripted/lagged
 * (k = |d|/(|d|+A) interpolation), 3 = free/cruise (velocity-EMA, dirty flag
 * bit 0x10). Requested by the sim-fills tranche 2.
 *
 * Portable CameraHandle records hold view-tick position/facing state. They
 * are separate from the DDiScene interface targets at 0x475430–0x4755e0,
 * which normalize a scene object and dispatch through its vtable. The Win32
 * primary owns the composed RM eye and the last displayed projection.
 * World queries use the level-owned terrain. Entity target pose
 * resolution is owned by the game entity module and mapped at FUN_0044a070.
 */
#ifndef LEGORRR_VIEW_HPP
#define LEGORRR_VIEW_HPP

#include "engine.hpp"

/* A camera handle: the corpus's camera objects hold a position + facing. */
typedef struct CameraHandle {
    int   inuse;
    float pos[3];
    float rot[3];
} CameraHandle;

/* World-space camera frame and the projection of the last displayed scene.
 * These are host sidecars, never a retail camera/controller record. */
typedef struct ViewCameraPose {
    float eye[3];
    float forward[3];
    float up[3];
} ViewCameraPose;
typedef struct ViewCameraProjection {
    ViewCameraPose pose;
    const void* level;
    int width, height;
    float focal_x, focal_y, center_x, center_y;
    unsigned int generation;
} ViewCameraProjection;

/* View object — the corpus's flat view array read as named fields. */
typedef struct ViewContext {
    int    mode;        /* [0]  1 follow / 2 scripted-lag / 3 free */
    int    follow_id;   /* [1]  followed/velocity state target */
    int    a2;          /* [2] */
    int    a3;          /* [3] */
    int    a4;          /* [4] */
    int    a5;          /* [5] */
    float  offset[3];   /* [6..8] follow offsets */
    CameraHandle cam_world;  /* [9]   main camera */
    CameraHandle cam_follow; /* [10]  follow camera */
    CameraHandle cam_facing; /* [0xb] facing camera */
    int    cam_mode;   /* [0xc] */
    float  follow_span;/* [0x24] */
    float  pan_accum;  /* +0x9c pointer-pan accumulator (CameraPointerPanUpdate) */
    float  _gap_a5;    /* corpus dword gap after the accumulator slot */
    float  _gap_a9;    /* ditto */
    float  _gap_ad;    /* ditto */
    float  step;       /* [0x28] */
    int    timer_num;  /* [0x29] */
    int    timer_acc;  /* [0x2a] */
    float  velocity[3];/* [0x2b..2d] */
    uint8_t flags;     /* [0x2e] bit 0x10 = dirty */
    void*  target_record; /* host-owned retail target-record sidecar */
    /* Primary kind-1 ownership: the existing bounded-controller buffer,
     * requested world X/Y (0x556150/154), and its 0x556158 tween latch. */
    void* controller;
    float target_xy[2];
    int target_pending;
    int target_tween;
    void* scene_follow;
    void* scene_tilt;
    void* scene_eye;
    ViewCameraPose pose;
    int pose_valid;
    ViewCameraProjection displayed;
} ViewContext;

void view_primary_target_request(EngineContext* ctx, float x, float y,
                                 int tween);
float view_camera_delta_from_seconds(float seconds);
void view_primary_target_tick(EngineContext* ctx, float reference_delta);
/* Called once after each real level load, following the NERP reset. */
void view_primary_level_release(EngineContext* ctx);
int view_primary_level_initialize(EngineContext* ctx, float x, float y,
                                   float heading);
/* A replacement image invalidates picking without cancelling camera motion. */
void view_primary_projection_invalidate(ViewContext* view);
int view_primary_projection_publish(ViewContext* view, const void* level,
                                     int width, int height);
int view_primary_projection_get(const ViewContext* view, const void* level,
                                 int width, int height,
                                 ViewCameraProjection* projection);

/* Per-frame camera update: delta/scale are reference 25-Hz frame units. */
void view_tick(ViewContext* v, EngineContext* ctx, float dt, float scale);
/* viewport_rectangle_set, ViewportDimensionsQueryFn and
 * viewport_dimensions_query_hook_set have no definition since the 2D unit
 * (G98-019) was rebuilt in draw.c. They stay declared only to hold the
 * declaration count (finding 506): removing them moves
 * lava_erosion_delay_from_cell (0040e9c0) from 100% to 84.21%. */
void viewport_rectangle_set(float left, float top, float right, float bottom);
#ifdef __cplusplus
extern "C"
#endif
void viewport_rectangle_get(struct Area2F* window); /* FUN_00485fb0, draw.h */
typedef int (*ViewportDimensionsQueryFn)(void* context, float* width,
                                         float* height);
void viewport_dimensions_query_hook_set(ViewportDimensionsQueryFn query,
                                        void* context);
#ifdef __cplusplus
extern "C"
#endif
void viewport_subrectangle_set(const struct Area2F* window); /* FUN_00485ea0, draw.h */
void model_transform_copy_to_scene_node(void* model_record,
                                        void* source_scene_node);
typedef void (*CameraViewContextReleaseHook)(void* context, void* view,
                                             unsigned int field_index);
typedef void (*CameraViewContextFreeHook)(void* context, void* view);
void camera_view_context_close_hooks_set(CameraViewContextReleaseHook release,
                                         CameraViewContextFreeHook free_view,
                                         void* context);
#ifdef __cplusplus
extern "C"
#endif
void camera_view_context_close(void* view); /* FUN_00435dc8 */
typedef void (*AnimationObjectTransformApplyHook)(void* context,
                                                  void* render_object,
                                                  float first, float second,
                                                  const float orientation[4]);
typedef float (*RenderNodePositionValueFn)(float x, float y, void* context);
typedef void (*RenderNodePositionApplyHook)(void* context,
                                           void* render_object,
                                           float x, float y, float z);
void animation_transform_hooks_set(AnimationObjectTransformApplyHook orient,
                                   RenderNodePositionApplyHook position,
                                   void* context);
/* Superseded by game/animation_object.c (GAME-004, finding 549); kept
 * uncalled so that the symbol count of every includer is unchanged (ADR 0005
 * entropy rule 1). */
void animation_object_transform_set(void* record, float first, float second);
void render_node_position_from_callback(void* record, float x, float y,
                                        RenderNodePositionValueFn value_fn,
                                        void* context);
typedef void (*SceneMatrixApplyHook)(void* context, void* target,
                                     const float matrix[16]);
void scene_matrix_apply_hook_set(SceneMatrixApplyHook hook, void* context);
#ifdef __cplusplus
extern "C"
#endif
float animation_track_find_keyframe_and_fraction(const void* track,
                                                 float time,
                                                 uint16_t* keyframe_out); /* 0x00487cb0 */
void animation_track_keys_interpolate(void* owner, const void* track,
                                      uint32_t keyframe, float fraction); /* 0x00488170 */
void scene_node_transform_matrix_apply(void* owner, const void* track,
                                       const float position[3],
                                       const float rotation[3],
                                       const float scale[3]); /* 0x004882e0 */
void matrix_apply_rotation_about_z_with_offset(void* target,
                                               float matrix[16],
                                               float offset); /* 0x0044b7a0 */
typedef struct SceneFrameApplyHooks {
    void* context;
    void (*orientation_set)(void* context, void* object, unsigned int frame,
                            const float normal[3], const float forward[3]);
    void (*position_set)(void* context, void* object, unsigned int frame,
                         const float position[3]);
    void (*point_transform)(void* context, void* object, float out[3],
                            const float point[3]);
    void (*projection_set)(void* context, void* object, unsigned int frame,
                           const float matrix[16]);
} SceneFrameApplyHooks;
void scene_frame_apply_hooks_set(const SceneFrameApplyHooks* hooks);
void scene_frame_from_triplet_apply(void* object, float projection_scale,
                                   const float first[3],
                                   const float second[3],
                                   const float third[3]); /* 0x0040bb90 */
typedef struct SceneCellVisualLookupTable {
    uint32_t stride;
    uint32_t reserved;
    uint32_t* entries;
} SceneCellVisualLookupTable;
typedef struct SceneCellVisualRelayRecord {
    void* resource_roots[2];
    void* channel_resources[2][100];
    float projection_scale;
    uint32_t channel_cursor;
    uint32_t reserved[3];
    SceneCellVisualLookupTable* lookup_table;
} SceneCellVisualRelayRecord;
void scene_cell_visual_channels_apply(SceneCellVisualRelayRecord* record,
                                      const float corner_points[12],
                                      uint32_t lookup_selector,
                                      uint32_t corner_index, float scale,
                                      float uniform_scale); /* 0x0040b880 */
typedef struct ViewportTargetClampState {
    int map_width;
    int map_height;
    int edge_extent;
    int target[2];
    uint32_t global_flags;
} ViewportTargetClampState;
int viewport_target_position_clamp(ViewportTargetClampState* state,
                                   const int position[2]); /* 0x0042de40 */
typedef struct RenderQualitySettings {
    float fp_clip_blocks;
    const char* quality;
    int dither;
    int filter;
    int blend;
    int alpha;
    int mipmap;
    int linear_mipmap;
} RenderQualitySettings;
typedef void (*RenderQualityApplyHook)(void* context, int quality,
                                       int dither, int filter, int mipmap,
                                       int linear_mipmap, int blend,
                                       int alpha);
void render_quality_apply_hook_set(RenderQualityApplyHook hook,
                                   void* context);
int render_quality_config_apply(const RenderQualitySettings* settings); /* 0x004334f0 */
typedef struct SelectionRectangleRenderState {
    uint32_t flags;
    float mouse_x;
    float mouse_y;
    uint32_t color[3];
} SelectionRectangleRenderState;
typedef struct SelectionRectangleRenderHooks {
    void* context;
    int (*anchor_get)(void* context, float point[2]);
    void (*polyline_draw)(void* context, const float points[10],
                          unsigned int count, const uint32_t color[3],
                          int mode);
} SelectionRectangleRenderHooks;
void selection_rectangle_render(const SelectionRectangleRenderState* state,
                                const SelectionRectangleRenderHooks* hooks); /* 0x004293b0 */
typedef struct CameraPointerRayHooks {
    void* context;
    int (*screen_point_to_ray)(void* context, float x, float y, float depth,
                               float point[3]);
    int (*plane_intersect)(void* context, const float point[3],
                           const float direction[3],
                           const float plane_point[3],
                           const float plane_normal[3], float intersection[3]);
} CameraPointerRayHooks;
int camera_pointer_ray_route(const CameraPointerRayHooks* hooks,
                             float x, float y, float* out_x, float* out_y); /* 0x00429470 */
typedef struct RenderViewportState {
    void* view_object;
    void* device;
    uint32_t flags;
    float fallback_position[2];
    float display_value;
} RenderViewportState;
typedef struct RenderViewportHooks {
    void* context;
    int (*view_position_get)(void* context, void* view_object,
                              float position[2]);
    void (*display_state_set)(void* context, void* device, float value);
    void (*position_set)(void* context, void* device,
                         const float position[2]);
} RenderViewportHooks;
void render_viewport_state_update(RenderViewportState* state,
                                  const RenderViewportHooks* hooks); /* 0x00426250 */
typedef struct GlobalRenderRecordHooks {
    void* context;
    float (*value_get)(void* context, void* record, float minimum,
                       float increment);
    void (*record_set)(void* context, void* view, void* record,
                       unsigned int mode, float value);
} GlobalRenderRecordHooks;
void global_render_record_set_from_input(void* record, void* view,
                                         const GlobalRenderRecordHooks* hooks); /* 0x00425d50 */
typedef struct EngineTaskCallbackRecord {
    /* Retail record is 0x34 bytes. Pointer-bearing words retain their
     * x86 offsets here; portable code must keep full-width interface values
     * in its adapter hooks rather than widening this target record.
     * Initialization writes the resource at +0x0c, caller callback at +0x10,
     * caller context at +0x14, mode at +0x18, one at +0x20, owner at
     * +0x28, and mode flags at +0x2c. */
    uint32_t words[13];
} EngineTaskCallbackRecord;
typedef char EngineTaskCallbackRecordSizeCheck[
    sizeof(EngineTaskCallbackRecord) == 0x34u ? 1 : -1];
enum SceneUserVisualReason {
    SCENE_USER_VISUAL_CAN_SEE = 0,
    SCENE_USER_VISUAL_RENDER = 1
};
/* Boundaries of the UserVisual manager's real COM/render operations.
 * No default renderer is installed. The owner must keep this context and
 * its borrowed device live until all visual callbacks have finished.
 * Except for the documented tri-state queries, operations return nonzero
 * on success. A failed operation must return any still-owned interface in
 * the output or release it and clear the output; state_end must undo partial
 * begin.
 * group_get resolves a readable, DWORD-aligned 0x70-byte target-layout
 * group, not a parsed Model. Misaligned groups are refused before access.
 * A complete table is admission only, not proof of a real renderer binding. */
typedef struct SceneUserVisualManagerHooks {
    void* context;
    void* renderer_device; /* the actual immediate-mode device (50591c) */
    /* 475830(owner), then owner property +14 bit80: -1 error,0 shown,1 hidden. */
    int (*owner_hidden)(void* context, EngineTaskCallbackRecord* record);
    /* Viewport/frame CANSEE update: -1 error,0 not needed,1 performed. */
    int (*facing_update)(void* context, EngineTaskCallbackRecord* record,
                          void* viewport);
    /* GetTransform(1),483580(viewport),QI(viewport),GetAppData, in order.
     * parameter may be NULL; interface_out must own one reference. */
    int (*render_context_open)(void* context, void* renderer_device,
                                void* viewport, float transform[16],
                                void** parameter_out, void** interface_out);
    void (*interface_release)(void* context, void* interface_object);
    void* (*group_get)(void* context, EngineTaskCallbackRecord* record,
                        uint32_t index);
    /* 4838b0 followed by482ac0(record,parameter,transform,0). */
    int (*state_begin)(void* context, EngineTaskCallbackRecord* record,
                        void* parameter, const float transform[16]);
    /* 483a90(record,group,transform,0) and4839c0(record,transform,0). */
    int (*group_draw)(void* context, EngineTaskCallbackRecord* record,
                       void* group, const float transform[16]);
    int (*model_draw)(void* context, EngineTaskCallbackRecord* record,
                       const float transform[16]);
    /* 478940 then483940, including when state_begin partially failed. */
    int (*state_end)(void* context);
} SceneUserVisualManagerHooks;
void scene_user_visual_manager_hooks_set(
    const SceneUserVisualManagerHooks* hooks);
int scene_user_visual_manager_operations_present(void);
/* Complete callback control owner. RENDER with no device returns one as in
 * retail; that no-draw result must never be used as renderer readiness.
 * Deferred queueing refuses record addresses not representable in32 bits. */
int scene_user_visual_manager_dispatch(EngineTaskCallbackRecord* record,
                                        uint32_t reason, void* viewport);
typedef struct EngineTaskCallbackHooks {
    void* context;
    /* D3DRM3::CreateUserVisual is IDirect3DRM3 slot +0x58. The callback
     * manager object is renderer-owned; `resource_out` represents record
     * +0x0c on x86. The callback_address argument is caller metadata;
     * a production adapter must supply the real manager entry and record
     * cookie to CreateUserVisual, not use that word as the COM callback. */
    void (*user_visual_create)(void* context, uint32_t callback_address,
                               EngineTaskCallbackRecord* record,
                               void** resource_out);
    /* Remaining calls are represented by their observed call shapes; their
     * concrete owner/resource interface contracts have no production
     * adapters in this checkout. */
    void (*owner_bind)(void* context, void* owner, void* resource_interface);
    void (*callback_attach)(void* context, void* resource_interface,
                            EngineTaskCallbackRecord* record);
} EngineTaskCallbackHooks;
void engine_task_callback_hooks_set(const EngineTaskCallbackHooks* hooks);
/* Five retail stack arguments, in callsite order. */
EngineTaskCallbackRecord* engine_task_callback_record_allocate_and_initialize(
    void* owner, void* callback, uint32_t mode, void* context,
    uint32_t flag_value); /* 0x00480870 */
void engine_task_callback_record_release(
    EngineTaskCallbackRecord* record);
typedef struct SceneRotationApplyHooks {
    void* context;
    unsigned int (*child_count)(void* context, void* object);
    void (*apply)(void* context, void* object, unsigned int operation,
                  unsigned int index, float a, float b, float c, float d);
    void (*frame_transform_apply)(void* context, void* object,
                                  float projection_scale,
                                  const float first[3],
                                  const float second[3],
                                  const float third[3]);
} SceneRotationApplyHooks;
void scene_rotation_apply_hooks_set(const SceneRotationApplyHooks* hooks);
void scene_rotation_flags_apply(void* object, float projection_scale,
                                const float first[3],
                                const float second[3], const float third[3],
                                float scale, float uniform_scale,
                                float child_value, uint8_t flags); /* 0x0040ba00 */
/* Preserve the earlier port-facing overload. */
void scene_rotation_flags_apply(void* object, const float first[3],
                                const float second[3], const float third[3],
                                float scale, float uniform_scale,
                                float child_value, uint8_t flags);
typedef struct RendererRecord {
    uint32_t first_word;
    void* visual_resource;
    uint32_t prefix[23];
    void* resource_lists[3];
    uint32_t flags;
} RendererRecord;
typedef struct RendererRecordCloneHooks {
    void* context;
    void* (*linked_records_clone)(void* context, void* head);
    void* (*visual_resource_load)(void* context, void* source);
} RendererRecordCloneHooks;
void renderer_record_clone_hooks_set(const RendererRecordCloneHooks* hooks);
/* Superseded by game/animation_object.c (GAME-004, finding 549); kept
 * uncalled so that the symbol count of every includer is unchanged (ADR 0005
 * entropy rule 1). */
void renderer_record_clone(const RendererRecord* source,
                           RendererRecord* destination); /* FUN_00406cb0 */
typedef struct SceneObjectRecord {
    void* words[0x53];
} SceneObjectRecord;
typedef void* (*SceneObjectRecordRetainHook)(void* context, void* resource);
void scene_object_record_retain_hook_set(SceneObjectRecordRetainHook hook,
                                         void* context);
#ifdef __cplusplus
extern "C"
#endif
void scene_object_record_clone_retain_handles(
    const SceneObjectRecord* source, SceneObjectRecord* destination);
/* Superseded by game/steering.h (GAME-016, finding 544); kept uncalled so
 * the symbol count of every includer is unchanged (ADR 0005 entropy rule 1). */
typedef void (*SceneObjectListApplyHook)(void* context, void* list,
                                         void* object, uint32_t operation,
                                         void* payload);
void scene_object_list_apply_hook_set(SceneObjectListApplyHook hook,
                                      void* context);
void scene_object_list_update_transforms(void* list, void* payload);
void scene_object_list_apply_float_callback(void* list, float value);
void scene_object_list_apply_position_callback(void* list,
                                              const float position[3]);
void scene_object_list_apply_second_position_callback(
    void* list, const float position[3]);
void* quad_mesh_render_record_pool_slot_allocate(void); /* FUN_0040a5c0 */
typedef struct QuadMeshRenderHooks {
    void* context;
    int (*entity_eligible)(void* context, void* entity);
    float (*entity_lift_get)(void* context, void* entity);
    void* (*entity_scene_get)(void* context, void* entity);
    void* (*render_object_create)(void* context, void* parent);
    void* (*callback_record_create)(void* context, void* render_object,
                                   void* pool_record, uint32_t flags);
    void (*digit_quad_prepare)(void* context, void* callback_record,
                               int a, int b, int c, const int offsets[6]);
    void (*render_object_attach)(void* context, void* render_object,
                                 void* parent);
    void (*render_object_disable)(void* context, void* render_object,
                                  int disabled);
    void (*digit_count_apply)(void* context, void* pool_record,
                              uint32_t count);
} QuadMeshRenderHooks;
void quad_mesh_render_hooks_set(const QuadMeshRenderHooks* hooks);
void quad_mesh_render_record_create(void* entity, uint32_t count);
typedef struct SinePhaseBlendState {
    uint32_t flags;
    float period;
    float phase;
    float from[3];
    float to[3];
} SinePhaseBlendState;
typedef struct SinePhaseBlendHooks {
    void* context;
    void (*object_prepare)(void* context, void* object);
    void (*color_apply)(void* context, float red, float green, float blue);
} SinePhaseBlendHooks;
void sine_phase_blend_state_set(const SinePhaseBlendState* state);
void sine_phase_blend_hooks_set(const SinePhaseBlendHooks* hooks);
void sine_phase_midpoint_blend_dispatch(void* object, float delta);
typedef struct RenderResourceBinding {
    void* key;
    void* resource; /* +4 scene object for 0x46c130's target ABI */
    uint32_t state;
    uint32_t flags;
} RenderResourceBinding;
typedef struct RenderResourceBindingHooks {
    void* context;
    void* (*resource_find)(void* context, void* config_a, void* config_b,
                           unsigned int selector);
    void* (*resource_load)(void* context, void* resource);
    void (*resource_release)(void* context, void* resource);
    float (*animation_time_get)(void* context, void* resource,
                               void* argument);
    int (*animation_set)(void* context, void* resource, void* name);
    void* fallback_animation;
} RenderResourceBindingHooks;
void render_resource_binding_hooks_set(
    const RenderResourceBindingHooks* hooks);
#ifdef __cplusplus
extern "C"
#endif
void render_resource_binding_initialize(RenderResourceBinding* binding,
                                        void* key, void* config_a,
                                        void* config_b);
#ifdef __cplusplus
extern "C"
#endif
void render_resource_binding_copy_and_convert(
    const RenderResourceBinding* source,
    RenderResourceBinding* destination);
#ifdef __cplusplus
extern "C"
#endif
void render_resource_binding_release(RenderResourceBinding* binding);
float animation_resource_advance_time(RenderResourceBinding* binding,
                                     void* argument);
void scene_object_set_animation_with_fallback(
    RenderResourceBinding* binding, void* name);
typedef struct SceneObjectChainRecord {
    int32_t type;
    void* node[5];
    float layer_depth;
    uint8_t flags_b8;
} SceneObjectChainRecord;
typedef struct SceneObjectChainHooks {
    void* context;
    void* (*node_create)(void* context, void* parent);
    void (*node_release)(void* context, void* node);
    void (*node_position_set)(void* context, void* node,
                              float x, float y, float z);
    void (*node_orientation_set)(void* context, void* node,
                                 const float direction[3],
                                 const float up[3]);
    void (*node_attach)(void* context, void* node, void* child);
} SceneObjectChainHooks;
void scene_object_chain_hooks_set(const SceneObjectChainHooks* hooks);
#ifdef __cplusplus
extern "C"
#endif
SceneObjectChainRecord* scene_object_chain_record_create(void* parent,
                                                         int type);
void scene_object_chain_record_release(SceneObjectChainRecord* record);
typedef struct SceneObjectDisposeHooks {
    void* context;
    void (*callback_state_remove)(void* context, void** owner,
                                  uint32_t callback_address, int mode);
    void (*scene_node_detach)(void* context, void* scene_node);
    void (*scene_node_release)(void* context, void* scene_node);
    void (*owner_record_release)(void* context, void** owner);
} SceneObjectDisposeHooks;
void scene_object_dispose_hooks_set(const SceneObjectDisposeHooks* hooks);
void scene_object_resource_and_callback_state_dispose(void** owner);

/* The corpus ticks the three view objects per frame (finding 03). */
void view_views_tick(EngineContext* ctx, float dt, float scale);

/* Reconstruction-owned globals (DAT_005560a4 grid follow scale,
 * DAT_005560ac tile-count edge clamp). */
extern float g_view_follow_scale;
extern int   g_view_grid_edge;
/* DAT_005560a0 / DAT_005560a8 — the pointer-pan cap (CameraAcceleration)
 * and per-move scale (CameraSpeed). No ported code feeds them before the
 * view-config setter below runs, so they start zero (the pan is inert
 * until configured — documented deviation). */
extern float g_view_pan_cap;
extern float g_view_pan_scale;

/* Test hook: when nonzero, the mode-3 velocity-EMA target is injected. */
extern int g_view_test_blend;
typedef int (*ViewObjectLookupHook)(int object_id, void* context);
typedef int (*ViewTargetBlendHook)(int object_id, int mode, float blend[3],
                                   float facing[3], void* context);
void view_follow_target_hooks_set(ViewObjectLookupHook lookup,
    ViewTargetBlendHook blend, void* context);

/* --- pure vector helpers (corpus FUN_00401720 / FUN_00401500 /
 * FUN_004016c0 / FUN_00401470 / FUN_00401750 / FUN_00401420 / FUN_004016f0 /
 * FUN_004013a0 / FUN_00401240 / FUN_00406730 / FUN_0047c0c0; finding 280) --
 * 3D unless noted; x87 float10 corpus math is byte-equivalent float here. */
#ifdef __cplusplus
extern "C"
#endif
float* vec_scale(float* dst, const float* src, float s);
#ifdef __cplusplus
extern "C"
#endif
float* vec_add(float* dst, const float* a, const float* b);
#ifdef __cplusplus
extern "C"
#endif
float* vec_sub(float* dst, const float* a, const float* b);
#ifdef __cplusplus
extern "C"
#endif
float* vec_cross(float* dst, const float* b, const float* c);
#ifdef __cplusplus
extern "C"
#endif
void vec_normalize(float* v); /* zero length unguarded, as corpus */
#ifdef __cplusplus
extern "C"
#endif
float vec_dist(const float* a, const float* b);
#ifdef __cplusplus
extern "C"
#endif
float vec_dot(const float* a, const float* b);
#ifdef __cplusplus
extern "C"
#endif
float vec_len(const float* a);
#ifdef __cplusplus
extern "C"
#endif
float vec2_dist(const float* a, const float* b);   /* 2D */
/* Unused: kept only to preserve the declaration count of every unit that
 * includes this header (finding 506) until the #388 relocation. The
 * function is game/bezier.c's (GAME-003, finding 514). */
float vec2_dist_alt(const float* a, const float* b);
/* Minimum 2D distance from point to the closed segment start→end. */
float segment_2d_point_distance(const float* point, const float* start,
                                const float* end); /* FUN_004089d0 */
float point_to_polyline_minimum_segment_distance(const float* point,
                                                 const float* first_point,
                                                 const float* last_point,
                                                 unsigned int segment_count);
/* Superseded by game/steering.h (GAME-016, finding 544); kept uncalled so
 * the symbol count of every includer is unchanged (ADR 0005 entropy rule 1). */
void matrix_rotation_compose_xyz(float* record); /* FUN_0040fb00 */
/* D3DRMVectorRandom takes one D3DVECTOR* (three adjacent floats). */
#ifdef __cplusplus
extern "C"
#endif
float* d3drm_vector_random(float* r); /* FUN_00479500 */
/* FUN_00479510: rotates vector by angle around axis; the import normalizes
 * axis in place. The DLL ABI is (return-vector, source-vector, axis, angle);
 * out may alias either vector input or axis. */
#ifdef __cplusplus
extern "C"
#endif
float* d3drm_vector_rotate(float* r, float* v, float* axis,
                                      float theta);
/* Superseded by game/steering.h (GAME-016, finding 544); kept uncalled so
 * the symbol count of every includer is unchanged (ADR 0005 entropy rule 1). */
void vector_scale_components_in_place(float* vector, float sx, float sy,
                                      float sz); /* FUN_0040fcc0 */
#ifdef __cplusplus
extern "C"
#endif
int vec_exact_equal(const float* a, const float* b);

typedef void (*ViewPointProjectCallback)(void* context, float* output,
                                         const float* input);
typedef struct ViewProjectionAdapter {
    void* context;
    ViewPointProjectCallback screen_project;
    ViewPointProjectCallback transform_point;
} ViewProjectionAdapter;

/* TerrainPointPickResolve (FUN_0045e6d0): the recovered screen rectangle
 * and scale transform used before querying the terrain grid. grid_token is
 * an opaque owner-side handle carried to the callback. */
typedef struct TerrainPointPickRegion {
    uint32_t grid_token;
    float screen_left;
    float screen_top;
    float screen_width;
    float screen_height;
    float tile_size;
    float scale;
    float world_origin_x;
    float world_origin_y;
} TerrainPointPickRegion;
typedef int (*TerrainPointPickCellCheckHook)(uint32_t grid_token,
                                            float world_x, float world_y,
                                            uint32_t argument_a,
                                            uint32_t argument_b,
                                            void* user);
void terrain_point_pick_cell_check_hook_set(
    TerrainPointPickCellCheckHook hook, void* user);
int terrain_point_pick_resolve(const TerrainPointPickRegion* region,
                               uint32_t screen_x, uint32_t screen_y,
                               float out_world_xy[2],
                               uint32_t argument_a,
                               uint32_t argument_b); /* FUN_0045e6d0 */
typedef struct ViewportPointDispatchHooks {
    void* user;
    int (*resolve)(uint32_t grid_token, float world_x, float world_y,
                   float radius, uint32_t* object_handle, void* user);
    int (*position_get)(uint32_t object_handle, float out_xyz[3], void* user);
} ViewportPointDispatchHooks;
void viewport_point_dispatch_hooks_set(const ViewportPointDispatchHooks* hooks);
#ifdef __cplusplus
extern "C"
#endif
int viewport_point_callback_dispatch(const TerrainPointPickRegion* region,
                                                uint32_t screen_x, uint32_t screen_y,
                                                uint32_t* object_handle,
                                                float out_position_xy[2]);
typedef struct TexturedSceneDrawHooks {
    void* user;
    int (*state_save)(void* user, float out_matrix[16]);
    void (*view_transform_set)(void* user, const void* view_transform);
    void (*draw)(void* user, const float position_xyz[3], int width,
                 int height, float scale_x, float scale_y, float scale_z);
    void (*state_restore)(void* user, const float matrix[16]);
} TexturedSceneDrawHooks;
void textured_scene_draw_hooks_set(const TexturedSceneDrawHooks* hooks);
void textured_scene_object_draw_at_position(const void* view,
                                            const float position_xy[2],
                                            float z, float size); /* FUN_0045db00 */
#ifdef __cplusplus
extern "C"
#endif
void camera_screen_coordinate_project(const ViewProjectionAdapter* adapter,
                                      float* output, const float* input);
#ifdef __cplusplus
extern "C"
#endif
void camera_project_point_to_screen(const ViewProjectionAdapter* adapter,
                                   float* output_xy, const float* input_xyz);
float projected_quad_screen_area_calculate(
    const ViewProjectionAdapter* adapter, const float* quad_xyz,
    const float bounds_xy[2], int disable_visibility_clip); /* FUN_0040b470 */

/* 2D orthogonal projections; zero-length directions remain unguarded. */
void point_project_onto_line2d(const float* line_start, const float* line_end,
                               const float* point, float* out);
void point2d_project_onto_direction(const float* origin, const float* direction,
                                    const float* point, float* out);

/* --- geometry helpers (corpus FUN_0045b8a0 / FUN_0045e660 / FUN_00479a30 /
 * FUN_00479af0 / FUN_00479c10; findings 363/323) --------------------------- */
/* True when distance (x1,y1)-(x2,y2) < radius (FUN_0045b8a0). */
#ifdef __cplusplus
extern "C"
#endif
int point_distance_within_radius(int x1, int y1, int x2, int y2,
                                 int radius);
/* Half-open rect test on the record's float quad at offsets +4 through +0x10. */
#ifdef __cplusplus
extern "C"
#endif
int rect_contains_point_half_open(const float* rect_record, int x, int y);

/* --- offset-point rect family (corpus FUN_0045b590 / FUN_00463c00 /
 * FUN_00463c90; findings 241/363/446) --------------------------------------
 * All three are the same half-open containment test over a bounds-quad
 * rect {x_min, y_min, x_max, y_max}: two float offsets are converted by
 * __ftol (round to nearest) and subtracted from the integer point
 * before the compare — `x_min <= float(x - ox) < x_max` and the same for
 * y. Retail 0x45b590/0x463c00 take `(unused, bounds, x, y)` on the stack;
 * the offsets are already in x87 ST(0)/ST(1). The helpers below are
 * port-local adapters and do not model that hidden floating-point ABI. */

/* Direct bounds quad (FUN_0045b590 / FUN_00463c00; the corpus's two call
 * shapes carry identical arithmetic). */
/* Port-local helpers; neither declaration models the retail x87 ABI. */
int rect_contains_offset_point(const float rect_bounds[4],
                               const float offset[2], int x, int y);
int rect_contains_offset_point(const float rect_bounds[4], float off_x,
                               float off_y, int x, int y);
int rect_contains_point_by_converted_coords(const float rect_bounds[4],
                                            const float offset[2],
                                            int x, int y);
int rect_contains_point_by_converted_coords(const float rect_bounds[4],
                                            float off_x, float off_y,
                                            int x, int y);

/* Port-local explicit-bounds helper. Retail FUN_00463c90 takes
 * (record, x, y), resolves bounds at +0x14, and uses two conversion values
 * from state not exposed by this helper's API. */
int rect_contains_point_from_object_bounds(const float* const* bounds_at_14,
                                           float off_x, float off_y,
                                           int x, int y);

/* MenuPointerRectangleHitTest (FUN_004112e0, finding 23/240): the
 * integer widget-rectangle test — inside when left <= x <= left+width
 * and top <= y <= top+height, all four edges closed. The corpus's menu
 * machinery (unported) feeds it the stored pointer position. */
int menu_pointer_rectangle_hit_test(int x, int y, int rect_x, int rect_y,
                                    int rect_w, int rect_h);

/* MenuOverlayListCount (FUN_00412170, findings 23/240): count the
 * overlay-list chain — the head pointer slot at record +0x40, nodes
 * linked through the slot at node +0x18. Deviation: the corpus's
 * 32-bit pointer slots are read native-size (the port's model of
 * corpus pointer fields); NULL head answers 0. */
int menu_overlay_list_count(const void* menu);
/* t = dot(q-p, n) / dot(d, n) into t_out for ray (p,d) × plane (q,n);
 * answers 0 for zero/unordered denominator and leaves t_out unchanged.
 * Negative t is accepted (FUN_00479a30). */
#ifdef __cplusplus
extern "C"
#endif
int ray_plane_intersection_parameter(float* t_out, const float* p,
                                     const float* d, const float* q,
                                     const float* n);
/* Line1 (p1,d1) × line2 (p2,d2): writes the hit into out, returns it;
 * NULL for zero/unordered determinant, leaving out untouched
 * (FUN_00479af0). */
#ifdef __cplusplus
extern "C"
#endif
float* line_intersection_2d_compute(float* out, const float* p1,
                                    const float* d1, const float* p2,
                                    const float* d2);
/* Positive-discriminant ray-sphere test, corpus expression verbatim;
 * `radius2` is the corpus's squared-radius parameter (FUN_00479c10). */
#ifdef __cplusplus
extern "C"
#endif
int sphere_ray_intersection_discriminant_positive(const float* center,
                                                  float radius2,
                                                  const float* origin,
                                                  const float* dir);

/* DDiScene animation record accessors (Finding 430): each asks the scene
 * interface for its current animation/action record and reads one field.
 * A missing record returns zero, matching the reference branch. */
float ddiscene_animation_duration_get(void* scene_interface);
uint32_t ddiscene_animation_state_get(void* scene_interface);
const char* ddiscene_animation_sample_name_get(void* scene_interface);
uint32_t ddiscene_animation_last_frame_get(void* scene_interface);
uint32_t ddiscene_action_record_field_1c_get(void* scene_interface);
void* ddiscene_action_record_field_1c_pointer_get(void* scene_interface);
uint32_t ddiscene_action_record_field_0c_get(void* scene_interface);
const char* scene_property_secondary_name_get(void* property);
uint32_t scene_property_value_at08_get(void* scene_interface);
float scene_property_float_at14_get(void* property); /* 0x004761c0 */
#ifdef __cplusplus
extern "C"
#endif
float typed_child_resource_scalar_get(void* scene_object); /* 0x004757f0 */
void attached_object_data_release_and_clear(void* property); /* 0x004760e0 */
void callback_record_set_flag10_and_refresh(void* property); /* 0x00475e10 */
typedef struct SceneChildSearchHooks {
    void* context;
    void* (*property_get)(void* child_interface, void* context);
    void* (*next_get)(void* child_interface, void* context);
    void (*release)(void* child_interface, void* context);
} SceneChildSearchHooks;
void scene_child_search_hooks_set(const SceneChildSearchHooks* hooks);
void* scene_object_first_non_kind6_child_get(
    void* child_interface); /* 0x00475830 */
typedef struct SceneFormattedStringHooks {
    void* context;
    void (*string_set)(void* scene_object, const char* text, void* context);
    void (*animation_string_bind)(void* scene_object, const char* text,
                                  void* context);
} SceneFormattedStringHooks;
void scene_formatted_string_hooks_set(
    const SceneFormattedStringHooks* hooks);
void scene_object_set_formatted_string(void* scene_object,
                                       const char* format, ...);
typedef struct RenderInterfacePairHooks {
    void* context;
    void (*pair_update)(void* destination, void* source, void* context);
    void (*interface_unlink)(void* interface_object, void* context);
} RenderInterfacePairHooks;
void render_interface_pair_hooks_set(const RenderInterfacePairHooks* hooks);
void render_interface_pair_update(void* destination, void* source);
void ddiscene_scene_object_attach_or_unlink(void* pair_record,
                                            void* source_pair_record);
void scene_object_transform_matrix_set(void* object, uint32_t mode,
                                       const float* matrix);
void scene_object_camera_teleport_or_offset(void* object, uint32_t mode,
                                            float x, float y, float z);
void render_object_set_identity_transform(void* object);
#ifdef __cplusplus
extern "C"
#endif
void render_object_rotate_axis(void* object, uint32_t mode, float x, float y,
                               float z, float angle); /* 0x00475580 */
/* SceneObjectBoundsUnion (FUN_00474f10): query the primary scene interface,
 * then union its child bounds into the caller's six-float min/max box. */
typedef void* (*SceneBoundsRootGetHook)(void* scene_object, void* context);
typedef void (*SceneBoundsQueryHook)(void* scene_interface,
                                     float bounds_min_max[6], void* context);
typedef const void* (*SceneBoundsChildrenGetHook)(void* scene_interface,
                                                   uint32_t* count,
                                                   void* context);
typedef void* (*SceneBoundsChildGetHook)(const void* children,
                                         uint32_t index, void* context);
typedef struct SceneObjectBoundsHooks {
    void* context;
    SceneBoundsRootGetHook root_get;
    SceneBoundsQueryHook bounds_query;
    SceneBoundsChildrenGetHook children_get;
    SceneBoundsChildGetHook child_get;
} SceneObjectBoundsHooks;
void scene_object_bounds_hooks_set(const SceneObjectBoundsHooks* hooks);
uint32_t scene_object_bounds_union(void* scene_object, float bounds_min_max[6]);
typedef struct RenderPackedRgbaHooks {
    void* context;
    void (*packed_set)(void* render_object, uint32_t packed_rgba,
                       void* context);
} RenderPackedRgbaHooks;
void render_packed_rgba_hooks_set(const RenderPackedRgbaHooks* hooks);
void render_object_set_packed_rgba_from_float_channels(
    void* scene_object, float red, float green, float blue,
    float alpha); /* 0x004750b0 */
typedef struct RenderHierarchyFloat3Hooks {
    void* context;
    void (*mesh_record_set)(void* render_record, const float xyz[3],
                            void* context);
    void (*scene_interface_set)(void* scene_interface, const float xyz[3],
                                void* context);
    const void* (*children_get)(void* scene_interface, uint32_t* count,
                                void* context);
    void* (*child_get)(const void* children, uint32_t index, void* context);
    void (*child_interface_set)(void* child_interface, const float xyz[3],
                                void* context);
} RenderHierarchyFloat3Hooks;
void render_hierarchy_float3_hooks_set(
    const RenderHierarchyFloat3Hooks* hooks);
uint32_t render_object_hierarchy_float3_set(void* scene_object,
                                            const float xyz[3]);
typedef int (*SceneActionNameLookupHook)(void* scene_object, void* action_name,
                                         void* context);
void scene_action_name_lookup_hook_set(SceneActionNameLookupHook hook,
                                       void* context);
uint32_t scene_action_name_present(void* scene_object, void* action_name);
#ifdef __cplusplus
extern "C"
#endif
uint32_t scene_action_name_set(void* scene_object, const char* action_name);
typedef struct SceneActFrameScanHooks {
    void* context;
    void* (*animation_interface_get)(void* scene_object, unsigned int slot,
                                     void* context);
    unsigned int (*action_count_get)(void* animation_interface, void* context);
    void* (*action_record_get)(void* animation_interface,
                               unsigned int index, void* context);
    const char* (*action_name_get)(void* animation_interface,
                                   unsigned int index, void* context);
    int (*action_value_get)(void* animation_interface, unsigned int index,
                            int* value, void* context);
} SceneActFrameScanHooks;
void scene_act_frame_scan_hooks_set(const SceneActFrameScanHooks* hooks);
unsigned int scene_action_record_act_frame_scan(void* scene_object,
                                                void** records,
                                                int* frame_indices,
                                                char** names);
/* SceneActFrameRecordCloneOrCreate @ 0x489660. The reference's 32-byte
 * record owns COM interface pointers; the port keeps eight pointer-sized
 * words and requires explicit lifetime hooks where the reference AddRefs
 * an interface. */
typedef struct SceneActFrameRecord {
    uintptr_t words[8];
} SceneActFrameRecord;
typedef struct SceneActFrameRecordCloneHooks {
    void* context;
    void* (*interface_retain)(void* interface_object, void* context);
    int (*full_record_clone)(const SceneActFrameRecord* source,
                             uint32_t clone_argument,
                             SceneActFrameRecord* destination,
                             void* context);
} SceneActFrameRecordCloneHooks;
void scene_act_frame_record_clone_hooks_set(
    const SceneActFrameRecordCloneHooks* hooks);
SceneActFrameRecord* scene_act_frame_record_clone_or_create(
    const SceneActFrameRecord* source, uint32_t clone_argument,
    uintptr_t* output_value);

/* The retail hierarchy exposes its child count/borrowed item-array through
 * vtable +0x80. QueryInterface and child-array traversal use those raw slots;
 * template lookup and creation of the paired source/clone child interfaces
 * need renderer-owned state that has no portable owner here. The child clone
 * adapter returns retained interfaces, which the recursive walker releases. */
namespace lego { struct CachedRenderResourceTemplateRecord; }
typedef struct ObjectHierarchyResourceHooks {
    void* context;
    lego::CachedRenderResourceTemplateRecord* (*template_resolve)(
        void* resource, uint32_t mode, void* context);
    int (*child_clone_register)(void* source_parent, void* clone_parent,
                                void* child_interface,
                                void** source_child_out,
                                void** clone_child_out, void* context);
} ObjectHierarchyResourceHooks;
void object_hierarchy_resource_hooks_set(
    const ObjectHierarchyResourceHooks* hooks);
void com_child_items_visit(void* parent); /* 0x00489bc0 */
void object_interface_array_resource_create(void* object, uint32_t mode);
void object_hierarchy_recursive_clone(void* source, void* clone,
                                      uint32_t mode); /* 0x004899f0 */
typedef struct EntityVisualResourceLoadHooks {
    void* context;
    int (*kind_get)(void* scene_object, void* context);
    const char* (*name_get)(void* scene_object, void* context);
    void* (*resource_lookup)(const char* name, void* context);
    void* (*resource_create)(void* scene_object, const char* name,
                             void* context);
    void (*resource_prepare)(void* resource, void* scene_object,
                             void* context);
    void (*animation_build)(void* resource, void* scene_object,
                            void* context);
    void (*resource_attach)(void* scene_object, void* resource,
                            void* context);
} EntityVisualResourceLoadHooks;
void entity_visual_resource_load_hooks_set(
    const EntityVisualResourceLoadHooks* hooks);
#ifdef __cplusplus
extern "C"
#endif
void* entity_visual_resource_load(void* scene_object); /* 0x473690 */
typedef void* (*SceneActionInterfaceGetHook)(void* scene_object,
                                             void* action_name,
                                             uint32_t selector,
                                             void* context);
void scene_action_interface_get_hook_set(SceneActionInterfaceGetHook hook,
                                         void* context);
typedef float (*VideoAspectRatioQueryHook)(void* video_object,
                                           void* context);
void video_aspect_ratio_query_hook_set(VideoAspectRatioQueryHook hook,
                                       void* context);
float video_object_aspect_ratio_get(void* video_object);
typedef struct SceneContextHooks {
    void* shared_scene;
    int (*owner_interface_get)(void* scene_object, void** owner_interface,
                               void* context);
    void* (*owner_link_ensure)(void* owner_interface, void* context);
    void (*interface_release)(void* owner_interface, void* context);
    void* context;
} SceneContextHooks;
void scene_context_hooks_set(const SceneContextHooks* hooks);
void* ddiscene_context_get(void* scene_object);
void* scene_object_owner_link_ensure(void* owner_interface);
typedef struct DDiSceneAnimationHookRecord {
    void* scene_object;
    char* name;
    int value_08;
    uint32_t value_0c;
    uint32_t value_10;
    uint32_t value_14;
    int value_18;
    int callback;
    uint32_t value_20;
    int value_24;
} DDiSceneAnimationHookRecord;
DDiSceneAnimationHookRecord* ddiscene_animation_hook_record_get(void* object);
typedef struct DDiSceneAnimationHookRecordHooks {
    void* context;
    DDiSceneAnimationHookRecord* (*record_get)(void* scene_object,
                                                void* context);
    void (*record_set)(void* scene_object,
                       DDiSceneAnimationHookRecord* record,
                       void* context);
} DDiSceneAnimationHookRecordHooks;
void ddiscene_animation_hook_record_hooks_set(
    const DDiSceneAnimationHookRecordHooks* hooks);
#ifdef __cplusplus
extern "C"
#endif
void ddiscene_animation_hook_create(void* scene_object, int callback,
                                    const char* name,
                                    const uint32_t* value_0c, int value_08,
                                    const uint32_t* value_10,
                                    const uint32_t* value_14,
                                    int value_18, int value_24,
                                    const uint32_t* value_20); /* 0x475f70 */
/* The third argument is the interface-value slot used by the nested +0x5c /
 * +0x74 dispatch; it is not a float output buffer. */
void ddiscene_timed_hook_dispatch(void* hook_record, float time,
                                  void* interface_value); /* 0x4897e0 */
void ddiscene_apply_act_frame_timing(void* scene_object); /* 0x475300 */
typedef struct DDiSceneFrameRecordFindHooks {
    void* context;
    unsigned int (*count_get)(void* action_interface, int selector,
                              void* context);
    void* (*record_get)(void* action_interface, int selector,
                        unsigned int index, void* context);
    const char* (*record_name_get)(void* record, void* context);
    void (*record_retain)(void* record, void* context);
} DDiSceneFrameRecordFindHooks;
void ddiscene_frame_record_find_hooks_set(
    const DDiSceneFrameRecordFindHooks* hooks);
void* ddiscene_frame_record_find(void* action_interface,
                                 const char* action_name,
                                 int selector); /* 0x475e40 */
typedef struct DDiSceneLightCreateHooks {
    void* context;
    void* (*record_create)(void* owner, void* context);
    void (*record_kind_set)(void* record, unsigned int kind, void* context);
    int (*light_create)(void* record, uint32_t value_a, uint32_t value_b,
                        uint32_t value_c, uint32_t value_d,
                        void** light_interface,
                        void* context);
    void (*light_configure)(void* record, uint32_t value_b, void* context);
    void (*animation_hook_set)(void* record, void* renderer_interface,
                               void* context);
} DDiSceneLightCreateHooks;
void ddiscene_light_create_hooks_set(const DDiSceneLightCreateHooks* hooks);
#ifdef __cplusplus
extern "C"
#endif
void* ddiscene_light_create(void* owner, uint32_t value_a,
                            uint32_t value_b, uint32_t value_c,
                            uint32_t value_d); /* 0x4734f0 */
typedef struct DDiSceneKindModeSetupHooks {
    void* context;
    void* (*record_create)(void* owner, void* context);
    void (*record_kind_set)(void* record, unsigned int kind, void* context);
    void* (*mode_resource_create)(void* record, unsigned int resource_kind,
                                  void* context);
    void (*animation_resource_set)(void* record, void* resource,
                                   void* context);
    void (*legacy_mode_setup)(void* record, int mode, void* context);
} DDiSceneKindModeSetupHooks;
void ddiscene_kind_mode_setup_hooks_set(
    const DDiSceneKindModeSetupHooks* hooks);
void* ddiscene_object_kind_mode_setup(void* owner, int mode); /* 0x473560 */
typedef void (*DDiSceneDrawBeginHook)(void* render_object, int begin,
                                      void* context);
void ddiscene_draw_begin_hook_set(DDiSceneDrawBeginHook hook,
                                  void* context);
void ddiscene_draw_begin(void* render_object, int begin); /* 0x477010 */
#ifdef __cplusplus
extern "C"
#endif
void ddiscene_draw_end(void* render_object, const void* commit_source,
                       float frame_delta); /* 0x477150 */
void* com_property_result_get(void* resource_owner); /* 0x477310 */
void* com_resource_enumeration_result_get(void* resource_owner);
typedef struct SceneHierarchyNodeInfo {
    void* scene_interface;
    void* first_child;
    const char* display_name;
    unsigned int flags;
    unsigned short node_id;
} SceneHierarchyNodeInfo;
typedef struct SceneHierarchyRegisterHooks {
    void* context;
    int (*node_inspect)(void* node, SceneHierarchyNodeInfo* info,
                        void* context);
    void (*interface_store)(void* root, unsigned int index,
                            void* scene_interface, void* context);
    void (*node_index_set)(void* node, unsigned short index, void* context);
    void (*identity_transform_set)(void* scene_interface, void* context);
    void (*interface_name_set)(void* scene_interface, const char* name,
                               void* context);
    void* (*next_sibling)(void* node, void* context);
} SceneHierarchyRegisterHooks;
void scene_hierarchy_register_hooks_set(
    const SceneHierarchyRegisterHooks* hooks);
void scene_hierarchy_interface_register_recursive(unsigned short* next_index,
                                                  void* root, void* node); /* 0x4887c0 */

/* Shared-object scene resource loader. 0x4876c0 stores a path and callbacks
 * used later by the LWS entry expander; it does not enumerate directories. */
typedef int (*SharedObjectEntryResolveFn)(const char* entry_name,
                                          void* entry_record);
typedef int (*SharedObjectSoundEnabledFn)(void);
typedef int (*SharedObjectSoundSubmitFn)(void* sound_object, int alias,
                                         uint32_t loop, uint32_t positional,
                                         const float* xyz);
void shared_object_directory_scan_configure(
    const char* directory, SharedObjectEntryResolveFn resolve,
    SharedObjectSoundEnabledFn sound_enabled,
    SharedObjectSoundSubmitFn sound_submit); /* 0x4876c0 */
/* Owns a copied prefix; NULL clears the borrowed model fallback before free. */
int shared_object_directory_root_set(const char* directory);
const char* shared_object_directory_root_get(void);
void shared_object_sound_callbacks_get(
    SharedObjectSoundEnabledFn* enabled_out,
    SharedObjectSoundSubmitFn* submit_out);
void shared_object_timed_sound_event_update(void* owner, void* track); /* 0x487de0 */
void scene_material_manager_set(void* manager);
#ifdef __cplusplus
extern "C"
#endif
void* scene_material_create(float color0, float color1, float color2,
                            float color3, float color4, float color5,
                            float power); /* 0x4894c0 */

typedef struct SceneEntryExpandInfo {
    const char* name;
    uint32_t flags;
    uint8_t slot_index;
} SceneEntryExpandInfo;
typedef struct SceneEntryExpandedRecord {
    unsigned char bytes[0xd0];
} SceneEntryExpandedRecord;
typedef struct SceneEntryExpandOps {
    void* context;
    int (*counts_get)(void* scene, uint16_t* source_count,
                      uint16_t* expanded_capacity, void* context);
    int (*entry_get)(void* scene, uint16_t index,
                     SceneEntryExpandInfo* info, void* context);
    void (*entry_slot_set)(void* scene, uint16_t index, uint8_t slot,
                           void* context);
    void (*expanded_table_set)(void* scene,
                               SceneEntryExpandedRecord* table,
                               void* context);
    void (*expanded_capacity_set)(void* scene, uint16_t capacity,
                                  void* context);
    uint32_t (*debug_flags_get)(void* context);
} SceneEntryExpandOps;
void scene_entry_expand_ops_set(const SceneEntryExpandOps* ops);
void shared_object_scene_entry_slot_table_expand(void* scene); /* 0x4877e0 */

/* The reference clone copies the fixed header but rebuilds child registration
 * and each interface-backed component through its owning scene/runtime. */
typedef struct SharedObjectCloneOps {
    void* context;
    void* (*source_base_get)(void* source, void* context);
    void* (*child_first_get)(void* source, void* context);
    void* (*child_next_get)(void* child, void* context);
    uint16_t (*entry_count_get)(void* source, void* context);
    uint32_t (*entry_flags_get)(void* source, uint16_t index,
                                void* context);
    void* (*component_get)(void* source, uint16_t index, void* context);
    void (*component_table_set)(void* clone, void** table,
                                void* context);
    void (*source_link_set)(void* clone, void* source, void* context);
    void (*clone_reference_set)(void* clone, uint32_t count,
                                void* context);
    void (*source_reference_increment)(void* source, void* context);
    void (*child_register)(void* clone, void* child, void* caller_context,
                           void* context);
    void (*component_clone)(void* source_interface, void* clone_interface,
                            void* clone, uint16_t index, void* context);
    void (*component_resource_create)(void* source_interface,
                                      void* clone_interface,
                                      void* clone, uint16_t index,
                                      void* context);
    void (*time_update)(void* clone, float time, void* context);
} SharedObjectCloneOps;
void shared_object_clone_ops_set(const SharedObjectCloneOps* ops);
void* shared_object_clone(void* source, void* caller_context); /* 0x487a00 */

typedef void (*DDiSceneObjectDestroyHook)(void* scene_object, int mode,
                                          void* context);
void ddiscene_object_destroy_hook_set(DDiSceneObjectDestroyHook hook,
                                      void* context);
void ddiscene_object_destroy(void* scene_object); /* 0x472a40 */
void ddiscene_object_destroy_with_mode(void* scene_object, int mode); /* 0x472a50 */
#ifdef __cplusplus
extern "C"
#endif
float ddiscene_object_length_get(void* scene_object); /* 0x00475390 */
#ifdef __cplusplus
extern "C"
#endif
uint32_t ddiscene_object_frame_position_get(
    void* scene_object); /* 0x004753f0 */
void ddiscene_animation_callbacks_set(void* scene_object,
                                      void* callback_a, void* callback_b,
                                      void* callback_c, void* callback_d);
typedef struct DDiScenePayloadReleaseHooks {
    void* context;
    void (*interface_release)(void* interface_object, void* context);
    void (*kind1_detach)(void* owner, void* resource, void* context);
    void (*kind8_detach)(void* resource, void* owner, void* context);
} DDiScenePayloadReleaseHooks;
void ddiscene_payload_release_hooks_set(
    const DDiScenePayloadReleaseHooks* hooks);
void ddiscene_payload_release(void* scene_object); /* 0x00475c80 */
int ddiscene_resource_name_resolve(const char* name); /* 0x00476770 */
typedef int (*DdiSceneResourceExistsHook)(const void* file_data,
                                          uint32_t file_size,
                                          const char* path,
                                          void* scene_interface,
                                          void* context);
/* The existence adapter receives bytes owned by the query and must not keep
 * the pointer after it returns. */
/* FUN_00476850's 32-bit callback record is {path, normalized flag, resource
 * handle, data}. This host adapter keeps that field order with native-width
 * pointers; it is consumed synchronously while the caller owns the resource. */
typedef struct DdiSceneMeshLoadContext {
    const char* path;
    uint32_t optional_flag;
    void* resource_handle;
    const void* file_data;
} DdiSceneMeshLoadContext;
typedef void* (*DdiSceneResourceOpenHook)(
    const DdiSceneMeshLoadContext* load_context, void* scene_interface,
    void* context);
void ddiscene_resource_hooks_set(DdiSceneResourceExistsHook exists_hook,
                                 DdiSceneResourceOpenHook open_hook,
                                 void* context);
/* Retail args: resource handle, data, path, scene interface, optional flag.
 * It creates a mesh builder through DAT_00505914, loads via LAB_00476900,
 * gets/releases the builder mesh, and attaches through scene +0x48. */
void* ddiscene_resource_open(void* resource_handle, const void* file_data,
                             const char* path, void* scene_interface,
                             uint32_t optional_flag); /* 0x00476850 */
typedef void (*DDiSceneSampleHook)(const char* sample_name,
                                   void* scene_object, void* context);
typedef void (*DDiSceneEndHook)(void* scene_object, void* context);
void ddiscene_frame_callbacks_set(DDiSceneSampleHook sample_hook,
                                  void* sample_context, int sample_enabled,
                                  DDiSceneEndHook end_hook,
                                  void* end_context);
/* Add a sample callback without replacing the scene owner's callback.
 * Re-adding the same callback/context pair is harmless. */
int ddiscene_sample_hook_add(DDiSceneSampleHook sample_hook,
                             void* sample_context);
/* Typed ACT owners share the installed sample/trigger dispatch without
 * pretending that their native records have the original ACT layout. */
void ddiscene_activity_sample_dispatch(const char* sample, void* scene_object);
void ddiscene_activity_trigger_dispatch(void* scene_object);
#ifdef __cplusplus
extern "C"
#endif
float ddiscene_frame_update_and_callbacks(void* scene_object, float time); /* 0x475140 */
float ddiscene_advance_by_time(void* scene_object, float delta); /* 0x00475120 */
#ifdef __cplusplus
extern "C"
#endif
float ddiscene_advance_by_time_target(void* scene_object, float delta);
typedef float (*SceneTimingValueGetHook)(void* frame, void* argument,
                                         void* context);
typedef int (*SceneTimingCountGetHook)(void* frame, void* context);
void scene_timing_value_hooks_install(SceneTimingValueGetHook value_get,
                                      SceneTimingCountGetHook count_get,
                                      void* context);
#ifdef __cplusplus
extern "C"
#endif
float renderer_object_timing_value_adjust_by_count(void* record,
                                                   float time,
                                                   uint32_t count); /* FUN_00406d30 */
typedef void (*AnimationObjectTimeUpdateHook)(void* frame, float time,
                                              void* context);
typedef void (*AnimationObjectActionHook)(void* frame, void* context);
void animation_object_hooks_install(AnimationObjectTimeUpdateHook update,
                                    AnimationObjectActionHook time_get,
                                    AnimationObjectActionHook value_get,
                                    void* context);
/* Superseded by game/animation_object.c (GAME-004, finding 549); kept
 * uncalled so that the symbol count of every includer is unchanged (ADR 0005
 * entropy rule 1). */
void animation_object_time_update(void* record, float time); /* FUN_00406d10 */
void animation_object_time_get(void* record); /* FUN_00406da0 */
void animation_object_value_get(void* record); /* FUN_00407090 */
typedef void* (*RendererSubobjectFindHook)(void* frame, void* key,
                                           void** details, void* context);
typedef void (*RendererSubobjectApplyHook)(void* frame, void* subobject,
                                           int mode, int flags,
                                           void* context);
void renderer_subobject_hooks_install(RendererSubobjectFindHook find,
                                      RendererSubobjectApplyHook apply,
                                      void* context);
/* Superseded by game/animation_object.c (GAME-004, finding 549); kept
 * uncalled so that the symbol count of every includer is unchanged (ADR 0005
 * entropy rule 1). */
void* renderer_object_subobject_find_apply(void* record, void* key);
/* Superseded, like the line above; FUN_00406f50 is now
 * animation_object_null_find in game/animation_object.c. */
void renderer_object_subobject_dispatch(void* record, void* key);
#ifdef __cplusplus
extern "C"
#endif
void* renderer_cached_handle_48_get_or_create(void* record);
                                                /* FUN_00406fe0 */
/* Superseded by game/animation_object.c (GAME-004, finding 549); kept
 * uncalled so that the symbol count of every includer is unchanged (ADR 0005
 * entropy rule 1). */
void* renderer_cached_handle_50_get_or_create(void* record);
                                                /* FUN_00407010 */
typedef float (*RendererHandleScalarGetHook)(void* handle, void* context);
void renderer_handle_scalar_hook_install(RendererHandleScalarGetHook hook,
                                         void* context);
/* Superseded by game/animation_object.c (GAME-004, finding 549); kept
 * uncalled so that the symbol count of every includer is unchanged (ADR 0005
 * entropy rule 1). */
int renderer_cached_handle_4c_is_not_one(void* record); /* FUN_00407040 */
void* scene_object_indexed_handle_get_or_create(void* record, int index);
#ifdef __cplusplus
extern "C"
#endif
void* scene_object_primary_handle_get_or_create(void* record);
#ifdef __cplusplus
extern "C"
#endif
void* scene_object_secondary_handle_get_or_create(void* record);
#ifdef __cplusplus
extern "C"
#endif
void* scene_object_indexed_secondary_handle_get_or_create(void* record,
                                                           int index);
float animation_loop_time_adjust(void* record, float argument,
                                 uint32_t count); /* FUN_00408860 */
void scene_frame_scale_set(void* object, uint32_t mode, float x, float y,
                           float z); /* 0x004755b0 */
void scene_object_coordinate_transform_to_output(void* object,
                                                  void* input, void* output);
void scene_object_coordinate_transform_in_place(void* object,
                                                void* coordinates,
                                                void* transform);
/* DDiSceneObject position/pose dispatch (FUN_004754c0/430/470). These
 * typed wrappers normalize the object and optional alternate frame, then
 * call the scene object's +0x60/+0xf0/+0xec interface methods. */
#ifdef __cplusplus
extern "C"
#endif
int ddiscene_object_position_get(void* object, void* alternate_frame,
                                 float position[3]);
int ddiscene_object_facing_get(void* object, void* alternate_frame,
                               float facing[3]); /* FUN_00475500 */
#ifdef __cplusplus
extern "C"
#endif
int ddiscene_object_facing_pair_get(void* object, void* alternate_frame,
                                    float first[3], float second[3]);
#ifdef __cplusplus
extern "C"
#endif
int ddiscene_object_position_set(void* object, void* alternate_frame,
                                 float x, float y, float z);
#ifdef __cplusplus
extern "C"
#endif
int ddiscene_object_pose_method_dispatch(void* object,
                                         void* alternate_frame,
                                         float parameter_3,
                                         float parameter_4,
                                         float parameter_5,
                                         float parameter_6,
                                         float parameter_7,
                                         float parameter_8);
int light_interface_call_slot_38(void* record, uint32_t value);
int light_interface_call_slot_3c(void* record, uint32_t value);
int light_interface_call_slot_40(void* record, uint32_t value);

/* DDiSceneRenderSurfacePoolGrow (FUN_00477370): grow the surface-record
 * free list geometrically and expose its acquire/release operations. The
 * original uses 0x20-byte records with a pointer at +0x1c; native 64-bit
 * builds widen the stride to keep that pointer in bounds. */
void ddiscene_render_surface_pool_grow(void);
void* ddiscene_render_surface_pool_acquire(void);
void ddiscene_render_surface_pool_release(void* record);
extern void* s_render_surface_pool_blocks[32];
extern unsigned int s_render_surface_pool_level_count;
extern void* s_render_surface_pool_free_head;
void ddiscene_render_surface_pool_head_push(void* record);
void ddiscene_render_surface_pool_initialize(void);
unsigned int ddiscene_render_surface_pool_block_count(void);
void* ddiscene_render_surface_pool_block_get(unsigned int index);
void ddiscene_render_surface_pool_release_all(void);

/* SceneObjectRegistryReleaseAll/Teardown (FUN_0047e130/FUN_0047d430).
 * The small-record pool owner registers each allocated block here. Live
 * records carry a 32-bit interface token at +0 and a self marker at +0x1c;
 * an optional callback resolves/releases that token on hosts where the
 * reference's 32-bit interface pointer cannot be called directly. */
typedef void (*SceneRegistrySurfaceReleaseFn)(uint32_t interface_token);
#ifdef __cplusplus
extern "C"
#endif
void image_surface_release(void* record); /* 0x0047d470 */
void scene_object_registry_surface_release_set(
    SceneRegistrySurfaceReleaseFn release);
void scene_object_registry_pool_block_register(void* block);
void scene_object_registry_release_all(void);
#ifdef __cplusplus
extern "C"
#endif
void scene_object_registry_teardown(void);

/* Owner-link records retain up to two direct references, an optional child
 * chain, and a counted handle array. */
typedef void (*SceneOwnerLinkReleaseFn)(void* object);
typedef struct SceneObjectOwnerLinkRecord {
    int kind;
    void* child_chain;
    void* first_handle;
    int shared_mode;
    void* second_handle;
    void** handles;
    uint32_t handle_count;
} SceneObjectOwnerLinkRecord;
void scene_object_owner_link_release_hooks_set(
    SceneOwnerLinkReleaseFn interface_release,
    SceneOwnerLinkReleaseFn child_chain_release);
void scene_object_owner_link_release(SceneObjectOwnerLinkRecord* record);

/* Advisor companion transforms (FUN_004734c0) cross two scene interfaces
 * owned by the advisor scene. Keep the recovered two-stage call order while
 * letting the owner register those interfaces. */
typedef void (*AdvisorCompanionApplyFn)(void* object,
                                        const void* transform,
                                        void* reference_slot);
void advisor_companion_apply_hooks_set(const void* transform,
                                       AdvisorCompanionApplyFn object_apply,
                                       AdvisorCompanionApplyFn scene_apply);
#ifdef __cplusplus
extern "C"
#endif
void advisor_companion_transform_apply(void* unused, void** object_reference);

/* SceneObjectQueryValueIfReady (FUN_00476f70): inspect object +0x10's
 * readiness interface, then query the current value and release the +8
 * reference on the ready path. The current-value interface is advisor-owned
 * hidden state in the original; install it explicitly in the port. */
typedef int (*SceneObjectReadyQueryFn)(void* ready_interface);
typedef const uint32_t* (*SceneObjectCurrentValueFn)(void* context);
typedef void (*SceneObjectValueReleaseFn)(void* owner, void** object_reference);
void scene_object_value_query_hooks_set(void* context,
                                        SceneObjectReadyQueryFn ready_query,
                                        SceneObjectCurrentValueFn value_query,
                                        SceneObjectValueReleaseFn release);
uint32_t scene_object_query_value_if_ready(void* object);

/* --- 4x4 matrix primitives + polygon parity test (corpus FUN_00479ea0 /
 * FUN_00479ce0 / FUN_00479b80; findings 309/388) ---------------------------
 * Row-major float storage; corpus x87 math is byte-equivalent float here. */

/* Zero the matrix (FUN_00479ea0: 16 stores). */
#ifdef __cplusplus
extern "C"
#endif
void matrix4_set_zero(float* out);

/* Multiply: corpus takes (out, right, left) and accumulates each output
 * cell as a four-term dot product, i.e. out = left × right, row-major
 * (FUN_00479ce0). */
#ifdef __cplusplus
extern "C"
#endif
void matrix4_multiply(float* out, const float* right,
                                 const float* left);

/* Even-odd horizontal-ray crossing parity (FUN_00479b80). Edge k runs
 * verts[k] → next[k]; the corpus reads the edge end through a pointer pair
 * that strides both arrays in lockstep, so any caller whose `next` is the
 * vertex array shifted one vertex (the typical ring form) behaves 1:1.
 * No zero-span/boundary handling beyond the corpus's, including the
 * potentially zero-span divide. */
#ifdef __cplusplus
extern "C"
#endif
int point_in_polygon_2d(const float* point, const float* verts,
                                   const float* next, unsigned int count);

/* --- matrix/ray math leaves (2026-10-24) ----------------------------------
 * Pure single-precision arithmetic over row-major float[16] storage; no
 * corpus state. The corpus rotation setters evaluate x87 `fcos`/`fsin` on
 * the angle promoted to extended precision; the port evaluates cos/sin in
 * double and rounds each result into the float cell — the documented x87
 * deviation (cf. x87_round_to_integral), which the float store hides at
 * nearly every angle. */

/* out = origin + t · dir, corpus component order (FUN_00479ab0). */
#ifdef __cplusplus
extern "C"
#endif
void point_along_ray(float* out, const float* origin,
                                const float* dir, float t);

/* ViewRayIntersectionTest (FUN_004798e0): normalizes both directions,
 * intersects ray (p, normalized dir_a) with the plane through q with the
 * normalized dir_b as normal (through ray_plane_intersection_parameter),
 * and writes out = p + t · normalized dir_a, answering 1; parallel rays
 * answer 0 with `out` untouched. */
#ifdef __cplusplus
extern "C"
#endif
int view_ray_intersection_test(float* out, const float* p,
                               const float* dir_a, const float* q,
                               const float* dir_b);

/* Matrix4SetIdentity (FUN_00479e70): 1 on the diagonal, 0 elsewhere. */
#ifdef __cplusplus
extern "C"
#endif
void matrix4_set_identity(float* out);

/* MatrixIdentityAndTranslationSet (FUN_00479e40): the identity with the
 * translation triad written into row-major row 3. */
#ifdef __cplusplus
extern "C"
#endif
void matrix4_identity_translation_set(float* out,
                                                 const float* translation);

/* Matrix4Copy (FUN_00479eb0): 16 dword copies. */
#ifdef __cplusplus
extern "C"
#endif
void matrix4_copy(float* out, const float* src);

/* Rotation setters: each writes the identity, then overwrites the
 * two-row/two-column rotation block with (cos, −sin/sin, cos) at the
 * corpus's slots:
 *   - matrix4_set_rotation_x: m[5]=c, m[6]=−s, m[9]=s, m[10]=c (rows 1–2);
 *   - matrix4_set_rotation_y: m[0]=c, m[2]=s, m[8]=−s, m[10]=c (rows 0/2);
 *   - matrix4_set_rotation_z: m[0]=c, m[1]=−s, m[4]=s, m[5]=c (rows 0–1).
 * The corpus reference names call FUN_00479d50 "Matrix4SetRotationZ" as
 * well as FUN_00479df0; the row/column blocks distinguish the three and
 * are what the port names. */
#ifdef __cplusplus
extern "C"
#endif
void matrix4_set_rotation_x(float* out, float angle); /* FUN_00479d50 */
#ifdef __cplusplus
extern "C"
#endif
void matrix4_set_rotation_y(float* out, float angle); /* FUN_00479da0 */
#ifdef __cplusplus
extern "C"
#endif
void matrix4_set_rotation_z(float* out, float angle); /* FUN_00479df0 */

/* BuildPerspectiveProjectionMatrix (FUN_0040bd60): zeroes the matrix,
 * sets m[10] = m[15] = 1, then the three quotients m[0] = p4/p1,
 * m[5] = p3/p2, m[1] = (p5/p4) · (p4/p1) — corpus expression as written. */
void perspective_projection_matrix_set(float* out, float p1, float p2,
                                       float p3, float p4, float p5);

/* RandomReachableGridPointSelect (FUN_00450560): iterate the ray
 * (p, dir) down through the z-plane — starting at the plane point
 * (0, 0, -50) with normal (0, 0, -1) — `rounds` times, letting the
 * terrain height of the hit (the FUN_0044f940 hook; the port reads the
 * voxel surface byte) ride back into the plane point's z, so every
 * iteration from the second descends the plane to the previous height.
 * The final hit's grid cell then lands in gx/gy through the FUN_0044f6d0
 * hook. The corpus loop tests != 0, so a negative `rounds` runs until
 * it wraps; with rounds == 0 the corpus grid-picks from uninitialized
 * stack, so the port reads whatever `out` the caller left. */
void random_reachable_grid_point_select(EngineContext* ctx, const float* p,
                                        const float* dir, float* out,
                                        int* gx, int* gy, int rounds);

/* --- camera-handle primitives (record-backed) ----------------------------- */
void cam_offset(CameraHandle* c, float x, float y, float z);
void cam_get_pos(CameraHandle* c, float out[3]);            /* FUN_004754c0 */
void cam_set_pos(CameraHandle* c, float x, float y, float z);/* FUN_00475430 */
void cam_get_facing(CameraHandle* c, float out[3]);         /* FUN_00475500 */
void cam_set_facing(CameraHandle* c, const float* r);       /* FUN_00475470 */

/* TerrainGridBorderInsetGet (FUN_00435f28): the tile-count edge clamp —
 * g_view_grid_edge is DAT_005560ac with its ported writer, so this is
 * the faithful getter. */
#ifdef __cplusplus
extern "C"
#endif
int view_grid_edge_get(void);

/* --- camera-selection cluster (2026-10-24) ---------------------------------
 * CameraViewContextSet / SelectionRecordPointSet /
 * CameraGetPositionWithTerrainHeight / CameraPointerPanUpdate
 * (FUN_00435e0e / FUN_00436b0b / FUN_00436b33 / FUN_00436b87, findings
 * 102/339/378/424/426): the follow-view camera leaves. The setter stores
 * the four view-config globals (cap, follow scale, pan scale, edge). The
 * point set writes the follow camera's x/y for follow-mode views; the
 * position read copies the follow camera and replaces its z with the
 * terrain height (the corpus's world-handle argument maps to the engine
 * context's bound level block — documented deviation). The pan update
 * normalizes the move vector, advances the per-view accumulator by
 * amount * pan scale, caps it, scales the vector by accumulator * amount,
 * and accumulates it into the follow offsets (the corpus's +0x18 field).
 * Zero-length vectors normalize as-is (the corpus's unguarded leaf). */
#ifdef __cplusplus
extern "C"
#endif
void camera_view_context_set(float cap, float follow_scale,
                             float pan_scale, int edge);
#ifdef __cplusplus
extern "C"
#endif
void selection_record_point_set(ViewContext* v, float x, float y);
int  camera_get_position_with_terrain_height(ViewContext* v,
                                             EngineContext* ctx,
                                             float out[3]);
#ifdef __cplusplus
extern "C"
#endif
void camera_pointer_pan_update(ViewContext* v, float vec[3], float amount);

/* --- the camera-controller bounds band (2026-10-24; finding 280) ------------
 * The corpus's camera-key controller record (offsets into the record its
 * per-view callers pass): bounds stores write the lo/hi floats and raise
 * a gate bit in the flag byte at +0xb8; the distance setter is gated on
 * the bit-4 gate unless bit 8 overrides; zoom adds to the stored value.
 * The record itself is not modeled in the port (its owners are the
 * per-view camera objects) — these take a raw record pointer and write
 * fixed offsets, the port's record-module style, entry-guarded where the
 * corpus dereferences blindly (documented deviations). */
#ifdef __cplusplus
extern "C"
#endif
void camera_tilt_bounds_set(void* camera, float lo, float hi);
                                        /* FUN_00436831 */
typedef void (*CameraPoseDispatchHook)(void* scene_object, void* pose_context,
                                       float x, float y, float z,
                                       float pitch, float yaw, float roll,
                                       void* context);
void camera_pose_dispatch_hook_install(CameraPoseDispatchHook hook,
                                       void* context);
#ifdef __cplusplus
extern "C"
#endif
void camera_tilt_set(void* camera, float value); /* FUN_00436860 */
/* Signature also fits CameraDragHooks' camera_vertical_apply callback when
 * its context is the controller record. */
#ifdef __cplusplus
extern "C"
#endif
void camera_pan_axis_update(void* camera, float delta); /* FUN_00436900 */
void screen_effect_record_set(void* camera_record,
                              uint32_t field4, uint32_t field8,
                              uint32_t fieldc, float tilt,
                              uint32_t field10); /* FUN_00435ebb */
void camera_scalar_bounds_set(void* camera, float lo, float hi);
                                        /* FUN_00436921 */
#ifdef __cplusplus
extern "C"
#endif
void camera_rotation_set(void* camera, float value); /* FUN_00436953 */
#ifdef __cplusplus
extern "C"
#endif
void camera_distance_bounds_set(void* camera, float lo, float hi);
                                        /* FUN_00436a18 */
#ifdef __cplusplus
extern "C"
#endif
void camera_distance_set(void* camera, float value); /* FUN_00436a4a */
#ifdef __cplusplus
extern "C"
#endif
void camera_zoom_update(void* camera, float delta);  /* FUN_00436aa5 */

/* CursorScreenRayContextReset (FUN_00435f16): clear the modeled ViewContext
 * pan accumulator at +0x9c. The target x86 body is the corpus's blind store;
 * the portable path retains a NULL guard. */
#ifdef __cplusplus
extern "C"
#endif
void cursor_screen_ray_context_reset(ViewContext* v);

/* CurrentLevelTerrainGridPick (FUN_00433be0): the corpus's per-level pick
 * wrapper — the world pointer from the state root's +4 field, x/y floats
 * in, grid indices out. The port's grid_pick models the pick (with the
 * stub-boundary note above); this is the wrapper shape, and NULL index
 * outputs skip their stores the way the corpus's bounds check does. */
void current_level_terrain_grid_pick(EngineContext* ctx, float x, float y,
                                     int* gx, int* gy);

/* GridOperationArgumentAdapter (FUN_00442540): the corpus's grid-op
 * argument rotator — the terrain-height call takes (level, x, y) while
 * grid code hands (x, y, level); the float result is discarded (void).
 * NULL contexts skip (deviation). */
void grid_operation_argument_adapter(float x, float y, EngineContext* ctx);

/* Superseded by game/steering.h (GAME-016, finding 544); kept uncalled so
 * the symbol count of every includer is unchanged (ADR 0005 entropy rule 1). */
/* SetGlobalScalarToOne (FUN_0040fec0): the corpus draws the 0…1 ramp and
 * drops it; the port models that ramp as 1.0, so the call's observable
 * effect is nothing — faithful. */
void set_global_scalar_to_one(void);

/* --- the view math band (2026-10-24; findings 280/256) -----------------------
 * FloatNegateAbsoluteValue (FUN_0044d250): both corpus branches flip the
 * sign — the port keeps the single in-place negate.
 * CameraZoomStepAdjust (FUN_0045bf20): direction 0 zooms by -3x the
 * step, 1 by +3x, anything else is inert; the target is the primary
 * view/camera context at DAT_005560e4. */
void float_negate_absolute_value(float* value);
/* Unused: kept for the declaration count (see vec2_dist_alt above). */
void vector2_set_length(float v[2], float length);
void camera_zoom_step_context_set(void* context);
#ifdef __cplusplus
extern "C"
#endif
void camera_zoom_step_adjust(int direction, float step);

/* SelectionRecordPointGet (FUN_00436ae9): the follow-mode readback — the
 * selection record's camera position through the corpus's move-or-pose read
 * (cam_get_pos). Non-follow records answer nothing (the corpus's gate). */
#ifdef __cplusplus
extern "C"
#endif
void selection_record_point_get(ViewContext* v, float out[3]); /* FUN_00436ae9 */

/* SelectedGlobalPointTransformAndReturnX (FUN_0046a420, finding 426): picks
 * the follow view in-game (DAT_005560e4) or the editor view otherwise
 * (DAT_005560ec — the engine context's mode word and view windows), reads
 * the camera position with the terrain-height replacement through the
 * ported pair, and returns the frame array's first word (x). The corpus's
 * level-terrain-handle read rides in the call as the ported callee's
 * world-handle argument (folded into the bound level block per deviation). */
float selected_global_point_transform_and_return_x(EngineContext* ctx);
                                                /* FUN_0046a420 */

/* RendererNodeRelativePositionSet (FUN_00436ac9, findings 435/372): the
 * renderer node's +0x28 camera handle rides the ported mode-1 side of the
 * camera teleport/offset dispatch with x/y and a zero z. Deviation: the
 * corpus strides the handle as a dword; the port reads it as the module's
 * widened pointer slot. */
void renderer_node_relative_position_set(const void* node, float x, float y);
                                                /* FUN_00436ac9 */

/* Application-owned scene arena. The Win32 bootstrap installs a real
 * retained-mode factory before creating the root and camera/light frames.
 * CameraHandle is a different layout; it is never passed to a retail leaf. */
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
int scene_runtime_initialize(void);
void scene_runtime_shutdown(void);
int scene_runtime_primary_camera_bind(ViewContext* view, void* controller);
void* scene_runtime_camera_bind(CameraHandle* camera);
int scene_runtime_cameras_sync(void);
/*4291a0 samples the follow position and primary eye forward through real
 * retained-mode frames. No CameraHandle is cast to a DDi record. */
int scene_runtime_collection_pose_get(const ViewContext* view, int primary,
                                       float position[3], float forward[3]);
void* scene_runtime_light_create(void* owner, unsigned int kind,
                                  const float rgb[3]);
int scene_runtime_light_parameter_set(void* light, unsigned int offset,
                                       float value);

/* WinAPI backend owns the DLL and its IDirect3DRM3 factory reference. */
int scene_winapi_initialize(void);
void scene_winapi_shutdown(void);
struct IDirect3DRMUserVisual;
struct IDirect3DRMDevice;
struct IDirect3DRMViewport;
/* The COM method is stdcall, but D3DRMUSERVISUALCALLBACK is cdecl.
 * The reference manager at 0x4827f0 also returns without popping args. */
typedef int (__cdecl *SceneUserVisualCallback)(
    IDirect3DRMUserVisual*, void*, SceneUserVisualReason,
    IDirect3DRMDevice*, IDirect3DRMViewport*);
int __cdecl scene_runtime_user_visual_manager(
    IDirect3DRMUserVisual* visual, void* cookie, SceneUserVisualReason reason,
    IDirect3DRMDevice* device, IDirect3DRMViewport* viewport);
/* On success the caller owns one interface reference. On failure the
 * output is NULL, including when the factory returns a failed object. */
int scene_winapi_user_visual_create(SceneUserVisualCallback callback,
                                    void* cookie, void** visual);
/* RMDevice3::GetDirect3DDevice2 then QI(IDirect3DDevice3). Success transfers
 * one owned immediate-device reference; failure clears the output. */
int scene_winapi_device3_acquire(void* retained_device, void** immediate);
/* Borrows both inputs; success transfers one RMViewport2 reference. Failure
 * clears both outputs and releases temporaries; a successful viewport bind
 * is not rolled back if the later RM interface query fails. */
int scene_winapi_user_visual_context_open(void* immediate_device,
    void* retained_viewport, float transform[16], void** parameter_out,
    void** interface_out);
int scene_winapi_frame_create(void* parent, void** frame);
void scene_winapi_frame_release(void* frame);
void scene_winapi_interface_release(void* object);
int scene_winapi_light_create(unsigned int kind, const float rgb[3],
                               void** light);
#endif
void scene_root_install(void* root);            /* port hook */
void* scene_root_peek(void);                    /* port hook */

/* SceneRootObjectGet (FUN_00472940): the corpus's root-object read. */
void* scene_root_object_get(void);              /* FUN_00472940 */
/* Direct scene-interface wrappers: the interface pointer is at object + 0x10;
 * clip distance uses vtable byte slot 0x3c and point transform uses 0x50. */
#ifdef __cplusplus
extern "C"
#endif
void renderer_object_set_clip_distance(void* object, float distance);
#ifdef __cplusplus
extern "C"
#endif
void render_interface_transform_point(void* object, void* output,
                                      const void* input);
/* SceneProperty callback returns a two-dword value record; null inputs or
 * missing callback/record return zero (the native port's guard policy). */
uint32_t scene_property_primary_value_get(void* property); /* FUN_00476120 */
uint32_t scene_property_secondary_value_get(void* property); /* FUN_00476140 */

/* Superseded by game/steering.h (GAME-016, finding 544); kept uncalled so
 * the symbol count of every includer is unchanged (ADR 0005 entropy rule 1). */
/* SceneNodePositionSet (FUN_00410020): the scene node's position store —
 * the node's position-valid flag at +0xc goes to 1, and the three floats
 * land on the inner object's +0xc/+0x10/+0x14. Deviation: the corpus
 * strides the node's dword handle slot blindly; the port reads the inner
 * through the module's widened pointer slot (stride native) and guards a
 * missing handle. */
void scene_node_position_set(void* node, const float pos[3]);
                                                /* FUN_00410020 */

/* DDiSceneObjectArgumentsNormalize (FUN_00475900): resolves the dispatch
 * pair — the first slot rides out of the first argument's slot, and the
 * second resolves to its own slot when the argument is non-NULL, else to
 * the root object's first slot (the corpus's null-handle fallback).
 * Arguments point to wrapper records, not local variables containing wrapper
 * pointers. The x86 target retains the retail blind dereferences; other hosts
 * retain their guarded missing-root path. */
void scene_object_arguments_normalize(const void** obj, const void** alt,
                                      void** out_obj, const void** out_alt);
                                                /* FUN_00475900 */

/* Matrix4IdentityInitialize (FUN_00483540): the straight-line shape — the
 * twelve off-diagonal dwords clear, then the four diagonal entries land as
 * the float-1.0 dwords. (The corpus's row-looping sibling stays cited as
 * its own row.) */
void matrix4_identity_initialize(float* out);   /* FUN_00483540 */

/* SurfaceTriangleNormalCalculate (FUN_00479530): the triangle normal as
 * the unit cross product of the edges (p3 − p2) and (p4 − p3). Deviation
 * kept faithful: there is no zero-length guard before the reciprocal, so
 * degenerate input yields the corpus's inf/NaN. */
#ifdef __cplusplus
extern "C"
#endif
float* surface_triangle_normal_calculate(float* out, const float* p2,
                                       const float* p3, const float* p4);
                                                /* FUN_00479530 */
#ifdef __cplusplus
extern "C"
#endif
float surface_triangle_area_calculate(const float* p1, const float* p2,
                                      const float* p3,
                                      int only_downward); /* FUN_00479630 */

/* SceneMeshColorAndAlphaSet (FUN_00475020): the scene-side color/alpha
 * pair over an object's mesh record — the object's +0xc inner pointer
 * chains to the indexed record the pair's window model reads (widened
 * pointer slots per convention), then the RGB pair lands through the
 * slot-0 color set and the alpha through the mode-4 opacity composition.
 * Deviations: the corpus strides both link slots blindly — the port
 * guards them, and a missing record leaves both windows untouched. */
#ifdef __cplusplus
extern "C"
#endif
void scene_mesh_color_and_alpha_set(void* object, int block, float red,
                                    float green, float blue, float alpha);

/* SceneMeshColorSet (FUN_00474ff0): the color triple without the alpha,
 * mode three of the ported color-vector setter (the block's +0x34 slot).
 * Deviation: the corpus strides both record links blindly. */
#ifdef __cplusplus
extern "C"
#endif
void scene_mesh_color_set(void* object, int block, float red, float green,
                          float blue);
                                                /* FUN_00475020 */

/* RotatedPairsScratchFill (FUN_00464110): copies count pair dwords from
 * the source into the module scratch, applies the corpus's signed
 * rotation class (the (param_4 + 0x8000) sign-fold mod-4 selector: 1
 * swaps both negated, 2 negates both, 3 lands (y, -x), else identity —
 * negative folds default), adds the offset pair, and answers the
 * scratch pointer. Deviations: bounded scratch (the corpus writes its
 * DAT region blindly) and count guarded. */
extern "C" void* rotated_pairs_scratch_fill(const int* offset_pair, const int* source,
                                 int count, int rotation);          /* FUN_00464110 */

/* Observer hook answering the scratch base (test pin). */
void* rotated_pairs_scratch_peek(void);

/* --- curve band (2026-10-25) -------------------------------------------------
 * The Bezier helpers (FUN_004065f0-FUN_004068c0) are game/bezier.c (GAME-003,
 * issue #417, finding 514), with the original's types. The four declarations
 * below are unused and have no definition: they are kept only to preserve
 * the declaration count of every unit that includes this header (finding
 * 506) until the #388 relocation. Use game/bezier.h. */
void cubic_bezier_point_evaluate(float out[2], const float* p0, const float* p1,
                                 const float* p2, const float* p3, float t);
void bezier_curve_samples_build(uint32_t* dest, const uint32_t* control,
                                const uint32_t* p3, const uint32_t* p4,
                                const uint32_t* p5, uint32_t count);
float polyline_segment_lengths_total(uint32_t* curve);
uint32_t path_arc_length_interpolate(const uint32_t* curve, float travel,
                                     float* out);

/* ViewPointToScreenGeometryTransform (FUN_0045e940): projects four floats
 * in place through the projection window — screen offsets at +1c/+20 come
 * off first, the scale ratio (+14 / +18) scales x, y, and depth, x lands
 * through the half-size + center pair, y mirrors through its own, and w
 * rides the ratio. Deviations: NULL guards; the ratio is computed once. */
void view_point_to_screen_geometry_transform(const void* view,
                                             float p[4]);     /* FUN_0045e940 */

/* Map row-span staging (FUN_0046ead0 / 0x0046eb30): append x/y pairs to
 * a row buffer and begin a new buffer record. The port exposes staged
 * records for the renderer's unported consumer. */
void map_row_span_append(void* buffer, uint32_t y, uint32_t x_begin,
                         uint32_t x_end);
void map_row_span_record_start(uint32_t y, uint32_t x_begin,
                               uint32_t x_end);
uint32_t map_row_span_record_count(void);
const void* map_row_span_record_get(uint32_t index);
int float_round_to_nearest_int(float value); /* FUN_00463e40 */

/* ActivityRunnerStateRead is mapped at 0x00406f80. Scene activity handle
 * getters use the renderer subobject resolver below and cache the returned
 * value in the original activity-record slots (0x004085f0, 0x00408770,
 * 0x004087b0, 0x004087e0, 0x00408810). */
typedef void* (*RendererSubobjectResolveFn)(void* owner, void* source,
                                            int index);
void renderer_subobject_resolver_set(RendererSubobjectResolveFn resolver);
/* Superseded by game/animation_object.c (GAME-004, finding 549); kept
 * uncalled so that the symbol count of every includer is unchanged (ADR 0005
 * entropy rule 1). */
void* activity_runner_state_read(void* runner, int index);
void* scene_activity_change_secondary(void* activity, int index);
#ifdef __cplusplus
extern "C"
#endif
void* renderer_cached_handle_40_get_or_create(void* renderer);
typedef int (*SceneActivityChangeFn)(void* activity, void* state,
                                     void* value);
void scene_activity_change_callback_set(SceneActivityChangeFn callback);
int scene_activity_change(void* activity, void* state, void* value);
/* Superseded by game/steering.h (GAME-016, finding 544); kept uncalled so
 * the symbol count of every includer is unchanged (ADR 0005 entropy rule 1). */
typedef void (*GlobalVectorNotifyFn)(void* context, const uint32_t values[4]);
void global_vector_notify_callback_set(GlobalVectorNotifyFn callback);
void global_vector_set_and_notify(void* context, const uint32_t values[4]);
void global_vector_get(uint32_t values[4]); /* FUN_0040ffe0 state snapshot */
typedef void (*GlobalEntityRenderParamFn)(void* entity, float first,
                                          float second, void* user);
void global_entity_render_parameters_install(void** entities, uint32_t count,
                                             GlobalEntityRenderParamFn update,
                                             void* user);
#ifdef __cplusplus
extern "C"
#endif
void global_entity_render_parameter_mode_set(int mode); /* 0x0046e1c0 */
typedef void* (*GridSurfaceLookupFn)(void* surface, int key,
                                     int* entry_index, void* user);
typedef void (*GridSurfaceHandleReleaseFn)(uint32_t handle, void* user);
void grid_surface_hooks_set(GridSurfaceLookupFn lookup,
                            GridSurfaceHandleReleaseFn release, void* user);
void grid_surface_entry_reset(void* surface, int key); /* 0x0046e330 */
typedef struct WorldSpatialRecordModel {
    uint32_t point_count;
    const float* point_pairs;
    void* user_record;
} WorldSpatialRecordModel;
void world_spatial_records_install(WorldSpatialRecordModel* records,
                                   uint32_t count);
#ifdef __cplusplus
extern "C"
#endif
WorldSpatialRecordModel* world_spatial_record_find_at_grid_coordinate(
    int x, int y, uint32_t* point_index); /* 0x0046e8a0 */
typedef void* (*UIRenderTreeHook)(int operation, void* tree, void* entry,
                                  void* subrecord, void* user);
void ui_render_tree_hook_set(UIRenderTreeHook hook, void* user);
void ui_render_tree_layout_and_release(void* tree, void* layout_context);
void ui_render_element_children_enumerate(void* tree, void* entry);
void ui_render_tree_entry_process(void* tree, void* entry);
void ui_render_entry_subrecord_apply(void* tree, void* entry,
                                     uint32_t subrecord_index);
#endif /* LEGORRR_VIEW_HPP */
