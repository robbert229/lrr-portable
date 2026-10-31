// SPDX-License-Identifier: LGPL-3.0-only
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "heap.hpp"
#include "string.hpp"

/* Engine heap records and text helpers. CRT allocation implementation lives
 * in ../crt/crt_heap.cpp. */

// FUNCTION: LEGORR 0x0047ff40
void engine_heap_free_wrapper(void* ptr)
{
    engine_heap_free(ptr);
}

// FUNCTION: LEGORR 0x0045daf0
void ui_render_record_free(void* record)
{
    engine_heap_free(record);
}

// FUNCTION: LEGORR 0x0047ff30
void engine_heap_allocate(long size)
{
    (void)engine_heap_alloc(size);
}


/* OptionalResourceDestroy (FUN_0040f7b0) — see heap.hpp. */
// FUNCTION: LEGORR 0x0040f7b0
void optional_resource_destroy(void* record)
{
    if (record != NULL) {
        engine_heap_free(record);
    }
}

/* TextBufferDestroy (FUN_00485190) — see heap.hpp. */
// FUNCTION: LEGORR 0x00485190
void text_buffer_destroy(void* record)
{
    void* buffer = NULL;
    __builtin_memcpy(&buffer, (char*)record + 0x14, sizeof(buffer));
    engine_heap_free(buffer);
    engine_heap_free(record);
}

/* TextOutputField82CBit2SetClear (FUN_00485160) — see heap.hpp. */
// FUNCTION: LEGORR 0x00485160
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

/* ObjectField38Release (FUN_00489d70) — see heap.hpp. */
// FUNCTION: LEGORR 0x00489d70
void object_field_38_release(void* record)
{
    if (record == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    void* field = *(void**)((char*)record + 0x38);
    if (field != NULL) {
        engine_heap_free(field);
    }
}

/* ThreeWordRecordAllocateAndInitialize (FUN_00410b60) — see heap.hpp. */
// FUNCTION: LEGORR 0x00410b60
void* three_word_record_allocate_and_initialize(uint32_t a, uint32_t b,
                                                uint32_t c)
{
    uint32_t* rec = (uint32_t*)engine_heap_alloc(12);
    if (rec == NULL) {
        return NULL; /* deviation: the corpus strides the record blindly */
    }
    rec[0] = a;
    rec[1] = b;
    rec[2] = c;
    return rec;
}

/* DwordListAllocateAndZero (FUN_00414070) — see heap.hpp. */
// FUNCTION: LEGORR 0x00414070
void** dword_list_allocate_and_zero(uint32_t count)
{
    void** hdr = (void**)engine_heap_alloc(2 * sizeof(void*));
    if (hdr == NULL) {
        return NULL; /* deviation: the corpus strides both halves blindly */
    }
    hdr[0] = NULL;
    hdr[1] = NULL;
    void* data = engine_heap_alloc((long)(count * sizeof(void*)));
    hdr[0] = data;
    /* Corpus shape: the zero-pass sweeps `count & 0x3fffffff` slots and the
     * second pass is dead code (loop seed 0) — kept faithfully. */
    void** p = (void**)data;
    const uint32_t n = count & 0x3fffffffu;
    for (uint32_t i = 0; i != n; ++i) {
        p[i] = NULL;
    }
    if (data == NULL && count != 0) {
        engine_heap_free(hdr); /* deviation: the corpus leaves both behind */
        return NULL;
    }
    hdr[1] = (void*)(size_t)count;
    return hdr;
}

/* TextBufferReset (FUN_004859b0) — see heap.hpp. On x86, retain the
 * reference's dword/byte buffer sweep, 0x100-dword record sweep and flag
 * update. The record and buffer checks preserve the port's null-safe API. */
// FUNCTION: LEGORR 0x004859b0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void text_buffer_reset(void* record)
{
    __asm {
        mov edx, dword ptr [esp + 4]
        test edx, edx
        je text_buffer_reset_return
        push esi
        push edi
        xor eax, eax
        mov ecx, dword ptr [edx + 0x418]
        mov edi, dword ptr [edx + 0x14]
        test edi, edi
        je text_buffer_reset_record_window
        mov esi, ecx
        shr ecx, 2
        rep stosd
        mov ecx, esi
        and ecx, 3
        rep stosb
    text_buffer_reset_record_window:
        mov ecx, 0x100
        xor eax, eax
        lea edi, dword ptr [edx + 0x18]
        rep stosd
        mov dword ptr [edx + 0x820], eax
        mov dword ptr [edx + 0x41c], eax
        mov eax, dword ptr [edx + 0x82c]
        and al, 0xfe
        mov dword ptr [edx + 0x82c], eax
        pop edi
        pop esi
    text_buffer_reset_return:
        ret
    }
}
#else
void text_buffer_reset(void* record)
{
    if (record == NULL) {
        return; /* deviation: the corpus strides the record blindly */
    }

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
#endif

/* UITextOutputObjectRelease (FUN_0045a620) — see heap.hpp. */
// FUNCTION: LEGORR 0x0045a620
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void ui_text_output_object_release(void* const* holder)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        test eax, eax
        je ui_text_output_release_return
        mov ecx, dword ptr [eax]
        push ecx
        call text_buffer_reset
        add esp, 4
    ui_text_output_release_return:
        ret
    }
}
#else
void ui_text_output_object_release(void* const* holder)
{
    if (holder == NULL) {
        return; /* pinned deviation: the corpus dereferences blindly */
    }
    text_buffer_reset(*holder);
}
#endif

/* TextBufferCreate (FUN_004850f0) — see heap.hpp. */
// FUNCTION: LEGORR 0x004850f0
void* text_buffer_create(uint32_t mode, const uint32_t* header, long size)
{
    char* record = (char*)engine_heap_alloc(0x830);
    if (record == NULL) {
        return NULL; /* corpus: je — the allocation guard exists in the original */
    }
    memset(record, 0, 0x830); /* the corpus's 0x20c-dword sweep */
    __builtin_memcpy(record + 0x00, &mode, sizeof mode);
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

/* TextBufferAppendVFormat (FUN_00485250): append formatted bytes to the
 * text-output circular buffer, optionally starting after `skip_bells` BEL
 * markers. The record layout follows TextBufferCreate/Reset. */
// FUNCTION: LEGORR 0x00485250
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

/* TextBufferAppendFormat (FUN_00485230): variadic wrapper with bell-skip
 * count fixed at zero, matching the decompiled call to TextBufferAppendVFormat. */
// FUNCTION: LEGORR 0x00485230
void text_buffer_append_format(void* record, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    text_buffer_append_vformat(record, 0, format, args);
    va_end(args);
}

/* FormattedTextAppendToRingBuffer (FUN_00485210): variadic face of the
 * text-buffer append path, forwarding its argument list to the VFormat body. */
// FUNCTION: LEGORR 0x00485210
void formatted_text_append_to_ring_buffer(void* record, int skip_bells,
                                          const char* format, ...)
{
    va_list args;
    va_start(args, format);
    text_buffer_append_vformat(record, skip_bells, format, args);
    va_end(args);
}

static TextBufferDrawOps s_text_buffer_draw_ops;

void text_buffer_draw_ops_set(const TextBufferDrawOps* ops)
{
    if (ops == NULL) memset(&s_text_buffer_draw_ops, 0,
                            sizeof(s_text_buffer_draw_ops));
    else s_text_buffer_draw_ops = *ops;
}

static uint32_t text_buffer_read_u32(const void* record, size_t offset)
{
    uint32_t value = 0;
    if (record != NULL) memcpy(&value, (const char*)record + offset,
                               sizeof(value));
    return value;
}

static void text_buffer_write_u32(void* record, size_t offset,
                                  uint32_t value)
{
    if (record != NULL) memcpy((char*)record + offset, &value,
                               sizeof(value));
}

static float text_buffer_read_float(const void* record, size_t offset)
{
    float value = 0.0f;
    if (record != NULL) memcpy(&value, (const char*)record + offset,
                               sizeof(value));
    return value;
}

static void* text_buffer_font_get(const void* record)
{
    void* font = NULL;
    if (record != NULL) memcpy(&font, record, sizeof(font));
    return font;
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
    return glyph == ' ' || glyph == '\t' || glyph == '\r';
}

static int text_buffer_glyph_width(void* font, unsigned char glyph)
{
    int width = 0;
    if (s_text_buffer_draw_ops.glyph_advance != NULL) {
        width = s_text_buffer_draw_ops.glyph_advance(
            s_text_buffer_draw_ops.context, font, glyph);
    }
    return width > 0 ? width : 1;
}

static int text_buffer_draw_glyph(void* font, int x, int y,
                                  unsigned char glyph)
{
    int width = text_buffer_glyph_width(font, glyph);
    if (s_text_buffer_draw_ops.glyph_draw != NULL) {
        int advance = s_text_buffer_draw_ops.glyph_draw(
            s_text_buffer_draw_ops.context, font, x, y, glyph, 1);
        if (advance > 0) width = advance;
    }
    return width;
}

static int text_buffer_layout_inline(void* record, void* font,
                                    unsigned int starts[257],
                                    unsigned int lengths[256],
                                    unsigned int widths[256])
{
    const char* text = (const char*)record + 0x18;
    const unsigned int max_chars = 0x400u;
    float max_width = text_buffer_read_float(record, 0x0c);
    unsigned int line_count = 1;
    unsigned int line_start = 0;
    unsigned int i = 0;
    unsigned int last_break = 0xffffffffu;
    unsigned int width_at_break = 0;
    unsigned int width = 0;
    starts[0] = 0;
    lengths[0] = 0;
    widths[0] = 0;
    while (i < max_chars && text[i] != '\0') {
        unsigned char ch = (unsigned char)text[i];
        if (ch == '\n') {
            lengths[line_count - 1] = i - line_start;
            widths[line_count - 1] = width;
            if (line_count >= 256u) break;
            ++line_count;
            line_start = i + 1;
            starts[line_count - 1] = line_start;
            lengths[line_count - 1] = 0;
            widths[line_count - 1] = 0;
            last_break = 0xffffffffu;
            width_at_break = 0;
            width = 0;
            ++i;
            continue;
        }
        {
            unsigned int advance = (unsigned int)
                text_buffer_glyph_width(font, ch);
            if (max_width > 0.0f &&
                width + advance > (unsigned int)max_width &&
                i > line_start && line_count < 256u) {
                if (last_break != 0xffffffffu && last_break >= line_start) {
                    unsigned int next_start = last_break + 1u;
                    lengths[line_count - 1] = last_break - line_start;
                    widths[line_count - 1] = width_at_break;
                    ++line_count;
                    while (next_start < max_chars &&
                           text_buffer_is_break(
                               (unsigned char)text[next_start])) ++next_start;
                    line_start = next_start;
                    i = next_start;
                }
                else {
                    lengths[line_count - 1] = i - line_start;
                    widths[line_count - 1] = width;
                    ++line_count;
                    line_start = i;
                }
                starts[line_count - 1] = line_start;
                lengths[line_count - 1] = 0;
                widths[line_count - 1] = 0;
                width = 0;
                last_break = 0xffffffffu;
                width_at_break = 0;
                continue;
            }
            if (text_buffer_is_break(ch)) {
                last_break = i;
                width_at_break = width;
            }
            width += advance;
        }
        ++i;
    }
    lengths[line_count - 1] = i - line_start;
    widths[line_count - 1] = width;
    starts[line_count] = i;
    return (int)line_count;
}

// FUNCTION: LEGORR 0x00485710
int text_wrap_measure_and_draw_lines(void* record, float elapsed,
                                     int* height_out)
{
    unsigned int starts[257];
    unsigned int lengths[256];
    unsigned int widths[256];
    void* font;
    int line_height;
    int line_count;
    int x;
    int y;
    int max_height = 0;
    int result = 0;
    unsigned int scroll;
    unsigned int line;
    uint32_t flags;
    float timer;
    float x_float;
    float y_float;
    if (height_out != NULL) *height_out = 0;
    if (record == NULL) return 0;
    font = text_buffer_font_get(record);
    line_height = text_buffer_line_height(font);
    x_float = text_buffer_read_float(record, 4);
    y_float = text_buffer_read_float(record, 8);
    x = (int)x_float;
    y = (int)y_float;
    line_count = text_buffer_layout_inline(record, font, starts, lengths,
                                           widths);
    if (line_count <= 0) return 0;
    scroll = text_buffer_read_u32(record, 0x824);
    if (scroll >= (unsigned int)line_count) {
        scroll %= (unsigned int)line_count;
        result = 1;
    }
    for (line = 0; line < (unsigned int)line_count; ++line) {
        unsigned int actual = (line + scroll) % (unsigned int)line_count;
        unsigned int j;
        int pen_x = x;
        int pen_y = y + (int)line * line_height;
        const char* text = (const char*)record + 0x18;
        for (j = 0; j < lengths[actual]; ++j) {
            unsigned char ch = (unsigned char)text[starts[actual] + j];
            if (ch != '\n' && ch != '\r')
                pen_x += text_buffer_draw_glyph(font, pen_x, pen_y, ch);
        }
        if (pen_y + line_height > max_height) {
            max_height = pen_y + line_height;
        }
    }
    if (height_out != NULL) *height_out = max_height - y;
    timer = text_buffer_read_float(record, 0x828) - elapsed;
    memcpy((char*)record + 0x828, &timer, sizeof(timer));
    flags = text_buffer_read_u32(record, 0x82c);
    if (timer < 0.0f) {
        flags &= ~2u;
        text_buffer_write_u32(record, 0x82c, flags);
    }
    text_buffer_write_u32(record, 0x824, scroll);
    return result;
}

// FUNCTION: LEGORR 0x00485390
int text_buffer_draw_wrapped(void* record, uint32_t start_line,
                             float line_scale, int* height_out)
{
    unsigned int starts[257];
    unsigned int ends[256];
    unsigned int widths[256];
    void* font;
    char* buffer;
    uint32_t capacity;
    uint32_t read_at;
    uint32_t flags;
    uint32_t line_count = 0;
    uint32_t index;
    uint32_t line_start;
    uint32_t break_index;
    uint32_t steps;
    unsigned int line_width;
    unsigned int word_width;
    uint32_t word_break;
    unsigned int max_width;
    int line_height;
    int x;
    int y;
    int height = 0;
    float x_float;
    float y_float;
    if (height_out != NULL) *height_out = 0;
    if (record == NULL) return 0;
    flags = text_buffer_read_u32(record, 0x82c);
    if ((flags & 2u) != 0u) {
        return text_wrap_measure_and_draw_lines(record, line_scale,
                                                height_out);
    }
    font = text_buffer_font_get(record);
    buffer = NULL;
    memcpy(&buffer, (char*)record + 0x14, sizeof(buffer));
    capacity = text_buffer_read_u32(record, 0x418);
    read_at = text_buffer_read_u32(record, 0x41c);
    if (buffer == NULL || capacity == 0u) return 0;
    if (read_at >= capacity) read_at %= capacity;
    {
        float width = text_buffer_read_float(record, 0x0c);
        max_width = width > 0.0f ? (unsigned int)width : 0u;
    }
    line_height = text_buffer_line_height(font);
    x_float = text_buffer_read_float(record, 4);
    y_float = text_buffer_read_float(record, 8);
    x = (int)x_float;
    y = (int)y_float;
    line_start = read_at;
    line_width = 0;
    word_width = 0;
    word_break = 0xffffffffu;
    starts[0] = line_start;
    widths[0] = 0;
    line_count = 1;
    index = read_at;
    steps = 0;
    while (steps < capacity && line_count < 256u) {
        uint32_t at = index % capacity;
        unsigned char ch = (unsigned char)buffer[at];
        if (ch == 7u) break;
        if (ch == '\n') {
            ends[line_count - 1] = at;
            widths[line_count - 1] = line_width;
            ++line_count;
            ++index;
            ++steps;
            line_start = index % capacity;
            starts[line_count - 1] = line_start;
            line_width = 0;
            word_width = 0;
            word_break = 0xffffffffu;
            continue;
        }
        {
            unsigned int advance = (unsigned int)
                text_buffer_glyph_width(font, ch);
            if (max_width != 0u && line_width + advance > max_width &&
                at != line_start) {
                if (word_break != 0xffffffffu) {
                    uint32_t next_index = break_index + 1u;
                    ends[line_count - 1] = word_break;
                    widths[line_count - 1] = word_width;
                    ++line_count;
                    while (steps < capacity &&
                           text_buffer_is_break((unsigned char)
                               buffer[next_index % capacity])) {
                        ++next_index;
                        ++steps;
                    }
                    index = next_index;
                    steps = index - read_at;
                    line_start = index % capacity;
                }
                else {
                    ends[line_count - 1] = at;
                    widths[line_count - 1] = line_width;
                    ++line_count;
                    line_start = at;
                }
                starts[line_count - 1] = line_start;
                widths[line_count - 1] = 0;
                line_width = 0;
                word_break = 0xffffffffu;
                word_width = 0;
                continue;
            }
            if (text_buffer_is_break(ch)) {
                word_break = at;
                break_index = index;
                word_width = line_width;
            }
            line_width += advance;
        }
        ++index;
        ++steps;
    }
    ends[line_count - 1] = index % capacity;
    widths[line_count - 1] = line_width;
    starts[line_count] = ends[line_count - 1];
    text_buffer_write_u32(record, 0x820, line_count);
    for (index = 0; index < line_count; ++index) {
        text_buffer_write_u32(record, 0x420u + index * 4u, starts[index]);
    }
    if (line_scale <= 0.0f) line_scale = 1.0f;
    start_line %= line_count;
    for (index = 0; index < line_count; ++index) {
        uint32_t line = (start_line + index) % line_count;
        uint32_t at = starts[line];
        uint32_t end = ends[line];
        int pen_x = x;
        int pen_y = y + (int)(index * line_height * line_scale);
        uint32_t guard = 0;
        while (at != end && guard++ < capacity) {
            unsigned char ch = (unsigned char)buffer[at];
            pen_x += text_buffer_draw_glyph(font, pen_x, pen_y, ch);
            at = (at + 1u) % capacity;
        }
        if (pen_y + line_height > height) height = pen_y + line_height;
    }
    if (height_out != NULL) *height_out = height - y;
    text_buffer_write_u32(record, 0x824, start_line);
    return 0;
}
