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
