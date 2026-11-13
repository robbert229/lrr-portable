// SPDX-License-Identifier: LGPL-3.0-or-later
/* FrontEnd.c (GAME-017) link names (finding 623). In the matching build the
 * unit's Front_Callback_TriggerPlayCredits (004103d0) takes the port's
 * name, main_menu_text_item, which Reward.c (reward.c) and the port call;
 * frontend_port.cpp compiles its own definition out of that build.
 * Natively the port's definition stays the live one. */
#ifndef LEGORRR_GAME_FRONTEND_UNIT_LINK_NAMES_H
#define LEGORRR_GAME_FRONTEND_UNIT_LINK_NAMES_H

/* The callees' own link names: the names their definitions take in the
 * matching build. */
#include "game/pointer_link_names.h"
#include "game/screen_effect_quads_link_names.h"
#include "game/sfx_link_names.h"
#include "game/tooltip_link_names.h"
#include "gods98/viewports_link_names.h"

#if defined(_MSC_VER) && _MSC_VER < 1200
#define front_callback_trigger_play_credits main_menu_text_item /* 004103d0 */
#endif

#endif /* LEGORRR_GAME_FRONTEND_UNIT_LINK_NAMES_H */
