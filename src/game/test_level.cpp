// SPDX-License-Identifier: LGPL-3.0-or-later
// Level `.map` loading pipeline (finding 07) — synthetic grid fixture.
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "game/level.hpp"
#include "game/game.hpp"
#include "game/util.hpp"
#include "gods98/block.hpp"
#include "gods98/heap.hpp"
#include "gods98/message.hpp"
#include "gods98/resource.hpp"

static int s_failures = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) {                                                         \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg);      \
            ++s_failures;                                                      \
        }                                                                      \
    } while (0)

#define GRID 8

static const char* FIX = "level_fixture";

static void write_fixture(const char* name, const char* body)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", FIX, name);
    FILE* fp = fopen(path, "w");
    if (fp != NULL) {
        fputs(body, fp);
        fclose(fp);
    }
}

/* Write a `.map` grid: `MAP ` + u32 len + u32 w + u32 h + u16 cells. */
static void write_map(const char* name, int w, int h, int (*cell)(int x, int y))
{
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", FIX, name);
    FILE* fp = fopen(path, "wb");
    if (fp == NULL) {
        fprintf(stderr, "cannot write %s\n", path);
        return;
    }
    const char magic[4] = { 'M', 'A', 'P', ' ' };
    uint32_t len = (uint32_t)(w * h * 2);
    uint32_t ww = (uint32_t)w, hh = (uint32_t)h;
    fwrite(magic, 1, 4, fp);
    fwrite(&len, 1, 4, fp);
    fwrite(&ww, 1, 4, fp);
    fwrite(&hh, 1, 4, fp);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            uint16_t v = (uint16_t)cell(x, y);
            fwrite(&v, 1, 2, fp);
        }
    }
    fclose(fp);
}

static int const0_cell(int x, int y) { (void)x; (void)y; return 0; }
static int height_cell(int x, int y) { (void)x; (void)y; return 4; }
static int const7_cell(int x, int y) { (void)x; (void)y; return 7; }
static int predug_cell(int x, int y)
{
    if (x <= 0 || y <= 0 || x >= GRID - 1 || y >= GRID - 1) return 0;
    return (x + y) % 2 ? 1 : 2;
}
static int terrain_cell(int x, int y)
{
    if (x == 2 && y == 2) return 6;
    if (x == 5 && y == 5) return 11;
    return 1;
}
static int blockptrs_cell(int x, int y)
{
    if (x == 1 && y == 1) return 6;  /* >4: count reduced by 4 */
    return (x == 0 && y == 0) ? 3 : 0;
}
static int emerge_cell(int x, int y)
{
    return (x == 3 && y == 3) ? 7 : 0; /* look at odd-value scatter */
}

/* Optional real-data probe (ran when --real <wad-dir> is passed): mount the
 * shipped wads and count config groups that look like levels (have a
 * SurfaceMap). Best-effort — skipped (reported) when the wads are absent. */
static int real_probe(const char* wad_dir)
{
    int mounted = resource_mount(wad_dir, "LegoRR");
    BlockFile* cfg = mounted > 0 ? block_open("Lego.cfg") : NULL;
    if (cfg == NULL) {
        fprintf(stderr, "level_test: --real probe skipped (no Lego.cfg from %s)\n",
                wad_dir);
        return 0;
    }
    int levels = 0;
    for (size_t i = 0; i < block_record_count(cfg); ++i) {
        BlockRec* g = block_record_at(cfg, i);
        if (g == NULL) {
            continue;
        }
        char path[512];
        snprintf(path, sizeof(path), "%s Levels %s SurfaceMap",
                 util_root_name(cfg), block_name(g));
        int found = block_get_strptr(cfg, path) != NULL;
        if (!found) {
            snprintf(path, sizeof(path), "%s %s SurfaceMap",
                     util_root_name(cfg), block_name(g));
            found = block_get_strptr(cfg, path) != NULL;
        }
        if (found) {
            ++levels;
        }
    }
    fprintf(stderr, "level_test: --real probe: %d level groups in Lego.cfg\n",
            levels);
    block_close(cfg);
    return levels;
}

int main(int argc, char** argv)
{
    for (int a = 1; a < argc; ++a) {
        if (strcmp(argv[a], "--real") == 0 && a + 1 < argc) {
            real_probe(argv[a + 1]);
            return 0;
        }
    }
    char rmcmd[64]; snprintf(rmcmd, sizeof(rmcmd), "rm -rf %s", FIX); system(rmcmd);
    mkdir(FIX, 0755);

    write_fixture("Lego.cfg",
        "Lego {\n"
        "  Level01 {\n"
        "    blocksize 2\n"
        "    digdepth 2\n"
        "    useroof 1\n"
        "    roofheight 5\n"
        "    roughlevel 4\n"
        "    selboxheight 8\n"
        "    FallinMultiplier 2\n"
        "    SurfaceMap L01_surface.map\n"
        "    PredugMap L01_predug.map\n"
        "    TerrainMap L01_terrain.map\n"
        "    BlockPointersMap L01_blockpt.map\n"
        "    CryOreMap L01_cryore.map\n"
        "    ErodeMap L01_erode.map\n"
        "    PathMap L01_path.map\n"
        "    EmergeMap L01_emerge.map\n"
        "    AIMap L01_ai.map\n"
        "    FallinMap L01_fallin.map\n"
        "    StartFP 1\n"
        "    nextlevel Level02\n"
        "  }\n"
        "}\n");

    write_map("L01_surface.map",  GRID, GRID, height_cell);
    write_map("L01_predug.map",   GRID, GRID, predug_cell);
    write_map("L01_terrain.map",   GRID, GRID, terrain_cell);
    write_map("L01_blockpt.map",   GRID, GRID, const7_cell);
    write_map("L01_cryore.map",   GRID, GRID, const0_cell);
    write_map("L01_erode.map",    GRID, GRID, const0_cell);
    write_map("L01_path.map",     GRID, GRID, const0_cell);
    write_map("L01_emerge.map",   GRID, GRID, emerge_cell);
    write_map("L01_ai.map",       GRID, GRID, const0_cell);
    write_map("L01_fallin.map",   GRID, GRID, blockptrs_cell);

    if (chdir(FIX) != 0) {
        fprintf(stderr, "chdir %s failed\n", FIX);
        return 1;
    }

    BlockFile* cfg = block_open("Lego.cfg");
    CHECK(cfg != NULL, "open cfg");
    LevelState* s = level_load(cfg, "Level01");
    if (cfg == NULL || s == NULL) {
        fprintf(stderr, "level_load returned NULL\n");
        return 1;
    }

    CHECK(level_width(s) == GRID, "grid width");
    CHECK(level_height(s) == GRID, "grid height");

    /* Corpus scalar stores. */
    CHECK(level_float(s, LEVEL_OFF_BLOCKSIZE) == 2.0f, "blocksize stored");
    CHECK(level_int(s, LEVEL_OFF_USEROOF) == 1, "useroof stored");
    CHECK(level_int(s, LEVEL_OFF_STARTP) == 1, "StartFP stored");

    /* Voxel fields: surface byte (+4) comes from BlockPointersMap (all 7),
     * and the block-pointer count from the *FallinMap* (the code's mapping,
     * level.hpp header note). */
    {
        uint8_t* v00 = level_voxel(s, 0, 0);
        uint8_t* v11 = level_voxel(s, 1, 1);
        CHECK(v00 != NULL && v00[0x04] == 7, "voxel +4 surface at (0,0)");
        CHECK(v11 != NULL && v11[0x04] == 7, "voxel +4 surface at (1,1)");

        /* Erosion/cryo (+2) from TerrainMap, with the 6/11 special cases. */
        uint8_t* v22 = level_voxel(s, 2, 2);
        uint8_t* v55 = level_voxel(s, 5, 5);
        CHECK(v22 != NULL && v22[0x02] == 6, "voxel +2 erosion at (2,2)");
        CHECK(v22[0x07] == 4, "voxel +7 crystal seam marker at (2,2)");
        CHECK(v55 != NULL && v55[0x02] == 11, "voxel +2 erosion at (5,5)");

        /* Predug flags: interior odd cells get the 3x3 edge-wall bit. */
        uint32_t f8;
        __builtin_memcpy(&f8, level_voxel(s, 3, 3) + 0x08, 4);
        CHECK((f8 & 0x4000000u) != 0, "predug edge-wall bit at (3,3)");

        /* Block pointers: count >4 reduced by 4, flag set. */
        uint32_t* c38 = (uint32_t*)(level_voxel(s, 1, 1) + 0x38);
        CHECK(*c38 == 1, "block-pointer flag at (1,1)");
        uint32_t c3c;
        __builtin_memcpy(&c3c, level_voxel(s, 1, 1) + 0x3C, 4);
        CHECK(c3c == 2, "block-pointer count reduced 6->2 at (1,1)");
    }

    /* Terrain-cell flag primitives (port cluster 0x432bd0–0x433120): the
     * flag-word accessors over a hand-driven record. (3,4) keeps no
     * load-time state the later checks read, and its flag word is
     * restored afterwards. */
    {
        const int w = (int)level_width(s), h = (int)level_height(s);
        uint8_t* rec = level_voxel(s, 3, 4);
        uint32_t saved, f, zero = 0;
        __builtin_memcpy(&saved, rec + 0x08, 4);
        __builtin_memcpy(rec + 0x08, &zero, 4);

        CHECK(level_cell_speed_class(s, 3, 4) == 1, "zero flags -> class 1");
        CHECK(level_cell_flag_wave(s, 3, 4) == 0, "zero -> no wave flag");
        CHECK(level_cell_walkable_flag(s, 3, 4) == 0, "zero -> not traversable");
        CHECK(level_cell_mask18_clear(s, 3, 4) == 1, "zero 0x18 -> clear");
        CHECK(level_cell_occupied_at(s, 3, 4) == 0, "zero -> unoccupied");
        CHECK(level_cell_mask_c0(s, 3, 4) == 0, "zero low byte -> mask 0");
        CHECK(level_cell_flag_edge_wall(s, 3, 4) == 0, "zero -> no edge wall");
        CHECK(level_cell_mask_180000(s, 3, 4) == 0, "zero -> no 0x180000");

        f = 0x10u; __builtin_memcpy(rec + 0x08, &f, 4);
        CHECK(level_cell_walkable_flag(s, 3, 4) == 1, "traversable bit -> walkable");
        f = 0x18u; __builtin_memcpy(rec + 0x08, &f, 4);
        CHECK(level_cell_walkable_flag(s, 3, 4) == 0, "blocked gate clears walkable");
        CHECK(level_cell_mask18_clear(s, 3, 4) == 0, "set bits fail the clear test");

        f = 0x200000u; __builtin_memcpy(rec + 0x08, &f, 4);
        CHECK(level_cell_speed_class(s, 3, 4) == 0, "block bit -> class 0");
        f = 3u; __builtin_memcpy(rec + 0x08, &f, 4);
        CHECK(level_cell_speed_class(s, 3, 4) == 4, "type bits 3 -> class 4");

        f = 0x40000u; __builtin_memcpy(rec + 0x08, &f, 4);
        CHECK(level_cell_flag_wave(s, 3, 4) == 0x40000u, "wave flag reads masked");
        f = 0x4000000u; __builtin_memcpy(rec + 0x08, &f, 4);
        CHECK(level_cell_flag_edge_wall(s, 3, 4) == 0x4000000u, "edge wall reads masked");
        f = 0x180000u; __builtin_memcpy(rec + 0x08, &f, 4);
        CHECK(level_cell_mask_180000(s, 3, 4) == 1, "the 0x180000 pair reads in");
        f = 0x2000u; __builtin_memcpy(rec + 0x08, &f, 4);
        CHECK(level_cell_flag_2000(s, 3, 4) == 0x2000u, "bit 0x2000 reads");
        f = 0x80u; __builtin_memcpy(rec + 0x08, &f, 4);
        CHECK(level_cell_flag_80(s, 3, 4) == 0x80u, "bit 0x80 reads");
        f = 0x8000u; __builtin_memcpy(rec + 0x08, &f, 4);
        CHECK(level_cell_flag_8000(s, 3, 4) == 0x8000u, "bit 0x8000 reads");
        __builtin_memcpy(rec + 0x08, &zero, 4);
        rec[8] = 0xc0;
        CHECK(level_cell_mask_c0(s, 3, 4) == 0xc0u, "low-byte mask reads rec+8");
        f = 0x20000000u;
        {
            uint32_t low = rec[8]; rec[8] = 0; /* keep the byte write from echoing */
            __builtin_memcpy(rec + 0x08, &f, 4);
            rec[8] = (uint8_t)low;
        }
        CHECK(level_cell_occupied_at(s, 3, 4) == 1, "the occupied-class bit reads");

        /* Secondary word (record+12): bit 0 test mirrors the word. */
        uint32_t sec;
        __builtin_memcpy(&sec, rec + 0x0C, 4);
        CHECK(level_cell_secondary_bit0(s, 3, 4) == (sec & 1u),
              "secondary bit 0 mirrors the word");
        sec |= 1u;
        __builtin_memcpy(rec + 0x0C, &sec, 4);
        CHECK(level_cell_secondary_bit0(s, 3, 4) == 1u, "secondary bit 0 reads");
        sec &= ~1u;
        __builtin_memcpy(rec + 0x0C, &sec, 4);

        /* Flag raise/clear word pair (the corpus's write side). */
        __builtin_memcpy(rec + 0x08, &zero, 4);
        level_cell_flag_10000000_set(s, 3, 4, 1);
        f = 0;
        __builtin_memcpy(&f, rec + 0x08, 4);
        CHECK(f == 0x10000000u, "the flag word is raised");
        level_cell_flag_10000000_set(s, 3, 4, 0);
        f = 1;
        __builtin_memcpy(&f, rec + 0x08, 4);
        CHECK(f == 0u, "the flag word is cleared");
        level_cell_flag_10000000_set(s, -1, 2, 1); /* OOB write ignored */
        level_cell_flag_10000000_set(s, (int)w, 0, 1);

        /* Bounds deviations: the port reads 0 / class 1 / false off-map. */
        CHECK(level_cell_flag_wave(s, w, 0) == 0, "off-map flag word reads 0");
        CHECK(level_cell_speed_class(s, -1, -1) == 1, "off-map class falls back to 1");
        CHECK(level_cell_occupied_at(s, w, h) == 0, "off-map is unoccupied");

        /* The corpus's dim-1 gates, kept in-map so the gate itself decides:
         * the last column sits inside the voxels but outside dim-1. */
        f = 0x14u; /* walkable + bit 4 */
        __builtin_memcpy(rec + 0x08, &f, 4);
        CHECK(level_cell_walkable_flag(s, w - 1, 0) == 0,
              "the dim-1 gate rejects the last column");
        CHECK(level_cell_mask18_clear(s, w - 1, 0) == 1,
              "outside the gate counts as clear");
        CHECK(level_cell_flag_4(s, w - 1, 0) == 0u,
              "flag 4 reads 0 outside the gate");
        CHECK(level_cell_flag_4(s, 3, 4) == 4u, "flag 4 reads inside the gate");
        (void)h;

        /* State-byte equality: the TerrainMap fixture (6 at (2,2), 11 at
         * (5,5), plain 1 elsewhere). */
        CHECK(level_cell_state_is_6(s, 2, 2) == 1, "state-6 cell matches");
        CHECK(level_cell_state_is_1(s, 2, 2) == 0, "state 6 is not 1");
        CHECK(level_cell_state_is_1(s, 0, 3) == 1, "plain terrain is state 1");
        CHECK(level_cell_state_is_1(s, w, 0) == 0, "off-map equals nothing");

        __builtin_memcpy(rec + 0x08, &saved, 4); /* leave the map as loaded */
    }

    /* Second terrain leaf cluster: secondary-word bits, the OR-test,
     * gated bit reads, the state-set test, the reservation bit read,
     * the word-pair writes, and the +3 record byte. Same (3,4) record,
     * saved and restored around each field. */
    {
        const int w = (int)level_width(s), h = (int)level_height(s);
        uint8_t* rec = level_voxel(s, 3, 4);
        uint32_t saved, f, zero = 0;
        __builtin_memcpy(&saved, rec + 0x08, 4);
        uint32_t saved2;
        __builtin_memcpy(&saved2, rec + 0x0C, 4);
        const uint8_t saved_b2 = rec[2], saved_b3 = rec[3];
        (void)h;

        __builtin_memcpy(rec + 0x08, &zero, 4);
        __builtin_memcpy(rec + 0x0C, &zero, 4);

        /* Secondary word. */
        f = 0x100u;
        __builtin_memcpy(rec + 0x0C, &f, 4);
        CHECK(level_cell_secondary_flag_100(s, 3, 4) == 0x100u,
              "secondary 0x100 reads masked");
        f = 2u;
        __builtin_memcpy(rec + 0x0C, &f, 4);
        CHECK(level_cell_visited_flag(s, 3, 4) == 2u, "visited mark reads");
        __builtin_memcpy(rec + 0x0C, &zero, 4);
        CHECK(level_cell_visited_flag(s, 3, 4) == 0u, "cleared visited");
        level_cell_secondary_bit0_set(s, 3, 4);
        __builtin_memcpy(&f, rec + 0x0C, 4);
        CHECK(f == 1u, "secondary bit 0 raised");
        level_cell_secondary_bit0_clear(s, 3, 4);
        __builtin_memcpy(&f, rec + 0x0C, 4);
        CHECK(f == 0u, "secondary bit 0 cleared");
        level_cell_secondary_bit8_set(s, 3, 4, 1);
        CHECK(level_cell_secondary_flag_8(s, 3, 4) == 8u, "secondary bit 3 reads");
        level_cell_secondary_bit8_set(s, 3, 4, 0);
        CHECK(level_cell_secondary_flag_8(s, 3, 4) == 0u, "secondary bit 3 clears");

        /* The OR-test pair: mode 0 masks the flag word, mode 1 answers
         * whether either the flag 0x400 or the secondary byte's bit 4
         * marks the cell. */
        f = 0x400u;
        __builtin_memcpy(rec + 0x08, &f, 4);
        CHECK(level_cell_occupied_or_flag(s, 3, 4, 0) == 0x400u,
              "mode 0 masks the flag word");
        __builtin_memcpy(rec + 0x08, &zero, 4);
        CHECK(level_cell_occupied_or_flag(s, 3, 4, 1) == 0u,
              "neither marker -> 0");
        rec[12] = 4; /* secondary low byte, bit 4 */
        CHECK(level_cell_occupied_or_flag(s, 3, 4, 1) == 1u,
              "secondary byte bit 4 marks");
        f = 0x400u;
        __builtin_memcpy(rec + 0x0C, &zero, 4);
        __builtin_memcpy(rec + 0x08, &f, 4);
        CHECK(level_cell_occupied_or_flag(s, 3, 4, 1) == 1u,
              "flag 0x400 marks");

        /* Gated bit reads: inside, then the last column outside dim-1. */
        f = 0x800u | 0x20u;
        __builtin_memcpy(rec + 0x08, &f, 4);
        CHECK(level_cell_flag_800_bounded(s, 3, 4) == 0x800u, "0x800 reads inside");
        CHECK(level_cell_flag_20_bounded(s, 3, 4) == 0x20u, "0x20 reads inside");
        CHECK(level_cell_flag_8_bounded(s, 3, 4) == 0u, "bit 8 clear here");
        f = 8u;
        __builtin_memcpy(rec + 0x08, &f, 4);
        CHECK(level_cell_flag_8_bounded(s, 3, 4) == 8u, "bit 8 reads");
        CHECK(level_cell_flag_800_bounded(s, w - 1, 0) == 0u,
              "0x800 reads 0 outside the gate");
        CHECK(level_cell_flag_8_bounded(s, w - 1, 0) == 0u,
              "bit 8 reads 0 outside the gate");

        /* Reservation bit: the level_tile_mark write side, read back. */
        level_tile_mark(s, 3, 4, 1);
        CHECK(level_cell_flag_reserved(s, 3, 4) == 0x8000000u,
              "the reservation bit reads masked");
        level_tile_mark(s, 3, 4, 0);
        __builtin_memcpy(rec + 0x08, &zero, 4);

        /* Word-pair write: raise/clear 0x80000. */
        level_cell_flag_80000_set(s, 3, 4, 1);
        __builtin_memcpy(&f, rec + 0x08, 4);
        CHECK(f == 0x80000u, "flag 0x80000 raised");
        level_cell_flag_80000_set(s, 3, 4, 0);
        __builtin_memcpy(&f, rec + 0x08, 4);
        CHECK(f == 0u, "flag 0x80000 cleared");
        level_cell_flag_80000_set(s, w, 0, 1); /* OOB no-op */

        /* Work bit: gated raise, OOB and outside-gate no-ops. */
        level_cell_work_flag_set(s, 3, 4);
        __builtin_memcpy(&f, rec + 0x08, 4);
        CHECK(f == 4u, "the work bit is raised inside the gate");
        level_cell_work_flag_set(s, w - 1, 0);
        __builtin_memcpy(&f, rec + 0x08, 4);
        CHECK(f == 4u, "the outside-gate call left the record alone");
        CHECK(level_cell_flag_4(s, 3, 4) == 4u, "the work bit is the 0x4 test");
        __builtin_memcpy(rec + 0x08, &zero, 4);

        /* Record byte +3 and the state-set test. */
        rec[3] = 0x5a;
        CHECK(level_cell_record_byte3(s, 3, 4) == 0x5a, "record byte +3 reads");
        CHECK(level_cell_record_byte3(s, w, 0) == 0, "off-map byte +3 reads 0");
        rec[2] = 10;
        CHECK(level_cell_type_is_8_or_10_or_11(s, 3, 4) == 1, "state 10 is blocky");
        rec[2] = 11;
        CHECK(level_cell_type_is_8_or_10_or_11(s, 3, 4) == 1, "state 11 is blocky");
        rec[2] = 8;
        CHECK(level_cell_type_is_8_or_10_or_11(s, 3, 4) == 1, "state 8 is blocky");
        rec[2] = 6;
        CHECK(level_cell_type_is_8_or_10_or_11(s, 3, 4) == 0, "state 6 is not");
        CHECK(level_cell_type_is_8_or_10_or_11(s, w, 0) == 0, "off-map is not");

        /* Restore every field the block touched. */
        __builtin_memcpy(rec + 0x08, &saved, 4);
        __builtin_memcpy(rec + 0x0C, &saved2, 4);
        rec[2] = saved_b2;
        rec[3] = saved_b3;
    }

    /* Coordinate-pair membership + the flag-8 check it gates. */
    {
        const int w = (int)level_width(s), h = (int)level_height(s);
        CHECK(level_coord_pair_within_bounds(s, 3, 4) == 1, "inside pair bounds");
        CHECK(level_coord_pair_within_bounds(s, w, 0) == 0, "x at w is outside");
        CHECK(level_coord_pair_within_bounds(s, 0, h) == 0, "y at h is outside");
        CHECK(level_coord_pair_within_bounds(s, -1, 0) == 0,
              "negative wraps outside as unsigned");
        CHECK(level_coord_pair_within_bounds(s, 0, h - 1) == 1, "last row is inside");

        uint8_t* rec = level_voxel(s, 3, 4);
        uint32_t saved, f;
        __builtin_memcpy(&saved, rec + 0x08, 4);
        f = 8u;
        __builtin_memcpy(rec + 0x08, &f, 4);
        CHECK(level_cell_flag8_check(s, 3, 4) == 1, "blocked bit checks in");
        {
            uint8_t* r70 = level_voxel(s, w - 1, 0);
            uint32_t s70;
            __builtin_memcpy(&s70, r70 + 0x08, 4);
            f = 8u;
            __builtin_memcpy(r70 + 0x08, &f, 4);
            CHECK(level_cell_flag8_check(s, w - 1, 0) == 1,
                  "plain bounds include the last column (not dim-1)");
            __builtin_memcpy(r70 + 0x08, &s70, 4);
        }
        __builtin_memcpy(rec + 0x08, &saved, 4);
        CHECK(level_cell_flag8_check(s, -1, -1) == 0, "off-grid never blocked");
        CHECK(level_cell_flag8_check(s, w, 5) == 0, "past the grid never blocked");
    }

    /* Destination/placement predicates (FUN_00432950 / FUN_00432a20). */
    {
        const int w = (int)level_width(s), h = (int)level_height(s);
        uint8_t* rec = level_voxel(s, 3, 4);
        uint32_t saved, f;
        const uint8_t saved_b2 = rec[2];
        __builtin_memcpy(&saved, rec + 0x08, 4);

        EntitySlot mover;
        memset(&mover, 0, sizeof(mover));

        /* Usable side: mover gate open, cell walkable-marked. */
        f = 0x10u;
        __builtin_memcpy(rec + 0x08, &f, 4);
        rec[2] = 6;
        mover.flags_3e8 = 0x8000u;
        CHECK(entity_move_destination_cell_usable(&mover, s, 3, 4) == 1,
              "the gated mover takes the walkable cell");
        mover.flags_3e8 = 0x8004u; /* mover bit + the placement byte bit */
        CHECK(terrain_cell_allows_placement(&mover, s, 3, 4) == 1,
              "the same cell passes placement (byte gate open)");
        mover.flags_3e8 = 0x8000u;

        /* Entity gates: bit 0x8000 for the mover, byte bit 0x4 for placement. */
        mover.flags_3e8 = 0u;
        CHECK(entity_move_destination_cell_usable(&mover, s, 3, 4) == 0,
              "the destination gate needs flag bit 0x8000");
        CHECK(terrain_cell_allows_placement(&mover, s, 3, 4) == 0,
              "placement needs the +1000 byte bit 0x4");
        mover.flags_3e8 = 0x100u; /* byte 0x01 — bit 0x4 clear */
        CHECK(terrain_cell_allows_placement(&mover, s, 3, 4) == 0,
              "the placement gate reads the low byte");
        mover.flags_3e8 = 0x104u;
        CHECK(terrain_cell_allows_placement(&mover, s, 3, 4) == 1,
              "the low byte's 0x4 opens the placement gate");
        CHECK(terrain_cell_allows_placement(NULL, s, 3, 4) == 1,
              "the NULL record (config call) passes the gate");

        /* Coordinate gates: zero and the 2-cell margin. */
        mover.flags_3e8 = 0x8000u;
        CHECK(entity_move_destination_cell_usable(&mover, s, 0, 4) == 0,
              "zero x is not a destination");
        CHECK(entity_move_destination_cell_usable(&mover, s, 3, 0) == 0,
              "zero y is not a destination");
        CHECK(entity_move_destination_cell_usable(&mover, s, w - 1, 1) == 0,
              "the margin gate rejects the last column");
        CHECK(terrain_cell_allows_placement(NULL, s, w - 1, 1) == 0,
              "placement shares the margin gate");
        CHECK(entity_move_destination_cell_usable(&mover, s, -1, 1) == 0,
              "negative wraps outside as unsigned");

        /* Cell conditions: the 0x40 walk block, the 0x20 placement block,
         * and the excluded type sets. */
        f = 0x10u | 0x40u;
        __builtin_memcpy(rec + 0x08, &f, 4);
        CHECK(entity_move_destination_cell_usable(&mover, s, 3, 4) == 0,
              "flag 0x40 blocks the destination");
        CHECK(terrain_cell_allows_placement(NULL, s, 3, 4) == 1,
              "flag 0x40 is not a placement blocker");
        f = 0x10u | 0x20u;
        __builtin_memcpy(rec + 0x08, &f, 4);
        CHECK(entity_move_destination_cell_usable(&mover, s, 3, 4) == 1,
              "flag 0x20 is not a destination blocker");
        CHECK(terrain_cell_allows_placement(NULL, s, 3, 4) == 0,
              "flag 0x20 blocks placement");
        f = 0u;
        __builtin_memcpy(rec + 0x08, &f, 4);
        CHECK(entity_move_destination_cell_usable(&mover, s, 3, 4) == 0,
              "no walkable flag: destination unusable");
        CHECK(terrain_cell_allows_placement(NULL, s, 3, 4) == 0,
              "no walkable flag: placement refused");

        f = 0x10u;
        __builtin_memcpy(rec + 0x08, &f, 4);
        rec[2] = 8;
        CHECK(entity_move_destination_cell_usable(&mover, s, 3, 4) == 1,
              "type 8 does not block the destination");
        rec[2] = 11;
        CHECK(entity_move_destination_cell_usable(&mover, s, 3, 4) == 0,
              "type 11 blocks the destination");
        rec[2] = 1;
        CHECK(entity_move_destination_cell_usable(&mover, s, 3, 4) == 0,
              "type 1 blocks the destination");
        rec[2] = 7;
        CHECK(entity_move_destination_cell_usable(&mover, s, 3, 4) == 0,
              "type 7 blocks the destination");
        rec[2] = 1;
        CHECK(terrain_cell_allows_placement(NULL, s, 3, 4) == 1,
              "type 1 places");
        rec[2] = 7;
        CHECK(terrain_cell_allows_placement(NULL, s, 3, 4) == 1,
              "type 7 places");
        rec[2] = 11;
        CHECK(terrain_cell_allows_placement(NULL, s, 3, 4) == 0,
              "type 11 blocks placement");
        rec[2] = 8;
        CHECK(terrain_cell_allows_placement(NULL, s, 3, 4) == 0,
              "type 8 blocks placement");
        rec[2] = 10;
        CHECK(terrain_cell_allows_placement(NULL, s, 3, 4) == 0,
              "type 10 blocks placement");
        rec[2] = 5;
        CHECK(terrain_cell_allows_placement(NULL, s, 3, 4) == 1,
              "a plain type places");

        __builtin_memcpy(rec + 0x08, &saved, 4);
        rec[2] = saved_b2;
    }

    /* Terrain leaves: gated type read + the +5 byte classifier. */
    {
        const int w = (int)level_width(s);
        uint8_t* rec = level_voxel(s, 3, 4);
        const uint8_t s2 = rec[2], s5 = rec[5];
        uint32_t out = 0;

        rec[2] = 6;
        CHECK(level_cell_type_get_at(s, 3, 4, &out) == 1 && out == 6,
              "the gated type read writes the byte");
        out = 0xdeadu;
        CHECK(level_cell_type_get_at(s, (unsigned)w - 1, 0, &out) == 0 &&
                  out == 0xdeadu,
              "outside the gate leaves the out param alone");

        uint32_t a = 9, b = 9, c = 9, d = 9;
        rec[5] = 0;
        CHECK(level_cell_byte_state_classify(s, 3, 4, &a, &b, &c, &d) == 0 &&
                  (a | b | c | d) == 0,
              "the zero byte clears the outs and reports 0");
        rec[5] = 1; /* idx 0: table[0] -> out_a, others stay zeroed */
        a = b = c = d = 9;
        CHECK(level_cell_byte_state_classify(s, 3, 4, &a, &b, &c, &d) == 1 &&
                  a == 1 && (b | c | d) == 0,
              "byte 1 routes table[0] to out_a");
        rec[5] = 3; /* idx 2: table[0] -> out_b (routed before out_c) */
        a = b = c = d = 9;
        CHECK(level_cell_byte_state_classify(s, 3, 4, &a, &b, &c, &d) == 1 &&
                  b == 1 && (a | c | d) == 0,
              "the b/c swap: idx 2 writes out_b, not out_c");
        rec[5] = 2; /* idx 1: table[0] -> out_c */
        a = b = c = d = 9;
        CHECK(level_cell_byte_state_classify(s, 3, 4, &a, &b, &c, &d) == 1 &&
                  c == 1 && (a | b | d) == 0, "idx 1 routes to out_c");
        rec[5] = 5; /* idx 4: table[1] = 3, low bits 0 -> out_a */
        a = b = c = d = 9;
        CHECK(level_cell_byte_state_classify(s, 3, 4, &a, &b, &c, &d) == 1 &&
                  a == 3 && (b | c | d) == 0, "the (v-1)/4 table index wins");
        rec[5] = 22; /* idx 21: past the table — reported 0 as documented */
        a = b = c = d = 9;
        CHECK(level_cell_byte_state_classify(s, 3, 4, &a, &b, &c, &d) == 1 &&
                  (a | b | c | d) == 0, "bytes past the table report 0");
        CHECK(level_cell_byte_state_classify(s, -1, 2, &a, &b, &c, &d) == 0 &&
                  (a | b | c | d) == 0, "off-map classifies as 0");

        rec[2] = s2;
        rec[5] = s5;
    }

    /* Map-file resource slots (the 0x489xxx registry + accessors). */
    {
        level_map_slots_reset();
        CHECK(level_map_slot_get(0) == NULL, "reset empties the table");
        CHECK(level_map_slot_get(2001) == NULL, "past the table resolves NULL");

        const size_t sz = 16 + 2 * 2 * 2;
        int s0 = level_map_slot_allocate(sz);
        CHECK(s0 == 0, "first claim takes slot 0");
        int s1 = level_map_slot_allocate(sz);
        CHECK(s1 == 1, "the next claim takes slot 1");

        uint8_t* img = level_map_slot_get((uint32_t)s1);
        CHECK(img != NULL, "a claimed slot resolves");
        if (img != NULL) {
            const uint32_t w2 = 2, h2 = 2;
            __builtin_memcpy(img + 0x08, &w2, 4);
            __builtin_memcpy(img + 0x0C, &h2, 4);
            const uint16_t cells[4] = { 11, 12, 13, 14 };
            for (int i = 0; i < 4; ++i) {
                __builtin_memcpy(img + 0x10 + i * 2, &cells[i], 2);
            }
            uint32_t dw = 0, dh = 0;
            level_map_dimensions_get((uint32_t)s1, &dw, &dh);
            CHECK(dw == 2 && dh == 2, "the slot header reads dims");
            CHECK(level_map_cell_get((uint32_t)s1, 1, 0) == 12,
                  "the slot cell read takes x fastest");
            CHECK(level_map_cell_get((uint32_t)s1, 0, 1) == 13,
                  "the slot cell read steps rows by width");
        }

        level_map_slot_release((uint32_t)s1);
        CHECK(level_map_slot_get((uint32_t)s1) == NULL, "release frees the id");
        level_map_slot_release((uint32_t)s1); /* free id: no-op */
        level_map_slot_release(50000u);       /* past the table: no-op */
        CHECK(level_map_slot_allocate(sz) == 1, "the released id is re-claimed");

        /* Raw header/cell twins, including the NULL guards. */
        uint8_t buf[16 + 4];
        memset(buf, 0, sizeof buf);
        const uint32_t w1 = 1;
        __builtin_memcpy(buf + 0x08, &w1, 4);
        const uint16_t cell = 777;
        __builtin_memcpy(buf + 0x12, &cell, 2); /* +16 + (1*2) */
        CHECK(level_map_image_cell(buf, 1, 1, 0) == 777, "the raw cell reads");
        uint32_t dw = 1, dh = 1;
        level_map_image_header(NULL, &dw, &dh);
        CHECK(dw == 0 && dh == 0, "the NULL image header reads 0");
        CHECK(level_map_image_cell(NULL, 0, 1, 0) == 0, "the NULL image cell reads 0");

        /* Unresolved slot: the corpus reads address 8/12 here; the port
         * reports 0 (deviation) and the cell read lands on zero dims. */
        level_map_dimensions_get(1234, &dw, &dh);
        CHECK(dw == 0 && dh == 0 && level_map_cell_get(1234, 0, 0) == 0,
              "the unresolved slot reads zero");

        level_map_slots_reset(); /* leak-clears the table as the corpus does */
        (void)s0;
    }

    /* Terrain handle + block table. */
    void* terr = level_load_ptr(s, LEVEL_OFF_TERRAIN);
    CHECK(terr != NULL, "terrain created");

    /* Objective word defaults to advisor-suppressed (0x10|0x20|0x80) with
     * no goals armed for a level with no objective keys. */
    CHECK((s->objective & 0x10u) != 0, "objective word defaults advisor-suppressed");
    CHECK((s->objective & (0x100u | 0x200u | 0x400u | 0x800u | 0x1000u)) == 0,
          "no goals armed");

    const char* next = (const char*)level_load_ptr(s, LEVEL_OFF_NEXTLEVEL);
    CHECK(next != NULL && strcmp(next, "Level02") == 0, "nextlevel stored");

    /* Active-engine state accessors (findings 295/324/392/475) drive the
     * same block words; fields saved and restored. */
    {
        uint32_t saved_b0 = 0, saved_84 = 0, saved_a0 = 0, saved_ac = 0;
        __builtin_memcpy(&saved_b0, s->word + 0xb0 / 4, 4);
        __builtin_memcpy(&saved_84, s->word + 0x84 / 4, 4);
        __builtin_memcpy(&saved_a0, s->word + 0xa0 / 4, 4);
        __builtin_memcpy(&saved_ac, s->word + 0xac / 4, 4);

        level_set_int(s, LEVEL_OFF_ENGINE_B0, 0x1234);
        CHECK(active_engine_field_b0_get(s) == 0x1234u, "b0 dword read");
        level_set_int(s, LEVEL_OFF_STATE84, 0x55aa55aau);
        CHECK(global_state_field84_get(s) == 0x55aa55aau,
              "state-84 dword read");
        level_set_float(s, LEVEL_OFF_ENGINE_F260, 1.75f);
        CHECK(engine_float_field260_get(s) == 1.75f, "260 float read");

        /* Indexed float array: write slot 1 and read it back. */
        {
            float saved_211 = level_float(s, LEVEL_OFF_ENGINE_FLOATS + 4);
            level_set_float(s, LEVEL_OFF_ENGINE_FLOATS + 4, 2.5f);
            CHECK(engine_float_field210_get_by_index(s, 1) == 2.5f,
                  "210-indexed float (index 1)");
            level_set_float(s, LEVEL_OFF_ENGINE_FLOATS + 4, saved_211);
        }

        /* Upgrade resource check: the selector picks the counter. */
        level_set_int(s, LEVEL_OFF_RES_A, 40);
        level_set_int(s, LEVEL_OFF_RES_B, 70);
        CHECK(upgrade_resource_check(s, 0) == 40, "selector 0 -> +0xa0");
        CHECK(upgrade_resource_check(s, 1) == 70, "selector 1 -> +0xac");
        CHECK(upgrade_resource_check(s, -3) == 70,
              "any nonzero selector -> +0xac");

        /* NULL level answers 0 across the family (documented deviation). */
        CHECK(active_engine_field_b0_get(NULL) == 0, "NULL b0 read");
        CHECK(engine_float_field210_get_by_index(NULL, 0) == 0.0f,
              "NULL indexed float");
        CHECK(engine_float_field260_get(NULL) == 0.0f, "NULL 260 float");
        CHECK(global_state_field84_get(NULL) == 0, "NULL state-84 read");
        CHECK(upgrade_resource_check(NULL, 0) == 0, "NULL resource check");

        __builtin_memcpy(s->word + 0xb0 / 4, &saved_b0, 4);
        __builtin_memcpy(s->word + 0x84 / 4, &saved_84, 4);
        __builtin_memcpy(s->word + 0xa0 / 4, &saved_a0, 4);
        __builtin_memcpy(s->word + 0xac / 4, &saved_ac, 4);
    }

    level_destroy(s);
    block_close(cfg);

    /* AlwaysAcceptPlaceCommandCell (FUN_00432550, finding 455). */
    CHECK(always_accept_place_command_cell() == 1,
          "the kind-0x16 callback accepts unconditionally");

    /* The cell timestamp dedup pair (2026-10-24). */
    {
        int k;
        CHECK(cell_coordinate_timestamp_exists(1234, 5678) == 0u,
              "the table starts empty");
        cell_coordinate_timestamp_remember(1234, 5678);
        CHECK(cell_coordinate_timestamp_exists(1234, 5678) == 1u,
              "remember registers the pair");
        CHECK(cell_coordinate_timestamp_exists(1234, 5679) == 0u,
              "the near miss answers no");
        for (k = 0; k < 999; ++k) {
            cell_coordinate_timestamp_remember(k * 7 + 1, k * 11 + 1);
        }
        cell_coordinate_timestamp_remember(987654, 321);
        CHECK(cell_coordinate_timestamp_exists(987654, 321) == 0u,
              "the full table stores nothing (faithful)");
        CHECK(cell_coordinate_timestamp_exists(998 * 7 + 1, 998 * 11 + 1)
                  == 1u,
              "the filled table answers for its entries");
    }

    /* The surface-type config leaves (2026-10-24): the config loader is
     * unported, so every name falls back and every value reads zero. */
    {
        const char* n = terrain_surface_type_name_get(0);
        CHECK(strcmp(n, "Need config for surface type") == 0,
              "the missing-config fallback answers");
        CHECK(strcmp(terrain_surface_type_name_get(3), n) == 0,
              "the typed indexes all fall back");
        CHECK(strcmp(terrain_surface_type_name_get(-1), n) == 0
                  && strcmp(terrain_surface_type_name_get(1 << 20), n) == 0,
              "the guarded indexes fall back too");
        CHECK(surface_type_configured_value_at_index(0) == 0u
                  && surface_type_configured_value_at_index(5) == 0u,
              "the configured values read zero");
        CHECK(surface_type_configured_value_at_index(-1) == 0u
                  && surface_type_configured_value_at_index(1 << 20) == 0u,
              "the guarded indexes read zero");
    }

    /* The grid-record count tables (2026-10-24): both statics start and
     * stay zero — the corpus's entity-side writers do not feed them. */
    {
        CHECK(grid_record_count_get(3, 7, 2) == 0u, "the count reads zero");
        CHECK(grid_record_count_difference_get(3, 7, 2, 1) == 0,
              "the difference reads zero with the subtraction");
        CHECK(grid_record_count_difference_get(3, 7, 2, 0) == 0,
              "the second table reads zero too");

        /* The slot band: every slot of a cell answers zero; the tables'
         * edges and the blind-index band past them stay guarded. */
        CHECK(grid_count_table_slot_peek(19, 14, 15, 0) == 0,
              "the last cell's last slot is in range");
        CHECK(grid_count_table_slot_peek(20, 0, 0, 1) == -1,
              "the row past the table reports out of range");
        CHECK(grid_count_table_slot_peek(-1, 0, 0, 0) == -1,
              "the negative row reports out of range");
        CHECK(grid_record_count_get(1 << 20, 0, 0) == 0u
                  && grid_record_count_difference_get(0, 0, -1, 1) == 0,
              "the guarded indices answer zero");

        /* The clear leaf sweeps both tables (all zero already). */
        grid_record_count_tables_clear();
        CHECK(grid_count_table_slot_peek(0, 0, 0, 0) == 0
                  && grid_count_table_slot_peek(10, 5, 9, 1) == 0,
              "the clear leaves the tables zero");
    }

    /* The tile-variant grid-record table (2026-10-24). */
    {
        uint32_t a = 0, b = 0;

        /* Fresh region: every reader answers zero. */
        CHECK(cell_flag100_is_set(2, 1, 3) == 0, "the table starts empty");
        CHECK(cell_flag4_is_set(2, 1, 3) == 0, "flag 4 answers zero");
        terrain_cell_record_get(3, 2, 1, &a, &b);
        CHECK(a == 0u && b == 0u, "the record fields start zero");

        /* The 0x100 setter reads per (cell, field), never neighbours. */
        cell_flag100_set(2, 1, 3);
        CHECK(cell_flag100_is_set(2, 1, 3) != 0, "the flag raises");
        CHECK(cell_flag100_is_set(2, 1, 4) == 0, "the field neighbour holds");
        CHECK(cell_flag100_is_set(1, 2, 3) == 0, "the cell neighbour holds");

        /* Flag 4 tracks the field-0 word while no variant record owns
         * the block (the port case): the field argument is ignored. */
        cell_flag100_set(3, 3, 0);
        CHECK(cell_flag4_is_set(3, 3, 5) == 0,
              "the field word answers but not bit 0x4");

        /* Out-of-region coordinates skip (bounds deviation). */
        cell_flag100_set(999999, 0, 0);
        CHECK(cell_flag100_is_set(999999, 0, 0) == 0,
              "the far cell stays unaddressable");

        /* Records resolve to the zero defaults; NULL outputs are safe. */
        terrain_cell_record_get(3, 0, 0, &a, &b);
        a = 0xdeadbeefu;
        b = 0xdeadbeefu;
        terrain_cell_record_get(999999, 0, 0, &a, &b);
        CHECK(a == 0xdeadbeefu && b == 0xdeadbeefu,
              "the far cell leaves the outputs alone");
        terrain_cell_record_get(1, 1, 0, NULL, NULL); /* NULL-safe */
        CHECK(1, "NULL record outputs are safe");

        /* The mask sweep: fields 0..15 drop, field 16 keeps. */
        cell_flag100_set(5, 7, 16);
        cell_flag100_set(14, 19, 7);
        grid_record_flag_mask_clear();
        CHECK(cell_flag100_is_set(5, 7, 3) == 0 &&
                  cell_flag100_is_set(2, 1, 3) == 0 &&
                  cell_flag100_is_set(14, 19, 7) == 0,
              "the sweep clears every in-band flag");
        CHECK(cell_flag100_is_set(5, 7, 16) != 0,
              "the sweep leaves field 16 alone (faithful)");

        /* The present table: NULL without the unported grid init. */
        CHECK(grid_cell_record_present_check(3, 4) == 0,
              "no cell records before the grid initialize leaf");
        CHECK(grid_cell_record_present_check(0, 0) == 0,
              "the origin answers zero too");
    }

    /* The global-object field band (2026-10-24). */
    {
        LevelState g;
        memset(&g, 0, sizeof(g));
        global_object_field8c_increment(&g);
        global_object_field8c_increment(&g);
        global_object_field8c_subtract(&g, 3);
        CHECK((int)level_field8c_get(&g) == -1, "the 8c band walks");

        global_object_field98_add(&g, 7);
        global_object_field98_add(&g, -2);
        CHECK((int)level_field98_get(&g) == 5, "the 98 add accumulates");
        global_object_field98_clear(&g);
        CHECK(level_field98_get(&g) == 0u, "the 98 clear zeroes");

        level_state_field9c_add(&g, 11);
        CHECK((int)level_field9c_get(&g) == 11, "the 9c add rides");

        global_object_field94_increment(&g);
        global_object_field94_increment(&g);
        CHECK(level_field94_get(&g) == 2u, "the 94 increments");

        global_object_fielda8_increment(&g);
        CHECK(level_fielda8_get(&g) == 1u, "the a8 increments");

        /* The NULL-level guards answer nothing, as the corpus's null
         * global does. */
        global_object_field8c_increment(NULL);
        global_object_field8c_subtract(NULL, 1);
        global_object_field98_add(NULL, 1);
        global_object_field98_clear(NULL);
        level_state_field9c_add(NULL, 1);
        global_object_field94_increment(NULL);
        global_object_fielda8_increment(NULL);
        CHECK(level_field8c_get(NULL) == 0u && level_field98_get(NULL) == 0u
                  && level_field9c_get(NULL) == 0u
                  && level_field94_get(NULL) == 0u
                  && level_fielda8_get(NULL) == 0u,
              "the guarded null level answers zero everywhere");
    }

    /* GlobalOptionLowBitsEnabled (2026-10-24). */
    {
        LevelState g;
        memset(&g, 0, sizeof(g));
        CHECK(global_option_low_bits_enabled(&g) == 0
                  && global_option_low_bits_enabled(NULL) == 0,
              "the option gate answers for a clear word and the null level");
        g.objective = 8; /* low bits stay clear */
        CHECK(global_option_low_bits_enabled(&g) == 0,
              "the gate ignores the high word");
        g.objective = 0x11;
        CHECK(global_option_low_bits_enabled(&g) != 0,
              "any low bit opens the gate");
    }

    /* The objective-result read (2026-10-24). */
    {
        LevelState g;
        memset(&g, 0, sizeof(g));
        CHECK(level_objective_result_get(&g) == 0
                  && level_objective_result_get(NULL) == 0,
              "the result word starts clear");
        g.objective_result = 1;
        CHECK(level_objective_result_get(&g) == 1, "the achieved word reads");
    }

    /* RandomSeedSet (2026-10-24): the draw follows from the seed exactly as
     * the corpus's LCG does. */
    {
        level_random_seed_set(1u);
        const int first = level_random_draw15();
        level_random_seed_set(1u);
        CHECK(level_random_draw15() == first, "the same seed draws the same");
        level_random_seed_set(0x1234u);
        const int seeded = level_random_draw15();
        const int expect = (int)(((uint32_t)0x1234u * 214013u + 2531011u)
                                 >> 16) & 0x7fff;
        CHECK(seeded == expect, "the seed lands into the LCG state");
        CHECK((expect & ~0x7fff) == 0, "the draw stays 15-bit");
    }

    /* The terrain-object SFX grid getter (2026-10-24): pair indexing. */
    {
        uint32_t pos[2] = { 1u, 2u };
        terrain_sfx_grid_cell_set(17, 555);
        CHECK(terrain_object_type_sfx_at_cell_get(pos) == 555,
              "the cell pair indexes the sfx grid");
        const uint32_t big[2] = { 0xffffu, 0u };
        CHECK(terrain_object_type_sfx_at_cell_get(big) == 0,
              "the out-of-range cell answers clear");
        CHECK(terrain_object_type_sfx_at_cell_get(NULL) == 0,
              "a null record answers clear");
    }

    /* TerrainObjectTypeSfxGridConfigLoad (2026-10-25): config chain walk,
     * alias stores, and the cross-entry coordinate carry. */
    {
        static char text[] =
            "Lego* {\n"
            "  S_ObjTtSFXs { placeholder }\n"
            "  smallDigger SFX_Drill\n"
            "  ore SFX_RockBreak\n"
            "  powerCrystal SFX_Ambient\n"
            "  bogus SFX_Drip\n"
            "  slug SFX_NOTHING\n"
            "  tvcamera SFX_Drill\n"
            "}\n";
        BlockFile* bf = block_parse(text, strlen(text));
        CHECK(bf != NULL, "the sfx config parses");
        terrain_sfx_grid_cell_set(17, 555);
        terrain_object_sfx_grid_config_load(bf, "Lego*");
        CHECK(terrain_sfx_grid_cell_peek(16) == 2,
              "the vehicle-row pair lands the drill alias");
        CHECK(terrain_sfx_grid_cell_peek(105) == 4,
              "the ore dedicated row lands the break alias");
        CHECK(terrain_sfx_grid_cell_peek(90) == 6,
              "the crystal inherits the ore row across entries (carry)");
        CHECK(terrain_sfx_grid_cell_peek(17) == 0,
              "the load clears the window first");
        CHECK(terrain_sfx_grid_cell_peek(15) == 0
                  && terrain_sfx_grid_cell_peek(106) == 0,
              "unresolved names and failed aliases store nothing");
        block_close(bf);
    }

    /* TerrainChoiceResourceSelect (2026-10-25): window/scalar selects,
     * the default clear, the guard, and the record-pointer slot. */
    {
        CHECK(terrain_choice_active_peek() == 0u
                  && terrain_choice_flags_peek() == 0u
                  && terrain_choice_record_peek() == 0u,
              "the selection window starts clear");

        terrain_choice_window_install(0, 11u);
        terrain_choice_window_install(1, 22u);
        terrain_choice_window_install(2, 33u);
        terrain_choice_window_install(3, 44u);
        terrain_choice_scalar_install(0, 77u);
        terrain_choice_scalar_install(1, 88u);
        CHECK(terrain_choice_window_peek(4) == 0u, "the window guard answers zero");

        /* Kind 1 / index 0 reads the first window word and raises both bits. */
        uint32_t sel[2] = { 1u, 0u };
        terrain_choice_resource_select(sel);
        CHECK(terrain_choice_active_peek() == 11u && terrain_choice_flags_peek() == 3u
                  && terrain_choice_record_peek() == (uintptr_t)sel,
              "the kind 1 select stores the window word and the record");

        /* Kind 2 / index 1 folds to window word 2. */
        sel[0] = 2u; sel[1] = 1u;
        terrain_choice_resource_select(sel);
        CHECK(terrain_choice_active_peek() == 33u, "the folded window word reads");

        /* Kind 3 / index 3 strides past the guarded window: the answer
         * clears (deviation) while the success path still rides. */
        sel[0] = 3u; sel[1] = 3u;
        terrain_choice_resource_select(sel);
        CHECK(terrain_choice_active_peek() == 0u && terrain_choice_flags_peek() == 3u,
              "the guarded word answers clear on the success path (dev)");

        /* Kind 4 / index 0 reaches word 3. */
        sel[0] = 4u; sel[1] = 0u;
        terrain_choice_resource_select(sel);
        CHECK(terrain_choice_active_peek() == 44u, "the kind 4 select reads word 3");

        /* The scalars: kind 6 / kind 7. */
        sel[0] = 6u;
        terrain_choice_resource_select(sel);
        CHECK(terrain_choice_active_peek() == 77u, "the kind 6 select reads scalar 0");
        sel[0] = 7u;
        terrain_choice_resource_select(sel);
        CHECK(terrain_choice_active_peek() == 88u, "the kind 7 select reads scalar 1");

        /* Default kind: clears active and the flag bit 0, bit 1 survives,
         * the record slot stays out. */
        sel[0] = 9u;
        terrain_choice_resource_select(sel);
        CHECK(terrain_choice_active_peek() == 0u && terrain_choice_flags_peek() == 2u,
              "the default kind clears bit 0 only");

        /* A null record folds into the default case through the guarded
         * pair primitive. */
        terrain_choice_resource_select(NULL);
        CHECK(terrain_choice_active_peek() == 0u && terrain_choice_flags_peek() == 2u,
              "the null record stays inert on the default path");

        for (int w = 0; w < 4; ++w) {
            terrain_choice_window_install(w, 0u);
        }
        terrain_choice_scalar_install(0, 0u);
        terrain_choice_scalar_install(1, 0u);
    }

    /* EntityAvailableResourceSpend (2026-10-24): gate + branches. */
    {
        LevelState g;
        memset(&g, 0, sizeof g);
        int hint = 0;
        CHECK(entity_available_resource_spend(&g, 5, &hint) == 0,
              "the positive spend takes the dispatch branch");
        CHECK(level_field98_get(&g) == 0u,
              "the dispatch branch accumulates nothing");
        CHECK(entity_available_resource_spend(&g, -3, NULL) == 1,
              "the clear amount takes the accumulate branch");
        CHECK((int)level_field98_get(&g) == -3,
              "the negative amount lands in +0x98");
        const uint32_t one = 1u;
        __builtin_memcpy((char*)g.word + 0x1f8, &one, sizeof one);
        CHECK(entity_available_resource_spend(&g, 9, NULL) == 1,
              "the raised gate zeroes the amount");
        CHECK((int)level_field98_get(&g) == -3,
              "the gated spend adds zero");
        const uint32_t zero = 0u;
        __builtin_memcpy((char*)g.word + 0x1f8, &zero, sizeof zero);
        entity_available_resource_spend(NULL, 4, NULL); /* guarded */
    }

    /* CellCardinalNeighborApply (2026-10-24): the four-call sweep. */
    {
        void* terrain = terrain_create("L01_surface.map", 1.0f);
        CHECK(terrain != NULL, "the sweep terrain loads");
        if (terrain != NULL) {
            GameContext nctx;
            game_context_init(&nctx);
            static LevelState nlvl;
            memset(&nlvl, 0, sizeof(nlvl));
            nctx.level = &nlvl;
            nlvl.ptr_terrain = terrain;
            terrain_cell_surface_type_set(terrain, 2, 5, 4);
            terrain_cell_surface_type_set(terrain, 2, 4, 4);
            terrain_cell_surface_type_set(terrain, 3, 5, 4);
            terrain_cell_surface_type_set(terrain, 2, 6, 4);
            terrain_cell_surface_type_set(terrain, 1, 5, 4);
            cell_cardinal_neighbor_apply(&nctx, 2, 5);
            CHECK(terrain_cell_surface_type_get(terrain, 2, 5) == 4,
                  "the anchor cell is not swept");
            CHECK(terrain_cell_surface_type_get(terrain, 2, 4) == 0
                      && terrain_cell_surface_type_get(terrain, 3, 5) == 0
                      && terrain_cell_surface_type_get(terrain, 2, 6) == 0
                      && terrain_cell_surface_type_get(terrain, 1, 5) == 0,
                  "the four cardinal neighbors clear");
            terrain_cell_surface_type_set(terrain, 1, 2, 6);
            cell_cardinal_neighbor_apply(&nctx, 1, 3);
            CHECK(terrain_cell_surface_type_get(terrain, 1, 2) == 0,
                  "the sweep repeats over the live table");
            nctx.level = NULL;
            cell_cardinal_neighbor_apply(&nctx, 0, 0); /* skipped (deviation) */
            cell_cardinal_neighbor_apply(NULL, 0, 0);
            CHECK(1, "the missing-level and null guards stay inert");
        }
    }

    /* TerrainCellSpecialStatePredicate (2026-10-24): the gate pair. */
    {
        static uint8_t spgrid[GRID * GRID * 0x48];
        static LevelState sps;
        memset(spgrid, 0, sizeof spgrid);
        memset(&sps, 0, sizeof sps);
        sps.word[0x14 / 4] = GRID; /* width */
        sps.word[0x18 / 4] = GRID; /* height */
        sps.ptr_voxels = spgrid;
#define SPCELL(x, y) (spgrid + ((size_t)(y) * GRID + (size_t)(x)) * 0x48)
#define SPPUT(x, y, off, v)                                                    \
    do {                                                                       \
        const uint32_t v_ = (uint32_t)(v);                                     \
        memcpy(SPCELL(x, y) + (off), &v_, 4);                                  \
    } while (0)
        CHECK(terrain_cell_special_state_predicate(&sps, 3, 4, 0) == 0,
              "the empty cell is not special");
        SPPUT(3, 4, 0x08, 0x400u); /* the occupied flag */
        CHECK(terrain_cell_special_state_predicate(&sps, 3, 4, 0) == 1,
              "the flagged cell passes without the gate");
        CHECK(terrain_cell_special_state_predicate(&sps, 3, 4, 1) == 0,
              "the gated check needs the secondary 0x100");
        SPPUT(3, 4, 0x0C, 0x100u);
        CHECK(terrain_cell_special_state_predicate(&sps, 3, 4, 1) == 1,
              "the secondary 0x100 lands through the gate");
        SPPUT(3, 4, 0x08, 0u);
        SPPUT(3, 4, 0x0C, 0u);
        CHECK(terrain_cell_special_state_predicate(NULL, 3, 4, 0) == 0
                  && terrain_cell_special_state_predicate(&sps, 0, 0, 1) == 0,
              "the null state and the edge cell stay quiet (deviation)");
#undef SPCELL
#undef SPPUT
    }

    /* TerrainNeighborWorkFlagScan (2026-10-24): the work-flag sweep. */
    {
        static uint8_t wfgrid[GRID * GRID * 0x48];
        static LevelState wfs;
        memset(wfgrid, 0, sizeof wfgrid);
        memset(&wfs, 0, sizeof wfs);
        wfs.word[0x14 / 4] = GRID;
        wfs.word[0x18 / 4] = GRID;
        wfs.ptr_voxels = wfgrid;
#define WFW(x, y) (wfgrid + ((size_t)(y) * GRID + (size_t)(x)) * 0x48 + 0x08)
#define WFPUT(x, y, v)                                                         \
    do {                                                                       \
        const uint32_t v_ = (uint32_t)(v);                                     \
        memcpy(WFW(x, y), &v_, 4);                                             \
    } while (0)
        /* A single flagged cell: the sweep finds it around the seed,
         * clears it, queues the drifted quirk copy, and stops. */
        WFPUT(2, 2, 0x20000u);
        terrain_neighbor_work_flag_scan(&wfs, 2, 2);
        uint32_t got = 0;
        memcpy(&got, WFW(2, 2), 4);
        CHECK(got == 0u, "the seed cell loses its work flag");
        terrain_neighbor_work_flag_scan(&wfs, 2, 2);
        memcpy(&got, WFW(2, 2), 4);
        CHECK(got == 0u, "the swept map stays clear when rerun");

        /* Two flagged cells inside the seed's neighbourhood: both clear;
         * the far one only by riding the queue (the col-bound drift
         * keeps it reachable — here it also sits in the 3×3). */
        memset(wfgrid, 0, sizeof wfgrid);
        WFPUT(1, 1, 0x20000u);
        WFPUT(2, 3, 0x20000u);
        terrain_neighbor_work_flag_scan(&wfs, 2, 2);
        memcpy(&got, WFW(1, 1), 4);
        uint32_t got2 = 0;
        memcpy(&got2, WFW(2, 3), 4);
        CHECK((got & 0x20000u) == 0u && (got2 & 0x20000u) == 0u,
              "both flagged neighbours are processed and cleared");

        /* A cell only reachable through the queue (two columns away from
         * the seed's col-bound): the sweep hops once and gets it. */
        memset(wfgrid, 0, sizeof wfgrid);
        WFPUT(3, 2, 0x20000u);
        terrain_neighbor_work_flag_scan(&wfs, 2, 2);
        memcpy(&got, WFW(3, 2), 4);
        CHECK(got == 0u, "the queue hop clears the next-column cell");

        /* Out-of-map seeds and NULL state guard: nothing writes, nothing
         * crashes (the corpus strides both blindly). */
        terrain_neighbor_work_flag_scan(NULL, 1, 1);
        memset(wfgrid, 0, sizeof wfgrid);
        terrain_neighbor_work_flag_scan(&wfs, 9999u, 1u);
        memcpy(&got, WFW(0, 1), 4);
        CHECK(got == 0u, "an off-map seed sweeps the far column in-bounds");
#undef WFW
#undef WFPUT
    }

    /* AirSupplyAmountUpdate (FUN_00424560, 2026-10-24). */
    {
        static LevelState s;
        memset(&s, 0, sizeof s);
        static uint8_t stats[0x140];
        memset(stats, 0, sizeof stats);
        EntitySlot e;
        memset(&e, 0, sizeof e);
        e.stats_record = stats;
        const float rate = 2.0f;
        level_set_float(&s, LEVEL_OFF_OXYGEN_RATE, rate);
        const uint32_t zero_word = 0u;
        system_state_flags_set(&zero_word);

        /* Stats float +0xDC = 1.0, acc 10.0: 5 * 0.04 * 2 * 1 + 10 = 10.4. */
        const float one = 1.0f;
        memcpy(stats + 0xdc, &one, sizeof one);
        level_set_float(&s, LEVEL_OFF_AIR_SUPPLY, 10.0f);
        air_supply_amount_update(&s, &e, 5.0f);
        CHECK(level_float(&s, LEVEL_OFF_AIR_SUPPLY) == 10.4f,
              "the delta folds through rate and stats onto the accumulator");

        /* High clamp at exactly 100.0. */
        air_supply_amount_update(&s, &e, 1e5f);
        CHECK(level_float(&s, LEVEL_OFF_AIR_SUPPLY) == 100.0f,
              "the accumulator clamps high");

        /* Kind-4 gate: type 4 without the +0x3e8 bit keeps the acc. */
        float before = level_float(&s, LEVEL_OFF_AIR_SUPPLY);
        e.type = 4;
        air_supply_amount_update(&s, &e, -1e6f);
        CHECK(level_float(&s, LEVEL_OFF_AIR_SUPPLY) == before,
              "the kind-4 gate skips the update");
        e.flags_3e8 |= 0x20000000u;
        air_supply_amount_update(&s, &e, 0.0f);
        CHECK(level_float(&s, LEVEL_OFF_AIR_SUPPLY) == before,
              "the gated kind-4 folds zero (0 + acc)");
        e.type = 0;
        e.flags_3e8 = 0u;

        /* Depletion: negative lands as +0.0 and announces failure (who 2)
         * unless the system-state bit 0x08000000 suppresses it. */
        const uint32_t suppress = 0x08000000u;
        s.objective = 0u;
        level_set_float(&s, LEVEL_OFF_AIR_SUPPLY, 1.0f);
        system_state_flags_set(&suppress);
        air_supply_amount_update(&s, &e, -1e3f);
        CHECK(level_float(&s, LEVEL_OFF_AIR_SUPPLY) == 0.0f &&
                  s.objective == 0u,
              "suppressed depletion stores zero, no announce");
        system_state_flags_set(&zero_word);
        air_supply_amount_update(&s, &e, 0.0f); /* acc is 0: <= 0 branch */
        CHECK(level_float(&s, LEVEL_OFF_AIR_SUPPLY) == 0.0f &&
                  (s.objective & 4u) != 0u,
              "the unsuppressed zero announces the failed objective");

        /* Guard: NULL entity / NULL block change nothing. */
        air_supply_amount_update(&s, NULL, 1.0f);
        air_supply_amount_update(NULL, &e, 1.0f);
        CHECK(level_float(&s, LEVEL_OFF_AIR_SUPPLY) == 0.0f,
              "the guards keep the block untouched");
    }

    /* CellVariantEntryMatchCheck (2026-10-25): the window probe. */
    {
        static char win[CELL_VARIANT_WINDOW_SIZE];
        static unsigned char blocks[0x18 + 4 * 0x28];
        void* blocks_ptr = blocks;
        uint32_t u32;
        memset(win, 0, sizeof win);
        memset(blocks, 0xff, sizeof blocks);
        u32 = 2; /* width */
        memcpy(win, &u32, sizeof u32);
        u32 = 2; /* height */
        memcpy(win + 4, &u32, sizeof u32);
        u32 = 1; /* stride */
        memcpy(win + 8, &u32, sizeof u32);
        memcpy(win + 0x28, &blocks_ptr, sizeof blocks_ptr);
        /* cell (1,1) -> index 2; cell (0,1) -> index 1. */
        blocks[0x18 + 2 * 0x28] = 3;
        blocks[0x18 + 1 * 0x28] = 7;
        CHECK(cell_variant_entry_match_check(win, 1, 1, 1, 1) == 0,
              "the same cell answers 0");
        CHECK(cell_variant_entry_match_check(win, 2, 1, 0, 1) == 0
                  && cell_variant_entry_match_check(win, 1, 1, 0, 2) == 0,
              "the bounds test answers 0");
        CHECK(cell_variant_entry_match_check(win, 1, 1, 0, 1) == 0,
              "the empty variant list answers 0");
        win[0x4034u + 3u] = 1;
        win[0x34u + 3u * 0x80u] = 7;
        CHECK(cell_variant_entry_match_check(win, 1, 1, 0, 1) == 1,
              "the variant list lands the match");
        win[0x34u + 3u * 0x80u] = 8;
        CHECK(cell_variant_entry_match_check(win, 1, 1, 0, 1) == 0,
              "the list miss answers 0");
        blocks[0x18 + 2 * 0x28] = 0xff;
        CHECK(cell_variant_entry_match_check(win, 1, 1, 0, 1) == 0,
              "the missing first variant answers 0");
        blocks[0x18 + 2 * 0x28] = 3;
        blocks[0x18 + 1 * 0x28] = 0xff;
        CHECK(cell_variant_entry_match_check(win, 1, 1, 0, 1) == 0
                  && cell_variant_entry_match_check(NULL, 1, 1, 0, 1) == 0,
              "the missing second variant and window guards hold");
    }

    /* Path record window band (2026-10-25): serial, pair, and prev link. */
    {
        const uint32_t before = path_record_count_peek();
        void* first = path_record_allocate_and_link(NULL);
        CHECK(first != NULL && path_record_count_peek() == before + 1u,
              "the NULL pair serializes and links (guarded)");
        CHECK(path_record_last_peek() == first, "the last slot rides the record");
        const uint32_t pair[2] = { 0x1111u, 0x2222u };
        void* second = path_record_allocate_and_link(pair);
        uint32_t serial = 0u, wa = 0u, wb = 0u;
        memcpy(&serial, (char*)second + 0x08, 4);
        memcpy(&wa, (char*)second + 0x0c, 4);
        memcpy(&wb, (char*)second + 0x10, 4);
        CHECK(path_record_count_peek() == before + 2u && serial == before + 1u,
              "the serial store rides the counter");
        CHECK(wa == 0x1111u && wb == 0x2222u, "the coordinate pair lands");
        void* prev = NULL;
        memcpy(&prev, (char*)second + 0x24, sizeof prev);
        CHECK(prev == first, "the prev link rides the previous record");
        uint32_t word = 0xdeadbeefu;
        memcpy(&word, (char*)first + 0x1c, 4);
        CHECK(word == 0u, "the window arrives zeroed (corpus belt stores)");
        CHECK(path_record_last_peek() == second, "the last slot ends on second");
    }

    /* TerrainCellQueuedWorkTransition (2026-10-25): the flag gate and the
     * queued dispatch chain, on a hand-built grid (the loaded level closes
     * before the tail of this suite). */
    {
        static LevelState qg;
        static uint8_t qvox[8 * 8 * 0x48];
        memset(&qg, 0, sizeof qg);
        qg.ptr_voxels = qvox;
        level_set_int(&qg, LEVEL_OFF_WIDTH, 8);
        level_set_int(&qg, LEVEL_OFF_HEIGHT, 8);
        GameContext qctx;
        game_context_init(&qctx);
        qctx.level = &qg;
        uint8_t* rec = qvox + ((size_t)4 * 8 + 3) * 0x48; /* cell (3,4) */
        uint32_t f = 0u, now = 0u;
        f = 0x11u;
        __builtin_memcpy(rec + 0x08, &f, 4);
        terrain_cell_queued_work_transition(&qctx, 3, 4);
        __builtin_memcpy(&now, rec + 0x08, 4);
        CHECK(now == 0x31u, "the gate raises the work bit, rest preserved");
        f = 0x50u;
        __builtin_memcpy(rec + 0x08, &f, 4);
        terrain_cell_queued_work_transition(&qctx, 3, 4);
        __builtin_memcpy(&now, rec + 0x08, 4);
        CHECK(now == 0x50u, "the busy bits keep the cell out of the queue");
        f = 0x10u;
        __builtin_memcpy(rec + 0x08, &f, 4);
        qctx.level = NULL;
        terrain_cell_queued_work_transition(&qctx, 3, 4);
        qctx.level = &qg;
        terrain_cell_queued_work_transition(&qctx, 1 << 20, 1 << 20);
        __builtin_memcpy(&now, rec + 0x08, 4);
        CHECK(now == 0x10u, "the missing-level and OOB guards stay inert");
    }

    /* RandomVariantCellScatter (2026-10-25): modes, key bounds, radius
     * guard. The quote-only spawn route keeps every state change invisible
     * (deviation), so the band runs as a guarded smoke. */
    {
        level_random_seed_set(7u);
        random_variant_cell_scatter(4, 4, 2, 0); /* box 4, quartered  */
        random_variant_cell_scatter(4, 4, 2, 1); /* box 4, full limit */
        random_variant_cell_scatter(1, 1, 5, 1); /* box 10: keys in range */
        random_variant_cell_scatter(1, 1, 6, 0); /* box 12: guarded keys */
        random_variant_cell_scatter(1, 1, 0, 0); /* zero radius (deviation) */
        CHECK(1, "the scatter band runs every mode guarded");
    }

    /* RngThunkDraw15 (2026-10-25): the corpus thunk copy forwards the same
     * seeded 15-bit draw as the module model. */
    {
        level_random_seed_set(9u);
        const uint32_t thunk = rng_thunk_draw15();
        level_random_seed_set(9u);
        const int model = level_random_draw15();
        CHECK(thunk == (uint32_t)model && model >= 0 && model <= 0x7fff,
              "the thunk forwards the module draw");
    }

    /* Erosion timing band (2026-10-25): the scaled seeds, the cell delay,
     * and the neighbour sweep on a hand-built grid. */
    {
        erosion_timing_state_init(1.0f, 1.0f, 2.0f);
        int want_tick = 0;
        const float tick50 = 2.0f * 25.0f;
        memcpy(&want_tick, &tick50, sizeof want_tick);
        CHECK(erosion_decay_scale_peek() == 25.0f
                  && erosion_delay_scale_peek() == 1.0f * 25.0f * 0.2f
                  && erosion_timestamp_tick_peek() == want_tick,
              "the init seeds the scales and the timestamp tick (float bits)");
        erosion_timing_state_init(0.0f, 0.0f, 0.0f);
        CHECK(erosion_scratch_peek(0, 1999) == 0
                  && erosion_scratch_peek(1, 999) == 0
                  && erosion_scratch_peek(0, -1) == -1
                  && erosion_scratch_peek(1, 1000) == -1,
              "the scratch arrives cleared and bounded on peek");

        static LevelState ev;
        static uint8_t evox[4 * 4 * 0x48];
        memset(evox, 0, sizeof evox);
        memset(&ev, 0, sizeof ev);
        ev.ptr_voxels = evox;
        level_set_int(&ev, LEVEL_OFF_WIDTH, 4);
        level_set_int(&ev, LEVEL_OFF_HEIGHT, 4);
        uint8_t* seed = evox + ((size_t)2 * 4 + 2) * 0x48; /* cell (2,2) */
        seed[2] = 6; /* lava type */
        erosion_timing_state_init(1.0f, 1.0f, 0.0f);
        seed[6] = 3;
        CHECK(lava_erosion_delay_from_cell(&ev, 2, 2)
                  == (float)(6 - 3) * (1.0f * 25.0f * 0.2f) * 25.0f,
              "the delay counts the erosion byte down from six");
        seed[6] = 9;
        CHECK(lava_erosion_delay_from_cell(&ev, 2, 2)
                  == (float)(6 - 9) * (1.0f * 25.0f * 0.2f) * 25.0f,
              "the count goes negative past six (faithful)");
        CHECK(lava_erosion_delay_from_cell(NULL, 0, 0) == 0.0f
                  && lava_erosion_delay_from_cell(&ev, 9, 9) == 0.0f,
              "the guarded grid answers zero (deviation)");
        seed[6] = 0;

        int out[2] = { -5, -5 };
        erosion_timing_state_init(0.0f, 0.0f, 0.0f);

        /* Cell (1,2) seeds the sweep. Its north neighbour (1,1) carries
         * a coordinate timestamp from the earlier dedup fill — the table
         * is full, so a fresh remember here would store nothing. */
        uint8_t* seed2 = evox + ((size_t)2 * 4 + 1) * 0x48; /* (1,2) */
        uint8_t* north = evox + ((size_t)1 * 4 + 1) * 0x48; /* (1,1) */
        uint8_t* east = evox + ((size_t)2 * 4 + 2) * 0x48;  /* (2,2) */
        seed2[2] = 6;
        const uint32_t busyfree = 8u; /* bit 3, sign clear */
        memcpy(north + 8, &busyfree, 4);
        north[6] = 1;
        CHECK(lava_erosion_neighbor_check(&ev, 1, 2, out) == 0,
              "the timestamped north neighbour rides past the answer");
        seed2[2] = 5;
        CHECK(lava_erosion_neighbor_check(&ev, 1, 2, out) == 0,
              "the type gate skips the whole sweep");
        seed2[2] = 6;
        memcpy(east + 8, &busyfree, 4);
        east[6] = 1;
        CHECK(lava_erosion_neighbor_check(&ev, 1, 2, out) == 1
                  && out[0] == 2 && out[1] == 2,
              "the busy-free untimestamped east neighbour answers");
        const uint32_t signedflag = 0x80000008u;
        memcpy(east + 8, &signedflag, 4);
        CHECK(lava_erosion_neighbor_check(&ev, 1, 2, out) == 0,
              "the sign-bit neighbour stays out");
        /* No-grid guard + no-pair out slot still walk the neighbours. */
        CHECK(lava_erosion_neighbor_check(NULL, 1, 1, out) == 0
                  && lava_erosion_neighbor_check(&ev, 0, 0, NULL) == 0,
              "the guarded seed answers zero");
    }

    /* Cell route + neighbour pick (FUN_00431da0 / FUN_00431c70; 2026-10-25).
     * Synthetic 8x8 grid: state byte +0x00, type byte +0x02, flag word
     * +0x08, gate byte +0x0c — the corpus cell-record shape. */
    {
        static LevelState rs;
        static uint8_t voxels[8 * 8 * LEVEL_VOXEL_STRIDE];
        static uint8_t stats[0x150];
        memset(&rs, 0, sizeof rs);
        memset(voxels, 0, sizeof voxels);
        memset(stats, 0, sizeof stats);
        rs.ptr_voxels = voxels;
        level_set_int(&rs, LEVEL_OFF_WIDTH, 8);
        level_set_int(&rs, LEVEL_OFF_HEIGHT, 8);
        auto cellput = [](LevelState* g, int x, int y, uint32_t flags,
                          int type, int state, int gate) {
            uint8_t* rec = level_voxel(g, x, y);
            memcpy(rec + 0x08, &flags, 4);
            rec[0x00] = (uint8_t)state;
            rec[0x02] = (uint8_t)type;
            rec[0x0c] = (uint8_t)gate;
        };
        auto wordput = [](uint8_t* st, int off, uint32_t v) {
            memcpy(st + off, &v, 4);
        };
        /* The routine's plain-entity stats: special bit + walk relief. */
        wordput(stats, 0x144, 1u | 0x2000000u);
        EntitySlot mover;
        memset(&mover, 0, sizeof mover);
        mover.stats_record = stats;

        /* The entity-less lane: only blocked (flag 8) cells answer. */
        cellput(&rs, 3, 3, 0x8u, 1, 1, 0);
        CHECK(terrain_cell_route_and_occupancy_check(NULL, &rs, 3, 3, 3, 3,
                                                     0) == 1,
              "the entity-less gate takes the blocked cell");
        cellput(&rs, 3, 3, 0x8u | 0x400u, 1, 1, 0);
        CHECK(terrain_cell_route_and_occupancy_check(NULL, &rs, 3, 3, 3, 3,
                                                     0) == 0,
              "the +0x400 flag closes the cell");
        cellput(&rs, 3, 3, 0x8u | 0x40u, 1, 1, 0);
        CHECK(terrain_cell_route_and_occupancy_check(NULL, &rs, 3, 3, 3, 3,
                                                     1) == 0
                  && terrain_cell_route_and_occupancy_check(NULL, &rs, 3, 3,
                                                            3, 3, 0) == 1,
              "strict mode rejects the +0x40 flag");
        cellput(&rs, 3, 3, 0x8u, 1, 1, 4);
        CHECK(terrain_cell_route_and_occupancy_check(NULL, &rs, 3, 3, 3, 3,
                                                     0) == 0,
              "the +0x0c gate byte closes the cell (no relief flag)");
        cellput(&rs, 3, 3, 0u, 1, 1, 0);

        /* The diagonal corner pair along the destination row. */
        cellput(&rs, 3, 3, 0x8u, 1, 0, 0);
        cellput(&rs, 3, 4, 8u, 1, 0, 0);
        cellput(&rs, 4, 4, 8u, 1, 0, 0);
        CHECK(terrain_cell_route_and_occupancy_check(NULL, &rs, 3, 3, 4, 4,
                                                     0) == 2,
              "the blocked corner pair answers 2");
        cellput(&rs, 3, 4, 0u, 1, 0, 0);
        CHECK(terrain_cell_route_and_occupancy_check(NULL, &rs, 3, 3, 4, 4,
                                                     0) == 1,
              "one open corner drops back to the plain answer");
        cellput(&rs, 4, 4, 0u, 1, 0, 0);
        cellput(&rs, 3, 3, 0u, 1, 1, 0);

        /* The owner gate: +0x20000 stats without the +0x36c owner. */
        wordput(stats, 0x144, 1u | 0x2000000u | 0x20000u);
        CHECK(terrain_cell_route_and_occupancy_check(&mover, &rs, 3, 3, 3, 3,
                                                     1) == 0,
              "the owner-less +0x20000 entity answers zero");
        mover.owner_36c = (const void*)&rs;
        CHECK(terrain_cell_route_and_occupancy_check(&mover, &rs, 3, 3, 3, 3,
                                                     1) == 1,
              "the installed owner reopens the entity");
        mover.owner_36c = NULL;

        /* The type-11 entity gate. */
        mover.type = 0xb;
        CHECK(terrain_cell_route_and_occupancy_check(&mover, &rs, 3, 3, 3, 3,
                                                     1) == 0,
              "the type-11 entity is refused");
        mover.type = 1;
        wordput(stats, 0x144, 1u | 0x2000000u);

        /* The on-site answer (strict skips it). */
        cellput(&rs, 3, 3, 0u, 1, 0, 0);
        mover.x = 3.0f;
        mover.y = 5.0f;
        CHECK(terrain_cell_route_and_occupancy_check(&mover, &rs, 3, 3, 3, 5,
                                                     0) == 1,
              "the entity standing on the destination row answers in place");
        CHECK(terrain_cell_route_and_occupancy_check(&mover, &rs, 3, 3, 3, 5,
                                                     1) == 0,
              "strict skips the in-place answer (the cell reads empty)");
        mover.x = 1.0f;
        mover.y = 1.0f;

        /* Type gates: 9 needs the +0x1000000 stats bit, 6 the state mask. */
        cellput(&rs, 5, 5, 0u, 9, 1, 0);
        CHECK(terrain_cell_route_and_occupancy_check(&mover, &rs, 5, 5, 5, 5,
                                                     1) == 0,
              "the idle-type cell needs the stats relief bit");
        wordput(stats, 0x144, 1u | 0x2000000u | 0x1000000u);
        CHECK(terrain_cell_route_and_occupancy_check(&mover, &rs, 5, 5, 5, 5,
                                                     1) == 1,
              "the idle-type cell answers once relieved");
        cellput(&rs, 7, 7, 0u, 6, 1, 0);
        CHECK(terrain_cell_route_and_occupancy_check(&mover, &rs, 7, 7, 7, 7,
                                                     1) == 0,
              "the six-type cell needs the state mask or 0x200");
        wordput(stats, 0x144, 1u | 0x2000000u);
        wordput(stats, 0x148, 0x200000u);
        CHECK(terrain_cell_route_and_occupancy_check(&mover, &rs, 7, 7, 7, 7,
                                                     1) == 1,
              "the state mask opens the six-type cell");
        wordput(stats, 0x148, 0u);

        /* Open-cell promotion + the cardinal slot-0 quirk. */
        cellput(&rs, 6, 3, 0x18u, 1, 1, 0);
        CHECK(terrain_cell_route_and_occupancy_check(&mover, &rs, 6, 3, 6, 4,
                                                     1) == 3,
              "the open cell promotes the answer to 3");
        cellput(&rs, 6, 2, 0x10u, 1, 1, 0);
        cellput(&rs, 6, 3, 0x2000u, 1, 1, 0);
        CHECK(terrain_cell_route_and_occupancy_check(&mover, &rs, 6, 2, 6, 3,
                                                     1) == 2,
              "the slot-0 neighbour quirk reads the 0x2000 pair (answers 2)");
        cellput(&rs, 6, 2, 0u, 1, 1, 0);
        cellput(&rs, 6, 3, 0u, 1, 1, 0);

        /* Out-of-range / NULL guards read zero. */
        CHECK(terrain_cell_route_and_occupancy_check(NULL, NULL, 1, 1, 1, 1,
                                                     0) == 0
                  && terrain_cell_route_and_occupancy_check(NULL, &rs, 99, 0,
                                                            99, 0, 0) == 0,
              "the guarded level and grid answer zero");

        /* FirstFreeNeighborCellPick. */
        memset(voxels, 0, sizeof voxels);
        int center[2] = {4, 4};
        int out[2] = {0, 0};
        const uint32_t rot_before = neighbor_pick_rotation_peek();
        CHECK(first_free_neighbor_cell_pick(NULL, &rs, center, out, 0) == 0,
              "the empty grid finds nothing");
        CHECK(neighbor_pick_rotation_peek() == rot_before + 1u,
              "the pick advances the corpus rotation counter");

        cellput(&rs, 5, 4, 0x10u, 1, 1, 0); /* open, unblocked */
        CHECK(first_free_neighbor_cell_pick(&mover, &rs, center, out, 0) == 1
                  && out[0] == 5 && out[1] == 4,
              "mode 0 takes the open neighbour (rotation order aside)");
        CHECK(first_free_neighbor_cell_pick(&mover, &rs, center, out, 1) == 0,
              "mode 1 skips the already-open neighbour");
        cellput(&rs, 5, 4, 0u, 1, 1, 0);
        cellput(&rs, 4, 3, 8u, 1, 1, 0); /* blocked candidate for mode 1 */
        CHECK(first_free_neighbor_cell_pick(&mover, &rs, center, out, 1) == 1
                  && out[0] == 4 && out[1] == 3,
              "mode 1 tries the non-open neighbour (route answers blocked)");

        /* The NULL guards ride the same hooks. */
        CHECK(first_free_neighbor_cell_pick(NULL, NULL, center, out, 0) == 0,
              "the NULL level finds nothing");
    }

    /* TutorialPathCoordinateIndex (2026-10-25): the consumed-cell chain
     * sweep over the level block's +0x114 table. */
    {
        static LevelState g;
        memset(&g, 0, sizeof g);
        static uint8_t vox[4 * 3 * 0x48];
        memset(vox, 0, sizeof vox);
        level_set_int(&g, LEVEL_OFF_WIDTH, 4);
        level_set_int(&g, LEVEL_OFF_HEIGHT, 3);
        level_store_ptr(&g, LEVEL_OFF_VOXELS, vox);
        vox[(1 * 4 + 1) * 0x48 + 4] = 3;   /* first  (x=1, y=1) */
        vox[(2 * 4 + 2) * 0x48 + 4] = 3;   /* later: wins the head (x=2, y=2) */
        vox[(0 * 4 + 3) * 0x48 + 4] = 7;   /* singleton (x=3, y=0) */
        vox[(2 * 4 + 0) * 0x48 + 4] = 200; /* out-of-range byte: guarded */
        tutorial_path_coordinate_index(&g);

        CHECK(g.pool_heads[3] != NULL && g.pool_heads[7] != NULL,
              "both coordinates chain");
        void* n3 = g.pool_heads[3];
        void* n7 = g.pool_heads[7];
        uint32_t nx = 0, ny = 0;
        if (n3 != NULL) {
            __builtin_memcpy(&nx, (char*)n3 + 0x0, 4);
            __builtin_memcpy(&ny, (char*)n3 + 0x4, 4);
        }
        CHECK(n3 != NULL && nx == 2 && ny == 2,
              "the last-swept cell owns the head");
        void* n3b = NULL;
        if (n3 != NULL) {
            __builtin_memcpy(&n3b, (char*)n3 + 0x8, sizeof n3b);
        }
        void* next3 = NULL;
        if (n3 != NULL) {
            __builtin_memcpy(&next3, (char*)n3 + 0x8, sizeof next3);
        }
        CHECK(next3 != NULL && next3 != n3 && next3 == n3b,
              "the earlier cell rides the next link");
        if (n3b != NULL) {
            __builtin_memcpy(&nx, (char*)n3b + 0x0, 4);
            __builtin_memcpy(&ny, (char*)n3b + 0x4, 4);
        }
        CHECK(n3b != NULL && nx == 1 && ny == 1,
              "the first-swept cell trails at (1, 1)");
        void* next3b = (void*)0xdeadbeef;
        if (n3b != NULL) {
            __builtin_memcpy(&next3b, (char*)n3b + 0x8, sizeof next3b);
        }
        CHECK(next3b == NULL, "the chain terminates");
        void* next7 = (void*)0xdeadbeef;
        if (n7 != NULL) {
            __builtin_memcpy(&next7, (char*)n7 + 0x8, sizeof next7);
        }
        CHECK(next7 == NULL, "the singleton chain is one node");

        CHECK(vox[(1 * 4 + 1) * 0x48 + 4] == 0
                  && vox[(2 * 4 + 2) * 0x48 + 4] == 0
                  && vox[(0 * 4 + 3) * 0x48 + 4] == 0,
              "the swept coordinate bytes are consumed");

        /* Re-zeroing re-runs leak the superseded chains (corpus-faithful). */
        tutorial_path_coordinate_index(&g);
        CHECK(g.pool_heads[3] == NULL && g.pool_heads[7] == NULL,
              "the re-sweep zeroes the table (nothing left to sweep)");

        /* Free the live nodes the re-sweep orphaned. */
        if (n3 != NULL) {
            engine_heap_free(n3b);
            engine_heap_free(n3);
        }
        if (n7 != NULL) {
            engine_heap_free(n7);
        }
    }

    if (s_failures != 0) {
        fprintf(stderr, "level_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("level_test: OK\n");
    return 0;
}
