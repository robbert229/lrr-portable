// SPDX-License-Identifier: LGPL-3.0-or-later
#include <string.h>

#include "entity.hpp"
#include "level.hpp"
#include "render.hpp"

/* --- framebuffer helpers (RGBA32 byte order R,G,B,A on little-endian) --- */

static void px_set(uint8_t* px, uint8_t r, uint8_t g, uint8_t b)
{
    px[0] = r;
    px[1] = g;
    px[2] = b;
    px[3] = 255;
}

static uint8_t* px_at(int x, int y, int w, int h, void* pixels, int pitch)
{
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= w) x = w - 1;
    if (y >= h) y = h - 1;
    return (uint8_t*)pixels + (size_t)y * (size_t)pitch + (size_t)x * 4;
}

static void fill_rect(int x0, int y0, int x1, int y1,
                      int w, int h, void* pixels, int pitch,
                      uint8_t r, uint8_t g, uint8_t b)
{
    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            px_set(px_at(x, y, w, h, pixels, pitch), r, g, b);
        }
    }
}

/* --- voxel → colour ------------------------------------------------------
 * Base material from the erosion/cryo byte (+0x02, finding 07), brightness
 * from the surface byte (+0x04). The palette is ours; semantics follow the
 * documented cell meanings (1 = diggable, 6 = crystal seam, 11 = ore). */
static void cell_colour(const uint8_t* vox, uint8_t* r, uint8_t* g, uint8_t* b)
{
    uint8_t m = vox[0x02];
    uint8_t s = vox[0x04];
    uint8_t base_r, base_g, base_b;
    switch (m) {
    case 1:   base_r = 176; base_g = 132; base_b =  84; break; /* dirt     */
    case 6:   base_r =  60; base_g = 200; base_b = 220; break; /* crystal  */
    case 11:  base_r = 235; base_g = 180; base_b =  50; break; /* ore      */
    default:  base_r =  92; base_g =  78; base_b =  66; break; /* rock     */
    }
    /* subtle brightness from the surface byte */
    uint8_t shade = (uint8_t)(96 + (s & 7u) * 18u);
    *r = (uint8_t)((unsigned)base_r * shade >> 8);
    *g = (uint8_t)((unsigned)base_g * shade >> 8);
    *b = (uint8_t)((unsigned)base_b * shade >> 8);
}

static void entity_colour(int type, uint8_t* r, uint8_t* g, uint8_t* b)
{
    switch (type) {
    case 2:  *r = 60;  *g = 200; *b = 60;  break; /* minifig green */
    case 3:  *r = 230; *g = 130; *b = 60;  break; /* object orange */
    case 4:  *r = 150; *g = 150; *b = 165; break; /* building grey */
    case 6:  *r = 220; *g = 220; *b = 90;  break; /* minion yellow */
    default: *r = 240; *g = 240; *b = 240; break; /* vehicle white */
    }
}

/* --- the frame ----------------------------------------------------------- */

void level_render_map(const LevelState* lvl, const GameContext* ctx,
                      int mouse_x, int mouse_y,
                      int w, int h, void* pixels, int pitch)
{
    if (lvl == NULL || ctx == NULL || pixels == NULL || w <= 0 || h <= 0) {
        return;
    }
    int gw = level_width(lvl);
    int gh = level_height(lvl);
    if (gw <= 0 || gh <= 0) {
        return;
    }
    /* Whole-frame background (void) so regions outside the grid are not
     * stale garbage from the previous frame. */
    fill_rect(0, 0, w - 1, h - 1, w, h, pixels, pitch, 8, 16, 48);
    /* Terrain grid: one cell maps to a W/gw x H/gh block. */
    int cell_w = w / gw;
    int cell_h = h / gh;
    if (cell_w < 1) cell_w = 1;
    if (cell_h < 1) cell_h = 1;
    const uint8_t* vox = (const uint8_t*)level_load_ptr(lvl, LEVEL_OFF_VOXELS);
    for (int cy = 0; cy < gh; ++cy) {
        for (int cx = 0; cx < gw; ++cx) {
            uint8_t r = 8, g = 16, b = 48; /* void colour */
            if (vox != NULL) {
                const uint8_t* v = vox + ((size_t)cy * gw + cx) * LEVEL_VOXEL_STRIDE;
                cell_colour(v, &r, &g, &b);
            }
            fill_rect(cx * cell_w, cy * cell_h,
                      cx * cell_w + cell_w - 1, cy * cell_h + cell_h - 1,
                      w, h, pixels, pitch, r, g, b);
        }
    }

    /* Entity markers at their live grid positions. */
    for (int t = 0; t < ENTITY_TYPES; ++t) {
        for (int i = 0; i < ctx->entity_count[t]; ++i) {
            const EntitySlot* e = &ctx->entity[t][i];
            int px = (int)(e->x / (float)gw * (float)w);
            int py = (int)(e->y / (float)gh * (float)h);
            uint8_t r, g, b;
            entity_colour(e->type, &r, &g, &b);
            fill_rect(px - 2, py - 2, px + 2, py + 2, w, h, pixels, pitch, r, g, b);
            if (e->flags & ENTITY_SELECTED) {
                /* yellow ring around the marker */
                fill_rect(px - 4, py - 4, px + 4, py - 4, w, h, pixels, pitch, 250, 220, 60);
                fill_rect(px - 4, py + 4, px + 4, py + 4, w, h, pixels, pitch, 250, 220, 60);
                fill_rect(px - 4, py - 4, px - 4, py + 4, w, h, pixels, pitch, 250, 220, 60);
                fill_rect(px + 4, py - 4, px + 4, py + 4, w, h, pixels, pitch, 250, 220, 60);
            }
        }
    }

    /* Reward overlay: draw the Save/Advance button zones in screen space so
     * the zone hit-test (finding 09) has visible targets. */
    if (ctx->reward_active) {
        const RewardScreen* rs = &ctx->reward_screen;
        /* Save (green) */
        fill_rect((int)rs->save_x, (int)rs->save_y,
                  (int)(rs->save_x + rs->save_w), (int)rs->save_y,
                  w, h, pixels, pitch, 90, 200, 90);
        fill_rect((int)rs->save_x, (int)(rs->save_y + rs->save_h),
                  (int)(rs->save_x + rs->save_w), (int)(rs->save_y + rs->save_h),
                  w, h, pixels, pitch, 90, 200, 90);
        fill_rect((int)rs->save_x, (int)rs->save_y,
                  (int)rs->save_x, (int)(rs->save_y + rs->save_h),
                  w, h, pixels, pitch, 90, 200, 90);
        fill_rect((int)(rs->save_x + rs->save_w), (int)rs->save_y,
                  (int)(rs->save_x + rs->save_w), (int)(rs->save_y + rs->save_h),
                  w, h, pixels, pitch, 90, 200, 90);
        /* Advance (blue) */
        fill_rect((int)rs->advance_x, (int)rs->advance_y,
                  (int)(rs->advance_x + rs->advance_w), (int)rs->advance_y,
                  w, h, pixels, pitch, 90, 150, 220);
        fill_rect((int)rs->advance_x, (int)(rs->advance_y + rs->advance_h),
                  (int)(rs->advance_x + rs->advance_w), (int)(rs->advance_y + rs->advance_h),
                  w, h, pixels, pitch, 90, 150, 220);
        fill_rect((int)rs->advance_x, (int)rs->advance_y,
                  (int)rs->advance_x, (int)(rs->advance_y + rs->advance_h),
                  w, h, pixels, pitch, 90, 150, 220);
        fill_rect((int)(rs->advance_x + rs->advance_w), (int)rs->advance_y,
                  (int)(rs->advance_x + rs->advance_w), (int)(rs->advance_y + rs->advance_h),
                  w, h, pixels, pitch, 90, 150, 220);
    }

    /* Cursor cell outline (the selection pick reads the same mapping). */
    if (mouse_x >= 0 && mouse_y >= 0) {
        int cx = mouse_x / cell_w;
        int cy = mouse_y / cell_h;
        if (cx < gw && cy < gh) {
            fill_rect(cx * cell_w, cy * cell_h,
                      cx * cell_w + cell_w - 1, cy * cell_h,
                      w, h, pixels, pitch, 240, 240, 240);
            fill_rect(cx * cell_w, cy * cell_h + cell_h - 1,
                      cx * cell_w + cell_w - 1, cy * cell_h + cell_h - 1,
                      w, h, pixels, pitch, 240, 240, 240);
            fill_rect(cx * cell_w, cy * cell_h,
                      cx * cell_w, cy * cell_h + cell_h - 1,
                      w, h, pixels, pitch, 240, 240, 240);
            fill_rect(cx * cell_w + cell_w - 1, cy * cell_h,
                      cx * cell_w + cell_w - 1, cy * cell_h + cell_h - 1,
                      w, h, pixels, pitch, 240, 240, 240);
        }
    }
}
