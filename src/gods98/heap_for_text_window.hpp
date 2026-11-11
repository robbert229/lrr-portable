// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; #496): what text_window.cpp sees from
 * heap.cpp. A verbatim snapshot of heap_shared.hpp as it was when the
 * #496 migration froze it, so the unit's declarations no longer change
 * with later relocations. Never edited by tools/relocate_unit.py. */
#ifndef LRR_GODS98_HEAP_FOR_TEXT_WINDOW_HPP
#define LRR_GODS98_HEAP_FOR_TEXT_WINDOW_HPP

typedef struct TextBufferFontBinding {
    struct TextBufferFontBinding* next;
    void* record;
    void* font;
} TextBufferFontBinding;

extern TextBufferFontBinding* s_text_buffer_font_bindings;
extern TextBufferDrawOps s_text_buffer_draw_ops;

#endif /* LRR_GODS98_HEAP_FOR_TEXT_WINDOW_HPP */
