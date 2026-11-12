// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — corpus record access primitives (see record.hpp). */

#include "record.hpp"
#include "heap.hpp"
#include "sound.hpp"
#include "string.hpp"
#include "view.hpp"
#include "texture.hpp"
#include "resource.hpp"
#include "crt/crt.hpp"
#include "game/util.hpp"
#include <math.h>  /* sqrtf for the instance-quad normal window */
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include "record_shared.hpp"

#if defined(_MSC_VER)
#define RECORD_COM_CALL __stdcall
#elif defined(__i386__) && (defined(__GNUC__) || defined(__clang__))
#define RECORD_COM_CALL __attribute__((stdcall))
#else
#define RECORD_COM_CALL
#endif

static void* s_large_record_free;
static void* s_large_record_blocks[32];
static unsigned int s_large_record_block_count;
// GLOBAL: LEGORR 0x00533988
void* s_small_record_free;
static void* s_small_record_blocks[32];
static unsigned int s_small_record_block_count;

static void pool_link_write(void* record, size_t offset, void* next)
{
    __builtin_memcpy((char*)record + offset, &next, sizeof(next));
}

static void* pool_link_read(void* record, size_t offset)
{
    void* next = NULL;
    __builtin_memcpy(&next, (char*)record + offset, sizeof(next));
    return next;
}

/* FUNCTION: LEGORR 0x0047a5c0 */
void fixed_size_large_record_pool_grow(void)
{
    if (s_large_record_block_count >= 32u) return;
    unsigned int count = 1u << (s_large_record_block_count & 31u);
    const size_t stride = 0xbf0u + sizeof(void*);
    void* block = engine_heap_alloc((long)(count * stride));
    if (block == NULL) return;
    s_large_record_blocks[s_large_record_block_count++] = block;
    for (unsigned int i = 0; i < count; ++i) {
        void* record = (char*)block + (size_t)i * stride;
        void* next = i + 1u < count
            ? (char*)record + stride : s_large_record_free;
        pool_link_write(record, 0xbf0u, next);
    }
    s_large_record_free = block;
}

/* FUNCTION: LEGORR 0x0047a580 */
void* fixed_size_large_record_acquire(uint32_t first_dword)
{
    if (s_large_record_free == NULL) fixed_size_large_record_pool_grow();
    void* record = s_large_record_free;
    if (record == NULL) return NULL;
    s_large_record_free = pool_link_read(record, 0xbf0u);
    memset(record, 0, 0xbf0u);
    pool_link_write(record, 0xbf0u, record);
    __builtin_memcpy(record, &first_dword, sizeof(first_dword));
    return record;
}

/* FUNCTION: LEGORR 0x0047e0c0 */
void fixed_size_small_record_pool_grow(void)
{
    if (s_small_record_block_count >= 32u) return;
    unsigned int count = 1u << (s_small_record_block_count & 31u);
    const size_t stride = 0x1cu + sizeof(void*);
    void* block = engine_heap_alloc((long)(count * stride));
    if (block == NULL) return;
    s_small_record_blocks[s_small_record_block_count++] = block;
    scene_object_registry_pool_block_register(block);
    for (unsigned int i = 0; i < count; ++i) {
        void* record = (char*)block + (size_t)i * stride;
        void* next = i + 1u < count
            ? (char*)record + stride : s_small_record_free;
        pool_link_write(record, 0x1cu, next);
    }
    s_small_record_free = block;
}

void fixed_size_small_record_pool_teardown(void)
{
    unsigned int i;
    for (i = 0u; i < s_small_record_block_count; ++i) {
        engine_heap_free(s_small_record_blocks[i]);
        s_small_record_blocks[i] = NULL;
    }
    s_small_record_block_count = 0u;
    s_small_record_free = NULL;
}

/* Byte-safe field reads: corpus dword/float slots at offsets that may
 * not be pointer-aligned on 64-bit hosts. */
uint32_t rec_dword(const void* base, size_t off)
{
    uint32_t v = 0;
    if (base != NULL) {
        __builtin_memcpy(&v, (const char*)base + off, sizeof(v));
    }
    return v;
}

float rec_float(const void* base, size_t off)
{
    float v = 0.0f;
    if (base != NULL) {
        __builtin_memcpy(&v, (const char*)base + off, sizeof(v));
    }
    return v;
}

/* Pointer slots are modeled native-size (documented deviation from the
 * corpus's 32-bit pointers); reads stay byte-safe like the fields. */
void* rec_ptr(const void* base, size_t off)
{
    void* v = NULL;
    if (base != NULL) {
        __builtin_memcpy(&v, (const char*)base + off, sizeof(v));
    }
    return v;
}

/* Defined with the mesh-window band below; used by the visibility setter. */
void rec_dword_put(void* base, size_t off, uint32_t v);

/* Render-resource instance records have their free-list link at +0x30.
 * Include the native pointer width in the stride so x86 keeps the corpus's
 * 0x34-byte records. */
#define RENDER_RESOURCE_INSTANCE_RECORD_SIZE (0x30u + sizeof(void*))
#define RENDER_RESOURCE_INSTANCE_LINK_OFFSET 0x30
// GLOBAL: LEGORR 0x005405d8
static uint32_t s_render_resource_instance_pool_count;
// GLOBAL: LEGORR 0x00540584
static void* s_render_resource_instance_pool_blocks[20];
// GLOBAL: LEGORR 0x005405d4
static void* s_render_resource_instance_pool_head;

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

/* SharedTextureRootRegister clears DAT_00540584/D4/D8 after dropping the
 * model-render list. Keep that owner state in this pool module instead of
 * mirroring it as a synthetic twenty-resource texture table. The retail
 * reset drops the block references without freeing them. */
void render_resource_instance_pool_reset(void)
{
    memset(s_render_resource_instance_pool_blocks, 0,
           sizeof(s_render_resource_instance_pool_blocks));
    s_render_resource_instance_pool_head = NULL;
    s_render_resource_instance_pool_count = 0;
}

/* EntityFlag80Set (FUN_00473b20) — see record.hpp. */
// FUNCTION: LEGORR 0x00473b20
void entity_flag_80_set_corpus(void* record, int set)
{
    uint32_t* word = (uint32_t*)((char*)record + 0x14);
    if (set != 0) {
        *word |= 0x80u;
    }
    else {
        *word &= ~0x80u;
    }
}

void entity_flag_80_set(void* record, int set)
{
    if (record == NULL) {
        return; /* deviation: the corpus strides the record blindly */
    }
    entity_flag_80_set_corpus(record, set);
}

/* CategoryValueIsNeitherZeroNorFifteen (FUN_0041b470). */
// FUNCTION: LEGORR 0x0041b470
int category_value_is_neither_zero_nor_fifteen(int value)
{
    if (value != 0 && value != 0xf) {
        return 1;
    }
    return 0;
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

uint32_t entity_type_flag_bit1_get(const void* record)
{
    if (record == NULL) return 0u;
    return entity_type_flag_bit1_get_target(record);
}

/* ------------------------------------------------------------------ *
 *  Field-reader sweep (see record.hpp). */

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

// FUNCTION: LEGORR 0x00473db0
uint32_t scene_object_owner_get(const void* scene_object)
{
    uint32_t owner;
    __builtin_memcpy(&owner, (const char*)scene_object + 0x20,
                     sizeof(owner));
    return owner;
}

// FUNCTION: LEGORR 0x00473680
uint32_t scene_object_wrapped_pointer_get(const void* wrapped)
{
    uint32_t value;
    __builtin_memcpy(&value, wrapped, sizeof(value));
    return value;
}

/* The mapped SceneObjectFlagBit3Get body follows the corpus's unchecked
 * +0x14 load; animation_object_visibility_get retains its null-safe path. */
// FUNCTION: LEGORR 0x00473ba0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) uint32_t scene_object_flag_bit3_get_target(
    const void* scene_object)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        mov eax, dword ptr [eax + 14h]
        and eax, 0ffh
        shr eax, 3
        and eax, 1
        ret
    }
}
#else
uint32_t scene_object_flag_bit3_get_target(const void* scene_object)
{
    return (*(const uint32_t*)((const char*)scene_object + 0x14) & 0xffu)
           >> 3 & 1u;
}
#endif

uint32_t scene_object_flag_bit3_get(const void* scene_object)
{
    if (scene_object == NULL) return 0u;
    return scene_object_flag_bit3_get_target(scene_object);
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

/* IndexedRecordDataAddressGet (FUN_00483400) is the retail unchecked
 * address calculation. Keep null handling in the portable accessor below. */
// FUNCTION: LEGORR 0x00483400
#if defined(_MSC_VER) && defined(_M_IX86)
static __declspec(naked) void* indexed_record_data_address_get_target(
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
static void* indexed_record_data_address_get_target(const void* record,
                                                     int index)
{
    char* base = (char*)*(void* const*)((const char*)record + 0x08);
    return base + index * 0x70 + 0x10;
}
#endif

void* indexed_record_data_address_get(const void* record, int index)
{
    void* base;
    if (record == NULL) return NULL;
    base = *(void* const*)((const char*)record + 0x08);
    if (base == NULL) return NULL;
    return indexed_record_data_address_get_target(record, index);
}

/* AviPlayerReadyGet (FUN_0047ed80, findings 185/414): the readiness
 * byte at +0x99 (byte-safe — the corpus reads it as a raw byte). */
// FUNCTION: LEGORR 0x0047ed80
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) uint32_t avi_player_ready_get(const void* movie)
{
    __asm {
        mov ecx, dword ptr [esp + 4]
        xor eax, eax
        mov al, byte ptr [ecx + 99h]
        ret
    }
}
#else
uint32_t avi_player_ready_get(const void* movie)
{
    if (movie != NULL) {
        uint8_t v = 0;
        __builtin_memcpy(&v, (const char*)movie + 0x99, sizeof(v));
        return v;
    }
    return 0u;
}
#endif

/* GameFileSystemTypeGet (FUN_0047fe20, findings 234/404): the handle's
 * leading type dword. */
// FUNCTION: LEGORR 0x0047fe20
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) uint32_t game_file_system_type_get(const void* handle)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        mov eax, dword ptr [eax]
        ret
    }
}
#else
uint32_t game_file_system_type_get(const void* handle)
{
    if (handle == NULL) {
        return 0u; /* pinned deviation: the corpus loads blindly */
    }
    return *(const uint32_t*)handle;
}
#endif

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

/* --- grid-descriptor coordinate leaves (see record.hpp) ------------------ */

/* --- indexed flag slot and resource-handle writers (see record.hpp) ------ */

/* CopyTwoDwordPairToOutputs (FUN_00439930) — see record.hpp. */
// FUNCTION: LEGORR 0x00439930
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void copy_two_dword_pair_to_outputs(
    const void* record, uint32_t* out_a, uint32_t* out_b)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        mov edx, dword ptr [esp + 8]
        mov ecx, dword ptr [eax]
        mov dword ptr [edx], ecx
        mov ecx, dword ptr [esp + 0ch]
        mov eax, dword ptr [eax + 4]
        mov dword ptr [ecx], eax
        ret
    }
}
#else
void copy_two_dword_pair_to_outputs(const void* record, uint32_t* out_a,
                                    uint32_t* out_b)
{
    if (record == NULL || out_a == NULL || out_b == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    __builtin_memcpy(out_a, (const char*)record, 4);
    __builtin_memcpy(out_b, (const char*)record + 4, 4);
}
#endif

// FUNCTION: LEGORR 0x00473da0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void scene_object_owner_set(void* scene_object,
                                              uint32_t owner)
{
    __asm {
        mov ecx, dword ptr [esp + 4]
        mov eax, dword ptr [esp + 8]
        mov dword ptr [ecx + 20h], eax
        ret
    }
}
#else
void scene_object_owner_set(void* scene_object, uint32_t owner)
{
    if (scene_object == NULL) {
        return; /* deviation: the corpus writes blindly */
    }
    __builtin_memcpy((char*)scene_object + 0x20, &owner, sizeof(owner));
}
#endif

/* --- the record primitive band (see record.hpp) ----------------------------- */
// FUNCTION: LEGORR 0x00482350
#if defined(_MSC_VER) && defined(_M_IX86)
static __declspec(naked) void indexed_record_field64_set_target(
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
static void indexed_record_field64_set_target(void* record, int index,
                                               uint32_t value)
{
    char* base = *(char**)((char*)record + 8);
    *(uint32_t*)(base + index * 0x70 + 100) = value;
}
#endif

void indexed_record_field64_set(void* record, int index, uint32_t value)
{
    void* base;
    if (record == NULL) return;
    __builtin_memcpy(&base, (char*)record + 8, sizeof(base));
    if (base == NULL) {
        return;
    }
    indexed_record_field64_set_target(record, index, value);
}

/* Preserve the test-pinned null-record behavior of the portable API. */
void object_state_pair_get(const void* record, uint32_t* out_a,
                           uint32_t* out_b)
{
    if (record != NULL) object_state_pair_get_target(record, out_a, out_b);
}

/* --- the scene-visibility wrapper band (see record.hpp) ---------------------
 * The wrappers route through the render-lock toggle. Adapter paths record
 * their calls at the seam; directly mapped target wrappers retain the retail
 * call sequence. */
static int s_scene_vis_calls;
static void* s_scene_vis_last_obj;
static int s_scene_vis_last_flag;

static void scene_visibility_observation_record(void* object, int flag)
{
    ++s_scene_vis_calls;
    s_scene_vis_last_obj = object;
    s_scene_vis_last_flag = flag;
}

void scene_visibility_hook_call(void* object, int flag)
{
    scene_visibility_observation_record(object, flag);
    scene_render_lock_toggle(object, flag);
}

void scene_visibility_hook_reset(void)
{
    s_scene_vis_calls = 0;
    s_scene_vis_last_obj = NULL;
    s_scene_vis_last_flag = 0;
}

int scene_visibility_calls(void) { return s_scene_vis_calls; }

void scene_visibility_last_call(void** out_object, int* out_flag)
{
    if (out_object != NULL) {
        *out_object = s_scene_vis_last_obj;
    }
    if (out_flag != NULL) {
        *out_flag = s_scene_vis_last_flag;
    }
}

/* GraphicsOverlayVisibilitySet (FUN_004634b0) — the overlay words
 * DAT_00552180/DAT_0055218c (handle, cache) have no ported owner; they land
 * here as zero-init statics (deviation: corpus strides them blindly). */
// GLOBAL: LEGORR 0x00552180
uint32_t s_overlay_word;  /* DAT_00552180 role */
// GLOBAL: LEGORR 0x0055218c
uint32_t s_overlay_cache; /* DAT_0055218c role */

void graphics_overlay_word_set(uint32_t handle_word)
{
    if (s_overlay_word != handle_word) s_overlay_cache = 0u;
    s_overlay_word = handle_word;
}

uint32_t graphics_overlay_word_peek(void) { return s_overlay_word; }

uint32_t s_indexed_surface_width;
uint32_t s_indexed_surface_entry_count;
const void* const* s_indexed_surface_entries;
uint32_t s_indexed_surface_hidden_count;

void indexed_surface_grid_install(uint32_t width, const void* const* entries,
                                  uint32_t entry_count)
{
    s_indexed_surface_width = width;
    s_indexed_surface_entries = entries;
    s_indexed_surface_entry_count = entry_count;
    s_indexed_surface_hidden_count = 0u;
}

SurfaceVisibilityAdapter s_surface_visibility_adapter;
void* s_surface_visibility_context;

int surface_visibility_adapter_install(SurfaceVisibilityAdapter adapter,
                                        void* context)
{
    if (adapter == NULL) return 0;
    if (s_surface_visibility_adapter != NULL &&
        (s_surface_visibility_adapter != adapter ||
         s_surface_visibility_context != context)) return 0;
    s_surface_visibility_adapter = adapter;
    s_surface_visibility_context = context;
    return 1;
}

int surface_visibility_adapter_remove(SurfaceVisibilityAdapter adapter,
                                       void* context)
{
    if (adapter == NULL || s_surface_visibility_adapter != adapter ||
        s_surface_visibility_context != context) return 0;
    s_surface_visibility_adapter = NULL;
    s_surface_visibility_context = NULL;
    return 1;
}

/* SceneRenderLockToggle (FUN_00473b40): update the paired render interfaces
 * on the transition into or out of lock mode, then toggle flag bit 3. The
 * channel slots use the corpus offsets; rec_ptr preserves this port's
 * native-width pointer-slot convention. */
// FUNCTION: LEGORR 0x00473B40
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void scene_render_lock_toggle(void* scene_object,
                                                int enabled)
{
    __asm {
        mov ecx, dword ptr [esp + 8]
        push esi
        mov esi, dword ptr [esp + 8]
        mov eax, dword ptr [esi + 14h]
        and eax, 8
        test ecx, ecx
        je scene_render_lock_toggle_disable_check
        test eax, eax
        jne scene_render_lock_toggle_disable_gate
        mov eax, dword ptr [esi + 4]
        mov ecx, dword ptr [esi + 8]
        push eax
        push ecx
        call render_interface_pair_update
        mov eax, dword ptr [esi + 14h]
        add esp, 8
        or al, 8
        mov dword ptr [esi + 14h], eax
        pop esi
        ret
    scene_render_lock_toggle_disable_gate:
        test ecx, ecx
        jne scene_render_lock_toggle_done
    scene_render_lock_toggle_disable_check:
        test eax, eax
        je scene_render_lock_toggle_done
        mov edx, dword ptr [esi + 4]
        mov eax, dword ptr [esi]
        push edx
        push eax
        call render_interface_pair_update
        mov eax, dword ptr [esi + 14h]
        add esp, 8
        and al, 0f7h
        mov dword ptr [esi + 14h], eax
    scene_render_lock_toggle_done:
        pop esi
        ret
    }
}
#else
void scene_render_lock_toggle(void* scene_object, int enabled)
{
    uint32_t flags;
    if (scene_object == NULL) return;
    flags = rec_dword(scene_object, 0x14);
    if (enabled != 0) {
        if ((flags & 8u) == 0u) {
            render_interface_pair_update(rec_ptr(scene_object, 0x08u),
                                         rec_ptr(scene_object, 0x04u));
            rec_dword_put(scene_object, 0x14, flags | 8u);
        }
        return;
    }
    if ((flags & 8u) != 0u) {
        render_interface_pair_update(rec_ptr(scene_object, 0x00u),
                                     rec_ptr(scene_object, 0x04u));
        rec_dword_put(scene_object, 0x14, flags & ~8u);
    }
}
#endif

/* --- the UI record factories (see record.hpp) ------------------------------
 * Corpus record windows stride as dwords; the port lays both shapes out as
 * native field words (the crt_stream_record window convention). Field reads
 * by later band members follow this layout. */

static void record_window_put(void* rec, int field, const void* src)
{
    if (rec == NULL || src == NULL) {
        return;
    }
    __builtin_memcpy((char*)rec + (size_t)field * sizeof(void*), src,
                     sizeof(uint32_t));
}

static void record_window_put_ptr(void* rec, int field, const void* value)
{
    if (rec != NULL) {
        __builtin_memcpy((char*)rec + (size_t)field * sizeof(void*), &value,
                         sizeof(value));
    }
}

static void record_window_put_u32(void* rec, int field, uint32_t value)
{
    record_window_put(rec, field, &value);
}

/* Resource rows keep the corpus's 0x20-byte shape on x86. On a 64-bit
 * host, widen pointer fields and the row stride so none of the handles are
 * truncated. Menu resource producers must use this same layout. */
#if UINTPTR_MAX == UINT32_MAX
typedef struct MenuResourceEntry {
    uint32_t flags;
    void* owned_string;
    TextureSurface* textures[3];
    uint32_t reserved[3];
} MenuResourceEntry;
#else
typedef struct MenuResourceEntry {
    uint32_t flags;
    uint32_t reserved0;
    void* owned_string;
    TextureSurface* textures[3];
} MenuResourceEntry;
#endif

static void menu_texture_surface_release(TextureSurface* surface)
{
    texture_release(surface);
}

static void menu_resource_entry_release(MenuResourceEntry* entry)
{
    if ((entry->flags & 1u) != 0) {
        engine_heap_free(entry->owned_string);
    } else if ((entry->flags & 2u) != 0) {
        for (int i = 0; i < 3; ++i) {
            menu_texture_surface_release(entry->textures[i]);
        }
    }
}

// FUNCTION: LEGORR 0x004105a0
char* campaign_level_fallback_label(const char* label)
{
    if (label == NULL) {
        return NULL;
    }
    /* Corpus strdup idiom: engine_heap_alloc(strlen+1) then the inlined
     * strcpy intrinsic (rep scas + rep movs). The port's 0x7fffffff
     * length bound was a deviation — the corpus bounds nothing (audited:
     * no test and both callers pass ordinary config labels). */
    char* copy = (char*)engine_heap_alloc((long)strlen(label) + 1);
    if (copy == NULL) {
        return NULL;
    }
    strcpy(copy, label);
    return copy;
}

// FUNCTION: LEGORR 0x00410a10
#if defined(LEGORRR_BACKEND_WINAPI)
__declspec(naked) void* menu_item_choice_strings_allocate(
    uint32_t count, uint32_t f3, uint32_t f4, uint32_t f5, uint32_t f6)
{
    __asm {
        push ebx
        push ebp
        push esi
        push edi
        push 0x1c
        call malloc
        mov esi, eax
        add esp, 4
        test esi, esi
        jne have_record
        pop edi
        pop esi
        pop ebp
        pop ebx
        ret
    have_record:
        mov ebp, dword ptr [esp + 0x14]
        lea ebx, [ebp * 4]
        push ebx
        call malloc
        mov edi, eax
        add esp, 4
        test edi, edi
        mov dword ptr [esi], edi
        jne have_strings
        push esi
        call resource_handle_list_dispose
        add esp, 4
        xor eax, eax
        pop edi
        pop esi
        pop ebp
        pop ebx
        ret
    have_strings:
        mov ecx, ebx
        xor eax, eax
        mov edx, ecx
        shr ecx, 2
        rep stosd
        mov ecx, edx
        mov edx, dword ptr [esp + 0x20]
        and ecx, 3
        rep stosb
        mov eax, dword ptr [esp + 0x18]
        mov ecx, dword ptr [esp + 0x1c]
        mov dword ptr [esi + 0x0c], eax
        mov eax, dword ptr [esp + 0x24]
        mov dword ptr [esi + 0x18], eax
        mov dword ptr [esi + 4], ebp
        mov dword ptr [esi + 8], 0
        mov dword ptr [esi + 0x10], ecx
        mov dword ptr [esi + 0x14], edx
        mov eax, esi
        pop edi
        pop esi
        pop ebp
        pop ebx
        ret
    }
}
#else
void* menu_item_choice_strings_allocate(uint32_t count, uint32_t f3,
                                        uint32_t f4, uint32_t f5,
                                        uint32_t f6)
{
    if ((size_t)count > (size_t)LONG_MAX / sizeof(void*)) return NULL;
    void* rec = engine_heap_alloc(7 * sizeof(void*));
    if (rec == NULL) return NULL;
    memset(rec, 0, 7 * sizeof(void*));
    void* strings = engine_heap_alloc((long)((size_t)count * sizeof(void*)));
    if (strings == NULL) {
        engine_heap_free(rec);
        return NULL;
    }
    memset(strings, 0, (size_t)count * sizeof(void*));
    *(void**)((char*)rec + 0 * sizeof(void*)) = strings;
    *(uint32_t*)((char*)rec + 1 * sizeof(void*)) = count;
    *(uint32_t*)((char*)rec + 2 * sizeof(void*)) = 0;
    *(uint32_t*)((char*)rec + 3 * sizeof(void*)) = f3;
    *(uint32_t*)((char*)rec + 4 * sizeof(void*)) = f4;
    *(uint32_t*)((char*)rec + 5 * sizeof(void*)) = f5;
    *(uint32_t*)((char*)rec + 6 * sizeof(void*)) = f6;
    return rec;
}
#endif

// FUNCTION: LEGORR 0x004105f0
#if defined(LEGORRR_BACKEND_WINAPI)
__declspec(naked) void* menu_param_record_create(
    uint32_t mode, const char* first, const char* second, uint32_t f4,
    uint32_t f5, uint32_t f6, uint32_t f7, uint32_t f8, uint32_t f9,
    uint32_t f10, uint32_t f11, uint32_t f12, uint32_t f13, uint32_t f14)
{
    __asm {
        push esi
        push edi
        push 0x5c
        call malloc
        mov esi, eax
        mov ecx, 0x17
        xor eax, eax
        mov edi, esi
        add esp, 4
        rep stosd
        mov eax, dword ptr [esp + 0x10]
        push eax
        call campaign_level_fallback_label
        mov ecx, dword ptr [esp + 0x18]
        add esp, 4
        mov dword ptr [esi + 0x20], eax
        push ecx
        call campaign_level_fallback_label
        mov edx, dword ptr [esp + 0x10]
        mov ecx, dword ptr [esp + 0x20]
        mov dword ptr [esi + 0x24], eax
        mov eax, dword ptr [esp + 0x1c]
        mov dword ptr [esi + 0x2c], eax
        mov eax, dword ptr [esp + 0x28]
        mov dword ptr [esi + 0x28], edx
        mov edx, dword ptr [esp + 0x24]
        mov dword ptr [esi + 0x30], ecx
        mov ecx, dword ptr [esp + 0x2c]
        mov dword ptr [esi + 0x38], eax
        mov eax, dword ptr [esp + 0x34]
        mov dword ptr [esi + 0x34], edx
        mov edx, dword ptr [esp + 0x30]
        mov dword ptr [esi + 0x3c], ecx
        mov ecx, dword ptr [esp + 0x38]
        mov dword ptr [esi + 0x44], eax
        mov eax, dword ptr [esp + 0x40]
        add esp, 4
        mov dword ptr [esi + 0x40], edx
        mov edx, dword ptr [esp + 0x38]
        mov dword ptr [esi + 0x48], ecx
        mov ecx, dword ptr [esp + 0x40]
        mov dword ptr [esi + 0x54], eax
        mov dword ptr [esi + 0x50], edx
        mov dword ptr [esi + 0x4c], 0
        mov dword ptr [esi + 0x58], ecx
        mov eax, esi
        pop edi
        pop esi
        ret
    }
}
#else
void* menu_param_record_create(uint32_t mode, const char* first,
                              const char* second, uint32_t f4,
                              uint32_t f5, uint32_t f6, uint32_t f7,
                              uint32_t f8, uint32_t f9, uint32_t f10,
                              uint32_t f11, uint32_t f12, uint32_t f13,
                              uint32_t f14)
{
    void* rec = engine_heap_alloc(23 * sizeof(void*));
    if (rec == NULL) return NULL;
    memset(rec, 0, 23 * sizeof(void*));
    char* first_copy = campaign_level_fallback_label(first);
    char* second_copy = campaign_level_fallback_label(second);
    if ((first != NULL && first_copy == NULL)
        || (second != NULL && second_copy == NULL)) {
        engine_heap_free(first_copy);
        engine_heap_free(second_copy);
        engine_heap_free(rec);
        return NULL;
    }
    char* bytes = (char*)rec;
    *(void**)(bytes + 8 * sizeof(void*)) = first_copy;
    *(void**)(bytes + 9 * sizeof(void*)) = second_copy;
    *(uint32_t*)(bytes + 10 * sizeof(void*)) = mode;
    *(uint32_t*)(bytes + 11 * sizeof(void*)) = f4;
    *(uint32_t*)(bytes + 12 * sizeof(void*)) = f5;
    *(uint32_t*)(bytes + 13 * sizeof(void*)) = f6;
    *(uint32_t*)(bytes + 14 * sizeof(void*)) = f7;
    *(uint32_t*)(bytes + 15 * sizeof(void*)) = f8;
    *(uint32_t*)(bytes + 16 * sizeof(void*)) = f9;
    *(uint32_t*)(bytes + 17 * sizeof(void*)) = f10;
    *(uint32_t*)(bytes + 18 * sizeof(void*)) = f11;
    *(uint32_t*)(bytes + 20 * sizeof(void*)) = f12;
    *(uint32_t*)(bytes + 21 * sizeof(void*)) = f13;
    *(uint32_t*)(bytes + 22 * sizeof(void*)) = f14;
    return rec;
}
#endif

// FUNCTION: LEGORR 0x00410690
#if defined(LEGORRR_BACKEND_WINAPI)
__declspec(naked) void menu_record_resources_release(void* record)
{
    __asm {
        push ebp
        push edi
        mov edi, dword ptr [esp + 0x0c]
        xor ebp, ebp
        cmp edi, ebp
        je release_done
        mov eax, dword ptr [edi + 0x20]
        push esi
        cmp eax, ebp
        push ebx
        je release_second
        push eax
        call free
        add esp, 4
    release_second:
        mov eax, dword ptr [edi + 0x24]
        cmp eax, ebp
        je release_entries
        push eax
        call free
        add esp, 4
    release_entries:
        cmp dword ptr [edi], ebp
        je release_record
        mov eax, dword ptr [edi + 0x1c]
        mov dword ptr [esp + 0x14], ebp
        cmp eax, ebp
        jle release_array
    release_entry_loop:
        mov eax, dword ptr [edi]
        lea ecx, [eax + ebp]
        mov eax, dword ptr [eax + ebp]
        test al, 1
        je release_textures_test
        mov ecx, dword ptr [ecx + 4]
        push ecx
        call free
        add esp, 4
        jmp release_entry_next
    release_textures_test:
        test al, 2
        je release_entry_next
        lea esi, [ebp + 8]
        mov ebx, 3
    release_texture_loop:
        mov edx, dword ptr [edi]
        mov eax, dword ptr [edx + esi]
        push eax
        call image_surface_release
        add esp, 4
        add esi, 4
        dec ebx
        jne release_texture_loop
    release_entry_next:
        mov eax, dword ptr [esp + 0x14]
        mov ecx, dword ptr [edi + 0x1c]
        inc eax
        add ebp, 0x20
        cmp eax, ecx
        mov dword ptr [esp + 0x14], eax
        jl release_entry_loop
    release_array:
        mov ecx, dword ptr [edi]
        push ecx
        call free
        add esp, 4
    release_record:
        push edi
        call free
        add esp, 4
        pop ebx
        pop esi
    release_done:
        pop edi
        pop ebp
        ret
    }
}
#else
void menu_record_resources_release(void* record)
{
    if (record == NULL) return;
    engine_heap_free(rec_ptr(record, 8 * sizeof(void*)));
    engine_heap_free(rec_ptr(record, 9 * sizeof(void*)));
    void* resources = rec_ptr(record, 0);
    const uint32_t count = rec_dword(record, 7 * sizeof(void*));
    if (resources != NULL) {
        for (uint32_t i = 0; i < count; ++i) {
            menu_resource_entry_release((MenuResourceEntry*)resources + i);
        }
        engine_heap_free(resources);
    }
    engine_heap_free(record);
}
#endif

/* ResourceRecordAddRef (FUN_0047dbc0): the wrapped DirectDraw object's
 * SetColorKey(DDCKEY_SRCBLT, range) equivalent, then mark flag bit 1. */
// FUNCTION: LEGORR 0x0047dbc0
void resource_record_add_ref(void* record)
{
    if (record == NULL) return;
    TextureSurface* surface = (TextureSurface*)record;
    const uint32_t low = surface->key_count > 0 ? surface->keys[0] : 0;
    const uint32_t high = surface->key_count > 1 ? surface->keys[1] : low;
    texture_source_color_key_set(surface, low, high);
    surface->resource_flags |= 2u;
}

/* LoadItemStrings (FUN_00414010): the corpus uses eight dword resource
 * pointers at byte offset 0x20. This port reads the equivalent eight
 * native-width pointer slots beginning at slot 4, preserving handles on
 * 64-bit hosts. */
// FUNCTION: LEGORR 0x00414010
void load_item_strings(uint32_t record_type, const void* record,
                       void* out_resources[8])
{
    if (out_resources == NULL) {
        return;
    }
    for (int i = 0; i < 8; ++i) {
        out_resources[i] = NULL;
    }
    if (record_type != 0x10 || record == NULL) {
        return;
    }
    for (int resource_index = 0; resource_index < 8; ++resource_index) {
        const char* key = (const char*)rec_ptr(
            record, (size_t)(4 + resource_index) * sizeof(void*));
        void* resource = string_cache_lookup_or_load(key);
        out_resources[resource_index] = resource;
        if (resource != NULL) {
            resource_record_add_ref(resource);
        }
    }
}

// FUNCTION: LEGORR 0x00411100
void* menu_section_record_create(const char* name, const char* title,
                                 uint32_t f3, uint32_t f4, uint32_t f5,
                                 uint32_t f6, uint32_t f7, uint32_t f8,
                                 uint32_t f9, const char* integer_pair)
{
    if (name == NULL || title == NULL) return NULL;
    void* rec = engine_heap_alloc(40 * sizeof(void*));
    if (rec == NULL) return NULL;
    memset(rec, 0, 40 * sizeof(void*));
    char* name_copy = campaign_level_fallback_label(name);
    char* title_copy = campaign_level_fallback_label(title);
    if (name_copy == NULL || title_copy == NULL) {
        engine_heap_free(name_copy);
        engine_heap_free(title_copy);
        engine_heap_free(rec);
        return NULL;
    }
    void* child = engine_heap_alloc(15 * sizeof(void*));
    if (child == NULL) {
        engine_heap_free(name_copy);
        engine_heap_free(title_copy);
        engine_heap_free(rec);
        return NULL;
    }
    memset(child, 0, 15 * sizeof(void*));
    record_window_put_ptr(rec, 0, name_copy);
    record_window_put_ptr(rec, 1, title_copy);
    record_window_put_u32(rec, 2, (uint32_t)strlen(name));
    record_window_put_ptr(rec, 6, child);
    record_window_put_u32(rec, 3, f3);
    record_window_put_u32(rec, 11, f4);
    record_window_put_u32(rec, 12, f5);
    record_window_put_u32(rec, 17, f6);
    record_window_put_u32(rec, 18, f7);
    record_window_put_u32(rec, 22, f8);
    record_window_put_u32(rec, 8, 15);
    record_window_put_u32(rec, 13, f9 != 0 ? 2u : 0u);
    if (integer_pair != NULL && integer_pair[0] != '\0') {
        char copy[1024];
        size_t length = strlen(integer_pair);
        if (length >= sizeof(copy)) length = sizeof(copy) - 1;
        memcpy(copy, integer_pair, length);
        copy[length] = '\0';
        char* first = copy;
        char* second = NULL;
        for (size_t i = 0; i + 1 < length; ++i) {
            if (copy[i] == ',') {
                copy[i] = '\0';
                second = copy + i + 1;
                break;
            }
        }
        if (second != NULL) {
            record_window_put_u32(rec, 19, 1);
            record_window_put_u32(rec, 20, (uint32_t)atoi(first));
            record_window_put_u32(rec, 21, (uint32_t)atoi(second));
        }
    }
    return rec;
}

// FUNCTION: LEGORR 0x00410c70
void resource_variant_record_release(void* record)
{
    if (record == NULL) return;
    engine_heap_free(rec_ptr(record, 0));
    const uint32_t kind = rec_dword(record, 5 * sizeof(void*));
    void* payload = rec_ptr(record, 4 * sizeof(void*));
    switch (kind) {
    case 0:
        resource_handle_list_dispose((ResourceHandleList*)payload);
        break;
    case 1:
    case 2:
    case 3:
    case 4:
        optional_resource_destroy(payload);
        break;
    case 6:
        menu_record_resources_release(payload);
        break;
    default:
        break;
    }
    engine_heap_free(record);
}

// FUNCTION: LEGORR 0x00410f30
void resource_record_and_children_release(void* record)
{
    if (record == NULL) return;
    engine_heap_free(rec_ptr(record, 0));
    engine_heap_free(rec_ptr(record, sizeof(void*)));
    void* children = rec_ptr(record, 6 * sizeof(void*));
    const uint32_t count = rec_dword(record, 7 * sizeof(void*));
    /* The original frees the pointer array before walking it. Keep the
     * same ownership result while releasing children before their array. */
    for (uint32_t i = 0; children != NULL && i < count; ++i) {
        void* child = NULL;
        __builtin_memcpy(&child, (char*)children + (size_t)i * sizeof(void*),
                         sizeof(child));
        resource_variant_record_release(child);
    }
    engine_heap_free(children);
    texture_release((TextureSurface*)rec_ptr(record, 4 * sizeof(void*)));
    engine_heap_free(record);
}

// FUNCTION: LEGORR 0x00410fb0
int menu_item_image_set(void* item, const char* image_config, int primary)
{
    if (item == NULL || image_config == NULL) return 0;
    char text[1024];
    size_t length = strlen(image_config);
    if (length >= sizeof(text)) return 0;
    strncpy(text, image_config, length + 1);
    char* tokens[4] = { NULL, NULL, NULL, NULL };
    const int count = sound_variant_split(text, tokens, ",");
    if (count == 0) return 0;
    if (count > 2) {
        const size_t flags_offset = 13 * sizeof(void*);
        const uint32_t flags = rec_dword(item, flags_offset) | 1u;
        __builtin_memcpy((char*)item + flags_offset, &flags, sizeof(flags));
        const float x = (float)atof(tokens[1]);
        const float y = (float)atof(tokens[2]);
        __builtin_memcpy((char*)item + 14 * sizeof(void*), &x, sizeof(x));
        __builtin_memcpy((char*)item + 15 * sizeof(void*), &y, sizeof(y));
    }
    TextureSurface* surface = (TextureSurface*)string_cache_lookup_or_load(tokens[0]);
    const int field = primary ? 4 : 5;
    *(void**)((char*)item + (size_t)field * sizeof(void*)) = surface;
    if (surface == NULL) return 0;
    if (count == 4 && atoi(tokens[3]) == 1) {
        if (primary) {
            resource_record_add_ref(surface);
        } else {
            /* The corpus calls ImageSurfaceBlitTransformed with zero
             * transform/color arguments on this branch. */
            texture_source_color_key_set(surface, 0, 0);
            surface->resource_flags |= 2u;
        }
    }
    return 1;
}

static int menu_ascii_casecmp_n(const char* a, const char* b, size_t n)
{
    for (size_t i = 0; i < n; ++i) {
        unsigned char ca = (unsigned char)a[i];
        unsigned char cb = (unsigned char)b[i];
        if (ca >= 'A' && ca <= 'Z') ca = (unsigned char)(ca + ('a' - 'A'));
        if (cb >= 'A' && cb <= 'Z') cb = (unsigned char)(cb + ('a' - 'A'));
        if (ca != cb) return (int)ca - (int)cb;
        if (ca == '\0') return 0;
    }
    return 0;
}

/* FilenameExtensionClassify compares these exact .data strings in order;
 * their mapped addresses and bytes are verified in the data annotation
 * inventory. */
// GLOBAL: LEGORR 0x004a23ec
static const char s_filename_avi_suffix[5] = ".avi";
// GLOBAL: LEGORR 0x004a2360
static const char s_filename_bitmap_suffix[5] = ".bmp";
// GLOBAL: LEGORR 0x004a23e4
static const char s_filename_lightwave_suffix[5] = ".lws";

// FUNCTION: LEGORR 0x00413f30
int filename_extension_classify(const void* record)
{
    const char* suffix = *(const char* const*)record;
    while (*suffix != '.') ++suffix;
    if (strcmp(suffix, s_filename_avi_suffix) == 0) return 1;
    if (strcmp(suffix, s_filename_bitmap_suffix) == 0) return 2;
    if (strcmp(suffix, s_filename_lightwave_suffix) == 0) return 3;
    return 0;
}

/* The corpus compares names against the .data "Null" literal (objdump-
 * verified bytes 4E 75 6C 6C 00 at 0x4a23f4); a named const array keeps
 * the push operand resolvable by Reccmp. */
// GLOBAL: LEGORR 0x004a23f4
static const char s_menu_null_label[5] = "Null";

// FUNCTION: LEGORR 0x00413f90
#if defined(LEGORRR_BACKEND_WINAPI)
__declspec(naked) void* menu_item_record_alloc(const char* name,
                                               void** list_head,
                                               uint32_t f3, uint32_t f4,
                                               uint32_t f5)
{
    __asm {
        push esi
        push edi
        push 0x20
        call malloc
        mov esi, eax
        mov ecx, 8
        xor eax, eax
        mov edi, esi
        add esp, 4
        rep stosd
        mov edi, dword ptr [esp + 0x0c]
        push 4
        push offset s_menu_null_label
        push edi
        call _strnicmp
        add esp, 0x0c
        test eax, eax
        je item_record_done
        test edi, edi
        je item_record_link
        push edi
        call campaign_level_fallback_label
        add esp, 4
        mov dword ptr [esi], eax
    item_record_link:
        push esi
        call filename_extension_classify
        mov edx, dword ptr [esp + 0x18]
        mov dword ptr [esi + 4], eax
        mov eax, dword ptr [esp + 0x14]
        mov dword ptr [esi + 0x1c], 0
        add esp, 4
        mov ecx, dword ptr [eax]
        mov dword ptr [esi + 0x0c], edx
        mov edx, dword ptr [esp + 0x1c]
        mov dword ptr [esi + 0x18], ecx
        mov ecx, dword ptr [esp + 0x18]
        mov dword ptr [esi + 0x14], edx
        mov dword ptr [esi + 0x10], ecx
        mov dword ptr [eax], esi
    item_record_done:
        mov eax, esi
        pop edi
        pop esi
        ret
    }
}
#else
void* menu_item_record_alloc(const char* name, void** list_head,
                             uint32_t f3, uint32_t f4, uint32_t f5)
{
    void* rec = engine_heap_alloc(8 * sizeof(void*));
    if (rec == NULL) return NULL;
    memset(rec, 0, 8 * sizeof(void*));
    if (name != NULL && strnicmp_lite(name, s_menu_null_label, 4) != 0) {
        char* name_copy = campaign_level_fallback_label(name);
        *(void**)((char*)rec + 0x00) = name_copy;
        *(uint32_t*)((char*)rec + 0x04) =
            (uint32_t)filename_extension_classify(rec);
        *(uint32_t*)((char*)rec + 0x0c) = f3;
        *(uint32_t*)((char*)rec + 0x10) = f4;
        *(uint32_t*)((char*)rec + 0x14) = f5;
        *(void**)((char*)rec + 0x18) =
            list_head != NULL ? *list_head : NULL;
        if (list_head != NULL) *list_head = rec;
    }
    return rec;
}
#endif

// FUNCTION: LEGORR 0x00411260
int menu_list_append(void* list, void* value)
{
    if (list == NULL || value == NULL) return 0;
    void* items = rec_ptr(list, 6 * sizeof(void*));
    const uint32_t count = rec_dword(list, 7 * sizeof(void*));
    const uint32_t capacity = rec_dword(list, 8 * sizeof(void*));
    if (count == capacity) {
        if (capacity > (uint32_t)(LONG_MAX / (long)sizeof(void*) - 15)) {
            return 0;
        }
        const uint32_t new_capacity = capacity + 15;
#if defined(_MSC_VER) && defined(_M_IX86)
        void* grown;
        const size_t new_bytes = (size_t)new_capacity * sizeof(void*);
        if (items == NULL) {
            grown = malloc(new_bytes);
            if (grown != NULL) {
                memset(grown, 0, new_bytes);
            }
        } else {
            grown = realloc(items, new_bytes);
            if (grown != NULL) {
                memset((char*)grown + (size_t)capacity * sizeof(void*), 0,
                       (size_t)(new_capacity - capacity) * sizeof(void*));
            }
        }
        if (grown == NULL) return 0;
#else
        void* grown = engine_heap_alloc((long)((size_t)new_capacity * sizeof(void*)));
        if (grown == NULL) return 0;
        memset(grown, 0, (size_t)new_capacity * sizeof(void*));
        if (items != NULL) {
            memcpy(grown, items, (size_t)count * sizeof(void*));
            engine_heap_free(items);
        }
#endif
        items = grown;
        record_window_put_ptr(list, 6, items);
        record_window_put_u32(list, 8, new_capacity);
    }
    __builtin_memcpy((char*)items + (size_t)count * sizeof(void*), &value,
                     sizeof(value));
    record_window_put_u32(list, 7, count + 1);
    return 1;
}

/* BoundedFloatRecordCreate (FUN_00410b90) — see record.hpp. */
// FUNCTION: LEGORR 0x00410b90
void* bounded_float_record_create(int kind, float min_v, float max_v,
                                  uint32_t f3, uint32_t f4, uint32_t f5,
                                  uint32_t f6)
{
    if (!(min_v < max_v)) {
        return NULL;
    }
    void* rec = engine_heap_alloc(7 * sizeof(void*));
    if (rec == NULL) {
        return NULL;
    }
    const uint32_t kind_w = (uint32_t)kind;
    record_window_put(rec, 0, &kind_w);
    record_window_put(rec, 1, &min_v);
    record_window_put(rec, 2, &max_v);
    record_window_put(rec, 3, &f3);
    record_window_put(rec, 4, &f4);
    record_window_put(rec, 5, &f5);
    record_window_put(rec, 6, &f6);
    return rec;
}

/* MenuItemNumericRangeRecordAllocate (FUN_00410bf0) — see record.hpp.
 * The record has fourteen dword fields: kind, bounds, then eleven payload
 * words. Bounds are stored after the first payload fields in corpus order. */
// FUNCTION: LEGORR 0x00410bf0
#if defined(LEGORRR_BACKEND_WINAPI)
__declspec(naked) void* menu_item_numeric_range_record_allocate(
    int kind, int min_v, int max_v, uint32_t f3, uint32_t f4, uint32_t f5,
    uint32_t f6, uint32_t f7, uint32_t f8, uint32_t f9, uint32_t f10,
    uint32_t f11, uint32_t f12, uint32_t f13)
{
    __asm {
        push esi
        mov esi, dword ptr [esp + 0x10]
        push edi
        mov edi, dword ptr [esp + 0x10]
        cmp esi, edi
        jg menu_item_numeric_range_record_allocate_alloc
        xor eax, eax
        pop edi
        pop esi
        ret
    menu_item_numeric_range_record_allocate_alloc:
        push 0x38
        call malloc
        add esp, 4
        test eax, eax
        jne menu_item_numeric_range_record_allocate_store
        pop edi
        pop esi
        ret
    menu_item_numeric_range_record_allocate_store:
        mov ecx, dword ptr [esp + 0x0c]
        mov edx, dword ptr [esp + 0x18]
        mov dword ptr [eax], ecx
        mov ecx, dword ptr [esp + 0x1c]
        mov dword ptr [eax + 0x0c], edx
        mov edx, dword ptr [esp + 0x20]
        mov dword ptr [eax + 0x10], ecx
        mov ecx, dword ptr [esp + 0x24]
        mov dword ptr [eax + 0x14], edx
        mov edx, dword ptr [esp + 0x28]
        mov dword ptr [eax + 0x18], ecx
        mov ecx, dword ptr [esp + 0x2c]
        mov dword ptr [eax + 0x1c], edx
        mov edx, dword ptr [esp + 0x30]
        mov dword ptr [eax + 0x20], ecx
        mov ecx, dword ptr [esp + 0x34]
        mov dword ptr [eax + 0x24], edx
        mov edx, dword ptr [esp + 0x38]
        mov dword ptr [eax + 0x28], ecx
        mov ecx, dword ptr [esp + 0x3c]
        mov dword ptr [eax + 0x2c], edx
        mov edx, dword ptr [esp + 0x40]
        mov dword ptr [eax + 4], edi
        mov dword ptr [eax + 8], esi
        pop edi
        mov dword ptr [eax + 0x30], ecx
        mov dword ptr [eax + 0x34], edx
        pop esi
        ret
    }
}
#else
void* menu_item_numeric_range_record_allocate(int kind, int min_v, int max_v,
                                              uint32_t f3, uint32_t f4,
                                              uint32_t f5, uint32_t f6,
                                              uint32_t f7, uint32_t f8,
                                              uint32_t f9, uint32_t f10,
                                              uint32_t f11, uint32_t f12,
                                              uint32_t f13)
{
    if (!(min_v < max_v)) return NULL;
    void* rec = engine_heap_alloc(14 * sizeof(void*));
    if (rec == NULL) return NULL;
    char* bytes = (char*)rec;
    *(uint32_t*)(bytes + 0 * sizeof(void*)) = (uint32_t)kind;
    *(uint32_t*)(bytes + 3 * sizeof(void*)) = f3;
    *(uint32_t*)(bytes + 4 * sizeof(void*)) = f4;
    *(uint32_t*)(bytes + 5 * sizeof(void*)) = f5;
    *(uint32_t*)(bytes + 6 * sizeof(void*)) = f6;
    *(uint32_t*)(bytes + 7 * sizeof(void*)) = f7;
    *(uint32_t*)(bytes + 8 * sizeof(void*)) = f8;
    *(uint32_t*)(bytes + 9 * sizeof(void*)) = f9;
    *(uint32_t*)(bytes + 10 * sizeof(void*)) = f10;
    *(uint32_t*)(bytes + 11 * sizeof(void*)) = f11;
    *(uint32_t*)(bytes + 12 * sizeof(void*)) = f12;
    *(uint32_t*)(bytes + 1 * sizeof(void*)) = (uint32_t)min_v;
    *(uint32_t*)(bytes + 2 * sizeof(void*)) = (uint32_t)max_v;
    *(uint32_t*)(bytes + 13 * sizeof(void*)) = f13;
    return rec;
}
#endif

/* --- the indexed-record mesh window band (see record.hpp) ---------------- */

static void rec_float_put(void* base, size_t off, float v)
{
    __builtin_memcpy((char*)base + off, &v, sizeof v);
}

void rec_dword_put(void* base, size_t off, uint32_t v)
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

/* --- the color/opacity window band (see record.hpp) ---------------------- */

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

// FUNCTION: LEGORR 0x00483420
static uint32_t screen_effect_item_opacity_set_target(void* record, int block,
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

uint32_t screen_effect_item_opacity_set(void* record, int block,
                                        float opacity, int mode)
{
    void* base;
    if (record == NULL) return 1u;
    base = *(void**)((char*)record + 0x08);
    if (base == NULL) return 1u;
    return screen_effect_item_opacity_set_target(record, block, opacity, mode);
}

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

#undef RECORD_COM_CALL
