// SPDX-License-Identifier: LGPL-3.0-or-later
// Engine <-> level binding (seam #35 + the 64-bit pointer fix).
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "resource.hpp"
#include "game/level.hpp"
#include "game/test_level_fixture.hpp"
#include "gods98/camera.hpp"
#include "gods98/engine.hpp"
#include "gods98/input.hpp"
#include "gods98/view.hpp"

static int s_failures = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) {                                                         \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg);      \
            ++s_failures;                                                      \
        }                                                                      \
    } while (0)

#define GRID 8

static const char* FIX = "bind_fixture";

static void write_map(const char* name, int fill)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", FIX, name);
    FILE* fp = fopen(path, "wb");
    if (fp == NULL) {
        return;
    }
    const char magic[4] = { 'M', 'A', 'P', ' ' };
    uint32_t len = GRID * GRID * 2, ww = GRID, hh = GRID;
    fwrite(magic, 1, 4, fp);
    fwrite(&len, 1, 4, fp);
    fwrite(&ww, 1, 4, fp);
    fwrite(&hh, 1, 4, fp);
    for (int i = 0; i < GRID * GRID; ++i) {
        uint16_t v = (uint16_t)fill;
        fwrite(&v, 1, 2, fp);
    }
    fclose(fp);
}

/* Build the EngineContext the way the frame loop does (current field order). */
static void make_ctx(EngineContext* ctx, LevelState* lvl,
                     EngineState* st, InputState* in, ViewContext* views,
                     int view_mode)
{
    input_reset(in);
    for (int i = 0; i < 3; ++i) {
        memset(&views[i], 0, sizeof(views[i]));
        views[i].mode = view_mode;
        views[i].step = 8.0f;
    }
    memset(ctx, 0, sizeof(*ctx));
    ctx->app_mode = ENGINE_APP_NORMAL;
    engine_app_mode_global_set(ctx->app_mode);
    ctx->context_mode = 1;
    engine_context_mode_global_set(ctx->context_mode);
    ctx->state_block = st;
    ctx->input = in;
    ctx->views[0] = &views[0];
    ctx->views[1] = &views[1];
    ctx->views[2] = &views[2];
    /* Bind: the raw corpus slot cannot carry the pointer on 64-bit, so the
     * resolved voxel/terrain pointers are bound exactly as the seam does. */
    ctx->level_block = lvl;
    ctx->level_voxels = (const uint8_t*)level_load_ptr(lvl, LEVEL_OFF_VOXELS);
    ctx->level_terrain = level_load_ptr(lvl, LEVEL_OFF_TERRAIN);
}

int main(void)
{
    char rmcmd[64]; snprintf(rmcmd, sizeof(rmcmd), "rm -rf %s", FIX); system(rmcmd);
    mkdir(FIX, 0755);
    {
        char path[512];
        snprintf(path, sizeof(path), "%s/Lego.cfg", FIX);
        FILE* fp = fopen(path, "w");
        if (fp != NULL) {
            fputs(
                "Lego {\n"
                "  L1 {\n"
                "    blocksize 2\n"
                "    SurfaceMap L1_s.map\n"
                "    PredugMap L1_p.map\n"
                "    TerrainMap L1_t.map\n"
                "    BlockPointersMap L1_bp.map\n"
                /* The faithful LevelLoad opens every layer unconditionally
                 * (an unset key reaches MapOpen with an empty path). */
                "    CryOreMap L1_c.map\n"
                "    ErodeMap L1_e.map\n"
                "    PathMap L1_pa.map\n"
                "    EmergeMap L1_em.map\n"
                "    AIMap L1_ai.map\n"
                "    FallinMap L1_f.map\n"
                TLF_LEVEL_KEYS
                "  }\n"
                TLF_ROOT_GROUPS
                "}\n",
                fp);
            fclose(fp);
        }
    }
    tlf_write_aux_files(FIX);
    write_map("L1_s.map", 4);  /* heights */
    write_map("L1_p.map", 0);
    write_map("L1_t.map", 1);
    write_map("L1_bp.map", 7); /* surface-type byte at voxel +0x04 == 7 */
    write_map("L1_c.map", 0);
    write_map("L1_e.map", 0);
    write_map("L1_pa.map", 0);
    write_map("L1_em.map", 0);
    write_map("L1_ai.map", 0);
    write_map("L1_f.map", 0);
    if (chdir(FIX) != 0) {
        fprintf(stderr, "chdir failed\n");
        return 1;
    }
    /* File opens resolve under the CD root; the fixture is the CWD. */
    resource_set_cd_root(".");
    resource_set_cd_data_dir(".");

    BlockFile* cfg = block_open("Lego.cfg");
    CHECK(cfg != NULL, "open cfg");
    GameContext level_ctx;
    tlf_bind_context(&level_ctx);
    LevelState* lvl = level_load(cfg, "L1");
    CHECK(lvl != NULL, "level loads");
    if (lvl == NULL) {
        return 1;
    }

    EngineState state = {};
    InputState  input = {};
    ViewContext views[3];
    EngineContext ctx;
    make_ctx(&ctx, lvl, &state, &input, views, 1 /* follow */);

    /* --- the engine reads the bound level -------------------------------- */
    CHECK(engine_level_width(&ctx) == GRID, "bind: width from the level");
    CHECK(engine_level_height(&ctx) == GRID, "bind: height from the level");
    CHECK(engine_level_blocksize(&ctx) == 2.0f, "bind: blocksize (level +0x1c)");
    /* The resolved voxel pointer: without it the 64-bit build reads a
     * truncated +0xb8 word and this would dereference garbage. */
    /* level_load now ends with TutorialPathCoordinateIndex, which consumes
     * (clears) the +0x04 byte it threads onto the coordinate chains, so the
     * read-through is checked on the TerrainMap byte at +0x02 instead. */
    CHECK(engine_voxel_byte(&ctx, 1, 1, 0x02) == 1,
          "bind: voxel terrain byte through the resolved array");

    /* --- view mode-1 follow runs against the real grid -------------------- */
    cam_set_pos(&views[0].cam_follow, (float)GRID * 0.5f, (float)GRID * 0.5f, 0);
    view_views_tick(&ctx, 0.1f, 1.0f);
    {
        float pos[3] = { 0, 0, 0 };
        cam_get_pos(&views[0].cam_facing, pos);
        CHECK(pos[0] >= 0.0f && pos[0] < (float)GRID + 1.0f
              && pos[1] >= 0.0f && pos[1] < (float)GRID + 1.0f,
              "view: follow camera clamped inside the level bounds");
        CHECK((int)pos[2] >= 0 && (int)pos[2] <= 15,
              "view: terrain height read from the bounding voxel");
    }

    /* --- the engine's camera grid sweep walks the real voxel grid --------- */
    g_camera_grid_enabled = 1;
    {
        /* simulate a busy cell (block-pointer busy count at voxel +0x3c) */
        uint8_t* vox = (uint8_t*)level_load_ptr(lvl, LEVEL_OFF_VOXELS);
        uint32_t busy = 1;
        __builtin_memcpy(vox + (2 * GRID + 2) * LEVEL_VOXEL_STRIDE + 0x3c,
                         &busy, 4);
        camera_cluster(&ctx, 0.5f, 1.0f);
        float t0 = 0.0f;
        __builtin_memcpy(&t0,
                         vox + (2 * GRID + 2) * LEVEL_VOXEL_STRIDE + 0x40, 4);
        CHECK(t0 > 0.0f, "bind: camera grid sweep advanced the cell timer");
    }
    g_camera_grid_enabled = 0;

    level_destroy(lvl);
    block_close(cfg);

    if (s_failures != 0) {
        fprintf(stderr, "bind_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("bind_test: OK\n");
    return 0;
}
