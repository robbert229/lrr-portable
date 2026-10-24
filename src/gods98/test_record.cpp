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

/* TaskField84GetAndField88Out (FUN_00408360): the +0x84/+0x88 pair. */
static void test_task_pair_reads(void)
{
    ProbeRecord task;
    memset(&task, 0, sizeof(task));
    poke_dword(&task, 0x84, 0x11111111u);
    poke_dword(&task, 0x88, 0x22222222u);

    uint32_t out = 0;
    CHECK(task_field84_get_and_field88_out(&task, &out) == 0x11111111u,
          "the +0x84 dword is the answer");
    CHECK(out == 0x22222222u, "the +0x88 dword lands in the output");

    /* No output pointer: only the answer, nothing to trip. */
    CHECK(task_field84_get_and_field88_out(&task, NULL) == 0x11111111u,
          "the answer stands without the output");

    /* NULL task: zero answer, output untouched (deviation documented). */
    out = 0xdeadbeefu;
    CHECK(task_field84_get_and_field88_out(NULL, &out) == 0u,
          "NULL task answers zero");
    CHECK(out == 0xdeadbeefu, "NULL task writes nothing through the output");
}

/* PackedGridCellValueGet (FUN_0040b850): nibble-indexed grid cells. */
static void test_packed_grid_cell(void)
{
    static uint32_t cells[8 * 16 + 16];
    for (size_t i = 0; i < sizeof(cells) / sizeof(cells[0]); ++i) {
        cells[i] = 0x100u + (uint32_t)i;
    }
    typedef struct GridProbe {
        uint32_t width;
        uint32_t pad;
        void* cells;
    } GridProbe;
    GridProbe grid;
    grid.width = 8u;
    grid.pad = 0u;
    grid.cells = cells;

    /* index = low nibble * width + high nibble. */
    CHECK(packed_grid_cell_value_get(&grid, 0x00u) == cells[0],
          "cell 0 is the origin");
    /* Low nibble steps by the width, high nibble is the short step —
     * finding 373's y*width + x decode of the packed byte. */
    CHECK(packed_grid_cell_value_get(&grid, 0x10u) == cells[1],
          "the high nibble is the short step");
    CHECK(packed_grid_cell_value_get(&grid, 0x01u) == cells[8],
          "the low nibble steps by the width");
    CHECK(packed_grid_cell_value_get(&grid, 0xf7u) == cells[7 * 8u + 15],
          "the corner cell (low 7, high 15)");
    /* Above byte 0xff the nibble mask still rules (corpus masks first). */
    CHECK(packed_grid_cell_value_get(&grid, 0x1f0u)
              == cells[0u * 8u + ((0x1f0u & 0xffu) >> 4)],
          "bits above byte zero are masked before decoding");

    /* NULL grid answers zero — the corpus's own guard. */
    CHECK(packed_grid_cell_value_get(NULL, 0x00u) == 0u,
          "NULL grid answers zero");
    /* NULL cell slot answers zero too (the port's pointer-slot read). */
    grid.cells = NULL;
    CHECK(packed_grid_cell_value_get(&grid, 0x00u) == 0u,
          "an empty cell slot answers zero");
}

/* ObjectPoolListHeadGet (FUN_00456c30): +0x114 slot table, corpus bounds. */
static void test_object_pool_head(void)
{
    ProbeRecord pools;
    memset(&pools, 0, sizeof(pools));
    poke_dword(&pools, 0x114 + 0 * 4, 0x11111111u);
    poke_dword(&pools, 0x114 + 54 * 4, 0x22222222u);

    CHECK(object_pool_list_head_get(&pools, 0) == 0x11111111,
          "head slot zero reads");
    CHECK(object_pool_list_head_get(&pools, 54) == 0x22222222,
          "the 55th head reads (corpus bound is exclusive)");
    CHECK(object_pool_list_head_get(&pools, 55) == 0,
          "index 55 is out of range");
    CHECK(object_pool_list_head_get(&pools, -1) == 0,
          "index -1 is out of range");
    CHECK(object_pool_list_head_get(&pools, 1) == 0,
          "an empty slot answers zero");
    CHECK(object_pool_list_head_get(NULL, 0) == 0,
          "NULL pool table answers zero");
}

/* ModelChildRenderEligibilityCheck (FUN_00483a70): the +0x6c two-bit gate. */
static void test_model_child_eligibility(void)
{
    ProbeRecord object;
    memset(&object, 0, sizeof(object));
    CHECK(model_child_render_eligibility_check(&object) == 1,
          "no gate bits: eligible");
    poke_dword(&object, 0x6c, 0x1u);
    CHECK(model_child_render_eligibility_check(&object) == 0,
          "bit 0x1 blocks");
    poke_dword(&object, 0x6c, 0x10000000u);
    CHECK(model_child_render_eligibility_check(&object) == 0,
          "bit 0x10000000 blocks");
    poke_dword(&object, 0x6c, 0x00000002u);
    CHECK(model_child_render_eligibility_check(&object) == 1,
          "other flag bits stay eligible");
    CHECK(model_child_render_eligibility_check(NULL) == 0,
          "NULL object answers not-eligible (deviation)");
}

/* EntityTypeFlagBit1Get (FUN_0046d330): bit 1 of the +0x1e8 flag byte. */
static void test_entity_flag_bit1(void)
{
    ProbeRecord r;
    memset(&r, 0, sizeof(r));
    poke_dword(&r, 0x1e8, 0x2u);
    CHECK(entity_type_flag_bit1_get(&r) == 1u, "bit 1 of the flag byte");
    poke_dword(&r, 0x1e8, 0x0u);
    CHECK(entity_type_flag_bit1_get(&r) == 0u, "clear byte answers zero");
    poke_dword(&r, 0x1e8, 0x1u);
    CHECK(entity_type_flag_bit1_get(&r) == 0u, "bit 0 does not count");
    poke_dword(&r, 0x1e8, 0x102u);
    CHECK(entity_type_flag_bit1_get(&r) == 1u, "high bytes do not matter");
    poke_dword(&r, 0x1e4, 0xffu); /* byte BELOW +0x1e8 stays ignored */
    poke_dword(&r, 0x1ec, 0xffu); /* byte above too */
    CHECK(entity_type_flag_bit1_get(&r) == 1u, "the read is exactly +0x1e8");
    CHECK(entity_type_flag_bit1_get(NULL) == 0u, "NULL answers zero");
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
    test_task_pair_reads();
    test_packed_grid_cell();
    test_object_pool_head();
    test_model_child_eligibility();
    test_entity_flag_bit1();
    test_interpolated_z();
    if (s_failures != 0) {
        fprintf(stderr, "record_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("record_test: PASS\n");
    return 0;
}
