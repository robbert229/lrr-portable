// SPDX-License-Identifier: LGPL-3.0-or-later
/* C boundary header (ADR 0005) for the Mesh.c (G98-015) entries that the C
 * units lws.c, animclone.c, sfx.c and weapons.c call. Mesh.c is rebuilt in
 * original shape as gods98/mesh_unit.c (finding 625), whose own header is
 * gods98/mesh_api.h; the port's models are the port tail
 * gods98/mesh_port.cpp.
 *
 * Mesh_Clone (FUN_00480900) and Mesh_Remove (FUN_004820d0) are the unit's,
 * by these names, in every build. The other declarations keep the port's
 * names (lwo_model_entry_load and pointer_value_get are the port's entries;
 * mesh_texture_time_set and mesh_group_material_values_set are thin
 * adapters in gods98/lws_port.cpp). In the matching build (MSVC 5.0,
 * _MSC_VER 1100) the macros below point the C units' calls at mesh_unit.c
 * instead: every one of them passes the original's arguments. The port's
 * C++ files that include this header keep the port's names. A macro
 * renames a declaration one for one, so MSVC's symbol numbering (finding
 * 538) does not change.
 *
 * Shared C++ headers do not include this file; src/game/test_c_boundaries.cpp
 * checks it against model.hpp and record.hpp. */
#ifndef LEGORRR_GODS98_MESH_UNIT_H
#define LEGORRR_GODS98_MESH_UNIT_H

#include <stdint.h>

#if defined(_MSC_VER) && _MSC_VER < 1200 && !defined(__cplusplus)
#define lwo_model_entry_load mesh_load /* 004809e0 */
#define mesh_texture_time_set mesh_set_texture_time2 /* 00482040 */
#define pointer_value_get mesh_get_group_count /* 00482210 */
#define mesh_group_material_values_set mesh_set_group_material_values /* 00483420 */
/* sfx_imports.h and weapons_imports.h, which include this header first. */
#define engine_task_callback_record_allocate_and_initialize mesh_create_on_frame /* 00480870 */
#define instance_quad_vertex_data_fill mesh_set_vertices_point_normal_at /* 00482500 */
#define mesh_segment_vertex_fill mesh_set_vertices_same_normal /* 00482620 */
#define indexed_record_flag6c_bit0_set mesh_hide_group /* 004827a0 */
#define record_flag2c_bit0_set mesh_hide /* 004827d0 */
#define sfx_mesh_load_texture_adapter mesh_load_texture /* 00482e70 */
#define sfx_mesh_set_group_texture_adapter animated_texture_frame_surface_set /* 004830c0 */
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct Model;

void* mesh_clone(void* mesh, void* frame);

struct Model* lwo_model_entry_load(const char* path, void* flags,
                                   void* context);

int mesh_texture_time_set(void* mesh, float time);

void mesh_remove(void* mesh, void* frame);

uint32_t pointer_value_get(const void* pointed);

uint32_t mesh_group_material_values_set(void* mesh, int group,
                                        float value, int type);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_MESH_UNIT_H */
