// SPDX-License-Identifier: LGPL-3.0-or-later
// Software level renderer (finding 22 §3).
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game/entity.hpp"
#include "game/game.hpp"
#include "game/level.hpp"
#include "game/render.hpp"
#include "game/select.hpp"

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

    /* Selected entities get a yellow ring around the marker. */
    selection_add(&ctx, a);
    level_render_map(lvl, &ctx, -1, -1, PW, PH, buf, PITCH);
    is_pixel(buf, 4, 8, 250, 220, 60, "selection ring yellow (left edge)");

    /* Cursor cell outline renders white at the mouse-mapped cell. */
    level_render_map(lvl, &ctx, 3 * (PW / GW), 1 * (PH / GH), PW, PH, buf, PITCH);
    is_pixel(buf, 12, 3, 240, 240, 240, "cursor cell outline");

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