// SPDX-License-Identifier: LGPL-3.0-or-later
/* G98-015 G98-015: the unit's functions in original address order and the
 * state they own, relocated verbatim by tools/relocate_unit.py (ADR 0006).
 * Body hashes: docs/coverage/relocations/G98-015.csv. */
#include "record.hpp"
#include "heap.hpp"
#include "sound.hpp"
#include "string.hpp"
#include "view.hpp"
#include "texture.hpp"
#include "resource.hpp"
#include "crt/crt.hpp"
#include "game/util.hpp"
#include <math.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include "record_shared.hpp"
#include <float.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include "spawn.hpp"
#include "port_loop_ports.hpp"
#include "game/entity.hpp"
#include "level.hpp"
#include "cmd.hpp"
#include "select.hpp"
#include "terrain_grid.hpp"
#include "type_config_ports.hpp"
#include "ai.hpp"
#include "gods98/engine.hpp"
#include "gods98/scene_resources.hpp"
#include "../gods98/message.hpp"
#include "../gods98/camera.hpp"
#include "input.hpp"
#include "frameloop.hpp"
#include "model.hpp"
#include "surface.hpp"
#include "platform/platform.h"
#include "surface_for_directdraw.hpp"
#include "images.h"
#include "image_adapters.h"
#include <stdarg.h>
#include "scene_pool.hpp"
#if defined(LEGORRR_BACKEND_MINIWIN)
#include "container_native.hpp"
#endif
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
#include "lwsc_winapi.hpp"
#endif
#include "record_for_mesh_unit.hpp"
#include "game/spawn_for_mesh_unit.hpp"
#include "scene_resources_for_mesh_unit.hpp"
#include "surface_for_mesh_unit.hpp"
#include "view_for_mesh_unit.hpp"

/* Render-resource instance records have their free-list link at +0x30.
 * Include the native pointer width in the stride so x86 keeps the corpus's
 * 0x34-byte records. */
#define RENDER_RESOURCE_INSTANCE_RECORD_SIZE (0x30u + sizeof(void*))
#define RENDER_RESOURCE_INSTANCE_LINK_OFFSET 0x30

/* ModelRenderListClear (FUN_00482d30) — see crt.hpp. */
// GLOBAL: LEGORR 0x005343c4
void* s_model_render_list_head; /* DAT_005343c4 role: unowned */

// GLOBAL: LEGORR 0x0054057c
uint32_t s_model_material_handle;

// GLOBAL: LEGORR 0x00540584
void* s_render_resource_instance_pool_blocks[20];
// GLOBAL: LEGORR 0x005405d4
void* s_render_resource_instance_pool_head;
// GLOBAL: LEGORR 0x005405d8
uint32_t s_render_resource_instance_pool_count;

namespace unported_ledger_stubs {
static InputActionToggleState s_input_action_toggle_state;
} // namespace unported_ledger_stubs

static uint32_t s_d3d_snapshot_light;
static uint32_t s_d3d_snapshot_render;
static uint32_t s_d3d_snapshot_texture_stage;

static void* s_direct3d_cached_texture;
static uint32_t s_direct3d_cached_material_handle;

/* Defined below (forward declarations added by tools/relocate_unit.py). */
static void portable_model_render_base_pass(void*, void* model, void* device, const void*);
static uint32_t portable_model_render_entry_flags(void*, void* opaque_entry);
static void* portable_model_render_first(void*);
static void* portable_model_render_model(void*, void* opaque_entry);
static uint32_t portable_model_render_model_flags(void*, void* opaque_model);
static void* portable_model_render_next(void*, void* opaque_entry);
static const void* portable_model_render_parameters(void*, void* opaque_entry);
static void portable_model_render_queue_flush(void*);
static void* model_texture_lookup(const char* path_prefix, const char* name, ModelTextureBinding* binding);
static int model_texture_name_build(const char* path_prefix, const char* image_name, char output[260]);
static char* mesh_block(const void* record, int block);
static void* mesh_record_default_allocate(void*, size_t bytes);
static void mesh_record_default_release(void*, void* memory);
static void rec_float_put(void* base, size_t off, float v);
static float channel_clamp(float v);
static int direct3d_interface_create_from_device(void* device);

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

namespace unported_ledger_stubs {
// SceneObjectSetFormattedString
// AnimationTrackApplyWrappedFrameOffset
void ledger_stub_004830c0_AnimatedTextureFrameSurfaceSet(
    void* texture, int frame_index, const void* frame_state);

int ledger_stub_00482040_AnimationTrackApplyWrappedFrameOffset(
    void* animation_owner, int frame_offset);
} // namespace unported_ledger_stubs

namespace lego {
// FUNCTION: LEGORR 0x004805b0
int shared_texture_root_register(const char* root)
{
    /* SharedTextureRootRegister drops the shared-pool link and releases the
     * model-render list (DAT_005343c4 + FUN_00482d30) before re-seeding;
     * crt.cpp owns that list head, so the walk-and-free runs there. */
    model_render_list_clear();
    if (root != 0) {
        size_t length = strlen(root);
        size_t written;
        if (length >= sizeof(s_shared_texture_root))
            length = sizeof(s_shared_texture_root) - 1u; /* port bound */
        written = length;
        /* The reference checks the final input character: if the root
         * lacks a trailing backslash, it appends one.  The reference also
         * repoints the model-render list
         * head (DAT_00534558) at this freshly allocated string; this port
         * keeps the root in the module static instead. */
        if (length == 0u || root[length - 1u] != '\\') {
            if (length >= sizeof(s_shared_texture_root) - 1u)
                length = sizeof(s_shared_texture_root) - 2u;
            written = length + 1u;
        }
        if (length != 0u) memcpy(s_shared_texture_root, root, length);
        s_shared_texture_root[written - 1u] = '\\';
        s_shared_texture_root[written] = '\0';
    }
    /* DAT_00540584/D4/D8 are the render-resource instance pool owned by
     * record.cpp. TextureGetOrCreate keys cached shared resources by their
     * complete path, so changing this root does not require cache eviction. */
    render_resource_instance_pool_reset();
    (void)shared_texture_root_initialize();
    return 1;
}

// FUNCTION: LEGORR 0x00480780
int zero_result_is_success(void* value)
{
    return s_result_check_hook != 0 && s_result_check_hook(s_result_check_context, value) == 0;
}
} // namespace lego

// FUNCTION: LEGORR 0x004807a0
void* render_resource_instance_record_acquire(void)
{
    if (s_render_resource_instance_pool_head == NULL) {
        render_resource_instance_pool_grow();
    }
    if (s_render_resource_instance_pool_head == NULL) return NULL;
    char* record = (char*)s_render_resource_instance_pool_head;
    void* next = NULL;
    __builtin_memcpy(&next,
                     record + RENDER_RESOURCE_INSTANCE_LINK_OFFSET,
                     sizeof(next));
    s_render_resource_instance_pool_head = next;
    memset(record, 0, 0x34);
    __builtin_memcpy(record + RENDER_RESOURCE_INSTANCE_LINK_OFFSET,
                     &record, sizeof(record));
    return record;
}

// FUNCTION: LEGORR 0x004807d0
void render_resource_instance_record_release(void* record)
{
    __builtin_memcpy((char*)record + RENDER_RESOURCE_INSTANCE_LINK_OFFSET,
                     &s_render_resource_instance_pool_head,
                     sizeof(s_render_resource_instance_pool_head));
    s_render_resource_instance_pool_head = record;
}

// FUNCTION: LEGORR 0x004807f0
void render_resource_instance_pool_grow(void)
{
    const uint32_t count = 1u <<
        ((uint32_t)(unsigned char)s_render_resource_instance_pool_count & 0x1fu);
    void* base = engine_heap_alloc((long)count *
                                   RENDER_RESOURCE_INSTANCE_RECORD_SIZE);
    s_render_resource_instance_pool_blocks[
        s_render_resource_instance_pool_count] = base;
    if (base == NULL) return;
    s_render_resource_instance_pool_count++;

    char* record = (char*)base;
    for (uint32_t i = 1; i < count; ++i) {
        void* next = record + RENDER_RESOURCE_INSTANCE_RECORD_SIZE;
        __builtin_memcpy(record + RENDER_RESOURCE_INSTANCE_LINK_OFFSET,
                         &next, sizeof(next));
        record += RENDER_RESOURCE_INSTANCE_RECORD_SIZE;
    }
    __builtin_memcpy(record + RENDER_RESOURCE_INSTANCE_LINK_OFFSET,
                     &s_render_resource_instance_pool_head,
                     sizeof(s_render_resource_instance_pool_head));
    s_render_resource_instance_pool_head = base;
}

// FUNCTION: LEGORR 0x00480870
EngineTaskCallbackRecord* engine_task_callback_record_allocate_and_initialize(
    void* owner, void* callback, uint32_t mode, void* context,
    uint32_t flag_value)
{
    EngineTaskCallbackRecord* record;
    void* resource_interface = NULL;
    const EngineTaskCallbackHooks* hooks = &s_engine_task_callback_hooks;
    record = (EngineTaskCallbackRecord*)
        render_resource_instance_record_acquire();
    if (record == NULL) return NULL;
    /* The fixed-size pool has already cleared +0..+0x2f and installed its
     * self-link at +0x30; do not erase that link. */
    /* FUN_00472940 is an observed root-global read whose return is discarded. */
    (void)scene_root_object_get();
    if (hooks->user_visual_create != NULL)
        hooks->user_visual_create(hooks->context,
                                  (uint32_t)(uintptr_t)callback,
                                  record, &resource_interface);
    if (hooks->owner_bind != NULL)
        hooks->owner_bind(hooks->context, owner, resource_interface);
    if (hooks->callback_attach != NULL)
        hooks->callback_attach(hooks->context, resource_interface, record);
    /* 0x4808aa..0x4808c6 copies stack arguments 2/4/3 into +10/+14/+18.
     * LAB_004827f0 is the UserVisual manager entry passed to the factory;
     * its address and the record's self-link are not these metadata words. */
    record->words[3] = (uint32_t)(uintptr_t)resource_interface;
    record->words[4] = (uint32_t)(uintptr_t)callback;
    record->words[5] = (uint32_t)(uintptr_t)context;
    record->words[6] = mode;
    record->words[8] = 1u;
    record->words[10] = (uint32_t)(uintptr_t)owner;
    if (flag_value == 1u) record->words[11] |= 2u;
    else if (flag_value == 2u) record->words[11] |= 4u;
    return record;
}

namespace lego {
// FUNCTION: LEGORR 0x00480900
CachedRenderResourceInstanceRecord* cached_render_resource_instance_create(
    CachedRenderResourceTemplateRecord* source)
{
    CachedRenderResourceTemplateRecord* template_record;
    CachedRenderResourceInstanceRecord* instance;
    uint32_t count;
    uint32_t index;
    size_t bytes;
    void* pool_record;
    if (source == NULL) return NULL;

    template_record = source->shared_template != NULL
        ? source->shared_template : source;
    count = template_record->words[0];
    if (count > 0x7fffffffu / 0x70u) return NULL;
    bytes = (size_t)count * 0x70u;

    pool_record = render_resource_instance_record_acquire();
    if (pool_record == NULL) return NULL;
    instance = (CachedRenderResourceInstanceRecord*)engine_heap_alloc(
        (long)sizeof(*instance));
    if (instance == NULL) {
        render_resource_instance_record_release(pool_record);
        return NULL;
    }
    memset(instance, 0, sizeof(*instance));
    memcpy(instance->words, template_record->words,
           sizeof(instance->words));
    instance->pool_record = pool_record;
    instance->words[1] = count;
    instance->words[2] = 0u;
    instance->words[3] = 0u;
    instance->words[8] = 0u;
    instance->words[9] = 0u;
    instance->words[10] = 0u;
    memcpy(pool_record, instance->words, sizeof(instance->words));
    instance->source_template = template_record;

    if (bytes != 0u) {
        if (template_record->entry_block == NULL) {
            render_resource_instance_record_release(pool_record);
            engine_heap_free(instance);
            return NULL;
        }
        instance->entry_block = (unsigned char*)engine_heap_alloc(
            (long)bytes);
        if (instance->entry_block == NULL) {
            render_resource_instance_record_release(pool_record);
            engine_heap_free(instance);
            return NULL;
        }
        /* The source pointer includes a 0x10-byte header. Retail copies
         * count * 0x70 bytes from it, then initializes and copies each
         * per-entry 0x50-byte transform window at +0x10. */
        memcpy(instance->entry_block, template_record->entry_block, bytes);
        for (index = 0u; index < count; ++index) {
            unsigned char* destination = instance->entry_block + 0x10u
                + (size_t)index * 0x70u;
            const unsigned char* entry_source =
                template_record->entry_block + 0x10u
                + (size_t)index * 0x70u;
            uint32_t transform[20];
            unsigned int word;
            transform[0] = 0x50u;
            transform[1] = 0x3f800000u;
            transform[2] = 0x3f800000u;
            transform[3] = 0x3f800000u;
            transform[4] = 0x3f800000u;
            for (word = 5u; word < 20u; ++word) transform[word] = 0u;
            memcpy(destination, transform, sizeof(transform));
            memcpy(destination, entry_source, sizeof(transform));
        }
    }

    /* These renderer calls create and attach the wrapped interface. Their
     * COM vtables belong to the platform renderer; the hook updates the
     * host-side handles without guessing those ABI signatures here. */
    if (s_cached_resource_renderer_hook != NULL)
        s_cached_resource_renderer_hook(instance,
                                        s_cached_resource_renderer_context);

    ++template_record->words[8];
    template_record->words[11] |= 0x00200000u;
    return instance;
}
} // namespace lego

/* LwoModelEntryLoad (FUN_004809e0) derives a directory prefix, opens and
 * parses the model, then binds each parsed surface's texture through the
 * existing host resource/cache path. The task-record and renderer-object
 * inputs still have no portable owner. The bounded copy preserves the valid
 * path prefix behavior without reproducing the retail stack overwrite. */
// FUNCTION: LEGORR 0x004809e0
Model* lwo_model_entry_load(const char* path, void* flags, void* context)
{
    char directory[260];
    size_t length;
    size_t last_separator;
    size_t i;
    Model* model;

    (void)flags;    /* task callback flags have no host material equivalent */
    (void)context;  /* renderer object creation remains a backend operation */
    if (path == NULL) return NULL;
    length = strlen(path);
    if (length >= sizeof(directory)) length = sizeof(directory) - 1u;
    memcpy(directory, path, length);
    directory[length] = '\0';
    last_separator = 0u;
    for (i = 0; i < length; ++i)
        if (directory[i] == '\\' || directory[i] == '/')
            last_separator = i + 1u;
    directory[last_separator] = '\0';   /* keeps the backslash, else empties */
    model = model_open(path);
    if (model != NULL) {
        model->render_source = (ModelRenderSourceRecord*)calloc(
            1u, sizeof(*model->render_source));
        if (model->render_source != NULL) {
            model->render_source->model = model;
            memcpy(model->render_source->path_prefix, directory,
                   strlen(directory) + 1u);
        }
    }
    if (model != NULL && model->n_surfaces != 0u &&
        model->n_surfaces <= (size_t)(0x7fffffffL /
                                      sizeof(ModelTextureBinding))) {
        size_t bytes = model->n_surfaces * sizeof(ModelTextureBinding);
        ModelTextureBinding* bindings =
            (ModelTextureBinding*)calloc(1u, bytes);
        if (bindings != NULL) {
            bind_model_textures_for_model(directory, model, bindings, 0);
            model->material_bindings = bindings;
            model->material_binding_count = model->n_surfaces;
        }
        else {
            if (bindings != NULL) {
                model_texture_bindings_release(bindings,
                    (uint32_t)model->n_surfaces);
                free(bindings);
            }
        }
    }
    return model;
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

namespace unported_ledger_stubs {
// FUNCTION: LEGORR 0x00482040
int ledger_stub_00482040_AnimationTrackApplyWrappedFrameOffset(
    void* animation_owner, int frame_offset)
{
    unsigned char* owner = (unsigned char*)animation_owner;
    unsigned char* records;
    uint32_t count;
    uint32_t flags;
    uint32_t slot;
    if (owner == NULL) return 0;
    memcpy(&count, owner, sizeof(count));
    memcpy(&flags, owner + 0x2c, sizeof(flags));
    if ((flags & 4u) == 0u) return 0;
    memcpy(&records, owner + 8, sizeof(records));
    if (records == NULL) return 1;
    for (slot = 0; slot < count; ++slot) {
        unsigned char* record = records + (size_t)slot * 0x70u;
        unsigned char* track;
        uint32_t track_flags;
        int frame_count;
        int frame_base;
        int frame_index;
        void* frames;
        void* frame;
        memcpy(&track, record + 0x68, sizeof(track));
        if (track == NULL) continue;
        memcpy(&track_flags, track + 0x34, sizeof(track_flags));
        if ((track_flags & 0x80u) == 0u) continue;
        memcpy(&frame_count, track + 8, sizeof(frame_count));
        memcpy(&frame_base, track + 0x38, sizeof(frame_base));
        memcpy(&frames, track, sizeof(frames));
        if (frame_count <= 0 || frames == NULL) continue;
        frame_index = (frame_base + frame_offset) % frame_count;
        /* The reference clamps a negative remainder to frame zero. */
        if (frame_index < 0) frame_index = 0;
        memcpy(&frame, (unsigned char*)frames +
               (size_t)frame_index * sizeof(frame), sizeof(frame));
        ledger_stub_004830c0_AnimatedTextureFrameSurfaceSet(
            animation_owner, (int)slot, frame);
    }
    return 1;
}
} // namespace unported_ledger_stubs

/* IndexedRecordEntryRead (FUN_004821a0) — see record.hpp. */
// FUNCTION: LEGORR 0x004821a0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void indexed_record_entry_read(
    void* record, int block, uint32_t* field_out, uint32_t reserved,
    uint32_t* kind_out, uint32_t* count_out, uint32_t* indices_out)
{
    __asm {
        mov ecx, dword ptr [esp + 8]
        push esi
        mov eax, ecx
        push edi
        shl eax, 3
        sub eax, ecx
        mov ecx, dword ptr [esp + 0ch]
        shl eax, 4
        mov esi, dword ptr [ecx + 8]
        mov ecx, dword ptr [esp + 14h]
        add eax, esi
        test ecx, ecx
        je short indexed_record_entry_read_kind
        mov edx, dword ptr [eax + 4]
        mov dword ptr [ecx], edx
    indexed_record_entry_read_kind:
        mov ecx, dword ptr [esp + 1ch]
        test ecx, ecx
        je short indexed_record_entry_read_count
        mov dword ptr [ecx], 3
    indexed_record_entry_read_count:
        mov ecx, dword ptr [esp + 20h]
        test ecx, ecx
        je short indexed_record_entry_read_indices
        mov edx, dword ptr [eax]
        mov dword ptr [ecx], edx
    indexed_record_entry_read_indices:
        mov edx, dword ptr [esp + 24h]
        test edx, edx
        je short indexed_record_entry_read_done
        mov esi, dword ptr [eax]
        xor ecx, ecx
        test esi, esi
        jbe short indexed_record_entry_read_done
    indexed_record_entry_read_loop:
        mov esi, dword ptr [eax + 8]
        xor edi, edi
        add edx, 4
        mov di, word ptr [esi + ecx*2]
        inc ecx
        mov dword ptr [edx - 4], edi
        mov esi, dword ptr [eax]
        cmp ecx, esi
        jb short indexed_record_entry_read_loop
    indexed_record_entry_read_done:
        pop edi
        pop esi
        ret
    }
}
#else
void indexed_record_entry_read(void* record, int block, uint32_t* field_out,
                               uint32_t reserved, uint32_t* kind_out,
                               uint32_t* count_out, uint32_t* indices_out)
{
    (void)reserved;
    const char* blk = (const char*)mesh_block(record, block);
    if (blk == NULL) {
        return; /* deviation: the corpus strides a missing table blindly */
    }
    if (field_out != NULL) {
        *field_out = rec_dword(blk, 0x04);
    }
    if (kind_out != NULL) {
        *kind_out = 3u; /* corpus: the entry read answers shape kind 3 */
    }
    const uint32_t count = rec_dword(blk, 0x00);
    if (count_out != NULL) {
        *count_out = count;
    }
    if (indices_out != NULL && count != 0u) {
        const char* idx = (const char*)rec_ptr(blk, 0x08);
        if (idx == NULL) {
            return; /* deviation: the corpus strides a missing pointer */
        }
        for (uint32_t i = 0; i < count; ++i) {
            uint16_t v = 0u;
            __builtin_memcpy(&v, idx + (size_t)i * 2u, sizeof v);
            indices_out[i] = v;
        }
    }
}
#endif
// FUNCTION: LEGORR 0x00482210
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) uint32_t pointer_value_get(const void* pointed)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        mov eax, dword ptr [eax]
        ret
    }
}
#else
uint32_t pointer_value_get(const void* pointed)
{
    if (pointed == NULL) {
        return 0u; /* pinned deviation: the corpus loads blindly */
    }
    return *(const uint32_t*)pointed;
}
#endif

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

/* --- the record primitive band (see record.hpp) ----------------------------- */
// FUNCTION: LEGORR 0x00482350
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void indexed_record_field64_set_target(
    void* record, int index, uint32_t value)
{
    __asm {
        mov eax, dword ptr [esp + 8]
        mov edx, dword ptr [esp + 4]
        mov ecx, eax
        shl ecx, 3
        sub ecx, eax
        mov eax, dword ptr [edx + 8]
        mov edx, dword ptr [esp + 0ch]
        shl ecx, 4
        mov dword ptr [eax + ecx + 64h], edx
        ret
    }
}
#else
void indexed_record_field64_set_target(void* record, int index,
                                               uint32_t value)
{
    char* base = *(char**)((char*)record + 8);
    *(uint32_t*)(base + index * 0x70 + 100) = value;
}
#endif

// FUNCTION: LEGORR 0x00482370
void mesh_vertices_scale_apply(void* record, float scale_x, float scale_y,
                               float scale_z)
{
    for (uint32_t i = 0; i < rec_dword(record, 0); ++i) {
        char* block = mesh_block(record, (int)i);
        void* verts = block != NULL ? rec_ptr(block, 0x0c) : NULL;
        if (verts == NULL || rec_dword(block, 0x04) == 0) {
            continue; /* corpus: the zero-count block skips its do-while */
        }
        uint32_t j = 0;
        do {
            char* v = (char*)verts + (size_t)j * 0x20;
            rec_float_put(v, 0x00, scale_x * rec_float(v, 0x00));
            rec_float_put(v, 0x04, scale_y * rec_float(v, 0x04));
            rec_float_put(v, 0x08, scale_z * rec_float(v, 0x08));
            ++j;
            /* corpus shape: the vertex count is re-read every round */
        } while (j < rec_dword(block, 0x04));
    }
}

/* TerrainCellQuadVerticesWrite (FUN_004823e0) — see record.hpp. */
// FUNCTION: LEGORR 0x004823e0
void terrain_cell_quad_vertices_write(void* record, int block,
                                      int first_vertex, int count,
                                      const void* source)
{
    char* blk = mesh_block(record, block);
    char* verts = blk != NULL ? (char*)rec_ptr(blk, 0x0c) : NULL;
    if (verts == NULL || source == NULL || count == 0) {
        return; /* deviation: the corpus strides both windows blindly */
    }
    /* Corpus shape: eight dwords per round (3+3+2) from 36-byte source
     * slots into the block's 32-byte vertex stride. */
    size_t dst = (size_t)((uint32_t)first_vertex << 5);
    const char* src = (const char*)source;
    int n = count;
    do {
        __builtin_memcpy(verts + dst + 0x00, src + 0x00, 12);
        __builtin_memcpy(verts + dst + 0x0c, src + 0x0c, 12);
        __builtin_memcpy(verts + dst + 0x18, src + 0x18, 8);
        dst += 0x20;
        src += 36;
    } while (--n != 0);
}
// FUNCTION: LEGORR 0x00482470
void indexed_geometry_data_copy_to_output(void* record, int block,
                                          int first_vertex, int count,
                                          void* output)
{
    char* blk = mesh_block(record, block);
    char* verts = blk != NULL ? (char*)rec_ptr(blk, 0x0c) : NULL;
    if (verts == NULL || output == NULL || count == 0) {
        return; /* deviation: the corpus strides both windows blindly */
    }
    /* Corpus shape: a 36-byte output slot per round, filled 3+3+2
     * dwords — the slot's fourth dword stays untouched. */
    size_t src = (size_t)((uint32_t)first_vertex << 5);
    char* dst = (char*)output;
    int n = count;
    do {
        __builtin_memcpy(dst + 0x00, verts + src + 0x00, 12);
        __builtin_memcpy(dst + 0x0c, verts + src + 0x0c, 12);
        __builtin_memcpy(dst + 0x18, verts + src + 0x18, 8);
        src += 0x20;
        dst += 36;
    } while (--n != 0);
}

// FUNCTION: LEGORR 0x00482500
void instance_quad_vertex_data_fill(void* record, int block,
                                    int first_vertex, int count,
                                    const float* positions,
                                    const float triple[3],
                                    const uint32_t* pairs)
{
    char* blk = mesh_block(record, block);
    char* verts = blk != NULL ? (char*)rec_ptr(blk, 0x0c) : NULL;
    if (verts == NULL || positions == NULL || count == 0) {
        return; /* deviation: the corpus strides its windows blindly */
    }
    size_t off = (size_t)((uint32_t)first_vertex << 5);
    const float* src = positions;
    int n = count;
    do {
        char* v = verts + off;
        __builtin_memcpy(v + 0x00, src, 12);
        float nx = triple[0] - rec_float(v, 0x00);
        float ny = triple[1] - rec_float(v, 0x04);
        float nz = triple[2] - rec_float(v, 0x08);
        /* Corpus shape: the sum squares runs x², z², y² (the x87 order)
         * and the reciprocal rides through in place. A zero normal
         * answers inf/NaN exactly as the corpus's division does. */
        const float inv = 1.0f / sqrtf(nx * nx + nz * nz + ny * ny);
        rec_float_put(v, 0x0c, nx * inv);
        rec_float_put(v, 0x10, inv * ny);
        rec_float_put(v, 0x14, inv * nz);
        rec_dword_put(v, 0x18, *pairs);
        rec_dword_put(v, 0x1c, pairs[1]);
        src += 3;
        pairs += 2;
        off += 0x20;
    } while (--n != 0);
}

/* MeshSegmentVertexFill (FUN_00482620): keep the corpus's blind table and
 * vertex-buffer accesses, calculate the 0x70 table stride inline, and retain
 * its bottom-test loop. The disassembly reloads the vertex-buffer slot for
 * each position/pair group, advances positions by 12 and pairs by 8, and
 * reloads the same triple pointer at `[esp+0x28]` on every round. */
// FUNCTION: LEGORR 0x00482620
void mesh_segment_vertex_fill(void* record, int block, int first_vertex,
                              int count, const float* positions,
                              const uint32_t triple[3],
                              const uint32_t* pairs)
{
    const size_t block_offset = (((size_t)block << 3) - (size_t)block) << 4;
    char* table = (char*)*(void* const*)((char*)record + 0x08);
    char* blk = table + block_offset;
    if (count == 0) {
        return;
    }
    const uint32_t* src = (const uint32_t*)positions;
    const uint32_t* pair = pairs;
    uint32_t vertex_offset = (uint32_t)first_vertex << 5;
    uint32_t remaining = (uint32_t)count;
    do {
        char* verts = (char*)*(void* const*)(blk + 0x0c);
        *(uint32_t*)(verts + vertex_offset + 0x00) = src[0];
        verts = (char*)*(void* const*)(blk + 0x0c);
        *(uint32_t*)(verts + vertex_offset + 0x04) = src[1];
        verts = (char*)*(void* const*)(blk + 0x0c);
        *(uint32_t*)(verts + vertex_offset + 0x08) = src[2];
        /* Pinned corpus quirk: the triple pointer never advances. */
        verts = (char*)*(void* const*)(blk + 0x0c);
        char* triple_dst = verts + vertex_offset + 0x0c;
        *(uint32_t*)(triple_dst + 0x00) = triple[0];
        *(uint32_t*)(triple_dst + 0x04) = triple[1];
        *(uint32_t*)(triple_dst + 0x08) = triple[2];
        verts = (char*)*(void* const*)(blk + 0x0c);
        *(uint32_t*)(verts + vertex_offset + 0x18) = pair[0];
        verts = (char*)*(void* const*)(blk + 0x0c);
        *(uint32_t*)(verts + vertex_offset + 0x1c) = pair[1];
        src += 3;
        pair += 2;
        vertex_offset += 0x20;
    } while (--remaining != 0u);
}

// FUNCTION: LEGORR 0x004826c0
void mesh_vertex_record_fill(void* record, int block, int first_vertex,
                             int count, const float* positions,
                             const void* const* triples,
                             const uint32_t* pairs)
{
    char* blk = mesh_block(record, block);
    char* verts = blk != NULL ? (char*)rec_ptr(blk, 0x0c) : NULL;
    if (verts == NULL || positions == NULL || count == 0) {
        return; /* deviation: the corpus strides its windows blindly */
    }
    size_t off = (size_t)((uint32_t)first_vertex << 5);
    const float* src = positions;
    int n = count;
    do {
        char* v = verts + off;
        __builtin_memcpy(v + 0x00, src, 12);
        /* Corpus shape: the triple stream advances one entry per round —
         * each vertex receives its own triple record. */
        if (triples != NULL && *triples != NULL) {
            const uint32_t* t = (const uint32_t*)*triples;
            rec_dword_put(v, 0x0c, t[0]);
            rec_dword_put(v, 0x10, t[1]);
            rec_dword_put(v, 0x14, t[2]);
        }
        rec_dword_put(v, 0x18, *pairs);
        rec_dword_put(v, 0x1c, pairs[1]);
        src += 3;
        ++triples;
        pairs += 2;
        off += 0x20;
    } while (--n != 0);
}
// FUNCTION: LEGORR 0x00482780
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) uint32_t indexed_record_flag6c_bit0_get(
    const void* record, int index)
{
    __asm {
        mov eax, dword ptr [esp + 8]
        mov edx, dword ptr [esp + 4]
        mov ecx, eax
        shl ecx, 3
        sub ecx, eax
        mov eax, dword ptr [edx + 8]
        shl ecx, 4
        mov eax, dword ptr [ecx + eax + 6ch]
        and eax, 1
        ret
    }
}
#else
uint32_t indexed_record_flag6c_bit0_get(const void* record, int index)
{
    const char* base = (const char*)rec_ptr(record, 0x08);
    if (base == NULL) {
        return 0u;
    }
    const size_t off = (size_t)index * 0x70u + 0x6cu;
    return rec_dword(base + off, 0) & 1u;
}
#endif

// FUNCTION: LEGORR 0x004827a0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void indexed_record_flag6c_bit0_set(
    void* record, int index, int set)
{
    __asm {
        mov ecx, dword ptr [esp + 8]
        mov eax, ecx
        shl eax, 3
        sub eax, ecx
        mov ecx, dword ptr [esp + 4]
        shl eax, 4
        mov edx, dword ptr [ecx + 8]
        mov ecx, dword ptr [esp + 0ch]
        add eax, edx
        test ecx, ecx
        mov ecx, dword ptr [eax + 6ch]
        je short indexed_record_flag6c_bit0_clear
        or ecx, 1
        mov dword ptr [eax + 6ch], ecx
        ret
    indexed_record_flag6c_bit0_clear:
        and ecx, 0fffffffeh
        mov dword ptr [eax + 6ch], ecx
        ret
    }
}
#else
void indexed_record_flag6c_bit0_set(void* record, int index, int set)
{
    char* base = (char*)rec_ptr(record, 0x08);
    if (base == NULL) {
        return; /* follows the indexed getter's null-table convention */
    }
    const size_t off = (size_t)index * 0x70u + 0x6cu;
    uint32_t flags = rec_dword(base, off);
    flags = set != 0 ? flags | 1u : flags & ~1u;
    rec_dword_put(base, off, flags);
}
#endif

// FUNCTION: LEGORR 0x004827d0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void record_flag2c_bit0_set(void* record, int set)
{
    __asm {
        mov eax, dword ptr [esp + 8]
        test eax, eax
        mov eax, dword ptr [esp + 4]
        mov ecx, dword ptr [eax + 2ch]
        je short record_flag2c_bit0_clear
        or ecx, 1
        mov dword ptr [eax + 2ch], ecx
        ret
    record_flag2c_bit0_clear:
        and ecx, 0fffffffeh
        mov dword ptr [eax + 2ch], ecx
        ret
    }
}
#else
void record_flag2c_bit0_set(void* record, int set)
{
    if (record == NULL) {
        return;
    }
    if (set != 0) {
        rec_dword_put(record, 0x2c, rec_dword(record, 0x2c) | 1u);
    }
    else {
        rec_dword_put(record, 0x2c, rec_dword(record, 0x2c) & ~1u);
    }
}
#endif

#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
// FUNCTION: LEGORR 0x004827f0
int __cdecl scene_runtime_user_visual_manager(
    IDirect3DRMUserVisual* visual, void* cookie, SceneUserVisualReason reason,
    IDirect3DRMDevice* device, IDirect3DRMViewport* viewport)
{
    (void)visual;
    (void)device;
    return scene_user_visual_manager_dispatch(
        (EngineTaskCallbackRecord*)cookie, (uint32_t)reason, viewport);
}
#endif

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

/* RendererCommandNodePrepend (FUN_00482ce0) — see crt.hpp. */
// FUNCTION: LEGORR 0x00482ce0
void renderer_command_node_prepend(uint32_t kind, const uint32_t* payload)
{
    /* Deviation: the port lays the corpus's dword node window out with
     * the widened host-word window (the CrtOwnedStreamBufferRelease
     * convention) while the next link rides as a widened void* at the
     * corpus's byte slot +0x44 (the ModelRenderListClear precedent). */
    char* node = (char*)engine_heap_alloc(18 * (long)sizeof(void*));
    if (node == NULL) {
        return; /* deviation: the corpus strides its alloc blindly */
    }
    memset(node, 0, 18 * (size_t)sizeof(void*));
    const uint32_t zero = 0u;
    __builtin_memcpy(node + 0x00, &kind, sizeof kind);
    for (int i = 0; i < 16; ++i) {
        const uint32_t v = (payload != NULL) ? payload[i] : zero;
        __builtin_memcpy(node + 0x04 + (size_t)i * sizeof v, &v, sizeof v);
    }
    void* head = model_render_list_head_peek();
    __builtin_memcpy(node + 0x44, &head, sizeof head);
    model_render_list_head_set(node);
}
// FUNCTION: LEGORR 0x00482d30
void model_render_list_clear(void)
{
    void* node = s_model_render_list_head;
    while (node != NULL) {
        void* next = NULL;
        __builtin_memcpy(&next, (char*)node + 0x44, sizeof next);
        engine_heap_free(node);
        node = next;
    }
    s_model_render_list_head = NULL;
}

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

/* --- the cache-window name pair (see texture.hpp) ------------------------- */
/* Both corpus halves work over a caller-supplied entry window; entries
 * ride as widened slots {word, name, word}. */
// FUNCTION: LEGORR 0x00483030
void* texture_cache_name_slot_find(const void* table, uint32_t count,
                                   const char* name)
{
    /* Corpus 0x483030 gates the unsigned count once, then walks name fields
     * at 0x0c-byte strides; its bottom comparison is unsigned (jb). */
    const long field = (long)sizeof(void*);
    for (uint32_t i = 0u; i < count; ++i) {
        const char* entry_name = *(const char* const*)
            ((const char*)table + (long)i * 3 * field + field);
        if (strcasecmp_lite(name, entry_name) == 0) {
            return (void*)((const char*)table + (long)i * 3 * field);
        }
    }
    return NULL;
}

// FUNCTION: LEGORR 0x00483080
void texture_cache_name_slot_append(void* table, int* count,
                                    const char* name, void* slot_a,
                                    void* slot_b)
{
    const long field = (long)sizeof(void*);
    char* copy = string_duplicate(name); /* FUN_00477550 */
    char* entry = (char*)table + (long)*count * 3 * field;
    __builtin_memcpy(entry, &slot_a, sizeof slot_a);
    __builtin_memcpy(entry + field, &copy, sizeof copy);
    __builtin_memcpy(entry + 2 * field, &slot_b, sizeof slot_b);
    *count += 1;
}

namespace lego {
/* AnimatedTextureFrameSurfaceSet (FUN_004830c0): the frame state's slot-zero
 * method is called with the record's shared-surface selector word and the
 * indexed frame record (records + 0x60 + frame * 0x70); the portable port
 * routes that call through the installed apply hook.  The state's third
 * dword then raises or clears the 0x00400000 shared-surface bit in the
 * frame record's flag word at +0x0c. */
// FUNCTION: LEGORR 0x004830c0
void animated_texture_frame_surface_set(void* texture, int frame_index,
                                        const void* frame_state)
{
    unsigned char* record = static_cast<unsigned char*>(texture);
    const unsigned char* state =
        static_cast<const unsigned char*>(frame_state);
    unsigned char* records;
    void* interface_object = 0;
    unsigned int flags;
    unsigned char* frame_record;
    if (record == 0 || state == 0 || frame_index < 0) return;
    memcpy(&records, record + 8, sizeof(records));
    if (records == 0) return;
    frame_record = records + 0x60 + (size_t)frame_index * 0x70;
    memcpy(&interface_object, state, sizeof(interface_object));
    if (interface_object != 0 && s_texture_frame_apply_hook != 0)
        s_texture_frame_apply_hook(interface_object, frame_record,
                                   s_texture_frame_apply_context);
    memcpy(&flags, frame_record + 0x0c, sizeof(flags));
    if (((const uint32_t*)state)[2] != 0u) flags |= 0x00400000u;
    else flags &= ~0x00400000u;
    memcpy(frame_record + 0x0c, &flags, sizeof(flags));
}
} // namespace lego

namespace unported_ledger_stubs {
// IndexedRecordInterfaceRelease
// FUNCTION: LEGORR 0x00483140
void ledger_stub_00483140_IndexedRecordInterfaceRelease(void* owner, int index)
{
    unsigned char* bytes = (unsigned char*)owner;
    void* records;
    void* interface_object;
    void** vtable;
    typedef uint32_t (RECORD_COM_CALL *ReleaseMethod)(void*);
    if (bytes == 0 || index < 0) return;
    memcpy(&records, bytes + 8, sizeof(records));
    if (records == 0) return;
    bytes = (unsigned char*)records + (size_t)index * 0x70u;
    memcpy(&interface_object, bytes + 0x60, sizeof(interface_object));
    if (interface_object == 0) return;
    vtable = *(void***)interface_object;
    if (vtable != 0 && vtable[2] != 0) {
        ReleaseMethod release = (ReleaseMethod)vtable[2];
        release(interface_object);
    }
    memset(bytes + 0x60, 0, sizeof(void*));
}
} // namespace unported_ledger_stubs

// FUNCTION: LEGORR 0x00483170
uint32_t indexed_record_transform_initialize(void* record, int index)
{
    uint32_t block[20];
    const float one = 1.0f;
    memset(block, 0, sizeof block);
    block[0] = 0x50u; /* the block's size word, as the corpus writes it */
    float* f = (float*)(block + 1);
    for (int i = 0; i < 4; ++i) {
        f[i] = one; /* the identity's four unit diagonals */
    }
    void* dst = indexed_record_data_address_get(record, index);
    if (dst != NULL) {
        __builtin_memcpy(dst, block, sizeof block);
    }
    return 1u; /* the corpus answers 1 either way (deviation: guarded copy) */
}
/* IndexedRecordTransformCopyIn (FUN_00483240, findings 372/431): the
 * inline-block copy into the indexed record's data slot. Deviation: the
 * corpus strides both links and the copy blindly; the three null answers
 * are pinned by test_record.cpp, and the helper route scored best. */
// FUNCTION: LEGORR 0x00483240
uint32_t indexed_record_transform_copy_in(void* record, int index,
                                          const void* source)
{
    void* dst = indexed_record_data_address_get(record, index);
    if (dst != NULL && source != NULL) {
        __builtin_memcpy(dst, source, 0x50u);
    }
    return 1u;
}

// FUNCTION: LEGORR 0x00483270
uint32_t render_object_color_vector_set(void* record, int block, float red,
                                        float green, float blue, int slot)
{
    char* blk = mesh_block(record, block);
    char* data = (char*)indexed_record_data_address_get(record, block);
    const float r = channel_clamp(red);
    const float g = channel_clamp(green);
    const float b = channel_clamp(blue);
    if (slot == 0) {
        if (data != NULL) {
            rec_float_put(data, 0x04, r);
            rec_float_put(data, 0x08, g);
            rec_float_put(data, 0x0c, b);
        }
        if (blk != NULL) {
            const uint32_t flags = rec_dword(blk, 0x6c);
            rec_dword_put(blk, 0x6c,
                          (r == 1.0f && g == 1.0f && b == 1.0f)
                              ? (flags | 0x100000u)
                              : (flags & 0xffefffffu));
        }
        return 1u;
    }
    if (slot == 1 || slot == 2 || slot == 3) {
        const size_t off = (slot == 1) ? 0x14u : ((slot == 2) ? 0x24u : 0x34u);
        if (data != NULL) { /* the corpus stores x first, then y, then z */
            rec_float_put(data, off + 0x00, r);
            rec_float_put(data, off + 0x04, g);
            rec_float_put(data, off + 0x08, b);
        }
        return 1u;
    }
    return 1u; /* the corpus leaves every other slot untouched */
}
/* IndexedRecordDataAddressGet (FUN_00483400) is the retail unchecked
 * address calculation. Keep null handling in the portable accessor below. */
// FUNCTION: LEGORR 0x00483400
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void* indexed_record_data_address_get_target(
    const void* record, int index)
{
    __asm {
        mov eax, dword ptr [esp + 8]
        mov edx, dword ptr [esp + 4]
        mov ecx, eax
        shl ecx, 3
        sub ecx, eax
        mov eax, dword ptr [edx + 8]
        shl ecx, 4
        lea eax, dword ptr [eax + ecx + 10h]
        ret
    }
}
#else
void* indexed_record_data_address_get_target(const void* record,
                                                     int index)
{
    char* base = (char*)*(void* const*)((const char*)record + 0x08);
    return base + index * 0x70 + 0x10;
}
#endif

// FUNCTION: LEGORR 0x00483420
uint32_t screen_effect_item_opacity_set_target(void* record, int block,
                                                      float opacity, int mode)
{
    char* const base = *(char**)((char*)record + 0x08);
    char* const blk = base + (size_t)block * 0x70u;
    char* const data = (char*)indexed_record_data_address_get_target(record,
                                                                     block);
    float v = opacity;
    uint32_t flags;
    uint32_t field64;
    uint32_t field18;
    if (v < 0.0f) v = 0.0f;
    if (1.0f < v) v = 1.0f;
    if (mode != 4) {
        if (mode == 5) *(float*)(data + 0x44) = v;
        return 1u;
    }
    field64 = *(uint32_t*)(blk + 0x64);
    field18 = *(uint32_t*)((char*)record + 0x18);
    flags = *(uint32_t*)(blk + 0x6c) & 0xeff7ffffu;
    *(uint32_t*)(blk + 0x6c) = flags;
    if (v == 0.0f) {
        if (field64 == 0u && (field18 & 0x1c00u) == 0u) goto screen_opacity_flags_done;
        flags |= 0x10000000u;
    } else {
        if (v == 1.0f ||
            (field64 == 0u && (field18 & 0x1c00u) == 0u)) {
            goto screen_opacity_flags_done;
        }
        flags |= 0x80000u;
    }
    *(uint32_t*)(blk + 0x6c) = flags;
screen_opacity_flags_done:
    if (field64 != 0u) {
        if ((field64 & 0xc00u) != 0u) {
            *(uint32_t*)(blk + 0x6c) |= 0x80000u;
        }
        *(float*)(data + 0x10) = v;
        return 1u;
    }
    if ((field18 & 0xc00u) != 0u) {
        *(uint32_t*)(blk + 0x6c) |= 0x80000u;
    }
    *(float*)(data + 0x10) = v;
    return 1u;
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

namespace unported_ledger_stubs {
// FUNCTION: LEGORR 0x00483810
int ledger_stub_00483810_InputActionStateToggle(unsigned int action,
                                                 const void* data)
{
    return input_action_state_toggle(&s_input_action_toggle_state, NULL,
                                     action, data);
}
} // namespace unported_ledger_stubs

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

/* ModelChildRenderEligibilityCheck (FUN_00483a70): the two-bit render
 * gate (finding 431). NULL answers "not eligible" — the corpus would
 * crash on it (documented deviation). */
// FUNCTION: LEGORR 0x00483a70
int model_child_render_eligibility_check(const void* object)
{
    if (object == NULL) {
        return 0;
    }
    const uint32_t flags = *(const uint32_t*)((const char*)object + 0x6c);
    if ((flags & 1u) == 0u && (flags & 0x10000000u) == 0u) {
        return 1;
    }
    return 0;
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

static void rec_float_put(void* base, size_t off, float v)
{
    __builtin_memcpy((char*)base + off, &v, sizeof v);
}

static char* mesh_block(const void* record, int block)
{
    char* table = (char*)rec_ptr(record, 0x08);
    if (table == NULL) {
        return NULL; /* deviation: the corpus strides a missing table blindly */
    }
    return table + (size_t)block * 0x70;
}

static float channel_clamp(float v)
{
    if (v < 0.0f) {
        return 0.0f;
    }
    if (1.0f < v) {
        return 1.0f;
    }
    return v; /* corpus shape: a NaN channel rides through untouched */
}

static void* mesh_record_default_allocate(void*, size_t bytes)
{
    return engine_heap_alloc((long)bytes);
}

static void mesh_record_default_release(void*, void* memory)
{
    engine_heap_free(memory);
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
