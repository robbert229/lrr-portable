// SPDX-License-Identifier: LGPL-3.0-or-later
/* G98-017 TextWindow.c (finding 600): the port's link names of the unit's
 * functions (as findings 590, 592 and 597).
 *
 * In the matching build (MSVC 5.0, _MSC_VER 1100) every caller of these
 * names passes the original's arguments and a record with the original's
 * 0x830-byte layout: on a 32-bit build the port's text-buffer record is that
 * layout. So the unit's definitions take the port's names, and the C
 * callers (helpwindow.c, lego_unit.c, panel_unit.c, reward.c) and the C++
 * ones (frontend.cpp, frontend_unit.cpp, objective_unit.cpp, text.cpp,
 * heap.cpp, record.cpp) reach them with no caller edit. The port tail
 * text_window_port.cpp compiles its definitions of these names out of the
 * matching build (LRR_TEXT_WINDOW_PORT_NAMES).
 *
 * Natively the unit keeps its own names and nothing calls it: the port's
 * text windows draw through its font hooks and keep a host-width font
 * sidecar, which the unit's record does not have.
 *
 * Included only by textwindow.c, before any other header. A macro renames a
 * definition one for one, so MSVC's symbol numbering (finding 538) does not
 * change. */
#ifndef LEGORRR_GODS98_TEXTWINDOW_LINK_NAMES_H
#define LEGORRR_GODS98_TEXTWINDOW_LINK_NAMES_H

#if defined(_MSC_VER) && _MSC_VER < 1200
#define text_window_create text_buffer_create /* 004850f0 */
#define text_window_enable_centering text_output_field82c_bit2_set_clear /* 00485160 */
#define text_window_remove text_buffer_destroy_target /* 00485190 */
#define text_window_change_position float_pair_at_offset4_set_from_ints /* 004851b0 */
#define text_window_change_size float_pair_at_offsetc_set_from_uints /* 004851d0 */
#define text_window_page_printf formatted_text_append_to_ring_buffer /* 00485210 */
#define text_window_printf text_buffer_append_format /* 00485230 */
#define text_window_vprintf text_buffer_append_vformat /* 00485250 */
#define text_window_printf_overlay objective_advisor_text_copy /* 00485300 */
#define text_window_update text_buffer_draw_wrapped /* 00485390 */
#define text_window_update_overlay text_wrap_measure_and_draw_lines /* 00485710 */
#define text_window_clear text_buffer_reset_target /* 004859b0 */
#define text_window_get_info object_state_pair_get_target /* 00485a00 */
#endif

#endif /* LEGORRR_GODS98_TEXTWINDOW_LINK_NAMES_H */
