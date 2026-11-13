// SPDX-License-Identifier: LGPL-3.0-or-later
/* The port's bitmap-font text renderer: the port tail of Font.c (G98-008,
 * finding 605), which is rebuilt in original shape in font.c. These fonts
 * are software TextureSurfaces drawn into a render target, so the entries
 * here have the port's contract. Unannotated. Natively they serve every
 * caller. In the matching build font.c takes the names whose callers pass
 * the original's arguments (font_link_names.h), so this file compiles its
 * definitions of those out there (LRR_FONT_PORT_NAMES). */
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include "frontend.hpp"
#include "frontend.h"
#include "objective.hpp"
#include "level.hpp"
#include "game/entity.hpp"
#include "terrain_feedback.hpp"
#include "port_loop_ports.hpp"
#include "effects.hpp"
#include "savegame.hpp"
#include "gods98/audio.hpp"
#include "gods98/engine.hpp"
#include "gods98/simtick.hpp"
#include "gods98/message.hpp"
#include "gods98/flic.hpp"
#include "gods98/media.hpp"
#include "crt/crt.hpp"
#include "gods98/heap.hpp"
#include "gods98/record.hpp"
#include "gods98/texture.hpp"
#include "gods98/surface.hpp"
#include "gods98/sound.hpp"
#include "gods98/block.hpp"
#include "gods98/input.hpp"
#include "util.hpp"
#include "select.hpp"
#include "cmd.hpp"
#include "priority_config_ports.hpp"
#include "tutorial_hud_ports.hpp"
#include "ui_action_dispatch.hpp"
#include "simulation_ui_ports.hpp"
#include "camera_drag.hpp"
#include "screen_pick.hpp"
#include "pointer_cursor.hpp"
#include "ui_list.hpp"
#include "gods98/view.hpp"
#include "gods98/string.hpp"
#include "gods98/resource.hpp"
#include "frontend_for_bitmap_font.hpp"
#include <limits.h>
#include "record_for_bitmap_font.hpp"
#include "platform/platform.h"
#include "model.hpp"
#include "frameloop.hpp"
#include "scene_resources.hpp"
#include "game/frontend_for_bitmap_font.hpp"
#include "record_for_bitmap_font.hpp"
#include "surface_for_bitmap_font.hpp"

/* The port's definitions of the names font.c takes in the matching build
 * (font_link_names.h): every build but MSVC 5.0's. */
#if defined(_MSC_VER) && _MSC_VER < 1200
#define LRR_FONT_PORT_NAMES 0
#else
#define LRR_FONT_PORT_NAMES 1
#endif

#if LRR_FONT_PORT_NAMES /* font.c serves these names in the matching build */
static FontAtlasRecord* s_font_atlas_record_free_list;
#endif

/* Defined below (forward declarations added by tools/relocate_unit.py). */
static void fe_blit_glyph(void* px, int pitch, int w, int h, int dx, int dy, const TextureSurface* f, int cw, int ch, int g, const uint32_t keys[2]);

#if LRR_FONT_PORT_NAMES /* font.c serves this name in the matching build */
/* Port model of 0x00479ee0. */
void* font_atlas_record_create(const char* name)
{
    TextureSurface* surface;
    FontAtlasRecord* record;
    int cell_width;
    int cell_height;
    int row;
    int column;
    uint32_t key1;

    if (name == NULL || name[0] == '\0') return NULL;
    surface = image_surface_load(name, 0u, 0u);
    if (surface == NULL || surface->pixels == NULL ||
        surface->width < 10 || surface->height < 19) {
        texture_release(surface);
        return NULL;
    }
    cell_width = surface->width / 10;
    cell_height = surface->height / 19;
    if (cell_width <= 0 || cell_height <= 0) {
        texture_release(surface);
        return NULL;
    }
    key1 = surface->key_count >= 2 ? surface->keys[1] : surface->pixels[0];

    record = s_font_atlas_record_free_list;
    if (record != NULL) {
        s_font_atlas_record_free_list = record->free_next;
        memset(record, 0, sizeof(*record));
    }
    else {
        record = (FontAtlasRecord*)calloc(1, sizeof(*record));
    }
    if (record == NULL) {
        texture_release(surface);
        return NULL;
    }
    record->surface = surface;
    record->line_height = (uint32_t)cell_height;
    record->tab_width = (uint32_t)(cell_width * 8);
    for (column = 0; column < 10; ++column) {
        for (row = 0; row < 19; ++row) {
            FontAtlasGlyphRecord* glyph =
                &record->glyphs[column * 19 + row];
            int x0 = column * cell_width;
            int y0 = row * cell_height;
            int width = cell_width;
            int x;
            glyph->x = (float)x0;
            glyph->y = (float)y0;
            glyph->height = (float)cell_height;
            /* FontResourceLoad scans the first row against the second
             * palette key, retaining at least one pixel of advance. The
             * first key may encode spacing without visible glyph ink. */
            for (x = cell_width - 1; x > 0; --x) {
                uint32_t pixel = surface->pixels[(size_t)y0 *
                    (size_t)surface->width + (size_t)(x0 + x)];
                if (pixel != key1) break;
                --width;
            }
            glyph->width = (float)width;
        }
    }
    return record;
}
#endif /* LRR_FONT_PORT_NAMES */

/* FormattedTextFormatDraw (FUN_0047a150): expand the engine's `%b` inline
 * texture token, format into bounded storage, then delegate drawing and
 * measurement to the decoded-text path. `%b` values are 32-bit handles
 * resolved by the software renderer hook. */
/* Port model of 0x0047a150. */
void formatted_text_format_draw(const TextureSurface* font,
                                uint32_t* width_out, int* line_count,
                                void* px, int pitch, int w, int h,
                                int x, int y, const char* format,
                                va_list args)
{
    if (width_out != NULL) *width_out = 0;
    if (line_count != NULL) *line_count = 0;
    if (font == NULL || format == NULL) return;

    char expanded[1024];
    size_t used = 0;
    for (size_t i = 0; format[i] != '\0' && used + 1 < sizeof(expanded); ++i) {
        if (format[i] == '%' && format[i + 1] == 'b') {
            static const char texture_format[] = "@[0x%0.8x]";
            const size_t n = sizeof(texture_format) - 1;
            if (used + n >= sizeof(expanded)) break;
            memcpy(expanded + used, texture_format, n);
            used += n;
            ++i;
        } else {
            expanded[used++] = format[i];
        }
    }
    expanded[used] = '\0';

    char text[1024];
    formatted_text_buffer_build(text, sizeof(text), expanded, args);
    const uint32_t width = formatted_text_draw_and_measure(
        font, px, pitch, w, h, x, y, text, line_count);
    if (width_out != NULL) *width_out = width;
}

/* RewardTextDraw (FUN_0047a180): positioned variadic convenience wrapper for
 * formatted UI text. The legacy routine forwards its format and argument
 * list to FormattedTextFormatDraw; the port supplies its software target. */
/* Port model of 0x0047a180. */
void reward_text_draw(const TextureSurface* font, void* px, int pitch,
                      int w, int h, int x, int y, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    formatted_text_format_draw(font, NULL, NULL, px, pitch, w, h,
                               x, y, format, args);
    va_end(args);
}
/* FormattedTextDraw (FUN_0047a1b0): software-framebuffer draw of an already
 * resolved text value using the selected bitmap face. Format expansion is
 * performed at the caller boundary in this port. */
/* Port model of 0x0047a1b0. */
void formatted_text_draw(const TextureSurface* font, void* px, int pitch,
                         int w, int h, int x, int y, const char* text)
{
    if (font == NULL || px == NULL || text == NULL || w <= 0 || h <= 0) {
        return;
    }
    formatted_text_draw_and_measure(font, px, pitch, w, h, x, y, text, NULL);
}

/* FormattedTextDrawAndMeasure (FUN_0047a1e0): draw decoded bitmap-font text,
 * returning its widest line and optionally the number of lines. Newlines
 * reset the pen and advance one glyph cell; tabs advance to the next eight-
 * cell stop. Inline @[0x........] handles are resolved through the port-side
 * hook, then blitted through the software surface path. */
/* Port model of 0x0047a1e0. */
uint32_t formatted_text_draw_and_measure(const TextureSurface* font,
                                         void* px, int pitch, int w, int h,
                                         int x, int y, const char* text,
                                         int* line_count)
{
    if (line_count != NULL) {
        *line_count = 0;
    }
    if (font == NULL || text == NULL) {
        return 0;
    }

    int cell_w = 0;
    int cell_h = 0;
    if (!fe_font_cell(font, &cell_w, &cell_h)) {
        return 0;
    }
    uint32_t keys[2];
    fe_font_keys(font, cell_w, cell_h, keys);

    int pen_x = 0;
    int max_width = 0;
    int line_h = cell_h;
    int lines = 1;
    for (size_t i = 0; text[i] != '\0'; ++i) {
        if (text[i] == '\n') {
            if (pen_x > max_width) max_width = pen_x;
            pen_x = 0;
            y += line_h;
            line_h = cell_h;
            ++lines;
            continue;
        }
        if (text[i] == '\t') {
            const int tab_width = cell_w * 8;
            if (tab_width > 0) {
                pen_x += tab_width - (pen_x % tab_width);
            }
            continue;
        }
        if (text[i] == '@' && text[i + 1] == '[' && text[i + 2] == '0'
            && text[i + 3] == 'x' && strlen(text + i) >= 13
            && text[i + 12] == ']') {
            char* end;
            unsigned long parsed = strtoul(text + i + 4, &end, 16);
            int valid = end == text + i + 12 && *end == ']';
            uint32_t handle = (uint32_t)parsed;
            const TextureSurface* image = valid && handle != 0
                && s_formatted_text_texture_resolve != NULL
                ? s_formatted_text_texture_resolve(handle) : NULL;
            if (image != NULL) {
                if (px != NULL && w > 0 && h > 0) {
                    surface_blit_texture(px, pitch, w, h, image,
                                         x + pen_x, y);
                }
                pen_x += image->width;
                if (image->height > line_h) line_h = image->height;
            }
            i += 12;
            continue;
        }

        const int c = (unsigned char)text[i];
        const int glyph = c - 0x20;
        int advance;
        if (glyph < 0 || glyph >= FE_FONT_GRID_COLS * FE_FONT_GRID_ROWS) {
            advance = 0;
        } else {
            advance = fe_font_glyph_draw_and_measure(
                px, pitch, w, h, x + pen_x, y, font, cell_w, cell_h, glyph,
                keys, px != NULL && w > 0 && h > 0);
        }
        pen_x += advance;
    }
    if (pen_x > max_width) max_width = pen_x;
    if (line_count != NULL) {
        *line_count = lines;
    }
    return (uint32_t)max_width;
}
/* Partial BitmapFontGlyphDrawAndMeasure (FUN_0047a470): combine the port's
 * software glyph blit with the glyph advance calculation. The corpus uses
 * its DirectDraw surface and stored float advances; this path clips RGBA
 * pixels and derives the advance from the glyph's ink bounds. */
/* Port model of 0x0047a470. */
int fe_font_glyph_draw_and_measure(void* px, int pitch, int w, int h,
                                          int x, int y,
                                          const TextureSurface* f,
                                          int cw, int ch, int g,
                                          const uint32_t keys[2], int draw)
{
    int advance = fe_font_advance(f, cw, ch, g, keys);
    if (draw != 0) {
        fe_blit_glyph(px, pitch, w, h, x, y, f, cw, ch, g, keys);
    }
    return advance;
}

#if LRR_FONT_PORT_NAMES /* font.c serves this name in the matching build */
/* BitmapFontGlyphMeasureWidth (FUN_0047a520) — see frontend.hpp. */
/* Port model of 0x0047a520. */
int bitmap_font_glyph_measure_width(void* font, unsigned char glyph)
{
    const TextureSurface* surface = (const TextureSurface*)font;
    const int glyph_index = (int)glyph - 0x20;
    int cell_width = 0;
    int cell_height = 0;
    uint32_t keys[2];
    if (glyph_index < 0 ||
        glyph_index >= FE_FONT_GRID_COLS * FE_FONT_GRID_ROWS ||
        !fe_font_cell(surface, &cell_width, &cell_height)) {
        return 0;
    }
    fe_font_keys(surface, cell_width, cell_height, keys);
    return fe_font_advance(surface, cell_width, cell_height,
                           glyph_index, keys);
}
#endif /* LRR_FONT_PORT_NAMES */

#if LRR_FONT_PORT_NAMES /* font.c serves this name in the matching build */
/* Port model of 0x0047a540 (Font_GetHeight). */
uint32_t object_field_be4_dword_get(const void* record)
{
    uint32_t value;
    if (record == NULL) {
        return 0u;
    }
    memcpy(&value, (const char*)record + 0xbe4, sizeof(value));
    return value;
}
#endif /* LRR_FONT_PORT_NAMES */

#if LRR_FONT_PORT_NAMES /* font.c serves this name in the matching build */
/* Port model of 0x0047a550. */
void font_atlas_record_release(void* opaque_record)
{
    FontAtlasRecord* record = (FontAtlasRecord*)opaque_record;
    if (record == NULL) return;
    texture_release(record->surface);
    record->surface = NULL;
    record->free_next = s_font_atlas_record_free_list;
    s_font_atlas_record_free_list = record;
}
#endif /* LRR_FONT_PORT_NAMES */

static int fe_font_key_hit(uint32_t p, const uint32_t keys[2])
{
    return p == keys[0] || p == keys[1];
}

/* Ink bounding box of glyph g — the non-key pixels only. The engine blits
 * exactly this box (FUN_00479ee0's right-edge scan / per-glyph rect), which
 * keeps each face's background AND frame strips out while retaining the
 * glyph's dark outline when it is inside the box. */
static int fe_font_ink_box(const TextureSurface* f, int cw, int ch, int g,
                           const uint32_t keys[2], int* minx, int* miny,
                           int* maxx, int* maxy)
{
    int col = g % FE_FONT_GRID_COLS, row = g / FE_FONT_GRID_COLS;
    int x0 = col * cw, y0 = row * ch;
    *minx = cw; *miny = ch; *maxx = -1; *maxy = -1;
    for (int yy = 0; yy < ch; yy++) {
        for (int xx = 0; xx < cw; xx++) {
            if (fe_font_key_hit(
                    f->pixels[(size_t)(y0 + yy) * (size_t)f->width + (x0 + xx)],
                    keys)) {
                continue;
            }
            if (xx < *minx) { *minx = xx; }
            if (xx > *maxx) { *maxx = xx; }
            if (yy < *miny) { *miny = yy; }
            if (yy > *maxy) { *maxy = yy; }
        }
    }
    return *maxx >= 0;
}

static void fe_blit_glyph(void* px, int pitch, int w, int h,
                          int dx, int dy, const TextureSurface* f,
                          int cw, int ch, int g, const uint32_t keys[2])
{
    int col = g % FE_FONT_GRID_COLS, row = g / FE_FONT_GRID_COLS;
    int x0 = col * cw, y0 = row * ch;
    int minx, miny, maxx, maxy;
    if (!fe_font_ink_box(f, cw, ch, g, keys, &minx, &miny, &maxx, &maxy)) {
        return;
    }
    for (int yy = miny; yy <= maxy; yy++) {
        int dy2 = dy + yy;
        if (dy2 < 0 || dy2 >= h) {
            continue;
        }
        for (int xx = minx; xx <= maxx; xx++) {
            uint32_t p = f->pixels[(size_t)(y0 + yy) * (size_t)f->width
                                   + (x0 + xx)];
            if (fe_font_key_hit(p, keys)) {
                continue;               /* key *inside* the box */
            }
            int dx2 = dx + xx;
            if (dx2 < 0 || dx2 >= w) {
                continue;
            }
            uint8_t* o = (uint8_t*)px + (size_t)dy2 * (size_t)pitch
                         + (size_t)dx2 * 4;
            o[0] = (uint8_t)(p >> 16);
            o[1] = (uint8_t)(p >> 8);
            o[2] = (uint8_t)(p);
            o[3] = (uint8_t)(p >> 24);
        }
    }
}

/* --- Font.c entry points in the original ABI ------------------------------
 * Thin unannotated adapters (ADR 0005 step 6) over the port's renderer, for
 * the converted C units: Panel.c (font_v_get_string_info, font_v_print_f;
 * finding 554), Reward.c and the other C units (font_print_f; finding 560)
 * and TextWindow.c (font_output_char; finding 600). The original draws to
 * the back buffer, which the port's renderer owns, so these measure only.
 * In the matching build font.c defines these four names. */
#include "image_adapters.h"
#include "font_api.h"

#if LRR_FONT_PORT_NAMES /* font.c serves these names in the matching build */
// PORT-ONLY: adapter or forwarder for the port contract
void font_v_get_string_info(const TextureSurface* font, uint32_t* width,
                            uint32_t* lineCount, const char* msg,
                            va_list args)
{
    formatted_text_format_draw(font, width, (int*)lineCount, NULL, 0, 0, 0,
                               0, 0, msg, args);
}

// PORT-ONLY: adapter or forwarder for the port contract
void font_v_print_f(const TextureSurface* font, int x, int y,
                    const char* msg, va_list args)
{
    formatted_text_format_draw(font, NULL, NULL, NULL, 0, 0, 0, x, y, msg,
                               args);
}

// PORT-ONLY: adapter or forwarder for the port contract
void font_print_f(const TextureSurface* font, int x, int y, const char* msg, ...)
{
    va_list args;
    va_start(args, msg);
    font_v_print_f(font, x, y, msg, args);
    va_end(args);
}

// PORT-ONLY: adapter or forwarder for the port contract
extern "C" int font_output_char(void* font, int x, int y, unsigned char c,
                                int render)
{
    (void)x;
    (void)y;
    (void)render;
    return bitmap_font_glyph_measure_width(font, c);
}
#endif /* LRR_FONT_PORT_NAMES */
