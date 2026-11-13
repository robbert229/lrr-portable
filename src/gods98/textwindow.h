// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 TextWindow (original unit G98-017) entry points that units rebuilt
 * as C call, by the port's names (gods98/heap.hpp, gods98/record.hpp). In the
 * matching build textwindow.c defines them (textwindow_link_names.h,
 * finding 600); natively the port tail text_window_port.cpp does.
 *
 * C boundary header (ADR 0005). The C++ declarations carry extern "C";
 * src/game/test_c_boundaries.cpp includes them with this header. Shared C++
 * headers do not include this file (finding 506). */
#ifndef LEGORRR_GODS98_TEXTWINDOW_H
#define LEGORRR_GODS98_TEXTWINDOW_H

#include <stdarg.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* FUN_004850f0 TextWindow_Create(font, window area, buffer size). The port
 * takes the area as four 32-bit words. */
void* text_buffer_create(uintptr_t font_handle, const uint32_t* header,
                         long size);
/* FUN_00485250 TextWindow_VPrintF. */
void text_buffer_append_vformat(void* record, int skip_bells,
                                const char* format, va_list args);
/* FUN_00485390 TextWindow_Update. */
int text_buffer_draw_wrapped(void* record, uint32_t start_line,
                             float line_scale, int* height_out);
/* FUN_004859b0 TextWindow_Clear. */
void text_buffer_reset_target(void* record);
/* FUN_004851b0 TextWindow_ChangePosition. */
void float_pair_at_offset4_set_from_ints(void* record, int x, int y);
/* FUN_00485a00 TextWindow_GetInfo. */
void object_state_pair_get_target(const void* record, uint32_t* out_a,
                                  uint32_t* out_b);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_TEXTWINDOW_H */
