// SPDX-License-Identifier: LGPL-3.0-or-later
/* G98-003 Viewports.c (finding 593): the port's link names of the unit's
 * functions, for the matching build only (as findings 589 and 590).
 *
 * The port's viewport record (ScreenBoundedSurfaceRecord, surface.hpp) has
 * the original Viewport's layout on 32-bit hosts: the IDirect3DRMViewport2
 * at +0x10, the target field of view at +0x14, the rendering flag at +0x18
 * and the free-list link at +0x1c. In the matching build (MSVC 5.0,
 * _MSC_VER 1100) every caller of the names below is an original-shape C
 * unit that passes a viewport it created through these same names
 * (lego_unit.c's views, which advisor.c, sfx.c, terrain_map.c and
 * legoobject_unit.c read), with the original's arguments. So the unit's
 * definitions take the port's names: the callers reach them without an edit,
 * and the port tail viewports_port.cpp compiles its own definitions of these
 * names out of the matching build. Natively the port tail keeps serving them.
 *
 * Two of the names are original-ABI adapters the unit replaces:
 * render_target_viewport_create_original (00476e50) and
 * scene_current_object_set_original (00477240).
 *
 * The other seven keep the unit's names. Viewport_Shutdown,
 * Viewport_GetSize, Viewport_GetField and Viewport_WorldToScreen have C++
 * callers with the port's contract (engine.cpp's pool release, lego_port.cpp's
 * two adapters, entity.cpp's projection adapter), so lego_unit_imports.h and
 * tutorial_script_imports.h point their C callers at the unit instead.
 * Viewport_GetScene, Viewport_AddList and Viewport_RemoveAll are called only
 * from the unit.
 *
 * Included only by viewports.c, before any other header. A macro renames a
 * declaration or a definition one for one, so MSVC's symbol numbering
 * (finding 538) does not change. */
#ifndef LEGORRR_GODS98_VIEWPORTS_LINK_NAMES_H
#define LEGORRR_GODS98_VIEWPORTS_LINK_NAMES_H

#if defined(_MSC_VER) && _MSC_VER < 1200
#define viewport_initialise scene_resource_slots_initialize /* 00476d50 */
#define viewport_create render_target_viewport_scale_set /* 00476dc0 */
#define viewport_create_pixel render_target_viewport_create_original /* 00476e50 */
#define viewport_set_camera resource_interface_call_slot48 /* 00476f50 */
#define viewport_get_camera scene_object_query_value_if_ready /* 00476f70 */
#define viewport_set_back_clip renderer_object_set_clip_distance /* 00476fb0 */
#define viewport_get_back_clip resource_interface_call_slot8c /* 00476fd0 */
#define viewport_get_front_clip resource_interface_call_slot90 /* 00476ff0 */
#define viewport_clear ledger_stub_00477010_DDiSceneDrawBegin /* 00477010 */
#define viewport_render ddiscene_draw_end /* 00477150 */
#define viewport_remove render_resource_track_and_retain /* 00477220 */
#define viewport_smooth_set_field scene_current_object_set_original /* 00477240 */
#define viewport_set_field resource_interface_call_slot40_and_clear_field14 /* 00477250 */
#define viewport_inverse_transform camera_screen_coordinate_project /* 00477290 */
#define viewport_transform render_interface_transform_point /* 004772b0 */
#endif

#endif /* LEGORRR_GODS98_VIEWPORTS_LINK_NAMES_H */
