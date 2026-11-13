// SPDX-License-Identifier: LGPL-3.0-or-later
/* G98-008 Font.c (finding 605): the link names of the unit's functions (as
 * findings 590, 592, 597 and 600).
 *
 * In the matching build (MSVC 5.0, _MSC_VER 1100) the unit's definitions
 * take the port's names wherever every caller of that name passes the
 * original's arguments: the C units (helpwindow.c, tooltip.c, textwindow.c,
 * panel_unit.c, reward.c, lego_unit.c, info_unit.c, interface_unit.c,
 * advisor.c) and the C++ callers of Font_Load, Font_Remove and
 * Font_GetHeight then reach the unit with no caller edit. The port tail
 * bitmap_font_port.cpp compiles its definitions of these names out of the
 * matching build (LRR_FONT_PORT_NAMES).
 *
 * Natively the port tail serves those names, with its software-surface
 * fonts, and the unit takes names of its own that nothing calls.
 *
 * Included only by font.c, before any other header. A macro renames a
 * definition one for one, so MSVC's symbol numbering (finding 538) does not
 * change. */
#ifndef LEGORRR_GODS98_FONT_LINK_NAMES_H
#define LEGORRR_GODS98_FONT_LINK_NAMES_H

#if defined(_MSC_VER) && _MSC_VER < 1200
#define font_load font_atlas_record_create /* 00479ee0 */
#define font_get_char_width bitmap_font_glyph_measure_width /* 0047a520 */
#define font_get_height object_field_be4_dword_get /* 0047a540 */
#define font_remove font_atlas_record_release /* 0047a550 */
#else
#define font_v_get_string_info font_unit_v_get_string_info /* 0047a150 */
#define font_print_f font_unit_print_f /* 0047a180 */
#define font_v_print_f font_unit_v_print_f /* 0047a1b0 */
#define font_output_char font_unit_output_char /* 0047a470 */
#endif

#endif /* LEGORRR_GODS98_FONT_LINK_NAMES_H */
