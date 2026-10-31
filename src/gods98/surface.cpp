// SPDX-License-Identifier: LGPL-3.0-or-later
#include <string.h>

#include "platform/platform.h"
#include "surface.hpp"

#include "crt/crt.hpp"
#include "heap.hpp"
#include "texture.hpp"
#include "view.hpp"
#include "frameloop.hpp"

static void* mesh_record_default_allocate(void*, size_t bytes)
{
    return engine_heap_alloc((long)bytes);
}

static void mesh_record_default_release(void*, void* memory)
{
    engine_heap_free(memory);
}

static MeshRecordAppendOps s_mesh_record_append_ops;
static ModelTextureBindOps s_model_texture_bind_ops;
static MaterialPassOps s_material_pass_ops;

int mesh_record_allocate_or_append_ops_set(const MeshRecordAppendOps* ops)
{
    if (ops == NULL) {
        memset(&s_mesh_record_append_ops, 0,
               sizeof(s_mesh_record_append_ops));
        return 1;
    }
    s_mesh_record_append_ops = *ops;
    return 1;
}

/* FUN_00482220: append a zeroed render record, copy every other source
 * index word, allocate its vertex payload, then publish the new slot. */
int mesh_record_allocate_and_fill(MeshRecordPool* pool,
                                  uint32_t vertex_count,
                                  uint32_t index_rows,
                                  uint32_t index_columns,
                                  const uint16_t* source_indices)
{
    void* (*allocate)(void*, size_t);
    void (*release)(void*, void*);
    uint32_t index_count;
    uint32_t index;
    size_t slot_bytes;
    size_t index_bytes;
    size_t vertex_bytes;
    MeshRenderRecord* records;
    MeshRenderRecord* record;
    uint32_t i;

    if (pool == NULL || (index_rows != 0 &&
        index_columns > 0xffffffffu / index_rows)) return -1;
    index_count = index_rows * index_columns;
    if (index_count != 0 && source_indices == NULL) return -1;
    if (vertex_count > (size_t)-1 / 32u ||
        index_count > (size_t)-1 / sizeof(uint16_t)) return -1;
    allocate = pool->allocate != NULL ? pool->allocate :
                                         mesh_record_default_allocate;
    release = pool->release != NULL ? pool->release :
                                       mesh_record_default_release;

    if (pool->records == NULL) {
        pool->capacity = 10;
        pool->count = 1;
        slot_bytes = (size_t)pool->capacity * sizeof(MeshRenderRecord);
        pool->records = (MeshRenderRecord*)allocate(pool->context, slot_bytes);
        if (pool->records == NULL) {
            pool->count = 0;
            pool->capacity = 0;
            return -1;
        }
    }
    else {
        index = pool->count;
        pool->count++;
        if (index == pool->capacity) {
            uint32_t new_capacity = (pool->capacity * 150u) / 100u;
            MeshRenderRecord* grown;
            if (new_capacity <= pool->capacity) new_capacity = pool->capacity + 1;
            slot_bytes = (size_t)new_capacity * sizeof(MeshRenderRecord);
            grown = (MeshRenderRecord*)allocate(pool->context, slot_bytes);
            if (grown == NULL) return -1;
            memcpy(grown, pool->records,
                   (size_t)pool->capacity * sizeof(MeshRenderRecord));
            release(pool->context, pool->records);
            pool->records = grown;
            pool->capacity = new_capacity;
        }
    }

    index = pool->count - 1;
    record = &pool->records[index];
    memset(record, 0, sizeof(*record));
    index_bytes = (size_t)index_count * sizeof(uint16_t);
    record->indices = index_bytes != 0 ?
        (uint16_t*)allocate(pool->context, index_bytes) : NULL;
    if (index_bytes != 0 && record->indices == NULL) return -1;
    record->index_count = index_count;
    for (i = 0; i < index_count; ++i)
        record->indices[i] = source_indices[i * 2u];

    vertex_bytes = (size_t)vertex_count * 32u;
    record->vertices = vertex_bytes != 0 ?
        allocate(pool->context, vertex_bytes) : NULL;
    if (vertex_bytes != 0 && record->vertices == NULL) {
        if (record->indices != NULL) release(pool->context, record->indices);
        memset(record, 0, sizeof(*record));
        return -1;
    }
    if (record->vertices != NULL) memset(record->vertices, 0, vertex_bytes);
    record->vertex_count = vertex_count;
    if (pool->record_registered != NULL)
        pool->record_registered(pool->context, index);
    return (int)index;
}

/* MeshRecordAllocateOrAppend (FUN_004748f0): use the in-memory mesh pool
 * when present; otherwise route through the legacy Direct3DRM record path. */
int mesh_record_allocate_or_append(void* object, uint32_t vertex_count,
                                   uint32_t index_rows,
                                   uint32_t index_columns,
                                   const uint16_t* source_indices)
{
    MeshRecordPool* pool = NULL;
    if (object == NULL) return -1;
    if (s_mesh_record_append_ops.pool_get != NULL) {
        pool = s_mesh_record_append_ops.pool_get(
            s_mesh_record_append_ops.context, object);
    }
    if (pool != NULL) {
        return mesh_record_allocate_and_fill(pool, vertex_count, index_rows,
                                             index_columns, source_indices);
    }
    if (s_mesh_record_append_ops.legacy_append != NULL) {
        return s_mesh_record_append_ops.legacy_append(
            s_mesh_record_append_ops.context, object, vertex_count,
            index_rows, index_columns, source_indices);
    }
    return -1;
}

void model_texture_bind_ops_set(const ModelTextureBindOps* ops)
{
    if (ops != NULL) s_model_texture_bind_ops = *ops;
    else memset(&s_model_texture_bind_ops, 0,
                sizeof(s_model_texture_bind_ops));
}

/* BindModelTextures (FUN_00481820): choose source colors versus texture
 * binding per input, expand numbered texture sequences (up to 100 frames),
 * then copy each input's render parameters into its binding record. */
int bind_model_textures(void* device, const ModelTextureInput* inputs,
                        ModelTextureBinding* bindings, uint32_t count,
                        int force_source_color)
{
    uint32_t i;
    if (count != 0u && (inputs == NULL || bindings == NULL)) return 0;
    for (i = 0u; i < count; ++i) {
        const ModelTextureInput* input = &inputs[i];
        ModelTextureBinding* binding = &bindings[i];
        memset(binding, 0, sizeof(*binding));
        binding->texture = NULL;
        binding->animation_frames = NULL;
        binding->animation_frame_count = 0u;
        if (input->image_resource == NULL || force_source_color != 0) {
            binding->color[0] = (float)input->color[0] / 256.0f;
            binding->color[1] = (float)input->color[1] / 256.0f;
            binding->color[2] = (float)input->color[2] / 256.0f;
        }
        else {
            char name[260];
            uint32_t first_frame = 0u;
            int animated = 0;
            if (s_model_texture_bind_ops.name_get != NULL &&
                s_model_texture_bind_ops.name_get(
                    s_model_texture_bind_ops.context, input->image_resource,
                    0u, name, sizeof(name), &first_frame, &animated)) {
                if (input->animated_name == 0u || animated == 0) {
                    if (s_model_texture_bind_ops.texture_get_or_create != NULL)
                        binding->texture =
                            s_model_texture_bind_ops.texture_get_or_create(
                                s_model_texture_bind_ops.context, device, name);
                }
                else if (s_model_texture_bind_ops.allocate != NULL &&
                         s_model_texture_bind_ops.texture_get_or_create != NULL) {
                    uint32_t frame;
                    binding->animation_frames = (void**)
                        s_model_texture_bind_ops.allocate(
                            s_model_texture_bind_ops.context,
                            100u * sizeof(void*));
                    if (binding->animation_frames != NULL) {
                        for (frame = 0u; frame < 100u; ++frame) {
                            uint32_t first = first_frame + frame;
                            int still_animated = 1;
                            char frame_name[260];
                            void* texture;
                            if (!s_model_texture_bind_ops.name_get(
                                    s_model_texture_bind_ops.context,
                                    input->image_resource, first, frame_name,
                                    sizeof(frame_name), &first, &still_animated))
                                break;
                            texture =
                                s_model_texture_bind_ops.texture_get_or_create(
                                    s_model_texture_bind_ops.context, device,
                                    frame_name);
                            if (texture == NULL) break;
                            binding->animation_frames[frame] = texture;
                            binding->animation_frame_count++;
                        }
                        if (binding->animation_frame_count == 0u) {
                            if (s_model_texture_bind_ops.release != NULL)
                                s_model_texture_bind_ops.release(
                                    s_model_texture_bind_ops.context,
                                    binding->animation_frames);
                            binding->animation_frames = NULL;
                        }
                    }
                }
            }
            if (input->color_from_source != 0u) {
                binding->color[0] = (float)input->color[0] / 256.0f;
                binding->color[1] = (float)input->color[1] / 256.0f;
                binding->color[2] = (float)input->color[2] / 256.0f;
            }
            else {
                binding->color[0] = 1.0f;
                binding->color[1] = 1.0f;
                binding->color[2] = 1.0f;
            }
        }
        memcpy(binding->values, input->values, sizeof(input->values));
    }
    return 1;
}

void material_pass_ops_set(const MaterialPassOps* ops)
{
    if (ops != NULL) s_material_pass_ops = *ops;
    else memset(&s_material_pass_ops, 0, sizeof(s_material_pass_ops));
}

/* MaterialPass (FUN_00480ac0): prepare expanded layer geometry, then bind
 * each material pass, apply its state, submit it, and release the temporary
 * geometry on both success and failure. */
int material_pass_execute(void* device, void* model, void* layer)
{
    void* geometry = NULL;
    uint32_t pass_count = 0u;
    uint32_t pass;
    int result = 0;
    if (s_material_pass_ops.geometry_prepare == NULL || layer == NULL)
        return 0;
    if (!s_material_pass_ops.geometry_prepare(
            s_material_pass_ops.context, device, model, layer,
            &geometry, &pass_count)) {
        if (geometry != NULL && s_material_pass_ops.geometry_release != NULL)
            s_material_pass_ops.geometry_release(s_material_pass_ops.context,
                                                 geometry);
        return 0;
    }
    result = 1;
    for (pass = 0u; pass < pass_count; ++pass) {
        if (s_material_pass_ops.texture_bind != NULL &&
            !s_material_pass_ops.texture_bind(s_material_pass_ops.context,
                                               device, geometry, pass)) {
            result = 0;
            break;
        }
        if (s_material_pass_ops.material_state_apply != NULL &&
            !s_material_pass_ops.material_state_apply(
                s_material_pass_ops.context, device, geometry, pass)) {
            result = 0;
            break;
        }
        if (s_material_pass_ops.geometry_submit != NULL &&
            !s_material_pass_ops.geometry_submit(s_material_pass_ops.context,
                                                  device, geometry, pass)) {
            result = 0;
            break;
        }
    }
    if (geometry != NULL && s_material_pass_ops.geometry_release != NULL)
        s_material_pass_ops.geometry_release(s_material_pass_ops.context,
                                             geometry);
    return result;
}

// GLOBAL: LEGORR 0x00540814
static uint32_t s_surface_pixel_bits = 0;
// GLOBAL: LEGORR 0x00540824
static uint32_t s_surface_red_mask_bits = 0;
// GLOBAL: LEGORR 0x00540828
static uint32_t s_surface_green_mask_bits = 0;
// GLOBAL: LEGORR 0x0054082c
static uint32_t s_surface_blue_mask_bits = 0;
// GLOBAL: LEGORR 0x00540830
static uint32_t s_surface_render_flags = 0;
typedef struct RendererRecordTableState {
    uint32_t words[32];
    uint32_t reserved[2];
    uint32_t enabled;
} RendererRecordTableState;
// GLOBAL: LEGORR 0x00533908
static RendererRecordTableState s_renderer_record_table;

static FontPixelFormatQueryFn s_font_pixel_format_query;
static FontPixelFormatReleaseFn s_font_pixel_format_release;
static void* s_font_atlas_record_free_list;
static SurfacePixelBitsQueryFn s_surface_pixel_bits_query;
static SurfacePixelWriteFn s_surface_pixel_writer;
static SurfaceWriterUnlockHook s_surface_writer_unlock_hook;
static RendererBeginFrameHook s_renderer_begin_frame_hook;
static void* s_renderer_begin_frame_device;
static int s_renderer_frame_started;
static RenderStateApplyOps s_render_state_apply_ops;
static uint32_t s_d3d_material_state_saved[200];
static uint8_t s_d3d_material_state_dirty[200];
static RendererWindowModeOps s_renderer_window_mode_ops;
static int s_renderer_window_width;
static int s_renderer_window_height;
static uint32_t s_renderer_window_mode_flags;
static DirectDrawEnumerationHook s_directdraw_enumeration_hook;
static void* s_directdraw_enumeration_context;
static Direct3DRenderPreferenceApplyHook s_render_preference_apply_hook;
static void* s_render_preference_apply_context;
static Direct3DMatrixOps s_direct3d_matrix_ops;
static ModelRenderRecordStateOps s_model_render_record_state_ops;
static ModelRenderChildPassOps s_model_render_child_pass_ops;
static ModelRenderPassDispatchOps s_model_render_pass_dispatch_ops;
static ProjectionOverlayBlitHook s_projection_overlay_blit_hook;
static void* s_projection_overlay_blit_context;
static DirectDrawClipperRegionOps s_directdraw_clipper_region_ops;
static Direct3DRenderStateSnapshotOps s_d3d_render_state_snapshot_ops;
static uint32_t s_d3d_snapshot_light;
static uint32_t s_d3d_snapshot_render;
static uint32_t s_d3d_snapshot_texture_stage;
static Direct3DDeviceStateOps s_direct3d_device_state_ops;
static void* s_direct3d_cached_texture;
static void* s_direct3d_cached_light;
static int s_direct3d_device_texture_dirty;
static ModelMaterialUpdateHook s_model_material_update_hook;
static void* s_model_material_update_context;
static ModelChildMaterialOps s_model_child_material_ops;
static Direct3DInterfaceOps s_direct3d_interface_ops;
static DirectDrawDeviceCreateOps s_directdraw_device_create_ops;
static DirectDrawRendererInitOps s_directdraw_renderer_init_ops;
static Direct3DTextureMemoryCapsQueryFn s_texture_memory_caps_query;
static void* s_texture_memory_caps_context;
static uint32_t s_texture_memory_last_value;
static Direct3DRMDeviceInitOps s_direct3drm_device_init_ops;
static int s_direct3drm_device_mode = 1;
static DirectDrawDeviceInitializeOps s_directdraw_device_initialize_ops;

void render_state_apply_hooks_set(const RenderStateApplyOps* ops)
{
    if (ops == NULL) {
        memset(&s_render_state_apply_ops, 0, sizeof(s_render_state_apply_ops));
    }
    else {
        s_render_state_apply_ops = *ops;
    }
}

void d3d_material_state_cache_snapshot_store(uint32_t state, uint32_t value)
{
    if (state >= 200u) return;
    if (s_d3d_material_state_dirty[state] == 0u) {
        s_d3d_material_state_saved[state] = value;
        s_d3d_material_state_dirty[state] = 1u;
    }
}

/* Direct3DRenderStateSetCached (FUN_004788d0): read the current state,
 * remember its entry value once, and apply a changed value. Returning to the
 * saved value ends the cache interval. */
// FUNCTION: LEGORR 0x004788d0
void direct3d_render_state_set_cached(uint32_t state, uint32_t value)
{
    uint32_t current = 0u;
    int have_current = 0;
    if (state < 200u && s_render_state_apply_ops.get_state != NULL) {
        have_current = (s_render_state_apply_ops.get_state(
            s_render_state_apply_ops.context, state, &current) == 0);
    }
    if (state < 200u && s_d3d_material_state_dirty[state] != 0u &&
        value == s_d3d_material_state_saved[state]) {
        if (s_render_state_apply_ops.set_state != NULL &&
            (!have_current || current != value)) {
            s_render_state_apply_ops.set_state(
                s_render_state_apply_ops.context, state, value);
        }
        s_d3d_material_state_dirty[state] = 0u;
        return;
    }
    if (state < 200u && s_d3d_material_state_dirty[state] == 0u &&
        have_current && current != value) {
        d3d_material_state_cache_snapshot_store(state, current);
    }
    if (s_render_state_apply_ops.set_state != NULL &&
        (!have_current || current != value)) {
        s_render_state_apply_ops.set_state(
            s_render_state_apply_ops.context, state, value);
    }
}

/* D3DMaterialStateFlush (FUN_00478940): restore every saved render-state
 * value in the 200-entry cache and clear its dirty bit. */
// FUNCTION: LEGORR 0x00478940
void d3d_material_state_flush(void)
{
    uint32_t state;
    if (s_render_state_apply_ops.set_state == NULL) return;
    for (state = 0u; state < 200u; ++state) {
        if (s_d3d_material_state_dirty[state] != 0u) {
            s_render_state_apply_ops.set_state(
                s_render_state_apply_ops.context, state,
                s_d3d_material_state_saved[state]);
            s_d3d_material_state_dirty[state] = 0u;
        }
    }
}

/* RendererCommandModeAndPairSend (FUN_00482cb0) changes the mode state and
 * the two packed render-state values as one logical command. */
// FUNCTION: LEGORR 0x00482cb0
void renderer_command_mode_and_pair_send(uint32_t first, uint32_t second)
{
    RenderStateSetHook set_state = s_render_state_apply_ops.set_state;
    if (set_state == NULL) return;
    direct3d_render_state_set_cached(0x1bu, 1u);
    direct3d_render_state_set_cached(0x13u, first);
    direct3d_render_state_set_cached(0x14u, second);
}

/* RenderStateFlagsApply (FUN_00482b50) translates the material flags into
 * the D3D state sequence. Device-specific state and matrix operations are
 * supplied by the renderer seam. */
// FUNCTION: LEGORR 0x00482b50
void render_state_flags_apply(uint32_t flags, const float* transform,
                              int alternate_mode)
{
    RenderStateSetHook set_state = s_render_state_apply_ops.set_state;
    void* context = s_render_state_apply_ops.context;
    uint32_t mode_flags = flags & 0x1c00u;
    uint32_t value;
    float default_transform[16];
    const float* selected_transform = transform;

    if (set_state == NULL) return;
    if (mode_flags != 0u && alternate_mode != 0) {
        direct3d_render_state_set_cached(0x1cu, 0u);
    }
    if (mode_flags == 0u || alternate_mode != 0) {
        if ((flags & 0x400u) != 0u) {
            renderer_command_mode_and_pair_send(2u, 2u);
        }
        else if ((flags & 0x800u) != 0u) {
            renderer_command_mode_and_pair_send(5u, 2u);
        }
        else if ((flags & 0x1000u) != 0u) {
            renderer_command_mode_and_pair_send(5u, 6u);
        }
        else {
            direct3d_render_state_set_cached(0x1bu, 0u);
        }
    }
    else {
        direct3d_render_state_set_cached(0x1bu, 0u);
    }

    value = ((flags & 0x4000000u) != 0u) ? 1u : 3u;
    direct3d_render_state_set_cached(0x16u, value);
    if (s_render_state_apply_ops.render_flag_test == NULL ||
        !s_render_state_apply_ops.render_flag_test(context, 0x20u)) {
        if ((flags & 0x20000u) != 0u) value = 2u;
        else if ((flags & 0x40000u) != 0u) value = 3u;
        else value = 4u;
        direct3d_render_state_set_cached(4u, value);
    }
    direct3d_render_state_set_cached(7u, 1u);
    value = (mode_flags != 0u && alternate_mode == 0) ? 0u : 1u;
    direct3d_render_state_set_cached(0xeu, value);

    if ((flags & 0x2000u) == 0u && (flags & 0x4000u) != 0u) {
        if (s_render_state_apply_ops.texture_transform_default != NULL &&
            s_render_state_apply_ops.texture_transform_default(
                context, default_transform)) {
            selected_transform = default_transform;
        }
    }
    if ((flags & 0x2000u) != 0u || (flags & 0x4000u) != 0u) {
        if (s_render_state_apply_ops.texture_transform_apply != NULL) {
            s_render_state_apply_ops.texture_transform_apply(
                context, 1, selected_transform);
        }
    }
    if ((flags & 0x800000u) != 0u) {
        direct3d_render_state_set_cached(0x11u, 1u);
        direct3d_render_state_set_cached(0x12u, 1u);
    }
    else {
        direct3d_render_state_set_cached(0x11u, 2u);
        direct3d_render_state_set_cached(0x12u, 4u);
    }
}

void renderer_window_mode_ops_set(const RendererWindowModeOps* ops)
{
    if (ops != NULL) s_renderer_window_mode_ops = *ops;
    else memset(&s_renderer_window_mode_ops, 0,
                sizeof(s_renderer_window_mode_ops));
}

void directdraw_enumeration_hook_set(DirectDrawEnumerationHook hook,
                                     void* context)
{
    s_directdraw_enumeration_hook = hook;
    s_directdraw_enumeration_context = context;
}

/* DirectDrawEnumerateAndStoreCallbackResult (FUN_0047c1c0): enumerate using
 * the requested adapter identifier and return the callback's stored result. */
// FUNCTION: LEGORR 0x0047c1c0
int directdraw_enumerate_and_store_result(const void* guid, void** result_out)
{
    void* result = NULL;
    if (s_directdraw_enumeration_hook != NULL) {
        s_directdraw_enumeration_hook(s_directdraw_enumeration_context,
                                      guid, &result);
    }
    if (result_out != NULL) *result_out = result;
    return 1;
}

void direct3d_render_preference_hook_set(
    Direct3DRenderPreferenceApplyHook hook, void* context)
{
    s_render_preference_apply_hook = hook;
    s_render_preference_apply_context = context;
}

/* Direct3DRenderPreferencesApply (FUN_00478330): map the requested mode and
 * three enable flags to the device's preference and render-mode values. */
// FUNCTION: LEGORR 0x00478330
void direct3d_render_preferences_apply(uint32_t mode, uint32_t parameter,
                                       int enabled, int secondary,
                                       int high_quality)
{
    uint32_t preference = 0x40u;
    uint32_t render_mode;
    if (mode == 1u) preference = 0x80u;
    else if (mode == 2u) preference = 0x88u;
    else if (mode == 3u) preference = 0x89u;
    if (enabled == 0) render_mode = 0u;
    else if (secondary == 0) render_mode = 1u;
    else {
        render_mode = (high_quality != 0) ? 5u : 3u;
        s_renderer_window_mode_flags |= 8u;
    }
    if (s_render_preference_apply_hook != NULL) {
        s_render_preference_apply_hook(s_render_preference_apply_context,
                                       parameter, preference, render_mode);
    }
}

void direct3d_matrix_ops_set(const Direct3DMatrixOps* ops)
{
    if (ops != NULL) s_direct3d_matrix_ops = *ops;
    else memset(&s_direct3d_matrix_ops, 0,
                sizeof(s_direct3d_matrix_ops));
}

/* MatrixMethodPairApply (FUN_00483690): compare the requested 4x4 transform
 * against the device's current transform and issue the update only on change. */
// FUNCTION: LEGORR 0x00483690
int matrix_method_pair_apply(uint32_t method, const float matrix[16])
{
    float current[16];
    int current_available = 0;
    int index;
    int changed = 0;
    if (matrix == NULL) return 0;
    memset(current, 0, sizeof(current));
    if (s_direct3d_matrix_ops.get_transform != NULL) {
        current_available = (s_direct3d_matrix_ops.get_transform(
            s_direct3d_matrix_ops.context, method, current) == 0);
    }
    if (!current_available) changed = 1;
    else {
        for (index = 0; index < 16; ++index) {
            if (current[index] != matrix[index]) {
                changed = 1;
                break;
            }
        }
    }
    if (changed) {
        if (s_direct3d_matrix_ops.set_transform == NULL ||
            s_direct3d_matrix_ops.set_transform(
                s_direct3d_matrix_ops.context, method, matrix) != 0) return 0;
    }
    return 1;
}

void model_render_record_state_ops_set(const ModelRenderRecordStateOps* ops)
{
    if (ops != NULL) s_model_render_record_state_ops = *ops;
    else memset(&s_model_render_record_state_ops, 0,
                sizeof(s_model_render_record_state_ops));
}

/* ModelRenderRecordStateSetup (FUN_00482ac0): invoke the record callback,
 * establish the baseline render states and texture-stage defaults, then
 * apply the record's material flags. */
// FUNCTION: LEGORR 0x00482ac0
void model_render_record_state_setup(void* record, uint32_t parameter,
                                     const float* transform,
                                     int alternate_mode)
{
    uint32_t flags = 0u;
    if (record == NULL) return;
    if (s_model_render_record_state_ops.record_apply != NULL) {
        s_model_render_record_state_ops.record_apply(
            s_model_render_record_state_ops.context, record, parameter);
    }
    memcpy(&flags, (const char*)record + 0x18, sizeof(flags));
    if (s_render_state_apply_ops.set_state != NULL) {
        direct3d_render_state_set_cached(0x1du, 0u);
        direct3d_render_state_set_cached(9u, 2u);
        direct3d_render_state_set_cached(0x1au, 1u);
    }
    if (s_render_state_apply_ops.render_flag_test == NULL ||
        !s_render_state_apply_ops.render_flag_test(
            s_render_state_apply_ops.context, 0x20u)) {
        if (s_model_render_record_state_ops.texture_stage_state_set != NULL) {
            s_model_render_record_state_ops.texture_stage_state_set(
                s_model_render_record_state_ops.context, 2u, 2u);
            s_model_render_record_state_ops.texture_stage_state_set(
                s_model_render_record_state_ops.context, 3u, 0u);
            s_model_render_record_state_ops.texture_stage_state_set(
                s_model_render_record_state_ops.context, 5u, 2u);
            s_model_render_record_state_ops.texture_stage_state_set(
                s_model_render_record_state_ops.context, 6u, 0u);
        }
    }
    render_state_flags_apply(flags, transform, alternate_mode);
}

void model_render_child_pass_ops_set(const ModelRenderChildPassOps* ops)
{
    if (ops != NULL) s_model_render_child_pass_ops = *ops;
    else memset(&s_model_render_child_pass_ops, 0,
                sizeof(s_model_render_child_pass_ops));
}

/* ModelRenderChildPassDispatch (FUN_004839c0): render eligible children in
 * two ordered passes, first those without a material record, then those with
 * one. A failed child marks the aggregate false while the walk continues. */
// FUNCTION: LEGORR 0x004839c0
int model_render_child_pass_dispatch(void* model, uint32_t parameter,
                                     int material_pass)
{
    uint32_t count;
    uint32_t index;
    int pass;
    int all_succeeded = 1;
    if (model == NULL || s_model_render_child_pass_ops.child_count == NULL ||
        s_model_render_child_pass_ops.child_get == NULL) return all_succeeded;
    count = s_model_render_child_pass_ops.child_count(
        s_model_render_child_pass_ops.context, model);
    for (pass = 0; pass < 2; ++pass) {
        for (index = 0u; index < count; ++index) {
            void* child = s_model_render_child_pass_ops.child_get(
                s_model_render_child_pass_ops.context, model, index);
            uint32_t flags;
            int has_material;
            if (child == NULL ||
                s_model_render_child_pass_ops.child_flags_get == NULL ||
                s_model_render_child_pass_ops.child_has_material == NULL ||
                s_model_render_child_pass_ops.child_render == NULL) continue;
            flags = s_model_render_child_pass_ops.child_flags_get(
                s_model_render_child_pass_ops.context, child);
            if ((flags & 0x10000001u) != 0u) continue;
            has_material = s_model_render_child_pass_ops.child_has_material(
                s_model_render_child_pass_ops.context, child);
            if ((pass == 0 && has_material != 0) ||
                (pass != 0 && has_material == 0)) continue;
            if (s_model_render_child_pass_ops.child_render(
                    s_model_render_child_pass_ops.context, model, child,
                    parameter, material_pass) == 0) {
                all_succeeded = 0;
            }
        }
    }
    return all_succeeded;
}

void direct3d_render_state_snapshot_ops_set(
    const Direct3DRenderStateSnapshotOps* ops)
{
    if (ops != NULL) s_d3d_render_state_snapshot_ops = *ops;
    else memset(&s_d3d_render_state_snapshot_ops, 0,
                sizeof(s_d3d_render_state_snapshot_ops));
}

/* D3DRenderStateSync (FUN_004838b0): snapshot light/render state 1 and,
 * outside the alternate renderer mode, texture-stage state 0. Temporarily
 * clear render state 1 when it was set. */
// FUNCTION: LEGORR 0x004838b0
void d3d_render_state_sync(void)
{
    uint32_t value = 0u;
    if (s_d3d_render_state_snapshot_ops.light_get != NULL &&
        s_d3d_render_state_snapshot_ops.light_get(
            s_d3d_render_state_snapshot_ops.context, 1u, &value) == 0) {
        s_d3d_snapshot_light = value;
    }
    if ((s_renderer_window_mode_flags & 0x20u) == 0u) {
        value = 0u;
        if (s_d3d_render_state_snapshot_ops.texture_stage_get != NULL &&
            s_d3d_render_state_snapshot_ops.texture_stage_get(
                s_d3d_render_state_snapshot_ops.context, 0u, &value) == 0) {
            s_d3d_snapshot_texture_stage = value;
        }
        value = 0u;
        if (s_d3d_render_state_snapshot_ops.render_state_get != NULL &&
            s_d3d_render_state_snapshot_ops.render_state_get(
                s_d3d_render_state_snapshot_ops.context, 1u, &value) == 0) {
            s_d3d_snapshot_render = value;
            if (value != 0u &&
                s_d3d_render_state_snapshot_ops.render_state_set != NULL) {
                s_d3d_render_state_snapshot_ops.render_state_set(
                    s_d3d_render_state_snapshot_ops.context, 1u, 0u);
            }
        }
    }
    else if (s_d3d_render_state_snapshot_ops.render_state_get != NULL) {
        value = 0u;
        if (s_d3d_render_state_snapshot_ops.render_state_get(
                s_d3d_render_state_snapshot_ops.context, 1u, &value) == 0) {
            s_d3d_snapshot_render = value;
        }
    }
}

/* Direct3DRenderStateRestore (FUN_00483940): restore the captured device
 * states only where the current values differ, respecting alternate mode. */
// FUNCTION: LEGORR 0x00483940
void direct3d_render_state_restore(void)
{
    uint32_t current = 0u;
    if (s_d3d_render_state_snapshot_ops.light_get != NULL &&
        s_d3d_render_state_snapshot_ops.light_get(
            s_d3d_render_state_snapshot_ops.context, 1u, &current) == 0 &&
        current != s_d3d_snapshot_light &&
        s_d3d_render_state_snapshot_ops.light_set != NULL) {
        s_d3d_render_state_snapshot_ops.light_set(
            s_d3d_render_state_snapshot_ops.context, 1u,
            s_d3d_snapshot_light);
    }
    if ((s_renderer_window_mode_flags & 0x20u) == 0u) {
        current = 0u;
        if (s_d3d_render_state_snapshot_ops.texture_stage_get != NULL &&
            s_d3d_render_state_snapshot_ops.texture_stage_get(
                s_d3d_render_state_snapshot_ops.context, 0u, &current) == 0 &&
            current != s_d3d_snapshot_texture_stage &&
            s_d3d_render_state_snapshot_ops.texture_stage_set != NULL) {
            s_d3d_render_state_snapshot_ops.texture_stage_set(
                s_d3d_render_state_snapshot_ops.context, 0u,
                s_d3d_snapshot_texture_stage);
        }
        if (s_d3d_snapshot_render != 0u &&
            s_d3d_render_state_snapshot_ops.render_state_set != NULL) {
            s_d3d_render_state_snapshot_ops.render_state_set(
                s_d3d_render_state_snapshot_ops.context, 1u,
                s_d3d_snapshot_render);
        }
    }
    else if (s_d3d_render_state_snapshot_ops.render_state_set != NULL) {
        s_d3d_render_state_snapshot_ops.render_state_set(
            s_d3d_render_state_snapshot_ops.context, 1u,
            s_d3d_snapshot_render);
    }
}

void direct3d_device_state_ops_set(const Direct3DDeviceStateOps* ops)
{
    if (ops != NULL) s_direct3d_device_state_ops = *ops;
    else memset(&s_direct3d_device_state_ops, 0,
                sizeof(s_direct3d_device_state_ops));
}

void direct3d_device_texture_dirty_set(int dirty)
{
    s_direct3d_device_texture_dirty = (dirty != 0) ? 1 : 0;
}

/* Direct3DDeviceStateApply (FUN_00483b70): bind the selected texture path,
 * update light state 1 when its object changes, and submit the primitive. */
// FUNCTION: LEGORR 0x00483b70
int direct3d_device_state_apply(void* light, void* texture,
                                uint32_t a, uint32_t b, uint32_t c,
                                uint32_t d, uint32_t e)
{
    int result = 1;
    if ((s_renderer_window_mode_flags & 0x20u) == 0u) {
        if (texture != s_direct3d_cached_texture) {
            if (s_direct3d_device_state_ops.texture_stage_set == NULL ||
                s_direct3d_device_state_ops.texture_stage_set(
                    s_direct3d_device_state_ops.context, 0u, texture) != 0) {
                result = 0;
            }
            else {
                s_direct3d_cached_texture = texture;
            }
        }
    }
    else {
        if (texture != NULL &&
            (s_direct3d_device_state_ops.texture_interface_bind == NULL ||
             s_direct3d_device_state_ops.texture_interface_bind(
                 s_direct3d_device_state_ops.context, texture) != 0)) {
            result = 0;
        }
        if (s_direct3d_device_texture_dirty != 0) {
            if (s_direct3d_device_state_ops.render_state_set != NULL) {
                s_direct3d_device_state_ops.render_state_set(
                    s_direct3d_device_state_ops.context, 1u, 0u);
            }
            else result = 0;
            s_direct3d_device_texture_dirty = 0;
        }
    }
    if (light != s_direct3d_cached_light) {
        if (s_direct3d_device_state_ops.light_state_set == NULL ||
            s_direct3d_device_state_ops.light_state_set(
                s_direct3d_device_state_ops.context, 1u, light) != 0) {
            result = 0;
        }
        else {
            s_direct3d_cached_light = light;
        }
    }
    if (s_direct3d_device_state_ops.primitive_submit == NULL ||
        s_direct3d_device_state_ops.primitive_submit(
            s_direct3d_device_state_ops.context, 4u, a, b, c, d, e) != 0) {
        result = 0;
    }
    return result;
}

void model_material_update_hook_set(ModelMaterialUpdateHook hook,
                                    void* context)
{
    s_model_material_update_hook = hook;
    s_model_material_update_context = context;
}

void model_child_material_ops_set(const ModelChildMaterialOps* ops)
{
    if (ops != NULL) s_model_child_material_ops = *ops;
    else memset(&s_model_child_material_ops, 0,
                sizeof(s_model_child_material_ops));
}

/* WorldObjectRenderStateApply (FUN_00483b00): select the texture-stage
 * operation from the child flags, set the material's boolean state, then
 * apply the child material flags when present. */
// FUNCTION: LEGORR 0x00483b00
int world_object_render_state_apply(void* child, const float* transform,
                                    int alternate_mode)
{
    uint32_t flags = 0u;
    uint32_t material_flags = 0u;
    if (child == NULL) return 0;
    if (s_model_child_material_ops.flags_get != NULL) {
        flags = s_model_child_material_ops.flags_get(
            s_model_child_material_ops.context, child);
    }
    if ((s_renderer_window_mode_flags & 0x20u) == 0u &&
        s_model_render_record_state_ops.texture_stage_state_set != NULL) {
        int stage_value = ((flags & 0x100000u) != 0u &&
            s_model_child_material_ops.texture_get != NULL &&
            s_model_child_material_ops.texture_get(
                s_model_child_material_ops.context, child) != NULL) ? 2 : 4;
        s_model_render_record_state_ops.texture_stage_state_set(
            s_model_render_record_state_ops.context, 1u,
            (uint32_t)stage_value);
    }
    direct3d_render_state_set_cached(
        0x29u, ((flags & 0x400000u) != 0u) ? 1u : 0u);
    if (s_model_child_material_ops.material_flags_get != NULL) {
        material_flags = s_model_child_material_ops.material_flags_get(
            s_model_child_material_ops.context, child);
    }
    if (material_flags == 0u) return 0;
    render_state_flags_apply(material_flags, transform, alternate_mode);
    return 1;
}

/* ModelRenderChildMaterialPass (FUN_00483a90): apply world/material state,
 * update the material record, then submit the child's primitive. */
// FUNCTION: LEGORR 0x00483a90
int model_render_child_material_pass(void* light, void* child,
                                     const float* transform,
                                     int alternate_mode)
{
    uint32_t arguments[4] = { 0u, 0u, 0u, 0u };
    void* texture = NULL;
    const void* material = NULL;
    int material_ok = 0;
    int render_ok;
    if (child == NULL) return 0;
    world_object_render_state_apply(child, transform, alternate_mode);
    if (s_model_child_material_ops.texture_get != NULL) {
        texture = s_model_child_material_ops.texture_get(
            s_model_child_material_ops.context, child);
    }
    if (s_model_child_material_ops.material_record_get != NULL) {
        material = s_model_child_material_ops.material_record_get(
            s_model_child_material_ops.context, child);
    }
    if (s_model_material_update_hook != NULL && material != NULL) {
        material_ok = s_model_material_update_hook(
            s_model_material_update_context, material) != 0;
    }
    if (s_model_child_material_ops.render_arguments_get != NULL) {
        s_model_child_material_ops.render_arguments_get(
            s_model_child_material_ops.context, child, arguments);
    }
    render_ok = direct3d_device_state_apply(
        light, texture, 0x112u, arguments[3], arguments[1], arguments[2],
        arguments[0]);
    return (render_ok != 0 && material_ok != 0) ? 1 : 0;
}

void model_render_pass_dispatch_ops_set(
    const ModelRenderPassDispatchOps* ops)
{
    if (ops != NULL) s_model_render_pass_dispatch_ops = *ops;
    else memset(&s_model_render_pass_dispatch_ops, 0,
                sizeof(s_model_render_pass_dispatch_ops));
}

/* ModelRenderPassDispatch (FUN_00482d60): configure the frame, walk queued
 * models, render each base pass, then render eligible child records in the
 * corpus's two material ordering groups and flush shared state. */
void model_render_pass_dispatch(void* device)
{
    ModelRenderPassDispatchOps* ops = &s_model_render_pass_dispatch_ops;
    void* entry;
    if (ops->first == NULL) return;
    entry = ops->first(ops->context);
    if (entry == NULL) return;
    if (ops->frame_setup != NULL) ops->frame_setup(ops->context, device);
    if (ops->render_mode_begin != NULL)
        ops->render_mode_begin(ops->context);
    while (entry != NULL) {
        void* model = ops->model != NULL ?
            ops->model(ops->context, entry) : NULL;
        const void* parameters = ops->parameters != NULL ?
            ops->parameters(ops->context, entry) : NULL;
        uint32_t entry_flags = ops->entry_flags != NULL ?
            ops->entry_flags(ops->context, entry) : 0u;
        if (model != NULL && (entry_flags & 1u) == 0u) {
            uint32_t index;
            uint32_t count = 0u;
            uint32_t model_flags = ops->model_flags != NULL ?
                ops->model_flags(ops->context, model) : 0u;
            if (ops->model_base_pass != NULL)
                ops->model_base_pass(ops->context, model, device, parameters);
            if ((model_flags & 2u) != 0u) {
                if (ops->model_alternate_pass != NULL)
                    ops->model_alternate_pass(ops->context, model,
                                              parameters, 1);
            }
            else if (ops->record_count != NULL && ops->record != NULL) {
                count = ops->record_count(ops->context, model);
                for (index = 0u; index < count; ++index) {
                    void* record = ops->record(ops->context, model, index);
                    if (record != NULL && ops->record_ready != NULL &&
                        ops->record_ready(ops->context, record) != 0 &&
                        ops->record_flags != NULL &&
                        (ops->record_flags(ops->context, record) &
                         0x80000u) != 0u &&
                        (ops->record_has_secondary_material == NULL ||
                         ops->record_has_secondary_material(
                             ops->context, record) == 0) &&
                        ops->model_child_pass != NULL) {
                        ops->model_child_pass(ops->context, model, record,
                                              parameters, 1);
                    }
                }
                for (index = 0u; index < count; ++index) {
                    void* record = ops->record(ops->context, model, index);
                    if (record != NULL && ops->record_ready != NULL &&
                        ops->record_ready(ops->context, record) != 0 &&
                        ops->record_flags != NULL &&
                        (ops->record_flags(ops->context, record) &
                         0x80000u) != 0u &&
                        ops->record_has_secondary_material != NULL &&
                        ops->record_has_secondary_material(
                            ops->context, record) != 0 &&
                        ops->model_child_pass != NULL) {
                        ops->model_child_pass(ops->context, model, record,
                                              parameters, 1);
                    }
                }
            }
        }
        if (ops->next == NULL) break;
        entry = ops->next(ops->context, entry);
    }
    if (ops->render_queue_flush != NULL)
        ops->render_queue_flush(ops->context);
    if (ops->render_state_flush != NULL)
        ops->render_state_flush(ops->context);
}

void direct3d_interface_ops_set(const Direct3DInterfaceOps* ops)
{
    if (ops != NULL) s_direct3d_interface_ops = *ops;
    else memset(&s_direct3d_interface_ops, 0,
                sizeof(s_direct3d_interface_ops));
}

static int direct3d_interface_create_from_device(void* device)
{
    void* interface_pointer = NULL;
    int success = 0;
    if (device == NULL || s_direct3d_interface_ops.capability_check == NULL ||
        s_direct3d_interface_ops.interface_query == NULL ||
        s_direct3d_interface_ops.renderer_interface_attach == NULL) return 0;
    if (s_direct3d_interface_ops.capability_check(
            s_direct3d_interface_ops.context, device) == 0 &&
        s_direct3d_interface_ops.interface_query(
            s_direct3d_interface_ops.context, device,
            &interface_pointer) == 0 &&
        interface_pointer != NULL &&
        s_direct3d_interface_ops.renderer_interface_attach(
            s_direct3d_interface_ops.context, interface_pointer) == 0) {
        success = 1;
    }
    if (interface_pointer != NULL &&
        s_direct3d_interface_ops.interface_release != NULL) {
        s_direct3d_interface_ops.interface_release(
            s_direct3d_interface_ops.context, interface_pointer);
    }
    return success;
}

/* RenderInterfaceCapabilityCheck (FUN_00483580): query device capabilities,
 * obtain the required interface and attach it to the renderer. */
// FUNCTION: LEGORR 0x00483580
int render_interface_capability_check(void* device)
{
    return direct3d_interface_create_from_device(device);
}

/* Direct3DViewportSetup (FUN_00483600): get the viewport's device and run
 * the common renderer-interface setup path. */
// FUNCTION: LEGORR 0x00483600
int direct3d_viewport_setup(void* viewport)
{
    void* device;
    if (viewport == NULL ||
        s_direct3d_interface_ops.viewport_device_get == NULL) return 0;
    device = s_direct3d_interface_ops.viewport_device_get(
        s_direct3d_interface_ops.context, viewport);
    return direct3d_interface_create_from_device(device);
}

void directdraw_device_create_ops_set(const DirectDrawDeviceCreateOps* ops)
{
    if (ops != NULL) s_directdraw_device_create_ops = *ops;
    else memset(&s_directdraw_device_create_ops, 0,
                sizeof(s_directdraw_device_create_ops));
}

static int directdraw_device_create_internal(
    const void* guid, const void* adapter, int windowed, void** result_out)
{
    void* device = NULL;
    void* interface_pointer = NULL;
    void* result = NULL;
    int success = 0;
    if (s_directdraw_device_create_ops.device_create == NULL ||
        s_directdraw_device_create_ops.device_interface_query == NULL ||
        s_directdraw_device_create_ops.device_enumerate == NULL) return 0;
    if (s_directdraw_device_create_ops.device_create(
            s_directdraw_device_create_ops.context, guid, &device) == 0 &&
        device != NULL &&
        s_directdraw_device_create_ops.device_interface_query(
            s_directdraw_device_create_ops.context, device,
            &interface_pointer) == 0 && interface_pointer != NULL &&
        s_directdraw_device_create_ops.device_enumerate(
            s_directdraw_device_create_ops.context, device, interface_pointer,
            adapter, windowed, &result) == 0) {
        success = 1;
    }
    if (result_out != NULL) *result_out = result;
    if (interface_pointer != NULL &&
        s_directdraw_device_create_ops.release != NULL) {
        s_directdraw_device_create_ops.release(
            s_directdraw_device_create_ops.context, interface_pointer);
    }
    if (device != NULL && s_directdraw_device_create_ops.release != NULL) {
        s_directdraw_device_create_ops.release(
            s_directdraw_device_create_ops.context, device);
    }
    return success;
}

/* DirectDrawDeviceCreate (FUN_0047c2e0): create a device from its optional
 * GUID, query the required interface and run the display-mode enumeration. */
// FUNCTION: LEGORR 0x0047c2e0
int directdraw_device_create(const void* guid, void** result_out)
{
    if (result_out != NULL) *result_out = NULL;
    return directdraw_device_create_internal(guid, NULL, 0, result_out);
}

/* DirectDrawEnumeratedDeviceCreate (FUN_0047c4b0): require the adapter's
 * enumerated flag, choose its GUID only when flag 0x10 is set, and dispatch
 * mode enumeration with the requested windowed/fullscreen policy. */
// FUNCTION: LEGORR 0x0047c4b0
int directdraw_enumerated_device_create(const void* adapter,
                                        uint32_t adapter_flags,
                                        int windowed, void** result_out)
{
    const void* guid;
    if (result_out != NULL) *result_out = NULL;
    if (adapter == NULL || (adapter_flags & 1u) == 0u) return 0;
    guid = ((adapter_flags & 0x10u) != 0u) ? adapter : NULL;
    return directdraw_device_create_internal(guid, adapter, windowed,
                                             result_out);
}

void directdraw_renderer_init_ops_set(const DirectDrawRendererInitOps* ops)
{
    if (ops != NULL) s_directdraw_renderer_init_ops = *ops;
    else memset(&s_directdraw_renderer_init_ops, 0,
                sizeof(s_directdraw_renderer_init_ops));
}

/* DirectDrawRendererInit (FUN_0047c610): normalize the adapter and mode
 * selection, configure the window/device, create primary and back surfaces,
 * attach the clipper, initialize Direct3D, and commit the resulting state. */
// FUNCTION: LEGORR 0x0047c610
int directdraw_renderer_init(const DirectDrawRendererInitParams* params)
{
    const void* guid = NULL;
    int width;
    int height;
    int bits_per_pixel;
    void* device = NULL;
    void* primary = NULL;
    void* back_surface = NULL;
    int success = 0;
    if (params == NULL) return 0;
    width = params->width;
    height = params->height;
    bits_per_pixel = params->mode_bits_per_pixel > 0
        ? params->mode_bits_per_pixel : 16;
    if ((params->adapter_flags & 1u) != 0u &&
        (params->adapter_flags & 0x10u) != 0u) {
        guid = params->adapter_guid;
    }
    if (params->mode_width > 0) width = params->mode_width;
    if (params->mode_height > 0) height = params->mode_height;
    if (width <= 0 || height <= 0 ||
        s_directdraw_renderer_init_ops.device_create == NULL ||
        s_directdraw_renderer_init_ops.cooperative_level_set == NULL ||
        s_directdraw_renderer_init_ops.surfaces_create == NULL ||
        s_directdraw_renderer_init_ops.clipper_setup == NULL ||
        s_directdraw_renderer_init_ops.direct3d_initialize == NULL) return 0;

    renderer_window_mode_apply(params->fullscreen, params->window_x,
                               params->window_y, width, height);
    if (!s_directdraw_renderer_init_ops.device_create(
            s_directdraw_renderer_init_ops.context, guid, &device) ||
        device == NULL) goto cleanup;
    if (!s_directdraw_renderer_init_ops.cooperative_level_set(
            s_directdraw_renderer_init_ops.context, device,
            params->fullscreen,
            params->fullscreen != 0 ? 0x11u : 8u)) goto cleanup;
    if (params->fullscreen != 0) {
        if (s_directdraw_renderer_init_ops.display_mode_set == NULL ||
            !s_directdraw_renderer_init_ops.display_mode_set(
                s_directdraw_renderer_init_ops.context, device, width,
                height, bits_per_pixel)) goto cleanup;
    }
    if (!s_directdraw_renderer_init_ops.surfaces_create(
            s_directdraw_renderer_init_ops.context, device,
            params->fullscreen, width, height, bits_per_pixel,
            &primary, &back_surface) || primary == NULL ||
        back_surface == NULL) goto cleanup;
    if (!s_directdraw_renderer_init_ops.clipper_setup(
            s_directdraw_renderer_init_ops.context, width, height)) {
        goto cleanup;
    }
    if (!s_directdraw_renderer_init_ops.direct3d_initialize(
            s_directdraw_renderer_init_ops.context, device, primary,
            back_surface, params->fullscreen)) goto cleanup;
    if (s_directdraw_renderer_init_ops.renderer_state_commit != NULL) {
        s_directdraw_renderer_init_ops.renderer_state_commit(
            s_directdraw_renderer_init_ops.context, device, primary,
            back_surface, params->fullscreen);
    }
    success = 1;

cleanup:
    if (!success && s_directdraw_renderer_init_ops.release != NULL) {
        if (back_surface != NULL) {
            s_directdraw_renderer_init_ops.release(
                s_directdraw_renderer_init_ops.context, back_surface);
        }
        if (primary != NULL) {
            s_directdraw_renderer_init_ops.release(
                s_directdraw_renderer_init_ops.context, primary);
        }
    }
    if (device != NULL && s_directdraw_renderer_init_ops.release != NULL) {
        s_directdraw_renderer_init_ops.release(
            s_directdraw_renderer_init_ops.context, device);
    }
    return success;
}

void direct3d_texture_memory_query_hook_set(
    Direct3DTextureMemoryCapsQueryFn hook, void* context)
{
    s_texture_memory_caps_query = hook;
    s_texture_memory_caps_context = context;
}

/* QueryTextureMem (FUN_0047cd50): try the preferred texture-memory caps
 * query, then the fallback capability path, and retain the reported amount. */
// FUNCTION: LEGORR 0x0047cd50
void query_texture_memory(void)
{
    uint32_t memory = 0u;
    if (s_texture_memory_caps_query == NULL) return;
    if (s_texture_memory_caps_query(s_texture_memory_caps_context,
                                    0x800u, &memory) != 0) {
        memory = 0u;
        if (s_texture_memory_caps_query(s_texture_memory_caps_context,
                                        0x400u, &memory) != 0) return;
    }
    s_texture_memory_last_value = memory;
}

uint32_t query_texture_memory_last_value(void)
{
    return s_texture_memory_last_value;
}

void direct3drm_device_init_ops_set(const Direct3DRMDeviceInitOps* ops)
{
    if (ops != NULL) s_direct3drm_device_init_ops = *ops;
    else memset(&s_direct3drm_device_init_ops, 0,
                sizeof(s_direct3drm_device_init_ops));
}

/* Direct3DRMDeviceInit (FUN_004781d0): choose the renderer device mode from
 * adapter capabilities, create the RM/Direct3D chain, initialize its device,
 * query the renderer interface, and commit the successful device. */
// FUNCTION: LEGORR 0x004781d0
int direct3drm_device_initialize(void* adapter, uint32_t adapter_flags,
                                 void* surface, int force_single_mode)
{
    void* rm = NULL;
    void* direct3d = NULL;
    void* device = NULL;
    void* renderer = NULL;
    int success = 0;
    if (adapter != NULL && (adapter_flags & 0x4000u) != 0u) {
        s_renderer_window_mode_flags |= 4u;
    }
    if (force_single_mode != 0 ||
        (s_renderer_window_mode_flags & 0x80000u) != 0u ||
        (adapter_flags & 0x2000u) == 0u) {
        s_direct3drm_device_mode = 1;
    }
    else {
        s_direct3drm_device_mode = 2;
    }
    if (s_direct3drm_device_init_ops.rm_create == NULL ||
        s_direct3drm_device_init_ops.direct3d_query == NULL ||
        s_direct3drm_device_init_ops.device_create == NULL ||
        s_direct3drm_device_init_ops.device_initialize == NULL ||
        s_direct3drm_device_init_ops.renderer_interface_query == NULL) return 0;
    if (!s_direct3drm_device_init_ops.rm_create(
            s_direct3drm_device_init_ops.context, &rm) || rm == NULL) goto cleanup;
    if (!s_direct3drm_device_init_ops.direct3d_query(
            s_direct3drm_device_init_ops.context, rm, &direct3d) ||
        direct3d == NULL) goto cleanup;
    if (!s_direct3drm_device_init_ops.device_create(
            s_direct3drm_device_init_ops.context, direct3d, adapter, surface,
            s_direct3drm_device_mode, &device) || device == NULL) goto cleanup;
    if (!s_direct3drm_device_init_ops.device_initialize(
            s_direct3drm_device_init_ops.context, device)) goto cleanup;
    if (!s_direct3drm_device_init_ops.renderer_interface_query(
            s_direct3drm_device_init_ops.context, rm, &renderer) ||
        renderer == NULL) goto cleanup;
    if (adapter != NULL &&
        s_direct3drm_device_init_ops.device_display_mode_set != NULL &&
        !s_direct3drm_device_init_ops.device_display_mode_set(
            s_direct3drm_device_init_ops.context, device, 2)) goto cleanup;
    if (s_direct3drm_device_init_ops.state_commit != NULL) {
        s_direct3drm_device_init_ops.state_commit(
            s_direct3drm_device_init_ops.context, device, renderer,
            s_direct3drm_device_mode);
    }
    success = 1;

cleanup:
    if (s_direct3drm_device_init_ops.release != NULL) {
        if (success == 0) {
            if (renderer != NULL) s_direct3drm_device_init_ops.release(
                s_direct3drm_device_init_ops.context, renderer);
            if (device != NULL) s_direct3drm_device_init_ops.release(
                s_direct3drm_device_init_ops.context, device);
        }
        if (direct3d != NULL) s_direct3drm_device_init_ops.release(
            s_direct3drm_device_init_ops.context, direct3d);
        if (rm != NULL) s_direct3drm_device_init_ops.release(
            s_direct3drm_device_init_ops.context, rm);
    }
    return success;
}

int direct3drm_device_mode_get(void)
{
    return s_direct3drm_device_mode;
}

void directdraw_device_initialize_ops_set(
    const DirectDrawDeviceInitializeOps* ops)
{
    if (ops != NULL) s_directdraw_device_initialize_ops = *ops;
    else memset(&s_directdraw_device_initialize_ops, 0,
                sizeof(s_directdraw_device_initialize_ops));
}

/* DirectDrawDeviceInitialize (FUN_0049d030): register standard modes,
 * enumerate adapters, optionally auto-select a supported display mode, then
 * enter the renderer initializer with the selected setup. */
// FUNCTION: LEGORR 0x0049d030
int directdraw_device_initialize(int show_dialog, int register_extra_modes,
                                 int auto_select, int windowed,
                                 const char* error_message)
{
    int adapter_count;
    int fullscreen = (windowed == 0) ? 1 : 0;
    int selected = 0;
    int adapter_index;
    DirectDrawAdapterCandidate adapter;
    DirectDrawDisplayModeCandidate mode;
    if (s_directdraw_device_initialize_ops.mode_register != NULL) {
        s_directdraw_device_initialize_ops.mode_register(
            s_directdraw_device_initialize_ops.context, 640, 480, 16);
        if (register_extra_modes != 0) {
            s_directdraw_device_initialize_ops.mode_register(
                s_directdraw_device_initialize_ops.context, 800, 600, 16);
            s_directdraw_device_initialize_ops.mode_register(
                s_directdraw_device_initialize_ops.context, 1024, 768, 16);
        }
    }
    if (s_directdraw_device_initialize_ops.adapter_count_get == NULL ||
        s_directdraw_device_initialize_ops.adapter_get == NULL ||
        s_directdraw_device_initialize_ops.renderer_initialize == NULL) return 0;
    adapter_count = s_directdraw_device_initialize_ops.adapter_count_get(
        s_directdraw_device_initialize_ops.context);
    if (adapter_count <= 0) return 0;
    memset(&adapter, 0, sizeof(adapter));
    memset(&mode, 0, sizeof(mode));

    if (auto_select != 0) {
        int mode_index;
        for (adapter_index = adapter_count - 1;
             adapter_index >= 0 && !selected; --adapter_index) {
            int mode_count;
            if (!s_directdraw_device_initialize_ops.adapter_get(
                    s_directdraw_device_initialize_ops.context,
                    adapter_index, &adapter)) continue;
            if (s_directdraw_device_initialize_ops.mode_count_get == NULL ||
                s_directdraw_device_initialize_ops.mode_get == NULL ||
                s_directdraw_device_initialize_ops.mode_supported == NULL) {
                continue;
            }
            mode_count = s_directdraw_device_initialize_ops.mode_count_get(
                s_directdraw_device_initialize_ops.context, adapter.adapter,
                windowed);
            for (mode_index = mode_count - 1; mode_index >= 0; --mode_index) {
                DirectDrawDisplayModeCandidate candidate;
                memset(&candidate, 0, sizeof(candidate));
                if (!s_directdraw_device_initialize_ops.mode_get(
                        s_directdraw_device_initialize_ops.context,
                        adapter.adapter, mode_index, &candidate)) continue;
                if (s_directdraw_device_initialize_ops.mode_supported(
                        s_directdraw_device_initialize_ops.context,
                        adapter.adapter, &candidate, windowed)) {
                    mode = candidate;
                    selected = 1;
                    break;
                }
            }
        }
        if (selected == 0) {
            if (s_directdraw_device_initialize_ops.error_show != NULL) {
                s_directdraw_device_initialize_ops.error_show(
                    s_directdraw_device_initialize_ops.context,
                    error_message);
            }
            return 0;
        }
    }
    else {
        if (!s_directdraw_device_initialize_ops.adapter_get(
                s_directdraw_device_initialize_ops.context, 0, &adapter)) return 0;
        selected = 1;
        if (show_dialog != 0 &&
            s_directdraw_device_initialize_ops.selection_dialog != NULL) {
            if (!s_directdraw_device_initialize_ops.selection_dialog(
                    s_directdraw_device_initialize_ops.context, 0,
                    &adapter, &mode, &fullscreen)) return 0;
        }
    }
    return s_directdraw_device_initialize_ops.renderer_initialize(
        s_directdraw_device_initialize_ops.context, fullscreen,
        adapter.adapter, mode.descriptor != NULL ? &mode : NULL);
}

/* RendererWindowModeApply (FUN_004780b0): retain the requested dimensions,
 * switch the platform mode, then size/place and activate the main window. */
// FUNCTION: LEGORR 0x004780b0
void renderer_window_mode_apply(int fullscreen, int x, int y,
                                int width, int height)
{
    int rect[4];
    s_renderer_window_width = width;
    s_renderer_window_height = height;
    window_runtime_mode_set(fullscreen, (int32_t)0x80c80000u);
    if (fullscreen != 0) s_renderer_window_mode_flags |= 2u;
    if (s_renderer_window_mode_ops.window_mode_set != NULL) {
        s_renderer_window_mode_ops.window_mode_set(
            s_renderer_window_mode_ops.context, fullscreen);
    }
    if (fullscreen == 0) {
        rect[0] = x;
        rect[1] = y;
        rect[2] = width;
        rect[3] = height;
        if (s_renderer_window_mode_ops.window_style_set != NULL) {
            s_renderer_window_mode_ops.window_style_set(
                s_renderer_window_mode_ops.context, (int32_t)0x80c80000u);
        }
        if (s_renderer_window_mode_ops.window_position_set != NULL) {
            s_renderer_window_mode_ops.window_position_set(
                s_renderer_window_mode_ops.context, rect[0], rect[1],
                rect[2], rect[3], 4u);
        }
        if (s_renderer_window_mode_ops.cursor_show != NULL) {
            s_renderer_window_mode_ops.cursor_show(
                s_renderer_window_mode_ops.context, 0);
        }
    }
    else {
        if (s_renderer_window_mode_ops.desktop_rect_get != NULL &&
            s_renderer_window_mode_ops.desktop_rect_get(
                s_renderer_window_mode_ops.context, rect) &&
            s_renderer_window_mode_ops.window_position_set != NULL) {
            s_renderer_window_mode_ops.window_position_set(
                s_renderer_window_mode_ops.context, rect[0], rect[1],
                rect[2] - rect[0], rect[3] - rect[1], 4u);
        }
        if (s_renderer_window_mode_ops.cursor_hide != NULL) {
            s_renderer_window_mode_ops.cursor_hide(
                s_renderer_window_mode_ops.context);
        }
    }
    if (s_renderer_window_mode_ops.window_show_activate != NULL) {
        s_renderer_window_mode_ops.window_show_activate(
            s_renderer_window_mode_ops.context, 5);
    }
}

uint32_t renderer_window_mode_flags_peek(void)
{
    return s_renderer_window_mode_flags;
}

void renderer_window_dimensions_get(int* width_out, int* height_out)
{
    if (width_out != NULL) *width_out = s_renderer_window_width;
    if (height_out != NULL) *height_out = s_renderer_window_height;
}

void surface_pixel_bits_query_hook_set(SurfacePixelBitsQueryFn query)
{
    s_surface_pixel_bits_query = query;
}

/* RenderSetup (FUN_00485e80): set the render-ready bit, then apply the
 * optional subrectangle through the viewport's dimension-query seam. */
// FUNCTION: LEGORR 0x00485e80
void surface_render_setup(const float rectangle[4])
{
    s_surface_render_flags |= 1u;
    viewport_subrectangle_set(rectangle);
}

uint32_t surface_render_flags_peek(void)
{
    return s_surface_render_flags;
}

/* RendererRecordTableInitialize (FUN_0047d410) clears its 32-word table
 * and raises the adjacent initialized word. The two intervening dwords are
 * retained as reserved state because the corpus leaves them untouched. */
// FUNCTION: LEGORR 0x0047d410
void renderer_record_table_initialize(void)
{
    memset(s_renderer_record_table.words, 0,
           sizeof(s_renderer_record_table.words));
    s_renderer_record_table.enabled = 1u;
}

uint32_t renderer_record_table_word_peek(int index)
{
    return (index >= 0 && index < 32)
        ? s_renderer_record_table.words[index] : 0u;
}

void renderer_record_table_word_set(int index, uint32_t value)
{
    if (index >= 0 && index < 32) {
        s_renderer_record_table.words[index] = value;
    }
}

void renderer_begin_frame_hook_set(RendererBeginFrameHook hook, void* device)
{
    s_renderer_begin_frame_hook = hook;
    s_renderer_begin_frame_device = device;
    s_renderer_frame_started = 0;
}

/* RendererBeginFrameOnce (FUN_00477fa0): invoke the device's scene-begin
 * method once per frame; reset is driven by the frame owner. */
// FUNCTION: LEGORR 0x00477fa0
int renderer_begin_frame_once(void)
{
    if (s_renderer_frame_started) return 1;
    if (s_renderer_begin_frame_hook == NULL) return 0;
    s_renderer_begin_frame_hook(s_renderer_begin_frame_device);
    s_renderer_frame_started = 1;
    return 1;
}

void renderer_begin_frame_once_reset(void)
{
    s_renderer_frame_started = 0;
}

/* DirectDrawSurfaceDescriptorInitialize (FUN_0047e710): clear the
 * descriptor storage at +0x1c and restore DDSURFACEDESC.dwSize. */
// FUNCTION: LEGORR 0x0047e710
void surface_descriptor_initialize(void* record)
{
    unsigned char* bytes = (unsigned char*)record;
    if (bytes == NULL) return;
    memset(bytes + 0x1c, 0, 0x7c);
    *(uint32_t*)(bytes + 0x1c) = 0x7cu;
    *(uint32_t*)(bytes + 0x18) = 0u;
    *(uint32_t*)(bytes + 0x10) = 0u;
    bytes[0x08] = 0u;
    bytes[0x14] = 0u;
}

/* RenderObjectStateApplyToEntries (FUN_00473ec0): state kind 4 snapshots
 * the child set and applies each child callback; kind 1 applies the surface
 * property to each layer in order. */
// FUNCTION: LEGORR 0x00473ec0
void render_object_state_apply_to_entries(void* object, uint32_t value,
                                          const RenderObjectStateOps* ops)
{
    unsigned int i;
    int kind;
    if (object == NULL || ops == NULL || ops->object_kind == NULL) return;
    kind = ops->object_kind(ops->context, object);
    if (kind == 4 && ops->children_snapshot != NULL &&
        ops->child_state_apply != NULL) {
        void** children = NULL;
        unsigned int count = 0;
        if (ops->children_snapshot(ops->context, object, &children,
                                   &count) == 0 || children == NULL) {
            return;
        }
        for (i = 0; i < count; ++i) {
            if (children[i] != NULL)
                ops->child_state_apply(ops->context, children[i], value);
        }
        if (ops->children_snapshot_release != NULL)
            ops->children_snapshot_release(ops->context, children);
    }
    else if (kind == 1 && ops->layer_count != NULL &&
             ops->layer_property_set != NULL) {
        unsigned int count = ops->layer_count(ops->context, object);
        for (i = 0; i < count; ++i) {
            ops->layer_property_set(ops->context, object, i, value);
        }
    }
}

/* TerrainSurfacePropertySet (FUN_00474e30): mesh-backed objects already
 * receive material state through their mesh; regular layers get property
 * bit 4 set or cleared on the resolved animated layer frame. */
// FUNCTION: LEGORR 0x00474e30
void terrain_surface_property_set(void* object, unsigned int layer_index,
                                  int enabled,
                                  const TerrainSurfacePropertyOps* ops)
{
    unsigned int frame = 0u;
    void* layer;
    if (object == NULL || ops == NULL || ops->layer_resolve == NULL ||
        ops->property_set == NULL) return;
    if (ops->has_mesh != NULL && ops->has_mesh(ops->context, object) != 0) {
        return;
    }
    layer = ops->layer_resolve(ops->context, object, layer_index, &frame);
    if (layer != NULL) {
        ops->property_set(ops->context, layer, frame,
                          enabled != 0 ? 4u : 0u);
    }
}

/* LwoLayerEnabledSet (FUN_00474b30): route mesh-backed entries to their
 * mesh setter; otherwise attach/detach the selected layer and maintain its
 * disabled high bit. Index zero uses the collection's base enabled field. */
// FUNCTION: LEGORR 0x00474b30
void lwo_layer_enabled_set(void* object, unsigned int index, int enabled,
                           const LwoLayerEnabledOps* ops)
{
    void* mesh;
    void* layer;
    int current_enabled;
    uint32_t flags;
    if (object == NULL || ops == NULL) return;
    mesh = ops->mesh_get != NULL
        ? ops->mesh_get(ops->context, object) : NULL;
    if (mesh != NULL) {
        if (ops->mesh_set != NULL)
            ops->mesh_set(ops->context, mesh, index, enabled);
        return;
    }
    if (ops->layer_get == NULL) return;
    layer = ops->layer_get(ops->context, object, index);
    if (layer == NULL) return;
    if (index == 0u) {
        current_enabled = ops->base_enabled_get != NULL
            ? ops->base_enabled_get(ops->context, layer) : 0;
        if (enabled != 0 && current_enabled == 0 &&
            ops->layer_attach != NULL) {
            ops->layer_attach(ops->context, object, layer);
        }
        else if (enabled == 0 && current_enabled != 0 &&
                 ops->layer_detach != NULL) {
            ops->layer_detach(ops->context, object, layer);
        }
        if (ops->base_enabled_set != NULL)
            ops->base_enabled_set(ops->context, layer, enabled);
        return;
    }
    if (ops->layer_flags_get == NULL || ops->layer_flags_set == NULL) return;
    flags = ops->layer_flags_get(ops->context, layer);
    current_enabled = (flags & 0x80000000u) == 0u;
    if (enabled != 0 && current_enabled == 0 && ops->layer_attach != NULL) {
        ops->layer_attach(ops->context, object, layer);
    }
    else if (enabled == 0 && current_enabled != 0 &&
             ops->layer_detach != NULL) {
        ops->layer_detach(ops->context, object, layer);
    }
    flags = (flags & 0x7fffffffu) |
            (enabled != 0 ? 0u : 0x80000000u);
    ops->layer_flags_set(ops->context, layer, flags);
}

// FUNCTION: LEGORR 0x00474c40
uint32_t lwo_layer_polygon_vertex_count(
    void* object, unsigned int layer_index, const void* request,
    const LwoLayerDispatchOps* ops)
{
    void* mesh;
    void* layer;
    unsigned int frame = 0u;
    if (object == NULL || ops == NULL || ops->mesh_get == NULL) return 0u;
    mesh = ops->mesh_get(ops->context, object);
    if (mesh != NULL) {
        return ops->mesh_polygon_vertex_count != NULL
            ? ops->mesh_polygon_vertex_count(ops->context, mesh, request) : 0u;
    }
    if (ops->layer_resolve == NULL ||
        ops->layer_polygon_vertex_count == NULL) return 0u;
    layer = ops->layer_resolve(ops->context, object, layer_index, &frame);
    if (layer == NULL) return 0u;
    return ops->layer_polygon_vertex_count(ops->context, layer, frame,
                                           request) == 0 ? 1u : 0u;
}

// FUNCTION: LEGORR 0x00474cc0
uint32_t lwo_layer_polygon_vertex_data_read(
    void* object, unsigned int layer_index, const void* request,
    const LwoLayerDispatchOps* ops)
{
    void* mesh;
    void* layer;
    unsigned int frame = 0u;
    if (object == NULL || ops == NULL || ops->mesh_get == NULL) return 0u;
    mesh = ops->mesh_get(ops->context, object);
    if (mesh != NULL) {
        return ops->mesh_polygon_vertex_data_read != NULL
            ? ops->mesh_polygon_vertex_data_read(ops->context, mesh, request)
            : 0u;
    }
    if (ops->layer_resolve == NULL ||
        ops->layer_polygon_vertex_data_read == NULL) return 0u;
    layer = ops->layer_resolve(ops->context, object, layer_index, &frame);
    if (layer == NULL) return 0u;
    return ops->layer_polygon_vertex_data_read(ops->context, layer, frame,
            request) == 0 ? 0u : 0xffffffffu;
}

// FUNCTION: LEGORR 0x00474d30
uint32_t terrain_cell_quad_vertex_emit(
    void* object, unsigned int layer_index, const void* request,
    const LwoLayerDispatchOps* ops)
{
    void* mesh;
    void* layer;
    unsigned int frame = 0u;
    if (object == NULL || ops == NULL || ops->mesh_get == NULL) return 0u;
    mesh = ops->mesh_get(ops->context, object);
    if (mesh != NULL) {
        return ops->mesh_terrain_quad_emit != NULL
            ? ops->mesh_terrain_quad_emit(ops->context, mesh, request) : 0u;
    }
    if (ops->layer_resolve == NULL ||
        ops->layer_terrain_quad_emit == NULL) return 0u;
    layer = ops->layer_resolve(ops->context, object, layer_index, &frame);
    if (layer == NULL) return 0u;
    return ops->layer_terrain_quad_emit(ops->context, layer, frame, request)
        == 0 ? 0xffffffffu : 0u;
}

/* TerrainDetailEntryGetAtIndex (FUN_00474ae0): mesh paths delegate directly;
 * layer paths query child flags, with index zero reading the collection's
 * base enabled value. If no child array is available, the original returns
 * the requested index. */
// FUNCTION: LEGORR 0x00474ae0
uint32_t terrain_detail_entry_get_at_index(
    void* object, unsigned int index, const TerrainDetailLookupOps* ops)
{
    void* mesh;
    void* collection;
    void* children;
    void* child;
    if (object == NULL || ops == NULL || ops->mesh_get == NULL) return 0u;
    mesh = ops->mesh_get(ops->context, object);
    if (mesh != NULL) {
        return ops->mesh_entry_get != NULL
            ? ops->mesh_entry_get(ops->context, mesh, index) : 0u;
    }
    if (ops->layer_collection_get == NULL || ops->children_get == NULL)
        return index;
    collection = ops->layer_collection_get(ops->context, object);
    if (collection == NULL) return index;
    children = ops->children_get(ops->context, collection);
    if (children == NULL) return index;
    if (index == 0u) {
        return ops->base_enabled_get != NULL
            ? ops->base_enabled_get(ops->context, children) : 0u;
    }
    if (ops->child_get == NULL || ops->child_flags_get == NULL) return 0u;
    child = ops->child_get(ops->context, children, index - 1u);
    if (child == NULL) return 0u;
    return (ops->child_flags_get(ops->context, child) & 0x80000000u)
        == 0u ? 1u : 0u;
}

/* DirectDrawObjectsRelease (FUN_0047ccf0): release the primary surface and
 * clipper interfaces, restore display mode when the device was exclusive,
 * then release the DirectDraw device itself. */
// FUNCTION: LEGORR 0x0047ccf0
void directdraw_objects_release(DirectDrawObjectSet* objects,
                                const DirectDrawObjectReleaseOps* ops)
{
    if (objects == NULL) return;
    if (ops != NULL && ops->release != NULL) {
        if (objects->primary_surface != NULL)
            ops->release(ops->context, objects->primary_surface);
        if (objects->primary_clipper != NULL)
            ops->release(ops->context, objects->primary_clipper);
        if (objects->windowed_clipper != NULL)
            ops->release(ops->context, objects->windowed_clipper);
    }
    objects->primary_surface = NULL;
    objects->primary_clipper = NULL;
    objects->windowed_clipper = NULL;
    if (objects->device != NULL) {
        if (objects->restore_display_mode != 0 && ops != NULL &&
            ops->restore_display_mode != NULL) {
            ops->restore_display_mode(ops->context, objects->device);
        }
        if (ops != NULL && ops->release != NULL)
            ops->release(ops->context, objects->device);
    }
    objects->device = NULL;
    objects->restore_display_mode = 0;
}


/* SurfacePixelWriterSelectByFormat (FUN_00486690): select the packed-pixel
 * writer by depth; 16-bit modes 1 and 2 use their dedicated byte-order
 * paths, with other 16-bit formats taking the default writer. */
// FUNCTION: LEGORR 0x00486690
int surface_pixel_writer_select_by_format(
    uint32_t bits_per_pixel, int format,
    const SurfacePixelWriterTable* writers,
    SurfacePixelWriteFn* out_writer)
{
    SurfacePixelWriteFn writer = NULL;
    if (writers == NULL || out_writer == NULL) return 0;
    switch (bits_per_pixel) {
    case 8u: writer = writers->writer8; break;
    case 16u:
        if (format == 1) writer = writers->writer16_mode1;
        else if (format == 2) writer = writers->writer16_mode2;
        else writer = writers->writer16_default;
        break;
    case 24u: writer = writers->writer24; break;
    case 32u: writer = writers->writer32; break;
    default: break;
    }
    if (writer == NULL) return 0;
    s_surface_pixel_writer = writer;
    *out_writer = writer;
    return 1;
}

/* SurfaceLockConfigurePixelWriter (FUN_00486550): ask the surface owner for
 * its pixel depth and channel masks, retain the format facts used by color
 * conversion, then install the matching writer. */
// FUNCTION: LEGORR 0x00486550
int surface_lock_configure_pixel_writer(
    void* surface, int format, SurfaceLockFormatQueryFn query,
    const SurfacePixelWriterTable* writers)
{
    uint32_t bits_per_pixel = 0u;
    uint32_t red_mask = 0u;
    uint32_t green_mask = 0u;
    uint32_t blue_mask = 0u;
    SurfacePixelWriteFn writer = NULL;
    if (query == NULL || writers == NULL ||
        query(surface, &bits_per_pixel, &red_mask, &green_mask,
              &blue_mask) != 0) {
        s_surface_pixel_bits = 0u;
        s_surface_pixel_writer = NULL;
        return 0;
    }
    s_surface_pixel_bits = bits_per_pixel;
    s_surface_red_mask_bits = (uint32_t)pop_count32(red_mask);
    s_surface_green_mask_bits = (uint32_t)pop_count32(green_mask);
    s_surface_blue_mask_bits = (uint32_t)pop_count32(blue_mask);
    if (!surface_pixel_writer_select_by_format(bits_per_pixel, format,
                                                writers, &writer)) {
        s_surface_pixel_bits = 0u;
        s_surface_pixel_writer = NULL;
        return 0;
    }
    return 1;
}

void surface_writer_unlock_hook_set(SurfaceWriterUnlockHook hook)
{
    s_surface_writer_unlock_hook = hook;
}

/* DirectDrawSurfaceUnlock (FUN_0047ded0): forward a full-surface unlock to
 * the installed platform interface callback. */
// FUNCTION: LEGORR 0x0047ded0
void surface_directdraw_unlock(void* surface)
{
    if (s_surface_writer_unlock_hook != NULL) {
        s_surface_writer_unlock_hook(surface, 0);
    }
}

/* SurfaceUnlockResetPixelWriter (FUN_00486650): release the locked target
 * through its owner hook, then clear the depth and active pixel-writer
 * globals. The channel-shift counts remain as in the reference. */
// FUNCTION: LEGORR 0x00486650
void surface_unlock_reset_pixel_writer(void* surface)
{
    if (s_surface_writer_unlock_hook != NULL) {
        s_surface_writer_unlock_hook(surface, 0);
    }
    s_surface_pixel_bits = 0u;
    s_surface_pixel_writer = NULL;
}

SurfacePixelWriteFn surface_pixel_writer_peek(void)
{
    return s_surface_pixel_writer;
}

/* FUNCTION: LEGORR 0x0047df50 */
int32_t surface_pixel_channel_mask_get(void* surface)
{
    uint32_t bits_per_pixel = 0;
    uint32_t shift;
    if (s_surface_pixel_bits_query == NULL ||
        s_surface_pixel_bits_query(surface, &bits_per_pixel) != 0)
        return 0;
    shift = (32u - bits_per_pixel) & 31u;
    return (int32_t)(0xffffffffu << shift);
}

/* BresenhamLineWalkClipped (FUN_00486720): walk the inclusive major axis,
 * take a minor-axis step when the doubled error is nonnegative, and emit
 * only coordinates inside the active rectangle. */
// FUNCTION: LEGORR 0x00486720
void surface_bresenham_line_walk_clipped(
    SurfaceRasterContext* raster, int x0, int y0, int x1, int y1,
    uint32_t pixel)
{
    int delta_x;
    int delta_y;
    int x_step;
    int y_step;
    int error;
    int count;
    int steep;
    int x = x0;
    int y = y0;
    if (raster == NULL || raster->write_pixel == NULL) {
        return;
    }
    delta_x = x1 - x0;
    delta_y = y1 - y0;
    x_step = delta_x < 0 ? -1 : 1;
    y_step = delta_y < 0 ? -1 : 1;
    if (delta_x < 0) delta_x = -delta_x;
    if (delta_y < 0) delta_y = -delta_y;
    steep = delta_x < delta_y;
    if (steep) {
        error = 2 * delta_x - delta_y;
        count = delta_y + 1;
    }
    else {
        error = 2 * delta_y - delta_x;
        count = delta_x + 1;
    }
    while (count > 0) {
        if ((float)x >= raster->left && (float)y >= raster->top
            && (float)x < raster->right && (float)y < raster->bottom) {
            raster->write_pixel(raster->context, x, y, pixel);
        }
        if (steep) {
            if (error >= 0) {
                x += x_step;
                error -= 2 * delta_y;
            }
            y += y_step;
            error += 2 * delta_x;
        }
        else {
            if (error >= 0) {
                y += y_step;
                error -= 2 * delta_x;
            }
            x += x_step;
            error += 2 * delta_y;
        }
        --count;
    }
}

/* SurfaceFillPixelSpans (FUN_00486090): each span fills x0..x1 on every
 * scanline y0..y1, with both upper bounds exclusive, through the pixel
 * writer selected for the locked destination surface. */
// FUNCTION: LEGORR 0x00486090
void surface_fill_pixel_spans(SurfaceRasterContext* raster,
                              const SurfacePixelSpan* spans, int count,
                              uint32_t pixel)
{
    int span_index;
    if (raster == NULL || raster->write_pixel == NULL || spans == NULL
        || count <= 0) {
        return;
    }
    for (span_index = 0; span_index < count; ++span_index) {
        const SurfacePixelSpan* span = &spans[span_index];
        int y;
        if (span->x1 <= span->x0 || span->y1 <= span->y0) continue;
        for (y = span->y0; y < span->y1; ++y) {
            int x;
            if ((float)y < raster->top || (float)y >= raster->bottom) {
                continue;
            }
            for (x = span->x0; x < span->x1; ++x) {
                if ((float)x >= raster->left && (float)x < raster->right) {
                    raster->write_pixel(raster->context, x, y, pixel);
                }
            }
        }
    }
}

/* SurfaceDrawPolyline (FUN_00485ff0): connect each adjacent pair with the
 * same clipped integer walk used by the low-level line helper. */
// FUNCTION: LEGORR 0x00485ff0
void surface_draw_polyline(SurfaceRasterContext* raster,
                           const SurfacePoint* points, int count,
                           uint32_t pixel)
{
    int i;
    if (raster == NULL || points == NULL || count < 2) return;
    for (i = 1; i < count; ++i) {
        surface_bresenham_line_walk_clipped(raster,
            points[i - 1].x, points[i - 1].y,
            points[i].x, points[i].y, pixel);
    }
}

/* SurfaceFillPixelSpanRecords (FUN_00486210): consume the span records in
 * order and route each rectangle through the shared span filler. */
// FUNCTION: LEGORR 0x00486210
void surface_fill_pixel_span_records(SurfaceRasterContext* raster,
                                     const SurfacePixelSpan* spans, int count,
                                     uint32_t pixel)
{
    surface_fill_pixel_spans(raster, spans, count, pixel);
}

static void surface_circle_pixel(SurfaceRasterContext* raster,
                                 int x, int y, uint32_t pixel)
{
    if ((float)x >= raster->left && (float)x < raster->right &&
        (float)y >= raster->top && (float)y < raster->bottom) {
        raster->write_pixel(raster->context, x, y, pixel);
    }
}

/* SurfaceDrawCirclePixels (FUN_00486390): midpoint circle walk, emitting
 * the eight symmetric pixels at each step and clipping through the raster. */
// FUNCTION: LEGORR 0x00486390
void surface_draw_circle_pixels(SurfaceRasterContext* raster,
                                int center_x, int center_y, int radius,
                                uint32_t pixel)
{
    int x;
    int y = 0;
    int decision;
    if (raster == NULL || raster->write_pixel == NULL || radius < 0) return;
    x = radius;
    decision = 1 - radius;
    while (x >= y) {
        surface_circle_pixel(raster, center_x + x, center_y + y, pixel);
        surface_circle_pixel(raster, center_x + y, center_y + x, pixel);
        surface_circle_pixel(raster, center_x - y, center_y + x, pixel);
        surface_circle_pixel(raster, center_x - x, center_y + y, pixel);
        surface_circle_pixel(raster, center_x - x, center_y - y, pixel);
        surface_circle_pixel(raster, center_x - y, center_y - x, pixel);
        surface_circle_pixel(raster, center_x + y, center_y - x, pixel);
        surface_circle_pixel(raster, center_x + x, center_y - y, pixel);
        ++y;
        if (decision < 0) decision += 2 * y + 1;
        else { --x; decision += 2 * (y - x) + 1; }
    }
}

/* FUNCTION: LEGORR 0x0042b850
 * Rebuild the reference's compact surface glyph map by emitting each key's
 * 46 byte-valued aliases through the owning map's insertion seam. */
void surface_text_character_map_initialize(
    void* map, SurfaceTextCharacterMapInsertFn insert)
{
    static const uint8_t keys[31] = {
        0x00, 0x00, 0x00, 0x00, 0x46, 0x47, 0x45, 0x00,
        0x76, 0x66, 0x10, 0x11, 0x12, 0x13, 0x06, 0x16,
        0x26, 0x36, 0x07, 0x61, 0x60, 0x62, 0x63, 0x64,
        0x65, 0x71, 0x72, 0x73, 0x74, 0x75, 0x30
    };
    static const uint8_t values[46] = {
        5, 4, 3, 2, 1, 0x40, 0x20, 0x67,
        5, 4, 3, 2, 1, 0x40, 0x20, 0x67,
        0x25, 0x24, 0x23, 0x22, 0x21,
        0x35, 0x34, 0x33, 0x32, 0x31,
        0x35, 0x34, 0x33, 0x32, 0x31,
        0x55, 0x54, 0x53, 0x52, 0x51,
        0x55, 0x54, 0x53, 0x52, 0x51,
        0x77, 0x77, 0x77, 0x77, 0x77
    };
    static const uint8_t secondary_keys[6] = {
        0x70, 0x70, 0x70, 0x70, 0x70, 0x70
    };
    unsigned int i;
    unsigned int j;
    if (insert == NULL) return;
    for (i = 0; i < 31; ++i) {
        for (j = 0; j < 46; ++j) insert(map, keys[i], values[j]);
    }
    for (i = 0; i < 6; ++i) {
        for (j = 0; j < 46; ++j)
            insert(map, secondary_keys[i], values[j]);
    }
}

// FUNCTION: LEGORR 0x0047A550
void font_atlas_record_release(void* record)
{
    void** next_slot;
    if (record == NULL) return;
    texture_release((TextureSurface*)*(void**)record);
    next_slot = (void**)((unsigned char*)record + 0x2fc * sizeof(void*));
    *next_slot = s_font_atlas_record_free_list;
    s_font_atlas_record_free_list = record;
}

void font_pixel_format_hooks_set(FontPixelFormatQueryFn query,
                                 FontPixelFormatReleaseFn release)
{
    s_font_pixel_format_query = query;
    s_font_pixel_format_release = release;
}

// FUNCTION: LEGORR 0x0047e190
uint32_t font_device_color_convert(void* device, uint32_t color)
{
    uint32_t bits_per_pixel = 0u;
    uint32_t red_mask = 0u;
    uint32_t green_mask = 0u;
    uint32_t blue_mask = 0u;
    uint32_t red_bits;
    uint32_t green_bits;
    uint32_t blue_bits;
    uint32_t red_shift;
    uint32_t green_shift;
    uint32_t blue_shift;
    uint32_t result;
    uint32_t red = color & 0xffu;
    uint32_t green = (color >> 8) & 0xffu;
    uint32_t blue = (color >> 16) & 0xffu;
    if (s_font_pixel_format_query == NULL ||
        s_font_pixel_format_query(device, &bits_per_pixel, &red_mask,
                                  &green_mask, &blue_mask) != 0) {
        return 0u;
    }
    red_bits = (uint32_t)pop_count32(red_mask);
    green_bits = (uint32_t)pop_count32(green_mask);
    blue_bits = (uint32_t)pop_count32(blue_mask);
    red_shift = find_first_set_bit32(red_mask);
    green_shift = find_first_set_bit32(green_mask);
    blue_shift = find_first_set_bit32(blue_mask);
    result = ((red >> ((8u - red_bits) & 31u)) << (red_shift & 31u)) |
             ((green >> ((8u - green_bits) & 31u)) <<
              (green_shift & 31u)) |
             ((blue >> ((8u - blue_bits) & 31u)) << (blue_shift & 31u));
    if (bits_per_pixel < 32u) {
        result &= (1u << (bits_per_pixel & 31u)) - 1u;
    }
    if (s_font_pixel_format_release != NULL) {
        s_font_pixel_format_release(device);
    }
    return result;
}

// FUNCTION: LEGORR 0x00474410
int parse_indexed_surface_name(const char* path, uint32_t* index_out)
{
    const char* component;
    const char* digits;
    uint32_t value;
    unsigned int i;
    if (path == NULL || index_out == NULL) return 0;
    component = path;
    while (*path != '\0') {
        if (*path == '\\' || *path == '/') component = path + 1;
        ++path;
    }
    if (component[0] != 'a' && component[0] != 'A') return 0;
    digits = component + 1;
    value = 0;
    for (i = 0; i < 3; ++i) {
        if (digits[i] < '0' || digits[i] > '9') return 0;
        value = value * 10u + (uint32_t)(digits[i] - '0');
    }
    if (digits[3] != '_') return 0;
    *index_out = value;
    return 1;
}

// FUNCTION: LEGORR 0x004864d0
uint32_t surface_color_pack_to_pixel(float red, float green, float blue)
{
    if (s_surface_pixel_bits == 8) {
        return 0;
    }
    const uint32_t red_value = (uint32_t)float_to_long(red);
    const uint32_t green_value = (uint32_t)float_to_long(green);
    const uint32_t blue_value = (uint32_t)float_to_long(blue);
    const uint32_t red_bits = (uint8_t)s_surface_red_mask_bits;
    const uint32_t green_bits = (uint8_t)s_surface_green_mask_bits;
    const uint32_t blue_bits = (uint8_t)s_surface_blue_mask_bits;
    return ((red_value >> ((8u - red_bits) & 31u))
            << ((green_bits + blue_bits) & 31u))
         | ((green_value >> ((8u - green_bits) & 31u))
            << (blue_bits & 31u))
         | (blue_value >> ((8u - blue_bits) & 31u));
}

/* SurfacePixelValueEncode (FUN_0047d2d0): resolve an RGB color through an
 * indexed palette at 8 bpp; otherwise scale each byte into its channel mask. */
// FUNCTION: LEGORR 0x0047d2d0
uint32_t surface_pixel_value_encode(const SurfacePixelFormat* format,
                                    uint32_t color_rgb)
{
    uint32_t red = (color_rgb >> 16) & 0xffu;
    uint32_t green = (color_rgb >> 8) & 0xffu;
    uint32_t blue = color_rgb & 0xffu;
    if (format == NULL) return 0u;
    if (format->bits_per_pixel == 8u) {
        unsigned int i;
        if (format->palette_rgb == NULL) return 0u;
        for (i = 0; i < 256u; ++i) {
            uint32_t entry = format->palette_rgb[i];
            if (((entry >> 16) & 0xffu) == red &&
                ((entry >> 8) & 0xffu) == green &&
                (entry & 0xffu) == blue) {
                return i;
            }
        }
        return 0u;
    }
    else {
        const uint32_t red_bits = (uint32_t)pop_count32(format->red_mask);
        const uint32_t green_bits = (uint32_t)pop_count32(format->green_mask);
        const uint32_t blue_bits = (uint32_t)pop_count32(format->blue_mask);
        const uint32_t red_shift = find_first_set_bit32(format->red_mask);
        const uint32_t green_shift = find_first_set_bit32(format->green_mask);
        const uint32_t blue_shift = find_first_set_bit32(format->blue_mask);
        return ((red >> ((8u - red_bits) & 31u)) << red_shift) |
               ((green >> ((8u - green_bits) & 31u)) << green_shift) |
               (blue >> ((8u - blue_bits) & 31u) << blue_shift);
    }
}

void projection_overlay_blit_hook_set(ProjectionOverlayBlitHook hook,
                                      void* context)
{
    s_projection_overlay_blit_hook = hook;
    s_projection_overlay_blit_context = context;
}

/* ProjectionOverlayDrawSubmit (FUN_0047ce20): encode the requested fill
 * color for the target surface and submit a color-fill blit, optionally
 * constrained to the supplied rectangle. */
// FUNCTION: LEGORR 0x0047ce20
int projection_overlay_draw_submit(int has_rectangle,
                                   const int rectangle[4],
                                   const SurfacePixelFormat* format,
                                   uint32_t color_rgb)
{
    uint32_t fill_color;
    if (s_projection_overlay_blit_hook == NULL ||
        (has_rectangle != 0 && rectangle == NULL)) return 0;
    fill_color = surface_pixel_value_encode(format, color_rgb);
    return s_projection_overlay_blit_hook(
        s_projection_overlay_blit_context,
        has_rectangle != 0 ? rectangle : NULL,
        fill_color, 0x01000400u);
}

void directdraw_clipper_region_ops_set(const DirectDrawClipperRegionOps* ops)
{
    if (ops != NULL) s_directdraw_clipper_region_ops = *ops;
    else memset(&s_directdraw_clipper_region_ops, 0,
                sizeof(s_directdraw_clipper_region_ops));
}

/* DirectDrawClipperSetRegion (FUN_0047cee0): obtain a clipper, serialize a
 * client rectangle into region data, install it, attach it to the surface,
 * and release the temporary references. Callbacks use HRESULT convention. */
// FUNCTION: LEGORR 0x0047cee0
int directdraw_clipper_set_region(int width, int height)
{
    void* clipper = NULL;
    void* region_data = NULL;
    uint32_t region_size = 0u;
    int success = 0;
    if (s_directdraw_clipper_region_ops.clipper_get == NULL ||
        s_directdraw_clipper_region_ops.region_data_create == NULL ||
        s_directdraw_clipper_region_ops.clipper_region_set == NULL ||
        s_directdraw_clipper_region_ops.surface_clipper_set == NULL) {
        return 0;
    }
    if (s_directdraw_clipper_region_ops.clipper_get(
            s_directdraw_clipper_region_ops.context, &clipper) == 0 &&
        clipper != NULL &&
        s_directdraw_clipper_region_ops.region_data_create(
            s_directdraw_clipper_region_ops.context, width, height,
            &region_data, &region_size) == 0 && region_data != NULL &&
        s_directdraw_clipper_region_ops.clipper_region_set(
            s_directdraw_clipper_region_ops.context, clipper, region_data,
            region_size) == 0 &&
        s_directdraw_clipper_region_ops.surface_clipper_set(
            s_directdraw_clipper_region_ops.context, clipper) == 0) {
        success = 1;
    }
    if (region_data != NULL &&
        s_directdraw_clipper_region_ops.region_data_release != NULL) {
        s_directdraw_clipper_region_ops.region_data_release(
            s_directdraw_clipper_region_ops.context, region_data);
    }
    if (clipper != NULL &&
        s_directdraw_clipper_region_ops.clipper_release != NULL) {
        s_directdraw_clipper_region_ops.clipper_release(
            s_directdraw_clipper_region_ops.context, clipper);
    }
    return success;
}

struct SurfacePixelInfo {
    int width;
    int height;
    int reserved2;
    int reserved3;
    int bits_per_pixel;
    int reserved5;
    int row_stride;
    void* pixels;
};

class SurfacePixelInfoOwner {
public:
    virtual void slot00() = 0; virtual void slot04() = 0;
    virtual void slot08() = 0; virtual void slot0c() = 0;
    virtual void slot10() = 0; virtual void slot14() = 0;
    virtual void slot18() = 0; virtual void slot1c() = 0;
    virtual void slot20() = 0; virtual void slot24() = 0;
    virtual void slot28() = 0; virtual void slot2c() = 0;
    virtual void slot30() = 0; virtual void slot34() = 0;
    virtual void slot38() = 0; virtual void slot3c() = 0;
    virtual void slot40() = 0; virtual void slot44() = 0;
    virtual void slot48() = 0; virtual void slot4c() = 0;
    virtual void slot50() = 0; virtual void slot54() = 0;
    virtual void slot58() = 0; virtual void slot5c() = 0;
    virtual SurfacePixelInfo* get_pixel_info() = 0;
};

// FUNCTION: LEGORR 0x00476bf0
void surface_pixel_rows_flip_in_place(void* surface)
{
    SurfacePixelInfo* info =
        ((SurfacePixelInfoOwner*)surface)->get_pixel_info();
    if (info != NULL) {
        const int row_bits = info->bits_per_pixel * info->width;
        const uint32_t row_bytes = (uint32_t)(
            (row_bits + ((row_bits >> 31) & 7)) >> 3);
        unsigned char* scratch = (unsigned char*)engine_heap_alloc(row_bytes);
        if (scratch != NULL) {
            unsigned char* top = (unsigned char*)info->pixels;
            unsigned char* bottom = top
                + (ptrdiff_t)(info->height - 1) * info->row_stride;
            for (int row = 0; row < info->height / 2; ++row) {
                memcpy(scratch, bottom, row_bytes);
                memcpy(bottom, top, row_bytes);
                memcpy(top, scratch, row_bytes);
                top += info->row_stride;
                bottom -= info->row_stride;
            }
            engine_heap_free(scratch);
        }
    }
}

/* ---- DirectDraw-backed implementation (miniwin) ---------------------- */

#if defined(LEGORRR_BACKEND_MINIWIN)

static LPDIRECTDRAW s_dd(void* p)      { return (LPDIRECTDRAW)p; }
static LPDIRECTDRAWSURFACE s_surf(void* p) { return (LPDIRECTDRAWSURFACE)p; }

int surface_create(DisplaySurface* out, void* hwnd, int w, int h)
{
    LPDIRECTDRAW dd = NULL;
    if (out == NULL || w <= 0 || h <= 0) {
        return -1;
    }
    if (DirectDrawCreate(NULL, &dd, NULL) != DD_OK || dd == NULL) {
        return -2;
    }
    if (dd->SetCooperativeLevel((HWND)hwnd, DDSCL_NORMAL) != DD_OK) {
        return -3;
    }
    DDSURFACEDESC d;
    memset(&d, 0, sizeof(d));
    d.dwSize = sizeof(d);
    d.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    d.dwWidth = (DWORD)w;
    d.dwHeight = (DWORD)h;
    d.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
    LPDIRECTDRAWSURFACE surf = NULL;
    if (dd->CreateSurface(&d, &surf, NULL) != DD_OK || surf == NULL) {
        return -4;
    }
    out->dd = dd;
    out->surf = surf;
    out->hwnd = hwnd;
    out->w = w;
    out->h = h;
    return 0;
}

void surface_destroy(DisplaySurface* s)
{
    if (s == NULL) {
        return;
    }
    if (s->surf != NULL) {
        s_surf(s->surf)->Release();
    }
    if (s->dd != NULL) {
        s_dd(s->dd)->Release();
    }
    s->dd = NULL;
    s->surf = NULL;
}

int surface_width(DisplaySurface* s)  { return s != NULL ? s->w : 0; }
int surface_height(DisplaySurface* s) { return s != NULL ? s->h : 0; }

int surface_lock(DisplaySurface* s, void** pixels, int* pitch)
{
    if (s == NULL || s->surf == NULL || pixels == NULL || pitch == NULL) {
        return -1;
    }
    DDSURFACEDESC d;
    memset(&d, 0, sizeof(d));
    d.dwSize = sizeof(d);
    if (s_surf(s->surf)->Lock(NULL, &d, DDLOCK_WAIT | DDLOCK_WRITEONLY,
                              NULL) != DD_OK) {
        return -2;
    }
    *pixels = d.lpSurface;
    *pitch = d.lPitch;
    return 0;
}

void surface_unlock(DisplaySurface* s, void* pixels)
{
    if (s == NULL || s->surf == NULL) {
        return;
    }
    s_surf(s->surf)->Unlock(pixels);
}

static void surface_restore_from_primary_impl(DisplaySurface* destination,
                                              DisplaySurface* primary,
                                              int enabled)
{
    if (enabled != 0 && destination != NULL && destination->surf != NULL &&
        primary != NULL && primary->surf != NULL) {
        s_surf(destination->surf)->Blt(NULL, s_surf(primary->surf), NULL,
                                       DDBLT_WAIT, NULL);
    }
}

static void windowed_surface_blit_impl(DisplaySurface* primary,
                                      DisplaySurface* back_surface)
{
    RECT destination_rect;
    RECT source_rect;
    POINT origin;
    if (primary == NULL || primary->surf == NULL || primary->hwnd == NULL ||
        back_surface == NULL || back_surface->surf == NULL) return;
    destination_rect.left = 0;
    destination_rect.top = 0;
    destination_rect.right = primary->w;
    destination_rect.bottom = primary->h;
    source_rect.left = 0;
    source_rect.top = 0;
    source_rect.right = back_surface->w;
    source_rect.bottom = back_surface->h;
    origin.x = 0;
    origin.y = 0;
    ClientToScreen((HWND)primary->hwnd, &origin);
    OffsetRect(&destination_rect, origin.x, origin.y);
    s_surf(primary->surf)->Blt(&destination_rect, s_surf(back_surface->surf),
                               &source_rect, DDBLT_WAIT, NULL);
}

static int surface_directdraw_pixel_format_query(DisplaySurface* s,
                                                 SurfacePixelFormat* out)
{
    DDPIXELFORMAT format;
    if (s == NULL || s->surf == NULL || out == NULL) return 0;
    memset(&format, 0, sizeof(format));
    format.dwSize = sizeof(format);
    if (s_surf(s->surf)->GetPixelFormat(&format) != DD_OK) return 0;
    out->bits_per_pixel = format.dwRGBBitCount;
    out->red_mask = format.dwRBitMask;
    out->green_mask = format.dwGBitMask;
    out->blue_mask = format.dwBBitMask;
    out->palette_rgb = NULL;
    return 1;
}

static int surface_directdraw_description_query(DisplaySurface* s,
                                                SurfaceDescriptionInfo* out)
{
    DDSURFACEDESC d;
    if (s == NULL || s->surf == NULL || out == NULL) return 0;
    memset(&d, 0, sizeof(d));
    d.dwSize = sizeof(d);
    if (s_surf(s->surf)->GetSurfaceDesc(&d) != DD_OK) return 0;
    out->width = d.dwWidth;
    out->height = d.dwHeight;
    out->pitch = d.lPitch;
    out->pixels = d.lpSurface;
    out->bits_per_pixel = d.ddpfPixelFormat.dwRGBBitCount;
    return 1;
}

#else  /* !LEGORRR_BACKEND_MINIWIN — winapi stub (TODO reference build) */ 

int surface_create(DisplaySurface* out, void* hwnd, int w, int h)
{
    (void)out; (void)hwnd; (void)w; (void)h;
    return -5;
}
void surface_destroy(DisplaySurface* s) { (void)s; }
int surface_width(DisplaySurface* s)  { (void)s; return 0; }
int surface_height(DisplaySurface* s) { (void)s; return 0; }
int surface_lock(DisplaySurface* s, void** pixels, int* pitch)
{
    (void)s; (void)pixels; (void)pitch;
    return -5;
}
void surface_unlock(DisplaySurface* s, void* pixels)
{
    (void)s; (void)pixels;
}
static void surface_restore_from_primary_impl(DisplaySurface* destination,
                                              DisplaySurface* primary,
                                              int enabled)
{
    (void)destination; (void)primary; (void)enabled;
}
static void windowed_surface_blit_impl(DisplaySurface* primary,
                                       DisplaySurface* back_surface)
{
    (void)primary; (void)back_surface;
}
static int surface_directdraw_description_query(DisplaySurface* surface,
                                                SurfaceDescriptionInfo* out)
{
    (void)surface; (void)out;
    return 0;
}
static int surface_directdraw_pixel_format_query(DisplaySurface* surface,
                                                 SurfacePixelFormat* out)
{
    (void)surface; (void)out;
    return 0;
}

#endif /* LEGORRR_BACKEND_MINIWIN */

/* ---- backend-neutral pixel helpers ------------------------------------- */

/* RenderSurfaceRestoreFromPrimary (FUN_0047cc20): restore the current
 * windowed back surface by copying the primary surface when enabled. */
// FUNCTION: LEGORR 0x0047cc20
void render_surface_restore_from_primary(DisplaySurface* destination,
                                         DisplaySurface* primary,
                                         int restore_enabled)
{
    surface_restore_from_primary_impl(destination, primary, restore_enabled);
}

/* WindowedRenderSurfaceBlit (FUN_0047cc50): place the back surface at the
 * window's client origin on the primary surface and wait for the blit. */
// FUNCTION: LEGORR 0x0047cc50
void windowed_render_surface_blit(DisplaySurface* primary,
                                  DisplaySurface* back_surface)
{
    windowed_surface_blit_impl(primary, back_surface);
}

/* DirectDrawSurfaceDescriptionGet (FUN_0047de60): retrieve the active
 * surface descriptor through the backend-specific DirectDraw query. */
// FUNCTION: LEGORR 0x0047de60
int surface_directdraw_description_get(DisplaySurface* surface,
                                       SurfaceDescriptionInfo* out)
{
    return surface_directdraw_description_query(surface, out);
}

/* DirectDrawSurfacePixelFormatGet (FUN_0047def0): fetch the active
 * surface's depth and RGB masks through its DirectDraw pixel format. */
// FUNCTION: LEGORR 0x0047def0
int surface_directdraw_pixel_format_get(DisplaySurface* surface,
                                        SurfacePixelFormat* out)
{
    return surface_directdraw_pixel_format_query(surface, out);
}

/* SurfaceDescriptionGet (FUN_0047cdd0): retrieve the active surface's
 * dimensions; this backend-neutral record already stores the queried values. */
// FUNCTION: LEGORR 0x0047cdd0
int surface_description_get(const DisplaySurface* surface,
                            int* width_out, int* height_out)
{
    if (surface == NULL || width_out == NULL || height_out == NULL ||
        surface->w <= 0 || surface->h <= 0) {
        return 0;
    }
    *width_out = surface->w;
    *height_out = surface->h;
    return 1;
}

/* DirectDrawSurfacePixelRead (FUN_0047dfa0): read one in-bounds pixel from
 * the port's 32-bit locked display surface. The RGBA byte layout yields the
 * native little-endian packed word consumed by the renderer. */
// FUNCTION: LEGORR 0x0047dfa0
int surface_directdraw_pixel_read(DisplaySurface* surface, unsigned int x,
                                  unsigned int y, uint32_t* pixel_out)
{
    void* pixels = NULL;
    int pitch = 0;
    uint32_t pixel;
    if (surface == NULL || pixel_out == NULL || surface->w <= 0 ||
        surface->h <= 0 || x >= (unsigned int)surface->w ||
        y >= (unsigned int)surface->h || surface_lock(surface, &pixels,
                                                       &pitch) != 0 ||
        pixels == NULL || pitch < surface->w * 4) {
        return 0;
    }
    memcpy(&pixel, (const unsigned char*)pixels + (size_t)y * pitch +
           (size_t)x * 4u, sizeof(pixel));
    surface_unlock(surface, pixels);
    *pixel_out = pixel;
    return 1;
}

void surface_fill(void* px, int pitch, int w, int h, int r, int g, int b)
{
    if (px == NULL) {
        return;
    }
    for (int y = 0; y < h; ++y) {
        unsigned char* row = (unsigned char*)px + (size_t)y * pitch;
        for (int x = 0; x < w; ++x) {
            row[x * 4 + 0] = (unsigned char)r;
            row[x * 4 + 1] = (unsigned char)g;
            row[x * 4 + 2] = (unsigned char)b;
            row[x * 4 + 3] = 0xff;
        }
    }
}

void surface_blit_texture(void* px, int pitch, int w, int h,
                          const TextureSurface* t, int x, int y)
{
    if (px == NULL || t == NULL || t->pixels == NULL) {
        return;
    }
    for (int yy = 0; yy < t->height; ++yy) {
        int dy = y + yy;
        if (dy < 0 || dy >= h) {
            continue;
        }
        unsigned char* row = (unsigned char*)px + (size_t)dy * pitch;
        for (int xx = 0; xx < t->width; ++xx) {
            int dx = x + xx;
            if (dx < 0 || dx >= w) {
                continue;
            }
            uint32_t p = t->pixels[yy * t->width + xx];
            row[dx * 4 + 0] = (unsigned char)(p >> 16); /* R */
            row[dx * 4 + 1] = (unsigned char)(p >> 8);  /* G */
            row[dx * 4 + 2] = (unsigned char)(p);       /* B */
            row[dx * 4 + 3] = (unsigned char)(p >> 24); /* A */
        }
    }
}

void surface_draw_sweep(void* px, int pitch, int w, int h, int frame)
{
    if (px == NULL) {
        return;
    }
    int bar_x = (frame * 4) % w;
    int bar_w = 40;
    for (int y = 0; y < h; ++y) {
        unsigned char* row = (unsigned char*)px + (size_t)y * pitch;
        for (int x = bar_x; x < bar_x + bar_w && x < w; ++x) {
            row[x * 4 + 0] = 0xff; /* R */
            row[x * 4 + 1] = 0x00; /* G */
            row[x * 4 + 2] = 0xff; /* B */
            row[x * 4 + 3] = 0xff; /* A */
        }
    }
}
/* RGBChannelsPack (FUN_0047db90, findings 23/431): the corpus's CONCAT
 * chain drops the three bytes little-endian — argument one in bits 0-7,
 * two in 8-15, three in 16-23, top byte zero. Callers (the palette scan
 * of finding 23) feed the bytes in memory order, so the word is the
 * 0x00BBGGRR triple as stored. */
// FUNCTION: LEGORR 0x0047db90
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) uint32_t rgb_channels_pack(uint8_t red, uint8_t green,
                                             uint8_t blue)
{
    __asm {
        mov al, byte ptr [esp + 4]
        mov cl, byte ptr [esp + 8]
        mov dl, byte ptr [esp + 0x0c]
        mov byte ptr [esp + 4], al
        mov byte ptr [esp + 5], cl
        mov byte ptr [esp + 6], dl
        mov byte ptr [esp + 7], 0
        mov eax, dword ptr [esp + 4]
        ret
    }
}
#else
uint32_t rgb_channels_pack(uint8_t red, uint8_t green, uint8_t blue)
{
    return (uint32_t)red | ((uint32_t)green << 8) | ((uint32_t)blue << 16);
}
#endif

/* RGBChannelsUnpack (FUN_0047e2d0): optional outputs receive the packed
 * word's first three bytes, preserving the channel order used by the packer. */
// FUNCTION: LEGORR 0x0047e2d0
void rgb_channels_unpack(uint32_t color, uint8_t* red, uint8_t* green,
                         uint8_t* blue)
{
    if (red != NULL) {
        *red = (uint8_t)color;
    }
    if (green != NULL) {
        *green = (uint8_t)(color >> 8);
    }
    if (blue != NULL) {
        *blue = (uint8_t)(color >> 16);
    }
}

/* SurfacePixelRowsFlipWithScratch (FUN_0047D740) — see surface.hpp. The
 * corpus walks the row pairs from the outside in, three stage copies per
 * pair (top row to the scratch, bottom row onto the top, scratch onto the
 * bottom), each stage (row_bytes/2) DWORDs plus (even-rounded row_bytes) & 3
 * trailing bytes, advancing both cursors by the even-rounded row_bytes.
 * The dword/tail split sums to exactly that, so the port copies the whole
 * even span per stage. Deviations: NULL record/pixels return (the corpus
 * strides its surface blindly) and a non-positive row count skips the walk. */
// FUNCTION: LEGORR 0x0047d740
void surface_pixel_rows_flip_with_scratch(SurfaceRecord* rec)
{
    if (rec == NULL || rec->pixels == NULL) {
        return; /* deviation: the corpus strides the surface blindly */
    }
    const int half = rec->row_bytes / 2;          /* corpus's uVar2 */
    const long span = (long)half * 2;             /* even-rounded row_bytes */
    char* const first = (char*)rec->pixels;
    char* const last = first + (ptrdiff_t)(rec->row_count - 1) * span;
    char* scratch = (char*)engine_heap_alloc(span);
    uint32_t lap = 0u;
    if (rec->row_count > 1) { /* deviation: guarded; corpus tests (rows & ~1) */
        char* top = first;
        char* bottom = last;
        for (;;) {
            memcpy(scratch, top, (size_t)span);              /* A -> scratch */
            memcpy(top, bottom, (size_t)span);               /* B -> A */
            memcpy(bottom, scratch, (size_t)span);           /* scratch -> B */
            lap = lap + 1u;
            top = top + span;
            bottom = bottom - span;
            if (lap >= (uint32_t)((uint32_t)rec->row_count >> 1)) {
                break;
            }
        }
    }
    engine_heap_free(scratch);
}

/* Palette8To16BitSurfaceExpand (FUN_0047D520) — see surface.hpp. The corpus
 * reads the depth at +0x54: 8/24/32 do no work and answer 0, 16-bit runs,
 * any other depth answers 1 untouched. The 16-bit walk: for every source
 * byte (a palette index) the entry's three bytes land in their mask's field
 * as ((byte >> (8 - popcount(mask))) << first_set(mask)), OR'd into the
 * stored word — byte 0 rides the +0x58 mask, byte 1 the +0x5c mask, byte 2
 * the +0x60 mask, the entries four bytes apart. Rows then flip. Deviations:
 * the blind record/job/source strides are guarded. */
// FUNCTION: LEGORR 0x0047d520
int surface_palette8_to_16bit_surface_expand(SurfaceRecord* rec,
                                             const SurfaceConvertJob* job)
{
    if (rec == NULL || job == NULL || rec->pixels == NULL) {
        return 1; /* deviation: the corpus strides the record blindly */
    }
    if (rec->depth == 8 || rec->depth == 24 || rec->depth == 32) {
        return 0;
    }
    if (rec->depth != 16) {
        return 1;
    }
    if (job->src == NULL || job->palette == NULL) {
        return 1; /* deviation: the corpus strides the job blindly */
    }
    const uint32_t mask_low = rec->mask_low;
    const uint32_t mask_mid = rec->mask_mid;
    const uint32_t mask_hi = rec->mask_hi;
    const uint32_t ffs_low = find_first_set_bit32(mask_low);
    const uint32_t ffs_mid = find_first_set_bit32(mask_mid);
    const uint32_t ffs_hi = find_first_set_bit32(mask_hi);
    const uint32_t shr_low = (uint32_t)(8 - pop_count32(mask_low)) & 0x1fu;
    const uint32_t shr_mid = (uint32_t)(8 - pop_count32(mask_mid)) & 0x1fu;
    const uint32_t shr_hi = (uint32_t)(8 - pop_count32(mask_hi)) & 0x1fu;
    const char* src = (const char*)job->src;
    const char* palette = (const char*)job->palette;
    uint16_t* dst = (uint16_t*)rec->pixels;
    for (int y = 0; y < job->height; ++y) {
        for (int x = 0; x < job->width; ++x) {
            const uint32_t index = (uint8_t)*src++;
            const char* entry = palette + index * 4u;
            const uint16_t word = (uint16_t)(
                ((uint32_t)((uint8_t)entry[0] >> shr_low) << (ffs_low & 0x1fu))
                | ((uint32_t)((uint8_t)entry[1] >> shr_mid)
                   << (ffs_mid & 0x1fu))
                | ((uint32_t)((uint8_t)entry[2] >> shr_hi) << (ffs_hi & 0x1fu)));
            *dst++ = word;
        }
        src += job->src_stride - job->width;
        dst += (size_t)((rec->row_bytes >> 1) - job->width);
    }
    surface_pixel_rows_flip_with_scratch(rec);
    return 1;
}

/* PalettizedBitmapSurfaceDecode (FUN_0047d000): expand 8-bit palette
 * indices into a mask-packed 16-bit surface, validating dimensions and
 * honoring both locked-surface strides. */
// FUNCTION: LEGORR 0x0047d000
int palettized_bitmap_surface_decode(const PalettizedSurfaceDecodeJob* job)
{
    uint32_t red_bits;
    uint32_t green_bits;
    uint32_t blue_bits;
    uint32_t red_shift;
    uint32_t green_shift;
    uint32_t blue_shift;
    uint32_t y;
    if (job == NULL || job->source == NULL || job->palette_rgb == NULL ||
        job->destination == NULL || job->width == 0u || job->height == 0u ||
        job->source_stride < job->width ||
        job->destination_stride_pixels < job->width) return 0;
    red_bits = (uint32_t)pop_count32(job->red_mask);
    green_bits = (uint32_t)pop_count32(job->green_mask);
    blue_bits = (uint32_t)pop_count32(job->blue_mask);
    red_shift = find_first_set_bit32(job->red_mask);
    green_shift = find_first_set_bit32(job->green_mask);
    blue_shift = find_first_set_bit32(job->blue_mask);
    for (y = 0u; y < job->height; ++y) {
        const uint8_t* source_row = job->source + y * job->source_stride;
        uint16_t* destination_row = job->destination +
            y * job->destination_stride_pixels;
        uint32_t x;
        for (x = 0u; x < job->width; ++x) {
            uint32_t color = job->palette_rgb[source_row[x]];
            uint32_t red = (color >> 16) & 0xffu;
            uint32_t green = (color >> 8) & 0xffu;
            uint32_t blue = color & 0xffu;
            uint32_t packed =
                ((red >> ((8u - red_bits) & 31u)) << red_shift) |
                ((green >> ((8u - green_bits) & 31u)) << green_shift) |
                ((blue >> ((8u - blue_bits) & 31u)) << blue_shift);
            destination_row[x] = (uint16_t)packed;
        }
    }
    return 1;
}

/* Bitmap24BitSurfaceConvert (FUN_0047D800) — see surface.hpp. The 24-bit
 * sibling: same depth verdict, same field placement per mask, but the three
 * source bytes per pixel ride the masks REVERSED — byte 0 lands in the
 * +0x60 (high) field, byte 1 in +0x5c, byte 2 in +0x58 — and rows advance
 * by the source stride over three-byte pixels. Deviations and the flip tail
 * ride the palette walk's notes. */
// FUNCTION: LEGORR 0x0047d800
int surface_bitmap24_bit_surface_convert(SurfaceRecord* rec,
                                         const SurfaceConvertJob* job)
{
    if (rec == NULL || job == NULL || rec->pixels == NULL) {
        return 1; /* deviation: the corpus strides the record blindly */
    }
    if (rec->depth == 8 || rec->depth == 24 || rec->depth == 32) {
        return 0;
    }
    if (rec->depth != 16) {
        return 1;
    }
    if (job->src == NULL) {
        return 1; /* deviation: the corpus strides the job blindly */
    }
    const uint32_t mask_low = rec->mask_low;
    const uint32_t mask_mid = rec->mask_mid;
    const uint32_t mask_hi = rec->mask_hi;
    const uint32_t ffs_low = find_first_set_bit32(mask_low);
    const uint32_t ffs_mid = find_first_set_bit32(mask_mid);
    const uint32_t ffs_hi = find_first_set_bit32(mask_hi);
    const uint32_t shr_low = (uint32_t)(8 - pop_count32(mask_low)) & 0x1fu;
    const uint32_t shr_mid = (uint32_t)(8 - pop_count32(mask_mid)) & 0x1fu;
    const uint32_t shr_hi = (uint32_t)(8 - pop_count32(mask_hi)) & 0x1fu;
    const char* src = (const char*)job->src;
    uint16_t* dst = (uint16_t*)rec->pixels;
    for (int y = 0; y < job->height; ++y) {
        for (int x = 0; x < job->width; ++x) {
            const uint32_t b0 = (uint8_t)src[0];
            const uint32_t b1 = (uint8_t)src[1];
            const uint32_t b2 = (uint8_t)src[2];
            src += 3;
            const uint16_t word = (uint16_t)(
                ((b0 >> shr_hi) << (ffs_hi & 0x1fu))
                | ((b1 >> shr_mid) << (ffs_mid & 0x1fu))
                | ((b2 >> shr_low) << (ffs_low & 0x1fu)));
            *dst++ = word;
        }
        src += job->src_stride - 3 * job->width;
        dst += (size_t)((rec->row_bytes >> 1) - job->width);
    }
    surface_pixel_rows_flip_with_scratch(rec);
    return 1;
}

/* SurfacePixelFormatQuery (FUN_0047d490): dispatch the source formats that
 * need conversion; other successful format queries require no pixel walk. */
// FUNCTION: LEGORR 0x0047d490
int surface_pixel_format_query_convert(SurfaceRecord* rec,
                                       const SurfaceConvertJob* job,
                                       uint32_t source_bits_per_pixel)
{
    if (rec == NULL || job == NULL) return 0;
    if (source_bits_per_pixel == 8u) {
        (void)surface_palette8_to_16bit_surface_expand(rec, job);
    }
    else if (source_bits_per_pixel == 24u) {
        (void)surface_bitmap24_bit_surface_convert(rec, job);
    }
    return 1;
}
