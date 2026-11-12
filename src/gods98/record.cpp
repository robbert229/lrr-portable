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
#include "record_from_container_unit.hpp"
#include "record_for_mesh_unit.hpp"
#include "record_from_mesh_unit.hpp"

static void* s_large_record_free;
static void* s_large_record_blocks[32];
static unsigned int s_large_record_block_count;
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

void entity_flag_80_set(void* record, int set)
{
    if (record == NULL) {
        return; /* deviation: the corpus strides the record blindly */
    }
    entity_flag_80_set_corpus(record, set);
}

uint32_t entity_type_flag_bit1_get(const void* record)
{
    if (record == NULL) return 0u;
    return entity_type_flag_bit1_get_target(record);
}

/* ------------------------------------------------------------------ *
 *  Field-reader sweep (see record.hpp). */

uint32_t scene_object_flag_bit3_get(const void* scene_object)
{
    if (scene_object == NULL) return 0u;
    return scene_object_flag_bit3_get_target(scene_object);
}

void* indexed_record_data_address_get(const void* record, int index)
{
    void* base;
    if (record == NULL) return NULL;
    base = *(void* const*)((const char*)record + 0x08);
    if (base == NULL) return NULL;
    return indexed_record_data_address_get_target(record, index);
}

/* --- grid-descriptor coordinate leaves (see record.hpp) ------------------ */

/* --- indexed flag slot and resource-handle writers (see record.hpp) ------ */

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

/* --- the UI record factories (see record.hpp) ------------------------------
 * Corpus record windows stride as dwords; the port lays both shapes out as
 * native field words (the crt_stream_record window convention). Field reads
 * by later band members follow this layout. */

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

/* --- the indexed-record mesh window band (see record.hpp) ---------------- */

void rec_dword_put(void* base, size_t off, uint32_t v)
{
    __builtin_memcpy((char*)base + off, &v, sizeof v);
}

/* --- the color/opacity window band (see record.hpp) ---------------------- */

uint32_t screen_effect_item_opacity_set(void* record, int block,
                                        float opacity, int mode)
{
    void* base;
    if (record == NULL) return 1u;
    base = *(void**)((char*)record + 0x08);
    if (base == NULL) return 1u;
    return screen_effect_item_opacity_set_target(record, block, opacity, mode);
}


#undef RECORD_COM_CALL
