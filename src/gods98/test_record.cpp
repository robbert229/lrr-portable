// SPDX-License-Identifier: LGPL-3.0-only
// Corpus record access primitives (findings 305/332/354/356/367/378/460).
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "gods98/record.hpp"
#include "gods98/heap.hpp"

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

/* Grid-descriptor coordinate leaves (findings wave 2026-10-24). The
 * descriptor: w/h at +0/+4, int scale +0x10, int origins +0x18/+0x1C. */
static void test_descriptor_coordinates(void)
{
    uint8_t desc[0x20];
    memset(desc, 0, sizeof(desc));
    poke_dword(desc, 0x00, 4u);
    poke_dword(desc, 0x04, 4u);
    poke_dword(desc, 0x10, 2u);   /* scale as an int (bounds check's view) */
    poke_dword(desc, 0x18, 10u);
    poke_dword(desc, 0x1C, 10u);

    int cx = 0, cy = 0;
    /* In range: cell coordinates store and the leaf answers 1. */
    CHECK(world_to_terrain_cell_bounds_check(desc, 1.5f, 2.5f, &cx, &cy, NULL)
              == 1,
          "an in-range world point checks in");
    CHECK(cx == 1 && cy == 2, "cells truncate toward zero");
    /* Truncation on negatives: -2.5 truncates to -2 (toward zero). */
    CHECK(world_to_terrain_cell_bounds_check(desc, -2.5f, 0.0f, &cx, &cy, NULL)
              == 0,
          "cell -2 fails the -1 bound");
    CHECK(cx == -2 && cy == 0, "out-of-range coordinates still store");

    /* Subcell: px = (wx - (cx*bs + ox)) / (bs/2); the y partner folds
     * past 1 and the pair answers by lowest. Hand-computed:
     * wx=1.5 -> px = (1.5 - 12)/1 = -10.5; wy=2.5 -> py = -(2.5-6)/1
     * = 3.5 -> fold 2-3.5 = -1.5 -> lowest -10.5. */
    float sub = 0.0f;
    CHECK(world_to_terrain_cell_bounds_check(desc, 1.5f, 2.5f, &cx, &cy, &sub)
              == 1,
          "the subcell path answers in range");
    CHECK(sub == -10.5f, "the subcell is the lowest folded pair value");

    /* Clamp-and-store. */
    grid_coordinate_clamp_and_store(desc, 1.9f, -0.5f, &cx, &cy);
    CHECK(cx == 1 && cy == 0, "clamps floor at zero (truncated first)");
    grid_coordinate_clamp_and_store(desc, 99.0f, 99.0f, &cx, &cy);
    CHECK(cx == 3 && cy == 3, "clamps below the dimensions");
    grid_coordinate_clamp_and_store(desc, -2.9f, 5.0f, &cx, &cy);
    CHECK(cx == 0 && cy == 3, "negative floors, positive clamps");
    grid_coordinate_clamp_and_store(desc, 1.0f, 1.0f, NULL, NULL);
    CHECK(cx == 0 && cy == 3, "NULL stores change nothing");

    /* The forwarder-shaped leaf: scale +0x10 read as FLOAT here, so the
     * fixture stores float 1.0 while the center converts the same bits
     * as an int (the corpus's own mismatch, reproduced faithfully). */
    uint8_t desc2[0x20];
    memset(desc2, 0, sizeof(desc2));
    poke_dword(desc2, 0x00, 4u);
    poke_dword(desc2, 0x04, 4u);
    float one = 1.0f;
    uint32_t one_bits;
    __builtin_memcpy(&one_bits, &one, sizeof(one_bits));
    poke_dword(desc2, 0x10, one_bits);
    poke_dword(desc2, 0x18, 0u);
    poke_dword(desc2, 0x1C, 0u);

    float point[2] = { 1.5f, 1.5f };
    float cell[2] = { 9, 9 };
    float offset[2] = { 9, 9 };
    CHECK(terrain_world_point_to_cell(desc2, point, cell, offset) == 0u,
          "the leaf answers 0 (the corpus never answers otherwise)");
    CHECK(cell[0] == 1.0f && cell[1] == 1.0f, "the cell coordinates stream");
    /* Mirror the center computation for the offset check. */
    float si = (float)(int)one_bits; /* = 1.0f: bits-as-int casts once */
    float center_x = si * 0.5f + (float)1 * si + 0.0f;
    float center_y = 0.0f - (si * 0.5f + (float)1 * si);
    CHECK(offset[0] == 1.5f - center_x + 0.5f, "x offsets by +1/2 step");
    CHECK(offset[1] == 1.5f - center_y + 0.5f, "y offsets by +1/2 step");

    /* Corners: NULL point answers 0 untouched; out-of-range leaves both
     * outputs untouched. */
    CHECK(terrain_world_point_to_cell(desc2, NULL, cell, offset) == 0u,
          "NULL point is safe");
    float far_point[2] = { 99.0f, 0.0f };
    CHECK(terrain_world_point_to_cell(desc2, far_point, cell, offset) == 0u,
          "the out-of-range point answers 0");
    CHECK(offset[0] == 1.5f - center_x + 0.5f,
          "out of range changes no output");

    /* NULL-map deviations (documented). */
    cx = 42;
    CHECK(world_to_terrain_cell_bounds_check(NULL, 1.0f, 1.0f, &cx, NULL, NULL)
              == 0,
          "NULL map checks nothing out");
    CHECK(cx == 1, "the cell coordinate still stores through NULL");
    grid_coordinate_clamp_and_store(NULL, 1.0f, 1.0f, &cx, NULL);
    CHECK(cx == -1, "a zero width clamps the index below zero (faithful)");
}

/* Indexed flag slot and resource-handle writers (findings 354/373/450). */
static void test_flag_and_handle_writers(void)
{
    static uint8_t big[0x40b4 + 256 * 4];
    memset(big, 0xAA, sizeof(big));

    indexed_flag_value_set_one(big, 5u);
    uint32_t v = 0;
    __builtin_memcpy(&v, big + 0x40b4 + 5 * 4, 4);
    CHECK(v == 1u, "the slot answers one");
    v = 0xAAAAAAAAu;
    __builtin_memcpy(&v, big + 0x40b4 + 6 * 4, 4);
    CHECK(v == 0xAAAAAAAAu, "the neighbour slot is untouched");

    /* Low byte selects the slot: 0x105 aliases 5, 0x1ff the last. */
    indexed_flag_value_set_one(big, 0x105u);
    v = 0;
    __builtin_memcpy(&v, big + 0x40b4 + 5 * 4, 4);
    CHECK(v == 1u, "the masked index hits the same slot");
    indexed_flag_value_set_one(big, 0x1ffu);
    v = 0;
    __builtin_memcpy(&v, big + 0x40b4 + 255 * 4, 4);
    CHECK(v == 1u, "index 0x1ff hits the last slot");

    indexed_flag_value_set_one(NULL, 5u); /* NULL safe (deviation) */
    CHECK(1, "NULL record is safe");

    /* The resource-handle writer: +0x2c stores, neighbours hold. */
    uint8_t obj[0x30];
    memset(obj, 0x5A, sizeof(obj));
    scene_object_resource_handle_set(obj, 0xcafebabeu);
    v = 0;
    __builtin_memcpy(&v, obj + 0x2c, 4);
    CHECK(v == 0xcafebabeu, "the handle lands at +0x2c");
    CHECK(obj[0x28] == 0x5A && obj[0x2b] == 0x5A && 0x30 - 0x30 == 0,
          "the neighbours are untouched");
    scene_object_resource_handle_set(NULL, 1u);
    CHECK(1, "NULL object is safe");
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
    test_descriptor_coordinates();
    test_flag_and_handle_writers();
    test_straggler_reads();
    test_interpolated_z();
    /* The entity record-field leaves (2026-10-24): plain writes with the
     * corpus's offsets over a caller-owned record buffer. */
    {
        unsigned char buf[256];
        memset(buf, 0, sizeof(buf));
        record_fields_4_and_14_set(buf, 0x1111u, 0x2222u);
        uint32_t v = 0;
        __builtin_memcpy(&v, buf + 0x04, 4);
        CHECK(v == 0x1111u, "the +4 store lands");
        CHECK(record_field14_get(buf) == 0x2222u,
              "the +0x14 store reads back through the ported get leaf");
        v = 0;
        __builtin_memcpy(&v, buf + 0x08, 4);
        CHECK(v == 0u, "the neighbour dword stays zero");
        record_fields_4_and_14_set(NULL, 1, 2);
        CHECK(1, "NULL record skips");
    }

    {
        unsigned char buf[256];
        memset(buf, 0, sizeof(buf));
        memset(buf + 0xa0, 0x5a, 12);
        entity_fields_a0_a4_store_a8_clear(buf, 0x3333u, 0x4444u);
        uint32_t v = 0;
        __builtin_memcpy(&v, buf + 0xa0, 4);
        CHECK(v == 0x3333u, "the +0xa0 store lands");
        __builtin_memcpy(&v, buf + 0xa4, 4);
        CHECK(v == 0x4444u, "the +0xa4 store lands");
        __builtin_memcpy(&v, buf + 0xa8, 4);
        CHECK(v == 0u, "the +0xa8 word is cleared (faithful)");
        entity_fields_a0_a4_store_a8_clear(NULL, 1, 2);
        CHECK(1, "NULL record skips");
    }

    {
        unsigned char buf[256];
        memset(buf, 0, sizeof(buf));
        buf[0xb8] = 0x11;
        entity_b8_bit3_set_or_clear(buf, 1);
        CHECK(buf[0xb8] == 0x19u, "the raise sets bit 3");
        entity_b8_bit3_set_or_clear(buf, 1);
        CHECK(buf[0xb8] == 0x19u, "raising twice is idempotent");
        entity_b8_bit3_set_or_clear(buf, 0);
        CHECK(buf[0xb8] == 0x11u, "the clear drops bit 3, spares the rest");
        entity_b8_bit3_set_or_clear(NULL, 1);
        CHECK(1, "NULL record skips");
    }

    /* The record-field write band (2026-10-24). */
    {
        char rec[0x30];
        memset(rec, 0, sizeof(rec));
        record_field18_set(rec, 0x11223344u);
        uint32_t v18 = 0u;
        __builtin_memcpy(&v18, rec + 0x18, sizeof(v18));
        CHECK(v18 == 0x11223344u, "the +0x18 store lands");
        record_flags_24_or(rec, 2u);
        record_flags_24_or(rec, 8u);
        uint32_t f24 = 0u;
        __builtin_memcpy(&f24, rec + 0x24, sizeof(f24));
        CHECK(f24 == 0xAu, "the flag OR accumulates");
        object_field14_set(rec, 9u);
        uint32_t v14 = 0u;
        __builtin_memcpy(&v14, rec + 0x14, sizeof(v14));
        CHECK(v14 == 9u, "the +0x14 store lands");
        record_field18_set(NULL, 1u); /* guarded */
        object_field14_set(NULL, 1u); /* guarded */
        record_flags_24_or(NULL, 1u); /* guarded */

        /* The chain walk: three linked nodes all clear their +4. */
        char c[3][0x30];
        memset(c, 0, sizeof(c));
        unsigned long n1 = (unsigned long)(size_t)c[1];
        unsigned long n2 = (unsigned long)(size_t)c[2];
        __builtin_memcpy(c[0] + 0x14, &n1, sizeof(n1));
        __builtin_memcpy(c[1] + 0x14, &n2, sizeof(n2));
        __builtin_memcpy(c[0] + 4, "\x11\x11\x11\x11", 4);
        __builtin_memcpy(c[1] + 4, "\x22\x22\x22\x22", 4);
        __builtin_memcpy(c[2] + 4, "\x33\x33\x33\x33", 4);
        linked_record_field4_clear(c[0]);
        unsigned long p0 = 0, p1 = 0, p2 = 0;
        __builtin_memcpy(&p0, c[0] + 4, 4);
        __builtin_memcpy(&p1, c[1] + 4, 4);
        __builtin_memcpy(&p2, c[2] + 4, 4);
        CHECK(p0 == 0 && p1 == 0 && p2 == 0, "the chain clears every node");
        linked_record_field4_clear(NULL); /* guarded */
    }

    /* CopyTwoDwordPairToOutputs (2026-10-24). */
    {
        char rec[8];
        __builtin_memcpy(rec, "\x11\x11\x11\x11\x22\x22\x22\x22", 8);
        uint32_t a = 0, b = 0;
        copy_two_dword_pair_to_outputs(rec, &a, &b);
        CHECK(a == 0x11111111u && b == 0x22222222u, "the pair splits");
        copy_two_dword_pair_to_outputs(NULL, &a, &b);
        copy_two_dword_pair_to_outputs(rec, NULL, &b); /* guarded */
    }

    /* SceneObjectOwnerSet (2026-10-24). */
    {
        char so[0x28];
        memset(so, 0, sizeof(so));
        scene_object_owner_set(so, 0x99u);
        uint32_t owner = 0u;
        __builtin_memcpy(&owner, so + 0x20, 4);
        CHECK(owner == 0x99u, "the owner word lands at +0x20");
        scene_object_owner_set(NULL, 1u); /* guarded */
    }

    /* The record primitive band (2026-10-24). */
    {
        /* +0x64 store through the +8 base pointer, stride 0x70. */
        char base[0x128];
        memset(base, 0, sizeof(base));
        char rec[0x10];
        memset(rec, 0, sizeof(rec));
        unsigned long bp = (unsigned long)(size_t)base;
        __builtin_memcpy(rec + 8, &bp, sizeof(bp));
        indexed_record_field64_set(rec, 1, 0x22u);
        uint32_t v = 0u;
        __builtin_memcpy(&v, base + 100 + 0x70, 4);
        CHECK(v == 0x22u, "the stride-0x70 store lands");
        indexed_record_field64_set(NULL, 0, 1u); /* guarded */
        indexed_record_field64_set(rec, 0, 1u);  /* base stays: fine */

        /* ObjectVector3Set: slot 0 of the block is the base pointer. */
        char block[8];
        memset(block, 0, sizeof(block));
        __builtin_memcpy(block, &bp, sizeof(bp));
        object_vector3_set(block, 1.5f, 2.5f, 3.5f);
        float x = 0, y = 0, z = 0;
        __builtin_memcpy(&x, base + 0x3c, 4);
        __builtin_memcpy(&y, base + 0x40, 4);
        __builtin_memcpy(&z, base + 0x44, 4);
        CHECK(x == 1.5f && y == 2.5f && z == 3.5f, "the vector lands at +3c");
        object_vector3_set(NULL, 0.0f, 0.0f, 0.0f); /* guarded */

        char r2[0x20];
        memset(r2, 0, sizeof(r2));
        float_pair_at_offset4_set_from_ints(r2, -2, 7);
        float_pair_at_offsetc_set_from_uints(r2, 4u, 8u);
        CHECK(*(float*)(r2 + 0x04) == -2.0f && *(float*)(r2 + 0x08) == 7.0f
                  && *(float*)(r2 + 0x0c) == 4.0f
                  && *(float*)(r2 + 0x10) == 8.0f,
              "the int/uint-to-float pairs land with the casts");
        float_pair_at_offset4_set_from_ints(NULL, 1, 1); /* guarded */

        char big[0x828];
        memset(big, 0, sizeof(big));
        uint32_t p1 = 0x55u, p2 = 0x66u;
        __builtin_memcpy(big + 0x820, &p1, 4);
        __builtin_memcpy(big + 0x824, &p2, 4);
        uint32_t a = 0, b = 0;
        object_state_pair_get(big, &a, &b);
        CHECK(a == 0x55u && b == 0x66u, "the +0x820 pair reads");
        object_state_pair_get(NULL, &a, &b); /* guarded */
    }

    /* The scene-visibility wrapper band (2026-10-24). */
    {
        scene_visibility_hook_reset();

        /* SurfaceVisibilitySet: record +0 handle through the leaf. */
        char obj[0x20];
        memset(obj, 0, sizeof obj);
        char rec[0x10];
        memset(rec, 0, sizeof rec);
        void* p = obj;
        __builtin_memcpy(rec + 0x00, &p, sizeof p);
        surface_visibility_set(rec, 1u);
        void* last = NULL;
        int flag = -1;
        scene_visibility_last_call(&last, &flag);
        CHECK(scene_visibility_calls() == 1 && last == (void*)obj && flag == 1,
              "the surface wrapper lands on the record's object");

        /* AnimationObjectVisibilitySet: animation record +4. */
        char anim[0x20];
        memset(anim, 0, sizeof anim);
        __builtin_memcpy(anim + 0x04, &p, sizeof p);
        animation_object_visibility_set(anim, 0u);
        scene_visibility_last_call(&last, &flag);
        CHECK(scene_visibility_calls() == 2 && last == (void*)obj && flag == 0,
              "the animation wrapper reads its +4 handle");

        /* AnimationObjectVisibilityGet: +4 object's flag bit 3. */
        obj[0x14] = (char)0x10; /* bit 3 clear */
        CHECK(animation_object_visibility_get(anim) == 0u, "bit 3 reads clear");
        obj[0x14] = (char)0x08; /* bit 3 set */
        CHECK(animation_object_visibility_get(anim) == 1u, "bit 3 reads set");

        /* SceneAssignEntityOwner: owner store on the +4 object. */
        scene_assign_entity_owner(anim, 0x1234u);
        uint32_t owner = 0;
        __builtin_memcpy(&owner, obj + 0x20, sizeof owner);
        CHECK(owner == 0x1234u, "the owner lands through the ported setter");

        /* SceneObjectPairApplyMode: partner only when set. */
        scene_visibility_hook_reset();
        char pair[0x90];
        memset(pair, 0, sizeof pair);
        __builtin_memcpy(pair + 0x04, &p, sizeof p);
        scene_object_pair_apply_mode(pair, 1u);
        CHECK(scene_visibility_calls() == 1, "the partnerless pair calls once");
        char mate[0x10];
        memset(mate, 0, sizeof mate);
        void* mp = mate;
        __builtin_memcpy(pair + 0x7c, &mp, sizeof mp);
        scene_object_pair_apply_mode(pair, 1u);
        CHECK(scene_visibility_calls() == 3, "the second object rides the mode");
        scene_visibility_last_call(&last, &flag);
        CHECK(last == (void*)mate, "the partner call lands last");

        /* GraphicsOverlayVisibilitySet: cache gate, folded flag. */
        scene_visibility_hook_reset();
        graphics_overlay_word_set(7u);
        graphics_overlay_visibility_set(1u); /* cache moves 0 -> 1 */
        CHECK(scene_visibility_calls() == 1, "the first flag change calls");
        graphics_overlay_visibility_set(1u); /* equal cache: no call */
        CHECK(scene_visibility_calls() == 1, "the equal flag stays silent");
        graphics_overlay_visibility_set(0u);
        scene_visibility_last_call(&last, &flag);
        CHECK(scene_visibility_calls() == 2 && last == (void*)(uintptr_t)7u
                  && flag == 0,
              "the flag-off call folds zero and rides the handle word");
        CHECK(graphics_overlay_word_peek() == 7u, "the overlay word sticks");
    }

    /* SixResourceSlotsSetState (2026-10-24): six handle slots from +0x10. */
    {
        scene_visibility_hook_reset();
        char rec[0x60];
        memset(rec, 0, sizeof rec);
        char slots[6][0x10];
        for (int i = 0; i < 6; ++i) {
            memset(slots[i], 0, sizeof slots[i]);
            void* h = slots[i];
            __builtin_memcpy(rec + 0x10 + (size_t)i * sizeof(void*),
                             &h, sizeof h);
        }
        six_resource_slots_set_state(rec, 1u);
        int flag = -1;
        scene_visibility_last_call(NULL, &flag);
        CHECK(scene_visibility_calls() == 6 && flag == 1,
              "all six slots ride the leaf with the flag");
        void* saved3 = NULL;
        __builtin_memcpy(rec + 0x10 + 3 * sizeof(void*), &saved3,
                         sizeof saved3); /* slot 3 empty: skipped */
        scene_visibility_hook_reset();
        six_resource_slots_set_state(rec, 0u);
        CHECK(scene_visibility_calls() == 5, "the empty slot stays silent");
        scene_visibility_hook_reset();
        six_resource_slots_set_state(NULL, 1u); /* rec_ptr guards */
        CHECK(scene_visibility_calls() == 0, "the NULL record is guarded");
    }

    /* UIRecordCallbackForward (2026-10-24): handle deref to the pair. */
    {
        static char record[0x900];
        memset(record, 0, sizeof record);
        const uint32_t va = 0x21u, vb = 0x41u;
        __builtin_memcpy(record + 0x820, &va, sizeof va);
        __builtin_memcpy(record + 0x824, &vb, sizeof vb);
        void* slot = record;
        uint32_t out_a = 0, out_b = 0;
        ui_record_callback_forward((const void* const*)&slot, &out_a, &out_b);
        CHECK(out_a == 0x21u && out_b == 0x41u,
              "the forwarded read lands on the outputs");
        ui_record_callback_forward(NULL, &out_a, &out_b); /* guarded */
        const void* empty = NULL;
        ui_record_callback_forward(&empty, &out_a, &out_b); /* pair guards */
    }

    /* RecordFlagBitTest (2026-10-24): the +0x130 bit test + mask reject. */
    {
        static char rec[0x140];
        memset(rec, 0, sizeof rec);
        const uint32_t w = 0x100u; /* bit 8 */
        __builtin_memcpy(rec + 0x130, &w, sizeof w);
        CHECK(record_flag_bit_test(rec, 8u, 0) == 0x100u, "the set bit answers");
        CHECK(record_flag_bit_test(rec, 9u, 0) == 0u, "the clear bit answers 0");
        CHECK(record_flag_bit_test(rec, 40u, 0) == 0x100u,
              "the bit index folds through 0x1f");
        /* Masked reject: the bit also riding the type mask (+300) answers
         * 0 when rejecting. */
        const uint32_t mask = 0x111u; /* bit 8 raised in the mask */
        __builtin_memcpy(rec + 300, &mask, sizeof mask);
        CHECK(record_flag_bit_test(rec, 8u, 1) == 0u,
              "the masked bit is rejected when rejecting");
        CHECK(record_flag_bit_test(rec, 4u, 1) == 0u,
              "a mask hit answers clear without the reject bit");
        CHECK(record_flag_bit_test(rec, 8u, 0) == 0x100u,
              "the same bit rides the flag without rejecting");
        CHECK(record_flag_bit_test(NULL, 8u, 0) == 0u,
              "the NULL record guards");
    }

    /* ObjectPoolListsFreeAll (2026-10-24): the head sweep. */
    {
        static char pools[0x114 + 0x37 * sizeof(void*)];
        memset(pools, 0, sizeof pools);

        /* head 2 takes a two-node chain; head 54 takes a single node. */
        void* n0 = engine_heap_alloc(16);
        void* n1 = engine_heap_alloc(16);
        void* n2 = engine_heap_alloc(16);
        memset(n0, 0, 16);
        memset(n1, 0, 16);
        memset(n2, 0, 16);
        memcpy((char*)n0 + 8, &n1, sizeof n1);
        memcpy(pools + 0x114 + 2 * sizeof(void*), &n0, sizeof n0);
        memcpy(pools + 0x114 + 54 * sizeof(void*), &n2, sizeof n2);

        object_pool_lists_free_all(pools);
        void* head = (void*)0x1;
        memcpy(&head, pools + 0x114 + 2 * sizeof(void*), sizeof head);
        CHECK(head == NULL, "the freed chain clears its head");
        memcpy(&head, pools + 0x114 + 54 * sizeof(void*), sizeof head);
        CHECK(head == NULL, "the last head clears too");

        object_pool_lists_free_all(pools); /* empty-table walk */
        object_pool_lists_free_all(NULL);  /* guarded */
        CHECK(1, "the sweeps complete");
    }

    /* UI record factories (2026-10-24): windows + gates. */
    {
        void* rec = bounded_float_record_create(7, 1.0f, 3.0f,
                                                0xAAu, 0xBBu, 0xCCu, 0xDDu);
        CHECK(rec != NULL, "the bounded float record allocates");
        uint32_t kind = 0;
        float mn = 0.0f, mx = 0.0f, f3 = 0.0f;
        __builtin_memcpy(&kind, (char*)rec + 0 * sizeof(void*), sizeof kind);
        __builtin_memcpy(&mn, (char*)rec + 1 * sizeof(void*), sizeof mn);
        __builtin_memcpy(&mx, (char*)rec + 2 * sizeof(void*), sizeof mx);
        memcpy(&f3, (char*)rec + 3 * sizeof(void*), 4);
        CHECK(kind == 7u && mn == 1.0f && mx == 3.0f && f3 != 0.0f,
              "kind, bounds and payload land in the window");
        CHECK(bounded_float_record_create(1, 3.0f, 2.0f, 0u, 0u, 0u, 0u)
                  == NULL,
              "the closed interval is rejected");
        engine_heap_free(rec);

        void* wide = menu_item_numeric_range_record_allocate(
            3, 1, 10, 11u, 12u, 13u, 14u, 15u, 16u, 17u, 18u, 19u, 20u, 21u,
            22u);
        CHECK(wide != NULL, "the slider record allocates");
        uint32_t wkind = 0, lo = 0, hi = 0, f12 = 0, f13 = 0, f14 = 0;
        __builtin_memcpy(&wkind, wide + 0 * sizeof(void*), sizeof wkind);
        __builtin_memcpy(&lo, wide + 1 * sizeof(void*), sizeof lo);
        __builtin_memcpy(&hi, wide + 2 * sizeof(void*), sizeof hi);
        __builtin_memcpy(&f12, wide + 12 * sizeof(void*), sizeof f12);
        __builtin_memcpy(&f13, wide + 13 * sizeof(void*), sizeof f13);
        __builtin_memcpy(&f14, wide + 14 * sizeof(void*), sizeof f14);
        CHECK(wkind == 3u && lo == 1u && hi == 10u && f12 == 20u && f13 == 21u
                  && f14 == 22u,
              "the wide window lands in corpus field order");
        CHECK(menu_item_numeric_range_record_allocate(
                  1, 5, 5, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u)
                  == NULL,
              "equal bounds are rejected");
        engine_heap_free(wide);
    }

    /* NestedRecordListRemoveByKeyRecursive (2026-10-24): the deep sweep. */
    {
        void* nw = engine_heap_alloc(0x18);
        void* n1 = engine_heap_alloc(0x18);
        void* n2 = engine_heap_alloc(0x18);
        memset(nw, 0, 0x18);
        memset(n1, 0, 0x18);
        memset(n2, 0, 0x18);
        uint32_t k1 = 2u, k2 = 3u;
        memcpy((char*)n1 + 8, &k1, 4);
        memcpy((char*)n2 + 8, &k2, 4);
        memcpy((char*)nw + 0xc, &n1, sizeof n1);
        memcpy((char*)n1 + 0xc, &n2, sizeof n2);

        CHECK(nested_record_list_remove_by_key_recursive(2, nw) == 0
                  && nested_record_list_remove_by_key_recursive(5, nw) == 0,
              "the sweep answers through the head");
        /* The first call freed n1; n2 is still linked. The second call
         * finds key 2 absent (n1's successor is n2) and leaves the rest. */
        void* head = NULL;
        memcpy(&head, (char*)nw + 0xc, sizeof head);
        CHECK(head == n2, "the matched node promoted its successor");
        CHECK(nested_record_list_remove_by_key_recursive(3, nw) == 0,
              "the tail match answers through the head too");
        memcpy(&head, (char*)nw + 0xc, sizeof head);
        CHECK(head == NULL, "the freed tail node unlinks");
        CHECK(nested_record_list_remove_by_key_recursive(9, nw) == 0
                  && nested_record_list_remove_by_key_recursive(1, NULL) == 0,
              "the empty sweep and the null guard answer zero");
        engine_heap_free(nw);
    }

    if (s_failures != 0) {
        fprintf(stderr, "record_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("record_test: PASS\n");
    return 0;
}
