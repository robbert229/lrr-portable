// SPDX-License-Identifier: LGPL-3.0-or-later
/* SelectPlace.c (GAME-040) link names (finding 633). In the matching build
 * the unit's functions take the names their callers use, all of which pass
 * the original's arguments: lego_unit.c, legoobject_unit.c,
 * terrain_cell_objects.c and the port's C++ callers. selectplace_port.cpp
 * and legoobject_port.cpp compile their own definitions of these names out
 * of that build. Natively the port's definitions stay the live ones and
 * the unit keeps its own names.
 *
 * Included only by selectplace.c, before any other header. A macro renames
 * a definition one for one, so MSVC's symbol numbering (finding 538) does
 * not change. */
#ifndef LEGORRR_GAME_SELECTPLACE_LINK_NAMES_H
#define LEGORRR_GAME_SELECTPLACE_LINK_NAMES_H

/* The callees' own link names. */
#include "gods98/container_link_names.h"

#if defined(_MSC_VER) && _MSC_VER < 1200
#define selectplace_create select_place_create_adapter                  /* 00463f00 */
#define selectplace_transform_shape rotated_pairs_scratch_fill          /* 00464110 */
#define selectplace_check_and_update legoobject_gs_placement_footprint_check_adapter /* 004641c0 */
#define selectplace_hide surface_visibility_set                         /* 00464720 */
#endif

#endif /* LEGORRR_GAME_SELECTPLACE_LINK_NAMES_H */
