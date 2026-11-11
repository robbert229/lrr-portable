// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; #496): what bitmap_font.cpp sees from
 * surface.cpp. A verbatim snapshot of surface_shared.hpp as it was when the
 * #496 migration froze it, so the unit's declarations no longer change
 * with later relocations. Never edited by tools/relocate_unit.py. */
#ifndef LRR_GODS98_SURFACE_FOR_BITMAP_FONT_HPP
#define LRR_GODS98_SURFACE_FOR_BITMAP_FONT_HPP

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

#endif /* LRR_GODS98_SURFACE_FOR_BITMAP_FONT_HPP */
