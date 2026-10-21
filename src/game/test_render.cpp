// SPDX-License-Identifier: LGPL-3.0-or-later
// Software level renderer (finding 22 §3).
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "gods98/texture.hpp"
#include "game/entity.hpp"
#include "game/game.hpp"
#include "game/level.hpp"
#include "game/render.hpp"
#include "game/select.hpp"
#include "gods98/model.hpp"

static int s_failures = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) {                                                         \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg);      \
            ++s_failures;                                                      \
        }                                                                      \
    } while (0)

#define GW 16
#define GH 16
#define PW 64
#define PH 56
#define PITCH (PW * 4)

static void pixel(uint8_t* buf, int x, int y, uint8_t* r, uint8_t* g, uint8_t* b)
{
    uint8_t* p = buf + ((size_t)y * PITCH) + (size_t)x * 4;
    *r = p[0]; *g = p[1]; *b = p[2];
}

static void is_pixel(uint8_t* buf, int x, int y,
                     uint8_t r, uint8_t g, uint8_t b, const char* msg)
{
    uint8_t pr, pg, pb;
    pixel(buf, x, y, &pr, &pg, &pb);
    CHECK(pr == r && pg == g && pb == b, msg);
}

/* Mirror of the renderer's reward dim: c -> c*3/8. */
static uint8_t dim3_8(uint8_t c)
{
    return (uint8_t)(((unsigned)c * 3u) / 8u);
}

/* --- tiny synthetic LWO (a diamond quad in x/z) for model-aware tests --- */
static uint8_t m_buf[1024];

static void m_w16(uint8_t** p, unsigned v)
{
    (*p)[0] = (uint8_t)(v >> 8); (*p)[1] = (uint8_t)v; *p += 2;
}
static void m_w32(uint8_t** p, unsigned v)
{
    (*p)[0] = (uint8_t)(v >> 24); (*p)[1] = (uint8_t)(v >> 16);
    (*p)[2] = (uint8_t)(v >> 8);  (*p)[3] = (uint8_t)v; *p += 4;
}
static void m_wfl(uint8_t** p, float f)
{
    uint8_t t[4]; memcpy(t, &f, 4);
    (*p)[0] = t[3]; (*p)[1] = t[2]; (*p)[2] = t[1]; (*p)[3] = t[0]; *p += 4;
}
static void m_wtag(uint8_t** p, const char* t)
{
    memcpy(*p, t, 4); *p += 4;
}
static uint8_t* m_form_begin(void)
{
    uint8_t* p = m_buf;
    m_wtag(&p, "FORM");
    m_w32(&p, 0);
    m_wtag(&p, "LWOB");
    return p;
}
static void m_form_end(uint8_t* p)
{
    uint8_t* q = m_buf + 4;
    m_w32(&q, (unsigned)(p - m_buf - 8));
}
/* A single 4-gon diamond in the x/z plane, extent 8. */
static const void* build_diamond_model(void)
{
    uint8_t* p = m_form_begin();
    m_wtag(&p, "PNTS");
    m_w32(&p, 48);
    /* diamond in (x,z): (0,-4),(4,0),(0,4),(-4,0); y = 0 */
    const float v[12] = { 0, 0, -4,   4, 0, 0,   0, 0, 4,  -4, 0, 0 };
    for (int i = 0; i < 12; ++i) m_wfl(&p, v[i]);
    uint8_t pols[16];
    uint8_t* q = pols;
    m_w16(&q, 4);
    m_w16(&q, 1); m_w16(&q, 2); m_w16(&q, 3); m_w16(&q, 4);
    m_w16(&q, 1); /* surface (1-based -> slot 0) */
    m_wtag(&p, "POLS");
    m_w32(&p, (unsigned)(q - pols));
    for (uint8_t* w = pols; w != q; ) { *p++ = *w++; }
    m_form_end(p);
    return model_parse(m_buf, (size_t)(p - m_buf));
}

/* Write a 2x2 24-bit BI_RGB BMP for the wallpaper blit tests. File rows are
 * bottom-up: texels { red, green } on top, { blue, yellow } on bottom, so
 * texture_decode flips them to t->pixels = { red, green, blue, yellow }.
 * Returns 0 on any I/O failure (caller skips the assertion then). */
static int write_wall_bmp(char* path, size_t path_sz)
{
    const char* dir = getenv("TMPDIR");
    if (dir == NULL || dir[0] == '\0') {
        dir = "/tmp/opencode";
    }
    snprintf(path, path_sz, "%s/render_wall_test.bmp", dir);

    uint8_t data[70];
    memset(data, 0, sizeof(data));
    data[0] = 'B'; data[1] = 'M';
    data[2] = 70;                /* file size (little-endian u32) */
    data[10] = 54;               /* pixel data offset */
    data[14] = 40;               /* BITMAPINFOHEADER size */
    data[18] = 2;                /* width */
    data[22] = 2;                /* height (positive: bottom-up rows) */
    data[26] = 1;                /* planes */
    data[28] = 24;               /* bpp, BI_RGB */
    /* Row 0 (bottom of image): blue, yellow — BGR, 2 pad bytes. */
    uint8_t* row0 = data + 54;
    row0[0] = 255; row0[1] = 0;   row0[2] = 0;   /* blue   */
    row0[3] = 0;   row0[4] = 255; row0[5] = 255; /* yellow */
    /* Row 1 (top of image): red, green. */
    uint8_t* row1 = row0 + 8;
    row1[0] = 0;   row1[1] = 0;   row1[2] = 255; /* red    */
    row1[3] = 0;   row1[4] = 255; row1[5] = 0;   /* green  */

    FILE* fp = fopen(path, "wb");
    if (fp == NULL) {
        return 0;
    }
    int ok = fwrite(data, 1, sizeof(data), fp) == sizeof(data);
    fclose(fp);
    return ok;
}

int main(void)
{
    /* Fake 16x16 level: voxel grid with a dirt cell (0,0) + crystal (1,1). */
    LevelState* lvl = (LevelState*)calloc(1, sizeof(LevelState));
    uint8_t* vox = (uint8_t*)calloc((size_t)GW * GH, LEVEL_VOXEL_STRIDE);
    vox[(0 * GW + 0) * LEVEL_VOXEL_STRIDE + 0x02] = 1;  /* dirt    */
    vox[(1 * GW + 1) * LEVEL_VOXEL_STRIDE + 0x02] = 6;  /* crystal */
    level_set_int(lvl, LEVEL_OFF_WIDTH, GW);
    level_set_int(lvl, LEVEL_OFF_HEIGHT, GH);
    level_store_ptr(lvl, LEVEL_OFF_VOXELS, vox);

    GameContext ctx;
    game_context_init(&ctx);
    int idx = entity_create(&ctx, 1, "diggerA", 2, 2, 0, 0);
    CHECK(idx >= 0, "entity placed");
    EntitySlot* a = (EntitySlot*)entity_at(&ctx, 1, 0);

    uint8_t* buf = (uint8_t*)calloc((size_t)PH * PITCH, 1);
    level_render_map(lvl, &ctx, -1, -1, PW, PH, buf, PITCH);

    /* Cell (0,0) is dirt, cell (1,1) crystal, far corner is void. */
    uint8_t r, g, b;
    pixel(buf, 2, 2, &r, &g, &b);      /* dirt block center */
    CHECK(!(r == 8 && g == 16 && b == 48), "dirt cell is not void");
    uint8_t r2, g2, b2;
    pixel(buf, 6, 6, &r2, &g2, &b2);   /* crystal block center */
    CHECK(!(r2 == r && g2 == g && b2 == b), "crystal cell differs from dirt");
    is_pixel(buf, 63, 52, 8, 16, 48, "void outside the grid stays background");

    /* Entity marker: cell (2,2) -> pixel (8,8), type-1 white. */
    is_pixel(buf, 8, 8, 240, 240, 240, "entity marker white");

    /* Model-aware: attach a synthetic LWO diamond; the projected wireframe
     * right corner of the diamond lands at (14,7) for this buffer. */
    const void* dm = build_diamond_model();
    CHECK(dm != NULL, "diamond model parses");
    if (dm != NULL) {
        a->model = dm; /* inject directly; entity_set_model covers the loader */
        level_render_map(lvl, &ctx, -1, -1, PW, PH, buf, PITCH);
        is_pixel(buf, 14, 7, 240, 240, 240, "model wireframe corner drawn");
        is_pixel(buf, 8, 7, 240, 240, 240, "model centre marker kept");
        a->model = NULL;
        model_free((Model*)dm);
        level_render_map(lvl, &ctx, -1, -1, PW, PH, buf, PITCH);
    }

    /* Selected entities get a yellow ring around the marker. */
    selection_add(&ctx, a);
    level_render_map(lvl, &ctx, -1, -1, PW, PH, buf, PITCH);
    is_pixel(buf, 4, 8, 250, 220, 60, "selection ring yellow (left edge)");

    /* Cursor cell outline renders white at the mouse-mapped cell. */
    level_render_map(lvl, &ctx, 3 * (PW / GW), 1 * (PH / GH), PW, PH, buf, PITCH);
    is_pixel(buf, 12, 3, 240, 240, 240, "cursor cell outline");

    /* --- reward overlay (finding 09) ------------------------------------- */
    ctx.level = lvl;                         /* banner reads obj result */
    level_set_int(lvl, LEVEL_OFF_OBJ_RESULT, 1);

    /* Capture a clean (pre-dim) level pixel for the dim assertions. */
    level_render_map(lvl, &ctx, -1, -1, PW, PH, buf, PITCH);
    uint8_t dr, dg, db;
    pixel(buf, 2, 20, &dr, &dg, &db);        /* rock cell, clear of banner */

    RewardScreen* rs = &ctx.reward_screen;
    rs->save_x = 10; rs->save_y = 30; rs->save_w = 24; rs->save_h = 12;
    rs->advance_x = 36; rs->advance_y = 30; rs->advance_w = 22; rs->advance_h = 12;
    strcpy(rs->complete_text, "Complete");
    rs->centre_text = 1;
    rs->display = 1;
    ctx.reward_active = 1;
    level_render_map(lvl, &ctx, 20, 35, PW, PH, buf, PITCH); /* hover Save */

    /* Deliverable 1: the level is dimmed behind the overlay (c -> c*3/8).
     * The void corner dims exactly; the rock cell matches the captured
     * pre-dim value scaled the same way. */
    is_pixel(buf, 63, 52, dim3_8(8), dim3_8(16), dim3_8(48),
             "reward: void background dimmed");
    is_pixel(buf, 2, 20, dim3_8(dr), dim3_8(dg), dim3_8(db),
             "reward: level cell dimmed under overlay");

    /* Deliverable 3: achieved (result 1) -> complete green banner, plus the
     * centre-text content strip row. */
    is_pixel(buf, 32, 2, 70, 190, 90, "reward: achieved banner green");
    is_pixel(buf, 32, 6, 162, 222, 172, "reward: centre-text strip");

    /* Deliverable 4: hovered Save is filled bright, Advance keeps a dark
     * outline and its interior stays the dimmed level. */
    is_pixel(buf, 20, 36, 150, 255, 150, "reward: hovered Save filled bright");
    is_pixel(buf, 36, 36, 45, 80, 120, "reward: Advance dark outline");
    is_pixel(buf, 45, 36, dim3_8(dr), dim3_8(dg), dim3_8(db),
             "reward: un-hovered Advance interior stays dimmed level");

    /* Failed -> red banner. */
    level_set_int(lvl, LEVEL_OFF_OBJ_RESULT, 0);
    rs->complete_text[0] = '\0';
    strcpy(rs->failed_text, "Failed");
    level_render_map(lvl, &ctx, -1, -1, PW, PH, buf, PITCH);
    is_pixel(buf, 32, 2, 200, 70, 60, "reward: failed banner red");

    /* Quit -> grey-blue banner. */
    rs->failed_text[0] = '\0';
    strcpy(rs->quit_text, "Quit");
    level_render_map(lvl, &ctx, -1, -1, PW, PH, buf, PITCH);
    is_pixel(buf, 32, 2, 100, 110, 130, "reward: quit banner grey");

    /* Deliverable 2: wallpaper blit (nearest, full-bleed) of a 2x2 texture.
     * texel (0,0)=red shows at (16,14) below the banner; texel (1,1)=yellow
     * shows inside the un-hovered Advance zone at (48,36). */
    char wall_path[512];
    if (write_wall_bmp(wall_path, sizeof(wall_path))) {
        rs->complete_text[0] = '\0';
        rs->failed_text[0] = '\0';
        rs->quit_text[0] = '\0';
        strcpy(rs->wallpaper, wall_path);
        ctx.reward_active = 1;
        level_render_map(lvl, &ctx, -1, -1, PW, PH, buf, PITCH);
        is_pixel(buf, 16, 14, 255, 0, 0, "reward: wallpaper texel (0,0) red");
        is_pixel(buf, 48, 36, 255, 255, 0, "reward: wallpaper texel (1,1) yellow");
        unlink(wall_path);
    } else {
        CHECK(0, "reward: temp wallpaper write failed");
    }

    ctx.reward_active = 0;
    rs->wallpaper[0] = '\0';
    texture_cache_clear();

    free(buf);
    free(lvl);
    /* vox is owned by lvl's pointer registry; free by hand in the test. */
    free(vox);

    if (s_failures != 0) {
        fprintf(stderr, "render_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("render_test: OK\n");
    return 0;
}