// SPDX-License-Identifier: LGPL-3.0-or-later
/* Encyclopedia.c (GAME-014) link names (finding 633). In the matching build
 * the unit's functions whose callers already pass the original's arguments
 * take the names those callers use: lego_unit.c, interface_unit.c,
 * legoobject_unit.c and the port's C++ callers reach the unit with no
 * caller edit. encyclopedia_port.cpp compiles its own definitions of these
 * names out of that build. Natively the port's definitions stay the live
 * ones and the unit keeps its own names.
 *
 * Included only by encyclopedia.c, before any other header. A macro renames
 * a definition one for one, so MSVC's symbol numbering (finding 538) does
 * not change. */
#ifndef LEGORRR_GAME_ENCYCLOPEDIA_LINK_NAMES_H
#define LEGORRR_GAME_ENCYCLOPEDIA_LINK_NAMES_H

#if defined(_MSC_VER) && _MSC_VER < 1200
#define encyclopedia_initialise encyclopedia_config_files_open          /* 0040e490 */
#define encyclopedia_select_object terrain_choice_resource_select       /* 0040e700 */
#define encyclopedia_disable_update global_flag_bit0_clear              /* 0040e7e0 */
#define encyclopedia_remove_current_reference global_value_clear_if_equal /* 0040e910 */
#endif

#endif /* LEGORRR_GAME_ENCYCLOPEDIA_LINK_NAMES_H */
