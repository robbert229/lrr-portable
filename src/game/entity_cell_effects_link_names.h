// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-008 link names (finding 652). In the matching build three of the
 * unit's functions take the names their callers already use with the
 * original's arguments: lego_unit.c, objective_unit.c, legoobject_unit.c,
 * message_unit.c and the port's extern "C" callers then reach the unit
 * with no caller edit, and entity_cell_effects_port.cpp compiles its own
 * definitions of these names out of that build. Natively the port's
 * definitions stay the live ones and the unit keeps its own names.
 *
 * Included only by entity_cell_effects.c, before any other header. A macro
 * renames a definition one for one, so MSVC's symbol numbering (finding
 * 538) does not change. */
#ifndef LEGORRR_GAME_ENTITY_CELL_EFFECTS_LINK_NAMES_H
#define LEGORRR_GAME_ENTITY_CELL_EFFECTS_LINK_NAMES_H

/* The callees' own link names. */
#include "game/building_model_link_names.h"
#include "game/entity_stats_link_names.h"
#include "game/rock_fall_styles_link_names.h"
#include "game/selectplace_link_names.h"

#if defined(_MSC_VER) && _MSC_VER < 1200
#define building_set_cost_drop_disabled level_state_stored_value_set          /* 00409d40 */
#define building_remove_from_terrain entity_cell_path_effects_apply_current   /* 00409d50 */
#define object_tracked_replace_and_remove entity_destroy_tracked_pointer_cleanup /* 00409ff0 */
#endif

#endif /* LEGORRR_GAME_ENTITY_CELL_EFFECTS_LINK_NAMES_H */
