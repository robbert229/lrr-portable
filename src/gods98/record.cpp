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
