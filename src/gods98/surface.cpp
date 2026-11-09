// SPDX-License-Identifier: LGPL-3.0-or-later
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "platform/platform.h"
#include "surface.hpp"
#include "model.hpp"
#include "record.hpp"

#include "crt/crt.hpp"
#include "engine.hpp"
#include "heap.hpp"
#include "texture.hpp"
#include "view.hpp"
#include "frameloop.hpp"
#include "scene_resources.hpp"

#if defined(_MSC_VER)
#define SURFACE_COM_CALL __stdcall
#elif defined(__i386__) && (defined(__GNUC__) || defined(__clang__))
#define SURFACE_COM_CALL __attribute__((stdcall))
#else
#define SURFACE_COM_CALL
#endif

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
static TextureSurfaceCreateHook s_texture_surface_create_hook;
static void* s_texture_surface_create_context;
static DisplaySurface* s_active_render_target;
static void* s_screen_bounded_surface_renderer;
static void* s_screen_bounded_surface_device;

typedef uint32_t (SURFACE_COM_CALL *SurfaceDeviceDimensionFn)(void* device);
typedef int32_t (SURFACE_COM_CALL *Direct3DRMCreateViewportFn)(
    void* renderer, void* device, void* camera, int32_t x, int32_t y,
    int32_t width, int32_t height, void** viewport_out);
typedef int32_t (SURFACE_COM_CALL *Direct3DRMSetAppDataFn)(
    void* viewport, uintptr_t appdata);

static void** surface_interface_vtable(void* object)
{
    if (object == NULL) return NULL;
    return *(void***)object;
}

static int surface_screen_dimensions_get(uint32_t* width_out,
                                         uint32_t* height_out)
{
    void** vtable;
    SurfaceDeviceDimensionFn get_width;
    SurfaceDeviceDimensionFn get_height;
    if (s_screen_bounded_surface_device == NULL || width_out == NULL ||
        height_out == NULL) return 0;
    vtable = surface_interface_vtable(s_screen_bounded_surface_device);
    if (vtable == NULL || vtable[27] == NULL || vtable[26] == NULL)
        return 0;
    get_width = (SurfaceDeviceDimensionFn)vtable[27];
    get_height = (SurfaceDeviceDimensionFn)vtable[26];
    *width_out = get_width(s_screen_bounded_surface_device);
    *height_out = get_height(s_screen_bounded_surface_device);
    return 1;
}

void surface_active_render_target_set(DisplaySurface* surface)
{
    s_active_render_target = surface;
}

DisplaySurface* surface_active_render_target_get(void)
{
    return s_active_render_target;
}

void screen_bounded_surface_device_set(void* renderer, void* device)
{
    s_screen_bounded_surface_renderer = renderer;
    s_screen_bounded_surface_device = device;
}

/* ScreenBoundedSurfaceResourceAllocate (FUN_00476e50): negative positions
 * are measured from the far edge; reject rectangles that do not fit, then
 * pop a pooled record and attach the created surface to it. */
ScreenBoundedSurfaceRecord* screen_bounded_surface_allocate(
    uint32_t width, uint32_t height, void* camera, int x, int y)
{
    uint32_t screen_width, screen_height;
    uint32_t normalized_x, normalized_y;
    ScreenBoundedSurfaceRecord* record;
    if (s_screen_bounded_surface_renderer == NULL ||
        s_screen_bounded_surface_device == NULL || camera == NULL)
        return NULL;
    if (!surface_screen_dimensions_get(&screen_width, &screen_height))
        return NULL;
    if (screen_width > 0x7fffffffu || screen_height > 0x7fffffffu)
        return NULL;
    if (x < 0) x += (int)screen_width;
    if (y < 0) y += (int)screen_height;
    if (x < 0 || y < 0) return NULL;
    normalized_x = (uint32_t)x;
    normalized_y = (uint32_t)y;
    if ((uint64_t)normalized_x + width > screen_width ||
        (uint64_t)normalized_y + height > screen_height) return NULL;

    record = (ScreenBoundedSurfaceRecord*)ddiscene_render_surface_pool_acquire();
    if (record == NULL) return NULL;
    {
        void** vtable = surface_interface_vtable(s_screen_bounded_surface_renderer);
        Direct3DRMCreateViewportFn create_viewport;
        int32_t result;
        if (vtable == NULL || vtable[20] == NULL) return NULL;
        create_viewport = (Direct3DRMCreateViewportFn)vtable[20];
        result = create_viewport(
            s_screen_bounded_surface_renderer,
            s_screen_bounded_surface_device, camera,
            (int32_t)normalized_x, (int32_t)normalized_y,
            (int32_t)width, (int32_t)height, &record->viewport);
        if (result != 0) return NULL;
        if (record->viewport == NULL) return NULL;
        vtable = surface_interface_vtable(record->viewport);
        if (vtable != NULL && vtable[6] != NULL) {
            Direct3DRMSetAppDataFn set_appdata =
                (Direct3DRMSetAppDataFn)vtable[6];
            (void)set_appdata(record->viewport, (uintptr_t)record);
        }
        record->appdata = 0u;
    }
    return record;
}

/* RenderTargetViewportScaleSet (FUN_00476dc0): its actual stack ABI has four
 * normalized origin/size floats followed by a camera pointer. GameConfigInit
 * passes (0, 0, 1, 1, camera); convert x/width against device width and
 * y/height against device height before allocating the viewport. */
// FUNCTION: LEGORR 0x00476dc0
ScreenBoundedSurfaceRecord* render_target_viewport_scale_set(
    float x, float y, float width_scale, float height_scale, void* camera)
{
    uint32_t screen_width;
    uint32_t screen_height;
    float pixel_x;
    float pixel_y;
    float pixel_width;
    float pixel_height;
    if (s_screen_bounded_surface_device == NULL || camera == NULL)
        return NULL;
    if (!surface_screen_dimensions_get(&screen_width, &screen_height))
        return NULL;
    if (screen_width == 0u || screen_height == 0u) return NULL;
    pixel_x = x * (float)screen_width;
    pixel_y = y * (float)screen_height;
    pixel_width = width_scale * (float)screen_width;
    pixel_height = height_scale * (float)screen_height;
    if (pixel_x != pixel_x || pixel_y != pixel_y ||
        pixel_width != pixel_width || pixel_height != pixel_height ||
        pixel_x < -2147483648.0f || pixel_x > 2147483520.0f ||
        pixel_y < -2147483648.0f || pixel_y > 2147483520.0f ||
        pixel_width <= 0.0f || pixel_width > 2147483520.0f ||
        pixel_height <= 0.0f || pixel_height > 2147483520.0f)
        return NULL;
    return screen_bounded_surface_allocate(
        (uint32_t)crt_float_to_long(pixel_width),
        (uint32_t)crt_float_to_long(pixel_height), camera,
        crt_float_to_long(pixel_x), crt_float_to_long(pixel_y));
}

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
// FUNCTION: LEGORR 0x00482220
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
    uint32_t old_count;
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
        pool->count = 0;
        slot_bytes = (size_t)pool->capacity * sizeof(MeshRenderRecord);
        pool->records = (MeshRenderRecord*)allocate(pool->context, slot_bytes);
        if (pool->records == NULL) {
            pool->count = 0;
            pool->capacity = 0;
            return -1;
        }
        index = 0;
        old_count = 0;
    }
    else {
        if (pool->count > pool->capacity) return -1;
        old_count = pool->count;
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

    if (pool->count == 0u) pool->count = 1u;
    record = &pool->records[index];
    memset(record, 0, sizeof(*record));
    {
        /* FUN_00483170 initializes the eighty-byte D3DMATERIAL beginning
         * at record +0x10: size0x50, diffuse RGBA1, then zeros. On
         * the 32-bit reference build `reserved` starts at +0x10. On wider
         * hosts it remains an adapter payload after the host pointers. */
        static const uint32_t initial_state[20] = {
            0x50u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u,
            0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u,
            0u, 0u
        };
        memcpy(record->reserved, initial_state, sizeof(initial_state));
    }
    index_bytes = (size_t)index_count * sizeof(uint16_t);
    record->indices = index_bytes != 0 ?
        (uint16_t*)allocate(pool->context, index_bytes) : NULL;
    if (index_bytes != 0 && record->indices == NULL) {
        memset(record, 0, sizeof(*record));
        pool->count = old_count;
        return -1;
    }
    record->index_count = index_count;
    for (i = 0; i < index_count; ++i)
        record->indices[i] = source_indices[i * 2u];

    vertex_bytes = (size_t)vertex_count * 32u;
    record->vertices = vertex_bytes != 0 ?
        allocate(pool->context, vertex_bytes) : NULL;
    if (vertex_bytes != 0 && record->vertices == NULL) {
        if (record->indices != NULL) release(pool->context, record->indices);
        memset(record, 0, sizeof(*record));
        pool->count = old_count;
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
// FUNCTION: LEGORR 0x004748f0
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

static int model_texture_frame_name(const char* name, uint32_t frame,
                                    char output[260])
{
    size_t length;
    size_t extension;
    size_t digits;
    size_t prefix;
    size_t extension_length;
    uint32_t value;
    uint32_t width;
    uint32_t number_width;
    char number[16];
    uint32_t number_length = 0u;
    uint32_t i;
    if (name == NULL) return 0;
    length = strlen(name);
    if (length == 0u || length >= 260u) return 0;
    extension = length;
    for (i = 0u; i < length; ++i) {
        if (name[i] == '.') extension = i;
        if (name[i] == '/' || name[i] == '\\') {
            if (extension < i) extension = length;
        }
    }
    digits = extension;
    while (digits > 0u && name[digits - 1u] >= '0' &&
           name[digits - 1u] <= '9') --digits;
    /* StringTrailingDigitsSplit @ 0x00481ac0 returns NULL when the digit
     * run reaches the start of the basename; retail requires a prefix. */
    if (digits == 0u || digits == extension) return 0;
    prefix = digits;
    width = (uint32_t)(extension - digits);
    value = 0u;
    for (i = (uint32_t)digits; i < (uint32_t)extension; ++i) {
        uint32_t digit = (uint32_t)(name[i] - '0');
        if (value > (0xffffffffu - digit) / 10u) return 0;
        value = value * 10u + digit;
    }
    if (value > 0xffffffffu - frame) return 0;
    value += frame;
    do {
        number[number_length++] = (char)('0' + value % 10u);
        value /= 10u;
    } while (value != 0u && number_length < sizeof(number));
    extension_length = length - extension;
    number_width = width > number_length ? width : number_length;
    if (prefix + number_width + extension_length >= 260u) return 0;
    memcpy(output, name, prefix);
    for (i = number_length; i < number_width; ++i)
        output[prefix + (i - number_length)] = '0';
    for (i = 0u; i < number_length; ++i)
        output[prefix + (number_width - number_length) + i] =
            number[number_length - i - 1u];
    memcpy(output + prefix + number_width, name + extension, extension_length);
    output[prefix + number_width + extension_length] = '\0';
    return 1;
}

static void model_texture_binding_load_path(ModelTextureBinding* binding,
                                            const char* name,
                                            int animated)
{
    TextureSurface* texture;
    if (!animated) {
        binding->texture = texture_get_or_load(NULL, name);
        binding->texture_surface_owned = binding->texture != NULL;
        return;
    }
    binding->animation_frames = (void**)malloc(100u * sizeof(void*));
    if (binding->animation_frames == NULL) return;
    binding->animation_array_owned = 1u;
    binding->texture_surface_owned = 1u;
    while (binding->animation_frame_count < 100u) {
        char frame_name[260];
        uint32_t frame = binding->animation_frame_count;
        if (!model_texture_frame_name(name, frame, frame_name)) break;
        texture = texture_get_or_load(NULL, frame_name);
        if (texture == NULL) break;
        binding->animation_frames[frame] = texture;
        ++binding->animation_frame_count;
    }
    if (binding->animation_frame_count == 0u) {
        free(binding->animation_frames);
        binding->animation_frames = NULL;
    }
    else {
        void** frames = (void**)realloc(
            binding->animation_frames,
            (size_t)binding->animation_frame_count * sizeof(void*));
        if (frames != NULL) binding->animation_frames = frames;
    }
}

/* Portable texture-binding adapter: choose source colors versus texture
 * binding per input, expand numbered texture sequences (up to 100 frames),
 * then copy each input's render parameters into its binding record. */
int bind_model_texture_inputs(void* device, const ModelTextureInput* inputs,
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
                    if (binding->texture != NULL)
                        binding->texture_callback_owned = 1u;
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
                            binding->texture_callback_owned = 1u;
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
            else if (input->image_name != NULL &&
                     input->image_name[0] != '\0') {
                model_texture_binding_load_path(binding, input->image_name,
                                                input->animated_name != 0u);
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

/* Parse the filename as VC5's _splitpath does. The MSVC build resolves its
 * target entry to splitpat.obj; this bounded ASCII fallback serves other hosts. */
static void model_asset_path_components_parse(const char* path,
                                              char drive[3],
                                              char directory[256],
                                              char basename[256],
                                              char extension[256])
{
    const char* start;
    const char* end;
    const char* separator = NULL;
    const char* dot = NULL;
    size_t count;
    drive[0] = '\0';
    directory[0] = '\0';
    basename[0] = '\0';
    extension[0] = '\0';
    if (path == NULL) return;
#if defined(_MSC_VER)
    /* The MSVC build links this target entry to VC5's splitpat.obj. */
    _splitpath(path, drive, directory, basename, extension);
#else
    start = path;
    if (path[0] != '\0' && path[1] == ':') {
        drive[0] = path[0]; drive[1] = ':'; drive[2] = '\0';
        start += 2;
    }
    for (end = start; *end != '\0'; ++end) {
        if (*end == '/' || *end == '\\') separator = end;
    }
    for (end = separator != NULL ? separator + 1 : start;
         *end != '\0'; ++end) {
        if (*end == '.') dot = end;
    }
    if (separator != NULL) {
        count = (size_t)(separator + 1 - start);
        if (count > 255u) count = 255u;
        memcpy(directory, start, count);
        directory[count] = '\0';
    }
    start = separator != NULL ? separator + 1 : start;
    end = dot != NULL ? dot : path + strlen(path);
    count = (size_t)(end - start);
    if (count > 255u) count = 255u;
    memcpy(basename, start, count);
    basename[count] = '\0';
    if (dot != NULL) {
        count = strlen(dot);
        if (count > 255u) count = 255u;
        memcpy(extension, dot, count);
        extension[count] = '\0';
    }
#endif
}

static int model_texture_name_build(const char* path_prefix,
                                    const char* image_name,
                                    char output[260])
{
    char drive[3];
    char directory[256];
    char basename[256];
    char extension[256];
    size_t length = 0u;
    size_t i;
    model_asset_path_components_parse(image_name, drive, directory,
                                      basename, extension);
    for (i = 0u; basename[i] != '\0'; ++i) {
        if (length + 1u >= 260u) return 0;
        output[length++] = basename[i];
    }
    for (i = 0u; extension[i] != '\0'; ++i) {
        if (length + 1u >= 260u) return 0;
        output[length++] = extension[i];
    }
    output[length] = '\0';
    (void)directory; /* the reference discards TIMG's parsed directory */
    (void)drive;     /* the caller's model path prefix owns the lookup root */
    (void)path_prefix;
    return 1;
}

static int model_texture_path_join(const char* prefix, const char* name,
                                   char output[260])
{
    size_t length = 0u;
    size_t i;
    if (prefix != NULL) {
        for (i = 0u; prefix[i] != '\0'; ++i) {
            if (length + 1u >= 260u) return 0;
            output[length++] = prefix[i];
        }
    }
    for (i = 0u; name[i] != '\0'; ++i) {
        if (length + 1u >= 260u) return 0;
        output[length++] = name[i];
    }
    output[length] = '\0';
    return 1;
}

static void* model_texture_lookup(const char* path_prefix, const char* name,
                                  ModelTextureBinding* binding)
{
    char resolved_name[260];
    void* texture;
    if (!model_texture_path_join(path_prefix, name, resolved_name))
        return NULL;
    if (s_model_texture_bind_ops.texture_get_or_create != NULL) {
        texture = s_model_texture_bind_ops.texture_get_or_create(
            s_model_texture_bind_ops.context, (void*)path_prefix,
            resolved_name);
        if (texture != NULL) binding->texture_callback_owned = 1u;
        return texture;
    }
    texture = texture_get_or_load(NULL, resolved_name);
    if (texture != NULL) binding->texture_surface_owned = 1u;
    return texture;
}

// FUNCTION: LEGORR 0x00481820
void bind_model_textures_for_model(const char* path_prefix,
                                   const Model* model,
                                   ModelTextureBinding* bindings,
                                   int force_source_color)
{
    size_t index;
    if (model == NULL || model->n_surfaces > 0xffffffffu ||
        (model->n_surfaces != 0u && model->surfaces == NULL) ||
        (model->n_surfaces != 0u && bindings == NULL)) return;
    for (index = 0u; index < model->n_surfaces; ++index) {
        const ModelSurface* source = &model->surfaces[index];
        ModelTextureBinding* binding = &bindings[index];
        uint16_t material_flags = source->has_material_flags ?
            source->material_flags : 0u;
        uint16_t texture_flags = source->has_texture_flags ?
            source->texture_flags : 0u;
        memset(binding, 0, sizeof(*binding));
        if (source->has_tsiz != 0) {
            binding->texture_scale[0] = source->tsiz[0];
            binding->texture_scale[1] = source->tsiz[1];
            binding->texture_scale[2] = source->tsiz[2];
            binding->has_texture_scale = 1u;
        }
        if (source->has_tctr != 0) {
            binding->texture_center[0] = source->tctr[0];
            binding->texture_center[1] = source->tctr[1];
            binding->texture_center[2] = source->tctr[2];
        }
        if (source->timg != NULL && force_source_color == 0) {
            char lookup_name[260];
            if (!model_texture_name_build(path_prefix, source->timg,
                                          lookup_name)) {
                /* The retail path continues building later material slots
                 * when a path cannot be represented; only this lookup is
                 * skipped. */
                if ((texture_flags & 0x0080u) != 0u) {
                    binding->color[0] =
                        (float)source->colr_bytes[0] / 256.0f;
                    binding->color[1] =
                        (float)source->colr_bytes[1] / 256.0f;
                    binding->color[2] =
                        (float)source->colr_bytes[2] / 256.0f;
                } else {
                    binding->color[0] = 1.0f;
                    binding->color[1] = 1.0f;
                    binding->color[2] = 1.0f;
                }
                binding->surface_luminosity = source->luminosity;
                binding->surface_diffuse = source->diffuse;
                binding->surface_specular = source->specular;
                binding->surface_reflection = source->reflection;
                binding->surface_glossiness = source->glossiness;
                binding->surface_transparency = source->transparency;
                binding->surface_texture_flags = texture_flags;
                binding->surface_material_flags = material_flags;
                continue;
            }
            /* BindModelTextures tests the byte at SURF + 0x14 for the
             * numbered-image path. That is FLAG/material_flags; TFLG is at
             * +0x18 and controls the later render-state flags. */
            if ((material_flags & 0x0080u) != 0u) {
                uint32_t frame;
                binding->animation_frames = (void**)malloc(
                    100u * sizeof(void*));
                if (binding->animation_frames != NULL) {
                    binding->animation_array_owned = 1u;
                    for (frame = 0u; frame < 100u; ++frame) {
                        char frame_name[260];
                        void* texture;
                        if (!model_texture_frame_name(lookup_name, frame,
                                                      frame_name)) break;
                        texture = model_texture_lookup(path_prefix, frame_name,
                                                       binding);
                        if (texture == NULL) break;
                        binding->animation_frames[frame] = texture;
                        ++binding->animation_frame_count;
                    }
                    if (binding->animation_frame_count == 0u) {
                        free(binding->animation_frames);
                        binding->animation_frames = NULL;
                        binding->animation_array_owned = 0u;
                    }
                    else {
                        void** frames = (void**)realloc(
                            binding->animation_frames,
                            (size_t)binding->animation_frame_count *
                                sizeof(void*));
                        if (frames != NULL) binding->animation_frames = frames;
                    }
                }
            }
            else binding->texture = model_texture_lookup(path_prefix,
                                                         lookup_name, binding);
            if ((texture_flags & 0x0080u) != 0u) {
                binding->color[0] = (float)source->colr_bytes[0] / 256.0f;
                binding->color[1] = (float)source->colr_bytes[1] / 256.0f;
                binding->color[2] = (float)source->colr_bytes[2] / 256.0f;
            }
            else {
                binding->color[0] = 1.0f;
                binding->color[1] = 1.0f;
                binding->color[2] = 1.0f;
            }
        }
        else {
            /* Retail always converts source bytes with 1/256, including a
             * default-zero record whose COLR block was absent. */
            binding->color[0] = (float)source->colr_bytes[0] / 256.0f;
            binding->color[1] = (float)source->colr_bytes[1] / 256.0f;
            binding->color[2] = (float)source->colr_bytes[2] / 256.0f;
        }
        binding->surface_luminosity = source->luminosity;
        binding->surface_diffuse = source->diffuse;
        binding->surface_specular = source->specular;
        binding->surface_reflection = source->reflection;
        binding->surface_glossiness = source->glossiness;
        binding->surface_transparency = source->transparency;
        binding->surface_texture_flags = texture_flags;
        binding->surface_material_flags = material_flags;
    }
}

void model_texture_bindings_release(ModelTextureBinding* bindings,
                                    uint32_t count)
{
    uint32_t i;
    if (bindings == NULL) return;
    for (i = 0u; i < count; ++i) {
        uint32_t frame;
        if (bindings[i].texture_callback_owned != 0u &&
            bindings[i].texture != NULL &&
            s_model_texture_bind_ops.texture_release != NULL)
            s_model_texture_bind_ops.texture_release(
                s_model_texture_bind_ops.context, bindings[i].texture);
        if (bindings[i].texture_surface_owned != 0u &&
            bindings[i].texture != NULL)
            texture_release((TextureSurface*)bindings[i].texture);
        for (frame = 0u; bindings[i].texture_surface_owned != 0u &&
             frame < bindings[i].animation_frame_count; ++frame) {
            if (bindings[i].animation_frames != NULL &&
                bindings[i].animation_frames[frame] != NULL)
                texture_release((TextureSurface*)
                    bindings[i].animation_frames[frame]);
        }
        for (frame = 0u; bindings[i].texture_callback_owned != 0u &&
             frame < bindings[i].animation_frame_count; ++frame) {
            if (bindings[i].animation_frames != NULL &&
                bindings[i].animation_frames[frame] != NULL &&
                s_model_texture_bind_ops.texture_release != NULL)
                s_model_texture_bind_ops.texture_release(
                    s_model_texture_bind_ops.context,
                    bindings[i].animation_frames[frame]);
        }
        if (bindings[i].animation_array_owned != 0u)
            free(bindings[i].animation_frames);
        memset(&bindings[i], 0, sizeof(bindings[i]));
    }
}

void material_pass_ops_set(const MaterialPassOps* ops)
{
    if (ops != NULL) s_material_pass_ops = *ops;
    else memset(&s_material_pass_ops, 0, sizeof(s_material_pass_ops));
}

static int material_pass_size_multiply(size_t a, size_t b, size_t* result)
{
    if (a != 0u && b > (size_t)-1 / a) return 0;
    *result = a * b;
    return 1;
}

static void material_pass_uv_project(const ModelSurface* material,
                                     const float* position, float* uv)
{
    /* MaterialPass passes the source SURF word at +0x14 to
     * MeshVertexCoordinateRemap; +0x18 (TFLG) drives render state instead. */
    uint16_t flags = material->has_material_flags ? material->material_flags : 0u;
    const float* scale = material->tsiz;
    const float* center = material->tctr;
    uv[0] = (flags & 1u) == 0u ?
        (position[0] - center[0]) / scale[0] + 0.5f :
        (position[2] - center[2]) / scale[2] + 0.5f;
    uv[1] = (flags & 2u) == 0u ?
        (position[1] - center[1]) / scale[1] + 0.5f :
        (position[2] - center[2]) / scale[2] + 0.5f;
}

static uint32_t material_pass_render_flags(const ModelSurface* material)
{
    uint16_t texture_flags = material->has_texture_flags ?
        material->texture_flags : 0u;
    uint16_t material_flags = material->has_material_flags ?
        material->material_flags : 0u;
    uint32_t flags = (texture_flags & 0x0200u) != 0u ? 0x2800u : 0x3000u;
    if ((texture_flags & 0x0100u) != 0u) flags |= 0x04000000u;
    if ((material_flags & 0x0020u) == 0u) flags |= 0x00800000u;
    return flags;
}

static size_t material_pass_poly_corner_order(const ModelPoly* poly,
                                              uint16_t order[6])
{
    if (poly->count == 3u) {
        order[0] = 0u; order[1] = 1u; order[2] = 2u;
        if (poly->flipped != 0) {
            order[1] = 2u;
            order[2] = 1u;
        }
        return 3u;
    }
    order[0] = 0u; order[1] = 1u; order[2] = 2u;
    order[3] = 0u; order[4] = 2u; order[5] = 3u;
    if (poly->flipped != 0) {
        order[1] = 3u; order[2] = 2u;
        order[4] = 2u; order[5] = 1u;
    }
    return 6u;
}

typedef struct MaterialPassMeshData {
    size_t* polygon_offsets;
    uint16_t* corner_vertices;
    uint16_t* vertex_sources;
    float* vertex_uv;
    uint8_t* vertex_uv_assigned;
    float* vertex_normals;
    size_t vertex_count;
} MaterialPassMeshData;

static void material_pass_mesh_data_release(MaterialPassMeshData* data)
{
    if (data == NULL) return;
    free(data->polygon_offsets);
    free(data->corner_vertices);
    free(data->vertex_sources);
    free(data->vertex_uv);
    free(data->vertex_uv_assigned);
    free(data->vertex_normals);
    memset(data, 0, sizeof(*data));
}

/* ParseUV duplicates a point when a later polygon corner gives the same
 * source point a different UV pair. Build that remapped point table here so
 * the geometry packet preserves both the seam and its separate normals. */
static int material_pass_mesh_data_build(const Model* model,
                                         MaterialPassMeshData* data)
{
    size_t offset_bytes;
    size_t corner_bytes;
    size_t source_bytes;
    size_t uv_bytes;
    size_t normal_count;
    size_t normal_bytes;
    size_t total_corners = 0u;
    size_t capacity;
    size_t poly_index;
    size_t vertex_index;
    int has_uv;
    if (model == NULL || data == NULL) return 0;
    memset(data, 0, sizeof(*data));
    has_uv = model->uv_applied != 0;
    if (model->n_polys > (size_t)-1 - 1u ||
        !material_pass_size_multiply(model->n_polys + 1u,
                                     sizeof(size_t), &offset_bytes))
        return 0;
    data->polygon_offsets = (size_t*)malloc(offset_bytes);
    if (data->polygon_offsets == NULL) goto fail;
    data->polygon_offsets[0] = 0u;
    for (poly_index = 0u; poly_index < model->n_polys; ++poly_index) {
        if (model->polys == NULL ||
            model->polys[poly_index].count > (size_t)-1 - total_corners)
            goto fail;
        total_corners += model->polys[poly_index].count;
        data->polygon_offsets[poly_index + 1u] = total_corners;
    }
    if (!material_pass_size_multiply(total_corners, sizeof(uint16_t),
                                     &corner_bytes)) goto fail;
    if (corner_bytes != 0u) {
        data->corner_vertices = (uint16_t*)malloc(corner_bytes);
        if (data->corner_vertices == NULL) goto fail;
    }
    if (model->n_verts > (size_t)-1 - total_corners) goto fail;
    capacity = model->n_verts;
    if (has_uv) {
        capacity += total_corners;
        if (capacity > 65536u) capacity = 65536u;
    }
    if (capacity < model->n_verts) goto fail;
    if (!material_pass_size_multiply(capacity, sizeof(uint16_t),
                                     &source_bytes) ||
        !material_pass_size_multiply(capacity, 2u * sizeof(float),
                                     &uv_bytes) ||
        !material_pass_size_multiply(capacity, 3u, &normal_count) ||
        !material_pass_size_multiply(normal_count, sizeof(float),
                                     &normal_bytes)) goto fail;
    if (source_bytes != 0u) {
        data->vertex_sources = (uint16_t*)malloc(source_bytes);
        if (data->vertex_sources == NULL) goto fail;
    }
    if (capacity != 0u && has_uv) {
        data->vertex_uv = (float*)calloc(1u, uv_bytes);
        data->vertex_uv_assigned = (uint8_t*)calloc(capacity, sizeof(uint8_t));
        if (data->vertex_uv == NULL ||
            data->vertex_uv_assigned == NULL) goto fail;
    }
    data->vertex_count = model->n_verts;
    for (vertex_index = 0u; vertex_index < model->n_verts; ++vertex_index)
        data->vertex_sources[vertex_index] = (uint16_t)vertex_index;

    for (poly_index = 0u; poly_index < model->n_polys; ++poly_index) {
        const ModelPoly* poly = &model->polys[poly_index];
        size_t corner;
        if (poly->count != 0u && poly->ring == NULL) goto fail;
        for (corner = 0u; corner < poly->count; ++corner) {
            uint16_t source = poly->ring[corner];
            uint16_t mapped = source;
            if ((size_t)source >= model->n_verts) goto fail;
            if (has_uv && poly->uv != NULL && corner < poly->uv_count) {
                float u = poly->uv[corner * 2u];
                float v = poly->uv[corner * 2u + 1u];
                if (data->vertex_uv_assigned[source] == 0u) {
                    data->vertex_uv[(size_t)source * 2u] = u;
                    data->vertex_uv[(size_t)source * 2u + 1u] = v;
                    data->vertex_uv_assigned[source] = 1u;
                }
                else if (data->vertex_uv[(size_t)source * 2u] != u ||
                         data->vertex_uv[(size_t)source * 2u + 1u] != v) {
                    if (data->vertex_count >= capacity ||
                        data->vertex_count >= 65536u) goto fail;
                    mapped = (uint16_t)data->vertex_count++;
                    data->vertex_sources[mapped] = source;
                    data->vertex_uv[(size_t)mapped * 2u] = u;
                    data->vertex_uv[(size_t)mapped * 2u + 1u] = v;
                    data->vertex_uv_assigned[mapped] = 1u;
                }
            }
            data->corner_vertices[data->polygon_offsets[poly_index] +
                                  corner] = mapped;
        }
    }

    if (!material_pass_size_multiply(data->vertex_count, 3u,
                                     &normal_count) ||
        !material_pass_size_multiply(normal_count, sizeof(float),
                                     &normal_bytes)) goto fail;
    if (normal_bytes != 0u) {
        data->vertex_normals = (float*)calloc(1u, normal_bytes);
        if (data->vertex_normals == NULL) goto fail;
    }
    for (poly_index = 0u; poly_index < model->n_polys; ++poly_index) {
        const ModelPoly* poly = &model->polys[poly_index];
        uint16_t order[6];
        size_t emitted;
        size_t corner;
        float face_normal[3];
        const float* first;
        const float* second;
        const float* third;
        if (poly->count < 3u) continue;
        first = &model->verts[(size_t)poly->ring[0] * 3u];
        second = &model->verts[(size_t)poly->ring[1] * 3u];
        third = &model->verts[(size_t)poly->ring[2] * 3u];
        surface_triangle_normal_calculate(face_normal, first, second, third);
        if (poly->flipped != 0) {
            face_normal[0] = -face_normal[0];
            face_normal[1] = -face_normal[1];
            face_normal[2] = -face_normal[2];
        }
        emitted = material_pass_poly_corner_order(poly, order);
        for (corner = 0u; corner < emitted; ++corner) {
            size_t mapped_index = data->corner_vertices[
                data->polygon_offsets[poly_index] + order[corner]];
            float* normal;
            float length;
            if (mapped_index >= data->vertex_count) goto fail;
            normal = &data->vertex_normals[mapped_index * 3u];
            normal[0] += face_normal[0];
            normal[1] += face_normal[1];
            normal[2] += face_normal[2];
            length = (float)sqrt((double)(normal[0] * normal[0] +
                normal[1] * normal[1] + normal[2] * normal[2]));
            normal[0] /= length;
            normal[1] /= length;
            normal[2] /= length;
        }
    }
    return 1;
fail:
    material_pass_mesh_data_release(data);
    return 0;
}

void material_pass_prepared_release(MaterialPassPreparedModel* prepared)
{
    size_t surface_index;
    if (prepared == NULL) return;
    if (prepared->surfaces != NULL) {
        for (surface_index = 0u; surface_index < prepared->surface_count;
             ++surface_index) {
            free((void*)prepared->surfaces[surface_index].vertices);
            free((void*)prepared->surfaces[surface_index].indices);
        }
    }
    free(prepared->surfaces);
    if (prepared->owned_bindings != NULL) {
        model_texture_bindings_release(prepared->owned_bindings,
                                       (uint32_t)prepared->surface_count);
        free(prepared->owned_bindings);
    }
    memset(prepared, 0, sizeof(*prepared));
}

int material_pass_prepare(const char* path_prefix, const Model* model,
                          uint32_t flags,
                          MaterialPassPreparedModel* prepared)
{
    MaterialPassPreparedModel result;
    MaterialPassMeshData mesh_data;
    size_t binding_bytes;
    size_t remap_bytes;
    size_t vertex_bytes;
    size_t index_capacity;
    size_t index_bytes;
    size_t surface_index;
    memset(&result, 0, sizeof(result));
    memset(&mesh_data, 0, sizeof(mesh_data));
    if (prepared == NULL) return 0;
    memset(prepared, 0, sizeof(*prepared));
    if (model == NULL ||
        (model->n_verts != 0u && model->verts == NULL) ||
        (model->n_polys != 0u && model->polys == NULL) ||
        (model->n_surfaces != 0u && model->surfaces == NULL) ||
        model->n_verts > 65536u || model->n_surfaces > 0xffffffffu ||
        !material_pass_size_multiply(model->n_surfaces,
                                     sizeof(ModelTextureBinding),
                                     &binding_bytes)) return 0;
    result.surface_count = model->n_surfaces;
    if (result.surface_count != 0u) {
        if (flags == 0u && model->material_bindings != NULL &&
            model->material_binding_count == result.surface_count) {
            result.bindings = model->material_bindings;
        }
        else {
            result.owned_bindings = (ModelTextureBinding*)calloc(
                1u, binding_bytes);
            if (result.owned_bindings == NULL) goto fail;
            bind_model_textures_for_model(path_prefix, model,
                                          result.owned_bindings,
                                          flags != 0u);
            result.bindings = result.owned_bindings;
        }
        result.surfaces = (MaterialPassSurface*)calloc(
            result.surface_count, sizeof(MaterialPassSurface));
        if (result.surfaces == NULL) goto fail;
    }
    if (!material_pass_mesh_data_build(model, &mesh_data) ||
        !material_pass_size_multiply(mesh_data.vertex_count, sizeof(int32_t),
                                     &remap_bytes) ||
        !material_pass_size_multiply(model->n_polys, 6u,
                                     &index_capacity) ||
        !material_pass_size_multiply(index_capacity, sizeof(uint16_t),
                                     &index_bytes) ||
        !material_pass_size_multiply(mesh_data.vertex_count,
                                     sizeof(MaterialPassVertex),
                                     &vertex_bytes)) goto fail;

    for (surface_index = 0u; surface_index < result.surface_count;
         ++surface_index) {
        MaterialPassVertex* vertices;
        uint16_t* indices;
        int32_t* remap;
        size_t vertex_count = 0u;
        size_t index_count = 0u;
        size_t poly_index;
        MaterialPassSurface* packet = &result.surfaces[surface_index];
        const ModelSurface* material = &model->surfaces[surface_index];
        if (vertex_bytes != 0u) {
            vertices = (MaterialPassVertex*)calloc(1u, vertex_bytes);
            if (vertices == NULL) goto fail;
        }
        else vertices = NULL;
        if (index_bytes != 0u) {
            indices = (uint16_t*)malloc(index_bytes);
            if (indices == NULL) { free(vertices); goto fail; }
        }
        else indices = NULL;
        if (remap_bytes != 0u) {
            remap = (int32_t*)malloc(remap_bytes);
            if (remap == NULL) {
                free(indices);
                free(vertices);
                goto fail;
            }
            for (poly_index = 0u; poly_index < mesh_data.vertex_count;
                 ++poly_index)
                remap[poly_index] = -1;
        }
        else remap = NULL;

        for (poly_index = 0u; poly_index < model->n_polys; ++poly_index) {
            const ModelPoly* poly = &model->polys[poly_index];
            uint16_t corner_order[6];
            size_t emitted;
            size_t corner;
            if (poly->surface != surface_index || poly->count < 3u ||
                poly->ring == NULL) continue;
            emitted = material_pass_poly_corner_order(poly, corner_order);
            for (corner = 0u; corner < emitted; ++corner) {
                size_t source_corner = corner_order[corner];
                uint16_t source_index;
                int32_t destination;
                size_t component;
                if (source_corner >= poly->count) {
                    free(remap); free(indices); free(vertices); goto fail;
                }
                source_index = poly->ring[source_corner];
                if ((size_t)source_index >= model->n_verts) {
                    free(remap); free(indices); free(vertices); goto fail;
                }
                {
                    size_t corner_offset =
                        mesh_data.polygon_offsets[poly_index] + source_corner;
                    uint16_t mapped = mesh_data.corner_vertices[corner_offset];
                    if ((size_t)mapped >= mesh_data.vertex_count) {
                        free(remap); free(indices); free(vertices); goto fail;
                    }
                    destination = remap[mapped];
                }
                if (destination < 0) {
                    uint16_t mapped = mesh_data.corner_vertices[
                        mesh_data.polygon_offsets[poly_index] + source_corner];
                    uint16_t position_source = mesh_data.vertex_sources[mapped];
                    if (vertex_count >= mesh_data.vertex_count ||
                        vertex_count > 65535u ||
                        index_count >= index_capacity) {
                        free(remap); free(indices); free(vertices); goto fail;
                    }
                    destination = (int32_t)vertex_count++;
                    remap[mapped] = destination;
                    for (component = 0u; component < 3u; ++component)
                        vertices[destination].position[component] =
                            model->verts[(size_t)position_source * 3u + component];
                    for (component = 0u; component < 3u; ++component)
                        vertices[destination].normal[component] =
                            mesh_data.vertex_normals[(size_t)mapped * 3u +
                                                     component];
                    if (model->uv_applied != 0 &&
                        mesh_data.vertex_uv_assigned[mapped] != 0u) {
                        vertices[destination].uv[0] =
                            mesh_data.vertex_uv[(size_t)mapped * 2u];
                        vertices[destination].uv[1] =
                            mesh_data.vertex_uv[(size_t)mapped * 2u + 1u];
                    }
                    else {
                        material_pass_uv_project(material,
                            vertices[destination].position,
                            vertices[destination].uv);
                    }
                }
                if (index_count >= index_capacity) {
                    free(remap); free(indices); free(vertices); goto fail;
                }
                indices[index_count++] = (uint16_t)destination;
            }
        }
        free(remap);
        packet->surface_index = (uint32_t)surface_index;
        packet->vertex_count = (uint32_t)vertex_count;
        packet->index_count = (uint32_t)index_count;
        packet->render_flags = material_pass_render_flags(material);
        packet->diffuse_color[0] = result.bindings[surface_index].color[0] *
            result.bindings[surface_index].surface_diffuse;
        packet->diffuse_color[1] = result.bindings[surface_index].color[1] *
            result.bindings[surface_index].surface_diffuse;
        packet->diffuse_color[2] = result.bindings[surface_index].color[2] *
            result.bindings[surface_index].surface_diffuse;
        packet->emissive_color[0] = result.bindings[surface_index].color[0] *
            result.bindings[surface_index].surface_luminosity;
        packet->emissive_color[1] = result.bindings[surface_index].color[1] *
            result.bindings[surface_index].surface_luminosity;
        packet->emissive_color[2] = result.bindings[surface_index].color[2] *
            result.bindings[surface_index].surface_luminosity;
        if ((result.bindings[surface_index].surface_texture_flags & 8u) != 0u) {
            packet->specular_color[0] =
                result.bindings[surface_index].color[0] *
                result.bindings[surface_index].surface_specular;
            packet->specular_color[1] =
                result.bindings[surface_index].color[1] *
                result.bindings[surface_index].surface_specular;
            packet->specular_color[2] =
                result.bindings[surface_index].color[2] *
                result.bindings[surface_index].surface_specular;
        }
        else {
            packet->specular_color[0] =
                result.bindings[surface_index].surface_specular;
            packet->specular_color[1] =
                result.bindings[surface_index].surface_specular;
            packet->specular_color[2] =
                result.bindings[surface_index].surface_specular;
        }
        packet->gloss_power =
            result.bindings[surface_index].surface_glossiness * 10.0f;
        packet->opacity = 1.0f -
            result.bindings[surface_index].surface_transparency;
        packet->vertices = vertices;
        packet->indices = indices;
        packet->material = material;
        packet->texture = &result.bindings[surface_index];
    }
    material_pass_mesh_data_release(&mesh_data);
    *prepared = result;
    return 1;

fail:
    material_pass_mesh_data_release(&mesh_data);
    material_pass_prepared_release(&result);
    return 0;
}

int material_pass_submit(const char* path_prefix, void* render_object,
                         const MaterialPassPreparedModel* prepared)
{
    size_t surface_index;
    if (render_object == NULL || prepared == NULL ||
        (prepared->surface_count != 0u && prepared->surfaces == NULL)) return 0;
    if (prepared->surface_count == 0u) return 1;
    if (s_material_pass_ops.surface_submit == NULL) return 0;
    for (surface_index = 0u; surface_index < prepared->surface_count;
         ++surface_index) {
        if (!s_material_pass_ops.surface_submit(
                s_material_pass_ops.context, path_prefix, render_object,
                &prepared->surfaces[surface_index])) return 0;
    }
    return 1;
}

// FUNCTION: LEGORR 0x00480ac0
int material_pass_execute(const char* path_prefix, void* render_object,
                          const Model* model, uint32_t flags)
{
    MaterialPassPreparedModel prepared;
    int result;
    if (render_object == NULL ||
        !material_pass_prepare(path_prefix, model, flags, &prepared)) return 0;
    result = material_pass_submit(path_prefix, render_object, &prepared);
    material_pass_prepared_release(&prepared);
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
typedef struct FontAtlasGlyphRecord {
    float x;
    float y;
    float width;
    float height;
} FontAtlasGlyphRecord;
typedef struct FontAtlasRecord {
    TextureSurface* surface;
    struct FontAtlasRecord* free_next;
    FontAtlasGlyphRecord glyphs[10 * 19];
    uint32_t line_height;
    uint32_t tab_width;
} FontAtlasRecord;
static FontAtlasRecord* s_font_atlas_record_free_list;
static SurfacePixelBitsQueryFn s_surface_pixel_bits_query;
// GLOBAL: LEGORR 0x005407e8
static SurfacePixelWriteFn s_surface_pixel_writer;
static SurfaceWriterUnlockHook s_surface_writer_unlock_hook;
static RendererBeginFrameHook s_renderer_begin_frame_hook;
static void* s_renderer_begin_frame_device;
static RenderStateApplyOps s_render_state_apply_ops;
static uint32_t s_d3d_material_state_saved[200];
static uint8_t s_d3d_material_state_dirty[200];
static RendererWindowModeOps s_renderer_window_mode_ops;
static int s_renderer_window_width;
static int s_renderer_window_height;
static uint32_t s_renderer_window_mode_flags;
// GLOBAL: LEGORR 0x0076ac80
void* s_renderer_window_handle;
static DirectDrawEnumerationHook s_directdraw_enumeration_hook;
static void* s_directdraw_enumeration_context;
static Direct3DRenderPreferenceApplyHook s_render_preference_apply_hook;
static void* s_render_preference_apply_context;
static Direct3DMatrixOps s_direct3d_matrix_ops;
static ModelRenderRecordStateOps s_model_render_record_state_ops;
static ModelRenderChildPassOps s_model_render_child_pass_ops;
static ModelRenderPassDispatchOps s_model_render_pass_dispatch_ops;
typedef struct PortableModelRenderEntry {
    struct PortableModelRenderEntry* next;
    Model* model;
    uint32_t entry_flags;
    float parameters[16];
    char path_prefix[260];
} PortableModelRenderEntry;
static PortableModelRenderEntry* s_portable_model_render_head;
static PortableModelRenderEntry* s_portable_model_render_tail;
static PortableModelRenderEntry* s_portable_model_render_active;
static ProjectionOverlayBlitHook s_projection_overlay_blit_hook;
static void* s_projection_overlay_blit_context;
static DirectDrawClipperRegionOps s_directdraw_clipper_region_ops;
static Direct3DRenderStateSnapshotOps s_d3d_render_state_snapshot_ops;
static uint32_t s_d3d_snapshot_light;
static uint32_t s_d3d_snapshot_render;
static uint32_t s_d3d_snapshot_texture_stage;
static Direct3DDeviceStateOps s_direct3d_device_state_ops;
static void* s_direct3d_cached_texture;
static uint32_t s_direct3d_cached_material_handle;
// GLOBAL: LEGORR 0x0054057c
static uint32_t s_model_material_handle;
static int s_direct3d_device_texture_dirty;
static ModelMaterialUpdateHook s_model_material_update_hook;
static void* s_model_material_update_context;
static ModelChildMaterialOps s_model_child_material_ops;
static Direct3DInterfaceOps s_direct3d_interface_ops;
static DirectDrawDeviceCreateOps s_directdraw_device_create_ops;
static DirectDrawRendererInitOps s_directdraw_renderer_init_ops;
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
static int directdraw_renderer_init_winapi(
    const DirectDrawRendererInitParams* params, const void* guid,
    int width, int height, int bits_per_pixel);
static int winapi_renderer_active(void);
#endif
#if !defined(_WIN32)
static void* s_surface_description_interface;
#endif
static Direct3DTextureMemoryCapsQueryFn s_texture_memory_caps_query;
static void* s_texture_memory_caps_context;
static uint32_t s_texture_memory_last_value;
static Direct3DRMDeviceInitOps s_direct3drm_device_init_ops;
static int s_direct3drm_device_mode = 1;
static DirectDrawDeviceInitializeOps s_directdraw_device_initialize_ops;
static int s_directdraw_device_selection_cancelled;
static DirectDrawDeviceSelection s_directdraw_device_selection = {
    NULL, NULL, 1
};

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
void model_render_record_state_setup(void* record, void* parameter,
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
int model_render_child_pass_dispatch(void* model, const void* parameter,
                                     int material_pass)
{
    uint32_t index;
    int pass;
    int all_succeeded = 1;
    if (model == NULL || s_model_render_child_pass_ops.child_count == NULL ||
        s_model_render_child_pass_ops.child_get == NULL) return all_succeeded;
    for (pass = 0; pass < 2; ++pass) {
        for (index = 0u; ; ++index) {
            void* child;
            uint32_t flags;
            int has_material;
            if (index >= s_model_render_child_pass_ops.child_count(
                    s_model_render_child_pass_ops.context, model)) break;
            child = s_model_render_child_pass_ops.child_get(
                s_model_render_child_pass_ops.context, model, index);
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
 * outside the alternate renderer mode, texture-stage state 0. Seed the draw
 * caches from these live values before deciding which setters may be skipped.
 * Temporarily clear render state 1 when it was set. */
// FUNCTION: LEGORR 0x004838b0
void d3d_render_state_sync(void)
{
    uint32_t value = 0u;
    if (s_d3d_render_state_snapshot_ops.light_get != NULL &&
        s_d3d_render_state_snapshot_ops.light_get(
            s_d3d_render_state_snapshot_ops.context, 1u, &value) == 0) {
        s_d3d_snapshot_light = value;
        s_direct3d_cached_material_handle = value;
    }
    if ((s_renderer_window_mode_flags & 0x20u) == 0u) {
        value = 0u;
        if (s_d3d_render_state_snapshot_ops.texture_stage_get != NULL &&
            s_d3d_render_state_snapshot_ops.texture_stage_get(
                s_d3d_render_state_snapshot_ops.context, 0u, &value) == 0) {
            s_d3d_snapshot_texture_stage = value;
            s_direct3d_cached_texture = (void*)(uintptr_t)value;
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
 * update D3DLIGHTSTATE_MATERIAL(1) when its handle changes, then submit. */
// FUNCTION: LEGORR 0x00483b70
int direct3d_device_state_apply(uint32_t material_handle, void* texture,
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
    if (material_handle != s_direct3d_cached_material_handle) {
        if (s_direct3d_device_state_ops.light_state_set == NULL ||
            s_direct3d_device_state_ops.light_state_set(
                s_direct3d_device_state_ops.context, 1u, material_handle) != 0) {
            result = 0;
        }
        else {
            s_direct3d_cached_material_handle = material_handle;
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

void model_material_handle_set(uint32_t handle)
{
    s_model_material_handle = handle;
}

uint32_t model_material_handle_peek(void)
{
    return s_model_material_handle;
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
int model_render_child_material_pass(void* model, void* child,
                                     const float* transform,
                                     int alternate_mode)
{
    uint32_t arguments[4] = { 0u, 0u, 0u, 0u };
    void* texture = NULL;
    const void* material = NULL;
    int material_ok = 0;
    int render_ok;
    (void)model;
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
        s_model_material_handle, texture, 0x112u, arguments[3],
        arguments[1], arguments[2], arguments[0]);
    return (render_ok != 0 && material_ok != 0) ? 1 : 0;
}

void model_render_pass_dispatch_ops_set(
    const ModelRenderPassDispatchOps* ops)
{
    if (ops != NULL) s_model_render_pass_dispatch_ops = *ops;
    else memset(&s_model_render_pass_dispatch_ops, 0,
                sizeof(s_model_render_pass_dispatch_ops));
}

int model_render_dispatch_enqueue(const char* path_prefix, Model* model,
                                  const float parameters[16],
                                  uint32_t entry_flags)
{
    PortableModelRenderEntry* entry;
    const char* resolved_prefix = path_prefix;
    size_t prefix_length = 0u;
    if (model == NULL) return 0;
    if (resolved_prefix == NULL && model->render_source != NULL &&
        model->render_source->model == model)
        resolved_prefix = model->render_source->path_prefix;
    if (resolved_prefix != NULL) {
        prefix_length = strlen(resolved_prefix);
        if (prefix_length >= sizeof(entry->path_prefix)) return 0;
    }
    entry = (PortableModelRenderEntry*)calloc(1u, sizeof(*entry));
    if (entry == NULL) return 0;
    entry->model = model;
    entry->entry_flags = entry_flags;
    if (parameters != NULL)
        memcpy(entry->parameters, parameters, sizeof(entry->parameters));
    if (prefix_length != 0u)
        memcpy(entry->path_prefix, resolved_prefix, prefix_length);
    if (s_portable_model_render_tail != NULL)
        s_portable_model_render_tail->next = entry;
    else
        s_portable_model_render_head = entry;
    s_portable_model_render_tail = entry;
    return 1;
}

void model_render_dispatch_model_remove(Model* model)
{
    PortableModelRenderEntry** link = &s_portable_model_render_head;
    PortableModelRenderEntry* tail = NULL;
    while (*link != NULL) {
        PortableModelRenderEntry* entry = *link;
        if (entry->model == model) {
            *link = entry->next;
            if (s_portable_model_render_active == entry)
                s_portable_model_render_active = NULL;
            free(entry);
        }
        else {
            tail = entry;
            link = &entry->next;
        }
    }
    s_portable_model_render_tail = tail;
}

void model_render_dispatch_queue_clear(void)
{
    PortableModelRenderEntry* entry = s_portable_model_render_head;
    while (entry != NULL) {
        PortableModelRenderEntry* next = entry->next;
        free(entry);
        entry = next;
    }
    s_portable_model_render_head = NULL;
    s_portable_model_render_tail = NULL;
    s_portable_model_render_active = NULL;
}

static void* portable_model_render_first(void*)
{
    return s_portable_model_render_head;
}

static void* portable_model_render_next(void*, void* opaque_entry)
{
    PortableModelRenderEntry* entry =
        (PortableModelRenderEntry*)opaque_entry;
    return entry != NULL ? entry->next : NULL;
}

static void* portable_model_render_model(void*, void* opaque_entry)
{
    s_portable_model_render_active =
        (PortableModelRenderEntry*)opaque_entry;
    return s_portable_model_render_active != NULL ?
        s_portable_model_render_active->model : NULL;
}

static const void* portable_model_render_parameters(void*, void* opaque_entry)
{
    PortableModelRenderEntry* entry =
        (PortableModelRenderEntry*)opaque_entry;
    return entry != NULL ? entry->parameters : NULL;
}

static uint32_t portable_model_render_entry_flags(void*, void* opaque_entry)
{
    PortableModelRenderEntry* entry =
        (PortableModelRenderEntry*)opaque_entry;
    return entry != NULL ? entry->entry_flags : 0u;
}

static uint32_t portable_model_render_model_flags(void*, void* opaque_model)
{
    (void)opaque_model;
    /* The retail dispatch reads both tests from model + 0x2c.  The portable
     * queue keeps that modeled word beside its borrowed Model pointer. */
    return s_portable_model_render_active != NULL ?
        s_portable_model_render_active->entry_flags : 0u;
}

static void portable_model_render_base_pass(void*, void* model, void* device,
                                            const void*)
{
    if (s_portable_model_render_active == NULL || model == NULL ||
        s_material_pass_ops.surface_submit == NULL) return;
    (void)material_pass_execute(
        s_portable_model_render_active->path_prefix, device,
        (const Model*)model, 0u);
}

static void portable_model_render_queue_flush(void*)
{
    model_render_dispatch_queue_clear();
}

static ModelRenderPassDispatchOps s_portable_model_render_dispatch_ops = {
    NULL,
    portable_model_render_first,
    portable_model_render_next,
    portable_model_render_model,
    portable_model_render_parameters,
    portable_model_render_entry_flags,
    portable_model_render_model_flags,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    portable_model_render_base_pass,
    NULL,
    NULL,
    portable_model_render_queue_flush,
    NULL
};

/* ModelRenderPassDispatch (FUN_00482d60): preserve the retail list order,
 * two child-material passes, alternate-model branch, and renderer cleanup.
 * The record access and draw calls remain supplied by the renderer owner. */
// FUNCTION: LEGORR 0x00482d60
void model_render_pass_dispatch(void* device)
{
    ModelRenderPassDispatchOps* ops = &s_model_render_pass_dispatch_ops;
    void* entry;
    if (ops->first == NULL)
        ops = &s_portable_model_render_dispatch_ops;
    entry = ops->first(ops->context);
    if (entry == NULL) return;
    if (ops->frame_setup != NULL)
        ops->frame_setup(ops->context, device);
    else
        direct3d_viewport_setup(device);
    if (ops->render_mode_begin != NULL) {
        ops->render_mode_begin(ops->context);
    }
    else {
        d3d_render_state_sync();
    }
    while (entry != NULL) {
        void* model = ops->model != NULL ?
            ops->model(ops->context, entry) : NULL;
        const void* parameters = ops->parameters != NULL ?
            ops->parameters(ops->context, entry) : NULL;
        uint32_t model_flags = model != NULL && ops->model_flags != NULL ?
            ops->model_flags(ops->context, model) :
            (model != NULL && ops->entry_flags != NULL ?
             ops->entry_flags(ops->context, entry) : 0u);
        if (model != NULL && (model_flags & 1u) == 0u) {
            if (ops->model_base_pass != NULL)
                ops->model_base_pass(ops->context, model, device, parameters);
            else if (parameters != NULL)
                model_render_record_state_setup(
                    model, device, (const float*)parameters, 1);

            /* FUN_00482ac0 may update the record; the reference reloads its
             * model pointer and flags after that operation. */
            model = ops->model != NULL ?
                ops->model(ops->context, entry) : model;
            if (model != NULL &&
                (((ops->model_flags != NULL ?
                   ops->model_flags(ops->context, model) : 0u) & 2u) != 0u)) {
                if (ops->model_alternate_pass != NULL)
                    ops->model_alternate_pass(ops->context, model,
                                              parameters, 1);
                else if (parameters != NULL) {
                    (void)model_render_child_pass_dispatch(
                        model, parameters, 1);
                }
            }
            else if (model != NULL && ops->record_count != NULL &&
                     ops->record != NULL) {
                int pass;
                for (pass = 0; pass < 2; ++pass) {
                    uint32_t index = 0u;
                    for (;;) {
                        uint32_t count;
                        void* current_model = ops->model != NULL ?
                            ops->model(ops->context, entry) : model;
                        void* record;
                        uint32_t record_flags;
                        int has_secondary;
                        if (current_model == NULL) break;
                        count = ops->record_count(ops->context,
                                                  current_model);
                        if (index >= count) break;
                        record = ops->record(ops->context, current_model,
                                             index);
                        if (record != NULL &&
                            ops->record_ready != NULL &&
                            ops->record_ready(ops->context, record) != 0 &&
                            ops->record_flags != NULL &&
                            ops->record_has_secondary_material != NULL) {
                            record_flags = ops->record_flags(
                                ops->context, record);
                            has_secondary =
                                ops->record_has_secondary_material(
                                    ops->context, record);
                            if ((record_flags & 0x80000u) != 0u &&
                                ((pass == 0 && has_secondary == 0) ||
                                 (pass != 0 && has_secondary != 0))) {
                                if (ops->model_child_pass != NULL)
                                    ops->model_child_pass(
                                        ops->context, current_model, record,
                                        parameters, 1);
                                else if (parameters != NULL) {
                                    (void)model_render_child_material_pass(
                                        current_model, record,
                                        (const float*)parameters, 1);
                                }
                            }
                        }
                        ++index;
                    }
                }
            }
        }
        if (ops->next == NULL) break;
        entry = ops->next(ops->context, entry);
    }
    if (ops->render_queue_flush != NULL)
        ops->render_queue_flush(ops->context);
    else
        model_render_list_clear();
    if (ops->render_state_flush != NULL)
        ops->render_state_flush(ops->context);
    else
        d3d_material_state_flush();
    direct3d_render_state_restore();
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
    if ((params->adapter_flags & 1u) != 0u) {
        guid = params->adapter_guid;
    }
    if (params->mode_width > 0) width = params->mode_width;
    if (params->mode_height > 0) height = params->mode_height;
    if (width <= 0 || height <= 0) return 0;

    if (s_directdraw_renderer_init_ops.device_create == NULL ||
        s_directdraw_renderer_init_ops.cooperative_level_set == NULL ||
        s_directdraw_renderer_init_ops.surfaces_create == NULL ||
        s_directdraw_renderer_init_ops.clipper_setup == NULL ||
        s_directdraw_renderer_init_ops.direct3d_initialize == NULL) {
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
        /* Do not disturb an already committed display while attempting a
         * replacement: DirectDraw changes the display mode process-wide. */
        if (winapi_renderer_active()) return 0;
        if (!directdraw_renderer_init_winapi(params, guid, width, height,
                                            bits_per_pixel)) return 0;
        renderer_window_mode_apply(params->fullscreen, params->window_x,
                                   params->window_y, width, height);
        return 1;
#else
        return 0;
#endif
    }
    s_renderer_window_handle = params->window;
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
    screen_bounded_surface_device_set(renderer, device);
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

void directdraw_device_selection_get(DirectDrawDeviceSelection* out)
{
    if (out != NULL) *out = s_directdraw_device_selection;
}

/* DirectDrawDeviceInitialize (FUN_0049d030): register standard modes,
 * enumerate adapters, optionally auto-select a supported display mode, then
 * enter the renderer initializer with the selected setup. */
// FUNCTION: LEGORR 0x0049d030
int directdraw_device_selection_cancelled(void)
{
    return s_directdraw_device_selection_cancelled;
}

int directdraw_device_initialize(int show_dialog, int register_extra_modes,
                                 int auto_select, int windowed,
                                 const char* error_message)
{
    int adapter_count;
    int fullscreen = 1;
    int selected = 0;
    int adapter_index;
    DirectDrawDeviceSelection selection;
    DirectDrawAdapterCandidate adapter;
    DirectDrawDisplayModeCandidate mode;
    int result;
    s_directdraw_device_selection_cancelled = 0;
    /* FUN_0049d030 resets DAT_005754e0 and seeds the canonical resolution
     * filters before opening its dialog or trying automatic selection. */
    display_resolution_filter_tuples_reset();
    memset(&selection, 0,
           sizeof(selection));
    selection.fullscreen = 1;
    display_resolution_filter_tuple_append(640, 480, 16);
    if (register_extra_modes != 0) {
        display_resolution_filter_tuple_append(800, 600, 16);
        display_resolution_filter_tuple_append(1024, 768, 16);
    }
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
        fullscreen = (windowed == 0) ? 1 : 0;
        selection.fullscreen = fullscreen;
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
            if (mode_count < 0) {
                if (s_directdraw_device_initialize_ops.error_show != NULL) {
                    s_directdraw_device_initialize_ops.error_show(
                        s_directdraw_device_initialize_ops.context,
                        "Please install DirectX version 6.");
                }
                return 0;
            }
            for (mode_index = 0; mode_index < mode_count; ++mode_index) {
                DirectDrawDisplayModeCandidate candidate;
                memset(&candidate, 0, sizeof(candidate));
                if (!s_directdraw_device_initialize_ops.mode_get(
                        s_directdraw_device_initialize_ops.context,
                        adapter.adapter, mode_index, &candidate)) continue;
                if (s_directdraw_device_initialize_ops.mode_supported(
                        s_directdraw_device_initialize_ops.context,
                        adapter.adapter, &candidate, windowed)) {
                    mode = candidate;
                    selection.adapter = adapter.adapter;
                    selection.mode_descriptor =
                        candidate.descriptor;
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
        selection.adapter = adapter.adapter;
        if (show_dialog != 0) {
            if (s_directdraw_device_initialize_ops.selection_dialog == NULL) {
                return 0;
            }
            result = s_directdraw_device_initialize_ops.selection_dialog(
                    s_directdraw_device_initialize_ops.context, 0,
                    &adapter, &mode, &fullscreen);
            if (result <= 0) {
                s_directdraw_device_selection_cancelled = (result < 0);
                return 0;
            }
            selection.adapter = adapter.adapter;
            selection.mode_descriptor = mode.descriptor;
            selection.fullscreen = fullscreen;
        }
    }
    selection.fullscreen = fullscreen;
    result = s_directdraw_device_initialize_ops.renderer_initialize(
        s_directdraw_device_initialize_ops.context, fullscreen,
        adapter.adapter, mode.descriptor != NULL ? &mode : NULL);
    if (result != 0) s_directdraw_device_selection = selection;
    return result;
}

/* RendererWindowModeApply (FUN_004780b0): retain the requested dimensions,
 * switch the platform mode, then size/place and activate the main window. */
// FUNCTION: LEGORR 0x004780b0
void renderer_window_mode_apply(int fullscreen, int x, int y,
                                int width, int height)
{
    int rect[4];
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
    HWND window = s_renderer_window_handle != NULL
        ? (HWND)s_renderer_window_handle : GetActiveWindow();
#endif
    s_renderer_window_width = width;
    s_renderer_window_height = height;
    if (fullscreen != 0) s_renderer_window_mode_flags |= 2u;
    window_mode_flag_set((uint32_t)fullscreen);
    window_runtime_mode_set(fullscreen, (int32_t)0x80c80000u);
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
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
        else if (window != NULL) {
            SetWindowLongA(window, -16, (LONG)0x80c80000u);
        }
#endif
        if (s_renderer_window_mode_ops.window_position_set != NULL) {
            s_renderer_window_mode_ops.window_position_set(
                s_renderer_window_mode_ops.context, rect[0], rect[1],
                rect[2], rect[3], 4u);
        }
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
        else if (window != NULL) {
            RECT native_rect;
            native_rect.left = rect[0];
            native_rect.top = rect[1];
            native_rect.right = rect[0] + rect[2];
            native_rect.bottom = rect[1] + rect[3];
            AdjustWindowRect(&native_rect, (DWORD)0x80c80000u, FALSE);
            SetWindowPos(window, NULL, native_rect.left, native_rect.top,
                native_rect.right - native_rect.left,
                native_rect.bottom - native_rect.top, 4u);
        }
#endif
        if (s_renderer_window_mode_ops.cursor_show != NULL) {
            s_renderer_window_mode_ops.cursor_show(
                s_renderer_window_mode_ops.context, 0);
        }
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
        else {
            (void)ShowCursor(FALSE);
        }
#endif
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
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
        else if (window != NULL) {
            HWND desktop = GetDesktopWindow();
            RECT desktop_rect;
            if (desktop != NULL && GetWindowRect(desktop, &desktop_rect)) {
                SetWindowPos(window, NULL, desktop_rect.left,
                    desktop_rect.top,
                    desktop_rect.right - desktop_rect.left,
                    desktop_rect.bottom - desktop_rect.top, 4u);
            }
        }
#endif
        if (s_renderer_window_mode_ops.cursor_hide != NULL) {
            s_renderer_window_mode_ops.cursor_hide(
                s_renderer_window_mode_ops.context);
        }
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
        else {
            SetCursor(NULL);
        }
#endif
    }
    if (s_renderer_window_mode_ops.window_show_activate != NULL) {
        s_renderer_window_mode_ops.window_show_activate(
            s_renderer_window_mode_ops.context, 5);
    }
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
    else if (window != NULL) {
        ShowWindow(window, 5);
        SetActiveWindow(window);
    }
#endif
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
}

/* RendererBeginFrameOnce (FUN_00477fa0): test the engine feature bit, issue
 * the renderer device update once, then set the bit. The current context and
 * device are owned by EngineTick and Direct3DRM device initialization. */
int renderer_begin_frame_once(void)
{
    uint32_t* feature = engine_current_feature_word();
    if (feature == NULL) return 0;
    if ((*feature & 1u) != 0u) return 1;
    if (s_renderer_begin_frame_hook != NULL) {
        s_renderer_begin_frame_hook(s_renderer_begin_frame_device);
    }
#if defined(LEGORRR_BACKEND_MINIWIN)
    else {
        if (s_screen_bounded_surface_device == NULL ||
            ((IDirect3DRMDevice2*)s_screen_bounded_surface_device)->Update()
                != DD_OK) return 0;
    }
#else
    else {
        return 0;
    }
#endif
    *feature |= 1u;
    return 1;
}

void renderer_begin_frame_once_reset(void)
{
    uint32_t* feature = engine_current_feature_word();
    if (feature != NULL) *feature &= ~1u;
}

/* Retail RendererBeginFrameOnce uses the initialized Direct3DRM device at
 * DAT_00505918 directly. The target calls vtable slot +0x38 after the device
 * setup path has installed the corresponding sidecar pointer. */
// FUNCTION: LEGORR 0x00477fa0
void renderer_begin_frame_once_retail(void)
{
    if ((s_engine_feature_word & 1u) != 0u) return;
#if defined(LEGORRR_BACKEND_MINIWIN)
    (void)((IDirect3DRMDevice2*)s_screen_bounded_surface_device)->Update();
#else
    typedef HRESULT (SURFACE_COM_CALL *SurfaceRendererUpdateFn)(void*);
    void** vtable = *(void***)s_screen_bounded_surface_device;
    SurfaceRendererUpdateFn update = (SurfaceRendererUpdateFn)vtable[14];
    (void)update(s_screen_bounded_surface_device);
#endif
    s_engine_feature_word |= 1u;
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
    void* object, int block, uint32_t* field_out, uint32_t reserved,
    uint32_t* kind_out, uint32_t* count_out, uint32_t* indices_out)
{
    void* owner = *(void**)((char*)object + 0x0c);
    void* mesh = *(void**)((char*)owner + 0x0c);
    if (mesh != NULL) {
        indexed_record_entry_read(mesh, block, field_out, reserved, kind_out,
                                  count_out, indices_out);
        return 0u;
    }
    else {
        void* interface_object = *(void**)((char*)owner + 8);
        int resolved_block = block;
        void** vtable;
        typedef int32_t (SURFACE_COM_CALL *PolygonVertexCountFn)(
            void*, int, uint32_t*, uint32_t, uint32_t*, uint32_t*,
            uint32_t*);
        (void)lego::animated_texture_frame_resolve(&interface_object,
                                                  &resolved_block);
        vtable = *(void***)interface_object;
        return (uint32_t)(((PolygonVertexCountFn)
            vtable[0x5c / sizeof(void*)])(
                interface_object, resolved_block, field_out, reserved,
                kind_out, count_out, indices_out) == 0);
    }
}

uint32_t lwo_layer_polygon_vertex_count_dispatch(
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
    void* object, int block, int first_vertex, int count, void* output)
{
    void* owner = *(void**)((char*)object + 0x0c);
    void* mesh = *(void**)((char*)owner + 0x0c);
    if (mesh != NULL) {
        indexed_geometry_data_copy_to_output(mesh, block, first_vertex,
                                             count, output);
        return 0u;
    }
    else {
        void* interface_object = *(void**)((char*)owner + 8);
        int resolved_block = block;
        void** vtable;
        typedef int32_t (SURFACE_COM_CALL *PolygonVertexDataReadFn)(
            void*, int, int, int, void*);
        (void)lego::animated_texture_frame_resolve(&interface_object,
                                                  &resolved_block);
        vtable = *(void***)interface_object;
        if (((PolygonVertexDataReadFn)vtable[0x60 / sizeof(void*)])(
                interface_object, resolved_block, first_vertex, count,
                output) != 0) {
            return 0xffffffffu;
        }
        return 0u;
    }
}

uint32_t lwo_layer_polygon_vertex_data_read_dispatch(
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
uint32_t terrain_detail_entry_get_at_index(void* object,
                                           unsigned int index)
{
    void* owner = *(void**)((char*)object + 0x0c);
    void* mesh = *(void**)((char*)owner + 0x0c);
    if (mesh != NULL) {
        return indexed_record_flag6c_bit0_get(mesh, (int)index);
    }
    else {
        void* layer = *(void**)((char*)owner + 8);
        void** layer_vtable = *(void***)layer;
        typedef void* (SURFACE_COM_CALL *LayerCollectionGetFn)(void*);
        void* children = ((LayerCollectionGetFn)
            layer_vtable[0x1c / sizeof(void*)])(layer);
        if (children != NULL) {
            if (index != 0u) {
                void* data = *(void**)children;
                void* child = *(void**)((char*)data - sizeof(uint32_t) +
                    (size_t)index * sizeof(void*));
                void** child_vtable = *(void***)child;
                typedef uint32_t (SURFACE_COM_CALL *LayerFlagsGetFn)(void*);
                uint32_t flags = ((LayerFlagsGetFn)
                    child_vtable[0x1c / sizeof(void*)])(child);
                return ~flags >> 31;
            }
            return *(uint32_t*)((char*)children + 0x0c);
        }
        return index;
    }
}

uint32_t terrain_detail_entry_get_at_index_dispatch(
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
    /* FUN_00486690 clears DAT_005407e8 before returning failure for an
     * unsupported depth. Keep the portable global and output in sync so a
     * failed reconfiguration cannot leave the previous surface writer live. */
    s_surface_pixel_writer = NULL;
    *out_writer = NULL;
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

/* Portable callback adapter for SurfaceLockConfigurePixelWriter. */
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

/* SurfaceLockConfigurePixelWriter (FUN_00486550) locks the DirectDraw
 * surface and takes bpp plus channel masks from DDSURFACEDESC2 before it
 * installs the selected writer. The portable adapter receives the surface
 * owner from the frame binding and retains the lock through the draw call. */
// FUNCTION: LEGORR 0x00486550
int surface_active_pixel_writer_configure(
    int format, const SurfacePixelWriterTable* writers)
{
#if defined(LEGORRR_BACKEND_MINIWIN) || \
    (defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64))
    DisplaySurface* target = s_active_render_target;
    void* surface = target != NULL ? target->surf : NULL;
    void** vtable = surface_interface_vtable(surface);
    typedef int32_t (SURFACE_COM_CALL *SurfaceRawLockFn)(
        void*, void*, void*, uint32_t, void*);
    typedef int32_t (SURFACE_COM_CALL *SurfaceRawUnlockFn)(void*, void*);
    SurfaceRawLockFn lock;
    SurfaceRawUnlockFn unlock;
#if defined(LEGORRR_BACKEND_MINIWIN)
    DDSURFACEDESC descriptor;
#else
    uint32_t descriptor[31];
    uint32_t bits_per_pixel;
    uint32_t red_mask;
    uint32_t green_mask;
    uint32_t blue_mask;
    const unsigned char* bytes = (const unsigned char*)descriptor;
#endif
    SurfacePixelWriteFn writer = NULL;

    if (surface == NULL || vtable == NULL || vtable[25] == NULL ||
        vtable[32] == NULL || writers == NULL) return 0;
    lock = (SurfaceRawLockFn)vtable[25];
    unlock = (SurfaceRawUnlockFn)vtable[32];
#if defined(LEGORRR_BACKEND_MINIWIN)
    memset(&descriptor, 0, sizeof(descriptor));
    descriptor.dwSize = sizeof(descriptor);
#else
    memset(descriptor, 0, sizeof(descriptor));
    descriptor[0] = 0x7cu;
#endif
    if (lock(surface, NULL, descriptor, 1u, NULL) != 0) return 0;
#if defined(LEGORRR_BACKEND_MINIWIN)
    bits_per_pixel = descriptor.ddpfPixelFormat.dwRGBBitCount;
    red_mask = descriptor.ddpfPixelFormat.dwRBitMask;
    green_mask = descriptor.ddpfPixelFormat.dwGBitMask;
    blue_mask = descriptor.ddpfPixelFormat.dwBBitMask;
#else
    memcpy(&bits_per_pixel, bytes + 88u, sizeof(bits_per_pixel));
    memcpy(&red_mask, bytes + 92u, sizeof(red_mask));
    memcpy(&green_mask, bytes + 96u, sizeof(green_mask));
    memcpy(&blue_mask, bytes + 100u, sizeof(blue_mask));
#endif

    s_surface_pixel_bits = bits_per_pixel;
    s_surface_red_mask_bits = (uint32_t)pop_count32(red_mask);
    s_surface_green_mask_bits = (uint32_t)pop_count32(green_mask);
    s_surface_blue_mask_bits = (uint32_t)pop_count32(blue_mask);
    if (!surface_pixel_writer_select_by_format(bits_per_pixel, format,
                                                writers, &writer)) {
        (void)unlock(surface, NULL);
        s_surface_pixel_bits = 0u;
        s_surface_red_mask_bits = 0u;
        s_surface_green_mask_bits = 0u;
        s_surface_blue_mask_bits = 0u;
        s_surface_pixel_writer = NULL;
        return 0;
    }
    return 1;
#else
    (void)format;
    (void)writers;
    return 0;
#endif
}

void surface_writer_unlock_hook_set(SurfaceWriterUnlockHook hook)
{
    s_surface_writer_unlock_hook = hook;
}

static void surface_directdraw_unlock_call(void* surface)
{
    if (surface == NULL) return;
#if defined(LEGORRR_BACKEND_MINIWIN)
    (void)((LPDIRECTDRAWSURFACE)surface)->Unlock(NULL);
#elif defined(_WIN32)
    typedef long (__stdcall *SurfaceUnlockProc)(void*, void*);
    void** vtable = *(void***)surface;
    if (vtable == NULL || vtable[32] == NULL) return;
    SurfaceUnlockProc unlock = (SurfaceUnlockProc)vtable[32];
    (void)unlock(surface, NULL);
#else
    (void)surface;
#endif
}

/* DirectDrawSurfaceUnlock (FUN_0047ded0): forward a full-surface unlock to
 * the installed platform interface callback. */
// FUNCTION: LEGORR 0x0047ded0
void surface_directdraw_unlock(void* surface)
{
    if (s_surface_writer_unlock_hook != NULL) {
        s_surface_writer_unlock_hook(surface, 0);
    } else {
        surface_directdraw_unlock_call(surface);
    }
}

/* SurfaceUnlockResetPixelWriter (FUN_00486650): unlock the DirectDraw target,
 * then clear the format and active pixel-writer globals. */
// FUNCTION: LEGORR 0x00486650
void surface_unlock_reset_pixel_writer(void* surface)
{
    surface_directdraw_unlock(surface);
    s_surface_pixel_bits = 0u;
    s_surface_red_mask_bits = 0u;
    s_surface_green_mask_bits = 0u;
    s_surface_blue_mask_bits = 0u;
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

// FUNCTION: LEGORR 0x00479ee0
void* font_atlas_record_create(const char* name)
{
    TextureSurface* surface;
    FontAtlasRecord* record;
    int cell_width;
    int cell_height;
    int row;
    int column;
    uint32_t key1;

    if (name == NULL || name[0] == '\0') return NULL;
    surface = image_surface_load(name, 0u, 0u);
    if (surface == NULL || surface->pixels == NULL ||
        surface->width < 10 || surface->height < 19) {
        texture_release(surface);
        return NULL;
    }
    cell_width = surface->width / 10;
    cell_height = surface->height / 19;
    if (cell_width <= 0 || cell_height <= 0) {
        texture_release(surface);
        return NULL;
    }
    key1 = surface->key_count >= 2 ? surface->keys[1] : surface->pixels[0];

    record = s_font_atlas_record_free_list;
    if (record != NULL) {
        s_font_atlas_record_free_list = record->free_next;
        memset(record, 0, sizeof(*record));
    }
    else {
        record = (FontAtlasRecord*)calloc(1, sizeof(*record));
    }
    if (record == NULL) {
        texture_release(surface);
        return NULL;
    }
    record->surface = surface;
    record->line_height = (uint32_t)cell_height;
    record->tab_width = (uint32_t)(cell_width * 8);
    for (column = 0; column < 10; ++column) {
        for (row = 0; row < 19; ++row) {
            FontAtlasGlyphRecord* glyph =
                &record->glyphs[column * 19 + row];
            int x0 = column * cell_width;
            int y0 = row * cell_height;
            int width = cell_width;
            int x;
            glyph->x = (float)x0;
            glyph->y = (float)y0;
            glyph->height = (float)cell_height;
            /* FontResourceLoad scans the first row against the second
             * palette key, retaining at least one pixel of advance. The
             * first key may encode spacing without visible glyph ink. */
            for (x = cell_width - 1; x > 0; --x) {
                uint32_t pixel = surface->pixels[(size_t)y0 *
                    (size_t)surface->width + (size_t)(x0 + x)];
                if (pixel != key1) break;
                --width;
            }
            glyph->width = (float)width;
        }
    }
    return record;
}

TextureSurface* font_atlas_record_surface_get(const void* opaque_record)
{
    const FontAtlasRecord* record = (const FontAtlasRecord*)opaque_record;
    return record != NULL ? record->surface : NULL;
}

int font_atlas_record_line_height_get(const void* opaque_record)
{
    const FontAtlasRecord* record = (const FontAtlasRecord*)opaque_record;
    return record != NULL ? (int)record->line_height : 0;
}

int font_atlas_record_glyph_get(const void* opaque_record,
                                unsigned char glyph, float rect_out[4])
{
    const FontAtlasRecord* record = (const FontAtlasRecord*)opaque_record;
    const FontAtlasGlyphRecord* entry;
    unsigned int index;
    if (record == NULL || rect_out == NULL || glyph < 0x20u ||
        glyph >= 0x20u + 10u * 19u) return 0;
    index = (unsigned int)glyph - 0x20u;
    entry = &record->glyphs[(index % 10u) * 19u + (index / 10u)];
    rect_out[0] = entry->x;
    rect_out[1] = entry->y;
    rect_out[2] = entry->width;
    rect_out[3] = entry->height;
    return 1;
}

// FUNCTION: LEGORR 0x0047A550
void font_atlas_record_release(void* opaque_record)
{
    FontAtlasRecord* record = (FontAtlasRecord*)opaque_record;
    if (record == NULL) return;
    texture_release(record->surface);
    record->surface = NULL;
    record->free_next = s_font_atlas_record_free_list;
    s_font_atlas_record_free_list = record;
}

void font_pixel_format_hooks_set(FontPixelFormatQueryFn query,
                                 FontPixelFormatReleaseFn release)
{
    s_font_pixel_format_query = query;
    s_font_pixel_format_release = release;
}

/* FontDeviceColorConvert receives the raw DirectDraw surface interface used
 * by FontRecordCreate. The retail code locks it with a 0x7c-byte descriptor,
 * reads the RGB masks, and unlocks it. Keep that operation on the raw surface
 * here; the portable DisplaySurface wrapper is a different object contract. */
static int font_device_pixel_format_query(void* device,
                                         uint32_t* bits_per_pixel,
                                         uint32_t* red_mask,
                                         uint32_t* green_mask,
                                         uint32_t* blue_mask)
{
    if (device == NULL || bits_per_pixel == NULL || red_mask == NULL ||
        green_mask == NULL || blue_mask == NULL) return 1;
#if defined(LEGORRR_BACKEND_MINIWIN)
    {
        DDSURFACEDESC descriptor;
        HRESULT result;
        memset(&descriptor, 0, sizeof(descriptor));
        descriptor.dwSize = 0x7cu;
        result = ((IDirectDrawSurface*)device)->Lock(
            NULL, &descriptor, (DDLockFlags)1, NULL);
        if (result != DD_OK) return 1;
        *bits_per_pixel = descriptor.ddpfPixelFormat.dwRGBBitCount;
        *red_mask = descriptor.ddpfPixelFormat.dwRBitMask;
        *green_mask = descriptor.ddpfPixelFormat.dwGBitMask;
        *blue_mask = descriptor.ddpfPixelFormat.dwBBitMask;
        (void)((IDirectDrawSurface*)device)->Unlock(NULL);
        return 0;
    }
#elif defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
    {
        typedef HRESULT (SURFACE_COM_CALL *SurfaceLockFn)(
            void*, void*, void*, DWORD, HANDLE);
        typedef HRESULT (SURFACE_COM_CALL *SurfaceUnlockFn)(void*, void*);
        unsigned char descriptor[0x7c];
        void** vtable = surface_interface_vtable(device);
        SurfaceLockFn lock_surface;
        SurfaceUnlockFn unlock_surface;
        HRESULT result;
        DWORD descriptor_size = 0x7cu;
        if (vtable == NULL || vtable[25] == NULL || vtable[32] == NULL)
            return 1;
        lock_surface = (SurfaceLockFn)vtable[25];
        unlock_surface = (SurfaceUnlockFn)vtable[32];
        memset(descriptor, 0, sizeof(descriptor));
        memcpy(descriptor, &descriptor_size, sizeof(descriptor_size));
        result = lock_surface(device, NULL, descriptor, 1u, NULL);
        if (result != 0) return 1;
        memcpy(bits_per_pixel, descriptor + 0x54u, sizeof(*bits_per_pixel));
        memcpy(red_mask, descriptor + 0x58u, sizeof(*red_mask));
        memcpy(green_mask, descriptor + 0x5cu, sizeof(*green_mask));
        memcpy(blue_mask, descriptor + 0x60u, sizeof(*blue_mask));
        (void)unlock_surface(device, NULL);
        return 0;
    }
#else
    return 1;
#endif
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
    if ((s_font_pixel_format_query != NULL
             ? s_font_pixel_format_query(device, &bits_per_pixel, &red_mask,
                                         &green_mask, &blue_mask)
             : font_device_pixel_format_query(device, &bits_per_pixel,
                                              &red_mask, &green_mask,
                                              &blue_mask)) != 0) {
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
    if (s_font_pixel_format_query != NULL &&
        s_font_pixel_format_release != NULL) {
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
        /* The retail encoder packs channels by their mask bit counts (red
         * above green above blue); it does not shift according to each
         * mask's first set bit. Standard DirectDraw RGB layouts produce the
         * same result either way, while this preserves the reference result
         * for unusual/non-contiguous masks too. */
        return ((red >> ((8u - red_bits) & 31u))
                << ((green_bits + blue_bits) & 31u)) |
               ((green >> ((8u - green_bits) & 31u))
                << (blue_bits & 31u)) |
               (blue >> ((8u - blue_bits) & 31u));
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

/* Convert the frontend's RGBA bytes using the destination surface's masks. */
static int surface_rgba_mask_info(uint32_t mask, unsigned int* shift,
                                  unsigned int* bits)
{
    unsigned int s = 0, n = 0;
    if (mask == 0u) return 0;
    while ((mask & 1u) == 0u) { mask >>= 1; ++s; }
    while ((mask & 1u) != 0u) { mask >>= 1; ++n; }
    if (mask != 0u || n > 8u) return 0;
    *shift = s; *bits = n;
    return 1;
}

int surface_rgba_copy_to_rgb(void* target, int target_pitch, int target_width,
    int target_height, const void* source, int source_pitch, int width,
    int height, int x, int y, unsigned int bpp, uint32_t red_mask,
    uint32_t green_mask, uint32_t blue_mask)
{
    unsigned int rs, gs, bs, rb, gb, bb, bytes;
    int sx, sy, dx, dy, copy_width, copy_height, row, column;
    if (target == NULL || source == NULL || width <= 0 || height <= 0 ||
        target_width <= 0 || target_height <= 0 || source_pitch <= 0 ||
        target_pitch <= 0 || (bpp != 16u && bpp != 32u)) return 0;
    bytes = bpp / 8u;
    if (width > source_pitch / 4 || target_width > target_pitch / (int)bytes ||
        (red_mask & green_mask) != 0u || (red_mask & blue_mask) != 0u ||
        (green_mask & blue_mask) != 0u ||
        (bpp == 16u && ((red_mask | green_mask | blue_mask) & 0xffff0000u)) ||
        !surface_rgba_mask_info(red_mask, &rs, &rb) ||
        !surface_rgba_mask_info(green_mask, &gs, &gb) ||
        !surface_rgba_mask_info(blue_mask, &bs, &bb)) return 0;
    if (x <= -width || y <= -height || x >= target_width || y >= target_height)
        return 1;
    sx = x < 0 ? -x : 0; sy = y < 0 ? -y : 0;
    dx = x < 0 ? 0 : x; dy = y < 0 ? 0 : y;
    copy_width = width - sx; copy_height = height - sy;
    if (copy_width > target_width - dx) copy_width = target_width - dx;
    if (copy_height > target_height - dy) copy_height = target_height - dy;
    for (row = 0; row < copy_height; ++row) {
        const unsigned char* src = (const unsigned char*)source +
            (sy + row) * source_pitch + sx * 4;
        unsigned char* dst = (unsigned char*)target +
            (dy + row) * target_pitch + dx * bytes;
        for (column = 0; column < copy_width; ++column) {
            uint32_t value = (((uint32_t)src[0] >> (8u - rb)) << rs) |
                (((uint32_t)src[1] >> (8u - gb)) << gs) |
                (((uint32_t)src[2] >> (8u - bb)) << bs);
            unsigned int i;
            for (i = 0; i < bytes; ++i) dst[i] = (unsigned char)(value >> (8u * i));
            src += 4; dst += bytes;
        }
    }
    return 1;
}

/* ---- DirectDraw-backed implementation (miniwin) ---------------------- */

#if defined(LEGORRR_BACKEND_MINIWIN)

static LPDIRECTDRAW s_dd(void* p)      { return (LPDIRECTDRAW)p; }
static LPDIRECTDRAWSURFACE s_surf(void* p) { return (LPDIRECTDRAWSURFACE)p; }
static IDirect3DRM* s_screen_renderer_rm;
static IDirect3DRMDevice2* s_screen_renderer_device;
static void* s_screen_renderer_surface;

struct MiniwinRendererGuidList {
    GUID values[16];
    unsigned int count;
};

static HRESULT miniwin_renderer_guid_collect(LPGUID guid, LPSTR, LPSTR,
    LPD3DDEVICEDESC, LPD3DDEVICEDESC, LPVOID context)
{
    MiniwinRendererGuidList* list = (MiniwinRendererGuidList*)context;
    if (guid != NULL && list != NULL && list->count < 16u)
        list->values[list->count++] = *guid;
    return DD_OK;
}

static void miniwin_screen_renderer_release(void)
{
    if (s_screen_renderer_device != NULL) {
        s_screen_renderer_device->Release();
        s_screen_renderer_device = NULL;
    }
    if (s_screen_renderer_rm != NULL) {
        s_screen_renderer_rm->Release();
        s_screen_renderer_rm = NULL;
    }
    s_screen_renderer_surface = NULL;
    screen_bounded_surface_device_set(NULL, NULL);
}

static void miniwin_screen_renderer_initialize(DisplaySurface* surface)
{
    IDirect3D2* d3d = NULL;
    MiniwinRendererGuidList list;
    unsigned int i;
    memset(&list, 0, sizeof(list));
    miniwin_screen_renderer_release();
    if (surface == NULL || surface->dd == NULL || surface->surf == NULL)
        return;
    if (s_dd(surface->dd)->QueryInterface(IID_IDirect3D2,
            (void**)&d3d) != DD_OK || d3d == NULL) return;
    if (d3d->EnumDevices(miniwin_renderer_guid_collect, &list) != DD_OK) {
        d3d->Release();
        return;
    }
    d3d->Release();
    if (list.count == 0u || Direct3DRMCreate(&s_screen_renderer_rm) != DD_OK ||
        s_screen_renderer_rm == NULL) return;
    for (i = 0; i < list.count; ++i) {
        if (s_screen_renderer_rm->CreateDeviceFromSurface(
                &list.values[i], s_dd(surface->dd), s_surf(surface->surf),
                &s_screen_renderer_device) == DD_OK &&
            s_screen_renderer_device != NULL) {
            s_screen_renderer_surface = surface->surf;
            screen_bounded_surface_device_set(s_screen_renderer_rm,
                                               s_screen_renderer_device);
            return;
        }
        if (s_screen_renderer_device != NULL) {
            s_screen_renderer_device->Release();
            s_screen_renderer_device = NULL;
        }
    }
    miniwin_screen_renderer_release();
}

int surface_create(DisplaySurface* out, void* hwnd, int w, int h)
{
    LPDIRECTDRAW dd = NULL;
    if (out == NULL || w <= 0 || h <= 0) {
        return -1;
    }
    if (DirectDrawCreate(NULL, &dd, NULL) != DD_OK || dd == NULL) {
        if (dd != NULL) dd->Release();
        return -2;
    }
    if (dd->SetCooperativeLevel((HWND)hwnd, DDSCL_NORMAL) != DD_OK) {
        dd->Release();
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
        if (surf != NULL) surf->Release();
        dd->Release();
        return -4;
    }
    out->dd = dd;
    out->surf = surf;
    out->hwnd = hwnd;
    out->w = w;
    out->h = h;
    out->palette = NULL;
    out->format_flags = 0u;
    miniwin_screen_renderer_initialize(out);
    return 0;
}

void surface_destroy(DisplaySurface* s)
{
    if (s == NULL) {
        return;
    }
    if (s->surf != NULL && s->surf == s_screen_renderer_surface)
        miniwin_screen_renderer_release();
    if (s->surf != NULL) {
        s_surf(s->surf)->Release();
    }
    if (s->dd != NULL) {
        s_dd(s->dd)->Release();
    }
    s->dd = NULL;
    s->surf = NULL;
    s->palette = NULL;
    s->format_flags = 0u;
}

int surface_width(DisplaySurface* s)  { return s != NULL ? s->w : 0; }
int surface_height(DisplaySurface* s) { return s != NULL ? s->h : 0; }

int surface_palette_entries_set(DisplaySurface* surface, unsigned int start,
                                unsigned int count,
                                const SurfacePaletteEntry* entries)
{
    PALETTEENTRY native_entries[256];
    unsigned int index;
    if (surface == NULL || surface->palette == NULL || entries == NULL ||
        count == 0u || start >= 256u || count > 256u - start) return 0;
    for (index = 0u; index < count; ++index) {
        native_entries[index].peRed = entries[index].red;
        native_entries[index].peGreen = entries[index].green;
        native_entries[index].peBlue = entries[index].blue;
        native_entries[index].peFlags = entries[index].flags;
    }
    return ((LPDIRECTDRAWPALETTE)surface->palette)->SetEntries(
        0u, start, count, native_entries) == DD_OK;
}

int surface_lock(DisplaySurface* s, void** pixels, int* pitch)
{
    if (s == NULL || s->surf == NULL || pixels == NULL || pitch == NULL) {
        return -1;
    }
    DDSURFACEDESC d;
    memset(&d, 0, sizeof(d));
    d.dwSize = sizeof(d);
    if (s_surf(s->surf)->Lock(NULL, &d,
                              static_cast<DDLockFlags>(0x00000801u),
                              NULL) != DD_OK) {
        return -2;
    }
    *pixels = d.lpSurface;
    *pitch = d.lPitch;
    return 0;
}

void surface_unlock(DisplaySurface* s, void* pixels)
{
    (void)pixels;
    if (s == NULL || s->surf == NULL) {
        return;
    }
    s_surf(s->surf)->Unlock(NULL);
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
    struct tagPOINT origin;
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
    ClientToScreen((HWND)primary->hwnd, (LPPOINT)&origin);
    destination_rect.left += origin.x;
    destination_rect.right += origin.x;
    destination_rect.top += origin.y;
    destination_rect.bottom += origin.y;
    s_surf(primary->surf)->Blt(&destination_rect, s_surf(back_surface->surf),
                               &source_rect, DDBLT_WAIT, NULL);
}

static void surface_blit_wait_impl(void* destination,
                                   DisplaySurface* source)
{
    if (destination == NULL || source == NULL || source->surf == NULL) return;
    s_surf(destination)->Blt(NULL, s_surf(source->surf), NULL, DDBLT_WAIT,
                            NULL);
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

#elif defined(LEGORRR_BACKEND_WINAPI)

/* The WinAPI build intentionally avoids miniwin's shortened COM interfaces.
 * The reference requests IDirectDraw4/IDirectDrawSurface4 and passes the
 * 124-byte DDSURFACEDESC2 layout. Declare only the ABI pieces used here. */
#if defined(_WIN32) && !defined(_WIN64)
typedef struct SurfaceDdrawColorKey {
    DWORD low;
    DWORD high;
} SurfaceDdrawColorKey;

typedef struct SurfaceDdrawPixelFormat {
    DWORD size;
    DWORD flags;
    DWORD fourcc;
    DWORD rgb_bits;
    DWORD red_mask;
    DWORD green_mask;
    DWORD blue_mask;
    DWORD alpha_mask;
} SurfaceDdrawPixelFormat;

typedef struct SurfaceDdrawPaletteEntry {
    BYTE red;
    BYTE green;
    BYTE blue;
    BYTE flags;
} SurfaceDdrawPaletteEntry;
typedef char SurfaceDdrawPaletteEntryMustBe4Bytes[
    sizeof(SurfaceDdrawPaletteEntry) == 4u ? 1 : -1];

typedef struct SurfaceDdrawCaps2 {
    DWORD caps;
    DWORD caps2;
    DWORD caps3;
    DWORD caps4;
} SurfaceDdrawCaps2;

typedef struct SurfaceDdrawDesc2 {
    DWORD size;
    DWORD flags;
    DWORD height;
    DWORD width;
    union { LONG pitch; DWORD linear_size; };
    DWORD backbuffer_count;
    union { DWORD mipmap_count; DWORD zbuffer_depth; DWORD refresh_rate; };
    DWORD alpha_depth;
    DWORD reserved;
    void* surface;
    SurfaceDdrawColorKey keys[4];
    SurfaceDdrawPixelFormat pixel_format;
    SurfaceDdrawCaps2 caps;
    DWORD texture_stage;
} SurfaceDdrawDesc2;

typedef char SurfaceDdrawDesc2MustBe124Bytes[
    sizeof(SurfaceDdrawDesc2) == 0x7cu ? 1 : -1];
typedef char SurfaceDdrawPixelFormatMustBe32Bytes[
    sizeof(SurfaceDdrawPixelFormat) == 0x20u ? 1 : -1];

static const GUID s_iid_direct_draw4 = {
    0x9c59509a, 0x39bd, 0x11d1,
    { 0x8c, 0x4a, 0x00, 0xc0, 0x4f, 0xd9, 0x30, 0xc5 }
};
static HMODULE s_surface_ddraw_module;
static FARPROC s_surface_ddraw_create;

typedef HRESULT (WINAPI *SurfaceDdrawCreateFn)(GUID*, void**, void*);
typedef HRESULT (WINAPI *SurfaceQueryInterfaceFn)(void*, REFIID, void**);
typedef ULONG (WINAPI *SurfaceReleaseFn)(void*);
typedef HRESULT (WINAPI *SurfaceSetCooperativeLevelFn)(void*, HWND, DWORD);
typedef HRESULT (WINAPI *SurfaceSetDisplayModeFn)(void*, DWORD, DWORD,
                                                   DWORD, DWORD, DWORD);
typedef HRESULT (WINAPI *SurfaceCreateSurfaceFn)(void*, SurfaceDdrawDesc2*,
                                                 void**, void*);
typedef HRESULT (WINAPI *SurfaceCreateClipperFn)(void*, DWORD, void**, void*);
typedef HRESULT (WINAPI *SurfaceCreatePaletteFn)(void*, DWORD,
    SurfaceDdrawPaletteEntry*, void**, void*);
typedef HRESULT (WINAPI *SurfaceGetAttachedSurfaceFn)(void*,
    SurfaceDdrawCaps2*, void**);
typedef HRESULT (WINAPI *SurfaceSetClipperFn)(void*, void*);
typedef HRESULT (WINAPI *ClipperSetClipListFn)(void*, const RGNDATA*, DWORD);
typedef HRESULT (WINAPI *SurfaceLockFn)(void*, RECT*, SurfaceDdrawDesc2*,
                                        DWORD, HANDLE);
typedef HRESULT (WINAPI *SurfaceUnlockFn)(void*, RECT*);
typedef HRESULT (WINAPI *SurfaceSetPaletteFn)(void*, void*);
typedef HRESULT (WINAPI *SurfacePaletteSetEntriesFn)(void*, DWORD, DWORD,
    DWORD, SurfaceDdrawPaletteEntry*);
typedef HRESULT (WINAPI *SurfaceBltFn)(void*, RECT*, void*, RECT*, DWORD,
                                       void*);
typedef HRESULT (WINAPI *SurfaceGetDescriptionFn)(void*,
                                                   SurfaceDdrawDesc2*);
typedef HRESULT (WINAPI *SurfaceGetPixelFormatFn)(void*,
                                                  SurfaceDdrawPixelFormat*);

static void** surface_com_vtable(void* object)
{
    return object != NULL ? *(void***)object : NULL;
}

static ULONG surface_com_release(void* object)
{
    void** vtable = surface_com_vtable(object);
    if (vtable == NULL || vtable[2] == NULL) return 0;
    return ((SurfaceReleaseFn)vtable[2])(object);
}

static int surface_ddraw_api_get(SurfaceDdrawCreateFn* create)
{
    if (create == NULL) return 0;
    if (s_surface_ddraw_create == NULL) {
        s_surface_ddraw_module = LoadLibraryA("ddraw.dll");
        if (s_surface_ddraw_module == NULL) return 0;
        s_surface_ddraw_create = GetProcAddress(
            s_surface_ddraw_module, "DirectDrawCreate");
        if (s_surface_ddraw_create == NULL) return 0;
    }
    *create = (SurfaceDdrawCreateFn)s_surface_ddraw_create;
    return 1;
}

typedef struct WinapiRendererState {
    void* device;
    void* primary;
    void* back_surface;
    void* clipper;
    void* rm;
    void* rm_device;
    void* immediate_device;
    void* staging_surface;
    HMODULE rm_module;
    HWND window;
    int fullscreen;
    int width;
    int height;
} WinapiRendererState;

static WinapiRendererState s_winapi_renderer_state;
void* surface_winapi_immediate_device_peek(void)
{
    return s_winapi_renderer_state.immediate_device;
}

static int winapi_renderer_active(void)
{
    return s_winapi_renderer_state.device != NULL;
}
// GLOBAL: LEGORR 0x0076acbc
static uint32_t s_renderer_fullscreen;
// GLOBAL: LEGORR 0x0076acc0
static uint32_t s_renderer_surface_width;
// GLOBAL: LEGORR 0x0076acc4
static uint32_t s_renderer_surface_height;

static void windowed_surface_blit_impl(DisplaySurface* primary,
                                       DisplaySurface* back_surface);

static void winapi_renderer_state_release(WinapiRendererState* state)
{
    void* immediate;
    if (state == NULL) return;
    immediate = state->immediate_device;
    state->immediate_device = NULL;
    surface_com_release(immediate);
    if (s_screen_bounded_surface_device == state->rm_device &&
        state->rm_device != NULL)
        screen_bounded_surface_device_set(NULL, NULL);
    if (state->rm_device != NULL) surface_com_release(state->rm_device);
    if (state->rm != NULL) surface_com_release(state->rm);
    if (state->rm_module != NULL) FreeLibrary(state->rm_module);
    if (state->clipper != NULL) surface_com_release(state->clipper);
    if (state->back_surface != NULL)
        surface_com_release(state->back_surface);
    if (state->primary != NULL) surface_com_release(state->primary);
    if (state->device != NULL && state->fullscreen) {
        typedef HRESULT (WINAPI *RestoreFn)(void*);
        void** vt = surface_com_vtable(state->device);
        if (vt != NULL && vt[19] != NULL) ((RestoreFn)vt[19])(state->device);
    }
    if (state->device != NULL) surface_com_release(state->device);
    memset(state, 0, sizeof(*state));
    s_renderer_window_handle = NULL;
    s_renderer_primary_surface = NULL;
    render_surface_secondary_set(0u);
    s_renderer_fullscreen = 0u;
    s_renderer_surface_width = 0u;
    s_renderer_surface_height = 0u;
}

/* WindowedRenderSurfaceBlit (FUN_0047cc50): derive the screen-space target
 * rectangle, then blit the global back surface into the primary surface. */
// FUNCTION: LEGORR 0x0047cc50
static void windowed_render_surface_blit_retail(void)
{
    struct WindowedRenderSurfaceLocal {
        POINT origin;
        RECT destination;
        RECT source;
    } local;
    void** vtable;
    SurfaceBltFn blt;
    local.destination.right = (LONG)s_renderer_surface_width;
    local.origin.y = 0;
    local.origin.x = 0;
    local.destination.left = 0;
    local.destination.top = 0;
    local.destination.bottom = (LONG)s_renderer_surface_height;
    ClientToScreen(s_renderer_window_handle, &local.origin);
    OffsetRect(&local.destination, local.origin.x, local.origin.y);
    local.source.bottom = (LONG)s_renderer_surface_height;
    local.source.top = 0;
    local.source.left = 0;
    local.source.right = (LONG)s_renderer_surface_width;
    vtable = *(void***)s_renderer_primary_surface;
    blt = (SurfaceBltFn)vtable[5];
    (void)blt(s_renderer_primary_surface, &local.destination,
              (void*)s_render_surface_secondary, &local.source,
              0x01000000u, NULL);
}

/* RenderSurfacePresent (FUN_0047c8d0) chooses fullscreen flip or windowed
 * global-surface blit from the renderer mode word. */
// FUNCTION: LEGORR 0x0047c8d0
void render_surface_present(void)
{
    if (s_renderer_fullscreen != 0u) {
        typedef HRESULT (SURFACE_COM_CALL *SurfaceFlipFn)(void*, void*, DWORD);
        void** vtable = *(void***)s_renderer_primary_surface;
        SurfaceFlipFn flip = (SurfaceFlipFn)vtable[11];
        (void)flip(s_renderer_primary_surface, NULL, 1u);
        return;
    }
    windowed_render_surface_blit_retail();
}

static int winapi_renderer_clipper_attach(void* device, void* surface,
    HWND window, void** clipper_out)
{
    SurfaceCreateClipperFn create_clipper;
    typedef HRESULT (WINAPI *ClipperSetHwndFn)(void*, DWORD, HWND);
    ClipperSetHwndFn set_hwnd;
    SurfaceSetClipperFn set_clipper;
    void** vtable;
    void* clipper = NULL;
    int success = 0;

    if (clipper_out != NULL) *clipper_out = NULL;
    if (device == NULL || surface == NULL || clipper_out == NULL ||
        window == NULL) return 0;
    vtable = surface_com_vtable(device);
    create_clipper = vtable != NULL && vtable[4] != NULL
        ? (SurfaceCreateClipperFn)vtable[4] : NULL;
    if (create_clipper == NULL ||
        create_clipper(device, 0u, &clipper, NULL) != 0 || clipper == NULL)
        goto cleanup;

    vtable = surface_com_vtable(clipper);
    set_hwnd = vtable != NULL && vtable[8] != NULL
        ? (ClipperSetHwndFn)vtable[8] : NULL;
    if (set_hwnd == NULL || set_hwnd(clipper, 0u, window) != 0) goto cleanup;
    vtable = surface_com_vtable(surface);
    set_clipper = vtable != NULL && vtable[28] != NULL
        ? (SurfaceSetClipperFn)vtable[28] : NULL;
    if (set_clipper == NULL || set_clipper(surface, clipper) != 0)
        goto cleanup;
    *clipper_out = clipper;
    clipper = NULL;
    success = 1;

cleanup:
    if (clipper != NULL) surface_com_release(clipper);
    return success;
}

static int winapi_rm_device_create(void* rm1, void* device, void* back_surface,
    const GUID* device_guid, void** rm_out, void** device_out)
{
    typedef HRESULT (WINAPI *CreateDeviceFn)(void*, const GUID*, void*,
                                              void*, DWORD, void**);
    static const GUID iid_rm3 = { 0x4516ec83, 0x8f20, 0x11d0,
        { 0x9b, 0x6d, 0x00, 0x00, 0xc0, 0x78, 0x1b, 0xc3 } };
    static const GUID iid_dd1 = { 0x6c14db80, 0xa733, 0x11ce,
        { 0xa5, 0x21, 0x00, 0x20, 0xaf, 0x0b, 0xe5, 0x60 } };
    static const GUID iid_surface1 = { 0x6c14db81, 0xa733, 0x11ce,
        { 0xa5, 0x21, 0x00, 0x20, 0xaf, 0x0b, 0xe5, 0x60 } };
    void* dd1 = NULL;
    void* surface1 = NULL;
    int ok = 0;
    *rm_out = NULL;
    *device_out = NULL;
    if (device_guid != NULL &&
        ((SurfaceQueryInterfaceFn)surface_com_vtable(rm1)[0])(
            rm1, iid_rm3, rm_out) == 0 && *rm_out != NULL &&
        ((SurfaceQueryInterfaceFn)surface_com_vtable(device)[0])(
            device, iid_dd1, &dd1) == 0 && dd1 != NULL &&
        ((SurfaceQueryInterfaceFn)surface_com_vtable(back_surface)[0])(
            back_surface, iid_surface1, &surface1) == 0 && surface1 != NULL &&
        ((CreateDeviceFn)surface_com_vtable(*rm_out)[15])(
            *rm_out, device_guid, dd1, surface1, 0u, device_out) == 0 &&
        *device_out != NULL) ok = 1;
    if (surface1 != NULL) surface_com_release(surface1);
    if (dd1 != NULL) surface_com_release(dd1);
    return ok;
}

static int directdraw_renderer_init_winapi(
    const DirectDrawRendererInitParams* params, const void* guid,
    int width, int height, int bits_per_pixel)
{
    SurfaceDdrawCreateFn create;
    SurfaceQueryInterfaceFn query;
    SurfaceSetCooperativeLevelFn set_cooperative_level;
    SurfaceSetDisplayModeFn set_display_mode;
    SurfaceCreateSurfaceFn create_surface;
    SurfaceGetAttachedSurfaceFn get_attached_surface;
    void* direct_draw = NULL;
    void* device = NULL;
    void* primary = NULL;
    void* back_surface = NULL;
    void* clipper = NULL;
    void* rm = NULL;
    void* rm_device = NULL;
    void* immediate_device = NULL;
    HMODULE rm_module = NULL;
    int mode_set = 0;
    void** vtable;
    SurfaceDdrawDesc2 desc;
    SurfaceDdrawCaps2 back_caps;
    HWND window;
    int success = 0;

    if (params == NULL || width <= 0 || height <= 0 ||
        !surface_ddraw_api_get(&create)) return 0;
    window = params->window != NULL ? (HWND)params->window : GetActiveWindow();
    if (window == NULL) return 0;
    if (create((GUID*)guid, &direct_draw, NULL) != 0 || direct_draw == NULL)
        goto cleanup;
    vtable = surface_com_vtable(direct_draw);
    query = vtable != NULL && vtable[0] != NULL
        ? (SurfaceQueryInterfaceFn)vtable[0] : NULL;
    if (query == NULL || query(direct_draw, s_iid_direct_draw4, &device) != 0 ||
        device == NULL) goto cleanup;
    surface_com_release(direct_draw);
    direct_draw = NULL;

    vtable = surface_com_vtable(device);
    set_cooperative_level = vtable != NULL && vtable[20] != NULL
        ? (SurfaceSetCooperativeLevelFn)vtable[20] : NULL;
    if (set_cooperative_level == NULL ||
        set_cooperative_level(device, window,
            params->fullscreen != 0 ? 0x11u : 8u) != 0) goto cleanup;
    if (params->fullscreen != 0) {
        set_display_mode = vtable != NULL && vtable[21] != NULL
            ? (SurfaceSetDisplayModeFn)vtable[21] : NULL;
        if (set_display_mode == NULL || set_display_mode(device,
                (DWORD)width, (DWORD)height, (DWORD)bits_per_pixel,
                0u, 0u) != 0) goto cleanup;
        mode_set = 1;
    }

    memset(&desc, 0, sizeof(desc));
    desc.size = sizeof(desc);
    if (params->fullscreen != 0) {
        desc.flags = 0x21u;
        desc.backbuffer_count = 1u;
        desc.caps.caps = 0x2218u;
    }
    else {
        desc.flags = 1u;
        desc.caps.caps = 0x2200u;
    }
    create_surface = vtable != NULL && vtable[6] != NULL
        ? (SurfaceCreateSurfaceFn)vtable[6] : NULL;
    if (create_surface == NULL ||
        create_surface(device, &desc, &primary, NULL) != 0 ||
        primary == NULL) goto cleanup;

    if (params->fullscreen != 0) {
        memset(&back_caps, 0, sizeof(back_caps));
        back_caps.caps = 4u;
        vtable = surface_com_vtable(primary);
        get_attached_surface = vtable != NULL && vtable[12] != NULL
            ? (SurfaceGetAttachedSurfaceFn)vtable[12] : NULL;
        if (get_attached_surface == NULL ||
            get_attached_surface(primary, &back_caps, &back_surface) != 0 ||
            back_surface == NULL) goto cleanup;
    }
    else {
        memset(&desc, 0, sizeof(desc));
        desc.size = sizeof(desc);
        desc.flags = 7u;
        desc.height = (DWORD)height;
        desc.width = (DWORD)width;
        desc.caps.caps = 0x2040u;
        vtable = surface_com_vtable(device);
        create_surface = vtable != NULL && vtable[6] != NULL
            ? (SurfaceCreateSurfaceFn)vtable[6] : NULL;
        if (create_surface == NULL ||
            create_surface(device, &desc, &back_surface, NULL) != 0 ||
            back_surface == NULL) goto cleanup;
    }

    if (params->fullscreen == 0 &&
        !winapi_renderer_clipper_attach(device, primary, window,
                                        &clipper)) goto cleanup;

    /* RM3 consumes the selected Direct3D device GUID, independently of the
     * DirectDraw adapter GUID used to create the owning display. */
    {
        typedef HRESULT (WINAPI *CreateRMFn)(void**);
        char path[MAX_PATH];
        DWORD length = GetFullPathNameA("D3DRM.DLL", sizeof(path), path, NULL);
        void* rm1 = NULL;
        CreateRMFn create_rm;
        int ok = 0;
        if (params->device_guid != NULL && length != 0 && length < sizeof(path)) {
            rm_module = LoadLibraryA(path);
            create_rm = rm_module != NULL ? (CreateRMFn)GetProcAddress(
                rm_module, "Direct3DRMCreate") : NULL;
            if (create_rm != NULL && create_rm(&rm1) == 0 && rm1 != NULL &&
                winapi_rm_device_create(rm1, device, back_surface,
                    (const GUID*)params->device_guid, &rm, &rm_device)) ok = 1;
        }
        if (rm1 != NULL) surface_com_release(rm1);
        if (!ok) goto cleanup;
    }

    /* The UserVisual manager needs the real immediate device belonging to
     * this selected retained-mode device, as4781d0 does. Never substitute
     * the DirectDraw device or the RM factory for that interface. */
    if (!scene_winapi_device3_acquire(rm_device, &immediate_device))
        goto cleanup;

    if (s_directdraw_renderer_init_ops.renderer_state_commit != NULL) {
        s_directdraw_renderer_init_ops.renderer_state_commit(
            s_directdraw_renderer_init_ops.context, device, primary,
            back_surface, params->fullscreen);
    }
    winapi_renderer_state_release(&s_winapi_renderer_state);
    s_winapi_renderer_state.device = device;
    s_winapi_renderer_state.primary = primary;
    s_winapi_renderer_state.back_surface = back_surface;
    s_winapi_renderer_state.clipper = clipper;
    s_winapi_renderer_state.rm = rm;
    s_winapi_renderer_state.rm_device = rm_device;
    s_winapi_renderer_state.immediate_device = immediate_device;
    s_winapi_renderer_state.rm_module = rm_module;
    s_winapi_renderer_state.window = window;
    s_winapi_renderer_state.fullscreen = params->fullscreen != 0;
    s_winapi_renderer_state.width = width;
    s_winapi_renderer_state.height = height;
    s_renderer_window_handle = window;
    s_renderer_primary_surface = primary;
    render_surface_secondary_set((uint32_t)back_surface);
    s_renderer_fullscreen = params->fullscreen != 0 ? 1u : 0u;
    s_renderer_surface_width = (uint32_t)width;
    s_renderer_surface_height = (uint32_t)height;
    screen_bounded_surface_device_set(rm, rm_device);
    if ((params->device_flags & 0x4000u) != 0u)
        s_renderer_window_mode_flags |= 4u;
    s_direct3drm_device_mode =
        (s_renderer_window_mode_flags & 0x80000u) == 0u &&
        (params->device_flags & 0x2000u) != 0u ? 2 : 1;
    device = NULL;
    primary = NULL;
    back_surface = NULL;
    clipper = NULL;
    rm = NULL; rm_device = NULL; immediate_device = NULL;
    rm_module = NULL; mode_set = 0;
    if (params->fullscreen != 0) (void)ShowCursor(FALSE);
    success = 1;

cleanup:
    if (immediate_device != NULL) surface_com_release(immediate_device);
    if (rm_device != NULL) surface_com_release(rm_device);
    if (rm != NULL) surface_com_release(rm);
    if (rm_module != NULL) FreeLibrary(rm_module);
    if (clipper != NULL) surface_com_release(clipper);
    if (back_surface != NULL) surface_com_release(back_surface);
    if (primary != NULL) surface_com_release(primary);
    if (device != NULL && mode_set) {
        typedef HRESULT (WINAPI *RestoreFn)(void*);
        void** vt = surface_com_vtable(device);
        if (vt != NULL && vt[19] != NULL) ((RestoreFn)vt[19])(device);
    }
    if (device != NULL) surface_com_release(device);
    if (direct_draw != NULL) surface_com_release(direct_draw);
    return success;
}

int surface_winapi_present_rgba(const void* pixels, int pitch, int width, int height)
{
    WinapiRendererState* state = &s_winapi_renderer_state;
    SurfaceDdrawDesc2 desc;
    SurfaceLockFn lock;
    SurfaceUnlockFn unlock;
    void** vt;
    int copied;
    if (state->back_surface == NULL || state->primary == NULL) return 0;
    vt = surface_com_vtable(state->back_surface);
    if (vt == NULL || vt[25] == NULL || vt[32] == NULL) return 1;
    lock = (SurfaceLockFn)vt[25]; unlock = (SurfaceUnlockFn)vt[32];
    memset(&desc, 0, sizeof(desc)); desc.size = sizeof(desc);
    if (lock(state->back_surface, NULL, &desc, 0x801u, NULL) != 0) return 1;
    copied = surface_rgba_copy_to_rgb(desc.surface, (int)desc.pitch,
        (int)desc.width, (int)desc.height, pixels, pitch, width, height, 0, 0,
        desc.pixel_format.rgb_bits, desc.pixel_format.red_mask,
        desc.pixel_format.green_mask, desc.pixel_format.blue_mask);
    (void)unlock(state->back_surface, NULL);
    if (copied) render_surface_present();
    return 1;
}

static int surface_create_winapi_display_mode(DisplaySurface* out,
    void* hwnd, int width, int height, int bits_per_pixel, int fullscreen)
{
    SurfaceDdrawCreateFn create;
    SurfaceQueryInterfaceFn query;
    SurfaceCreateSurfaceFn create_surface;
    SurfaceCreatePaletteFn create_palette;
    SurfaceSetPaletteFn set_palette;
    SurfaceSetCooperativeLevelFn set_cooperative_level;
    SurfaceLockFn lock;
    SurfaceUnlockFn unlock;
    void* direct_draw = NULL;
    void* direct_draw4 = NULL;
    void* surface4 = NULL;
    void* palette = NULL;
    void** vtable;
    SurfaceDdrawDesc2 desc;
    HRESULT result;
    int effective_bits = bits_per_pixel == 0x0f ? 16 : bits_per_pixel;
    int success = 0;

    if (out == NULL) return -1;
    memset(out, 0, sizeof(*out));
    if (width < 0 || height < 0 || bits_per_pixel < 0)
        return -1;
    if (s_winapi_renderer_state.device != NULL) {
        typedef ULONG (WINAPI *AddRefFn)(void*);
        direct_draw4 = s_winapi_renderer_state.device;
        vtable = surface_com_vtable(direct_draw4);
        if (vtable == NULL || vtable[1] == NULL) return -2;
        ((AddRefFn)vtable[1])(direct_draw4);
    }
    else {
        if (!surface_ddraw_api_get(&create)) return -2;
        result = create(NULL, &direct_draw, NULL);
        if (result != 0 || direct_draw == NULL) goto cleanup;
        vtable = surface_com_vtable(direct_draw);
        query = vtable != NULL ? (SurfaceQueryInterfaceFn)vtable[0] : NULL;
        if (query == NULL || query(direct_draw, s_iid_direct_draw4,
                                   &direct_draw4) != 0 || direct_draw4 == NULL)
            goto cleanup;
        surface_com_release(direct_draw);
        direct_draw = NULL;
        vtable = surface_com_vtable(direct_draw4);
        set_cooperative_level = vtable != NULL && vtable[20] != NULL
            ? (SurfaceSetCooperativeLevelFn)vtable[20] : NULL;
        if (set_cooperative_level == NULL ||
            set_cooperative_level(direct_draw4, (HWND)hwnd, 8u) != 0)
            goto cleanup;
    }

    memset(&desc, 0, sizeof(desc));
    desc.size = sizeof(desc);
    desc.flags = 0x00000001u;
    if (width != 0) {
        desc.flags |= 0x00000004u;
        desc.width = (DWORD)width;
    }
    if (height != 0) {
        desc.flags |= 0x00000002u;
        desc.height = (DWORD)height;
    }
    if (bits_per_pixel != 0) {
        desc.flags |= 0x00001000u;
        desc.pixel_format.size = sizeof(desc.pixel_format);
        desc.pixel_format.rgb_bits = (DWORD)effective_bits;
        if (bits_per_pixel == 32 || bits_per_pixel == 24) {
            desc.pixel_format.flags = 0x40u;
            desc.pixel_format.red_mask = 0x00ff0000u;
            desc.pixel_format.green_mask = 0x0000ff00u;
            desc.pixel_format.blue_mask = 0x000000ffu;
        } else if (bits_per_pixel == 16) {
            desc.pixel_format.flags = 0x40u;
            desc.pixel_format.red_mask = 0x0000f800u;
            desc.pixel_format.green_mask = 0x000007e0u;
            desc.pixel_format.blue_mask = 0x0000001fu;
        } else if (bits_per_pixel == 0x0f) {
            desc.pixel_format.flags = 0x40u;
            desc.pixel_format.red_mask = 0x00007c00u;
            desc.pixel_format.green_mask = 0x000003e0u;
            desc.pixel_format.blue_mask = 0x0000001fu;
        } else if (bits_per_pixel == 8) {
            desc.pixel_format.flags = 0x60u;
            desc.pixel_format.red_mask = 0x00ff0000u;
            desc.pixel_format.green_mask = 0x0000ff00u;
            desc.pixel_format.blue_mask = 0x000000ffu;
        }
    }
    if (width == 0 && height == 0) desc.caps.caps = 0x00000200u;
    else desc.caps.caps = 0x00000840u |
        (fullscreen != 0 ? 0x00003800u : 0u);
    if (effective_bits == 16 &&
        (pop_count32(desc.pixel_format.red_mask) +
         pop_count32(desc.pixel_format.green_mask) +
         pop_count32(desc.pixel_format.blue_mask)) == 15) {
        out->format_flags |= 1u;
    }
    create_surface = vtable != NULL ?
        (SurfaceCreateSurfaceFn)vtable[6] : NULL;
    if (create_surface == NULL || create_surface(direct_draw4, &desc,
            &surface4, NULL) != 0 || surface4 == NULL) goto cleanup;

    vtable = surface_com_vtable(surface4);
    if (vtable != NULL && vtable[25] != NULL && vtable[32] != NULL) {
        memset(&desc, 0, sizeof(desc));
        desc.size = sizeof(desc);
        lock = (SurfaceLockFn)vtable[25];
        unlock = (SurfaceUnlockFn)vtable[32];
        if (lock(surface4, NULL, &desc, 0x00000801u, NULL) == 0) {
            if (width == 0) width = (int)desc.width;
            if (height == 0) height = (int)desc.height;
            (void)unlock(surface4, NULL);
        }
    }

    if (effective_bits == 8 && bits_per_pixel != 0) {
        SurfaceDdrawPaletteEntry entries[256];
        memset(entries, 0, sizeof(entries));
        vtable = surface_com_vtable(direct_draw4);
        create_palette = vtable != NULL && vtable[5] != NULL
            ? (SurfaceCreatePaletteFn)vtable[5] : NULL;
        if (create_palette == NULL || create_palette(direct_draw4, 4u,
                entries, &palette, NULL) != 0 || palette == NULL) goto cleanup;
        vtable = surface_com_vtable(surface4);
        set_palette = vtable != NULL && vtable[31] != NULL
            ? (SurfaceSetPaletteFn)vtable[31] : NULL;
        if (set_palette == NULL || set_palette(surface4, palette) != 0)
            goto cleanup;
    }

    out->dd = direct_draw4;
    out->surf = surface4;
    out->palette = palette;
    out->hwnd = hwnd;
    out->w = width;
    out->h = height;
    out->format_flags |= 0u;
    direct_draw4 = NULL;
    surface4 = NULL;
    palette = NULL;
    success = 1;

cleanup:
    if (palette != NULL) surface_com_release(palette);
    if (surface4 != NULL) surface_com_release(surface4);
    if (direct_draw4 != NULL) surface_com_release(direct_draw4);
    if (direct_draw != NULL) surface_com_release(direct_draw);
    if (success) return 0;
    memset(out, 0, sizeof(*out));
    return -4;
}

typedef struct WinApiStartupAdapter {
    GUID guid;
    int has_guid;
    char label[256];
} WinApiStartupAdapter;

static WinApiStartupAdapter s_winapi_startup_adapters[32];
static unsigned int s_winapi_startup_adapter_count;
static GUID s_winapi_startup_selected_guids[2];
typedef struct WinApiStartupDevice {
    GUID guid;
    DWORD flags;
    char label[256];
} WinApiStartupDevice;
static WinApiStartupDevice s_winapi_startup_selected_devices[2];

typedef struct WinApiStartupDialogState {
    DEVMODEA modes[128];
    WinApiStartupDevice devices[32];
    unsigned int device_count;
    unsigned int selected_device;
    unsigned int selected_adapter;
    int saved_fullscreen;
    unsigned int mode_count;
    unsigned int selected_mode;
    unsigned int raw_mode_count;
    int desktop_bits;
    int fullscreen;
} WinApiStartupDialogState;

/* A dialog edits the inactive slot.  The committed selection may still point
 * into the other slot if this attempt is cancelled or renderer setup fails. */
static DEVMODEA s_winapi_startup_selected_modes[2];
static unsigned int s_winapi_startup_committed_slot;
static unsigned int s_winapi_startup_pending_slot;
static void* s_winapi_startup_owner_window;

typedef HRESULT (WINAPI *SurfaceEnumDisplayModesCallback)(
    SurfaceDdrawDesc2*, void*);
typedef HRESULT (WINAPI *SurfaceEnumDisplayModesFn)(void*, DWORD,
    SurfaceDdrawDesc2*, void*, SurfaceEnumDisplayModesCallback);

static HRESULT WINAPI surface_winapi_startup_mode_collect(
    SurfaceDdrawDesc2* descriptor, void* context)
{
    WinApiStartupDialogState* state = (WinApiStartupDialogState*)context;
    DEVMODEA* mode;
    unsigned int i;
    int accepted = display_resolution_filter_tuple_count() == 0;
    if (descriptor == NULL || state == NULL || state->mode_count >= 128u)
        return 1;
    /* Retail keeps the first enumerated mode in either policy. In windowed
     * mode subsequent records must match the desktop depth, before the
     * dialog's resolution tuple filter is applied. */
    if (state->raw_mode_count++ != 0u && !state->fullscreen &&
        descriptor->pixel_format.rgb_bits != (DWORD)state->desktop_bits)
        return 1;
    for (i = 0u; i < (unsigned int)display_resolution_filter_tuple_count();
         ++i) {
        int width;
        int height;
        int bits;
        display_resolution_filter_tuple_get((int)i, &width, &height, &bits);
        if ((width == 0 || width == (int)descriptor->width) &&
            (height == 0 || height == (int)descriptor->height) &&
            (bits == 0 || bits == (int)descriptor->pixel_format.rgb_bits)) {
            accepted = 1;
            break;
        }
    }
    if (!accepted || descriptor->width == 0 || descriptor->height == 0 ||
        descriptor->pixel_format.rgb_bits < 16u) return 1;
    mode = &state->modes[state->mode_count];
    memset(mode, 0, sizeof(*mode));
    mode->dmSize = sizeof(*mode);
    mode->dmPelsWidth = descriptor->width;
    mode->dmPelsHeight = descriptor->height;
    mode->dmBitsPerPel = descriptor->pixel_format.rgb_bits;
    mode->dmDisplayFrequency = descriptor->refresh_rate;
    ++state->mode_count;
    return 1;
}

static int surface_winapi_startup_modes_enumerate(
    WinApiStartupDialogState* state)
{
    SurfaceDdrawCreateFn create;
    SurfaceQueryInterfaceFn query;
    SurfaceEnumDisplayModesFn enumerate;
    void* direct_draw = NULL;
    void* direct_draw4 = NULL;
    void** vtable;
    HRESULT result;
    int success = 0;
    if (state == NULL || !surface_ddraw_api_get(&create)) return 0;
    result = create(s_winapi_startup_adapters[state->selected_adapter].has_guid
        ? &s_winapi_startup_adapters[state->selected_adapter].guid : NULL,
        &direct_draw, NULL);
    if (result != 0 || direct_draw == NULL) goto cleanup;
    vtable = surface_com_vtable(direct_draw);
    query = vtable != NULL ? (SurfaceQueryInterfaceFn)vtable[0] : NULL;
    if (query == NULL || query(direct_draw, s_iid_direct_draw4,
            &direct_draw4) != 0 || direct_draw4 == NULL) goto cleanup;
    surface_com_release(direct_draw);
    direct_draw = NULL;
    vtable = surface_com_vtable(direct_draw4);
    enumerate = vtable != NULL && vtable[8] != NULL
        ? (SurfaceEnumDisplayModesFn)vtable[8] : NULL;
    if (enumerate != NULL && enumerate(direct_draw4, 0u, NULL, state,
            surface_winapi_startup_mode_collect) == 0 &&
        state->mode_count != 0u) success = 1;
cleanup:
    if (direct_draw4 != NULL) surface_com_release(direct_draw4);
    if (direct_draw != NULL) surface_com_release(direct_draw);
    return success;
}

static const GUID s_iid_direct3d_startup = {
    0x3bba0080, 0x2421, 0x11cf,
    { 0xa3, 0x1a, 0x00, 0xaa, 0x00, 0xb9, 0x33, 0x56 }
};
typedef HRESULT (WINAPI *SurfaceEnumDeviceCallback)(GUID*, LPSTR, LPSTR,
                                                     void*, void*, void*);
typedef HRESULT (WINAPI *SurfaceEnumDevicesFn)(void*,
    SurfaceEnumDeviceCallback, void*);

static HRESULT WINAPI surface_winapi_startup_device_collect(GUID* guid,
    LPSTR description, LPSTR name, void* hardware, void* software, void* context)
{
    WinApiStartupDialogState* state = (WinApiStartupDialogState*)context;
    WinApiStartupDevice* device;
    DWORD* descriptor = (DWORD*)hardware;
    DWORD flags = 1u;
    DWORD caps;
    if (guid == NULL || descriptor == NULL || state->device_count >= 32u)
        return 1;
    if (descriptor[2] != 0u) flags |= 0x2000u;
    else descriptor = (DWORD*)software;
    if (descriptor == NULL) return 1;
    if ((descriptor[1] & 1u) != 0u && descriptor[2] == 2u) flags |= 0x1000u;
    if ((descriptor[1] & 0x80u) != 0u) {
        caps = descriptor[0x9c / 4];
        if ((caps & 0x800u) != 0u) flags |= 0x10u;
        if ((caps & 0x400u) != 0u) flags |= 0x20u;
        if ((caps & 0x200u) != 0u) flags |= 0x40u;
        if ((caps & 0x100u) != 0u) flags |= 0x80u;
    }
    if ((descriptor[1] & 2u) != 0u) {
        if ((descriptor[3] & 0x200u) != 0u) flags |= 0x4000u;
        else if ((descriptor[3] & 0x100u) != 0u) flags |= 0x8000u;
    }
    device = &state->devices[state->device_count];
    memset(device, 0, sizeof(*device));
    device->guid = *guid;
    device->flags = flags;
    snprintf(device->label, sizeof(device->label), "%s (%s)",
             name != NULL ? name : "", description != NULL ? description : "");
    device->label[sizeof(device->label) - 1u] = 0;
    if ((flags & 0x2000u) != 0u) state->selected_device = state->device_count;
    ++state->device_count;
    return 1;
}

static void surface_winapi_startup_devices_refresh(HWND dialog,
    WinApiStartupDialogState* state)
{
    SurfaceDdrawCreateFn create;
    SurfaceQueryInterfaceFn query;
    SurfaceEnumDevicesFn enumerate;
    void* direct_draw = NULL;
    void* direct3d = NULL;
    void** vtable;
    unsigned int i;
    HWND combo = GetDlgItem(dialog, 1002);
    WinApiStartupAdapter* adapter =
        &s_winapi_startup_adapters[state->selected_adapter];
    SendMessageA(combo, LB_RESETCONTENT, 0, 0);
    state->device_count = 0;
    state->selected_device = 0;
    if (!surface_ddraw_api_get(&create) ||
        create(adapter->has_guid ? &adapter->guid : NULL,
               &direct_draw, NULL) != 0 || direct_draw == NULL) goto cleanup;
    vtable = surface_com_vtable(direct_draw);
    query = vtable != NULL ? (SurfaceQueryInterfaceFn)vtable[0] : NULL;
    if (query == NULL || query(direct_draw, s_iid_direct3d_startup,
                              &direct3d) != 0 || direct3d == NULL) goto cleanup;
    vtable = surface_com_vtable(direct3d);
    /* IDirect3D has Initialize at slot 3 and EnumDevices at slot 4. */
    enumerate = vtable != NULL ? (SurfaceEnumDevicesFn)vtable[4] : NULL;
    if (enumerate == NULL || enumerate(direct3d,
            surface_winapi_startup_device_collect, state) != 0) {
        state->device_count = 0;
        goto cleanup;
    }
    for (i = 0; i < state->device_count; ++i)
        SendMessageA(combo, LB_ADDSTRING, 0, (LPARAM)state->devices[i].label);
    if (state->device_count != 0u)
        SendMessageA(combo, LB_SETCURSEL, state->selected_device, 0);
cleanup:
    if (direct3d != NULL) surface_com_release(direct3d);
    if (direct_draw != NULL) surface_com_release(direct_draw);
}

static void surface_winapi_startup_modes_refresh(HWND dialog,
    WinApiStartupDialogState* state)
{
    unsigned int i;
    char label[80];
    HDC dc;
    HWND combo = GetDlgItem(dialog, 1003);
    SendMessageA(combo, LB_RESETCONTENT, 0, 0);
    state->mode_count = 0;
    state->selected_mode = 0;
    state->raw_mode_count = 0;
    dc = GetDC((HWND)s_winapi_startup_owner_window);
    state->desktop_bits = dc != NULL ? GetDeviceCaps(dc, BITSPIXEL) : 0;
    if (dc != NULL) ReleaseDC((HWND)s_winapi_startup_owner_window, dc);
    surface_winapi_startup_modes_enumerate(state);
    SetDlgItemTextA(dialog, 1006, state->fullscreen ? "Screen Mode" : "Window Size");
    EnableWindow(GetDlgItem(dialog, 1), state->mode_count != 0u);
    if (state->mode_count == 0u) {
        sprintf(label, "No supported %d bit modes found", state->desktop_bits);
        SendMessageA(combo, LB_ADDSTRING, 0, (LPARAM)label);
        SendMessageA(combo, LB_SETCURSEL, 0, 0);
        return;
    }
    for (i = 0; i < state->mode_count; ++i) {
        sprintf(label, "%ix%i (%i bit)",
            (int)state->modes[i].dmPelsWidth,
            (int)state->modes[i].dmPelsHeight,
            (int)state->modes[i].dmBitsPerPel);
        SendMessageA(combo, LB_ADDSTRING, 0, (LPARAM)label);
    }
    SendMessageA(combo, LB_SETCURSEL, state->selected_mode, 0);
}

static void surface_winapi_startup_radio_sync(HWND dialog,
    WinApiStartupDialogState* state)
{
    if (s_winapi_startup_adapters[state->selected_adapter].has_guid) {
        state->saved_fullscreen = state->fullscreen;
        state->fullscreen = 1;
        EnableWindow(GetDlgItem(dialog, 1005), FALSE);
    }
    else {
        EnableWindow(GetDlgItem(dialog, 1005), TRUE);
        if (state->saved_fullscreen == 0) state->fullscreen = 0;
    }
    CheckRadioButton(dialog, 1004, 1005, state->fullscreen ? 1004 : 1005);
}

static BOOL CALLBACK surface_winapi_mode_dialog_proc(HWND dialog,
    UINT message, WPARAM wparam, LPARAM lparam)
{
    WinApiStartupDialogState* state = (WinApiStartupDialogState*)
        GetWindowLongA(dialog, GWL_USERDATA);
    if (message == WM_INITDIALOG) {
        unsigned int i;
        state = (WinApiStartupDialogState*)lparam;
        state->fullscreen = 1;
        state->saved_fullscreen = 1;
        SetWindowLongA(dialog, GWL_USERDATA, (LONG)state);
        for (i = 0; i < s_winapi_startup_adapter_count; ++i)
            SendMessageA(GetDlgItem(dialog, 1000), LB_ADDSTRING, 0,
                (LPARAM)s_winapi_startup_adapters[i].label);
        SendMessageA(GetDlgItem(dialog, 1000), LB_SETCURSEL, 0, 0);
        surface_winapi_startup_radio_sync(dialog, state);
        surface_winapi_startup_modes_refresh(dialog, state);
        surface_winapi_startup_devices_refresh(dialog, state);
        return TRUE;
    }
    if (message == WM_COMMAND && state != NULL) {
        unsigned int command = (unsigned int)(wparam & 0xffffu);
        if (command == 1000u && HIWORD(wparam) == LBN_SELCHANGE) {
            LRESULT selected = SendMessageA(GetDlgItem(dialog, 1000),
                                            LB_GETCURSEL, 0, 0);
            if (selected >= 0 && (unsigned int)selected <
                    s_winapi_startup_adapter_count) {
                state->selected_adapter = (unsigned int)selected;
                surface_winapi_startup_radio_sync(dialog, state);
                surface_winapi_startup_modes_refresh(dialog, state);
                surface_winapi_startup_devices_refresh(dialog, state);
            }
            return TRUE;
        }
        if (command == 1002u && HIWORD(wparam) == LBN_SELCHANGE) {
            LRESULT selected = SendMessageA(GetDlgItem(dialog, 1002),
                                            LB_GETCURSEL, 0, 0);
            if (selected >= 0 && (unsigned int)selected < state->device_count)
                state->selected_device = (unsigned int)selected;
            return TRUE;
        }
        if (command == 1004u || command == 1005u) {
            state->fullscreen = command == 1004u;
            surface_winapi_startup_modes_refresh(dialog, state);
            return TRUE;
        }
        if (command == 1u) {
            LRESULT selected = SendMessageA(GetDlgItem(dialog, 1003),
                                            LB_GETCURSEL, 0, 0);
            if (selected < 0 || (unsigned int)selected >= state->mode_count ||
                state->device_count == 0u) return TRUE;
            state->selected_mode = (unsigned int)selected;
            state->fullscreen = IsDlgButtonChecked(dialog, 1004) != 0;
            s_winapi_startup_selected_modes[s_winapi_startup_pending_slot] =
                state->modes[state->selected_mode];
            EndDialog(dialog, 1);
            return TRUE;
        }
        if (command == 2u) {
            EndDialog(dialog, 2);
            return TRUE;
        }
    }
    return FALSE;
}

typedef BOOL (WINAPI *SurfaceEnumAdapterCallback)(GUID*, LPSTR, LPSTR, void*);
typedef HRESULT (WINAPI *SurfaceEnumAdaptersFn)(SurfaceEnumAdapterCallback,
                                               void*);

static BOOL WINAPI surface_winapi_startup_adapter_collect(GUID* guid,
    LPSTR description, LPSTR name, void* context)
{
    WinApiStartupAdapter* adapter;
    (void)context;
    if (s_winapi_startup_adapter_count >= 32u) return TRUE;
    adapter = &s_winapi_startup_adapters[s_winapi_startup_adapter_count++];
    memset(adapter, 0, sizeof(*adapter));
    if (guid != NULL) {
        adapter->guid = *guid;
        adapter->has_guid = 1;
    }
    sprintf(adapter->label, "%.120s (%.120s)",
        description != NULL ? description : "", name != NULL ? name : "");
    return TRUE;
}

static int surface_winapi_startup_adapter_count_get(void* context)
{
    SurfaceDdrawCreateFn create;
    SurfaceEnumAdaptersFn enumerate;
    (void)context;
    s_winapi_startup_adapter_count = 0;
    if (!surface_ddraw_api_get(&create)) return 0;
    enumerate = (SurfaceEnumAdaptersFn)GetProcAddress(s_surface_ddraw_module,
                                                       "DirectDrawEnumerateA");
    if (enumerate == NULL || enumerate(surface_winapi_startup_adapter_collect,
                                       NULL) != 0) return 0;
    return (int)s_winapi_startup_adapter_count;
}

static int surface_winapi_startup_adapter_get(void* context, int index,
    DirectDrawAdapterCandidate* adapter_out)
{
    WinApiStartupAdapter* adapter;
    (void)context;
    if (index < 0 || (unsigned int)index >= s_winapi_startup_adapter_count ||
        adapter_out == NULL) return 0;
    adapter = &s_winapi_startup_adapters[index];
    adapter_out->adapter = adapter->has_guid ? &adapter->guid : NULL;
    adapter_out->flags = adapter->has_guid ? 1u : 0x31u;
    return 1;
}

static int surface_winapi_startup_selection_dialog(void* context,
    int error_mode, DirectDrawAdapterCandidate* adapter_out,
    DirectDrawDisplayModeCandidate* mode_out, int* fullscreen_out)
{
    WinApiStartupDialogState state;
    int result;
    (void)error_mode;
    if (adapter_out == NULL || mode_out == NULL || fullscreen_out == NULL)
        return 0;
    memset(&state, 0, sizeof(state));
    s_winapi_startup_pending_slot = 1u - s_winapi_startup_committed_slot;
    result = (int)DialogBoxParamA(GetModuleHandleA(NULL),
        MAKEINTRESOURCEA(101), (HWND)context,
        (DLGPROC)surface_winapi_mode_dialog_proc, (LPARAM)&state);
    if (result == 2) return -1;
    if (result != 1) return 0;
    s_winapi_startup_selected_devices[s_winapi_startup_pending_slot] =
        state.devices[state.selected_device];
    if (s_winapi_startup_adapters[state.selected_adapter].has_guid) {
        s_winapi_startup_selected_guids[s_winapi_startup_pending_slot] =
            s_winapi_startup_adapters[state.selected_adapter].guid;
        adapter_out->adapter = &s_winapi_startup_selected_guids[
            s_winapi_startup_pending_slot];
        adapter_out->flags = 1u;
    }
    else {
        adapter_out->adapter = NULL;
        adapter_out->flags = 0x31u;
    }
    mode_out->descriptor = &s_winapi_startup_selected_modes[
        s_winapi_startup_pending_slot];
    mode_out->width = (int)s_winapi_startup_selected_modes[
        s_winapi_startup_pending_slot].dmPelsWidth;
    mode_out->height = (int)s_winapi_startup_selected_modes[
        s_winapi_startup_pending_slot].dmPelsHeight;
    mode_out->bits_per_pixel = (int)s_winapi_startup_selected_modes[
        s_winapi_startup_pending_slot].dmBitsPerPel;
    mode_out->flags = s_winapi_startup_selected_modes[
        s_winapi_startup_pending_slot].dmDisplayFlags;
    *fullscreen_out = state.fullscreen;
    return 1;
}

static int surface_winapi_startup_renderer_initialize(void* context,
    int fullscreen, void* adapter,
    const DirectDrawDisplayModeCandidate* mode)
{
    DirectDrawRendererInitParams params;
    HWND window = (HWND)context;
    if (mode == NULL || mode->width <= 0 || mode->height <= 0) return 0;
    memset(&params, 0, sizeof(params));
    params.window = window;
    params.fullscreen = fullscreen;
    params.adapter_guid = adapter;
    params.adapter_flags = adapter != NULL ? 1u : 0x31u;
    params.device_guid = &s_winapi_startup_selected_devices[
        s_winapi_startup_pending_slot].guid;
    params.device_flags = s_winapi_startup_selected_devices[
        s_winapi_startup_pending_slot].flags;
    params.mode_width = mode->width;
    params.mode_height = mode->height;
    params.mode_bits_per_pixel = mode->bits_per_pixel;
    params.width = mode->width;
    params.height = mode->height;
    params.window_x = 100;
    params.window_y = 100;
    if (!directdraw_renderer_init(&params)) return 0;
    ShowWindow(window, 5);
    s_winapi_startup_committed_slot = s_winapi_startup_pending_slot;
    return 1;
}
#endif

void surface_winapi_startup_ops_install(void* owner_window)
{
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
    DirectDrawDeviceInitializeOps ops;
    memset(&ops, 0, sizeof(ops));
    ops.adapter_count_get = surface_winapi_startup_adapter_count_get;
    ops.adapter_get = surface_winapi_startup_adapter_get;
    ops.selection_dialog = surface_winapi_startup_selection_dialog;
    ops.renderer_initialize = surface_winapi_startup_renderer_initialize;
    s_winapi_startup_owner_window = owner_window;
    ops.context = s_winapi_startup_owner_window;
    directdraw_device_initialize_ops_set(&ops);
#else
    (void)owner_window;
#endif
}

int surface_create(DisplaySurface* out, void* hwnd, int w, int h)
{
#if defined(_WIN32) && !defined(_WIN64)
    int result;
    if (w <= 0 || h <= 0) {
        if (out != NULL) memset(out, 0, sizeof(*out));
        return -1;
    }
    result = surface_create_winapi_display_mode(out, hwnd, w, h, 32, 0);
    if (result == 0 && s_winapi_renderer_state.device != NULL)
        s_winapi_renderer_state.staging_surface = out->surf;
    else if (result != 0 && s_winapi_renderer_state.device != NULL)
        winapi_renderer_state_release(&s_winapi_renderer_state);
    return result;
#else
    if (out == NULL) return -1;
    memset(out, 0, sizeof(*out));
    (void)hwnd; (void)w; (void)h;
    return -5;
#endif
}

void surface_destroy(DisplaySurface* s)
{
    if (s == NULL) return;
#if defined(_WIN32) && !defined(_WIN64)
    int release_renderer = s->surf != NULL &&
        s->surf == s_winapi_renderer_state.staging_surface;
    if (s->surf != NULL) surface_com_release(s->surf);
    if (s->palette != NULL) surface_com_release(s->palette);
    if (s->dd != NULL) surface_com_release(s->dd);
    if (release_renderer && s_winapi_renderer_state.device != NULL)
        winapi_renderer_state_release(&s_winapi_renderer_state);
#endif
    s->dd = NULL;
    s->surf = NULL;
    s->hwnd = NULL;
    s->w = 0;
    s->h = 0;
    s->palette = NULL;
    s->format_flags = 0u;
}

int surface_width(DisplaySurface* s)  { return s != NULL ? s->w : 0; }
int surface_height(DisplaySurface* s) { return s != NULL ? s->h : 0; }

int surface_palette_entries_set(DisplaySurface* surface, unsigned int start,
                                unsigned int count,
                                const SurfacePaletteEntry* entries)
{
#if defined(_WIN32) && !defined(_WIN64)
    SurfaceDdrawPaletteEntry native_entries[256];
    SurfacePaletteSetEntriesFn set_entries;
    void** vtable;
    unsigned int index;
    if (surface == NULL || surface->palette == NULL || entries == NULL ||
        count == 0u || start >= 256u || count > 256u - start) return 0;
    for (index = 0u; index < count; ++index) {
        native_entries[index].red = entries[index].red;
        native_entries[index].green = entries[index].green;
        native_entries[index].blue = entries[index].blue;
        native_entries[index].flags = entries[index].flags;
    }
    vtable = surface_com_vtable(surface->palette);
    set_entries = vtable != NULL && vtable[6] != NULL
        ? (SurfacePaletteSetEntriesFn)vtable[6] : NULL;
    if (set_entries == NULL) return 0;
    return set_entries(surface->palette, 0u, start, count, native_entries)
        == 0;
#else
    (void)surface; (void)start; (void)count; (void)entries;
    return 0;
#endif
}

int surface_lock(DisplaySurface* s, void** pixels, int* pitch)
{
    if (pixels != NULL) *pixels = NULL;
    if (pitch != NULL) *pitch = 0;
    if (s == NULL || s->surf == NULL || pixels == NULL || pitch == NULL)
        return -1;
#if defined(_WIN32) && !defined(_WIN64)
    {
        SurfaceDdrawDesc2 desc;
        SurfaceLockFn lock;
        void** vtable = surface_com_vtable(s->surf);
        HRESULT result;
        if (vtable == NULL || vtable[25] == NULL) return -2;
        memset(&desc, 0, sizeof(desc));
        desc.size = sizeof(desc);
        lock = (SurfaceLockFn)vtable[25];
        result = lock(s->surf, NULL, &desc, 0x00000801u, NULL);
        if (result != 0) return -2;
        *pixels = desc.surface;
        *pitch = desc.pitch;
        return 0;
    }
#else
    return -5;
#endif
}

void surface_unlock(DisplaySurface* s, void* pixels)
{
    (void)pixels;
    if (s == NULL || s->surf == NULL) return;
#if defined(_WIN32) && !defined(_WIN64)
    {
        void** vtable = surface_com_vtable(s->surf);
        SurfaceUnlockFn unlock = vtable != NULL && vtable[32] != NULL
            ? (SurfaceUnlockFn)vtable[32] : NULL;
        if (unlock != NULL) (void)unlock(s->surf, NULL);
    }
#endif
}

static void surface_restore_from_primary_impl(DisplaySurface* destination,
                                              DisplaySurface* primary,
                                              int enabled)
{
#if defined(_WIN32) && !defined(_WIN64)
    if (enabled != 0 && destination != NULL && destination->surf != NULL &&
        primary != NULL && primary->surf != NULL) {
        void** vtable = surface_com_vtable(destination->surf);
        SurfaceBltFn blt = vtable != NULL && vtable[5] != NULL
            ? (SurfaceBltFn)vtable[5] : NULL;
        if (blt != NULL)
            (void)blt(destination->surf, NULL, primary->surf, NULL,
                      0x01000000u, NULL);
    }
#else
    (void)destination; (void)primary; (void)enabled;
#endif
}

static void windowed_surface_blit_impl(DisplaySurface* primary,
                                       DisplaySurface* back_surface)
{
#if defined(_WIN32) && !defined(_WIN64)
    RECT destination_rect;
    RECT source_rect;
    POINT origin;
    void** vtable;
    SurfaceBltFn blt;
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
    vtable = surface_com_vtable(primary->surf);
    blt = vtable != NULL && vtable[5] != NULL
        ? (SurfaceBltFn)vtable[5] : NULL;
    if (blt != NULL)
        (void)blt(primary->surf, &destination_rect, back_surface->surf,
                  &source_rect, 0x01000000u, NULL);
#else
    (void)primary; (void)back_surface;
#endif
}

static void surface_blit_wait_impl(void* destination,
                                   DisplaySurface* source)
{
#if defined(_WIN32) && !defined(_WIN64)
    void** vtable;
    SurfaceBltFn blt;
    if (destination == NULL || source == NULL || source->surf == NULL) return;
    vtable = surface_com_vtable(destination);
    blt = vtable != NULL && vtable[5] != NULL
        ? (SurfaceBltFn)vtable[5] : NULL;
    if (blt != NULL)
        (void)blt(destination, NULL, source->surf, NULL, 0x01000000u, NULL);
#else
    (void)destination; (void)source;
#endif
}

static int surface_directdraw_pixel_format_query(DisplaySurface* s,
                                                 SurfacePixelFormat* out)
{
    if (out == NULL) return 0;
    memset(out, 0, sizeof(*out));
#if defined(_WIN32) && !defined(_WIN64)
    if (s != NULL && s->surf != NULL) {
        SurfaceDdrawPixelFormat format;
        void** vtable = surface_com_vtable(s->surf);
        SurfaceGetPixelFormatFn get_format = vtable != NULL && vtable[21] != NULL
            ? (SurfaceGetPixelFormatFn)vtable[21] : NULL;
        if (get_format == NULL) return 0;
        memset(&format, 0, sizeof(format));
        format.size = sizeof(format);
        if (get_format(s->surf, &format) != 0) return 0;
        out->bits_per_pixel = format.rgb_bits;
        out->red_mask = format.red_mask;
        out->green_mask = format.green_mask;
        out->blue_mask = format.blue_mask;
        return 1;
    }
#else
    (void)s;
#endif
    return 0;
}

static int surface_directdraw_description_query(DisplaySurface* s,
                                                SurfaceDescriptionInfo* out)
{
    if (out == NULL) return 0;
    memset(out, 0, sizeof(*out));
#if defined(_WIN32) && !defined(_WIN64)
    if (s != NULL && s->surf != NULL) {
        SurfaceDdrawDesc2 desc;
        SurfaceGetDescriptionFn get_description;
        void** vtable = surface_com_vtable(s->surf);
        if (vtable == NULL || vtable[22] == NULL) return 0;
        memset(&desc, 0, sizeof(desc));
        desc.size = sizeof(desc);
        get_description = (SurfaceGetDescriptionFn)vtable[22];
        if (get_description(s->surf, &desc) != 0) return 0;
        out->width = (int)desc.width;
        out->height = (int)desc.height;
        out->pitch = desc.pitch;
        out->pixels = desc.surface;
        out->bits_per_pixel = desc.pixel_format.rgb_bits;
        return 1;
    }
#else
    (void)s;
#endif
    return 0;
}

#else  /* Other non-miniwin platform: DirectDraw is unavailable. */

int surface_create(DisplaySurface* out, void* hwnd, int w, int h)
{
    (void)hwnd;
    if (out == NULL) return -1;
    out->dd = NULL;
    out->surf = NULL;
    out->hwnd = NULL;
    out->w = 0;
    out->h = 0;
    out->palette = NULL;
    out->format_flags = 0u;
    if (w <= 0 || h <= 0) return -1;
    return -5;
}
void surface_destroy(DisplaySurface* s)
{
    if (s == NULL) return;
    s->dd = NULL;
    s->surf = NULL;
    s->hwnd = NULL;
    s->w = 0;
    s->h = 0;
    s->palette = NULL;
    s->format_flags = 0u;
}
int surface_width(DisplaySurface* s)  { return s != NULL ? s->w : 0; }
int surface_height(DisplaySurface* s) { return s != NULL ? s->h : 0; }
int surface_palette_entries_set(DisplaySurface* surface, unsigned int start,
                                unsigned int count,
                                const SurfacePaletteEntry* entries)
{
    (void)surface; (void)start; (void)count; (void)entries;
    return 0;
}
int surface_lock(DisplaySurface* s, void** pixels, int* pitch)
{
    (void)s;
    if (pixels != NULL) *pixels = NULL;
    if (pitch != NULL) *pitch = 0;
    if (pixels == NULL || pitch == NULL) return -1;
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
static void surface_blit_wait_impl(void* destination,
                                   DisplaySurface* source)
{
    (void)destination; (void)source;
}
static int surface_directdraw_description_query(DisplaySurface* surface,
                                                SurfaceDescriptionInfo* out)
{
    (void)surface;
    if (out != NULL) {
        out->width = 0;
        out->height = 0;
        out->pitch = 0;
        out->pixels = NULL;
        out->bits_per_pixel = 0;
    }
    return 0;
}
static int surface_directdraw_pixel_format_query(DisplaySurface* surface,
                                                 SurfacePixelFormat* out)
{
    (void)surface;
    if (out != NULL) {
        out->bits_per_pixel = 0;
        out->red_mask = 0;
        out->green_mask = 0;
        out->blue_mask = 0;
        out->palette_rgb = NULL;
    }
    return 0;
}

#endif /* LEGORRR_BACKEND_MINIWIN */

int surface_create_display_mode(DisplaySurface* out, void* hwnd, int w,
                                int h, int bits_per_pixel, int fullscreen)
{
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
    return surface_create_winapi_display_mode(out, hwnd, w, h,
                                               bits_per_pixel, fullscreen);
#else
    (void)bits_per_pixel;
    (void)fullscreen;
    return surface_create(out, hwnd, w, h);
#endif
}

/* ---- backend-neutral pixel helpers ------------------------------------- */

/* RenderSurfaceRestoreFromPrimary (FUN_0047cc20): restore the current
 * windowed back surface by copying the primary surface when enabled. The
 * retail entrypoint is no-argument and reads renderer globals; this portable
 * adapter takes explicit DisplaySurface owners and is not ABI-compatible. */
void render_surface_restore_from_primary(DisplaySurface* destination,
                                         DisplaySurface* primary,
                                         int restore_enabled)
{
    surface_restore_from_primary_impl(destination, primary, restore_enabled);
}

/* WindowedRenderSurfaceBlit (FUN_0047cc50): place the back surface at the
 * window's client origin on the primary surface and wait for the blit. The
 * retail entrypoint is no-argument and reads renderer globals, unlike this
 * portable adapter. */
void windowed_render_surface_blit(DisplaySurface* primary,
                                  DisplaySurface* back_surface)
{
    windowed_surface_blit_impl(primary, back_surface);
}

/* AviPlaybackFrameLoop (FUN_0047ed00): copy the movie surface into the
 * shared render target with a full-surface, wait-for-completion blit. */
void surface_blit_wait(void* destination, DisplaySurface* source)
{
    surface_blit_wait_impl(destination, source);
}

/* Portable surface descriptor query. The retail helper consumes a raw
 * DirectDraw interface slot and returns selected values through separate
 * ABI outputs; this DisplaySurface record is not that interface contract. */
int surface_directdraw_description_get(DisplaySurface* surface,
                                       SurfaceDescriptionInfo* out)
{
    return surface_directdraw_description_query(surface, out);
}

/* Portable pixel-format query. The retail helper consumes a raw DirectDraw
 * interface slot and returns packed format data, not this output record. */
int surface_directdraw_pixel_format_get(DisplaySurface* surface,
                                        SurfacePixelFormat* out)
{
    return surface_directdraw_pixel_format_query(surface, out);
}

void surface_description_interface_set(void* interface_pointer)
{
#if defined(_WIN32)
    s_renderer_global_dword = (uint32_t)interface_pointer;
#else
    s_surface_description_interface = interface_pointer;
#endif
}

/* SurfaceDescriptionGet (FUN_0047cdd0): pass a 0x1000 description request
 * through slot 0x5c of the renderer's DirectDraw wrapper. */
// FUNCTION: LEGORR 0x0047cdd0
int surface_description_get(int* width_out, int* height_out)
{
    typedef long (SURFACE_COM_CALL *SurfaceDescriptionProc)(
        void*, uint32_t*, int*, int*);
    uint32_t description[4];
    void* interface_pointer;
    void** vtable;
    SurfaceDescriptionProc get_description;
    description[0] = 0x1000u;
    description[1] = 0u;
    description[2] = 0u;
    description[3] = 0u;
    *width_out = 0;
    *height_out = 0;
#if defined(_WIN32)
    interface_pointer = (void*)s_renderer_global_dword;
#else
    interface_pointer = s_surface_description_interface;
    if (interface_pointer == NULL) return 0;
#endif
    vtable = *(void***)interface_pointer;
    get_description = (SurfaceDescriptionProc)vtable[0x5c / 4];
    return get_description(interface_pointer, description,
                           width_out, height_out) == 0;
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
            if (t->source_key_set) {
                uint32_t rgb = p & 0x00ffffffu;
                uint32_t key_low = t->source_key_low & 0x00ffffffu;
                uint32_t key_high = t->source_key_high & 0x00ffffffu;
                uint32_t key_min = key_low < key_high ? key_low : key_high;
                uint32_t key_max = key_low < key_high ? key_high : key_low;
                if (rgb >= key_min && rgb <= key_max) continue;
            }
            row[dx * 4 + 0] = (unsigned char)(p >> 16); /* R */
            row[dx * 4 + 1] = (unsigned char)(p >> 8);  /* G */
            row[dx * 4 + 2] = (unsigned char)(p);       /* B */
            row[dx * 4 + 3] = (unsigned char)(p >> 24); /* A */
        }
    }
}

static uint32_t surface_image_key_color(float red, float green, float blue)
{
    uint8_t r = (uint8_t)crt_float_to_long(red);
    uint8_t g = (uint8_t)crt_float_to_long(green);
    uint8_t b = (uint8_t)crt_float_to_long(blue);
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

/* ImageSurfaceBlitTransformed (FUN_0047dbf0) converts the supplied channel
 * triples through the resource's pixel representation, installs the source
 * color-key range, and marks wrapper flag bit 1. The portable resource is a
 * decoded ARGB TextureSurface, so its equivalent key is stored as RGB24. */
// FUNCTION: LEGORR 0x0047dbf0
void image_surface_blit_transformed(TextureSurface* surface,
                                    float low_red, float low_green,
                                    float low_blue, float high_red,
                                    float high_green, float high_blue)
{
    uint32_t low;
    uint32_t high;
    if (surface == NULL) return;
    low = surface_image_key_color(low_red, low_green, low_blue);
    high = surface_image_key_color(high_red, high_green, high_blue);
    surface->source_key_low = low;
    surface->source_key_high = high;
    surface->source_key_set = 1;
    surface->resource_flags |= 2u;
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

/* SurfacePixelRowsFlipWithScratch (FUN_0047D740): reverse row pairs using
 * the original dword loop and its trailing-byte loop. */
// FUNCTION: LEGORR 0x0047d740
void surface_pixel_rows_flip_with_scratch(SurfaceRecord* rec)
{
    uint32_t half_bytes = (uint32_t)(rec->row_bytes / 2);
    uint32_t span = half_bytes * 2u;
    uint32_t* top = (uint32_t*)rec->pixels;
    uint32_t* bottom = (uint32_t*)((char*)top +
        ((uint32_t)rec->row_count - 1u) * span);
    uint32_t* scratch = (uint32_t*)engine_heap_alloc((long)span);
    uint32_t lap = 0u;

    if (((uint32_t)rec->row_count & 0xfffffffeu) != 0u) {
        do {
            uint32_t* source = top;
            uint32_t* destination = scratch;
            uint32_t count;
            for (count = (half_bytes & 0x7fffffffu) >> 1;
                 count != 0u; --count) {
                *destination++ = *source++;
            }
            for (count = span & 3u; count != 0u; --count) {
                *(uint8_t*)destination = *(uint8_t*)source;
                destination = (uint32_t*)((char*)destination + 1);
                source = (uint32_t*)((char*)source + 1);
            }

            source = bottom;
            destination = top;
            for (count = (half_bytes & 0x7fffffffu) >> 1;
                 count != 0u; --count) {
                *destination++ = *source++;
            }
            for (count = span & 3u; count != 0u; --count) {
                *(uint8_t*)destination = *(uint8_t*)source;
                destination = (uint32_t*)((char*)destination + 1);
                source = (uint32_t*)((char*)source + 1);
            }

            source = scratch;
            destination = bottom;
            for (count = (half_bytes & 0x7fffffffu) >> 1;
                 count != 0u; --count) {
                *destination++ = *source++;
            }
            for (count = span & 3u; count != 0u; --count) {
                *(uint8_t*)destination = *(uint8_t*)source;
                destination = (uint32_t*)((char*)destination + 1);
                source = (uint32_t*)((char*)source + 1);
            }

            ++lap;
            top = (uint32_t*)((char*)top + span);
            bottom = (uint32_t*)((char*)bottom - span);
        } while (lap < ((uint32_t)rec->row_count >> 1));
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
    const uint32_t shr_low = (uint32_t)(8 - pop_count32(mask_low)) & 0x1fu;
    const uint32_t shr_mid = (uint32_t)(8 - pop_count32(mask_mid)) & 0x1fu;
    const uint32_t shr_hi = (uint32_t)(8 - pop_count32(mask_hi)) & 0x1fu;
    const uint32_t ffs_low = find_first_set_bit32(mask_low);
    const uint32_t ffs_mid = find_first_set_bit32(mask_mid);
    const uint32_t ffs_hi = find_first_set_bit32(mask_hi);
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
        dst += (rec->row_bytes >> 1) - job->width;
    }
    surface_pixel_rows_flip_with_scratch(rec);
    return 1;
}

/* Portable palettized bitmap decoder. The retail 0x47d000 entrypoint instead
 * takes two DirectDraw surface interfaces plus a palette pointer, locks and
 * unlocks those surfaces, and requires the retail 0x7c-byte descriptor ABI. */
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
    if (rec->depth == 8) {
        return 0;
    }
    if (rec->depth == 16) {
        if (job->src == NULL) {
            return 1; /* deviation: the corpus strides the job blindly */
        }
        const uint32_t mask_low = rec->mask_low;
        const uint32_t mask_mid = rec->mask_mid;
        const uint32_t mask_hi = rec->mask_hi;
        const uint32_t shr_low =
            (uint32_t)(8 - pop_count32(mask_low)) & 0x1fu;
        const uint32_t shr_mid =
            (uint32_t)(8 - pop_count32(mask_mid)) & 0x1fu;
        const uint32_t shr_hi =
            (uint32_t)(8 - pop_count32(mask_hi)) & 0x1fu;
        const uint32_t ffs_low = find_first_set_bit32(mask_low);
        const uint32_t ffs_mid = find_first_set_bit32(mask_mid);
        const uint32_t ffs_hi = find_first_set_bit32(mask_hi);
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
            dst += (rec->row_bytes >> 1) - job->width;
        }
        surface_pixel_rows_flip_with_scratch(rec);
        return 1;
    }
    if (rec->depth == 24 || rec->depth == 32) return 0;
    return 1;
}

/* Portable conversion dispatcher used by host-side format tests. The retail
 * entrypoint below obtains this record from a locked DirectDraw surface. */
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

/* SurfacePixelFormatQueryConvert (FUN_0047d490): lock the supplied surface,
 * build its pixel record from the 0x7c-byte descriptor, dispatch the 8/24-bit
 * conversion, and unlock on every successful lock. The source buffer and
 * palette remain caller-owned in SurfaceConvertJob. */
// FUNCTION: LEGORR 0x0047d490
int surface_pixel_format_query_convert_surface(
    void* surface, const SurfaceConvertJob* job)
{
#if defined(LEGORRR_BACKEND_MINIWIN) || \
    (defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64))
    void** vtable = surface_interface_vtable(surface);
    typedef int32_t (SURFACE_COM_CALL *SurfaceRawLockFn)(
        void*, void*, void*, uint32_t, void*);
    typedef int32_t (SURFACE_COM_CALL *SurfaceRawUnlockFn)(void*, void*);
    SurfaceRawLockFn lock;
    SurfaceRawUnlockFn unlock;
#if defined(LEGORRR_BACKEND_MINIWIN)
    DDSURFACEDESC descriptor;
#else
    uint32_t descriptor[31];
    const unsigned char* bytes = (const unsigned char*)descriptor;
#endif
    SurfaceRecord record;
    uint32_t height;
    uint32_t width;
    uint32_t pitch;
    uint32_t bits_per_pixel;
    int result;

    if (surface == NULL || job == NULL || vtable == NULL ||
        vtable[25] == NULL || vtable[32] == NULL) return 0;
    lock = (SurfaceRawLockFn)vtable[25];
    unlock = (SurfaceRawUnlockFn)vtable[32];
#if defined(LEGORRR_BACKEND_MINIWIN)
    memset(&descriptor, 0, sizeof(descriptor));
    descriptor.dwSize = sizeof(descriptor);
#else
    memset(descriptor, 0, sizeof(descriptor));
    descriptor[0] = 0x7cu;
#endif
    if (lock(surface, NULL, descriptor, 1u, NULL) != 0) return 0;
#if defined(LEGORRR_BACKEND_MINIWIN)
    height = descriptor.dwHeight;
    width = descriptor.dwWidth;
    pitch = (uint32_t)descriptor.lPitch;
    bits_per_pixel = descriptor.ddpfPixelFormat.dwRGBBitCount;
    memset(&record, 0, sizeof(record));
    record.pixels = descriptor.lpSurface;
    record.mask_low = descriptor.ddpfPixelFormat.dwRBitMask;
    record.mask_mid = descriptor.ddpfPixelFormat.dwGBitMask;
    record.mask_hi = descriptor.ddpfPixelFormat.dwBBitMask;
#else
    memcpy(&height, bytes + 8u, sizeof(height));
    memcpy(&width, bytes + 12u, sizeof(width));
    memcpy(&pitch, bytes + 16u, sizeof(pitch));
    memcpy(&bits_per_pixel, bytes + 88u, sizeof(bits_per_pixel));
    memset(&record, 0, sizeof(record));
    {
        uint32_t pixels;
        memcpy(&pixels, bytes + 36u, sizeof(pixels));
        record.pixels = (void*)(uintptr_t)pixels;
    }
    memcpy(&record.mask_low, bytes + 92u, sizeof(record.mask_low));
    memcpy(&record.mask_mid, bytes + 96u, sizeof(record.mask_mid));
    memcpy(&record.mask_hi, bytes + 100u, sizeof(record.mask_hi));
#endif
    record.row_count = (int)height;
    record.row_bytes = (int)pitch;
    record.depth = (int)bits_per_pixel;
    (void)width; /* source width is carried by the conversion job */
    result = surface_pixel_format_query_convert(&record, job,
                                                bits_per_pixel);
    (void)unlock(surface, NULL);
    return result;
#else
    (void)surface;
    (void)job;
    return 0;
#endif
}

/* This frame-oriented adapter remains for callers which own the active
 * DisplaySurface. Bitmap/resource conversion callers should pass their
 * surface directly to SurfacePixelFormatQueryConvert. */
int surface_active_pixel_format_query_convert(const SurfaceConvertJob* job)
{
#if defined(LEGORRR_BACKEND_MINIWIN) || \
    (defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64))
    DisplaySurface* target = s_active_render_target;
    return surface_pixel_format_query_convert_surface(
        target != NULL ? target->surf : NULL, job);
#else
    (void)job;
    return 0;
#endif
}

namespace unported_ledger_stubs {
// Compatibility wrapper; the sole mapped body is the pool implementation
// above, which also initializes the FUN_00483170 state words.
int ledger_stub_00482220_MeshRecordAllocateAndFill(
    MeshRecordPool* pool, uint32_t vertex_count, uint32_t index_rows,
    uint32_t index_columns, const uint16_t* source_indices)
{
    return mesh_record_allocate_and_fill(pool, vertex_count, index_rows,
                                         index_columns, source_indices);
}



// ModelRenderPassDispatch
void ledger_stub_00482d60_ModelRenderPassDispatch(void* device)
{
    model_render_pass_dispatch(device);
}

// AnimatedTextureFrameSurfaceSet (implemented in gods98/scene_resources.cpp)
void ledger_stub_004830c0_AnimatedTextureFrameSurfaceSet(
    void* texture, int frame_index, const void* frame_state)
{
    lego::animated_texture_frame_surface_set(texture, frame_index,
                                              frame_state);
}

// LoadLWSCScene

// MeshRecordAllocateOrAppend compatibility entrypoint.
int ledger_stub_004748f0_MeshRecordAllocateOrAppend(
    void* object, uint32_t vertex_count, uint32_t index_rows,
    uint32_t index_columns, const uint16_t* source_indices)
{
    return mesh_record_allocate_or_append(object, vertex_count, index_rows,
                                          index_columns, source_indices);
}


// FUNCTION: LEGORR 0x00476e50
ScreenBoundedSurfaceRecord*
ledger_stub_00476e50_ScreenBoundedSurfaceResourceAllocate(
    uint32_t width, uint32_t height, void* camera, int x, int y)
{
    return screen_bounded_surface_allocate(width, height, camera, x, y);
}



// InterfaceMethod18StructApply
// FUNCTION: LEGORR 0x0047e300
int ledger_stub_0047e300_InterfaceMethod18StructApply(
    void* destination, const void* source, size_t size,
    void (*apply)(void* destination, const void* source, size_t size,
                  void* context), void* context)
{
    if (destination == NULL || source == NULL || size == 0u) return 0;
    if (apply != NULL) apply(destination, source, size, context);
    else memcpy(destination, source, size);
    return 1;
}

// InterfaceMethodResultRecordInitialize
// FUNCTION: LEGORR 0x0047e3e0
void ledger_stub_0047e3e0_InterfaceMethodResultRecordInitialize(
    unsigned int* result, void* device, unsigned int interface_value,
    void* context, unsigned int first_color, unsigned int second_color)
{
    if (result == NULL) return;
    if (s_small_record_free == NULL) {
        fixed_size_small_record_pool_grow();
    }
    result[1] = interface_value;
    result[7] = (unsigned int)(uintptr_t)result;
    result[6] = 0;
    result[2] = (unsigned int)(uintptr_t)context;
    result[0] = (unsigned int)(uintptr_t)device;
    result[5] = first_color;
    result[3] = font_device_color_convert(device, first_color);
    result[4] = font_device_color_convert(device, second_color);
}

// ComObjectDestroyAndOptionallyFree
// FUNCTION: LEGORR 0x0047e460
void ledger_stub_0047e480_TwoInterfaceFieldsReleaseAndReset(void* object);
void ledger_stub_0047e460_ComObjectDestroyAndOptionallyFree(void* object,
                                                              unsigned char flags)
{
    if (object == 0) return;
    ledger_stub_0047e480_TwoInterfaceFieldsReleaseAndReset(object);
    if ((flags & 1u) != 0) ui_render_record_free(object);
}

// TwoInterfaceFieldsReleaseAndReset
// FUNCTION: LEGORR 0x0047e480
void ledger_stub_0047e480_TwoInterfaceFieldsReleaseAndReset(void* object)
{
    typedef unsigned long (SURFACE_COM_CALL *ReleaseMethod)(void*);
    unsigned char* bytes = (unsigned char*)object;
    unsigned int* slots;
    void* interface_object;
    void** vtable;
    if (bytes == 0) return;
    slots = (unsigned int*)bytes;
    slots[0] = 0;
    interface_object = *(void**)(bytes + 24);
    if (interface_object != 0) {
        vtable = *(void***)interface_object;
        if (vtable != 0 && vtable[2] != 0) {
            ReleaseMethod release = (ReleaseMethod)vtable[2];
            release(interface_object);
        }
        *(void**)(bytes + 24) = 0;
    }
    interface_object = *(void**)(bytes + 16);
    if (interface_object != 0) {
        vtable = *(void***)interface_object;
        if (vtable != 0 && vtable[2] != 0) {
            ReleaseMethod release = (ReleaseMethod)vtable[2];
            release(interface_object);
        }
        *(void**)(bytes + 16) = 0;
    }
}


/* Compatibility wrapper; the mapped implementation is material_pass_execute. */
int ledger_stub_00480ac0_MaterialPass(const char* path_prefix,
                                     void* render_object,
                                     const Model* model, uint32_t flags)
{
    return material_pass_execute(path_prefix, render_object, model, flags);
}

/* Compatibility wrapper; the mapped implementation is the parser-backed
 * function above. */
int ledger_stub_00481820_BindModelTextures(
    const char* path_prefix, const Model* model,
    ModelTextureBinding* bindings, int force_source_color)
{
    bind_model_textures_for_model(path_prefix, model, bindings,
                                  force_source_color);
    return model != NULL &&
        (model->n_surfaces == 0u || bindings != NULL) ? 1 : 0;
}


} // namespace unported_ledger_stubs

void texture_surface_create_hook_set(TextureSurfaceCreateHook hook,
                                     void* context)
{
    s_texture_surface_create_hook = hook;
    s_texture_surface_create_context = context;
}

/* TextureSurfaceCreate (FUN_004744f0): mode zero resolves the configured
 * renderer surface database; nonzero mode uses the named bitmap path. The
 * configured database and its COM object tree are renderer-owned, so the
 * hook handles that route. Mode one can use the portable BMP decoder and its
 * release-tracked host record. */
// FUNCTION: LEGORR 0x004744f0
void* texture_surface_create(const char* name, uint32_t mode,
                             uint32_t* value_out, uint32_t mode_data)
{
    if (name == NULL) return NULL;
    if (s_texture_surface_create_hook != NULL) {
        return s_texture_surface_create_hook(s_texture_surface_create_context,
                                             name, mode, value_out,
                                             mode_data);
    }
    if (mode != 1u) return NULL;
    return texture_surface_create_host_record(name, mode);
}

#undef SURFACE_COM_CALL

static SurfaceRewardSuccessCaptureFn s_reward_success_capture;
static void* s_reward_success_capture_context;

void surface_reward_success_capture_hook_set(
    SurfaceRewardSuccessCaptureFn capture, void* context)
{
    s_reward_success_capture = capture;
    s_reward_success_capture_context = context;
}

int surface_reward_success_capture(uint32_t width, uint32_t height,
                                   uint32_t words_out[8])
{
    if (width == 0u || height == 0u || words_out == NULL ||
        s_reward_success_capture == NULL) return 0;
    return s_reward_success_capture(s_reward_success_capture_context,
                                    width, height, words_out);
}
