// SPDX-License-Identifier: LGPL-3.0-or-later
#include <string.h>

#include "entity.hpp"
#include "level.hpp"
#include "render.hpp"
#include "../gods98/model.hpp"
#include "../gods98/texture.hpp"

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

/* Bresenham line, clamped to the framebuffer. */
static void draw_line(int x0, int y0, int x1, int y1,
                      int w, int h, void* pixels, int pitch,
                      uint8_t r, uint8_t g, uint8_t b)
{
    int dx = x1 - x0, dy = y1 - y0;
    int sx = dx < 0 ? -1 : 1;
    int sy = dy < 0 ? -1 : 1;
    dx = dx < 0 ? -dx : dx;
    dy = dy < 0 ? -dy : dy;
    int err = dx - dy;
    for (;;) {
        px_set(px_at(x0, y0, w, h, pixels, pitch), r, g, b);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx)  { err += dx; y0 += sy; }
    }
}

/* LWO wireframe, top-down orthographic projection (model x -> screen x,
 * model z -> screen y), normalized to fit roughly a grid cell. */
static void draw_model_wire(const Model* m, int px, int py,
                            int w, int h, void* pixels, int pitch,
                            uint8_t r, uint8_t g, uint8_t b)
{
    if (m == NULL || m->n_verts == 0 || m->verts == NULL) {
        return;
    }
    float minx = m->verts[0], maxx = m->verts[0];
    float minz = m->verts[2], maxz = m->verts[2];
    for (size_t i = 1; i < m->n_verts; ++i) {
        float vx = m->verts[i * 3 + 0];
        float vz = m->verts[i * 3 + 2];
        if (vx < minx) minx = vx;
        if (vx > maxx) maxx = vx;
        if (vz < minz) minz = vz;
        if (vz > maxz) maxz = vz;
    }
    float cx = (minx + maxx) * 0.5f;
    float cz = (minz + maxz) * 0.5f;
    float extent = (maxx - minx) > (maxz - minz) ? (maxx - minx) : (maxz - minz);
    float scale = extent > 1e-4f ? 12.0f / extent : 1.0f;

    for (size_t p = 0; p < m->n_polys; ++p) {
        const ModelPoly* poly = &m->polys[p];
        if (poly->count < 2 || poly->ring == NULL) {
            continue;
        }
        for (uint16_t e = 0; e < poly->count; ++e) {
            unsigned a = poly->ring[e];
            unsigned b2 = poly->ring[(e + 1) % poly->count];
            if (a >= m->n_verts || b2 >= m->n_verts) {
                continue;
            }
            int sx0 = px + (int)((m->verts[a * 3 + 0] - cx) * scale);
            int sy0 = py + (int)((m->verts[a * 3 + 2] - cz) * scale);
            int sx1 = px + (int)((m->verts[b2 * 3 + 0] - cx) * scale);
            int sy1 = py + (int)((m->verts[b2 * 3 + 2] - cz) * scale);
            draw_line(sx0, sy0, sx1, sy1, w, h, pixels, pitch, r, g, b);
        }
    }
}

void render_apply_brightness(void* pixels, int pitch, int w, int h,
                             int value)
{
    if (pixels == NULL || w <= 0 || h <= 0) {
        return;
    }
    int v = value < 0 ? 0 : (value > 10 ? 10 : value);
    int k = v * 0.1f * 256.0f + 0.5f; /* 0..256, 256 = 1.0 (value 10) */
    if (k >= 256) {
        return;
    }
    uint8_t* px = (uint8_t*)pixels;
    for (int y = 0; y < h; ++y) {
        uint8_t* row = px + (size_t)y * (size_t)pitch;
        for (int x = 0; x < w; ++x) {
            row[x * 4 + 0] = (uint8_t)((row[x * 4 + 0] * k) >> 8);
            row[x * 4 + 1] = (uint8_t)((row[x * 4 + 1] * k) >> 8);
            row[x * 4 + 2] = (uint8_t)((row[x * 4 + 2] * k) >> 8);
        }
    }
}

/* --- reward-overlay helpers (finding 09) --------------------------------
 * The post-mission card (RewardScreen) renders on top of the level: the
 * frame is dimmed first, the wallpaper texture (get-or-load by config name)
 * is stretched over it, a state-coloured header band stands for the banner
 * text (no font renderer — blocks only), and the Save/Advance hit zones get
 * a hover highlight. Everything here is plain RGBA writes; alpha is always
 * opaque 255. */

/* One 0xAARRGGBB texture pixel -> framebuffer R,G,B (full alpha). */
static void px_set_argb(uint32_t argb, uint8_t* px)
{
    px[0] = (uint8_t)(argb >> 16); /* R */
    px[1] = (uint8_t)(argb >> 8);  /* G */
    px[2] = (uint8_t)(argb);       /* B */
    px[3] = 255;
}

/* Scale every RGB channel toward black so the level reads as "behind" the
 * overlay (deliverable 1). */
static void dim_framebuffer(int w, int h, void* pixels, int pitch)
{
    for (int y = 0; y < h; ++y) {
        uint8_t* row = (uint8_t*)pixels + (size_t)y * (size_t)pitch;
        for (int x = 0; x < w; ++x) {
            uint8_t* p = row + (size_t)x * 4;
            p[0] = (uint8_t)(((unsigned)p[0] * 3u) / 8u);
            p[1] = (uint8_t)(((unsigned)p[1] * 3u) / 8u);
            p[2] = (uint8_t)(((unsigned)p[2] * 3u) / 8u);
        }
    }
}

/* Full-bleed nearest-neighbour blit of the wallpaper (deliverable 2). NULL
 * (or an empty pixel buffer) is the "no texture" case — the caller keeps
 * the dimmed frame. */
static void blit_wallpaper(TextureSurface* t, int w, int h,
                           void* pixels, int pitch)
{
    if (t == NULL || t->pixels == NULL || t->width <= 0 || t->height <= 0) {
        return;
    }
    uint32_t tw = (uint32_t)t->width;
    uint32_t th = (uint32_t)t->height;
    for (int y = 0; y < h; ++y) {
        uint32_t ty = ((uint32_t)y * th) / (uint32_t)h;
        if (ty >= th) ty = th - 1;
        const uint32_t* src = t->pixels + ty * tw;
        uint8_t* row = (uint8_t*)pixels + (size_t)y * (size_t)pitch;
        for (int x = 0; x < w; ++x) {
            uint32_t tx = ((uint32_t)x * tw) / (uint32_t)w;
            if (tx >= tw) tx = tw - 1;
            px_set_argb(src[tx], row + (size_t)x * 4);
        }
    }
}

/* 1-pixel outline (inclusive corners, matching the old Save/Advance draw). */
static void outline_rect(int x0, int y0, int x1, int y1,
                         int w, int h, void* pixels, int pitch,
                         uint8_t r, uint8_t g, uint8_t b)
{
    fill_rect(x0, y0, x1, y0, w, h, pixels, pitch, r, g, b);
    fill_rect(x0, y1, x1, y1, w, h, pixels, pitch, r, g, b);
    fill_rect(x0, y0, x0, y1, w, h, pixels, pitch, r, g, b);
    fill_rect(x1, y0, x1, y1, w, h, pixels, pitch, r, g, b);
}

/* Only draw rect (x0..x1, y0..y1) when it can actually touch the frame. */
static int rect_visible(int x0, int y0, int x1, int y1, int w, int h)
{
    return x1 >= 0 && y1 >= 0 && x0 < w && y0 < h && x1 >= x0 && y1 >= y0;
}

/* State banner (deliverable 3): a header band whose colour encodes the
 * reward state. Achieved (objective result 1) -> complete green; otherwise
 * the presence of FailedText / QuitText picks the tint; no config -> a
 * neutral blue header. CentreText + Display gate a light content strip
 * (the "text line", drawn as a block). */
static void draw_reward_banner(const RewardScreen* rs, const LevelState* lvl,
                               int w, int h, void* pixels, int pitch)
{
    int bh = h / 8;
    if (bh < 4) bh = 4;
    if (bh > 64) bh = 64;
    if (bh > h) bh = h;
    if (bh <= 0) {
        return;
    }

    int result = (lvl != NULL) ? level_int(lvl, LEVEL_OFF_OBJ_RESULT) : 0;
    uint8_t br = 28, bg = 52, bb = 96;      /* default header band */
    if (result == 1) {
        br = 70; bg = 190; bb = 90;          /* achieved -> complete (green) */
    } else if (rs != NULL && rs->failed_text[0] != '\0') {
        br = 200; bg = 70; bb = 60;          /* failed (red) */
    } else if (rs != NULL && rs->quit_text[0] != '\0') {
        br = 100; bg = 110; bb = 130;        /* quit (grey-blue) */
    }

    fill_rect(0, 0, w - 1, bh - 1, w, h, pixels, pitch, br, bg, bb);

    /* Centre-text lane enabled -> light strip at the band's bottom edge. */
    if (rs != NULL && rs->centre_text != 0 && rs->display != 0) {
        uint8_t lr = (uint8_t)(br + ((255 - br) >> 1));
        uint8_t lg = (uint8_t)(bg + ((255 - bg) >> 1));
        uint8_t lb = (uint8_t)(bb + ((255 - bb) >> 1));
        fill_rect(0, bh - 1, w - 1, bh - 1, w, h, pixels, pitch, lr, lg, lb);
    }
}

/* Save/Advance buttons (deliverable 4): the zone under the mouse is filled
 * bright, the other keeps a darker outline (hit-test mirrors the zone_hit
 * math of the reward click handler). */
static void draw_reward_buttons(const RewardScreen* rs, int mx, int my,
                                int w, int h, void* pixels, int pitch)
{
    if (rs == NULL) {
        return;
    }
    int s_x = (int)rs->save_x,   s_y = (int)rs->save_y;
    int a_x = (int)rs->advance_x, a_y = (int)rs->advance_y;
    int s_w = (int)rs->save_w,   s_h = (int)rs->save_h;
    int a_w = (int)rs->advance_w, a_h = (int)rs->advance_h;

    /* Inline zone_hit: mx in [x, x+w), my in [y, y+h). */
    int save_hit = s_w > 0 && s_h > 0 && mx >= s_x && mx < s_x + s_w
                 && my >= s_y && my < s_y + s_h;
    int adv_hit  = a_w > 0 && a_h > 0 && mx >= a_x && mx < a_x + a_w
                 && my >= a_y && my < a_y + a_h;

    if (save_hit) {
        fill_rect(s_x, s_y, s_x + s_w - 1, s_y + s_h - 1,
                  w, h, pixels, pitch, 150, 255, 150);
    } else if (rect_visible(s_x, s_y, s_x + s_w, s_y + s_h, w, h)) {
        outline_rect(s_x, s_y, s_x + s_w, s_y + s_h,
                     w, h, pixels, pitch, 45, 110, 45);
    }
    if (adv_hit) {
        fill_rect(a_x, a_y, a_x + a_w - 1, a_y + a_h - 1,
                  w, h, pixels, pitch, 150, 200, 255);
    } else if (rect_visible(a_x, a_y, a_x + a_w, a_y + a_h, w, h)) {
        outline_rect(a_x, a_y, a_x + a_w, a_y + a_h,
                     w, h, pixels, pitch, 45, 80, 120);
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
    /* Screen-space viewport shake (finding 120): the callback shake branch
     * (0x43ce88) leaves ctx->shake_dx/dy in world units (the corpus scales
     * the work-object offset by amp*0.1 minus the 10.0 position scale —
     * 0x49e3d4). Convert to pixels with that same fixed 10 px/unit draw
     * scale so the amplitude/decay feels like the shipped game (amp 1.0 →
     * 1.0 px, halving as the amplitude decays). A zero offset (no shaking
     * entity) leaves the render byte-identical. */
    int shx = (ctx->shake_frame != 0) ? (int)(ctx->shake_dx * 10.0f) : 0;
    int shy = (ctx->shake_frame != 0) ? (int)(ctx->shake_dy * 10.0f) : 0;
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
            fill_rect(cx * cell_w + shx, cy * cell_h + shy,
                      cx * cell_w + cell_w - 1 + shx,
                      cy * cell_h + cell_h - 1 + shy,
                      w, h, pixels, pitch, r, g, b);
        }
    }

    /* Entity markers at their live grid positions. */
    for (int t = 0; t < ENTITY_TYPES; ++t) {
        for (int i = 0; i < ctx->entity_count[t]; ++i) {
            const EntitySlot* e = &ctx->entity[t][i];
            int px = (int)(e->x / (float)gw * (float)w) + shx;
            int py = (int)(e->y / (float)gh * (float)h) + shy;
            uint8_t r, g, b;
            entity_colour(e->type, &r, &g, &b);
            if (entity_model(e) != NULL) {
                /* Model-aware: draw the LWO wireframe (top-down) instead of
                 * the placeholder dot; keep a small centre marker. */
                draw_model_wire((const Model*)entity_model(e), px, py,
                                w, h, pixels, pitch, r, g, b);
                fill_rect(px - 1, py - 1, px + 1, py + 1,
                          w, h, pixels, pitch, r, g, b);
            } else {
                fill_rect(px - 2, py - 2, px + 2, py + 2,
                          w, h, pixels, pitch, r, g, b);
            }
            if (e->flags & ENTITY_SELECTED) {
                /* yellow ring around the marker */
                fill_rect(px - 4, py - 4, px + 4, py - 4, w, h, pixels, pitch, 250, 220, 60);
                fill_rect(px - 4, py + 4, px + 4, py + 4, w, h, pixels, pitch, 250, 220, 60);
                fill_rect(px - 4, py - 4, px - 4, py + 4, w, h, pixels, pitch, 250, 220, 60);
                fill_rect(px + 4, py - 4, px + 4, py + 4, w, h, pixels, pitch, 250, 220, 60);
            }
        }
    }

    /* Reward overlay (finding 09): dim the level, draw the wallpaper
     * backdrop, the state banner and the Save/Advance buttons, then stop —
     * the gameplay cursor outline has no place on the menu. */
    if (ctx->reward_active) {
        const RewardScreen* rs = &ctx->reward_screen;
        dim_framebuffer(w, h, pixels, pitch);

        if (rs->wallpaper[0] != '\0') {
            TextureSurface* t = texture_get_or_load(NULL, rs->wallpaper);
            blit_wallpaper(t, w, h, pixels, pitch);
            if (t != NULL) {
                texture_release(t); /* keep only the engine's cached ref */
            }
        }

        draw_reward_banner(rs, ctx->level, w, h, pixels, pitch);
        draw_reward_buttons(rs, mouse_x, mouse_y, w, h, pixels, pitch);
        return;
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
