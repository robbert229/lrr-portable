// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what images_unit uses from surface.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GODS98_SURFACE_FOR_IMAGES_UNIT_HPP
#define LRR_GODS98_SURFACE_FOR_IMAGES_UNIT_HPP

#if defined(_MSC_VER)
#define SURFACE_COM_CALL __stdcall
#elif defined(__i386__) && (defined(__GNUC__) || defined(__clang__))
#define SURFACE_COM_CALL __attribute__((stdcall))
#else
#define SURFACE_COM_CALL
#endif

typedef struct RendererRecordTableState {
    uint32_t words[32];
    uint32_t reserved[2];
    uint32_t enabled;
} RendererRecordTableState;

void** surface_interface_vtable(void* object);
extern FontPixelFormatQueryFn s_font_pixel_format_query;
extern FontPixelFormatReleaseFn s_font_pixel_format_release;
extern SurfaceWriterUnlockHook s_surface_writer_unlock_hook;

#endif /* LRR_GODS98_SURFACE_FOR_IMAGES_UNIT_HPP */
