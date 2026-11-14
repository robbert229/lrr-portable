// SPDX-License-Identifier: LGPL-3.0-or-later
/* ObjInfo.c (GAME-031) link names (finding 633). In the matching build the
 * unit's functions whose callers already pass the original's arguments
 * take the names those callers use: lego_unit.c, bubbles.c and the port's
 * C++ callers reach the unit with no caller edit. objinfo_port.cpp
 * compiles its own definitions of these names out of that build. Natively
 * the port's definitions stay the live ones and the unit keeps its own
 * names.
 *
 * Included only by objinfo.c, before any other header. A macro renames a
 * definition one for one, so MSVC's symbol numbering (finding 538) does not
 * change. */
#ifndef LEGORRR_GAME_OBJINFO_LINK_NAMES_H
#define LEGORRR_GAME_OBJINFO_LINK_NAMES_H

/* The callees' own link names. */
#include "game/screen_effect_quads_link_names.h"

#if defined(_MSC_VER) && _MSC_VER < 1200
#define objinfo_initialise frontend_render_three_pass_update        /* 00459530 */
#define objinfo_draw_health_bar legacy_entity_screen_ring_render    /* 00459b00 */
#define objinfo_draw_hunger objinfo_draw_hunger_image_adapter        /* 00459f50 */
#define objinfo_draw_bubble panel_resource_position_apply_if_enabled /* 00459fd0 */
#endif

#endif /* LEGORRR_GAME_OBJINFO_LINK_NAMES_H */
