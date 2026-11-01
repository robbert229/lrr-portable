// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — display surface scaffold (finding 13 create path).
 *
 * A thin wrapper over a DirectDraw offscreen surface (the corpus's
 * DDSURFACEDESC / create pipeline, FUN_00474050) reached through the
 * platform seam (platform.h re-exports the backend's DirectDraw). On miniwin
 * the surface is an SDL framebuffer in SDL_PIXELFORMAT_RGBA32 (little-endian
 * byte order R,G,B,A); Lock hands back pixels+pitch, Unlock releases, and the
 * platform glue presents it (miniwin's surface Flip is a stub by design).
 *
 * The draw helpers convert the texture loader's ARGB (0xAARRGGBB) surfaces
 * into that byte order. TODO(winapi): the reference build needs vendored
 * DirectDraw headers; until then the DDraw half compiles to no-ops there.
 */
#ifndef LEGORRR_SURFACE_HPP
#define LEGORRR_SURFACE_HPP

#include <cstdint>
#include <cstddef>

#include "texture.hpp"
#include "scene_resources.hpp"

typedef struct Model Model;
#if defined(LEGORRR_BACKEND_MINIWIN)
#include <miniwin/d3drm.h>
#endif

typedef struct DisplaySurface {
    void*  dd;    /* LPDIRECTDRAW  (opaque out of the seam) */
    void*  surf;  /* LPDIRECTDRAWSURFACE */
    void*  hwnd;
    int    w;
    int    h;
} DisplaySurface;

/* The original render-surface allocator normalizes a rectangle against the
 * display, pops a pooled record, and attaches the created surface to it
 * (FUN_00476e50). The backend owns display queries, pool growth, and the
 * device-specific surface call through these operations. */
typedef struct ScreenBoundedSurfaceRecord {
    unsigned char bytes[0x20u + sizeof(void*) - sizeof(uint32_t)];
} ScreenBoundedSurfaceRecord;
void screen_bounded_surface_device_set(void* renderer, void* device);
ScreenBoundedSurfaceRecord* screen_bounded_surface_allocate(
    uint32_t width, uint32_t height, void* camera, int x, int y);
/* RenderTargetViewportScaleSet (FUN_00476dc0): turn normalized viewport
 * origin and size values into device pixels, then allocate its viewport. */
ScreenBoundedSurfaceRecord* render_target_viewport_scale_set(
    float x, float y, float width_scale, float height_scale, void* camera);

/* FUN_00482220: one 0x70-byte mesh render record. The corpus stores the
 * index count/pointer at offsets 0/8 and vertex count/pointer at 4/0xc. */
typedef struct MeshRenderRecord {
    uint32_t index_count;
    uint32_t vertex_count;
    uint16_t* indices;
    void* vertices;
    uint8_t reserved[0x70 - 8 - 2 * sizeof(void*)];
} MeshRenderRecord;
typedef struct MeshRecordPool {
    uint32_t count;
    uint32_t capacity;
    MeshRenderRecord* records;
    void* context;
    void* (*allocate)(void* context, size_t bytes);
    void (*release)(void* context, void* memory);
    void (*record_registered)(void* context, uint32_t index);
} MeshRecordPool;
int mesh_record_allocate_and_fill(MeshRecordPool* pool,
                                  uint32_t vertex_count,
                                  uint32_t index_rows,
                                  uint32_t index_columns,
                                  const uint16_t* source_indices);
typedef struct ModelRenderPassDispatchOps {
    void* context;
    void* (*first)(void* context);
    void* (*next)(void* context, void* entry);
    void* (*model)(void* context, void* entry);
    const void* (*parameters)(void* context, void* entry);
    uint32_t (*entry_flags)(void* context, void* entry);
    uint32_t (*model_flags)(void* context, void* model);
    uint32_t (*record_count)(void* context, void* model);
    void* (*record)(void* context, void* model, uint32_t index);
    int (*record_ready)(void* context, void* record);
    uint32_t (*record_flags)(void* context, void* record);
    int (*record_has_secondary_material)(void* context, void* record);
    void (*frame_setup)(void* context, void* device);
    void (*render_mode_begin)(void* context);
    void (*model_base_pass)(void* context, void* model, void* device,
                            const void* parameters);
    void (*model_child_pass)(void* context, void* model, void* record,
                             const void* parameters, int enabled);
    void (*model_alternate_pass)(void* context, void* model,
                                 const void* parameters, int enabled);
    void (*render_queue_flush)(void* context);
    void (*render_state_flush)(void* context);
} ModelRenderPassDispatchOps;
void model_render_pass_dispatch_ops_set(
    const ModelRenderPassDispatchOps* ops);
void model_render_pass_dispatch(void* device); /* portable adapter */
typedef struct MeshRecordAppendOps {
    void* context;
    MeshRecordPool* (*pool_get)(void* context, void* object);
    int (*legacy_append)(void* context, void* object,
                         uint32_t vertex_count, uint32_t index_rows,
                         uint32_t index_columns,
                         const uint16_t* source_indices);
} MeshRecordAppendOps;
int mesh_record_allocate_or_append_ops_set(const MeshRecordAppendOps* ops);
int mesh_record_allocate_or_append(void* object, uint32_t vertex_count,
                                   uint32_t index_rows,
                                   uint32_t index_columns,
                                   const uint16_t* source_indices);
typedef struct ModelTextureInput {
    void* image_resource;
    uint8_t color[3];
    uint8_t reserved0;
    uint8_t animated_name;
    uint8_t color_from_source;
    uint8_t reserved1[2];
    float values[8];
    /* Portable path used when no renderer-owned name resolver is installed. */
    const char* image_name;
} ModelTextureInput;
typedef struct ModelTextureBinding {
    void* texture;
    void** animation_frames;
    uint32_t animation_frame_count;
    float color[3];
    float values[10];
    /* LWO SURF/TSIZ is planar-map scale consumed by MaterialPass. It is kept
     * separate from the opaque retail material values above. */
    float texture_scale[2];
    uint8_t has_texture_scale;
    uint8_t texture_surface_owned;
    uint8_t animation_array_owned;
    uint8_t texture_callback_owned;
} ModelTextureBinding;
typedef struct ModelTextureBindOps {
    void* context;
    int (*name_get)(void* context, void* image_resource,
                    uint32_t frame, char* output, size_t output_size,
                    uint32_t* first_frame, int* animated);
    void* (*texture_get_or_create)(void* context, void* device,
                                   const char* name);
    void (*texture_release)(void* context, void* texture);
    void* (*allocate)(void* context, size_t bytes);
    void (*release)(void* context, void* memory);
} ModelTextureBindOps;
void model_texture_bind_ops_set(const ModelTextureBindOps* ops);
int bind_model_textures(void* device, const ModelTextureInput* inputs,
                        ModelTextureBinding* bindings, uint32_t count,
                        int force_source_color);
/* Bind LWO SRFS/TIMG records to the portable texture cache. This API does
 * not use the retail 0x481820 material-record ABI. */
int bind_lwo_model_textures(void* device, const Model* model,
                            ModelTextureBinding* bindings, uint32_t count,
                            int force_source_color);
void model_texture_bindings_release(ModelTextureBinding* bindings,
                                    uint32_t count);
typedef struct MaterialPassOps {
    void* context;
    int (*geometry_prepare)(void* context, void* device, void* model,
                            void* layer, void** geometry_out,
                            uint32_t* pass_count_out);
    int (*texture_bind)(void* context, void* device, void* geometry,
                        uint32_t pass);
    int (*material_state_apply)(void* context, void* device,
                                void* geometry, uint32_t pass);
    int (*geometry_submit)(void* context, void* device, void* geometry,
                           uint32_t pass);
    void (*geometry_release)(void* context, void* geometry);
} MaterialPassOps;
void material_pass_ops_set(const MaterialPassOps* ops);
int material_pass_execute(void* device, void* model, void* layer);
typedef struct SurfacePixelFormat SurfacePixelFormat;
typedef struct SurfaceDescriptionInfo {
    unsigned int width;
    unsigned int height;
    int pitch;
    void* pixels;
    unsigned int bits_per_pixel;
} SurfaceDescriptionInfo;

int  surface_create(DisplaySurface* out, void* hwnd, int w, int h);
void surface_destroy(DisplaySurface* s);
int  surface_width(DisplaySurface* s);
int  surface_height(DisplaySurface* s);
int surface_description_get(const DisplaySurface* surface,
                            int* width_out, int* height_out); /* FUN_0047cdd0 */

/* Lock returns the framebuffer pointer + row pitch (RGBA32 R,G,B,A on LE). */
int  surface_lock(DisplaySurface* s, void** pixels, int* pitch);
void surface_unlock(DisplaySurface* s, void* pixels);
/* Portable adapters with explicit DisplaySurface records. Retail 0x47cc20
 * and 0x47cc50 are no-argument functions over renderer-owned globals. */
void render_surface_restore_from_primary(DisplaySurface* destination,
                                         DisplaySurface* primary,
                                         int restore_enabled);
void windowed_render_surface_blit(DisplaySurface* primary,
                                  DisplaySurface* back_surface);
void surface_blit_wait(void* destination_surface,
                       DisplaySurface* source); /* FUN_0047ed00 */
/* These DisplaySurface/output-record queries are portable adapters. Retail
 * 0x47de60 and 0x47def0 operate on DirectDraw interface slots and use
 * different output contracts. */
int surface_directdraw_description_get(DisplaySurface* surface,
                                       SurfaceDescriptionInfo* out);
int surface_directdraw_pixel_format_get(DisplaySurface* surface,
                                        SurfacePixelFormat* out);
typedef struct RenderObjectStateOps {
    void* context;
    int (*object_kind)(void* context, void* object);
    int (*children_snapshot)(void* context, void* object,
                             void*** children_out,
                             unsigned int* count_out);
    void (*children_snapshot_release)(void* context, void** children);
    void (*child_state_apply)(void* context, void* child, uint32_t value);
    unsigned int (*layer_count)(void* context, void* object);
    void (*layer_property_set)(void* context, void* object,
                               unsigned int index, uint32_t value);
} RenderObjectStateOps;
void render_object_state_apply_to_entries(void* object, uint32_t value,
                                          const RenderObjectStateOps* ops);
typedef struct TerrainSurfacePropertyOps {
    void* context;
    int (*has_mesh)(void* context, void* object);
    void* (*layer_resolve)(void* context, void* object,
                           unsigned int layer_index,
                           unsigned int* frame_out);
    void (*property_set)(void* context, void* layer,
                         unsigned int frame, uint32_t property_mask);
} TerrainSurfacePropertyOps;
void terrain_surface_property_set(void* object, unsigned int layer_index,
                                  int enabled,
                                  const TerrainSurfacePropertyOps* ops); /* FUN_00474e30 */
typedef struct LwoLayerEnabledOps {
    void* context;
    void* (*mesh_get)(void* context, void* object);
    void (*mesh_set)(void* context, void* mesh, unsigned int index,
                     int enabled);
    void* (*layer_get)(void* context, void* object, unsigned int index);
    int (*base_enabled_get)(void* context, void* layer);
    void (*base_enabled_set)(void* context, void* layer, int enabled);
    uint32_t (*layer_flags_get)(void* context, void* layer);
    void (*layer_flags_set)(void* context, void* layer, uint32_t flags);
    void (*layer_attach)(void* context, void* object, void* layer);
    void (*layer_detach)(void* context, void* object, void* layer);
} LwoLayerEnabledOps;
void lwo_layer_enabled_set(void* object, unsigned int index, int enabled,
                           const LwoLayerEnabledOps* ops); /* FUN_00474b30 */
typedef struct LwoLayerDispatchOps {
    void* context;
    void* (*mesh_get)(void* context, void* object);
    void* (*layer_resolve)(void* context, void* object,
                           unsigned int index, unsigned int* frame_out);
    uint32_t (*mesh_polygon_vertex_count)(void* context, void* mesh,
                                          const void* request);
    int (*layer_polygon_vertex_count)(void* context, void* layer,
                                     unsigned int frame,
                                     const void* request);
    uint32_t (*mesh_polygon_vertex_data_read)(void* context, void* mesh,
                                             const void* request);
    int (*layer_polygon_vertex_data_read)(void* context, void* layer,
                                         unsigned int frame,
                                         const void* request);
    uint32_t (*mesh_terrain_quad_emit)(void* context, void* mesh,
                                       const void* request);
    int (*layer_terrain_quad_emit)(void* context, void* layer,
                                   unsigned int frame,
                                   const void* request);
} LwoLayerDispatchOps;
uint32_t lwo_layer_polygon_vertex_count(
    void* object, unsigned int layer_index, const void* request,
    const LwoLayerDispatchOps* ops); /* FUN_00474c40 */
uint32_t lwo_layer_polygon_vertex_data_read(
    void* object, unsigned int layer_index, const void* request,
    const LwoLayerDispatchOps* ops); /* FUN_00474cc0 */
uint32_t terrain_cell_quad_vertex_emit(
    void* object, unsigned int layer_index, const void* request,
    const LwoLayerDispatchOps* ops); /* FUN_00474d30 */
typedef struct TerrainDetailLookupOps {
    void* context;
    void* (*mesh_get)(void* context, void* object);
    uint32_t (*mesh_entry_get)(void* context, void* mesh,
                               unsigned int index);
    void* (*layer_collection_get)(void* context, void* object);
    void* (*children_get)(void* context, void* collection);
    void* (*child_get)(void* context, void* children,
                       unsigned int index);
    uint32_t (*base_enabled_get)(void* context, void* children);
    uint32_t (*child_flags_get)(void* context, void* child);
} TerrainDetailLookupOps;
uint32_t terrain_detail_entry_get_at_index(
    void* object, unsigned int index, const TerrainDetailLookupOps* ops); /* FUN_00474ae0 */
typedef struct DirectDrawObjectSet {
    void* primary_surface;
    void* primary_clipper;
    void* windowed_clipper;
    void* device;
    int restore_display_mode;
} DirectDrawObjectSet;
typedef struct DirectDrawObjectReleaseOps {
    void* context;
    void (*release)(void* context, void* object);
    void (*restore_display_mode)(void* context, void* device);
} DirectDrawObjectReleaseOps;
void directdraw_objects_release(DirectDrawObjectSet* objects,
                                const DirectDrawObjectReleaseOps* ops); /* FUN_0047ccf0 */
int surface_directdraw_pixel_read(DisplaySurface* surface, unsigned int x,
                                  unsigned int y, uint32_t* pixel_out);

/* Fill the locked framebuffer with a solid color. */
void surface_fill(void* px, int pitch, int w, int h, int r, int g, int b);

/* Blit a texture (top-left at x,y), converting ARGB -> RGBA32. */
void surface_blit_texture(void* px, int pitch, int w, int h,
                          const TextureSurface* t, int x, int y);

/* Moving bar (animation proof; magenta). */
void surface_draw_sweep(void* px, int pitch, int w, int h, int frame);
typedef void (*SurfaceTextCharacterMapInsertFn)(void* map, uint8_t key,
                                                uint8_t value);
void surface_text_character_map_initialize(void* map,
                                           SurfaceTextCharacterMapInsertFn insert);
                                                /* FUN_0042b850 */
typedef int (*SurfacePixelBitsQueryFn)(void* surface, uint32_t* bits_per_pixel);
void surface_pixel_bits_query_hook_set(SurfacePixelBitsQueryFn query);
typedef int (*SurfaceLockFormatQueryFn)(void* surface,
                                        uint32_t* bits_per_pixel,
                                        uint32_t* red_mask,
                                        uint32_t* green_mask,
                                        uint32_t* blue_mask);
int32_t surface_pixel_channel_mask_get(void* surface); /* FUN_0047df50 */
void surface_render_setup(const float rectangle[4]); /* FUN_00485e80 */
typedef void (*RenderStateSetHook)(void* context, uint32_t state,
                                   uint32_t value);
typedef int (*RenderStateGetHook)(void* context, uint32_t state,
                                  uint32_t* value_out);
typedef struct RenderStateApplyOps {
    void* context;
    RenderStateSetHook set_state;
    RenderStateGetHook get_state;
    int (*render_flag_test)(void* context, uint32_t mask);
    int (*texture_transform_default)(void* context, float matrix_out[16]);
    void (*texture_transform_apply)(void* context, int mode,
                                    const float matrix[16]);
} RenderStateApplyOps;
void render_state_apply_hooks_set(const RenderStateApplyOps* ops);
void renderer_command_mode_and_pair_send(uint32_t first, uint32_t second);
void render_state_flags_apply(uint32_t flags, const float* transform,
                              int alternate_mode); /* FUN_00482b50 */
void d3d_material_state_cache_snapshot_store(uint32_t state,
                                            uint32_t value);
void direct3d_render_state_set_cached(uint32_t state,
                                      uint32_t value); /* FUN_004788d0 */
void d3d_material_state_flush(void); /* FUN_00478940 */
typedef struct RendererWindowModeOps {
    void* context;
    void (*window_mode_set)(void* context, int fullscreen);
    int (*desktop_rect_get)(void* context, int rect_out[4]);
    void (*window_style_set)(void* context, int32_t style);
    void (*window_position_set)(void* context, int x, int y,
                                int width, int height, unsigned int flags);
    void (*cursor_show)(void* context, int show);
    void (*cursor_hide)(void* context);
    void (*window_show_activate)(void* context, int show_command);
} RendererWindowModeOps;
void renderer_window_mode_ops_set(const RendererWindowModeOps* ops);
void renderer_window_mode_apply(int fullscreen, int x, int y,
                                int width, int height); /* FUN_004780b0 */
uint32_t renderer_window_mode_flags_peek(void);
void renderer_window_dimensions_get(int* width_out, int* height_out);
typedef int (*DirectDrawEnumerationHook)(void* context, const void* guid,
                                         void** enumerated_result);
void directdraw_enumeration_hook_set(DirectDrawEnumerationHook hook,
                                     void* context);
int directdraw_enumerate_and_store_result(const void* guid,
                                          void** result_out); /* FUN_0047c1c0 */
typedef void (*Direct3DRenderPreferenceApplyHook)(
    void* context, uint32_t parameter, uint32_t preference,
    uint32_t render_mode);
void direct3d_render_preference_hook_set(
    Direct3DRenderPreferenceApplyHook hook, void* context);
void direct3d_render_preferences_apply(uint32_t mode, uint32_t parameter,
                                       int enabled, int secondary,
                                       int high_quality); /* FUN_00478330 */
typedef struct Direct3DMatrixOps {
    void* context;
    /* Direct3D HRESULT convention: zero is success. */
    int (*get_transform)(void* context, uint32_t method, float matrix_out[16]);
    int (*set_transform)(void* context, uint32_t method,
                         const float matrix[16]);
} Direct3DMatrixOps;
void direct3d_matrix_ops_set(const Direct3DMatrixOps* ops);
int matrix_method_pair_apply(uint32_t method, const float matrix[16]); /* FUN_00483690 */
typedef struct ModelRenderRecordStateOps {
    void* context;
    void (*record_apply)(void* context, void* record, uint32_t parameter);
    void (*texture_stage_state_set)(void* context, uint32_t stage,
                                    uint32_t value);
} ModelRenderRecordStateOps;
void model_render_record_state_ops_set(const ModelRenderRecordStateOps* ops);
void model_render_record_state_setup(void* record, uint32_t parameter,
                                    const float* transform,
                                    int alternate_mode); /* FUN_00482ac0 */
typedef struct ModelRenderChildPassOps {
    void* context;
    uint32_t (*child_count)(void* context, void* model);
    void* (*child_get)(void* context, void* model, uint32_t index);
    uint32_t (*child_flags_get)(void* context, void* child);
    int (*child_has_material)(void* context, void* child);
    int (*child_render)(void* context, void* model, void* child,
                        uint32_t parameter, int material_pass);
} ModelRenderChildPassOps;
void model_render_child_pass_ops_set(const ModelRenderChildPassOps* ops);
int model_render_child_pass_dispatch(void* model, uint32_t parameter,
                                     int material_pass); /* FUN_004839c0 */
typedef struct Direct3DRenderStateSnapshotOps {
    void* context;
    int (*light_get)(void* context, uint32_t state, uint32_t* value_out);
    void (*light_set)(void* context, uint32_t state, uint32_t value);
    int (*render_state_get)(void* context, uint32_t state,
                            uint32_t* value_out);
    void (*render_state_set)(void* context, uint32_t state, uint32_t value);
    int (*texture_stage_get)(void* context, uint32_t stage, uint32_t* value_out);
    void (*texture_stage_set)(void* context, uint32_t stage, uint32_t value);
} Direct3DRenderStateSnapshotOps;
void direct3d_render_state_snapshot_ops_set(
    const Direct3DRenderStateSnapshotOps* ops);
void d3d_render_state_sync(void); /* FUN_004838b0 */
void direct3d_render_state_restore(void); /* FUN_00483940 */
typedef struct Direct3DDeviceStateOps {
    void* context;
    int (*texture_stage_set)(void* context, uint32_t stage, void* texture);
    int (*texture_interface_bind)(void* context, void* texture);
    int (*light_state_set)(void* context, uint32_t state, void* light);
    void (*render_state_set)(void* context, uint32_t state, uint32_t value);
    int (*primitive_submit)(void* context, uint32_t primitive_type,
                            uint32_t a, uint32_t b, uint32_t c,
                            uint32_t d, uint32_t e);
} Direct3DDeviceStateOps;
void direct3d_device_state_ops_set(const Direct3DDeviceStateOps* ops);
void direct3d_device_texture_dirty_set(int dirty);
int direct3d_device_state_apply(void* light, void* texture,
                                uint32_t a, uint32_t b, uint32_t c,
                                uint32_t d, uint32_t e); /* FUN_00483b70 */
typedef int (*ModelMaterialUpdateHook)(void* context, const void* material);
void model_material_update_hook_set(ModelMaterialUpdateHook hook,
                                    void* context);
typedef struct ModelChildMaterialOps {
    void* context;
    uint32_t (*flags_get)(void* context, void* child);
    void* (*texture_get)(void* context, void* child);
    const void* (*material_record_get)(void* context, void* child);
    uint32_t (*material_flags_get)(void* context, void* child);
    int (*render_arguments_get)(void* context, void* child,
                                uint32_t arguments_out[4]);
} ModelChildMaterialOps;
void model_child_material_ops_set(const ModelChildMaterialOps* ops);
int world_object_render_state_apply(void* child, const float* transform,
                                    int alternate_mode); /* FUN_00483b00 */
int model_render_child_material_pass(void* light, void* child,
                                     const float* transform,
                                     int alternate_mode); /* FUN_00483a90 */
typedef struct Direct3DInterfaceOps {
    void* context;
    int (*capability_check)(void* context, void* device);
    int (*interface_query)(void* context, void* device,
                           void** interface_out);
    int (*renderer_interface_attach)(void* context, void* interface_pointer);
    void* (*viewport_device_get)(void* context, void* viewport);
    void (*interface_release)(void* context, void* interface_pointer);
} Direct3DInterfaceOps;
void direct3d_interface_ops_set(const Direct3DInterfaceOps* ops);
int render_interface_capability_check(void* device); /* FUN_00483580 */
int direct3d_viewport_setup(void* viewport); /* FUN_00483600 */
typedef struct DirectDrawDeviceCreateOps {
    void* context;
    int (*device_create)(void* context, const void* guid, void** device_out);
    int (*device_interface_query)(void* context, void* device,
                                 void** interface_out);
    int (*device_enumerate)(void* context, void* device,
                            void* interface_pointer, const void* adapter,
                            int windowed, void** result_out);
    void (*release)(void* context, void* object);
} DirectDrawDeviceCreateOps;
void directdraw_device_create_ops_set(const DirectDrawDeviceCreateOps* ops);
int directdraw_device_create(const void* guid, void** result_out); /* FUN_0047c2e0 */
int directdraw_enumerated_device_create(const void* adapter,
                                        uint32_t adapter_flags,
                                        int windowed, void** result_out); /* FUN_0047c4b0 */
typedef struct DirectDrawRendererInitParams {
    int fullscreen;
    const void* adapter_guid;
    uint32_t adapter_flags;
    int mode_width;
    int mode_height;
    int mode_bits_per_pixel;
    int window_x;
    int window_y;
    int width;
    int height;
} DirectDrawRendererInitParams;
typedef struct DirectDrawRendererInitOps {
    void* context;
    /* These setup callbacks report success as nonzero. */
    int (*device_create)(void* context, const void* guid, void** device_out);
    int (*cooperative_level_set)(void* context, void* device,
                                 int fullscreen, uint32_t flags);
    int (*display_mode_set)(void* context, void* device, int width,
                            int height, int bits_per_pixel);
    int (*surfaces_create)(void* context, void* device, int fullscreen,
                           int width, int height, int bits_per_pixel,
                           void** primary_out, void** back_surface_out);
    int (*clipper_setup)(void* context, int width, int height);
    int (*direct3d_initialize)(void* context, void* device,
                               void* primary, void* back_surface,
                               int fullscreen);
    void (*release)(void* context, void* object);
    void (*renderer_state_commit)(void* context, void* device,
                                  void* primary, void* back_surface,
                                  int fullscreen);
} DirectDrawRendererInitOps;
void directdraw_renderer_init_ops_set(const DirectDrawRendererInitOps* ops);
int directdraw_renderer_init(const DirectDrawRendererInitParams* params); /* FUN_0047c610 */
typedef struct DirectDrawAdapterCandidate {
    void* adapter;
    uint32_t flags;
} DirectDrawAdapterCandidate;
typedef struct DirectDrawDisplayModeCandidate {
    void* descriptor;
    int width;
    int height;
    int bits_per_pixel;
    uint32_t flags;
} DirectDrawDisplayModeCandidate;
typedef struct DirectDrawDeviceInitializeOps {
    void* context;
    void (*mode_register)(void* context, int width, int height,
                          int bits_per_pixel);
    int (*adapter_count_get)(void* context);
    int (*adapter_get)(void* context, int index,
                       DirectDrawAdapterCandidate* adapter_out);
    int (*mode_count_get)(void* context, void* adapter, int windowed);
    int (*mode_get)(void* context, void* adapter, int index,
                    DirectDrawDisplayModeCandidate* mode_out);
    int (*mode_supported)(void* context, void* adapter,
                          const DirectDrawDisplayModeCandidate* mode,
                          int windowed);
    int (*selection_dialog)(void* context, int error_mode,
                            DirectDrawAdapterCandidate* adapter_out,
                            DirectDrawDisplayModeCandidate* mode_out,
                            int* fullscreen_out);
    void (*error_show)(void* context, const char* message);
    int (*renderer_initialize)(void* context, int fullscreen, void* adapter,
                               const DirectDrawDisplayModeCandidate* mode);
} DirectDrawDeviceInitializeOps;
void directdraw_device_initialize_ops_set(
    const DirectDrawDeviceInitializeOps* ops);
int directdraw_device_initialize(int show_dialog, int register_extra_modes,
                                 int auto_select, int windowed,
                                 const char* error_message); /* FUN_0049d030 */
typedef int (*Direct3DTextureMemoryCapsQueryFn)(void* context,
                                                uint32_t capability,
                                                uint32_t* memory_out);
void direct3d_texture_memory_query_hook_set(
    Direct3DTextureMemoryCapsQueryFn hook, void* context);
void query_texture_memory(void); /* FUN_0047cd50 */
uint32_t query_texture_memory_last_value(void);
typedef struct Direct3DRMDeviceInitOps {
    void* context;
    int (*rm_create)(void* context, void** rm_out);
    int (*direct3d_query)(void* context, void* rm, void** direct3d_out);
    int (*device_create)(void* context, void* direct3d, void* adapter,
                         void* surface, int device_mode, void** device_out);
    int (*device_initialize)(void* context, void* device);
    int (*renderer_interface_query)(void* context, void* rm, void** renderer_out);
    int (*device_display_mode_set)(void* context, void* device, int mode);
    void (*state_commit)(void* context, void* device, void* renderer,
                         int device_mode);
    void (*release)(void* context, void* object);
} Direct3DRMDeviceInitOps;
void direct3drm_device_init_ops_set(const Direct3DRMDeviceInitOps* ops);
int direct3drm_device_initialize(void* adapter, uint32_t adapter_flags,
                                 void* surface, int force_single_mode); /* FUN_004781d0 */
int direct3drm_device_mode_get(void);
uint32_t surface_render_flags_peek(void);
void renderer_record_table_initialize(void); /* FUN_0047d410 */
uint32_t renderer_record_table_word_peek(int index);
void renderer_record_table_word_set(int index, uint32_t value);
typedef void (*RendererBeginFrameHook)(void* device);
void renderer_begin_frame_hook_set(RendererBeginFrameHook hook, void* device);
int renderer_begin_frame_once(void);
void renderer_begin_frame_once_reset(void);
void surface_descriptor_initialize(void* record); /* FUN_0047e710 */
typedef void (*SurfacePixelWriteFn)(void* context, int x, int y,
                                    uint32_t pixel);
typedef struct SurfacePixelWriterTable {
    SurfacePixelWriteFn writer8;
    SurfacePixelWriteFn writer16_default;
    SurfacePixelWriteFn writer16_mode1;
    SurfacePixelWriteFn writer16_mode2;
    SurfacePixelWriteFn writer24;
    SurfacePixelWriteFn writer32;
} SurfacePixelWriterTable;
int surface_pixel_writer_select_by_format(
    uint32_t bits_per_pixel, int format,
    const SurfacePixelWriterTable* writers,
    SurfacePixelWriteFn* out_writer); /* FUN_00486690 */
int surface_lock_configure_pixel_writer(
    void* surface, int format, SurfaceLockFormatQueryFn query,
    const SurfacePixelWriterTable* writers); /* FUN_00486550 */
typedef void (*SurfaceWriterUnlockHook)(void* surface, int flags);
void surface_writer_unlock_hook_set(SurfaceWriterUnlockHook hook);
void surface_directdraw_unlock(void* surface); /* FUN_0047ded0 */
void surface_unlock_reset_pixel_writer(void* surface); /* FUN_00486650 */
SurfacePixelWriteFn surface_pixel_writer_peek(void);
typedef struct SurfaceRasterContext {
    float left;
    float top;
    float right;
    float bottom;
    SurfacePixelWriteFn write_pixel;
    void* context;
} SurfaceRasterContext;
void surface_bresenham_line_walk_clipped(
    SurfaceRasterContext* raster, int x0, int y0, int x1, int y1,
    uint32_t pixel); /* FUN_00486720 */
typedef struct SurfacePixelSpan {
    int x0;
    int y0;
    int x1;
    int y1;
} SurfacePixelSpan;
void surface_fill_pixel_spans(SurfaceRasterContext* raster,
                              const SurfacePixelSpan* spans, int count,
                              uint32_t pixel); /* FUN_00486090 */
void surface_fill_pixel_span_records(SurfaceRasterContext* raster,
                                     const SurfacePixelSpan* spans, int count,
                                     uint32_t pixel); /* FUN_00486210 */
typedef struct SurfacePoint {
    int x;
    int y;
} SurfacePoint;
void surface_draw_polyline(SurfaceRasterContext* raster,
                           const SurfacePoint* points, int count,
                           uint32_t pixel); /* FUN_00485ff0 */
void surface_draw_circle_pixels(SurfaceRasterContext* raster,
                                int center_x, int center_y, int radius,
                                uint32_t pixel); /* FUN_00486390 */

/* RGBChannelsPack (FUN_0047db90, findings 23/431): pack three channel
 * bytes into the corpus's 0x00BBGGRR word — first argument lands in the
 * low byte, matching the corpus's byte-array caller (palette entries
 * pack exactly as read: bytes 0..2 become bits 0..24). */
uint32_t rgb_channels_pack(uint8_t red, uint8_t green, uint8_t blue);

/* RGBChannelsUnpack (FUN_0047e2d0): write bytes 0, 1, and 2 of the packed
 * 0x00BBGGRR word to the optional red, green, and blue outputs. */
void rgb_channels_unpack(uint32_t color, uint8_t* red, uint8_t* green,
                         uint8_t* blue);

/* Pixel conversion records. Keep the retail field offsets on both 32-bit
 * builds and LP64 hosts; pointer-sized padding lets tests use native pointers
 * without moving the depth/masks or job palette fields. */
typedef struct SurfaceRecord {
    uint8_t reserved_00[0x08];
    int row_count;      /* +0x08 */
    uint8_t reserved_0c[0x04];
    int row_bytes;      /* +0x10 */
    uint8_t reserved_14[0x10];
    void* pixels;       /* +0x24 */
    uint8_t reserved_2c[0x54 - 0x24 - sizeof(void*)];
    int depth;          /* +0x54 */
    uint32_t mask_low;  /* +0x58 */
    uint32_t mask_mid;  /* +0x5c */
    uint32_t mask_hi;   /* +0x60 */
} SurfaceRecord;

typedef struct SurfaceConvertJob {
    int width;          /* +0x00 / param_2[0] */
    int height;         /* +0x04 / param_2[1] */
    uint8_t reserved_08[0x10];
    int src_stride;     /* +0x18 / param_2[6] */
    const void* src;    /* +0x1c / param_2[7] */
    uint8_t reserved_24[0x38 - 0x1c - sizeof(void*)];
    const void* palette;/* +0x38 / param_2[14] */
} SurfaceConvertJob;

struct SurfacePixelFormat {
    uint32_t bits_per_pixel;
    uint32_t red_mask;
    uint32_t green_mask;
    uint32_t blue_mask;
    const uint32_t* palette_rgb; /* 256 entries, 0x00RRGGBB */
};
uint32_t surface_pixel_value_encode(const SurfacePixelFormat* format,
                                    uint32_t color_rgb); /* FUN_0047d2d0 */
typedef int (*ProjectionOverlayBlitHook)(void* context,
                                         const int rectangle[4],
                                         uint32_t fill_color,
                                         uint32_t flags);
void projection_overlay_blit_hook_set(ProjectionOverlayBlitHook hook,
                                      void* context);
int projection_overlay_draw_submit(int has_rectangle,
                                    const int rectangle[4],
                                    const SurfacePixelFormat* format,
                                    uint32_t color_rgb); /* FUN_0047ce20 */
typedef struct DirectDrawClipperRegionOps {
    void* context;
    int (*clipper_get)(void* context, void** clipper_out);
    int (*region_data_create)(void* context, int width, int height,
                              void** data_out, uint32_t* size_out);
    int (*clipper_region_set)(void* context, void* clipper,
                              const void* data, uint32_t size);
    int (*surface_clipper_set)(void* context, void* clipper);
    void (*region_data_release)(void* context, void* data);
    void (*clipper_release)(void* context, void* clipper);
} DirectDrawClipperRegionOps;
void directdraw_clipper_region_ops_set(const DirectDrawClipperRegionOps* ops);
int directdraw_clipper_set_region(int width, int height); /* FUN_0047cee0 */

/* SurfacePixelRowsFlipWithScratch (FUN_0047D740): pair-flip the rows
 * through an engine-heap scratch, rounding each row to the even half. */
void surface_pixel_rows_flip_with_scratch(SurfaceRecord* rec);

/* Palette8To16BitSurfaceExpand (FUN_0047D520): the indexed walk — each
 * palette entry's bytes land through their mask's field ((byte >>
 * (8 - popcount)) << first-set), then the rows flip. 16-bit targets only;
 * the other depths return the corpus's verdict without work. */
int surface_palette8_to_16bit_surface_expand(SurfaceRecord* rec,
                                             const SurfaceConvertJob* job);
typedef struct PalettizedSurfaceDecodeJob {
    const uint8_t* source;
    uint32_t source_stride;
    const uint32_t* palette_rgb;
    uint16_t* destination;
    uint32_t destination_stride_pixels;
    uint32_t width;
    uint32_t height;
    uint32_t red_mask;
    uint32_t green_mask;
    uint32_t blue_mask;
} PalettizedSurfaceDecodeJob;
int palettized_bitmap_surface_decode(
    const PalettizedSurfaceDecodeJob* job);

/* Bitmap24BitSurfaceConvert (FUN_0047D800): the 24-bit sibling — the three
 * source bytes per pixel land through the masks REVERSED (byte 0 takes
 * mask_hi's field, byte 2 mask_low's), then the rows flip. */
int surface_bitmap24_bit_surface_convert(SurfaceRecord* rec,
                                         const SurfaceConvertJob* job);
int surface_pixel_format_query_convert(SurfaceRecord* rec,
                                       const SurfaceConvertJob* job,
                                       uint32_t source_bits_per_pixel);

/* SurfaceColorPackToPixel (FUN_004864d0): convert three 0..255 color
 * channels to the currently locked surface's pixel format. */
uint32_t surface_color_pack_to_pixel(float red, float green, float blue);

/* FontDeviceColorConvert (FUN_0047e190): query the device pixel format,
 * pack the low three source color bytes into its RGB masks, then release
 * the queried format interface. */
typedef int (*FontPixelFormatQueryFn)(void* device, uint32_t* bits_per_pixel,
                                      uint32_t* red_mask,
                                      uint32_t* green_mask,
                                      uint32_t* blue_mask);
typedef void (*FontPixelFormatReleaseFn)(void* device);
void font_pixel_format_hooks_set(FontPixelFormatQueryFn query,
                                 FontPixelFormatReleaseFn release);
uint32_t font_device_color_convert(void* device, uint32_t color);
/* Parse the A###_ surface naming prefix from a path's final component. */
int parse_indexed_surface_name(const char* path, uint32_t* index_out);
/* FontRecordAndSurfaceRelease (FUN_0047a550): release the atlas surface in
 * field zero and return the large atlas record to its dedicated free list. */
void font_atlas_record_release(void* record);

/* SurfacePixelRowsFlipInPlace (FUN_00476bf0): vertically flip a locked
 * renderer surface using its descriptor and engine heap scratch storage. */
void surface_pixel_rows_flip_in_place(void* surface);

#endif /* LEGORRR_SURFACE_HPP */
