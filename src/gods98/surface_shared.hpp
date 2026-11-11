// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private to src/gods98/surface.cpp and the units relocated out of it (ADR 0006
 * decision 4; tools/relocate_unit.py). Statics both sides use have
 * external linkage through the declarations below. Include it only from
 * those files: it is not a shared header. */
#ifndef LRR_GODS98_SURFACE_SHARED_HPP
#define LRR_GODS98_SURFACE_SHARED_HPP

typedef struct FontAtlasGlyphRecord {
    float x;
    float y;
    float width;
    float height;
} FontAtlasGlyphRecord;

typedef struct FontAtlasRecord {
    TextureSurface* surface;
    struct FontAtlasRecord* free_next;
    FontAtlasGlyphRecord glyphs[10 * 19];
    uint32_t line_height;
    uint32_t tab_width;
} FontAtlasRecord;

#endif /* LRR_GODS98_SURFACE_SHARED_HPP */
