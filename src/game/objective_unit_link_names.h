// SPDX-License-Identifier: LGPL-3.0-or-later
/* Objective.c (GAME-029) link names (finding 631). In the matching build
 * the unit's functions whose callers already pass the original's arguments
 * take the names those callers use, so Lego.c (lego_unit.c), NERPs
 * (tutorial_script.c) and Reward.c (reward.c) reach the unit with no
 * caller edit. objective_port.cpp and lego_port.cpp compile their own
 * definitions of these names out of that build (LRR_OBJECTIVE_PORT_NAMES).
 * Natively the port's definitions stay the live ones and the unit keeps
 * its own names.
 *
 * Included only by objective_unit.c, before any other header. A macro
 * renames a definition one for one, so MSVC's symbol numbering (finding
 * 538) does not change. */
#ifndef LEGORRR_GAME_OBJECTIVE_UNIT_LINK_NAMES_H
#define LEGORRR_GAME_OBJECTIVE_UNIT_LINK_NAMES_H

/* The callees' own link names: the names their definitions take in the
 * matching build. */
#include "game/bubbles_link_names.h"
#include "game/sfx_link_names.h"
#include "game/teleporter_link_names.h"
#include "game/text_link_names.h"
#include "game/tooltip_link_names.h"

#if defined(_MSC_VER) && _MSC_VER < 1200
#define objective_load_level objective_load_original                /* 00457d40 */
#define objective_has_achieved level_objective_result_get           /* 00458650 */
#define objective_set_end_teleport_enabled global_value_004ffe48_set /* 00458660 */
#define objective_set_status objective_announce_current             /* 00458670 */
#define objective_is_ended objective_is_ended_original              /* 004589a0 */
#define objective_handle_keys lego_quit_poll_adapter                /* 004589c0 */
#define objective_update objective_tick_target                      /* 00458be0 */
#endif

#endif /* LEGORRR_GAME_OBJECTIVE_UNIT_LINK_NAMES_H */
