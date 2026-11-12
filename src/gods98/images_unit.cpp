// SPDX-License-Identifier: LGPL-3.0-or-later
/* G98-011 Images.c: the unit's functions in original address order and the
 * state they own, relocated verbatim by tools/relocate_unit.py (ADR 0006).
 * Body hashes: docs/coverage/relocations/G98-011.csv. */
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "platform/platform.h"
#include "surface.hpp"
#include "model.hpp"
#include "record.hpp"
#include "crt/crt.hpp"
#include "engine.hpp"
#include "heap.hpp"
#include "texture.hpp"
#include "view.hpp"
#include "frameloop.hpp"
#include "scene_resources.hpp"
#include <float.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include "string.hpp"
#include "sound.hpp"
#include "resource.hpp"
#include "game/util.hpp"
#include <limits.h>
#include <stdarg.h>
#include "scene_pool.hpp"
#include "../game/ai.hpp"
#include "../game/entity.hpp"
#include "../game/level.hpp"
#include "view_for_com_object_tree.hpp"
#include "surface_for_images_unit.hpp"
#include "view_for_images_unit.hpp"

typedef uint32_t (VIEW_COM_CALL *SceneRegistryInterfaceReleaseFn)(void*);

// GLOBAL: LEGORR 0x00533908
RendererRecordTableState s_renderer_record_table;

// GLOBAL: LEGORR 0x00533988
void* s_small_record_free;

/* Defined below (forward declarations added by tools/relocate_unit.py). */
static void scene_registry_record_next_set(void* record, void* next);
static uint32_t surface_image_key_color(float red, float green, float blue);
static void surface_directdraw_unlock_call(void* surface);
static void* scene_registry_record_next_get(void* record);
static size_t scene_registry_record_stride(void);
static int font_device_pixel_format_query(void* device, uint32_t* bits_per_pixel, uint32_t* red_mask, uint32_t* green_mask, uint32_t* blue_mask);

/* RendererRecordTableInitialize (FUN_0047d410) clears its 32-word table
 * and raises the adjacent initialized word. The two intervening dwords are
 * retained as reserved state because the corpus leaves them untouched. */
// FUNCTION: LEGORR 0x0047d410
void renderer_record_table_initialize(void)
{
    memset(s_renderer_record_table.words, 0,
           sizeof(s_renderer_record_table.words));
    s_renderer_record_table.enabled = 1u;
}

// FUNCTION: LEGORR 0x0047d430
void scene_object_registry_teardown(void)
{
    unsigned int i;
    scene_object_registry_release_all();
    fixed_size_small_record_pool_teardown();
    for (i = 0u; i < s_scene_registry_block_count; ++i) {
        s_scene_registry_blocks[i] = NULL;
    }
    s_scene_registry_block_count = 0u;
}
// FUNCTION: LEGORR 0x0047d470
void image_surface_release(void* record)
{
    uint32_t interface_token = 0u;
    if (record == NULL) return;
    memcpy(&interface_token, record, sizeof(interface_token));
    /* ImageSurfaceRelease calls vtable slot +8 (IUnknown::Release) on the
     * record's first dword. The record ABI stores that interface as a 32-bit
     * value, so this direct call is valid only in the 32-bit reference ABI. */
    if (s_scene_registry_surface_release != NULL) {
        s_scene_registry_surface_release(interface_token);
    }
    else if (sizeof(void*) == sizeof(uint32_t) && interface_token != 0u) {
        void* interface_object = (void*)(uintptr_t)interface_token;
        void** vtable = *(void***)interface_object;
        if (vtable != NULL && vtable[2] != NULL) {
            ((SceneRegistryInterfaceReleaseFn)vtable[2])(interface_object);
        }
    }
    scene_registry_record_next_set(record, s_small_record_free);
    s_small_record_free = record;
}

/* SurfacePixelFormatQueryConvert (FUN_0047d490): lock the supplied surface,
 * build its pixel record from the 0x7c-byte descriptor, dispatch the 8/24-bit
 * conversion, and unlock on every successful lock. The source buffer and
 * palette remain caller-owned in SurfaceConvertJob. */
// FUNCTION: LEGORR 0x0047d490
int surface_pixel_format_query_convert_surface(
    void* surface, const SurfaceConvertJob* job)
{
#if defined(LEGORRR_BACKEND_MINIWIN) || \
    (defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64))
    void** vtable = surface_interface_vtable(surface);
    typedef int32_t (SURFACE_COM_CALL *SurfaceRawLockFn)(
        void*, void*, void*, uint32_t, void*);
    typedef int32_t (SURFACE_COM_CALL *SurfaceRawUnlockFn)(void*, void*);
    SurfaceRawLockFn lock;
    SurfaceRawUnlockFn unlock;
#if defined(LEGORRR_BACKEND_MINIWIN)
    DDSURFACEDESC descriptor;
#else
    uint32_t descriptor[31];
    const unsigned char* bytes = (const unsigned char*)descriptor;
#endif
    SurfaceRecord record;
    uint32_t height;
    uint32_t width;
    uint32_t pitch;
    uint32_t bits_per_pixel;
    int result;

    if (surface == NULL || job == NULL || vtable == NULL ||
        vtable[25] == NULL || vtable[32] == NULL) return 0;
    lock = (SurfaceRawLockFn)vtable[25];
    unlock = (SurfaceRawUnlockFn)vtable[32];
#if defined(LEGORRR_BACKEND_MINIWIN)
    memset(&descriptor, 0, sizeof(descriptor));
    descriptor.dwSize = sizeof(descriptor);
#else
    memset(descriptor, 0, sizeof(descriptor));
    descriptor[0] = 0x7cu;
#endif
#if defined(LEGORRR_BACKEND_MINIWIN)
    if (lock(surface, NULL, &descriptor, 1u, NULL) != 0) return 0;
#else
    if (lock(surface, NULL, descriptor, 1u, NULL) != 0) return 0;
#endif
#if defined(LEGORRR_BACKEND_MINIWIN)
    height = descriptor.dwHeight;
    width = descriptor.dwWidth;
    pitch = (uint32_t)descriptor.lPitch;
    bits_per_pixel = descriptor.ddpfPixelFormat.dwRGBBitCount;
    memset(&record, 0, sizeof(record));
    record.pixels = descriptor.lpSurface;
    record.mask_low = descriptor.ddpfPixelFormat.dwRBitMask;
    record.mask_mid = descriptor.ddpfPixelFormat.dwGBitMask;
    record.mask_hi = descriptor.ddpfPixelFormat.dwBBitMask;
#else
    memcpy(&height, bytes + 8u, sizeof(height));
    memcpy(&width, bytes + 12u, sizeof(width));
    memcpy(&pitch, bytes + 16u, sizeof(pitch));
    memcpy(&bits_per_pixel, bytes + 88u, sizeof(bits_per_pixel));
    memset(&record, 0, sizeof(record));
    {
        uint32_t pixels;
        memcpy(&pixels, bytes + 36u, sizeof(pixels));
        record.pixels = (void*)(uintptr_t)pixels;
    }
    memcpy(&record.mask_low, bytes + 92u, sizeof(record.mask_low));
    memcpy(&record.mask_mid, bytes + 96u, sizeof(record.mask_mid));
    memcpy(&record.mask_hi, bytes + 100u, sizeof(record.mask_hi));
#endif
    record.row_count = (int)height;
    record.row_bytes = (int)pitch;
    record.depth = (int)bits_per_pixel;
    (void)width; /* source width is carried by the conversion job */
    result = surface_pixel_format_query_convert(&record, job,
                                                bits_per_pixel);
    (void)unlock(surface, NULL);
    return result;
#else
    (void)surface;
    (void)job;
    return 0;
#endif
}
/* Palette8To16BitSurfaceExpand (FUN_0047D520) — see surface.hpp. The corpus
 * reads the depth at +0x54: 8/24/32 do no work and answer 0, 16-bit runs,
 * any other depth answers 1 untouched. The 16-bit walk: for every source
 * byte (a palette index) the entry's three bytes land in their mask's field
 * as ((byte >> (8 - popcount(mask))) << first_set(mask)), OR'd into the
 * stored word — byte 0 rides the +0x58 mask, byte 1 the +0x5c mask, byte 2
 * the +0x60 mask, the entries four bytes apart. Rows then flip. Deviations:
 * the blind record/job/source strides are guarded. */
// FUNCTION: LEGORR 0x0047d520
int surface_palette8_to_16bit_surface_expand(SurfaceRecord* rec,
                                             const SurfaceConvertJob* job)
{
    if (rec == NULL || job == NULL || rec->pixels == NULL) {
        return 1; /* deviation: the corpus strides the record blindly */
    }
    if (rec->depth == 8 || rec->depth == 24 || rec->depth == 32) {
        return 0;
    }
    if (rec->depth != 16) {
        return 1;
    }
    if (job->src == NULL || job->palette == NULL) {
        return 1; /* deviation: the corpus strides the job blindly */
    }
    const uint32_t mask_low = rec->mask_low;
    const uint32_t mask_mid = rec->mask_mid;
    const uint32_t mask_hi = rec->mask_hi;
    const uint32_t shr_low = (uint32_t)(8 - pop_count32(mask_low)) & 0x1fu;
    const uint32_t shr_mid = (uint32_t)(8 - pop_count32(mask_mid)) & 0x1fu;
    const uint32_t shr_hi = (uint32_t)(8 - pop_count32(mask_hi)) & 0x1fu;
    const uint32_t ffs_low = find_first_set_bit32(mask_low);
    const uint32_t ffs_mid = find_first_set_bit32(mask_mid);
    const uint32_t ffs_hi = find_first_set_bit32(mask_hi);
    const char* src = (const char*)job->src;
    const char* palette = (const char*)job->palette;
    uint16_t* dst = (uint16_t*)rec->pixels;
    for (int y = 0; y < job->height; ++y) {
        for (int x = 0; x < job->width; ++x) {
            const uint32_t index = (uint8_t)*src++;
            const char* entry = palette + index * 4u;
            const uint16_t word = (uint16_t)(
                ((uint32_t)((uint8_t)entry[0] >> shr_low) << (ffs_low & 0x1fu))
                | ((uint32_t)((uint8_t)entry[1] >> shr_mid)
                   << (ffs_mid & 0x1fu))
                | ((uint32_t)((uint8_t)entry[2] >> shr_hi) << (ffs_hi & 0x1fu)));
            *dst++ = word;
        }
        src += job->src_stride - job->width;
        dst += (rec->row_bytes >> 1) - job->width;
    }
    surface_pixel_rows_flip_with_scratch(rec);
    return 1;
}

/* FUN_0047d700: popcount via the per-position test loop. */
// FUNCTION: LEGORR 0x0047d700
int pop_count32(uint32_t value)
{
    int count = 0;
    uint32_t position = 0u;
    do {
        if ((value & (1u << position)) != 0u) {
            ++count;
        }
        ++position;
    } while (position < 0x20u);
    return count;
}

/* FUN_0047d720: index of the lowest set bit, 0xffffffff for zero. The
 * corpus's 0x47d72e `shl esi,cl` repeatedly shifts its live mask; without
 * resetting that mask, it answers 2 for 0x8 rather than the tested 3.
 * Keep the one-hot-per-position semantics; the i&31 is redundant under
 * the i < 32 bound. */
// FUNCTION: LEGORR 0x0047d720
uint32_t find_first_set_bit32(uint32_t value)
{
    for (uint32_t i = 0; i < 32u; ++i) {
        if ((value & (1u << i)) != 0u) {
            return i;
        }
    }
    return 0xffffffffu;
}

/* SurfacePixelRowsFlipWithScratch (FUN_0047D740): reverse row pairs using
 * the original dword loop and its trailing-byte loop. */
// FUNCTION: LEGORR 0x0047d740
void surface_pixel_rows_flip_with_scratch(SurfaceRecord* rec)
{
    uint32_t half_bytes = (uint32_t)(rec->row_bytes / 2);
    uint32_t span = half_bytes * 2u;
    uint32_t* top = (uint32_t*)rec->pixels;
    uint32_t* bottom = (uint32_t*)((char*)top +
        ((uint32_t)rec->row_count - 1u) * span);
    uint32_t* scratch = (uint32_t*)engine_heap_alloc((long)span);
    uint32_t lap = 0u;

    if (((uint32_t)rec->row_count & 0xfffffffeu) != 0u) {
        do {
            uint32_t* source = top;
            uint32_t* destination = scratch;
            uint32_t count;
            for (count = (half_bytes & 0x7fffffffu) >> 1;
                 count != 0u; --count) {
                *destination++ = *source++;
            }
            for (count = span & 3u; count != 0u; --count) {
                *(uint8_t*)destination = *(uint8_t*)source;
                destination = (uint32_t*)((char*)destination + 1);
                source = (uint32_t*)((char*)source + 1);
            }

            source = bottom;
            destination = top;
            for (count = (half_bytes & 0x7fffffffu) >> 1;
                 count != 0u; --count) {
                *destination++ = *source++;
            }
            for (count = span & 3u; count != 0u; --count) {
                *(uint8_t*)destination = *(uint8_t*)source;
                destination = (uint32_t*)((char*)destination + 1);
                source = (uint32_t*)((char*)source + 1);
            }

            source = scratch;
            destination = bottom;
            for (count = (half_bytes & 0x7fffffffu) >> 1;
                 count != 0u; --count) {
                *destination++ = *source++;
            }
            for (count = span & 3u; count != 0u; --count) {
                *(uint8_t*)destination = *(uint8_t*)source;
                destination = (uint32_t*)((char*)destination + 1);
                source = (uint32_t*)((char*)source + 1);
            }

            ++lap;
            top = (uint32_t*)((char*)top + span);
            bottom = (uint32_t*)((char*)bottom - span);
        } while (lap < ((uint32_t)rec->row_count >> 1));
    }
    engine_heap_free(scratch);
}

/* Bitmap24BitSurfaceConvert (FUN_0047D800) — see surface.hpp. The 24-bit
 * sibling: same depth verdict, same field placement per mask, but the three
 * source bytes per pixel ride the masks REVERSED — byte 0 lands in the
 * +0x60 (high) field, byte 1 in +0x5c, byte 2 in +0x58 — and rows advance
 * by the source stride over three-byte pixels. Deviations and the flip tail
 * ride the palette walk's notes. */
// FUNCTION: LEGORR 0x0047d800
int surface_bitmap24_bit_surface_convert(SurfaceRecord* rec,
                                         const SurfaceConvertJob* job)
{
    if (rec == NULL || job == NULL || rec->pixels == NULL) {
        return 1; /* deviation: the corpus strides the record blindly */
    }
    if (rec->depth == 8) {
        return 0;
    }
    if (rec->depth == 16) {
        if (job->src == NULL) {
            return 1; /* deviation: the corpus strides the job blindly */
        }
        const uint32_t mask_low = rec->mask_low;
        const uint32_t mask_mid = rec->mask_mid;
        const uint32_t mask_hi = rec->mask_hi;
        const uint32_t shr_low =
            (uint32_t)(8 - pop_count32(mask_low)) & 0x1fu;
        const uint32_t shr_mid =
            (uint32_t)(8 - pop_count32(mask_mid)) & 0x1fu;
        const uint32_t shr_hi =
            (uint32_t)(8 - pop_count32(mask_hi)) & 0x1fu;
        const uint32_t ffs_low = find_first_set_bit32(mask_low);
        const uint32_t ffs_mid = find_first_set_bit32(mask_mid);
        const uint32_t ffs_hi = find_first_set_bit32(mask_hi);
        const char* src = (const char*)job->src;
        uint16_t* dst = (uint16_t*)rec->pixels;
        for (int y = 0; y < job->height; ++y) {
            for (int x = 0; x < job->width; ++x) {
                const uint32_t b0 = (uint8_t)src[0];
                const uint32_t b1 = (uint8_t)src[1];
                const uint32_t b2 = (uint8_t)src[2];
                src += 3;
                const uint16_t word = (uint16_t)(
                    ((b0 >> shr_hi) << (ffs_hi & 0x1fu))
                    | ((b1 >> shr_mid) << (ffs_mid & 0x1fu))
                    | ((b2 >> shr_low) << (ffs_low & 0x1fu)));
                *dst++ = word;
            }
            src += job->src_stride - 3 * job->width;
            dst += (rec->row_bytes >> 1) - job->width;
        }
        surface_pixel_rows_flip_with_scratch(rec);
        return 1;
    }
    if (rec->depth == 24 || rec->depth == 32) return 0;
    return 1;
}
/* RGBChannelsPack (FUN_0047db90, findings 23/431): the corpus's CONCAT
 * chain drops the three bytes little-endian — argument one in bits 0-7,
 * two in 8-15, three in 16-23, top byte zero. Callers (the palette scan
 * of finding 23) feed the bytes in memory order, so the word is the
 * 0x00BBGGRR triple as stored. */
// FUNCTION: LEGORR 0x0047db90
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) uint32_t rgb_channels_pack(uint8_t red, uint8_t green,
                                             uint8_t blue)
{
    __asm {
        mov al, byte ptr [esp + 4]
        mov cl, byte ptr [esp + 8]
        mov dl, byte ptr [esp + 0x0c]
        mov byte ptr [esp + 4], al
        mov byte ptr [esp + 5], cl
        mov byte ptr [esp + 6], dl
        mov byte ptr [esp + 7], 0
        mov eax, dword ptr [esp + 4]
        ret
    }
}
#else
uint32_t rgb_channels_pack(uint8_t red, uint8_t green, uint8_t blue)
{
    return (uint32_t)red | ((uint32_t)green << 8) | ((uint32_t)blue << 16);
}
#endif

/* ResourceRecordAddRef (FUN_0047dbc0): the wrapped DirectDraw object's
 * SetColorKey(DDCKEY_SRCBLT, range) equivalent, then mark flag bit 1. */
// FUNCTION: LEGORR 0x0047dbc0
void resource_record_add_ref(void* record)
{
    if (record == NULL) return;
    TextureSurface* surface = (TextureSurface*)record;
    const uint32_t low = surface->key_count > 0 ? surface->keys[0] : 0;
    const uint32_t high = surface->key_count > 1 ? surface->keys[1] : low;
    texture_source_color_key_set(surface, low, high);
    surface->resource_flags |= 2u;
}

/* ImageSurfaceBlitTransformed (FUN_0047dbf0) converts the supplied channel
 * triples through the resource's pixel representation, installs the source
 * color-key range, and marks wrapper flag bit 1. The portable resource is a
 * decoded ARGB TextureSurface, so its equivalent key is stored as RGB24. */
// FUNCTION: LEGORR 0x0047dbf0
void image_surface_blit_transformed(TextureSurface* surface,
                                    float low_red, float low_green,
                                    float low_blue, float high_red,
                                    float high_green, float high_blue)
{
    uint32_t low;
    uint32_t high;
    if (surface == NULL) return;
    low = surface_image_key_color(low_red, low_green, low_blue);
    high = surface_image_key_color(high_red, high_green, high_blue);
    surface->source_key_low = low;
    surface->source_key_high = high;
    surface->source_key_set = 1;
    surface->resource_flags |= 2u;
}
/* DirectDrawSurfaceUnlock (FUN_0047ded0): forward a full-surface unlock to
 * the installed platform interface callback. */
// FUNCTION: LEGORR 0x0047ded0
void surface_directdraw_unlock(void* surface)
{
    if (s_surface_writer_unlock_hook != NULL) {
        s_surface_writer_unlock_hook(surface, 0);
    } else {
        surface_directdraw_unlock_call(surface);
    }
}

/* DirectDrawSurfacePixelRead (FUN_0047dfa0): read one in-bounds pixel from
 * the port's 32-bit locked display surface. The RGBA byte layout yields the
 * native little-endian packed word consumed by the renderer. */
// FUNCTION: LEGORR 0x0047dfa0
int surface_directdraw_pixel_read(DisplaySurface* surface, unsigned int x,
                                  unsigned int y, uint32_t* pixel_out)
{
    void* pixels = NULL;
    int pitch = 0;
    uint32_t pixel;
    if (surface == NULL || pixel_out == NULL || surface->w <= 0 ||
        surface->h <= 0 || x >= (unsigned int)surface->w ||
        y >= (unsigned int)surface->h || surface_lock(surface, &pixels,
                                                       &pitch) != 0 ||
        pixels == NULL || pitch < surface->w * 4) {
        return 0;
    }
    memcpy(&pixel, (const unsigned char*)pixels + (size_t)y * pitch +
           (size_t)x * 4u, sizeof(pixel));
    surface_unlock(surface, pixels);
    *pixel_out = pixel;
    return 1;
}

// FUNCTION: LEGORR 0x0047e130
void scene_object_registry_release_all(void)
{
    unsigned int level;
    size_t stride = scene_registry_record_stride();
    for (level = 0u; level < s_scene_registry_block_count; ++level) {
        unsigned int count = 1u << level;
        unsigned int i;
        unsigned char* block =
            (unsigned char*)s_scene_registry_blocks[level];
        for (i = 0u; i < count; ++i) {
            void* record = block + (size_t)i * stride;
            if (scene_registry_record_next_get(record) == record) {
                image_surface_release(record);
            }
        }
    }
}

// FUNCTION: LEGORR 0x0047e190
uint32_t font_device_color_convert(void* device, uint32_t color)
{
    uint32_t bits_per_pixel = 0u;
    uint32_t red_mask = 0u;
    uint32_t green_mask = 0u;
    uint32_t blue_mask = 0u;
    uint32_t red_bits;
    uint32_t green_bits;
    uint32_t blue_bits;
    uint32_t red_shift;
    uint32_t green_shift;
    uint32_t blue_shift;
    uint32_t result;
    uint32_t red = color & 0xffu;
    uint32_t green = (color >> 8) & 0xffu;
    uint32_t blue = (color >> 16) & 0xffu;
    if ((s_font_pixel_format_query != NULL
             ? s_font_pixel_format_query(device, &bits_per_pixel, &red_mask,
                                         &green_mask, &blue_mask)
             : font_device_pixel_format_query(device, &bits_per_pixel,
                                              &red_mask, &green_mask,
                                              &blue_mask)) != 0) {
        return 0u;
    }
    red_bits = (uint32_t)pop_count32(red_mask);
    green_bits = (uint32_t)pop_count32(green_mask);
    blue_bits = (uint32_t)pop_count32(blue_mask);
    red_shift = find_first_set_bit32(red_mask);
    green_shift = find_first_set_bit32(green_mask);
    blue_shift = find_first_set_bit32(blue_mask);
    result = ((red >> ((8u - red_bits) & 31u)) << (red_shift & 31u)) |
             ((green >> ((8u - green_bits) & 31u)) <<
              (green_shift & 31u)) |
             ((blue >> ((8u - blue_bits) & 31u)) << (blue_shift & 31u));
    if (bits_per_pixel < 32u) {
        result &= (1u << (bits_per_pixel & 31u)) - 1u;
    }
    if (s_font_pixel_format_query != NULL &&
        s_font_pixel_format_release != NULL) {
        s_font_pixel_format_release(device);
    }
    return result;
}

/* RGBChannelsUnpack (FUN_0047e2d0): optional outputs receive the packed
 * word's first three bytes, preserving the channel order used by the packer. */
// FUNCTION: LEGORR 0x0047e2d0
void rgb_channels_unpack(uint32_t color, uint8_t* red, uint8_t* green,
                         uint8_t* blue)
{
    if (red != NULL) {
        *red = (uint8_t)color;
    }
    if (green != NULL) {
        *green = (uint8_t)(color >> 8);
    }
    if (blue != NULL) {
        *blue = (uint8_t)(color >> 16);
    }
}

namespace unported_ledger_stubs {
// InterfaceMethod18StructApply
// FUNCTION: LEGORR 0x0047e300
int ledger_stub_0047e300_InterfaceMethod18StructApply(
    void* destination, const void* source, size_t size,
    void (*apply)(void* destination, const void* source, size_t size,
                  void* context), void* context)
{
    if (destination == NULL || source == NULL || size == 0u) return 0;
    if (apply != NULL) apply(destination, source, size, context);
    else memcpy(destination, source, size);
    return 1;
}

// InterfaceMethodResultRecordInitialize
// FUNCTION: LEGORR 0x0047e3e0
void ledger_stub_0047e3e0_InterfaceMethodResultRecordInitialize(
    unsigned int* result, void* device, unsigned int interface_value,
    void* context, unsigned int first_color, unsigned int second_color)
{
    if (result == NULL) return;
    if (s_small_record_free == NULL) {
        fixed_size_small_record_pool_grow();
    }
    result[1] = interface_value;
    result[7] = (unsigned int)(uintptr_t)result;
    result[6] = 0;
    result[2] = (unsigned int)(uintptr_t)context;
    result[0] = (unsigned int)(uintptr_t)device;
    result[5] = first_color;
    result[3] = font_device_color_convert(device, first_color);
    result[4] = font_device_color_convert(device, second_color);
}
} // namespace unported_ledger_stubs

static void surface_directdraw_unlock_call(void* surface)
{
    if (surface == NULL) return;
#if defined(LEGORRR_BACKEND_MINIWIN)
    (void)((LPDIRECTDRAWSURFACE)surface)->Unlock(NULL);
#elif defined(_WIN32)
    typedef long (__stdcall *SurfaceUnlockProc)(void*, void*);
    void** vtable = *(void***)surface;
    if (vtable == NULL || vtable[32] == NULL) return;
    SurfaceUnlockProc unlock = (SurfaceUnlockProc)vtable[32];
    (void)unlock(surface, NULL);
#else
    (void)surface;
#endif
}

/* FontDeviceColorConvert receives the raw DirectDraw surface interface used
 * by FontRecordCreate. The retail code locks it with a 0x7c-byte descriptor,
 * reads the RGB masks, and unlocks it. Keep that operation on the raw surface
 * here; the portable DisplaySurface wrapper is a different object contract. */
static int font_device_pixel_format_query(void* device,
                                         uint32_t* bits_per_pixel,
                                         uint32_t* red_mask,
                                         uint32_t* green_mask,
                                         uint32_t* blue_mask)
{
    if (device == NULL || bits_per_pixel == NULL || red_mask == NULL ||
        green_mask == NULL || blue_mask == NULL) return 1;
#if defined(LEGORRR_BACKEND_MINIWIN)
    {
        DDSURFACEDESC descriptor;
        HRESULT result;
        memset(&descriptor, 0, sizeof(descriptor));
        descriptor.dwSize = 0x7cu;
        result = ((IDirectDrawSurface*)device)->Lock(
            NULL, &descriptor, (DDLockFlags)1, NULL);
        if (result != DD_OK) return 1;
        *bits_per_pixel = descriptor.ddpfPixelFormat.dwRGBBitCount;
        *red_mask = descriptor.ddpfPixelFormat.dwRBitMask;
        *green_mask = descriptor.ddpfPixelFormat.dwGBitMask;
        *blue_mask = descriptor.ddpfPixelFormat.dwBBitMask;
        (void)((IDirectDrawSurface*)device)->Unlock(NULL);
        return 0;
    }
#elif defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
    {
        typedef HRESULT (SURFACE_COM_CALL *SurfaceLockFn)(
            void*, void*, void*, DWORD, HANDLE);
        typedef HRESULT (SURFACE_COM_CALL *SurfaceUnlockFn)(void*, void*);
        unsigned char descriptor[0x7c];
        void** vtable = surface_interface_vtable(device);
        SurfaceLockFn lock_surface;
        SurfaceUnlockFn unlock_surface;
        HRESULT result;
        DWORD descriptor_size = 0x7cu;
        if (vtable == NULL || vtable[25] == NULL || vtable[32] == NULL)
            return 1;
        lock_surface = (SurfaceLockFn)vtable[25];
        unlock_surface = (SurfaceUnlockFn)vtable[32];
        memset(descriptor, 0, sizeof(descriptor));
        memcpy(descriptor, &descriptor_size, sizeof(descriptor_size));
        result = lock_surface(device, NULL, descriptor, 1u, NULL);
        if (result != 0) return 1;
        memcpy(bits_per_pixel, descriptor + 0x54u, sizeof(*bits_per_pixel));
        memcpy(red_mask, descriptor + 0x58u, sizeof(*red_mask));
        memcpy(green_mask, descriptor + 0x5cu, sizeof(*green_mask));
        memcpy(blue_mask, descriptor + 0x60u, sizeof(*blue_mask));
        (void)unlock_surface(device, NULL);
        return 0;
    }
#else
    return 1;
#endif
}

static uint32_t surface_image_key_color(float red, float green, float blue)
{
    uint8_t r = (uint8_t)crt_float_to_long(red);
    uint8_t g = (uint8_t)crt_float_to_long(green);
    uint8_t b = (uint8_t)crt_float_to_long(blue);
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

static size_t scene_registry_record_stride(void)
{
    return (size_t)0x1cu + sizeof(void*);
}

static void* scene_registry_record_next_get(void* record)
{
    void* next;
    memcpy(&next, (unsigned char*)record + 0x1c, sizeof(next));
    return next;
}

static void scene_registry_record_next_set(void* record, void* next)
{
    memcpy((unsigned char*)record + 0x1c, &next, sizeof(next));
}
