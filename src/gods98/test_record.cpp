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
    test_interpolated_z();
    if (s_failures != 0) {
        fprintf(stderr, "record_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("record_test: PASS\n");
    return 0;
}
