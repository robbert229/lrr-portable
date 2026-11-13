// SPDX-License-Identifier: LGPL-3.0-or-later
/* Port tail (ADR 0006 decision 4) of the AVI player unit (G98-012,
 * finding 604). These are the port's models of the AVI player, which #496
 * relocated into avi_player.cpp with the unit's annotations. They do not
 * have the original's contract (AviPlaybackState is the port's record, and
 * the bodies run through the port's movie and surface layers and hooks), so
 * they keep their names here, unannotated. The original's code is
 * avi_player.cpp. Native movie playback does not call these: it runs
 * movie.cpp's FFmpeg reader directly. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "movie.hpp"
#include "engine.hpp"
#include "resource.hpp"
#include "string.hpp"
#include "surface.hpp"
#include "heap.hpp"
#include "platform/platform.h"
#if defined(LEGORRR_BACKEND_WINAPI)
#include <windows.h>
#endif
#if defined(LEGORRR_BACKEND_MINIWIN)

#include <dlfcn.h>
#include <dirent.h>
#include <sys/stat.h>

#endif
#include <math.h>
#include <float.h>
#include <ctype.h>
#include <errno.h>
#include "gods98/texture.hpp"
#include "crt/crt.hpp"
#include "spawn.hpp"
#include "port_loop_ports.hpp"
#include "game/entity.hpp"
#include "level.hpp"
#include "cmd.hpp"
#include "select.hpp"
#include "terrain_grid.hpp"
#include "type_config_ports.hpp"
#include "ai.hpp"
#include "gods98/view.hpp"
#include "gods98/record.hpp"
#include "gods98/scene_resources.hpp"
#include "../gods98/message.hpp"
#include "../gods98/camera.hpp"
#include "sound.hpp"
#include "game/util.hpp"
#include <limits.h>
#include "model.hpp"
#include "frameloop.hpp"
#include "surface_for_images_unit.hpp"
#include "movie_for_avi_player.hpp"
#include "crt/crt_for_avi_player.hpp"

uint8_t s_avi_playback_disabled;

static void* s_avi_file_context;

static int movie_surface_raw_dib_compatible(AviPlaybackState* state,
                                            const MovieOut* frame);
static int movie_surface_indexed_dib_copy(AviPlaybackState* state,
                                          const MovieOut* frame);

/* Defined below (forward declarations added by tools/relocate_unit.py). */
static uint32_t avi_clock_milliseconds(void);
static uint16_t movie_read_u16le(const uint8_t* bytes);
static uint32_t movie_read_u32le(const uint8_t* bytes);

int AviPlaybackState::display_mode_setup(uint32_t width, uint32_t height,
                                         uint32_t bits_per_pixel,
                                         int fullscreen)
{
    display_width = width != 0u ? width : display_width;
    display_height = height != 0u ? height : display_height;
    display_bits_per_pixel = bits_per_pixel;
    display_fullscreen = fullscreen != 0;
    if (s_avi_display_mode_hook == NULL) {
        DisplaySurface* surface;
        if (surface_object != NULL)
            return surface_object != NULL;
        surface = (DisplaySurface*)calloc(1, sizeof(DisplaySurface));
        if (surface == NULL) return 0;
        /* The WinAPI surface adapter builds the requested DirectDraw pixel
         * format, but creates its own DirectDraw object and does not set its
         * cooperative level. Miniwin creates an offscreen surface and uses
         * the HWND, but ignores pixel-format/fullscreen selection and the
         * retail 8-bit mode enumeration. Sharing DAT_0053399c and its full
         * display-mode behavior requires the absent renderer owner or the
         * AviDisplayModeHook. */
        if (surface_create_display_mode(surface, display_device,
                (int)width, (int)height, (int)bits_per_pixel,
                fullscreen) != 0) {
            free(surface);
            return 0;
        }
        surface_object = surface;
        display_width = (uint32_t)surface->w;
        display_height = (uint32_t)surface->h;
        surface_owned = 1u;
        /* DirectDrawDisplayModeSetup calls AviSurfaceLock followed by
         * AviFrameSurfaceUnlock after CreateSurface, even when the requested
         * dimensions were already supplied. The result is discarded; this
         * also verifies the newly-created AVI surface through the same
         * backend path used during frame updates. */
        (void)surface_lock();
        (void)surface_unlock();
        return 1;
    }
    return s_avi_display_mode_hook(s_avi_display_mode_context, this,
        width, height, bits_per_pixel, fullscreen, display_device) != 0;
}

/* DirectDrawSurfaceDescriptorInitialize (FUN_0047e710): clear the
 * descriptor storage at +0x1c and restore DDSURFACEDESC.dwSize. */
void surface_descriptor_initialize(void* record)
{
    unsigned char* bytes = (unsigned char*)record;
    if (bytes == NULL) return;
    memset(bytes + 0x1c, 0, 0x7c);
    *(uint32_t*)(bytes + 0x1c) = 0x7cu;
    *(uint32_t*)(bytes + 0x18) = 0u;
    *(uint32_t*)(bytes + 0x10) = 0u;
    bytes[0x08] = 0u;
    bytes[0x14] = 0u;
}

uint32_t AviPlaybackState::surface_lock()
{
    uint32_t result;
    void* pixels = NULL;
    int pitch = 0;
    int lock_result;
    if (surface_locked != 0) return 1u;
    if (s_avi_surface_lock_hook != NULL) {
        result = s_avi_surface_lock_hook(s_avi_surface_lock_context,
                                         surface_object, 1, 0x801u);
    } else {
        lock_result = ::surface_lock((DisplaySurface*)surface_object,
                                     &pixels, &pitch);
        result = lock_result == 0 ? 0u
            : (uint32_t)lock_result & 0xffffff00u;
        (void)pitch;
    }
    if (result == 0u) {
        surface_locked = 1u;
        surface_pixels = pixels;
        surface_pitch = pitch;
        return 1u;
    }
    return result;
}

uint32_t AviPlaybackState::surface_unlock()
{
    uint32_t result;
    if (surface_locked == 0) return 0u;
    if (s_avi_surface_lock_hook != NULL) {
        result = s_avi_surface_lock_hook(s_avi_surface_lock_context,
                                         surface_object, 0, 0u);
    } else {
        ::surface_unlock((DisplaySurface*)surface_object, surface_pixels);
        result = 0u;
    }
    if (result == 0u) {
        surface_locked = 0u;
        surface_pixels = NULL;
        surface_pitch = 0;
        return 1u;
    }
    return result;
}

int LEGORRR_CRT_STDCALL bit_count32_by_position(uint32_t value)
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

void AviPlaybackState::frame_retriever_open(const char* path)
{
    Movie* movie;
    MovieInfo info;
    uint32_t now = 0;
    frame_ready = 0;
    surface_locked = 0;
    surface_pixels = NULL;
    surface_pitch = 0;
    frame_retriever_object = NULL;
    stream_object = NULL;
    surface_object = NULL;
    surface_owned = 0u;
    stream_frame_count = 0;
    frame_index = 0;
    fps_numerator = 0u;
    fps_denominator = 0u;
    frame_rate = 0.0f;
    frame_clock = 0.0f;
    frame_delta = 0.0f;
    display_width = 0u;
    display_height = 0u;
    stream_width = 0u;
    stream_height = 0u;
    if (path == NULL || s_avi_playback_disabled) return;
    movie = movie_avi_retriever_open(path);
    if (movie == NULL) return;
    /* Retail stores the AVI stream as soon as it opens. A later frame
     * retriever or AVIStreamInfo failure leaves it for the object's destructor
     * to release. The Movie wrapper owns both VFW handles on WinAPI. */
    stream_object = movie;
    if (movie_info(movie, &info) != 0) {
        return;
    }
    fps_numerator = (uint32_t)info.fps_num;
    fps_denominator = (uint32_t)info.fps_den;
    stream_width = (uint32_t)info.width;
    stream_height = (uint32_t)info.height;
    stream_frame_count = movie_frame_count(movie);
    frame_index = 0;
    now = avi_clock_milliseconds();
    frame_delta = (float)now * 0.001f;
    frame_clock = 0.0f;
    /* The reference marks the retriever ready before allocating its
     * surface.  A failed surface allocation therefore leaves a ready
     * retriever with a null surface, matching the staged constructor. */
    frame_ready = 1u;
    (void)display_mode_setup((uint32_t)info.width,
                             (uint32_t)info.height, 0u, 0);
}

void AviPlaybackState::player_destroy(uint8_t flags)
{
    int release_player;
    frame_retriever_destroy();
    release_player = (flags & 1u) != 0u && player_owned != 0u;
    if (release_player) engine_heap_free(this);
}
void AviPlaybackState::frame_retriever_destroy()
{
    if (s_avi_playback_disabled) return;
    if (stream_object != NULL) {
        if (s_avi_release_hook != NULL)
            s_avi_release_hook(s_avi_release_context, stream_object);
        else
            movie_close((Movie*)stream_object);
    }
    stream_object = NULL;
    frame_retriever_object = NULL;
    frame_ready = 0u;
    if (surface_object != NULL) {
        if (surface_owned != 0u) {
            surface_destroy((DisplaySurface*)surface_object);
            free(surface_object);
        }
        else if (s_avi_display_release_hook != NULL) {
            s_avi_display_release_hook(s_avi_release_context);
        }
    }
    surface_object = NULL;
    surface_owned = 0u;
    surface_locked = 0u;
    surface_pixels = NULL;
    surface_pitch = 0;
}

void AviPlaybackState::frame_decode_and_copy(const uint8_t* dib)
{
    DisplaySurface* surface = (DisplaySurface*)surface_object;
    int acquired = 0;
    int width = (int)stream_width;
    int height = (int)stream_height;
    int row_bytes;
    uint32_t header_bytes;
    uint32_t source_pitch;
    uint32_t surface_bits_per_pixel = 0u;
    SurfacePixelFormat surface_format;
    const uint8_t* source;
    uint8_t* target;
    int y;

    if (dib == NULL || surface == NULL) return;
    if (surface_locked == 0u) {
        if (surface_lock() == 0u) return;
        acquired = 1;
    }
    if (surface_pixels == NULL || surface_pitch <= 0) goto done;
    memset(&surface_format, 0, sizeof(surface_format));
    if ((surface->format_flags & 1u) != 0u) {
        surface_bits_per_pixel = 15u;
    }
    else if (surface_directdraw_pixel_format_get(surface,
                                                  &surface_format)) {
        surface_bits_per_pixel = surface_format.bits_per_pixel;
    }
    if (surface_bits_per_pixel != 15u && surface_bits_per_pixel != 16u)
        goto done;
    if (width <= 0 || height <= 0) goto done;
    header_bytes = (uint32_t)dib[0] | ((uint32_t)dib[1] << 8)
        | ((uint32_t)dib[2] << 16) | ((uint32_t)dib[3] << 24);
    row_bytes = width * 2;
    if (header_bytes < 40u || surface_pitch < row_bytes) goto done;
    /* Retail steps the frame source using the locked DirectDraw pitch, just
     * as it steps the destination. AVIStreamGetFrame's selected output is
     * expected to have that same row stride. */
    source_pitch = (uint32_t)surface_pitch;
    source = dib + header_bytes + (size_t)(height - 1) * source_pitch;
    target = (uint8_t*)surface_pixels;
    for (y = 0; y < height; ++y) {
        memcpy(target, source, (size_t)row_bytes);
        target += surface_pitch;
        source -= source_pitch;
    }
done:
    if (acquired) (void)surface_unlock();
}
int AviPlaybackState::frame_update_and_blit()
{
    int frame_updated = 0;
    if (frame_ready == 0 || s_avi_playback_disabled) {
        return 0;
    }
    if (s_avi_frame_update_hook != NULL) {
        frame_updated = s_avi_frame_update_hook(s_avi_frame_hook_context,
                                                this) != 0;
    } else {
        MovieOut frame;
        Movie* movie = (Movie*)stream_object;
        if (movie == NULL) return 1;
        if (!movie_seek_frame(movie, frame_index)) return 1;
        if (movie_next(movie, &frame) != 0) {
            /* AVIStreamGetFrame returning null leaves the old surface and
             * index intact; the caller still performs its normal blit. */
            return 1;
        }
        frame_updated = 1;
        if (movie_surface_raw_dib_compatible(this, &frame))
            frame_decode_and_copy(frame.dib);
        else if (frame.dib_bits_per_pixel == 8)
            (void)movie_surface_indexed_dib_copy(this, &frame);
        else
            frame_copy_rgba(&frame);
    }
    if (frame_updated) {
        uint32_t now = avi_clock_milliseconds();
        frame_rate = fps_denominator != 0u
            ? (float)fps_numerator / (float)fps_denominator
            : 0.0f;
        frame_clock = (float)now * 0.001f;
        if (frame_rate != 0.0f) {
            float next_frame = (frame_clock - frame_delta) * frame_rate;
            if (next_frame >= 0.0f)
                frame_index = (uint32_t)next_frame;
        }
    }
    return 1;
}
uint32_t AviPlaybackState::stream_frame_count_get() const
{
    if (s_avi_playback_disabled) return 0u;
    if (stream_frame_count != 0u) return stream_frame_count;
    return movie_frame_count((Movie*)stream_object);
}

void AviPlaybackState::frame_seek_and_clock_rebase(uint32_t frame_index_value)
{
    uint32_t now;
    if (s_avi_playback_disabled) return;
    frame_index = frame_index_value;
    movie_seek_frame((Movie*)stream_object, frame_index_value);
    now = avi_clock_milliseconds();
    frame_clock = (float)now * 0.001f;
    frame_delta = frame_rate != 0.0f
        ? frame_clock - (float)frame_index / frame_rate : frame_clock;
}
uint32_t AviPlaybackState::playback_frame_index_get() const
{
    return frame_index &
        ~(uint32_t)(0u - (uint32_t)(s_avi_playback_disabled != 0));
}
void avi_file_initialize(void* context)
{
    s_avi_file_context = context;
    /* Retail DAT_0076ac84 is the DirectDraw interface used by the surface
     * description path as well as the AVI display-mode code. */
    surface_description_interface_set(context);
    movie_avi_file_initialize();
}

AviPlaybackState* avi_player_create(const char* path)
{
    AviPlaybackState* state = (AviPlaybackState*)engine_heap_alloc(
        (long)sizeof(*state));
    DisplaySurface* render_target;
    if (state == NULL) return NULL;
    memset(state, 0, sizeof(*state));
    /* Retail allocates 0xc4 bytes before invoking the frame-retriever
     * constructor. This portable record adds backend sidecars, so use its
     * full size and zero those sidecars before the constructor fills the
     * fields modeled at retail offsets. */
    state->player_owned = 1u;
    /* The retail DirectDraw global supplies the cooperative-level owner
     * implicitly. The miniwin surface adapter needs its HWND explicitly. */
    render_target = surface_active_render_target_get();
    if (render_target != NULL) state->display_device = render_target->hwnd;
    state->frame_retriever_open(path);
    return state;
}

/* Portable frame blit adapter. Retail 0x0047ed00 reads its source surface
 * from a renderer-owned player record at +0xc0/+0x18; AviPlaybackState keeps
 * its portable DisplaySurface pointer in a host sidecar instead. */
void avi_playback_frame_loop(AviPlaybackState* state, void* destination)
{
    if (state == NULL || state->frame_ready == 0) {
        return;
    }
    if (s_avi_frame_loop_hook != NULL) {
        s_avi_frame_loop_hook(s_avi_frame_hook_context, state, destination);
    } else if (destination != NULL && state->surface_object != NULL) {
        surface_blit_wait(destination, (DisplaySurface*)state->surface_object);
    }
}

namespace lego {
void interface_invoke_one_if_nonnull(void* object)
{
    if (object != 0) {
        void** vtable = *static_cast<void***>(object);
        LegacyInterfaceInvoke invoke =
            reinterpret_cast<LegacyInterfaceInvoke>(vtable[0]);
#if defined(_MSC_VER)
        invoke(object, 0, 1);
#else
        invoke(object, 1);
#endif
    }
}
} // namespace lego

/* AviPlayerReadyGet (FUN_0047ed80, findings 185/414): the readiness
 * byte at +0x99 (byte-safe — the corpus reads it as a raw byte). */
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) uint32_t avi_player_ready_get(const void* movie)
{
    __asm {
        mov ecx, dword ptr [esp + 4]
        xor eax, eax
        mov al, byte ptr [ecx + 99h]
        ret
    }
}
#else
uint32_t avi_player_ready_get(const void* movie)
{
    if (movie != NULL) {
        uint8_t v = 0;
        __builtin_memcpy(&v, (const char*)movie + 0x99, sizeof(v));
        return v;
    }
    return 0u;
}
#endif

static uint32_t avi_clock_milliseconds(void)
{
    if (s_avi_time_hook != NULL)
        return s_avi_time_hook(s_avi_time_context);
    return platform_time_milliseconds();
}

static int movie_surface_raw_dib_compatible(AviPlaybackState* state,
                                            const MovieOut* frame)
{
    DisplaySurface* surface = state != NULL
        ? (DisplaySurface*)state->surface_object : NULL;
    SurfacePixelFormat format;
    uint32_t header_size;
    uint16_t bits;
    uint32_t compression;
    int acquired = 0;
    int compatible = 0;
    if (frame == NULL || frame->dib == NULL || surface == NULL ||
        (frame->dib_bits_per_pixel != 15 &&
         frame->dib_bits_per_pixel != 16)) return 0;
    header_size = movie_read_u32le(frame->dib);
    if (header_size < 40u || header_size > 4096u) return 0;
    bits = movie_read_u16le(frame->dib + 14u);
    compression = movie_read_u32le(frame->dib + 16u);
    if ((bits != 15u && bits != 16u) || compression != 0u ||
        movie_read_u32le(frame->dib + 32u) != 0u ||
        frame->dib_header_bytes != (int)header_size) return 0;
    if (state->surface_locked == 0u) {
        if (state->surface_lock() == 0u) return 0;
        acquired = 1;
    }
    if (state->surface_pixels != NULL && state->surface_pitch > 0 &&
        frame->dib_pitch == state->surface_pitch) {
        if ((surface->format_flags & 1u) != 0u) {
            compatible = 1;
        }
        else {
            memset(&format, 0, sizeof(format));
            if (surface_directdraw_pixel_format_get(surface, &format) &&
                format.bits_per_pixel == 16u &&
                format.red_mask == 0x7c00u &&
                format.green_mask == 0x03e0u &&
                format.blue_mask == 0x001fu) compatible = 1;
        }
    }
    if (acquired) (void)state->surface_unlock();
    return compatible;
}

/* VfW returns an RGBQUAD palette beside 8-bit frame indices. Copy that table
 * into the destination's owned DirectDraw palette and copy the indexed rows
 * without expanding them. The frame memory belongs to AVIStreamGetFrame and
 * is only valid until the next retrieval, so both operations happen here. */
static int movie_surface_indexed_dib_copy(AviPlaybackState* state,
                                          const MovieOut* frame)
{
    DisplaySurface* surface = state != NULL
        ? (DisplaySurface*)state->surface_object : NULL;
    SurfacePixelFormat format;
    SurfacePaletteEntry entries[256];
    const uint8_t* source;
    uint8_t* target;
    uint32_t header_size;
    uint32_t compression;
    int32_t dib_height;
    unsigned int index;
    int width, height, x, y;
    int acquired = 0;
    if (state == NULL || frame == NULL || surface == NULL ||
        frame->dib == NULL || frame->dib_palette == NULL ||
        frame->dib_bits_per_pixel != 8 ||
        frame->dib_palette_entries == 0u ||
        frame->dib_palette_entries > 256u || frame->dib_pitch <= 0 ||
        frame->dib_header_bytes < 40 ||
        state->stream_width == 0u || state->stream_height == 0u ||
        state->stream_width > 8192u || state->stream_height > 8192u ||
        state->display_width == 0u || state->display_height == 0u ||
        state->display_width > 8192u || state->display_height > 8192u)
        return 0;
    header_size = movie_read_u32le(frame->dib);
    compression = movie_read_u32le(frame->dib + 16u);
    dib_height = (int32_t)movie_read_u32le(frame->dib + 8u);
    if (header_size < 40u || header_size > 4096u ||
        compression != 0u ||
        movie_read_u16le(frame->dib + 14u) != 8u ||
        frame->dib_header_bytes < (int)header_size ||
        (dib_height < 0 ? -(int64_t)dib_height : (int64_t)dib_height)
            != (int64_t)state->stream_height ||
        frame->dib_pitch < (int)state->stream_width ||
        surface->palette == NULL)
        return 0;
    memset(&format, 0, sizeof(format));
    if (!surface_directdraw_pixel_format_get(surface, &format) ||
        format.bits_per_pixel != 8u) return 0;
    for (index = 0u; index < frame->dib_palette_entries; ++index) {
        entries[index].red = frame->dib_palette[index * 4u + 2u];
        entries[index].green = frame->dib_palette[index * 4u + 1u];
        entries[index].blue = frame->dib_palette[index * 4u + 0u];
        entries[index].flags = 0u;
    }
    if (!surface_palette_entries_set(surface, 0u,
                                     frame->dib_palette_entries, entries))
        return 0;
    width = (int)state->display_width;
    height = (int)state->display_height;
    if (state->surface_locked == 0u) {
        if (state->surface_lock() == 0u) return 0;
        acquired = 1;
    }
    if (state->surface_pixels == NULL || state->surface_pitch < width)
        goto done;
    source = frame->dib + frame->dib_header_bytes;
    target = (uint8_t*)state->surface_pixels;
    for (y = 0; y < height; ++y) {
        uint32_t source_y = (uint32_t)(((uint64_t)(uint32_t)y *
            state->stream_height) / (uint32_t)height);
        uint32_t dib_y = dib_height < 0
            ? source_y : state->stream_height - 1u - source_y;
        const uint8_t* source_row = source +
            (size_t)dib_y * (size_t)frame->dib_pitch;
        uint8_t* target_row = target + (size_t)y *
            (size_t)state->surface_pitch;
        for (x = 0; x < width; ++x) {
            uint32_t source_x = (uint32_t)(((uint64_t)(uint32_t)x *
                state->stream_width) / (uint32_t)width);
            target_row[x] = source_row[source_x];
        }
    }
    if (acquired) (void)state->surface_unlock();
    return 1;
done:
    if (acquired) (void)state->surface_unlock();
    return 0;
}
static uint32_t movie_read_u32le(const uint8_t* bytes)
{
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) |
        ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
}

static uint16_t movie_read_u16le(const uint8_t* bytes)
{
    return (uint16_t)((uint16_t)bytes[0] |
        ((uint16_t)bytes[1] << 8));
}
