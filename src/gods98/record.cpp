// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — corpus record access primitives (see record.hpp). */

#include "record.hpp"
#include "heap.hpp"
#include "sound.hpp"
#include "string.hpp"
#include "view.hpp"
#include "texture.hpp"
#include "resource.hpp"
#include "crt.hpp"
#include "game/util.hpp"
#include <math.h>  /* sqrtf for the instance-quad normal window */
#include <limits.h>
#include <string.h>
#include <stdlib.h>

/* Byte-safe field reads: corpus dword/float slots at offsets that may
 * not be pointer-aligned on 64-bit hosts. */
static uint32_t rec_dword(const void* base, size_t off)
{
    uint32_t v = 0;
    if (base != NULL) {
        __builtin_memcpy(&v, (const char*)base + off, sizeof(v));
    }
    return v;
}

static float rec_float(const void* base, size_t off)
{
    float v = 0.0f;
    if (base != NULL) {
        __builtin_memcpy(&v, (const char*)base + off, sizeof(v));
    }
    return v;
}

/* Pointer slots are modeled native-size (documented deviation from the
 * corpus's 32-bit pointers); reads stay byte-safe like the fields. */
static void* rec_ptr(const void* base, size_t off)
{
    void* v = NULL;
    if (base != NULL) {
        __builtin_memcpy(&v, (const char*)base + off, sizeof(v));
    }
    return v;
}

/* Defined with the mesh-window band below; used by the visibility setter. */
static void rec_dword_put(void* base, size_t off, uint32_t v);

/* Contents of a native cell array: memcpy read (the corpus's cell
 * dwords can sit unaligned after a stride change). */
static uint32_t cells_dword(const void* cells, size_t off)
{
    uint32_t v = 0;
    if (cells != NULL) {
        __builtin_memcpy(&v, (const char*)cells + off, sizeof(v));
    }
    return v;
}

/* RecordField14Get (FUN_00435f0b). */
// FUNCTION: LEGORR 0x00435f0b
uint32_t record_field14_get(const void* record)
{
    return rec_dword(record, 0x14);
}

/* ObjectField10Get (FUN_004850e0). */
// FUNCTION: LEGORR 0x004850e0
uint32_t object_field10_get(const void* object)
{
    return rec_dword(object, 0x10);
}

/* RecordFieldD8EqualsOne (FUN_0046c3d0): int == 1, not "nonzero". */
// FUNCTION: LEGORR 0x0046c3d0
int record_field_d8_equals_one(const void* record)
{
    const int32_t v = (int32_t)rec_dword(record, 0xd8);
    return v == 1;
}

/* ReadIndexedPairSlotDword (FUN_00471a20): base + i*8 + j*4, exactly as
 * the corpus computes it. */
// FUNCTION: LEGORR 0x00471a20
uint32_t read_indexed_pair_slot_dword(const void* base, uint32_t i,
                                      uint32_t j)
{
    return rec_dword(base, (size_t)i * 8u + (size_t)j * 4u);
}

/* VelocitySlotPrimaryObjectGet / SecondaryObjectGet (FUN_00471a40 /
 * FUN_00471a50): indexed slot at +0x18 / +0x24. */
// FUNCTION: LEGORR 0x00471a40
uint32_t velocity_slot_primary_object_get(const void* record,
                                          uint32_t index)
{
    return rec_dword(record, 0x18u + (size_t)index * 4u);
}

// FUNCTION: LEGORR 0x00471a50
uint32_t velocity_slot_secondary_object_get(const void* record,
                                            uint32_t index)
{
    return rec_dword(record, 0x24u + (size_t)index * 4u);
}

/* VelocitySlotVectorRead (FUN_00471c70) — see record.hpp. The corpus
 * copies the three dwords bit-for-bit; the port reads them as floats
 * through the NULL-guarded reader, so the words ride unchanged. */
// FUNCTION: LEGORR 0x00471c70
void velocity_slot_vector_read(const void* record, float out[3],
                               uint32_t index)
{
    if (out == NULL) {
        return; /* deviation: the corpus strides the caller's buffer */
    }
    const size_t off = 0x44u + (size_t)index * 12u;
    for (int i = 0; i != 3; ++i) {
        const float v = rec_float(record, off + (size_t)i * 4u);
        __builtin_memcpy(&out[i], &v, sizeof v);
    }
}

/* EntityVelocityStateRecordGet (FUN_004712c0) — see record.hpp. The corpus
 * strides the type-keyed sub-record blindly (and walks into the sub-record
 * when the type answers no base at all); the port guards both links. */
// FUNCTION: LEGORR 0x004712c0
void* entity_velocity_state_record_get(const void* record)
{
    if (record == NULL) {
        return NULL; /* deviation: the corpus strides the record blindly */
    }
    const uint32_t type = rec_dword(record, 0x00);
    if (type == 1u) {
        void* sub = rec_ptr(record, 0x0c);
        return (sub != NULL) ? (void*)((char*)sub + 0x138)
                             : NULL; /* deviation: corpus strides blindly */
    }
    if (type == 4u) {
        void* sub = rec_ptr(record, 0x18);
        return (sub != NULL) ? (void*)((char*)sub + 0xa0)
                             : NULL; /* deviation: corpus strides blindly */
    }
    return NULL; /* deviation: the corpus walks a null base */
}

/* EntityVelocitySlotValueGet (FUN_004712f0): the float word at
 * +0x98 + slot*4 of the state record. The corpus widens through x87;
 * the bit pattern rides unchanged. Deviation: the corpus strides a null
 * base. */
// FUNCTION: LEGORR 0x004712f0
float entity_velocity_slot_value_get(const void* record, int slot)
{
    void* base = entity_velocity_state_record_get(record);
    if (base == NULL || slot < 0) {
        return 0.0f; /* deviation */
    }
    return rec_float(base, 0x98u + (size_t)slot * 4u);
}

/* EntityVelocitySlotValueSet (FUN_00471310): the store counterpart. */
// FUNCTION: LEGORR 0x00471310
void entity_velocity_slot_value_set(void* record, uint32_t value, int slot)
{
    void* base = entity_velocity_state_record_get(record);
    if (base == NULL || slot < 0) {
        return; /* deviation */
    }
    __builtin_memcpy((char*)base + 0x98u + (size_t)slot * 4u, &value,
                     sizeof value);
}

/* EntityFlag80Set (FUN_00473b20) — see record.hpp. */
// FUNCTION: LEGORR 0x00473b20
void entity_flag_80_set(void* record, int set)
{
    if (record == NULL) {
        return; /* deviation: the corpus strides the record blindly */
    }
    uint32_t word = 0;
    __builtin_memcpy(&word, (char*)record + 0x14, sizeof word);
    word = (set != 0) ? (word | 0x80u) : (word & ~0x80u);
    __builtin_memcpy((char*)record + 0x14, &word, sizeof word);
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

/* InterpolatedZWithinBounds (FUN_00471330): corpus order — the
 * interpolation, then `v <= target` and `target - t <= v`. */
// FUNCTION: LEGORR 0x00471330
int interpolated_z_within_bounds(const void* record_a, const void* record_b,
                                 float t, float target, float scale)
{
    const float v = rec_float(record_b, 8) * scale + rec_float(record_a, 8);
    if (v <= target && target - t <= v) {
        return 1;
    }
    return 0;
}

/* CameraRotationGet (FUN_00436b79): the float at +0x84 (opaque-record
 * reader; see header). */
// FUNCTION: LEGORR 0x00436b79
float camera_rotation_get(const void* camera)
{
    return rec_float(camera, 0x84);
}

/* MapCellToWorldCenter (FUN_0044f640, finding 225): the descriptor's
 * int fields convert to float at the use site — reproduced exactly,
 * including the unsigned bounds compare. */
// FUNCTION: LEGORR 0x0044f640
int map_cell_to_world_center(const void* map, uint32_t cell_x,
                             uint32_t cell_y, float* world_x, float* world_y)
{
    const uint32_t width = rec_dword(map, 0x00);
    const uint32_t height = rec_dword(map, 0x04);
    const int32_t scale_i = (int32_t)rec_dword(map, 0x10);
    const int32_t origin_x_i = (int32_t)rec_dword(map, 0x18);
    const int32_t origin_y_i = (int32_t)rec_dword(map, 0x1C);
    const float scale = (float)scale_i;
    const float origin_x = (float)origin_x_i;
    const float origin_y = (float)origin_y_i;

    if (cell_x < width && cell_y < height) {
        if (world_x != NULL) {
            *world_x = scale * 0.5f + (float)cell_x * scale + origin_x;
        }
        if (world_y != NULL) {
            *world_y = origin_y - (scale * 0.5f + (float)cell_y * scale);
        }
        return 1;
    }
    return 0;
}

/* TaskFlagBitQuery (FUN_00408970, findings 366/465). */
// FUNCTION: LEGORR 0x00408970
uint32_t task_flag_bit_query(const void* task, uint32_t bit, int suppress)
{
    const uint32_t mask = 1u << (bit & 0x1fu);
    const uint32_t flags = rec_dword(task, 0x98);
    if (suppress != 0 && (flags & mask) != 0) {
        return 0;
    }
    return flags & mask;
}

/* TaskField84GetAndField88Out (FUN_00408360): the pair read, corpus
 * order — the optional +0x88 write happens before the +0x84 answer. */
// FUNCTION: LEGORR 0x00408360
uint32_t task_field84_get_and_field88_out(const void* task, uint32_t* out88)
{
    if (task == NULL) {
        return 0u;
    }
    if (out88 != NULL) {
        *out88 = rec_dword(task, 0x88);
    }
    return rec_dword(task, 0x84);
}

/* PackedGridCellValueGet (FUN_0040b850, findings 373/451): low nibble
 * * width + high nibble indexes the cell dwords; the corpus's own NULL
 * grid guard answers 0, reproduced. */
// FUNCTION: LEGORR 0x0040b850
uint32_t packed_grid_cell_value_get(const void* grid, uint32_t packed_cell)
{
    void* cells = rec_ptr(grid, 0x08);
    if (cells == NULL) {
        return 0u;
    }
    const int32_t width = (int32_t)rec_dword(grid, 0x00);
    const int32_t index = (int32_t)(packed_cell & 0xfu) * width
        + (int32_t)((packed_cell & 0xffu) >> 4);
    return cells_dword(cells, (size_t)index * 4u);
}

/* ObjectPoolListHeadGet (FUN_00456c30, finding 374): the corpus's exact
 * in-range bounds (-1 < index < 0x37) over the +0x114 slot table. */
// FUNCTION: LEGORR 0x00456c30
int object_pool_list_head_get(const void* pools, int index)
{
    if (-1 < index && index < 0x37) {
        const int32_t v = (int32_t)rec_dword(
            pools, (size_t)(0x114 + index * 4));
        if (v != 0) {
            return v;
        }
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

/* EntityTypeFlagBit1Get (FUN_0046d330, findings 357/432): the corpus
 * reads +0x1e8 with one mov, no helper call, so the load is direct. */
// FUNCTION: LEGORR 0x0046d330
uint32_t entity_type_flag_bit1_get(const void* record)
{
    if (record == NULL) {
        return 0u; /* pinned deviation: the corpus loads blindly */
    }
    return (*(const uint32_t*)((const char*)record + 0x1e8) & 0xffu) >> 1 & 1u;
}

/* ------------------------------------------------------------------ *
 *  Field-reader sweep (see record.hpp). */

// FUNCTION: LEGORR 0x0046db60
uint32_t object_record_field11c_get(const void* record)
{
    return rec_dword(record, 0x11c);
}

// FUNCTION: LEGORR 0x0047a540
uint32_t object_field_be4_dword_get(const void* record)
{
    return rec_dword(record, 0xbe4);
}

// FUNCTION: LEGORR 0x0046cf80
uint32_t entity_type_mask_get(const void* record)
{
    return rec_dword(record, 300);
}

// FUNCTION: LEGORR 0x00482210
uint32_t pointer_value_get(const void* pointed)
{
    return rec_dword(pointed, 0);
}

// FUNCTION: LEGORR 0x00408850
uint32_t scene_object_flags_get(const void* scene_object)
{
    return rec_dword(scene_object, 0x70);
}

// FUNCTION: LEGORR 0x00473db0
uint32_t scene_object_owner_get(const void* scene_object)
{
    return rec_dword(scene_object, 0x20);
}

// FUNCTION: LEGORR 0x0046d9e0
uint32_t scene_object_resource_get(const void* scene_object)
{
    return rec_dword(scene_object, 0x08);
}

// FUNCTION: LEGORR 0x00473680
uint32_t scene_object_wrapped_pointer_get(const void* wrapped)
{
    return rec_dword(wrapped, 0);
}

/* SceneObjectFlagBit3Get (FUN_00473ba0): one corpus mov on +0x14, so the
 * load is direct. animation_object_visibility_get can hand us a NULL +4
 * handle through the guarded rec_ptr; NULL answers zero (the corpus
 * would crash on it). */
// FUNCTION: LEGORR 0x00473ba0
uint32_t scene_object_flag_bit3_get(const void* scene_object)
{
    if (scene_object == NULL) {
        return 0u; /* pinned deviation: the corpus loads blindly */
    }
    return (*(const uint32_t*)((const char*)scene_object + 0x14) & 0xffu)
           >> 3 & 1u;
}

// FUNCTION: LEGORR 0x00406980
int scene_object_mode_is_one(const void* scene_object)
{
    return rec_dword(scene_object, 0x28) == 1u ? 1 : 0;
}

// FUNCTION: LEGORR 0x00406e30
uint32_t animation_object_handle_get(const void* animation_object)
{
    return rec_dword(animation_object, 0x04);
}

// FUNCTION: LEGORR 0x004877d0
uint32_t resource_entry_id_get(const void* entry)
{
    if (entry != NULL) {
        uint16_t v = 0;
        __builtin_memcpy(&v, (const char*)entry + 2, sizeof(v));
        return v;
    }
    return 0u;
}

// FUNCTION: LEGORR 0x004850c0
uint32_t indexed_word_table_lookup(uint32_t index, const void* table)
{
    const size_t off = 0x41c + (size_t)(index & 0xffu) * 2u;
    if (table != NULL) {
        uint16_t v = 0;
        __builtin_memcpy(&v, (const char*)table + off, sizeof(v));
        return v;
    }
    return 0u;
}

// FUNCTION: LEGORR 0x00482780
uint32_t indexed_record_flag6c_bit0_get(const void* record, int index)
{
    const char* base = (const char*)rec_ptr(record, 0x08);
    if (base == NULL) {
        return 0u;
    }
    const size_t off = (size_t)index * 0x70u + 0x6cu;
    return rec_dword(base + off, 0) & 1u;
}

// FUNCTION: LEGORR 0x00483400
void* indexed_record_data_address_get(const void* record, int index)
{
    char* base = (char*)rec_ptr(record, 0x08);
    if (base == NULL) {
        return NULL;
    }
    return (void*)(base + (size_t)((size_t)index * 0x70u + 0x10u));
}

// FUNCTION: LEGORR 0x00408e10
int count_matching_coordinates_in_list(const void* manager,
                                       int32_t coord_x, int32_t coord_y)
{
    const int32_t count = (int32_t)rec_dword(manager, 0x20);
    if (count == 0) {
        return 0;
    }
    int found = 0;
    for (int32_t i = 0; i < count; ++i) {
        void* entry = rec_ptr(manager, 0x2cu + (size_t)i * sizeof(void*));
        if (entry != NULL
            && (int32_t)rec_dword(entry, 0) == coord_x
            && (int32_t)rec_dword(entry, 4) == coord_y) {
            ++found;
        }
    }
    return found;
}

/* AviPlayerReadyGet (FUN_0047ed80, findings 185/414): the readiness
 * byte at +0x99 (byte-safe — the corpus reads it as a raw byte). */
// FUNCTION: LEGORR 0x0047ed80
uint32_t avi_player_ready_get(const void* movie)
{
    if (movie != NULL) {
        uint8_t v = 0;
        __builtin_memcpy(&v, (const char*)movie + 0x99, sizeof(v));
        return v;
    }
    return 0u;
}

/* GameFileSystemTypeGet (FUN_0047fe20, findings 234/404): the handle's
 * leading type dword. */
// FUNCTION: LEGORR 0x0047fe20
uint32_t game_file_system_type_get(const void* handle)
{
    return rec_dword(handle, 0);
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

/* --- grid-descriptor coordinate leaves (see record.hpp) ------------------ */

// FUNCTION: LEGORR 0x0044f700
int world_to_terrain_cell_bounds_check(const void* map, float world_x,
                                       float world_y, int* cell_x,
                                       int* cell_y, float* subcell)
{
    const int cx = (int)world_x; /* __ftol: truncate toward zero */
    const int cy = (int)world_y;
    if (cell_x != NULL) {
        *cell_x = cx;
    }
    if (cell_y != NULL) {
        *cell_y = cy;
    }
    const int w = (int)rec_dword(map, 0x00);
    const int h = (int)rec_dword(map, 0x04);
    if (map == NULL || !(-1 < cx && cx < w && -1 < cy && cy < h)) {
        return 0;
    }
    if (subcell != NULL) {
        const float bs = (float)(int)rec_dword(map, 0x10);
        const float ox = (float)(int)rec_dword(map, 0x18);
        const float oy = (float)(int)rec_dword(map, 0x1C);
        float px = (world_x - ((float)cx * bs + ox)) / (bs * 0.5f);
        float py = -((world_y - (oy - (float)cy * bs)) / (bs * 0.5f));
        if (1.0f < px) {
            px = 2.0f - px;
        }
        if (1.0f < py) {
            py = 2.0f - py;
        }
        if (px < py) {
            py = px;
        }
        *subcell = py;
    }
    return 1;
}

/* TerrainGridPick (FUN_0044f6d0): forward the same cell outputs and retain
 * the bounds result; the corpus's temporary subcell write is discarded. */
// FUNCTION: LEGORR 0x0044f6d0
int terrain_grid_pick(const void* map, float world_x, float world_y,
                      int* cell_x, int* cell_y)
{
    float unused_subcell = 0.0f;
    return world_to_terrain_cell_bounds_check(
        map, world_x, world_y, cell_x, cell_y, &unused_subcell);
}

// FUNCTION: LEGORR 0x0044f810
void grid_coordinate_clamp_and_store(const void* map, float world_x,
                                     float world_y, int* cell_x, int* cell_y)
{
    const int w = (int)rec_dword(map, 0x00);
    const int h = (int)rec_dword(map, 0x04);
    int cx = (int)world_x;
    int cy = (int)world_y;
    if (cx < 0) {
        cx = 0;
    }
    if (w <= cx) {
        cx = w - 1;
    }
    if (cy < 0) {
        cy = 0;
    }
    if (h <= cy) {
        cy = h - 1;
    }
    if (cell_x != NULL) {
        *cell_x = cx;
    }
    if (cell_y != NULL) {
        *cell_y = cy;
    }
}

// FUNCTION: LEGORR 0x0044f870
uint32_t terrain_world_point_to_cell(const void* map, const float* point,
                                     float* cell_out, float* offset_out)
{
    int cx = 0, cy = 0;
    if (point != NULL
        && world_to_terrain_cell_bounds_check(map, point[0], point[1],
                                              &cx, &cy, NULL) != 0) {
        if (cell_out != NULL) {
            cell_out[0] = (float)cx;
            cell_out[1] = (float)cy;
        }
        if (offset_out != NULL) {
            float wx = 0.0f, wy = 0.0f;
            map_cell_to_world_center(map, (uint32_t)cx, (uint32_t)cy,
                                     &wx, &wy);
            float scale_f = 0.0f;
            __builtin_memcpy(&scale_f, (const char*)map + 0x10,
                             sizeof(scale_f));
            const float inv = 1.0f / scale_f;
            offset_out[0] = inv * (point[0] - wx) + 0.5f;
            offset_out[1] = inv * (point[1] - wy) + 0.5f;
        }
    }
    return 0u;
}

/* --- indexed flag slot and resource-handle writers (see record.hpp) ------ */

// FUNCTION: LEGORR 0x0044e6b0
void indexed_flag_value_set_one(void* record, uint32_t index)
{
    if (record == NULL) {
        return;
    }
    const uint32_t one = 1u;
    const size_t off = 0x40b4u + (size_t)(index & 0xffu) * 4u;
    __builtin_memcpy((char*)record + off, &one, sizeof(one));
}

// FUNCTION: LEGORR 0x0044f220
void scene_object_resource_handle_set(void* object, uint32_t handle)
{
    if (object == NULL) {
        return;
    }
    __builtin_memcpy((char*)object + 0x2cu, &handle, sizeof(handle));
}

/* --- entity record-field leaves (see record.hpp) --------------------------- */
// FUNCTION: LEGORR 0x00435ef4
void record_fields_4_and_14_set(void* record, uint32_t value_a,
                                uint32_t value_b)
{
    if (record == NULL) {
        return;
    }
    __builtin_memcpy((char*)record + 0x04, &value_a, sizeof(value_a));
    __builtin_memcpy((char*)record + 0x14, &value_b, sizeof(value_b));
}

// FUNCTION: LEGORR 0x00435f32
void entity_fields_a0_a4_store_a8_clear(void* record, uint32_t value_a,
                                        uint32_t value_b)
{
    if (record == NULL) {
        return;
    }
    const uint32_t zero = 0u;
    __builtin_memcpy((char*)record + 0xa0, &value_a, sizeof(value_a));
    __builtin_memcpy((char*)record + 0xa4, &value_b, sizeof(value_b));
    __builtin_memcpy((char*)record + 0xa8, &zero, sizeof(zero));
}

// FUNCTION: LEGORR 0x00435d91
void entity_b8_bit3_set_or_clear(void* record, int set)
{
    if (record == NULL) {
        return;
    }
    unsigned char* byte = (unsigned char*)record + 0xb8;
    if (set != 0) {
        *byte = (unsigned char)(*byte | 8);
    } else {
        *byte = (unsigned char)(*byte & ~8u);
    }
}

/* --- the record-field write band (see record.hpp) --------------------------- */
// FUNCTION: LEGORR 0x00462ab0
void record_field18_set(void* record, uint32_t value)
{
    if (record == NULL) {
        return; /* deviation: the corpus writes blindly */
    }
    __builtin_memcpy((char*)record + 0x18, &value, sizeof(value));
}

// FUNCTION: LEGORR 0x00462ac0
void record_flags_24_or(void* record, uint32_t flags)
{
    if (record == NULL) {
        return; /* deviation: the corpus reads blindly */
    }
    uint32_t f = 0u;
    __builtin_memcpy(&f, (char*)record + 0x24, sizeof(f));
    f |= flags;
    __builtin_memcpy((char*)record + 0x24, &f, sizeof(f));
}

// FUNCTION: LEGORR 0x0045d8a0
void object_field14_set(void* record, uint32_t value)
{
    if (record == NULL) {
        return; /* deviation: the corpus writes blindly */
    }
    __builtin_memcpy((char*)record + 0x14, &value, sizeof(value));
}

// FUNCTION: LEGORR 0x00451c30
void linked_record_field4_clear(void* record)
{
    if (record == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    char* node = (char*)record;
    while (node != NULL) {
        const uint32_t zero = 0u;
        __builtin_memcpy(node + 4, &zero, sizeof(zero));
        void* next = NULL;
        __builtin_memcpy(&next, node + 0x14, sizeof(next));
        node = (char*)next;
    }
}

/* CopyTwoDwordPairToOutputs (FUN_00439930) — see record.hpp. */
// FUNCTION: LEGORR 0x00439930
void copy_two_dword_pair_to_outputs(const void* record, uint32_t* out_a,
                                    uint32_t* out_b)
{
    if (record == NULL || out_a == NULL || out_b == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    __builtin_memcpy(out_a, (const char*)record, 4);
    __builtin_memcpy(out_b, (const char*)record + 4, 4);
}

// FUNCTION: LEGORR 0x00473da0
void scene_object_owner_set(void* scene_object, uint32_t owner)
{
    if (scene_object == NULL) {
        return; /* deviation: the corpus writes blindly */
    }
    __builtin_memcpy((char*)scene_object + 0x20, &owner, sizeof(owner));
}

/* --- the record primitive band (see record.hpp) ----------------------------- */
// FUNCTION: LEGORR 0x00482350
void indexed_record_field64_set(void* record, int index, uint32_t value)
{
    void* base = NULL;
    if (record == NULL) {
        return; /* deviation: the corpus reads blindly */
    }
    __builtin_memcpy(&base, (char*)record + 8, sizeof(base));
    if (base == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    __builtin_memcpy((char*)base + 100 + (size_t)index * 0x70,
                     &value, sizeof(value));
}

// FUNCTION: LEGORR 0x0040ff70
void object_vector3_set(void* object_block, float x, float y, float z)
{
    void* base = NULL;
    if (object_block == NULL) {
        return; /* deviation: the corpus reads blindly */
    }
    __builtin_memcpy(&base, (char*)object_block, sizeof(base));
    if (base == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    __builtin_memcpy((char*)base + 0x3c, &x, sizeof(x));
    __builtin_memcpy((char*)base + 0x40, &y, sizeof(y));
    __builtin_memcpy((char*)base + 0x44, &z, sizeof(z));
}

// FUNCTION: LEGORR 0x004851b0
void float_pair_at_offset4_set_from_ints(void* record, int x, int y)
{
    if (record == NULL) {
        return; /* deviation: the corpus writes blindly */
    }
    const float fx = (float)x; /* the corpus's int->float casts */
    const float fy = (float)y;
    __builtin_memcpy((char*)record + 0x04, &fx, sizeof(fx));
    __builtin_memcpy((char*)record + 0x08, &fy, sizeof(fy));
}

// FUNCTION: LEGORR 0x004851d0
void float_pair_at_offsetc_set_from_uints(void* record, uint32_t x,
                                          uint32_t y)
{
    if (record == NULL) {
        return; /* deviation: the corpus writes blindly */
    }
    const float fx = (float)x; /* the corpus's uint->float casts */
    const float fy = (float)y;
    __builtin_memcpy((char*)record + 0x0c, &fx, sizeof(fx));
    __builtin_memcpy((char*)record + 0x10, &fy, sizeof(fy));
}

// FUNCTION: LEGORR 0x00485a00
void object_state_pair_get(const void* record, uint32_t* out_a,
                           uint32_t* out_b)
{
    if (record == NULL || out_a == NULL || out_b == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    __builtin_memcpy(out_a, (const char*)record + 0x820, sizeof(*out_a));
    __builtin_memcpy(out_b, (const char*)record + 0x824, sizeof(*out_b));
}

/* --- the scene-visibility wrapper band (see record.hpp) ---------------------
 * FUN_00473b40 (the corpus surface lock/unlock leaf) is the port's hook
 * boundary: the wrappers reproduce their own gates and stores and route the
 * leaf through the recorded no-op below. */
static int s_scene_vis_calls;
static void* s_scene_vis_last_obj;
static int s_scene_vis_last_flag;

static void scene_visibility_hook_call(void* object, int flag)
{
    ++s_scene_vis_calls;
    s_scene_vis_last_obj = object;
    s_scene_vis_last_flag = flag;
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
static uint32_t s_overlay_word;  /* DAT_00552180 role */
static uint32_t s_overlay_cache; /* DAT_0055218c role */

// FUNCTION: LEGORR 0x004634b0
void graphics_overlay_visibility_set(uint32_t flag)
{
    if (s_overlay_cache != flag) {
        s_overlay_cache = flag;
        /* corpus: the handle rides the call, flag folded to 1/0. */
        scene_visibility_hook_call((void*)(uintptr_t)s_overlay_word,
                                   flag != 0u ? 1 : 0);
    }
}

void graphics_overlay_word_set(uint32_t handle_word)
{
    s_overlay_word = handle_word;
}

uint32_t graphics_overlay_word_peek(void) { return s_overlay_word; }

/* SurfaceVisibilitySet (FUN_00464720) — record +0 handle through the leaf. */
// FUNCTION: LEGORR 0x00464720
void surface_visibility_set(const void* record, uint32_t flag)
{
    scene_visibility_hook_call(rec_ptr(record, 0x00), (int)flag);
}

/* AnimationObjectVisibilitySet (FUN_00406c90) — animation record +4. */
// FUNCTION: LEGORR 0x00406c90
void animation_object_visibility_set(const void* anim, uint32_t flag)
{
    scene_visibility_hook_call(rec_ptr(anim, 0x04), (int)flag);
}

/* SceneObjectPairApplyMode (FUN_00408670) — the +4 object, then the +0x7c
 * partner when nonzero. Deviation: the partner test strides a dword in the
 * corpus; the port's widened slot answers through the pointer. */
// FUNCTION: LEGORR 0x00408670
void scene_object_pair_apply_mode(const void* record, uint32_t flag)
{
    scene_visibility_hook_call(rec_ptr(record, 0x04), (int)flag);
    void* partner = rec_ptr(record, 0x7c);
    if (partner != NULL) {
        scene_visibility_hook_call(partner, (int)flag);
    }
}

/* SceneResourceVisibilitySet (FUN_0046d2c0): raise (+0x1e8 |= 2) or clear
 * (&= ~2) the record's visibility word, then route the raw flag through the
 * leaf (FUN_00473b40) — unconditionally on the +8 object, on +0xc when it
 * is set, and on every live child in the +0x10 window counted at +0x40.
 * Deviations: the corpus's +8/+0xc object fields are adjacent 32-bit
 * dwords, which the port's native-size pointer-slot model cannot carry
 * four bytes apart — the port routes the field values as raw handles,
 * exactly as the corpus does (what routes through the corpus's leaf is the
 * dword itself). The children keep the band's native-size stride (the
 * slot-window precedent) walked only to the window's six entries before
 * +0x40 (the corpus strides its dword slots blindly past the count field);
 * the count is read once (the leaf never touches the record) and NULL
 * records are guarded. The flag rides the leaf raw, unlike the overlay's
 * folded call. */
// FUNCTION: LEGORR 0x0046d2c0
void scene_resource_visibility_set(void* record, uint32_t flag)
{
    if (record == NULL) {
        return; /* deviation: the corpus strides blindly */
    }
    const uint32_t word = rec_dword(record, 0x1e8);
    rec_dword_put(record, 0x1e8, flag == 0u ? word & ~2u : word | 2u);
    scene_visibility_hook_call(
        (void*)(uintptr_t)rec_dword(record, 0x08), (int)flag);
    const uint32_t secondary = rec_dword(record, 0x0c);
    if (secondary != 0u) {
        scene_visibility_hook_call((void*)(uintptr_t)secondary, (int)flag);
    }
    const uint32_t count = rec_dword(record, 0x40);
    const uint32_t window = (0x40u - 0x10u) / (uint32_t)sizeof(void*);
    for (uint32_t i = 0; i < count && i < window; ++i) {
        void* child = rec_ptr(record, (size_t)0x10 + (size_t)i * sizeof(void*));
        if (child != NULL) {
            scene_visibility_hook_call(child, (int)flag);
        }
    }
}

/* The pair band's shared pose apply: position (x, y, 0) plus the corpus's
 * constant (0, 0, -1) orientation, through the ported primitives. */
static void pair_handle_set_pose(void* h, float x, float y)
{
    cam_set_pos((CameraHandle*)h, x, y, 0.0f);
    const float up[3] = { 0.0f, 0.0f, -1.0f };
    cam_set_facing((CameraHandle*)h, up);
}

/* SceneObjectPairSetRenderParameters (FUN_004086c0): the pose dispatch's
 * argument block (x, y, 0 / 0, 0, -1) reads as position + constant facing;
 * the partner only when the +0x7c slot is non-NULL. */
// FUNCTION: LEGORR 0x004086c0
void scene_object_pair_set_render_parameters(void* pair, float x, float y)
{
    pair_handle_set_pose(rec_ptr(pair, 0x04), x, y);
    void* partner = rec_ptr(pair, 0x7c);
    if (partner != NULL) {
        pair_handle_set_pose(partner, x, y);
    }
}

/* SceneObjectPairSetCallbackScalar (FUN_00408710): run the scalar probe on
 * the (x, y) pair with its third argument — the corpus consumes the probe's
 * undefined float return as the camera z; the port pins it to zero (the
 * sole call-site probe, the ported height adapter, answers void). */
// FUNCTION: LEGORR 0x00408710
void scene_object_pair_set_callback_scalar(void* pair, float x, float y,
                                           SceneScalarProbe probe, void* arg)
{
    if (probe != NULL) {
        probe(x, y, arg);
    }
    cam_set_pos((CameraHandle*)rec_ptr(pair, 0x04), x, y, 0.0f);
    void* partner = rec_ptr(pair, 0x7c);
    if (partner != NULL) {
        cam_set_pos((CameraHandle*)partner, x, y, 0.0f);
    }
}


/* AnimationObjectVisibilityGet (FUN_004086a0) — the +4 object's flag bit 3
 * through the ported getter. */
// FUNCTION: LEGORR 0x004086a0
uint32_t animation_object_visibility_get(const void* anim)
{
    return scene_object_flag_bit3_get(rec_ptr(anim, 0x04));
}

/* SceneAssignEntityOwner (FUN_00408380) — owner store on the +4 object
 * through the ported owner set. */
// FUNCTION: LEGORR 0x00408380
void scene_assign_entity_owner(const void* scene, uint32_t owner)
{
    scene_object_owner_set(rec_ptr(scene, 0x04), owner);
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

/* SixResourceSlotsSetState (FUN_0046cfc0) — see record.hpp. The corpus
 * walks record+0x10..0x24 with a 6-count dec/jne loop from a lea'd cursor
 * and loads each slot directly; mirror that shape here. */
// FUNCTION: LEGORR 0x0046cfc0
void six_resource_slots_set_state(const void* record, uint32_t flag)
{
    const void* const* slot =
        (const void* const*)((const char*)record + 0x10);
    for (int remaining = 6; remaining != 0; --remaining, ++slot) {
        const void* handle = *slot;
        if (handle != NULL) {
            scene_visibility_hook_call((void*)handle, (int)flag);
        }
    }
}

/* UIRecordCallbackForward (FUN_0045a630) — see record.hpp. */
// FUNCTION: LEGORR 0x0045a630
void ui_record_callback_forward(const void* const* ui_record,
                                uint32_t* out_a, uint32_t* out_b)
{
    if (ui_record == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    void* handle = rec_ptr(ui_record, 0x00);
    object_state_pair_get(handle, out_a, out_b);
}

/* RecordFlagBitTest (FUN_0046cf40) — see record.hpp. */
// FUNCTION: LEGORR 0x0046cf40
uint32_t record_flag_bit_test(const void* record, uint32_t bit_index,
                              int reject_masked)
{
    /* The corpus shifts blind; the x86 shift masks the count to 5 bits. */
    const uint32_t bit = 1u << bit_index;
    if (reject_masked != 0 && (entity_type_mask_get(record) & bit) != 0u) {
        return 0u;
    }
    uint32_t flags = 0u;
    __builtin_memcpy(&flags, (const char*)record + 0x130, sizeof(flags));
    return flags & bit;
}

/* ObjectPoolListsFreeAll (FUN_00456c60) — see record.hpp. The corpus reads
 * each head through the bounds-checked getter and strides the dword slot
 * blind for the clear; the port reads/clears the native link slot. */
// FUNCTION: LEGORR 0x00456c60
void object_pool_lists_free_all(void* pools)
{
    if (pools == NULL) {
        return; /* deviation: the corpus strides the global blindly */
    }
    for (int i = 0; i < 0x37; ++i) {
        const size_t slot = 0x114 + (size_t)i * sizeof(void*);
        void* node = NULL;
        __builtin_memcpy(&node, (char*)pools + slot, sizeof node);
        while (node != NULL) {
            void* next = NULL;
            __builtin_memcpy(&next, (char*)node + 8, sizeof next);
            engine_heap_free(node);
            node = next;
        }
        void* zero = NULL;
        __builtin_memcpy((char*)pools + slot, &zero, sizeof zero);
    }
}

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

// FUNCTION: LEGORR 0x0047d470
void image_surface_release(TextureSurface* surface)
{
    texture_release(surface);
}

static void menu_resource_entry_release(MenuResourceEntry* entry)
{
    if ((entry->flags & 1u) != 0) {
        engine_heap_free(entry->owned_string);
    } else if ((entry->flags & 2u) != 0) {
        for (int i = 0; i < 3; ++i) {
            image_surface_release(entry->textures[i]);
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
        call engine_heap_alloc
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
        call engine_heap_alloc
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
        call engine_heap_alloc
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
        call engine_heap_free
        add esp, 4
    release_second:
        mov eax, dword ptr [edi + 0x24]
        cmp eax, ebp
        je release_entries
        push eax
        call engine_heap_free
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
        call engine_heap_free
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
        call engine_heap_free
        add esp, 4
    release_record:
        push edi
        call engine_heap_free
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
            record_window_put_u32(rec, 20, (uint32_t)strtol(first, NULL, 10));
            record_window_put_u32(rec, 21, (uint32_t)strtol(second, NULL, 10));
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
    memcpy(text, image_config, length + 1);
    char* tokens[4] = { NULL, NULL, NULL, NULL };
    const int count = sound_variant_split(text, tokens, ",");
    if (count == 0) return 0;
    if (count > 2) {
        const size_t flags_offset = 13 * sizeof(void*);
        const uint32_t flags = rec_dword(item, flags_offset) | 1u;
        __builtin_memcpy((char*)item + flags_offset, &flags, sizeof(flags));
        const float x = util_atof(tokens[1]);
        const float y = util_atof(tokens[2]);
        __builtin_memcpy((char*)item + 14 * sizeof(void*), &x, sizeof(x));
        __builtin_memcpy((char*)item + 15 * sizeof(void*), &y, sizeof(y));
    }
    TextureSurface* surface = (TextureSurface*)string_cache_lookup_or_load(tokens[0]);
    const int field = primary ? 4 : 5;
    *(void**)((char*)item + (size_t)field * sizeof(void*)) = surface;
    if (surface == NULL) return 0;
    if (count == 4 && util_atoi(tokens[3]) == 1) {
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

// FUNCTION: LEGORR 0x00413f30
int filename_extension_classify(const void* record)
{
    const char* suffix = *(const char* const*)record;
    while (*suffix != '.') ++suffix;
    if (strcmp(suffix, ".avi") == 0) return 1;
    if (strcmp(suffix, ".bmp") == 0) return 2;
    if (strcmp(suffix, ".lws") == 0) return 3;
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
        call engine_heap_alloc
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
        call strnicmp_lite
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
    if (name != NULL && menu_ascii_casecmp_n(name, s_menu_null_label, 4) != 0) {
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
        void* grown = engine_heap_alloc((long)((size_t)new_capacity * sizeof(void*)));
        if (grown == NULL) return 0;
        memset(grown, 0, (size_t)new_capacity * sizeof(void*));
        if (items != NULL) {
            memcpy(grown, items, (size_t)count * sizeof(void*));
            engine_heap_free(items);
        }
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
        call engine_heap_alloc
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

/* NestedRecordListRemoveByKeyRecursive (FUN_0047b2e0) — see record.hpp.
 * The corpus re-reads the node's next link after the recursion — a freed
 * deeper node can leave a different successor in the slot — so the port
 * re-reads before the unlink. */
// FUNCTION: LEGORR 0x0047b2e0
int nested_record_list_remove_by_key_recursive(int key, void* node)
{
    if (node == NULL) {
        return 0; /* deviation: the corpus strides the node blindly */
    }
    void* next = NULL;
    __builtin_memcpy(&next, (char*)node + 0x0c, sizeof next);
    if (next != NULL
        && nested_record_list_remove_by_key_recursive(key, next) != 0) {
        void* successor = NULL;
        __builtin_memcpy(&successor, (char*)next + 0x0c, sizeof successor);
        __builtin_memcpy((char*)node + 0x0c, &successor, sizeof successor);
        engine_heap_free(next);
    }
    uint32_t node_key = 0u;
    __builtin_memcpy(&node_key, (char*)node + 0x08, sizeof node_key);
    return node_key == (uint32_t)key;
}

/* KeyedResourceEntryRemove (FUN_0047b2a0) — see record.hpp. The corpus
 * routes through FUN_0047b2e0's +0x08/+0x0c node view; the port's sound
 * association nodes widened that window, so the sweep reproduces the same
 * deep-first promotion/unlink/free walk over the native field slots. */
static int keyed_sweep(int key, void* node)
{
    if (node == NULL) {
        return 0; /* deviation: the corpus strides the node blindly */
    }
    void* next = NULL;
    __builtin_memcpy(&next, (char*)node + 3 * sizeof(void*), sizeof next);
    if (next != NULL
        && keyed_sweep(key, next) != 0) {
        void* successor = NULL;
        __builtin_memcpy(&successor, (char*)next + 3 * sizeof(void*),
                         sizeof successor);
        __builtin_memcpy((char*)node + 3 * sizeof(void*), &successor,
                         sizeof successor);
        engine_heap_free(next);
    }
    uint32_t node_key = 0u;
    __builtin_memcpy(&node_key, (char*)node + 2 * sizeof(void*),
                     sizeof node_key);
    return node_key == (uint32_t)key;
}

// FUNCTION: LEGORR 0x0047b2a0
void keyed_resource_entry_remove(int key)
{
    void* head = sound_buffer_association_head_peek();
    if (head == NULL) {
        return; /* deviation: the corpus strides the head blindly */
    }
    if (keyed_sweep(key, head) != 0) {
        void* successor = NULL;
        __builtin_memcpy(&successor, (char*)head + 3 * sizeof(void*),
                         sizeof successor);
        sound_buffer_association_head_set(successor);
        engine_heap_free(head);
    }
}

/* --- the indexed-record mesh window band (see record.hpp) ---------------- */

static void rec_float_put(void* base, size_t off, float v)
{
    __builtin_memcpy((char*)base + off, &v, sizeof v);
}

static void rec_dword_put(void* base, size_t off, uint32_t v)
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
void indexed_record_entry_read(void* record, int block, uint32_t* field_out,
                               uint32_t* kind_out, uint32_t* count_out,
                               uint32_t* indices_out)
{
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

// FUNCTION: LEGORR 0x00482620
void mesh_segment_vertex_fill(void* record, int block, int first_vertex,
                              int count, const float* positions,
                              const uint32_t triple[3],
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
        /* Pinned corpus quirk: the triple pointer never advances — the
         * SAME triple lands on every vertex. */
        rec_dword_put(v, 0x0c, triple[0]);
        rec_dword_put(v, 0x10, triple[1]);
        rec_dword_put(v, 0x14, triple[2]);
        rec_dword_put(v, 0x18, *pairs);
        rec_dword_put(v, 0x1c, pairs[1]);
        src += 3;
        pairs += 2;
        off += 0x20;
    } while (--n != 0);
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
uint32_t screen_effect_item_opacity_set(void* record, int block,
                                        float opacity, int mode)
{
    const float v = channel_clamp(opacity);
    if (mode != 4) {
        char* data = (char*)indexed_record_data_address_get(record, block);
        if (mode == 5 && data != NULL) {
            rec_float_put(data, 0x44, v);
        }
        return 1u; /* deviation: the corpus strides a missing window blindly */
    }
    char* blk = mesh_block(record, block);
    char* data = (char*)indexed_record_data_address_get(record, block);
    if (blk == NULL) {
        return 1u; /* deviation: the corpus strides the flag word blindly */
    }
    const uint32_t w64 = rec_dword(blk, 0x64);
    const uint32_t w18 = rec_dword(record, 0x18);
    rec_dword_put(blk, 0x6c, rec_dword(blk, 0x6c) & 0xeff7ffffu);
    if (v == 0.0f) {
        if (w64 != 0u || (w18 & 0x1c00u) != 0u) {
            rec_dword_put(blk, 0x6c, rec_dword(blk, 0x6c) | 0x10000000u);
        }
    } else if (!(v == 1.0f || (w64 == 0u && (w18 & 0x1c00u) == 0u))) {
        rec_dword_put(blk, 0x6c, rec_dword(blk, 0x6c) | 0x80000u);
    }
    if (w64 != 0u) {
        if ((w64 & 0xc00u) != 0u) {
            rec_dword_put(blk, 0x6c, rec_dword(blk, 0x6c) | 0x80000u);
        }
        if (data != NULL) {
            rec_float_put(data, 0x10, v);
        }
        return 1u;
    }
    if ((w18 & 0xc00u) != 0u) {
        rec_dword_put(blk, 0x6c, rec_dword(blk, 0x6c) | 0x80000u);
    }
    if (data != NULL) {
        rec_float_put(data, 0x10, v);
    }
    return 1u;
}

/* SceneObjectScalarAdjustByDirection (FUN_004082e0) — see record.hpp. The
 * corpus callers are the config-record parse loop and the simulation flush;
 * the fold itself is a pure record-slot scale (the corpus's x87 double fold
 * lands as the same double arithmetic here). */
// FUNCTION: LEGORR 0x004082e0
void scene_object_scalar_adjust_by_direction(void* record, int direction)
{
    if (record == NULL) {
        return; /* deviation: the corpus strides the record blindly */
    }
    float scalar = 0.0f;
    __builtin_memcpy(&scalar, (const char*)record + 0x80, sizeof scalar);
    float result;
    if (direction == 0) {
        result = (float)((scalar - 0.1) * 10.0 * 0.11111111);
    } else {
        result = (float)(scalar - (1.0 - scalar) * -0.1);
    }
    if (result < 0.0f) {
        result = 0.0f;
    }
    __builtin_memcpy((char*)record + 0x80, &result, sizeof result);
    uint32_t flags = 0u;
    __builtin_memcpy(&flags, (const char*)record + 0x148, sizeof flags);
    flags |= 2u;
    __builtin_memcpy((char*)record + 0x148, &flags, sizeof flags);
}
