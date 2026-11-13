// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-036 RadarMap.c (finding 606): the port's link names of the unit's
 * functions (as findings 581, 585, 590, 592 and 597-601).
 *
 * In the matching build (MSVC 5.0, _MSC_VER 1100) every caller of these
 * names passes the original's arguments: the radar map (Lego_Level +0x84),
 * a screen position, a world position, the zoom or the radar map's window.
 * So the unit's definitions take the port's names, and the C callers
 * (lego_unit.c, legoobject_unit.c) and the C++ ones (avi_player.cpp and
 * movie_unit.cpp, which reach 0045daf0 as the shared operator-delete
 * body) reach them with no caller edit. The port tail map_view_port.cpp
 * compiles its own definitions of these names out of the matching build
 * (LRR_MAP_VIEW_PORT_NAMES), and lego_port.cpp does the same for its
 * PORT-ONLY radar_map_tv_position_set_adapter. Every port name here has C
 * linkage.
 *
 * Three functions keep the unit's names in every build:
 * radarmap_can_show_object (0045e400), radarmap_transform_rect (0045e940)
 * and radarmap_get_block_colour (0045e9e0). Only the unit calls them in the
 * original. Their port names (entity_type_and_state_flag_predicate,
 * view_point_to_screen_geometry_transform, terrain_tile_style_resolve) have
 * C++ linkage and the port's contracts, and the port tail keeps serving
 * them.
 *
 * Included only by radarmap.c, before any other header. A macro renames a
 * definition one for one, so MSVC's symbol numbering (finding 538) does not
 * change. */
#ifndef LEGORRR_GAME_RADARMAP_LINK_NAMES_H
#define LEGORRR_GAME_RADARMAP_LINK_NAMES_H

#if defined(_MSC_VER) && _MSC_VER < 1200
#define radarmap_initialise screen_coord_normalize_init                 /* 0045d8b0 */
#define radarmap_create ui_render_record_create                         /* 0045da90 */
#define radarmap_free ui_render_record_free                             /* 0045daf0 */
#define radarmap_draw_survey_dot_circle textured_scene_object_draw_at_position /* 0045db00 */
#define radarmap_draw radar_map_tv_position_set_adapter                 /* 0045dbc0 */
#define radarmap_inside_radar rect_contains_point_half_open             /* 0045e660 */
#define radarmap_screen_to_block_pos terrain_point_pick_resolve_original /* 0045e6d0 */
#define radarmap_try_select_object viewport_point_callback_dispatch     /* 0045e820 */
#endif

#endif /* LEGORRR_GAME_RADARMAP_LINK_NAMES_H */
