// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-018 HelpWindow.c (finding 595): the port's link names of the unit's
 * functions, for the matching build only (as findings 581, 585 and 590).
 *
 * In the matching build every caller of these names uses the original's
 * contract: lego_unit.c (Lego.c) and legoobject_unit.c (LegoObject.c), and
 * the C++ callers of building_upgrade_state_changed_dispatch (lego_port.cpp
 * and legoobject_port.cpp), which pass the same object type, index and
 * level. So the unit's definitions take the port's names, and the port
 * tail helpwindow_port.cpp compiles its own definitions of these names out
 * of the matching build (LRR_HELPWINDOW_PORT_NAMES). Natively the port tail
 * keeps serving them, over the port's own help-window model.
 *
 * help_window_initialise_adapter was the PORT-ONLY original-ABI adapter
 * for HelpWindow_Initialise (004181b0) that lego_unit.c calls.
 *
 * Included only by helpwindow.c, before any other header. A macro renames
 * a declaration or a definition one for one, so MSVC's symbol numbering
 * (finding 538) does not change. The other twelve functions keep the
 * unit's names: their only callers outside the unit are port models with
 * another contract, or tests of them. */
#ifndef LEGORRR_GAME_HELPWINDOW_LINK_NAMES_H
#define LEGORRR_GAME_HELPWINDOW_LINK_NAMES_H

#if defined(_MSC_VER) && _MSC_VER < 1200
#define help_window_set_font selection_mode_state_set /* 00418190 */
#define help_window_hide selection_state_flag_clear_bit0 /* 004181a0 */
#define help_window_initialise help_window_initialise_adapter /* 004181b0 */
#define help_window_show_text mode_flag_pair_apply_if_bit2 /* 00418800 */
#define help_window_recall_dependencies building_upgrade_state_changed_dispatch /* 00418830 */
#define help_window_check_buttons help_window_button_hit_test /* 00418da0 */
#define help_window_is_showing main_sim_state_admission_predicate /* 004191a0 */
#endif

#endif /* LEGORRR_GAME_HELPWINDOW_LINK_NAMES_H */
