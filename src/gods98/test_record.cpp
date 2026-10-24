// SPDX-License-Identifier: LGPL-3.0-only
// Corpus record access primitives (findings 305/332/354/356/367/378/460).
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "gods98/record.hpp"

static int s_failures = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) {                                                         \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg);      \
            ++s_failures;                                                      \
        }                                                                      \
    } while (0)

/* A record large enough for the widest slot the primitives read. */
typedef struct ProbeRecord {
    char storage[0x200];
} ProbeRecord;

static void poke_dword(void* base, size_t off, uint32_t v)
{
    __builtin_memcpy((char*)base + off, &v, sizeof(v));
}

static void poke_float(void* base, size_t off, float v)
{
    __builtin_memcpy((char*)base + off, &v, sizeof(v));
}

static void test_field_reads(void)
{
    ProbeRecord r;
    memset(&r, 0, sizeof(r));

    poke_dword(&r, 0x14, 0x11223344u);
    CHECK(record_field14_get(&r) == 0x11223344u, "field14 reads +0x14");
    poke_dword(&r, 0x14, 0);
    CHECK(record_field14_get(&r) == 0, "field14 sees the cleared slot");
    /* NULL guard answers 0 (documented deviation). */
    CHECK(record_field14_get(NULL) == 0, "NULL record answers zero");

    poke_dword(&r, 0x10, 0xcafebabeu);
    CHECK(object_field10_get(&r) == 0xcafebabeu, "field10 reads +0x10");
    CHECK(object_field10_get(NULL) == 0, "NULL object answers zero");

    /* D8 equality is exact — 1, not "nonzero". */
    poke_dword(&r, 0xd8, 1);
    CHECK(record_field_d8_equals_one(&r) == 1, "d8 == 1");
    poke_dword(&r, 0xd8, 2);
    CHECK(record_field_d8_equals_one(&r) == 0, "d8 == 2 is not one");
    poke_dword(&r, 0xd8, 1u << 31); /* -0x80000000 as int */
    CHECK(record_field_d8_equals_one(&r) == 0, "the sign bit is not one");
    poke_dword(&r, 0xd8, 0);
    CHECK(record_field_d8_equals_one(&r) == 0, "zero is not one");

    /* Category filter: 0 and 15 fail, everything else passes. */
    CHECK(category_value_is_neither_zero_nor_fifteen(1) == 1, "one passes");
    CHECK(category_value_is_neither_zero_nor_fifteen(0) == 0,
          "zero fails");
    CHECK(category_value_is_neither_zero_nor_fifteen(0xf) == 0,
          "fifteen fails");
    CHECK(category_value_is_neither_zero_nor_fifteen(16) == 1,
          "sixteen passes");
}

static void test_indexed_slots(void)
{
    ProbeRecord r;
    memset(&r, 0, sizeof(r));

    /* Pair table: row i stride 8, column j stride 4 within the row. */
    poke_dword(&r, 0, 100u);
    poke_dword(&r, 4, 101u);
    poke_dword(&r, 8, 200u);
    poke_dword(&r, 12, 201u);
    CHECK(read_indexed_pair_slot_dword(&r, 0, 0) == 100u, "row 0 column 0");
    CHECK(read_indexed_pair_slot_dword(&r, 0, 1) == 101u, "row 0 column 1");
    CHECK(read_indexed_pair_slot_dword(&r, 1, 1) == 201u, "row 1 column 1");
    CHECK(read_indexed_pair_slot_dword(NULL, 1, 1) == 0, "NULL base answers zero");

    /* Velocity slot arrays at +0x18 / +0x24. */
    poke_dword(&r, 0x18, 0x111u);
    poke_dword(&r, 0x1c, 0x112u);
    poke_dword(&r, 0x24, 0x221u);
    poke_dword(&r, 0x28, 0x222u);
    CHECK(velocity_slot_primary_object_get(&r, 0) == 0x111u,
          "primary slot 0");
    CHECK(velocity_slot_primary_object_get(&r, 1) == 0x112u,
          "primary slot 1");
    CHECK(velocity_slot_secondary_object_get(&r, 0) == 0x221u,
          "secondary slot 0");
    CHECK(velocity_slot_secondary_object_get(&r, 1) == 0x222u,
          "secondary slot 1");
    /* The slot arrays do not overlap: +0x1c is not secondary slot 1. */
    CHECK(velocity_slot_secondary_object_get(&r, 0) != 0x112u,
          "the two slot windows stay separate");
}

/* CameraRotationGet (FUN_00436b79): the opaque +0x84 float. */
static void test_camera_rotation(void)
{
    ProbeRecord cam;
    memset(&cam, 0, sizeof(cam));
    poke_float(&cam, 0x84, 1.25f);
    CHECK(camera_rotation_get(&cam) == 1.25f, "rotation reads +0x84");
    poke_float(&cam, 0x84, -3.5f);
    CHECK(camera_rotation_get(&cam) == -3.5f, "rotation sees the update");
    CHECK(camera_rotation_get(NULL) == 0.0f, "NULL camera answers zero");
}

/* MapCellToWorldCenter (FUN_0044f640): int descriptor fields convert to
 * float at the use site; unsigned bounds compare. */
static void test_map_cell_center(void)
{
    uint8_t map[0x20];
    memset(map, 0, sizeof(map));
    poke_dword(map, 0x00, 4u);          /* width  */
    poke_dword(map, 0x04, 4u);          /* height */
    poke_dword(map, 0x10, 2u);          /* scale  */
    poke_dword(map, 0x18, -10);         /* origin X */
    poke_dword(map, 0x1C, 100);         /* origin Y */

    float wx = 0.0f, wz = 0.0f;
    CHECK(map_cell_to_world_center(map, 1, 2, &wx, &wz) == 1,
          "in-bounds cell converts");
    CHECK(wx == 2.0f * 0.5f + 1.0f * 2.0f - 10.0f, "world X formula");
    CHECK(wz == 100.0f - (2.0f * 0.5f + 2.0f * 2.0f), "world Z formula");

    CHECK(map_cell_to_world_center(map, 4, 0, &wx, &wz) == 0,
          "x == width fails (corpus compare)");
    CHECK(map_cell_to_world_center(map, 0, 4, &wx, &wz) == 0,
          "y == height fails");
    /* Negative indices enter as huge unsigned values. */
    CHECK(map_cell_to_world_center(map, 0xffffffffu, 0, &wx, &wz) == 0,
          "index -1 reads as unsigned-huge and fails");
    /* NULL descriptor answers 0 (documented deviation). */
    CHECK(map_cell_to_world_center(NULL, 0, 0, &wx, &wz) == 0,
          "NULL map answers outside");
}

/* TaskFlagBitQuery (FUN_00408970): mask 1 << (bit & 31) over +0x98, with
 * the suppress-if-set form. */
static void test_task_flag_bit(void)
{
    ProbeRecord task;
    memset(&task, 0, sizeof(task));
    poke_dword(&task, 0x98, 0x104u); /* bits 2 and 8 set */

    CHECK(task_flag_bit_query(&task, 2, 0) == 0x4u, "bit 2 answers its mask");
    CHECK(task_flag_bit_query(&task, 8, 0) == 0x100u, "bit 8 answers its mask");
    CHECK(task_flag_bit_query(&task, 3, 0) == 0u, "clear bit answers zero");
    CHECK(task_flag_bit_query(&task, 2, 1) == 0u,
          "suppress form zeroes a set bit");
    CHECK(task_flag_bit_query(&task, 3, 1) == 0u,
          "suppress form keeps the clear-bit answer");
    /* The corpus masks the bit index: 34 behaves like 2. */
    CHECK(task_flag_bit_query(&task, 34, 0) == 0x4u, "bit index wraps by &31");
    /* Bit 31 corner. */
    poke_dword(&task, 0x98, 1u << 31);
    CHECK(task_flag_bit_query(&task, 31, 0) == (1u << 31), "top bit answers");
    CHECK(task_flag_bit_query(&task, 31, 1) == 0u, "top bit suppresses");
    CHECK(task_flag_bit_query(NULL, 0, 0) == 0u, "NULL task answers zero");
}

static void test_interpolated_z(void)
{
    /* a = 10 at +8, b = 4 at +8: v = 4 * 2.5 + 10 = 20. */
    char a[16], b[16];
    memset(a, 0, sizeof(a));
    memset(b, 0, sizeof(b));
    poke_float(a, 8, 10.0f);
    poke_float(b, 8, 4.0f);
    CHECK(interpolated_z_within_bounds(a, b, 1.0f, 20.0f, 2.5f) == 1,
          "interpolated value equals the target");
    CHECK(interpolated_z_within_bounds(a, b, 0.0f, 20.0f, 2.5f) == 1,
          "zero tolerance at exact equality passes");
    /* v sits below target: within the window when v >= target - t. */
    CHECK(interpolated_z_within_bounds(a, b, 5.0f, 25.0f, 2.5f) == 1,
          "inside the lower edge of the window");
    CHECK(interpolated_z_within_bounds(a, b, 1.0f, 25.0f, 2.5f) == 0,
          "below target - t fails");
    /* v above the target fails (the corpus keeps only v <= target). */
    CHECK(interpolated_z_within_bounds(a, b, 50.0f, 15.0f, 2.5f) == 0,
          "v above the target fails the first gate");
    CHECK(interpolated_z_within_bounds(NULL, b, 1.0f, 20.0f, 2.5f) == 0,
          "NULL record_a answers outside");
}

int main(void)
{
    test_field_reads();
    test_indexed_slots();
    test_camera_rotation();
    test_map_cell_center();
    test_task_flag_bit();
    test_interpolated_z();
    if (s_failures != 0) {
        fprintf(stderr, "record_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("record_test: PASS\n");
    return 0;
}
