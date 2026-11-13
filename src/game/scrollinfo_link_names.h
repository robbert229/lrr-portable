// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-039 ScrollInfo.c (finding 611): the port's link names of the unit's
 * functions (as findings 590, 597, 600 and 607).
 *
 * In the matching build (MSVC 5.0, _MSC_VER 1100) every caller of these
 * names passes the original's arguments, so the unit's definitions take
 * the port's names: Panel.c (panel_unit.c, info.h), Info.c (info_unit.c,
 * info_api.h) and Lego.c (lego_unit.c, lego_unit_imports.h) reach them
 * with no caller edit. The port tail scrollinfo_port.cpp compiles its
 * definitions of these names out of the matching build
 * (LRR_SCROLLINFO_PORT_NAMES). Every port name here has C linkage.
 *
 * ScrollInfo_AddCreateStructure (00463810) keeps the unit's name: the
 * port's name is defined in gods98/resource.cpp, so info.h points Panel.c
 * at the unit's name in the matching build instead. The two mouse-over
 * tests (00463c00, 00463c90) keep theirs: only the unit calls them.
 *
 * Included only by scrollinfo.c, before any other header. A macro renames
 * a definition one for one, so MSVC's symbol numbering (finding 538) does
 * not change. */
#ifndef LEGORRR_GAME_SCROLLINFO_LINK_NAMES_H
#define LEGORRR_GAME_SCROLLINFO_LINK_NAMES_H

#if defined(_MSC_VER) && _MSC_VER < 1200
#define scroll_info_initialise info_panel_initialise             /* 004636b0 */
#define scroll_info_set_xy_offset indexed_float_pair_set         /* 00463730 */
#define scroll_info_add_structure scroll_rect_resource_record_create /* 00463770 */
#define scroll_info_update info_panel_check_collision            /* 004638a0 */
#define scroll_info_draw view_blit_rect_positions_apply          /* 00463d20 */
#define scroll_info_round float_round_to_nearest_int             /* 00463e40 */
#define scroll_info_get_range indexed_text_metrics_pair_get      /* 00463ea0 */
#define scroll_info_set_range indexed_text_metrics_pair_set      /* 00463ed0 */
#endif

#endif /* LEGORRR_GAME_SCROLLINFO_LINK_NAMES_H */
