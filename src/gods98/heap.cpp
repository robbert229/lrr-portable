// SPDX-License-Identifier: LGPL-3.0-only
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "heap.hpp"
#include "string.hpp"
#include "surface.hpp"
#include "crt/crt.hpp"
#include "heap_shared.hpp"

/* Engine heap records and text helpers. CRT allocation implementation lives
 * in ../crt/crt_heap.cpp. */

// FUNCTION: LEGORR 0x0047ff40
void engine_heap_free_wrapper(void* ptr)
{
    engine_heap_free(ptr);
}

// FUNCTION: LEGORR 0x0047ff30
void engine_heap_allocate(long size)
{
    (void)engine_heap_alloc(size);
}


TextBufferFontBinding* s_text_buffer_font_bindings;

static void text_buffer_font_binding_remove(const void* record)
{
    TextBufferFontBinding** link = &s_text_buffer_font_bindings;
    while (*link != NULL) {
        TextBufferFontBinding* binding = *link;
        if (binding->record == record) {
            *link = binding->next;
            engine_heap_free(binding);
            return;
        }
        link = &binding->next;
    }
}

/* Keep the host-width font association in sync with the portable API. */
void text_buffer_destroy(void* record)
{
    if (record == NULL) return;
    text_buffer_font_binding_remove(record);
    text_buffer_destroy_target(record);
}

void object_field_38_release(void* record)
{
    if (record != NULL) object_field_38_release_target(record);
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

/* Keep the portable API null-safe; the mapped entry above follows the
 * retail function's unchecked pointer contract. */
void text_buffer_reset(void* record)
{
    if (record == NULL) return;
#if defined(_MSC_VER) && defined(_M_IX86)
    if (*(void**)((char*)record + 0x14) == NULL) {
        memset((char*)record + 0x18, 0, 0x100 * sizeof(uint32_t));
        *(uint32_t*)((char*)record + 0x820) = 0u;
        *(uint32_t*)((char*)record + 0x41c) = 0u;
        *(uint32_t*)((char*)record + 0x82c) &= ~1u;
        return;
    }
#endif
    text_buffer_reset_target(record);
}

/* The mapped release body blindly dereferences the holder and calls the
 * unchecked TextBufferReset target. */
// FUNCTION: LEGORR 0x0045a620
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void ui_text_output_object_release_target(
    void* const* holder)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        mov ecx, dword ptr [eax]
        push ecx
        call text_buffer_reset_target
        add esp, 4
        ret
    }
}
#else
void ui_text_output_object_release_target(void* const* holder)
{
    text_buffer_reset_target(*holder);
}
#endif

void ui_text_output_object_release(void* const* holder)
{
    if (holder == NULL || *holder == NULL) return;
    ui_text_output_object_release_target(holder);
}

TextBufferDrawOps s_text_buffer_draw_ops;

void text_buffer_draw_ops_set(const TextBufferDrawOps* ops)
{
    if (ops == NULL) memset(&s_text_buffer_draw_ops, 0,
                            sizeof(s_text_buffer_draw_ops));
    else s_text_buffer_draw_ops = *ops;
}

void text_buffer_draw_ops_get(TextBufferDrawOps* ops)
{
    if (ops != NULL) *ops = s_text_buffer_draw_ops;
}

typedef struct TextBufferFramebuffer {
    void* pixels;
    int pitch, width, height;
} TextBufferFramebuffer;
static TextBufferFramebuffer s_text_buffer_framebuffer;

static int text_buffer_software_font_line_height(void* context, void* font)
{
    (void)context;
    return font_atlas_record_line_height_get(font);
}

static int text_buffer_software_glyph_advance(void* context, void* font,
                                               unsigned char glyph)
{
    float rect[4];
    (void)context;
    if (!font_atlas_record_glyph_get(font, glyph, rect)) return 0;
    return crt_float_to_long(rect[2]);
}

static int text_buffer_software_glyph_draw(void* context, void* font,
                                            int x, int y,
                                            unsigned char glyph,
                                            int draw_mode)
{
    float rect[4];
    TextureSurface* atlas;
    DisplaySurface* target;
    void* pixels = NULL;
    int pitch = 0;
    int src_x, src_y, width, height, row;
    int advance;
    uint32_t key0, key1;
    TextBufferFramebuffer* framebuffer = (TextBufferFramebuffer*)context;
    if (!font_atlas_record_glyph_get(font, glyph, rect)) return 0;
    src_x = crt_float_to_long(rect[0]);
    src_y = crt_float_to_long(rect[1]);
    width = crt_float_to_long(rect[2]);
    height = crt_float_to_long(rect[3]);
    advance = width;
    if (draw_mode == 0 || width <= 0 || height <= 0) return advance;
    atlas = font_atlas_record_surface_get(font);
    target = framebuffer != NULL ? NULL : surface_active_render_target_get();
    if (atlas == NULL || atlas->pixels == NULL ||
        (framebuffer == NULL && target == NULL)) return advance;
    if (framebuffer != NULL) {
        pixels = framebuffer->pixels;
        pitch = framebuffer->pitch;
    } else if (surface_lock(target, &pixels, &pitch) != 0) return advance;
    int target_width = framebuffer != NULL ? framebuffer->width : target->w;
    int target_height = framebuffer != NULL ? framebuffer->height : target->h;
    if (pixels == NULL || pitch < target_width * 4) {
        if (target != NULL) surface_unlock(target, pixels);
        return advance;
    }
    key0 = atlas->key_count >= 2 ? atlas->keys[0] : atlas->pixels[0];
    key1 = atlas->key_count >= 2 ? atlas->keys[1] : key0;
    for (row = 0; row < height; ++row) {
        const int dest_y = y + row;
        int column;
        if (dest_y < 0 || dest_y >= target_height || src_y + row >= atlas->height)
            continue;
        for (column = 0; column < width; ++column) {
            const int dest_x = x + column;
            const int source_x = src_x + column;
            uint32_t source;
            uint8_t* destination;
            if (dest_x < 0 || dest_x >= target_width || source_x < 0 ||
                source_x >= atlas->width)
                continue;
            source = atlas->pixels[(size_t)(src_y + row) *
                                   (size_t)atlas->width +
                                   (size_t)source_x];
            if (source == key0 || source == key1 || (source >> 24) == 0u)
                continue;
            destination = (uint8_t*)pixels + (size_t)dest_y * (size_t)pitch +
                          (size_t)dest_x * 4u;
            destination[0] = (uint8_t)(source >> 16);
            destination[1] = (uint8_t)(source >> 8);
            destination[2] = (uint8_t)source;
            destination[3] = (uint8_t)(source >> 24);
        }
    }
    if (target != NULL) surface_unlock(target, pixels);
    return advance;
}

static int text_buffer_software_word_break(void* context,
                                            unsigned char glyph)
{
    (void)context;
    return crt_character_classify_mask((int)glyph, 8u) != 0u;
}

void text_buffer_draw_software_install(void)
{
    TextBufferDrawOps ops;
    memset(&ops, 0, sizeof(ops));
    ops.font_line_height = text_buffer_software_font_line_height;
    ops.glyph_advance = text_buffer_software_glyph_advance;
    ops.glyph_draw = text_buffer_software_glyph_draw;
    ops.is_word_break = text_buffer_software_word_break;
    text_buffer_draw_ops_set(&ops);
}

void text_buffer_draw_framebuffer_install(void* pixels, int pitch,
                                          int width, int height)
{
    text_buffer_draw_software_install();
    s_text_buffer_framebuffer.pixels = pixels;
    s_text_buffer_framebuffer.pitch = pitch;
    s_text_buffer_framebuffer.width = width;
    s_text_buffer_framebuffer.height = height;
    s_text_buffer_draw_ops.context = &s_text_buffer_framebuffer;
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

