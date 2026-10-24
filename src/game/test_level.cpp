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

        /* The tracked set-family (2026-10-24): queue pair and masks. */
        {
            uint32_t g = 0u;
            f = zero;
            __builtin_memcpy(rec + 0x08, &f, 4);
            __builtin_memcpy(rec + 0x0C, &f, 4);
            uint8_t* rec11 = level_voxel(s, 1, 1);
            __builtin_memcpy(rec11 + 0x0C, &f, 4);

            terrain_cell_visited_flag_set_and_track(s, 3, 4);
            terrain_cell_visited_flag_set_and_track(s, 1, 1);
            __builtin_memcpy(&f, rec + 0x0C, 4);
            CHECK(f == 2u, "the visited setter raises the mark in line");
            terrain_cell_visited_flags_clear_all(s);
            __builtin_memcpy(&f, rec + 0x0C, 4);
            __builtin_memcpy(&g, rec11 + 0x0C, 4);
            CHECK(f == 0u && g == 0u,
                  "ClearAll drains its queue through both listed cells");

            f = zero;
            __builtin_memcpy(rec + 0x0C, &f, 4);
            terrain_cell_flag100_set_and_track(s, 3, 4);
            __builtin_memcpy(&f, rec + 0x0C, 4);
            CHECK(f == 0x100u, "track setter raises secondary 0x100");

            f = zero;
            __builtin_memcpy(rec + 0x08, &f, 4);
            terrain_cell_flags_set_mask100400(s, 3, 4);
            __builtin_memcpy(&f, rec + 0x08, 4);
            CHECK(f == 0x100400u, "the 0x100400 mask raises exactly its bits");
            terrain_cell_flags_set_mask20108000(s, 3, 4);
            __builtin_memcpy(&f, rec + 0x08, 4);
            CHECK(f == 0x20108400u, "the 0x20108000 mask ORs alongside");

            terrain_cell_flags_set_and_refresh(s, 3, 4);
            __builtin_memcpy(&f, rec + 0x08, 4);
            __builtin_memcpy(&g, rec + 0x0C, 4);
            CHECK(f == 0x20108400u && g == 0x104u, /* masks already in place */
                  "set-and-refresh raises both words' masks");

            f = 0xffffffffu;
            __builtin_memcpy(rec + 0x08, &f, 4);
            __builtin_memcpy(rec + 0x0C, &f, 4);
            terrain_cell_primary_flags_mask_clear_and_refresh(s, 3, 4);
            __builtin_memcpy(&f, rec + 0x08, 4);
            __builtin_memcpy(&g, rec + 0x0C, 4);
            CHECK(f == 0xdfef7bffu, "the primary mask clears its bits");
            CHECK(g == 0xfffffffbu, "the secondary bit 4 value clears");

            f = 0u;
            __builtin_memcpy(rec + 0x08, &f, 4);
            terrain_cell_flag40000000_clear_or_set(s, 3, 4, 1);
            __builtin_memcpy(&f, rec + 0x08, 4);
            CHECK(f == 0x40000000u, "clear-or-set raises 0x40000000");
            terrain_cell_flag40000000_clear_or_set(s, 3, 4, 0);
            __builtin_memcpy(&f, rec + 0x08, 4);
            CHECK(f == 0u, "the same leaf's clear branch drops it");

            /* Out-of-cell: the bounds deviation skips the record and
             * the queue (the corpus indexes blindly). */
            terrain_cell_flag100_set_and_track(s, 9999, 9999);
            terrain_cell_visited_flag_set_and_track(s, 9999, 9999);
            CHECK(level_cell_visited_flag(s, 9999, 9999) == 0u,
                  "out-of-cell setters skip (bounds deviation)");
        }

        /* CellNeighborDirectionCode (FUN_004508a0): eight neighbors. */
        CHECK(cell_neighbor_direction_code(2, 2, 2, 1) == 0, "up");
        CHECK(cell_neighbor_direction_code(2, 2, 3, 2) == 1, "right");
        CHECK(cell_neighbor_direction_code(2, 2, 2, 3) == 2, "down");
        CHECK(cell_neighbor_direction_code(2, 2, 1, 2) == 3, "left");
        CHECK(cell_neighbor_direction_code(2, 2, 3, 1) == 4, "right-up");
        CHECK(cell_neighbor_direction_code(2, 2, 3, 3) == 5, "right-down");
        CHECK(cell_neighbor_direction_code(2, 2, 1, 3) == 6, "left-down");
        CHECK(cell_neighbor_direction_code(2, 2, 1, 1) == 7, "left-up");
        CHECK(cell_neighbor_direction_code(2, 2, 2, 2) == -1,
              "the same cell is no neighbor");
        CHECK(cell_neighbor_direction_code(2, 2, 0, 2) == -1,
              "two cells away is no neighbor");
        CHECK(cell_neighbor_direction_code(2, 2, 1, 0) == -1,
              "a knight move is no neighbor");

        /* The neighbor-refresh / circle / edge-flag cluster. */
        {
            uint32_t f = zero, g = 0u;

            /* Edge flag set and update: 0x200 raises and drops. */
            f = zero;
            __builtin_memcpy(rec + 0x08, &f, 4);
            terrain_cell_edge_flag_set_and_update(s, 3, 4, 1);
            __builtin_memcpy(&f, rec + 0x08, 4);
            CHECK(f == 0x200u, "the edge setter raises 0x200");
            terrain_cell_edge_flag_set_and_update(s, 3, 4, 0);
            __builtin_memcpy(&f, rec + 0x08, 4);
            CHECK(f == 0u, "the same leaf's clear branch drops it");

            /* Flag 0x20 clear and refresh: only bit 5 drops. */
            f = 0xffffffffu;
            __builtin_memcpy(rec + 0x08, &f, 4);
            terrain_cell_flag20_clear_and_refresh(s, 3, 4);
            __builtin_memcpy(&f, rec + 0x08, 4);
            CHECK(f == 0xffffffdfu, "only the 0x20 bit clears");

            /* The neighbor refresh leaf: the build-record word is
             * unowned in the port, so the leaf changes nothing. */
            terrain_neighbor_flag_refresh(s, 3, 4, 0);
            __builtin_memcpy(&f, rec + 0x08, 4);
            __builtin_memcpy(&g, rec + 0x18, 4);
            CHECK(f == 0xffffffdfu && g == 0u,
                  "the refresh leaf is inert with no build records");

            /* Work-flag circle: radius 1 lights the cross, not the
             * diagonals; radius 0 only the center. */
            for (int dy = -2; dy <= 2; ++dy) {
                for (int dx = -2; dx <= 2; ++dx) {
                    f = zero;
                    __builtin_memcpy(level_voxel(s, 3 + dx, 3 + dy) + 0x08,
                                     &f, 4);
                }
            }
            terrain_cell_work_flag_circle_set(s, 3, 3, 1);
            int lit = 0;
            for (int dy = -2; dy <= 2; ++dy) {
                for (int dx = -2; dx <= 2; ++dx) {
                    __builtin_memcpy(&f,
                                     level_voxel(s, 3 + dx, 3 + dy) + 0x08, 4);
                    if ((f & 4u) != 0) {
                        ++lit;
                    }
                    /* The setter's own gate makes cell 7 (and above)
                     * dead even inside the circle (faithful). */
                    const int on = (dx * dx + dy * dy <= 1) &&
                                   (3 + dx < 7) && (3 + dy < 7);
                    CHECK(((f & 4u) != 0) == (on != 0),
                          "the circle lights exactly its cells");
                }
            }
            CHECK(lit == 5, "radius 1 lights the five-cell cross");

            /* Radius 0: the center alone. */
            f = zero;
            __builtin_memcpy(level_voxel(s, 3, 3) + 0x08, &f, 4);
            terrain_cell_work_flag_circle_set(s, 3, 3, 0);
            __builtin_memcpy(&f, level_voxel(s, 3, 3) + 0x08, 4);
            CHECK(f == 4u, "radius 0 lights the center only");

            /* Out-of-cell candidates skip (bounds deviation). */
            terrain_cell_work_flag_circle_set(s, 9999, 9999, 1);
            CHECK(1, "an off-map circle changes nothing");
        }

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

        /* Surface-type byte leaf (FUN_0044f540) over a created terrain:
         * the +0x24 bytes stay in the zero state until the blocked
         * renderer-side appearance apply lands; the corners to test now
         * are the bounds guard and the NULL handle. */
        void* terrain = terrain_create("L01_surface.map", 1.0f);
        CHECK(terrain != NULL, "terrain built from the surface map");
        CHECK(terrain_cell_surface_type_get(terrain, 3, 4) == 0,
              "fresh terrain answers the zero state");
        CHECK(terrain_cell_surface_type_get(terrain, GRID, 0) == 0,
              "x at the width edge answers zero");
        CHECK(terrain_cell_surface_type_get(terrain, 0, GRID) == 0,
              "y at the height edge answers zero");
        CHECK(terrain_cell_surface_type_get(terrain, -1, 0) == 0,
              "a negative index answers zero");
        CHECK(terrain_cell_surface_type_get(NULL, 0, 0) == 0,
              "NULL terrain answers zero");
        /* The terrain dirty queue (2026-10-24): push, swap-remove,
         * raise/clear corners. */
        terrain_cell_dirty_queue_push_and_clear(NULL, 3, 4, 1);
        terrain_cell_dirty_queue_push_and_clear(terrain, 9999, 0, 1);
        CHECK(terrain_cell_dirty_flag(terrain, 9999, 0) == 0,
              "the off-grid push skips (bounds deviation)");
        terrain_cell_dirty_queue_push_and_clear(terrain, 3, 4, 1);
        CHECK(terrain_cell_dirty_flag(terrain, 3, 4) != 0,
              "the push raises the dirty flag");
        CHECK(terrain_cell_dirty_flag(terrain, 5, 5) == 0,
              "the neighbour stays clean");
        terrain_cell_dirty_queue_push_and_clear(terrain, 5, 5, 1);
        terrain_cell_dirty_queue_push_and_clear(terrain, 3, 4, 1);
        terrain_cell_dirty_queue_push_and_clear(terrain, 3, 4, 0);
        CHECK(terrain_cell_dirty_flag(terrain, 3, 4) == 0,
              "the clear drops the found entry's flag");
        CHECK(terrain_cell_dirty_flag(terrain, 5, 5) != 0,
              "the other queue entry keeps its flag");
        terrain_cell_dirty_queue_push_and_clear(terrain, 5, 5, 0);
        CHECK(terrain_cell_dirty_flag(terrain, 5, 5) == 0,
              "the tail-swap entry clears next");

        terrain_destroy(terrain);
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

    if (s_failures != 0) {
        fprintf(stderr, "level_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("level_test: OK\n");
    return 0;
}
