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
#include "draw.h"
#include "draw_port.hpp"

static DrawPortLostSurface s_draw_port_no_surface;

extern "C" {
LPDIRECTDRAWSURFACE4 draw_port_back_surface = &s_draw_port_no_surface;
}
#endif
