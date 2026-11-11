// SPDX-License-Identifier: LGPL-3.0-or-later
/* Native back buffer for draw.c (G98-019, finding 519).
 *
 * The original draws on the DirectDraw back buffer held in the common word
 * at 0076ac8c. The port keeps that word as a uint32_t (crt.hpp), which
 * cannot hold a 64-bit pointer, and the native port has no DirectDraw back
 * buffer. draw.c therefore reads draw_port_back_surface in the native
 * build. The default surface fails GetSurfaceDesc and Lock, so the clip
 * window stays as it was and nothing is drawn, as before the conversion.
 * Tests install their own surface. */
#if defined(LEGORRR_BACKEND_MINIWIN)
#include <stddef.h>

#include "draw.h"

static const long kDrawPortSurfaceLost = (long)0x887601c2; /* DDERR_SURFACELOST */

static long draw_port_get_surface_desc(DrawSurface* surf,
                                       DrawSurfaceDesc* desc)
{
    (void)surf;
    (void)desc;
    return kDrawPortSurfaceLost;
}

static long draw_port_lock(DrawSurface* surf, void* rect,
                           DrawSurfaceDesc* desc, uint32_t flags, void* event)
{
    (void)surf;
    (void)rect;
    (void)desc;
    (void)flags;
    (void)event;
    return kDrawPortSurfaceLost;
}

static long draw_port_unlock(DrawSurface* surf, void* rect)
{
    (void)surf;
    (void)rect;
    return kDrawPortSurfaceLost;
}

static const DrawSurfaceVtbl s_draw_port_no_surface_vtbl = {
    {}, draw_port_get_surface_desc, {}, draw_port_lock, {}, draw_port_unlock
};
static DrawSurface s_draw_port_no_surface = { &s_draw_port_no_surface_vtbl };

extern "C" {
DrawSurface* draw_port_back_surface = &s_draw_port_no_surface;
}
#endif
