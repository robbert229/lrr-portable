// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private to src/gods98/heap.cpp and the units relocated out of it (ADR 0006
 * decision 4; tools/relocate_unit.py). Statics both sides use have
 * external linkage through the declarations below. Include it only from
 * those files: it is not a shared header. */
#ifndef LRR_GODS98_HEAP_SHARED_HPP
#define LRR_GODS98_HEAP_SHARED_HPP

typedef struct TextBufferFontBinding {
    struct TextBufferFontBinding* next;
    void* record;
    void* font;
} TextBufferFontBinding;

extern TextBufferFontBinding* s_text_buffer_font_bindings;
extern TextBufferDrawOps s_text_buffer_draw_ops;

#endif /* LRR_GODS98_HEAP_SHARED_HPP */
