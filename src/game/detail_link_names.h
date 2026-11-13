// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-011 Detail.c (finding 615): the port's link names of the unit's
 * functions (as findings 597, 600, 607 and 612).
 *
 * In the matching build (MSVC 5.0, _MSC_VER 1100) every caller of these
 * names passes the original's arguments: a texture base name and the set's
 * width and height, or the texture set; the block's four corners, the
 * viewport, the screen size and the no-clip switch; the root container,
 * the two mesh names, the block size and the texture set; the mesh; the
 * texture set and a texture byte; or the mesh, the corners, a texture and
 * direction byte and two floats. So the unit's definitions take the port's
 * names, and the C callers (lego_unit.c, terrain_map.c) and the C++ ones
 * (terrain_grid.cpp, util.cpp, lego_port.cpp) reach them with no caller
 * edit. The port's records have the original's layout on x86
 * (TerrainCellResourceGrid, SceneCellVisualRelayRecord). What served these
 * names is compiled out of the matching build:
 * - tile_variants_port.cpp's 0040b310, 0040b410, 0040b850 and 0040b880, and
 *   its PORT-ONLY adapters of 0040b5f0 and 0040b7d0
 *   (LRR_DETAIL_PORT_NAMES);
 * - the PORT-ONLY adapters in lego_port.cpp (0040b470, 0040b810).
 * Every port name here has C linkage.
 *
 * 0040ba00, 0040bb90 and 0040bd60 keep the unit's names: only the unit
 * calls them, and the port's C++ models of the same addresses stay in the
 * port tail for view.cpp.
 *
 * Included only by detail.c, before any other header. A macro renames a
 * definition one for one, so MSVC's symbol numbering (finding 538) does not
 * change. */
#ifndef LEGORRR_GAME_DETAIL_LINK_NAMES_H
#define LEGORRR_GAME_DETAIL_LINK_NAMES_H

#if defined(_MSC_VER) && _MSC_VER < 1200
#define detail_load_texture_set terrain_cell_resource_grid_load            /* 0040b310 */
#define detail_free_texture_set terrain_cell_resource_grid_dispose         /* 0040b410 */
#define detail_get_block_screen_area lego_projected_quad_area_adapter      /* 0040b470 */
#define detail_load_meshes tile_model_entity_register_original             /* 0040b5f0 */
#define detail_free_mesh cell_visual_channel_table_dispose_original        /* 0040b7d0 */
#define detail_reset_all lego_detail_mesh_resources_clear_adapter          /* 0040b810 */
#define detail_get_texture packed_grid_cell_value_get                      /* 0040b850 */
#define detail_add_block scene_cell_visual_channels_apply                  /* 0040b880 */
#endif

#endif /* LEGORRR_GAME_DETAIL_LINK_NAMES_H */
