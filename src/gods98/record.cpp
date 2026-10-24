// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — corpus record access primitives (see record.hpp). */

#include "record.hpp"

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
