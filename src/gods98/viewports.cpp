// SPDX-License-Identifier: LGPL-3.0-or-later
/* G98-003 Viewports.c: the unit's functions in original address order and the
 * state they own, relocated verbatim by tools/relocate_unit.py (ADR 0006).
 * Body hashes: docs/coverage/relocations/G98-003.csv. */
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
#include "view_for_com_object_tree.hpp"
#include "game/util.hpp"
#include <limits.h>
#include "platform/platform.h"
#include "engine.hpp"
#include "frameloop.hpp"
#include "view_for_viewports.hpp"
#include "scene_resources_for_viewports.hpp"
#include "surface_for_viewports.hpp"

typedef struct DDiSceneDrawEndRecord {
    uint32_t prefix[4];
    void* render_interface;
    float pending_value;
    uint32_t busy;
} DDiSceneDrawEndRecord;

typedef float (VIEW_COM_CALL *DDiSceneGetCurrentValue)(void* interface_object);
typedef void (VIEW_COM_CALL *DDiSceneSetCurrentValue)(void* interface_object,
                                                      float value);
typedef int (VIEW_COM_CALL *DDiSceneCommitValue)(void* interface_object,
                                                  void* value);

class ResourceSlot40Interface
{
public:
    virtual void slot_00(void) = 0;
    virtual void slot_04(void) = 0;
    virtual void slot_08(void) = 0;
    virtual void slot_0c(void) = 0;
    virtual void slot_10(void) = 0;
    virtual void slot_14(void) = 0;
    virtual void slot_18(void) = 0;
    virtual void slot_1c(void) = 0;
    virtual void slot_20(void) = 0;
    virtual void slot_24(void) = 0;
    virtual void slot_28(void) = 0;
    virtual void slot_2c(void) = 0;
    virtual void slot_30(void) = 0;
    virtual void slot_34(void) = 0;
    virtual void slot_38(void) = 0;
    virtual void slot_3c(void) = 0;
    virtual void slot_40(uint32_t value) = 0;
};

class ResourceSlot88Interface
{
public:
    virtual void slot_00(void) = 0;
    virtual void slot_04(void) = 0;
    virtual void slot_08(void) = 0;
    virtual void slot_0c(void) = 0;
    virtual void slot_10(void) = 0;
    virtual void slot_14(void) = 0;
    virtual void slot_18(void) = 0;
    virtual void slot_1c(void) = 0;
    virtual void slot_20(void) = 0;
    virtual void slot_24(void) = 0;
    virtual void slot_28(void) = 0;
    virtual void slot_2c(void) = 0;
    virtual void slot_30(void) = 0;
    virtual void slot_34(void) = 0;
    virtual void slot_38(void) = 0;
    virtual void slot_3c(void) = 0;
    virtual void slot_40(void) = 0;
    virtual void slot_44(void) = 0;
    virtual void slot_48(void) = 0;
    virtual void slot_4c(void) = 0;
    virtual void slot_50(void) = 0;
    virtual void slot_54(void) = 0;
    virtual void slot_58(void) = 0;
    virtual void slot_5c(void) = 0;
    virtual void slot_60(void) = 0;
    virtual void slot_64(void) = 0;
    virtual void slot_68(void) = 0;
    virtual void slot_6c(void) = 0;
    virtual void slot_70(void) = 0;
    virtual void slot_74(void) = 0;
    virtual void slot_78(void) = 0;
    virtual void slot_7c(void) = 0;
    virtual void slot_80(void) = 0;
    virtual void slot_84(void) = 0;
    virtual void slot_88(void) = 0;
};

class ResourceRetainInterface
{
public:
    virtual void slot_00(void) = 0;
    virtual void retain(void) = 0;
};

/* Defined below (forward declarations added by tools/relocate_unit.py). */
namespace lego { static void resource_interface_call_noargs(void* wrapper, unsigned int byte_offset); }
static size_t render_surface_pool_stride(void);

namespace lego {
// FUNCTION: LEGORR 0x00476d50
void scene_resource_slots_initialize()
{
    memset(s_render_surface_pool_blocks, 0,
           sizeof(s_render_surface_pool_blocks));
    s_render_surface_pool_free_head = 0;
    s_render_surface_pool_level_count = 0;
    s_slots_live = 1;
}
// FUNCTION: LEGORR 0x00476d80
void scene_resource_slots_release_all()
{
    unsigned int i;
    scene_resource_tracked_references_release_all();
    for (i = 0; i < SCENE_RESOURCE_SLOT_COUNT; ++i) {
        if (s_render_surface_pool_blocks[i] != 0)
            engine_heap_free(s_render_surface_pool_blocks[i]);
    }
    s_render_surface_pool_free_head = 0;
    s_slots_live = 0;
    free(s_interface_forward_sync_items);
    s_interface_forward_sync_items = NULL;
    s_interface_forward_sync_items_capacity = 0u;
}
} // namespace lego

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

namespace unported_ledger_stubs {
// FUNCTION: LEGORR 0x00476e50
ScreenBoundedSurfaceRecord*
ledger_stub_00476e50_ScreenBoundedSurfaceResourceAllocate(
    uint32_t width, uint32_t height, void* camera, int x, int y)
{
    return screen_bounded_surface_allocate(width, height, camera, x, y);
}
} // namespace unported_ledger_stubs

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

namespace lego {
// FUNCTION: LEGORR 0x00476f50
void resource_interface_call_slot48(void* wrapper, void* value)
{
    void* interface_object;
    void** vtable;
    typedef void (SCENE_RESOURCE_COM_CALL *Method)(void*, void*);
    if (wrapper == 0) return;
    memcpy(&interface_object, static_cast<unsigned char*>(wrapper) + 0x10, sizeof(void*));
    if (interface_object == 0) return;
    vtable = *static_cast<void***>(interface_object);
    if (vtable != 0 && vtable[0x48 / 4] != 0) {
        Method method = reinterpret_cast<Method>(vtable[0x48 / 4]);
        method(interface_object, value);
    }
}
} // namespace lego

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

// FUNCTION: LEGORR 0x00476fb0
void renderer_object_set_clip_distance(void* object, float distance)
{
    SceneRootInterface* const scene =
        (SceneRootInterface*)*(void**)((char*)object + 0x10);
    scene->clip_distance_set(distance);
}

namespace lego {
// FUNCTION: LEGORR 0x00476fd0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) float resource_interface_call_slot8c(void* wrapper)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        mov eax, dword ptr [eax + 10h]
        push eax
        mov ecx, dword ptr [eax]
        call dword ptr [ecx + 8ch]
        ret
    }
}
#else
float resource_interface_call_slot8c(void* wrapper)
{
    /* The viewport clip getter's value is not modelled natively. */
    resource_interface_call_noargs(wrapper, 0x8c);
    return 0.0f;
}
#endif
// FUNCTION: LEGORR 0x00476ff0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) float resource_interface_call_slot90(void* wrapper)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        mov eax, dword ptr [eax + 10h]
        push eax
        mov ecx, dword ptr [eax]
        call dword ptr [ecx + 90h]
        ret
    }
}
#else
float resource_interface_call_slot90(void* wrapper)
{
    /* The viewport clip getter's value is not modelled natively. */
    resource_interface_call_noargs(wrapper, 0x90);
    return 0.0f;
}
#endif
} // namespace lego

namespace unported_ledger_stubs {
// DDiSceneDrawBegin
// FUNCTION: LEGORR 0x00477010
void ledger_stub_00477010_DDiSceneDrawBegin(void* render_object, int begin)
{
    ddiscene_draw_begin(render_object, begin);
}
} // namespace unported_ledger_stubs

/* DDiSceneDrawEnd (FUN_00477150): guard the draw record while it eases a
 * pending value through interface slots +0x88/+0x40, commits the caller's
 * referenced value through +0x34, then runs the shared model pass. The
 * recovered call sites pass a 32-bit-record pointer, its commit-source
 * address, and a frame delta; the record layout below widens the interface
 * pointer on 64-bit portable builds while keeping the target's field order. */
// FUNCTION: LEGORR 0x00477150
void ddiscene_draw_end(void* render_object, const void* commit_source,
                       float frame_delta)
{
    DDiSceneDrawEndRecord* record =
        (DDiSceneDrawEndRecord*)render_object;
    void* interface_object;
    void** vtable;
    void* commit_value = NULL;
    int result;
    if (record == NULL) return;
    record->busy = 1u;
    interface_object = record->render_interface;
    if (interface_object == NULL || commit_source == NULL) {
        record->busy = 0u;
        return;
    }
    vtable = *(void***)interface_object;
    if (vtable == NULL || vtable[0x88 / 4] == NULL ||
        vtable[0x40 / 4] == NULL || vtable[0x34 / 4] == NULL) {
        record->busy = 0u;
        return;
    }
    memcpy(&commit_value, commit_source, sizeof(commit_value));
    if (record->pending_value != 0.0f) {
        const float current =
            ((DDiSceneGetCurrentValue)vtable[0x88 / 4])(interface_object);
        float blend;
        float value;
        if (frame_delta == 0.0f) blend = 1.0f;
        else blend = 4.0f / frame_delta;
        value = (current * blend + record->pending_value) / (blend + 1.0f);
        ((DDiSceneSetCurrentValue)vtable[0x40 / 4])(
            interface_object, value);
        if (value == record->pending_value) record->pending_value = 0.0f;
    }
    result = ((DDiSceneCommitValue)vtable[0x34 / 4])(
        interface_object, commit_value);
    if (result == 0) {
        model_render_pass_dispatch(render_object);
    }
    else {
        static const char source_file[] =
            "C:\\Dev\\SourceSafe\\gods98\\dx6\\god.cpp";
        (void)unported_ledger_stubs::directx_result_diagnostic_dispatch(
            result, 1, source_file, 0x11e);
    }
    record->busy = 0u;
}

// FUNCTION: LEGORR 0x00477220
void render_resource_track_and_retain(void* resource)
{
    void** next;
    ResourceRetainInterface* interface_object;
    if (resource == NULL) {
        return;
    }
    next = (void**)((unsigned char*)resource + 0x1c);
    *next = s_render_surface_pool_free_head;
    s_render_surface_pool_free_head = resource;
    interface_object = *(ResourceRetainInterface**)((unsigned char*)resource + 0x10);
    interface_object->retain();
}

/* SceneCurrentObjectSet (FUN_00477240) — see record.hpp. The store lands
 * on the record's +0x14 slot; Deviation: the corpus strides a dword, the
 * port writes the module's widened pointer slot. */
// FUNCTION: LEGORR 0x00477240
void scene_current_object_set(void* record, void* object)
{
    if (record != NULL) {
        __builtin_memcpy((char*)record + 0x14, &object, sizeof object);
    }
}

// FUNCTION: LEGORR 0x00477250
void resource_interface_call_slot40_and_clear_field14(
    ResourceInterfaceWrapper* wrapper, uint32_t value)
{
    if (wrapper == NULL) {
        return;
    }
    if (wrapper->interface_object != NULL) {
        ((ResourceSlot40Interface*)wrapper->interface_object)->slot_40(value);
    }
    wrapper->field14 = 0;
}
// FUNCTION: LEGORR 0x00477270
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void resource_interface_call_slot88(void* wrapper)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        mov eax, dword ptr [eax + 10h]
        push eax
        mov ecx, dword ptr [eax]
        call dword ptr [ecx + 88h]
        ret
    }
}
#else
void resource_interface_call_slot88(void* wrapper)
{
    ResourceSlot88Interface* interface_object =
        *(ResourceSlot88Interface**)((unsigned char*)wrapper + 0x10);
    interface_object->slot_88();
}
#endif

// FUNCTION: LEGORR 0x00477290
void camera_screen_coordinate_project(const ViewProjectionAdapter* adapter,
                                      float* output, const float* input)
{
    if (adapter == NULL || adapter->screen_project == NULL || output == NULL ||
        input == NULL) return;
    adapter->screen_project(adapter->context, output, input);
}

// FUNCTION: LEGORR 0x004772b0
void render_interface_transform_point(void* object, void* output,
                                      const void* input)
{
    SceneRootInterface* const scene =
        (SceneRootInterface*)*(void**)((char*)object + 0x10);
    scene->transform_point(output, input);
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
/* ComPropertyResultGet (FUN_00477310): initializes a two-dword result on its
 * stack, calls interface slots +0x64 and +0x68, and releases the temporary
 * interface through the record's +0x08 callback. The retail body returns the
 * address of that local result; callers consume it synchronously in the draw
 * path. */
// FUNCTION: LEGORR 0x00477310
void* com_property_result_get(void* resource_owner)
{
#if defined(_MSC_VER)
#define SCENE_COM_STDCALL __stdcall
#elif defined(__i386__) && (defined(__GNUC__) || defined(__clang__))
#define SCENE_COM_STDCALL __attribute__((stdcall))
#else
#define SCENE_COM_STDCALL
#endif
    typedef void (SCENE_COM_STDCALL *ComMethodNoArgs)(void*);
    typedef void (SCENE_COM_STDCALL *ComRecordRelease)(void*);
    void* resource_interface = NULL;
    void** vtable;
    uint32_t result[2];
    result[0] = 0u;
    result[1] = 0u;
    if (resource_owner != NULL) {
        memcpy(&resource_interface, (char*)resource_owner + 0x10,
               sizeof(resource_interface));
    }
    if (resource_interface != NULL) {
        ComRecordRelease release_record;
        vtable = *(void***)resource_interface;
        if (vtable != NULL) {
            if (vtable[0x64 / 4] != NULL) {
                ((ComMethodNoArgs)vtable[0x64 / 4])(resource_interface);
            }
            if (vtable[0x08 / 4] != NULL) {
                ((ComMethodNoArgs)vtable[0x08 / 4])(resource_interface);
            }
            if (vtable[0x68 / 4] != NULL) {
                ((ComMethodNoArgs)vtable[0x68 / 4])(resource_interface);
            }
        }
        memcpy(&release_record, (char*)resource_interface + 0x08,
               sizeof(release_record));
        if (release_record != NULL)
            release_record(&resource_interface);
    }
#undef SCENE_COM_STDCALL
    return result;
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

namespace lego {
// FUNCTION: LEGORR 0x004773e0
void scene_resource_tracked_references_release_all()
{
    unsigned int pool;
    typedef uint32_t (SCENE_RESOURCE_COM_CALL *ReleaseMethod)(void*);
    for (pool = 0; pool < ddiscene_render_surface_pool_block_count(); ++pool) {
        unsigned char* records = static_cast<unsigned char*>(
            ddiscene_render_surface_pool_block_get(pool));
        unsigned int index;
        unsigned int record_count = 1u << pool;
        if (records == 0) continue;
        for (index = 0; index < record_count; ++index) {
            size_t stride = (size_t)0x20u + sizeof(void*) - sizeof(uint32_t);
            unsigned char* record = records + (size_t)index * stride;
            void* self_link;
            void* interface_object;
            void** vtable;
            memcpy(&self_link, record + 0x1c, sizeof(self_link));
            if (self_link != record) continue;
            memcpy(&interface_object, record + 0x10,
                   sizeof(interface_object));
            if (interface_object == 0) continue;
            vtable = *static_cast<void***>(interface_object);
            if (vtable != 0 && vtable[2] != 0) {
                ReleaseMethod release =
                    reinterpret_cast<ReleaseMethod>(vtable[2]);
                release(interface_object);
            }
        }
    }
}
} // namespace lego

static size_t render_surface_pool_stride(void)
{
    return (size_t)0x20u + (sizeof(void*) - sizeof(uint32_t));
}

namespace lego {
#if !defined(_MSC_VER) || !defined(_M_IX86)
static void resource_interface_call_noargs(void* wrapper, unsigned int byte_offset)
{
    void* interface_object;
    void** vtable;
    typedef void (SCENE_RESOURCE_COM_CALL *Method)(void*);
    if (wrapper == 0) return;
    memcpy(&interface_object, static_cast<unsigned char*>(wrapper) + 0x10, sizeof(void*));
    if (interface_object == 0) return;
    vtable = *static_cast<void***>(interface_object);
    if (vtable != 0 && vtable[byte_offset / 4] != 0) {
        Method method = reinterpret_cast<Method>(vtable[byte_offset / 4]);
        method(interface_object);
    }
}
#endif
} // namespace lego
