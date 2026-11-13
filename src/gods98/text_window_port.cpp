// SPDX-License-Identifier: LGPL-3.0-or-later
/* Port tail (ADR 0006 decision 4) of G98-017 TextWindow.c, which is rebuilt
 * in original shape in textwindow.c (finding 600). These are the port's text
 * windows: the same 0x830-byte record, with a host-width font sidecar, NULL
 * guards and drawing through the TextBufferDrawOps hooks (heap.hpp).
 * Unannotated. Natively they serve every caller. In the matching build
 * textwindow.c takes these names (textwindow_link_names.h), so this file
 * compiles them out there (LRR_TEXT_WINDOW_PORT_NAMES). */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "heap.hpp"
#include "string.hpp"
#include "surface.hpp"
#include "crt/crt.hpp"
#include "record.hpp"
#include "sound.hpp"
#include "view.hpp"
#include "texture.hpp"
#include "resource.hpp"
#include "game/util.hpp"
#include <math.h>
#include <limits.h>
#include "record_for_text_window.hpp"
#include "heap_for_text_window.hpp"
#include "record_for_text_window.hpp"

/* The port's definitions of the names the unit takes in the matching build
 * (textwindow_link_names.h): every build but MSVC 5.0's. */
#if defined(_MSC_VER) && _MSC_VER < 1200
#define LRR_TEXT_WINDOW_PORT_NAMES 0
#else
#define LRR_TEXT_WINDOW_PORT_NAMES 1
#endif

#if LRR_TEXT_WINDOW_PORT_NAMES /* textwindow.c serves these names in the matching build */
/* Defined below (forward declarations added by tools/relocate_unit.py). */
static int text_buffer_font_binding_add(void* record, uintptr_t font_handle);
static int text_buffer_draw_glyph(void* font, int x, int y, unsigned char glyph);
static void* text_buffer_font_get(const void* record);
static int text_buffer_glyph_width(void* font, unsigned char glyph);
static int text_buffer_is_break(unsigned char glyph);
static int text_buffer_line_height(void* font);
static uint32_t text_buffer_read_u32(const void* record, size_t offset);

/* TextBufferCreate (FUN_004850f0) — see heap.hpp. */
/* Port model of 0x004850f0. */
void* text_buffer_create(uintptr_t font_handle, const uint32_t* header,
                         long size)
{
    char* record = (char*)engine_heap_alloc(0x830);
    if (record == NULL) {
        return NULL; /* corpus: je — the allocation guard exists in the original */
    }
    memset(record, 0, 0x830); /* the corpus's 0x20c-dword sweep */
    {
        const uint32_t font_token = (uint32_t)font_handle;
        __builtin_memcpy(record + 0x00, &font_token, sizeof font_token);
    }
    if (!text_buffer_font_binding_add(record, font_handle)) {
        engine_heap_free(record);
        return NULL;
    }
    if (header != NULL) {
        for (int i = 0; i != 4; ++i) {
            __builtin_memcpy(record + 0x04 + i * 4, &header[i], 4);
        }
    }
    /* Deviation: the header pointer is guarded (the corpus strides it).
     * The lea-based destination base and early header load of the corpus
     * are unreachable with the guard (the mode store precedes the check);
     * the unroll/polarity variants scored lower — tested (b34: plain
     * typed stores also scored lower, reverted). */
    void* buffer = engine_heap_alloc(size);
    __builtin_memcpy(record + 0x14, &buffer, sizeof buffer);
    const uint32_t len = (uint32_t)size;
    __builtin_memcpy(record + 0x418, &len, sizeof len);
    text_output_field82c_bit2_set_clear(record, 1);
    return record;
}
/* TextOutputField82CBit2SetClear (FUN_00485160) — see heap.hpp. */
/* Port model of 0x00485160. */
void text_output_field82c_bit2_set_clear(void* record, int set_bit)
{
    if (record == NULL) {
        return; /* deviation: the corpus strides the record blindly */
    }
    uint32_t* flags = (uint32_t*)((char*)record + 0x82c);
    if (set_bit != 0) {
        *flags |= 4u;
    }
    else {
        *flags &= ~4u;
    }
}
/* The mapped TextBufferDestroy body follows the retail unchecked record
 * access. Keep the portable font-binding sidecar out of this target body. */
/* Port model of 0x00485190. */
void text_buffer_destroy_target(void* record)
{
    void* buffer = *(void**)((char*)record + 0x14);
    engine_heap_free(buffer);
    engine_heap_free(record);
}

/* Port model of 0x004851b0. */
void float_pair_at_offset4_set_from_ints(void* record, int x, int y)
{
    if (record == NULL) {
        return; /* deviation: the corpus writes blindly */
    }
    const float fx = (float)x; /* the corpus's int->float casts */
    const float fy = (float)y;
    __builtin_memcpy((char*)record + 0x04, &fx, sizeof(fx));
    __builtin_memcpy((char*)record + 0x08, &fy, sizeof(fy));
}

/* Port model of 0x004851d0. */
void float_pair_at_offsetc_set_from_uints(void* record, uint32_t x,
                                          uint32_t y)
{
    if (record == NULL) {
        return; /* deviation: the corpus writes blindly */
    }
    const float fx = (float)x; /* the corpus's uint->float casts */
    const float fy = (float)y;
    __builtin_memcpy((char*)record + 0x0c, &fx, sizeof(fx));
    __builtin_memcpy((char*)record + 0x10, &fy, sizeof(fy));
}

/* FormattedTextAppendToRingBuffer (FUN_00485210): variadic face of the
 * text-buffer append path, forwarding its argument list to the VFormat body. */
/* Port model of 0x00485210. */
void formatted_text_append_to_ring_buffer(void* record, int skip_bells,
                                          const char* format, ...)
{
    va_list args;
    va_start(args, format);
    text_buffer_append_vformat(record, skip_bells, format, args);
    va_end(args);
}
/* TextBufferAppendFormat (FUN_00485230): variadic wrapper with bell-skip
 * count fixed at zero, matching the decompiled call to TextBufferAppendVFormat. */
/* Port model of 0x00485230. */
void text_buffer_append_format(void* record, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    text_buffer_append_vformat(record, 0, format, args);
    va_end(args);
}
/* TextBufferAppendVFormat (FUN_00485250): append formatted bytes to the
 * text-output circular buffer, optionally starting after `skip_bells` BEL
 * markers. The record layout follows TextBufferCreate/Reset. */
/* Port model of 0x00485250. */
void text_buffer_append_vformat(void* record, int skip_bells,
                                const char* format, va_list args)
{
    if (record == NULL || format == NULL) return;

    char formatted[1024];
    const uint32_t length = formatted_text_buffer_build(
        formatted, sizeof(formatted), format, args);

    uint32_t capacity = 0;
    uint32_t cursor = 0;
    uint32_t flags = 0;
    void* buffer = NULL;
    memcpy(&buffer, (char*)record + 0x14, sizeof(buffer));
    memcpy(&capacity, (char*)record + 0x418, sizeof(capacity));
    memcpy(&cursor, (char*)record + 0x41c, sizeof(cursor));
    memcpy(&flags, (char*)record + 0x82c, sizeof(flags));
    if (buffer == NULL || capacity == 0 || cursor >= capacity) return;

    size_t start = 0;
    int bells = 0;
    while (start < length && bells < skip_bells) {
        if (formatted[start] == '\a') ++bells;
        ++start;
    }
    for (uint32_t i = (uint32_t)start; i < length; ++i) {
        ((char*)buffer)[cursor++] = formatted[i];
        if (cursor == capacity) {
            cursor = 0;
            flags |= 1u;
        }
    }
    memcpy((char*)record + 0x41c, &cursor, sizeof(cursor));
    memcpy((char*)record + 0x82c, &flags, sizeof(flags));
}

/* Port model of 0x00485300. */
void objective_advisor_text_copy(void* record, int mode,
                                 const char* format, ...)
{
    char local[1024];
    va_list args;
    va_start(args, format);
    formatted_text_buffer_build(local, sizeof(local), format, args);
    va_end(args);
    strcpy((char*)record + 0x18, local);
    if (mode == 0) {
        *(uint32_t*)((char*)record + 0x828) = 0x41c80000u;
    }
    else {
        *(uint32_t*)((char*)record + 0x828) = 0u;
    }
    *(uint32_t*)((char*)record + 0x82c) |= 2u;
}

/* Port model of 0x00485390. */
int text_buffer_draw_wrapped(void* record, uint32_t start_line,
                             float line_scale, int* height_out)
{
    int* words = (int*)record;
    int widths[257];
    void* font;
    char* buffer;
    uint32_t capacity;
    uint32_t cursor;
    uint32_t old_lines;
    uint32_t flags;
    uint32_t position = 0;
    uint32_t line_count = 0;
    uint32_t line;
    int line_width = 0;
    int word_width = 0;
    int line_height;
    int max_y = 0;
    int x;
    int y;
    float top_height;
    float wrapped_width;
    if (height_out != NULL) *height_out = 0;
    if (record == NULL) return 0;
    font = text_buffer_font_get(record);
    line_height = text_buffer_line_height(font);
    top_height = *(float*)((char*)record + 0x10);
    words[0x209] = crt_float_to_long(top_height) / line_height;
    flags = (uint32_t)words[0x20b];
    if ((flags & 2u) != 0u)
        return text_wrap_measure_and_draw_lines(record, line_scale,
                                                height_out);
    buffer = *(char**)((char*)record + 0x14);
    capacity = (uint32_t)words[0x106];
    cursor = (uint32_t)words[0x107];
    old_lines = (uint32_t)words[0x208];
    if ((flags & 1u) != 0u && old_lines != 0u) {
        int nearest = (int)capacity;
        int* saved = words + 0x108;
        uint32_t n;
        for (n = 0; n < old_lines; ++n) {
            int distance = saved[n] - (int)cursor;
            if (distance > 0 && distance < nearest) {
                nearest = distance;
                position = (uint32_t)saved[n];
            }
        }
    }
    words[0x208] = 0;
    words[0x108] = (int)position;
    words[0x208]++;
    line_count = 1;
    if (position != cursor && buffer != NULL) {
        do {
            unsigned char ch;
            ch = (unsigned char)buffer[position];
            if (ch == 7u) break;
            if (ch == '\n') {
                words[0x108 + line_count] = (int)(position + 1u);
                widths[line_count - 1u] = line_width;
                ++line_count;
                words[0x208] = (int)line_count;
                word_width = 0;
                line_width = 0;
            }
            else {
                int advance;
                if (text_buffer_is_break(ch)) {
                    word_width = 0;
                    words[0x108 + line_count] = (int)(position + 1u);
                    widths[line_count - 1u] = line_width;
                }
                advance = text_buffer_glyph_width(font, ch);
                line_width += advance;
                word_width += advance;
                if ((float)line_width > *(float*)((char*)record + 0x0c)) {
                    ++line_count;
                    words[0x208] = (int)line_count;
                    line_width = word_width;
                }
            }
            ++position;
            if (position >= capacity) position = 0;
        } while (position != cursor);
    }
    words[0x108 + line_count] = (int)position;
    widths[line_count - 1u] = line_width;
    words[0x208] = (int)line_count;
    wrapped_width = *(float*)((char*)record + 0x0c);
    {
        uint32_t rows = line_count - (uint32_t)words[0x209] + 1u;
        if (rows == 0u) start_line = 0u;
        else start_line %= rows;
    }
    x = crt_float_to_long(*(float*)((char*)record + 4));
    y = crt_float_to_long(*(float*)((char*)record + 8));
    for (line = 0; line < line_count; ++line) {
        uint32_t at = (uint32_t)words[0x108 + line];
        uint32_t end = (uint32_t)words[0x108 + line + 1u];
        int pen_x = x;
        int pen_y;
        if ((uint32_t)words[0x209] < line_count) {
            if (line < line_count - (uint32_t)words[0x209] ||
                line_count - start_line <= line) {
                pen_y = 0;
            }
            else {
                pen_y = y + ((int)(start_line + line +
                          (uint32_t)words[0x209] - line_count)) *
                          line_height;
            }
        }
        else pen_y = y + (int)line * line_height;
        if ((flags & 4u) != 0u && wrapped_width > (float)widths[line])
            pen_x += crt_float_to_long(
                (wrapped_width - (float)widths[line]) * 0.5f);
        while (at != end) {
            unsigned char ch = (unsigned char)buffer[at];
            pen_x += text_buffer_draw_glyph(font, pen_x, pen_y, ch);
            ++at;
            if (at >= capacity) at = 0;
        }
        if (pen_y > max_y) max_y = pen_y;
    }
    if (height_out != NULL) *height_out = max_y + line_height;
    return 0;
}
/* Port model of 0x00485710. */
int text_wrap_measure_and_draw_lines(void* record, float elapsed,
                                     int* height_out)
{
    int starts[257];
    int widths[257];
    const char* text;
    void* font;
    int line_height;
    int line_count = 1;
    int line_width = 0;
    int word_width = 0;
    int index = 0;
    int line = 0;
    int result = 0;
    int visible_lines;
    float elapsed_timer;
    char ch;
    if (record == NULL) return 0;
    visible_lines = (int)text_buffer_read_u32(record, 0x824);
    text = (const char*)record + 0x18;
    ch = text[0];
    font = text_buffer_font_get(record);
    line_height = text_buffer_line_height(font);
    starts[0] = 0;
    widths[0] = 0;
    if (ch != '\0') {
        do {
            if (ch == '\n') {
                widths[line_count - 1] = line_width;
                ++line_count;
                starts[line_count - 1] = index + 1;
                line_width = 0;
                word_width = 0;
            }
            else {
                if (text_buffer_is_break((unsigned char)ch)) {
                    widths[line_count - 1] = line_width;
                    starts[line_count] = index + 1;
                    word_width = 0;
                }
                {
                    int advance = text_buffer_glyph_width(
                        font, (unsigned char)ch);
                    line_width += advance;
                    word_width += advance;
                }
                if ((float)line_width > *(float*)((char*)record + 0x0c)) {
                    ++line_count;
                    line_width = word_width;
                }
            }
            ++index;
            ch = text[index];
        } while (ch != '\0');
    }
    widths[line_count - 1] = line_width;
    starts[line_count] = index;
    line = 0;
    while (line < line_count) {
        int x = crt_float_to_long(*(float*)((char*)record + 4));
        int y;
        int end = starts[line + 1];
        int pos = starts[line];
        if ((*(uint32_t*)((char*)record + 0x82c) & 4) != 0 &&
            *(float*)((char*)record + 0x0c) >
            (float)widths[line]) {
            x += crt_float_to_long((*(float*)((char*)record + 0x0c) -
                                    (float)widths[line]) * 0.5f);
        }
        if ((unsigned int)visible_lines < (unsigned int)line_count) {
            if ((unsigned int)line < (unsigned int)(line_count -
                    (uint32_t)visible_lines)) {
                y = 0;
            }
            else {
                y = crt_float_to_long(*(float*)((char*)record + 8)) +
                    ((int)line + visible_lines - line_count) *
                    line_height;
                result = 1;
            }
        }
        else y = crt_float_to_long(*(float*)((char*)record + 8)) +
                 line * line_height;
        if (y != 0) {
            while (pos < end) {
                unsigned char glyph = (unsigned char)text[pos];
                x += text_buffer_draw_glyph(font, x, y, glyph);
                ++pos;
            }
        }
        if (height_out != NULL && *height_out < y) *height_out = y;
        ++line;
    }
    elapsed_timer = *(float*)((char*)record + 0x828) - elapsed;
    *(float*)((char*)record + 0x828) = elapsed_timer;
    if (elapsed_timer < 0.0f)
        *(uint32_t*)((char*)record + 0x82c) &= ~2u;
    return result;
}
/* TextBufferReset target (FUN_004859b0) — see heap.hpp. The mapped body
 * follows the retail unchecked record and buffer accesses. */
/* Port model of 0x004859b0. */
void text_buffer_reset_target(void* record)
{
    uint32_t count = *(uint32_t const*)((char*)record + 0x418);
    char* buffer = *(char* const*)((char*)record + 0x14);
    if (buffer != NULL) {
        /* Pinned deviation (test_heap resets with a null slot): the corpus
         * sweeps count>>2 dwords then count&3 bytes blind through the slot;
         * the observable effect is count zero bytes. */
        memset(buffer, 0, count);
    }
    /* Deviation: the corpus's buffer slot is a dword ending before the
     * window, so the window zeroing never touches it; the port's widened
     * slot straddles the window start, so the slot is restored from the
     * pre-reset buffer value (a null slot stays zeroed either way). */
    memset((char*)record + 0x18, 0, 0x100 * 4);
    *(void**)((char*)record + 0x14) = buffer;

    *(uint32_t*)((char*)record + 0x820) = 0u;
    *(uint32_t*)((char*)record + 0x41c) = 0u;
    uint32_t word = *(uint32_t*)((char*)record + 0x82c);
    word &= ~1u;
    *(uint32_t*)((char*)record + 0x82c) = word;
}

/* The mapped body follows the corpus's blind record and output-pointer
 * accesses. */
/* Port model of 0x00485a00. */
void object_state_pair_get_target(const void* record, uint32_t* out_a,
                                  uint32_t* out_b)
{
    *out_a = *(const uint32_t*)((const char*)record + 0x820);
    *out_b = *(const uint32_t*)((const char*)record + 0x824);
}

static int text_buffer_font_binding_add(void* record, uintptr_t font_handle)
{
    TextBufferFontBinding* binding;
    if (font_handle <= 0xffffffffu) return 1;
    binding = (TextBufferFontBinding*)engine_heap_alloc(
        (long)sizeof(TextBufferFontBinding));
    if (binding == NULL) return 0;
    binding->record = record;
    binding->font = (void*)font_handle;
    binding->next = s_text_buffer_font_bindings;
    s_text_buffer_font_bindings = binding;
    return 1;
}

static void* text_buffer_font_binding_get(const void* record)
{
    TextBufferFontBinding* binding;
    for (binding = s_text_buffer_font_bindings; binding != NULL;
         binding = binding->next) {
        if (binding->record == record) return binding->font;
    }
    return NULL;
}

static uint32_t text_buffer_read_u32(const void* record, size_t offset)
{
    uint32_t value = 0;
    if (record != NULL) memcpy(&value, (const char*)record + offset,
                               sizeof(value));
    return value;
}

static void* text_buffer_font_get(const void* record)
{
    uint32_t token = 0u;
    void* font;
    if (record == NULL) return NULL;
    font = text_buffer_font_binding_get(record);
    if (font != NULL) return font;
    memcpy(&token, record, sizeof(token));
    return (void*)(uintptr_t)token;
}

static int text_buffer_line_height(void* font)
{
    int height = 1;
    if (s_text_buffer_draw_ops.font_line_height != NULL) {
        height = s_text_buffer_draw_ops.font_line_height(
            s_text_buffer_draw_ops.context, font);
    }
    return height > 0 ? height : 1;
}

static int text_buffer_is_break(unsigned char glyph)
{
    if (s_text_buffer_draw_ops.is_word_break != NULL) {
        return s_text_buffer_draw_ops.is_word_break(
            s_text_buffer_draw_ops.context, glyph) != 0;
    }
    return crt_character_classify_mask((int)glyph, 8u) != 0u;
}

static int text_buffer_glyph_width(void* font, unsigned char glyph)
{
    int width = 0;
    if (s_text_buffer_draw_ops.glyph_advance != NULL) {
        width = s_text_buffer_draw_ops.glyph_advance(
            s_text_buffer_draw_ops.context, font, glyph);
    }
    return width > 0 ? width : 0;
}

static int text_buffer_draw_glyph(void* font, int x, int y,
                                  unsigned char glyph)
{
    int width = text_buffer_glyph_width(font, glyph);
    if (s_text_buffer_draw_ops.glyph_draw != NULL) {
        return s_text_buffer_draw_ops.glyph_draw(
            s_text_buffer_draw_ops.context, font, x, y, glyph, 1);
    }
    return width;
}

#endif /* LRR_TEXT_WINDOW_PORT_NAMES */
