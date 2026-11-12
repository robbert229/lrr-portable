// SPDX-License-Identifier: LGPL-3.0-or-later
/* Globals that the port keeps in crt.cpp and that units rebuilt as C read.
 *
 * C boundary header (ADR 0005): C declarations, with C linkage, of the
 * crt.hpp globals that units rebuilt as C use. The C++ code keeps using
 * crt/crt.hpp, whose matching declarations carry extern "C";
 * src/game/test_c_boundaries.cpp includes both and fails to compile if they
 * disagree. Shared C++ headers do not include this file: every include and
 * declaration a header adds shifts MSVC's symbol numbering (finding 506). */
#ifndef LEGORRR_CRT_CRT_H
#define LEGORRR_CRT_CRT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* DAT_0076ac8c, a common symbol: the DirectDraw back buffer
 * (DirectDraw_bSurf). The port holds it as a word; the Win32 renderer
 * stores the surface pointer there. */
extern uint32_t s_render_surface_secondary;

/* DAT_00505924 and DAT_00505928: Main.c's window width and height. */
extern int s_display_w;
extern int s_display_h;

/* DAT_00505f84: Main.c's flags. */
extern uint32_t s_engine_feature_word;

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_CRT_CRT_H */
