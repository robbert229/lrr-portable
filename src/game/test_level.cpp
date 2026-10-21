// SPDX-License-Identifier: LGPL-3.0-or-later
// Level `.map` loading pipeline (finding 07) — synthetic grid fixture.
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "game/level.hpp"
#include "game/util.hpp"
#include "gods98/block.hpp"
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
        snprintf(path, sizeof(path), "%s %s SurfaceMap", util_root_name(cfg),
                 block_name(g));
        if (block_get_strptr(cfg, path) != NULL) {
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

    level_destroy(s);
    block_close(cfg);

    if (s_failures != 0) {
        fprintf(stderr, "level_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("level_test: OK\n");
    return 0;
}
