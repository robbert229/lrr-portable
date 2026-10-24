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
    char storage[0xc00]; /* fits the +0xbe4 sweep field */
} ProbeRecord;

/* Table-slot holder for the indexed readers (corpus record +0x8 slot). */
typedef struct PokeTableRecord {
    char     prefix[0x08];
    void*    table;
    char     rest[0x200];
} PokeTableRecord;

static void poke_ptr(void* base, size_t off, void* v)
{
    __builtin_memcpy((char*)base + off, &v, sizeof(v));
}

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

/* Field-reader sweep (findings 305/337/356/361/372/373/376/381/393/
 * 411/430/432/435) — the plain offset readers. */
static void test_reader_sweep(void)
{
    ProbeRecord r;
    memset(&r, 0, sizeof(r));

    poke_dword(&r, 0x11c, 0xa1a1a1a1u);
    CHECK(object_record_field11c_get(&r) == 0xa1a1a1a1u, "+0x11c");
    poke_dword(&r, 300, 0x12345678u);
    CHECK(entity_type_mask_get(&r) == 0x12345678u, "+300 (mask)");
    poke_dword(&r, 0x70, 0x00ffffffu);
    CHECK(scene_object_flags_get(&r) == 0x00ffffffu, "scene flags +0x70");
    poke_dword(&r, 0x20, 0x55555555u);
    CHECK(scene_object_owner_get(&r) == 0x55555555u, "scene owner +0x20");
    poke_dword(&r, 0x08, 0x77777777u);
    CHECK(scene_object_resource_get(&r) == 0x77777777u, "scene resource +8");

    /* Word at +2 zero-extends; +0xbe4 needs a wide record. */
    char wide[0xc00];
    memset(wide, 0, sizeof(wide));
    poke_dword(wide, 0xbe4, 0xdeadbeefu);
    CHECK(object_field_be4_dword_get(wide) == 0xdeadbeefu, "wide field +0xbe4");

    char two[8];
    memset(two, 0, sizeof(two));
    two[2] = (char)0xab;
    CHECK(resource_entry_id_get(two) == 0xabu, "entry id word zero-extends");

    /* Bit/flag corners. */
    poke_dword(&r, 0x14, 0x100u); /* bit 3 lives only in the low byte */
    CHECK(scene_object_flag_bit3_get(&r) == 0u, "bit 3 of the LOW byte");
    poke_dword(&r, 0x14, 0x08u);
    CHECK(scene_object_flag_bit3_get(&r) == 1u, "bit 3 set");
    poke_dword(&r, 0x14, 0x80u);
    CHECK(scene_object_flag_bit3_get(&r) == 0u, "bit 7 is not bit 3");
    poke_dword(&r, 0x28, 1u);
    CHECK(scene_object_mode_is_one(&r) == 1, "mode 1 is one");
    poke_dword(&r, 0x28, 2u);
    CHECK(scene_object_mode_is_one(&r) == 0, "mode 2 is not one");

    /* Pointer-deref twins: read through the given address. */
    poke_dword(&r, 0x04, 0x40000000u);
    CHECK(animation_object_handle_get(&r) == 0x40000000u, "anim handle +4");
    uint32_t target = 0xcafef00du;
    CHECK(pointer_value_get(&target) == 0xcafef00du, "pointer_value reads +0");
    CHECK(scene_object_wrapped_pointer_get(&target) == 0xcafef00du,
          "wrapped pointer reads its target's +0");

    /* NULL corners (deviations: zero answers). */
    CHECK(object_record_field11c_get(NULL) == 0u, "NULL record reads zero");
    CHECK(object_field_be4_dword_get(NULL) == 0u, "NULL wide field reads zero");
    CHECK(entity_type_mask_get(NULL) == 0u, "NULL mask reads zero");
    CHECK(resource_entry_id_get(NULL) == 0u, "NULL entry id reads zero");
    CHECK(pointer_value_get(NULL) == 0u, "NULL pointer reads zero");
}

/* Indexed readers: the +0x8 table slot, the stride-0x70 records, the
 * word table at +0x41c, and the coordinate list. */
static void test_indexed_sweep(void)
{
    /* Table of three stride-0x70 records: bit-0 flags 1, 0, 1. */
    uint8_t table[3 * 0x70];
    memset(table, 0, sizeof(table));
    poke_dword(table + 0 * 0x70, 0x6cu, 1u);
    poke_dword(table + 1 * 0x70, 0x6cu, 0u);
    poke_dword(table + 2 * 0x70, 0x6cu, 3u); /* bit 1 alongside bit 0 */
    PokeTableRecord holder;
    memset(&holder, 0, sizeof(holder));
    holder.table = table;
    CHECK(indexed_record_flag6c_bit0_get(&holder, 0) == 1u, "record 0 bit0");
    CHECK(indexed_record_flag6c_bit0_get(&holder, 1) == 0u, "record 1 clear");
    CHECK(indexed_record_flag6c_bit0_get(&holder, 2) == 1u,
          "record 2 answers bit 0 through the mask");
    CHECK(indexed_record_data_address_get(&holder, 1)
              == (void*)(table + 1 * 0x70 + 0x10),
          "data address is base + 0x10 + index*stride");
    holder.table = NULL;
    CHECK(indexed_record_flag6c_bit0_get(&holder, 0) == 0u,
          "empty table answers zero");
    CHECK(indexed_record_data_address_get(&holder, 0) == NULL,
          "empty table answers NULL");
    CHECK(indexed_record_data_address_get(NULL, 0) == NULL,
          "NULL record answers NULL");

    /* Word table: corpus order is (index, table); +0x41c start. */
    static uint8_t words[0x41c + 256 * 2];
    memset(words, 0, sizeof(words));
    uint16_t w1 = 0x1122, w255 = 0xffee;
    __builtin_memcpy(words + 0x41c + 1 * 2, &w1, sizeof(w1));
    __builtin_memcpy(words + 0x41c + 255 * 2, &w255, sizeof(w255));
    CHECK(indexed_word_table_lookup(1u, words) == 0x1122u, "index 1 word");
    CHECK(indexed_word_table_lookup(255u, words) == 0xffeeu,
          "the index masks low byte: 255");
    CHECK(indexed_word_table_lookup(256u, words) == 0u,
          "index 256 masks to 0 (untouched cell)");

    /* Coordinate pair list: an explicit byte layout — count at +0x20,
     * native pointer slots from +0x2c (the deviation's shape). */
    uint32_t entry_a[4] = { 10, 20, 0, 0 };
    uint32_t entry_b[4] = { 30, 40, 0, 0 };
    uint8_t mgr[0x2c + 16 * 8];
    memset(mgr, 0, sizeof(mgr));
    poke_dword(mgr, 0x20, 2u);
    poke_ptr(mgr, 0x2c + 0 * 8, entry_a);
    poke_ptr(mgr, 0x2c + 1 * 8, entry_b);
    CHECK(count_matching_coordinates_in_list(mgr, 10, 20) == 1,
          "the matching pair counts once");
    CHECK(count_matching_coordinates_in_list(mgr, 10, 21) == 0,
          "both coordinates must match");
    CHECK(count_matching_coordinates_in_list(mgr, 30, 40) == 1,
          "the second entry matches too");
    poke_dword(mgr, 0x20, 3u);
    poke_ptr(mgr, 0x2c + 2 * 8, entry_a);
    CHECK(count_matching_coordinates_in_list(mgr, 10, 20) == 2,
          "duplicate entries both count");
    poke_dword(mgr, 0x20, 0u);
    CHECK(count_matching_coordinates_in_list(mgr, 10, 20) == 0,
          "zero count walks nothing");
    CHECK(count_matching_coordinates_in_list(NULL, 0, 0) == 0,
          "NULL manager answers zero");
}

/* Straggler readers: the movie readiness byte and the game-file
 * system-kind word. */
static void test_straggler_reads(void)
{
    ProbeRecord movie;
    memset(&movie, 0, sizeof(movie));
    movie.storage[0x99] = 1;
    CHECK(avi_player_ready_get(&movie) == 1u, "ready byte at +0x99");
    movie.storage[0x99] = 0;
    CHECK(avi_player_ready_get(&movie) == 0u, "cleared byte answers zero");
    /* The byte reads alone: neighbours must not leak in. */
    poke_dword(&movie, 0x94, 0xffffffffu); /* ends below +0x99 */
    poke_dword(&movie, 0x9c, 0xffffffffu);
    CHECK(avi_player_ready_get(&movie) == 0u, "neighbour dwords stay out");
    movie.storage[0x99] = 0xff;
    CHECK(avi_player_ready_get(&movie) == 0xffu, "the byte answers full");
    CHECK(avi_player_ready_get(NULL) == 0u, "NULL movie answers zero");

    uint32_t kind = 2u; /* 0 = wad-backed in the corpus's callers */
    uint8_t handle[16];
    memset(handle, 0, sizeof(handle));
    __builtin_memcpy(handle, &kind, 4);
    CHECK(game_file_system_type_get(handle) == 2u, "the type dword reads +0");
    CHECK(game_file_system_type_get(NULL) == 0u, "NULL handle answers zero");
}

/* IndexedRecordTransformCopyIn (FUN_00483240, findings 372/431): the
 * inline-block 0x50-byte store at the indexed record's data address. */
static void test_transform_copy_in(void)
{
    /* One record's worth of table: everything the copy touches. */
    uint8_t table[0x10 + 0x70 + 0x50];
    memset(table, 0, sizeof(table));
    PokeTableRecord holder;
    memset(&holder, 0, sizeof(holder));
    holder.table = table;

    uint32_t src[20];
    for (int i = 0; i < 20; ++i) {
        src[i] = 0x01020300u + (uint32_t)i;
    }
    CHECK(indexed_record_transform_copy_in(&holder, 0, src) == 1u,
          "the inline copy answers success");
    for (int i = 0; i < 20; ++i) {
        char* dst = (char*)table + 0x10 + i * 4;
        uint32_t got = 0;
        __builtin_memcpy(&got, dst, 4);
        if (got != 0x01020300u + (uint32_t)i) {
            CHECK(0, "all twenty dwords land in order");
            return;
        }
    }
    CHECK(1, "all twenty dwords land in order");
    /* Only the record's block moves: the +0x10 header stays put. */
    CHECK(table[4] == 0, "the copy starts after the +0x10 header");
    /* The address the copy targets is the reader's answer. */
    CHECK(indexed_record_data_address_get(&holder, 0) == table + 0x10,
          "copy target equals the data-address reader");
    /* Deviation corners: success, no crash, no write. */
    CHECK(indexed_record_transform_copy_in(NULL, 0, src) == 1u,
          "NULL record answers success (deviation)");
    CHECK(indexed_record_transform_copy_in(&holder, 0, NULL) == 1u,
          "NULL source answers success and stores nothing");

    holder.table = NULL;
    CHECK(indexed_record_transform_copy_in(&holder, 0, src) == 1u,
          "empty table answers success without touching anything");
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
    test_reader_sweep();
    test_indexed_sweep();
    test_transform_copy_in();
    test_straggler_reads();
    test_interpolated_z();
    if (s_failures != 0) {
        fprintf(stderr, "record_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("record_test: PASS\n");
    return 0;
}
