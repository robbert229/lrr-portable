// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — corpus record access primitives (see record.hpp). */

#include "record.hpp"
#include "heap.hpp"
#include "sound.hpp"
#include "view.hpp"
#include <math.h>  /* sqrtf for the instance-quad normal window */

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
uint32_t record_field14_get(const void* record)
{
    return rec_dword(record, 0x14);
}

/* ObjectField10Get (FUN_004850e0). */
uint32_t object_field10_get(const void* object)
{
    return rec_dword(object, 0x10);
}

/* RecordFieldD8EqualsOne (FUN_0046c3d0): int == 1, not "nonzero". */
int record_field_d8_equals_one(const void* record)
{
    const int32_t v = (int32_t)rec_dword(record, 0xd8);
    return v == 1;
}

/* ReadIndexedPairSlotDword (FUN_00471a20): base + i*8 + j*4, exactly as
 * the corpus computes it. */
uint32_t read_indexed_pair_slot_dword(const void* base, uint32_t i,
                                      uint32_t j)
{
    return rec_dword(base, (size_t)i * 8u + (size_t)j * 4u);
}

/* VelocitySlotPrimaryObjectGet / SecondaryObjectGet (FUN_00471a40 /
 * FUN_00471a50): indexed slot at +0x18 / +0x24. */
uint32_t velocity_slot_primary_object_get(const void* record,
                                          uint32_t index)
{
    return rec_dword(record, 0x18u + (size_t)index * 4u);
}

uint32_t velocity_slot_secondary_object_get(const void* record,
                                            uint32_t index)
{
    return rec_dword(record, 0x24u + (size_t)index * 4u);
}

/* VelocitySlotVectorRead (FUN_00471c70) — see record.hpp. The corpus
 * copies the three dwords bit-for-bit; the port reads them as floats
 * through the NULL-guarded reader, so the words ride unchanged. */
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
float entity_velocity_slot_value_get(const void* record, int slot)
{
    void* base = entity_velocity_state_record_get(record);
    if (base == NULL || slot < 0) {
        return 0.0f; /* deviation */
    }
    return rec_float(base, 0x98u + (size_t)slot * 4u);
}

/* EntityVelocitySlotValueSet (FUN_00471310): the store counterpart. */
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
int category_value_is_neither_zero_nor_fifteen(int value)
{
    if (value != 0 && value != 0xf) {
        return 1;
    }
    return 0;
}

/* InterpolatedZWithinBounds (FUN_00471330): corpus order — the
 * interpolation, then `v <= target` and `target - t <= v`. */
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
float camera_rotation_get(const void* camera)
{
    return rec_float(camera, 0x84);
}

/* MapCellToWorldCenter (FUN_0044f640, finding 225): the descriptor's
 * int fields convert to float at the use site — reproduced exactly,
 * including the unsigned bounds compare. */
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
int model_child_render_eligibility_check(const void* object)
{
    if (object == NULL) {
        return 0;
    }
    const uint32_t flags = rec_dword(object, 0x6c);
    if ((flags & 1u) == 0 && (flags & 0x10000000u) == 0) {
        return 1;
    }
    return 0;
}

/* EntityTypeFlagBit1Get (FUN_0046d330, findings 357/432). */
uint32_t entity_type_flag_bit1_get(const void* record)
{
    return (rec_dword(record, 0x1e8) & 0xffu) >> 1 & 1u;
}

/* ------------------------------------------------------------------ *
 *  Field-reader sweep (see record.hpp). */

uint32_t object_record_field11c_get(const void* record)
{
    return rec_dword(record, 0x11c);
}

uint32_t object_field_be4_dword_get(const void* record)
{
    return rec_dword(record, 0xbe4);
}

uint32_t entity_type_mask_get(const void* record)
{
    return rec_dword(record, 300);
}

uint32_t pointer_value_get(const void* pointed)
{
    return rec_dword(pointed, 0);
}

uint32_t scene_object_flags_get(const void* scene_object)
{
    return rec_dword(scene_object, 0x70);
}

uint32_t scene_object_owner_get(const void* scene_object)
{
    return rec_dword(scene_object, 0x20);
}

uint32_t scene_object_resource_get(const void* scene_object)
{
    return rec_dword(scene_object, 0x08);
}

uint32_t scene_object_wrapped_pointer_get(const void* wrapped)
{
    return rec_dword(wrapped, 0);
}

uint32_t scene_object_flag_bit3_get(const void* scene_object)
{
    return (rec_dword(scene_object, 0x14) & 0xffu) >> 3 & 1u;
}

int scene_object_mode_is_one(const void* scene_object)
{
    return rec_dword(scene_object, 0x28) == 1u ? 1 : 0;
}

uint32_t animation_object_handle_get(const void* animation_object)
{
    return rec_dword(animation_object, 0x04);
}

uint32_t resource_entry_id_get(const void* entry)
{
    if (entry != NULL) {
        uint16_t v = 0;
        __builtin_memcpy(&v, (const char*)entry + 2, sizeof(v));
        return v;
    }
    return 0u;
}

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

uint32_t indexed_record_flag6c_bit0_get(const void* record, int index)
{
    const char* base = (const char*)rec_ptr(record, 0x08);
    if (base == NULL) {
        return 0u;
    }
    const size_t off = (size_t)index * 0x70u + 0x6cu;
    return rec_dword(base + off, 0) & 1u;
}

void* indexed_record_data_address_get(const void* record, int index)
{
    char* base = (char*)rec_ptr(record, 0x08);
    if (base == NULL) {
        return NULL;
    }
    return (void*)(base + (size_t)((size_t)index * 0x70u + 0x10u));
}

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
uint32_t game_file_system_type_get(const void* handle)
{
    return rec_dword(handle, 0);
}

/* IndexedRecordTransformCopyIn (FUN_00483240, findings 372/431): the
 * inline-block copy into the indexed record's data slot. */
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

void indexed_flag_value_set_one(void* record, uint32_t index)
{
    if (record == NULL) {
        return;
    }
    const uint32_t one = 1u;
    const size_t off = 0x40b4u + (size_t)(index & 0xffu) * 4u;
    __builtin_memcpy((char*)record + off, &one, sizeof(one));
}

void scene_object_resource_handle_set(void* object, uint32_t handle)
{
    if (object == NULL) {
        return;
    }
    __builtin_memcpy((char*)object + 0x2cu, &handle, sizeof(handle));
}

/* --- entity record-field leaves (see record.hpp) --------------------------- */
void record_fields_4_and_14_set(void* record, uint32_t value_a,
                                uint32_t value_b)
{
    if (record == NULL) {
        return;
    }
    __builtin_memcpy((char*)record + 0x04, &value_a, sizeof(value_a));
    __builtin_memcpy((char*)record + 0x14, &value_b, sizeof(value_b));
}

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
void record_field18_set(void* record, uint32_t value)
{
    if (record == NULL) {
        return; /* deviation: the corpus writes blindly */
    }
    __builtin_memcpy((char*)record + 0x18, &value, sizeof(value));
}

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

void object_field14_set(void* record, uint32_t value)
{
    if (record == NULL) {
        return; /* deviation: the corpus writes blindly */
    }
    __builtin_memcpy((char*)record + 0x14, &value, sizeof(value));
}

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
void copy_two_dword_pair_to_outputs(const void* record, uint32_t* out_a,
                                    uint32_t* out_b)
{
    if (record == NULL || out_a == NULL || out_b == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    __builtin_memcpy(out_a, (const char*)record, 4);
    __builtin_memcpy(out_b, (const char*)record + 4, 4);
}

void scene_object_owner_set(void* scene_object, uint32_t owner)
{
    if (scene_object == NULL) {
        return; /* deviation: the corpus writes blindly */
    }
    __builtin_memcpy((char*)scene_object + 0x20, &owner, sizeof(owner));
}

/* --- the record primitive band (see record.hpp) ----------------------------- */
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
void surface_visibility_set(const void* record, uint32_t flag)
{
    scene_visibility_hook_call(rec_ptr(record, 0x00), (int)flag);
}

/* AnimationObjectVisibilitySet (FUN_00406c90) — animation record +4. */
void animation_object_visibility_set(const void* anim, uint32_t flag)
{
    scene_visibility_hook_call(rec_ptr(anim, 0x04), (int)flag);
}

/* SceneObjectPairApplyMode (FUN_00408670) — the +4 object, then the +0x7c
 * partner when nonzero. Deviation: the partner test strides a dword in the
 * corpus; the port's widened slot answers through the pointer. */
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
uint32_t animation_object_visibility_get(const void* anim)
{
    return scene_object_flag_bit3_get(rec_ptr(anim, 0x04));
}

/* SceneAssignEntityOwner (FUN_00408380) — owner store on the +4 object
 * through the ported owner set. */
void scene_assign_entity_owner(const void* scene, uint32_t owner)
{
    scene_object_owner_set(rec_ptr(scene, 0x04), owner);
}

/* SceneCurrentObjectSet (FUN_00477240) — see record.hpp. The store lands
 * on the record's +0x14 slot; Deviation: the corpus strides a dword, the
 * port writes the module's widened pointer slot. */
void scene_current_object_set(void* record, void* object)
{
    if (record != NULL) {
        __builtin_memcpy((char*)record + 0x14, &object, sizeof object);
    }
}

/* SixResourceSlotsSetState (FUN_0046cfc0) — see record.hpp. */
void six_resource_slots_set_state(const void* record, uint32_t flag)
{
    for (int i = 0; i < 6; ++i) {
        void* handle = rec_ptr(record, 0x10 + (size_t)i * sizeof(void*));
        if (handle != NULL) {
            scene_visibility_hook_call(handle, (int)flag);
        }
    }
}

/* UIRecordCallbackForward (FUN_0045a630) — see record.hpp. */
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
uint32_t record_flag_bit_test(const void* record, uint32_t bit_index,
                              int reject_masked)
{
    const uint32_t bit = 1u << (bit_index & 0x1fu);
    if (reject_masked != 0 && (entity_type_mask_get(record) & bit) != 0u) {
        return 0u;
    }
    return rec_dword(record, 0x130) & bit;
}

/* ObjectPoolListsFreeAll (FUN_00456c60) — see record.hpp. The corpus reads
 * each head through the bounds-checked getter and strides the dword slot
 * blind for the clear; the port reads/clears the native link slot. */
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
                     sizeof(void*));
}

/* BoundedFloatRecordCreate (FUN_00410b90) — see record.hpp. */
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
 * Field order follows the corpus stores: bounds land late, payload first. */
void* menu_item_numeric_range_record_allocate(int kind, int min_v, int max_v,
                                              uint32_t f3, uint32_t f4,
                                              uint32_t f5, uint32_t f6,
                                              uint32_t f7, uint32_t f8,
                                              uint32_t f9, uint32_t f10,
                                              uint32_t f11, uint32_t f12,
                                              uint32_t f13, uint32_t f14)
{
    if (!(min_v < max_v)) {
        return NULL;
    }
    const uint32_t bounds_lo = (uint32_t)min_v, bounds_hi = (uint32_t)max_v;
    void* rec = engine_heap_alloc(15 * sizeof(void*));
    if (rec == NULL) {
        return NULL;
    }
    const uint32_t kind_w = (uint32_t)kind;
    record_window_put(rec, 0, &kind_w);
    record_window_put(rec, 3, &f3);
    record_window_put(rec, 4, &f4);
    record_window_put(rec, 5, &f5);
    record_window_put(rec, 6, &f6);
    record_window_put(rec, 7, &f7);
    record_window_put(rec, 8, &f8);
    record_window_put(rec, 9, &f9);
    record_window_put(rec, 10, &f10);
    record_window_put(rec, 11, &f11);
    record_window_put(rec, 12, &f12);
    record_window_put(rec, 1, &bounds_lo);
    record_window_put(rec, 2, &bounds_hi);
    record_window_put(rec, 13, &f13);
    record_window_put(rec, 14, &f14);
    return rec;
}

/* NestedRecordListRemoveByKeyRecursive (FUN_0047b2e0) — see record.hpp.
 * The corpus re-reads the node's next link after the recursion — a freed
 * deeper node can leave a different successor in the slot — so the port
 * re-reads before the unlink. */
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
