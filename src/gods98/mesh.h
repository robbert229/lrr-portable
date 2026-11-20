// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 container meshes, materials and viewport transforms for units
 * rebuilt as C (the original's Container_Mesh_*, Material_Create and
 * Viewport_Transform).
 *
 * C boundary header (ADR 0005). Shared C++ headers do not include this
 * file, and neither do the other C units, so its declarations move no other
 * unit's symbol numbering (finding 506). test_c_boundaries.cpp checks each
 * declaration against the C++ header that also declares it.
 *
 * Four entries have no original-ABI function in the port, so they are thin
 * unannotated adapters with the original signature (ADR 0005 step 6),
 * defined at the end of gods98/surface.cpp:
 * - container_make_mesh2 (FUN_00473560): the port's entry is a ledger stub
 *   in view.cpp with C++ linkage;
 * - container_mesh_hide_group (FUN_00474b30),
 *   container_mesh_set_vertices (FUN_00474d30) and
 *   container_mesh_perspective_correction_set (FUN_00474e30): the port's
 *   entries take an extra table of host operations. */
#ifndef LEGORRR_GODS98_MESH_H
#define LEGORRR_GODS98_MESH_H

#ifdef __cplusplus
extern "C" {
#endif

/* Container_MakeMesh2 (FUN_00473560): a mesh container of `type` under
 * `parent`. */
void* container_make_mesh2(void* parent, int type);

/* Container_Mesh_AddGroup (FUN_004748f0): returns the new group's index. */
unsigned long mesh_record_allocate_or_append(void* object, unsigned long vertex_count,
                                   unsigned long index_rows,
                                   unsigned long index_columns,
                                   const unsigned int* source_indices);

/* Container_Mesh_SetQuality (FUN_00474a60). */
void animated_texture_frame_bind(void* texture, unsigned long frame_index,
                                 unsigned long mode);

/* Container_Mesh_IsGroupHidden (FUN_00474ae0). */
unsigned int terrain_detail_entry_get_at_index(void* object,
                                               unsigned long index);

/* Container_Mesh_HideGroup (FUN_00474b30). */
void container_mesh_hide_group(void* mesh, unsigned long group, int hide);

/* Container_Mesh_GetVertices (FUN_00474cc0). */
unsigned long lwo_layer_polygon_vertex_data_read(void* object, unsigned long block,
                                                unsigned long first_vertex, unsigned long count,
                                                void* output);

/* Container_Mesh_SetVertices (FUN_00474d30). */
unsigned long container_mesh_set_vertices(void* mesh, unsigned long group,
                                 unsigned long index, unsigned long count,
                                 const void* vertices);

/* Container_Mesh_SetTexture (FUN_00474da0). */
void animated_texture_frame_set(void* texture, unsigned long frame_index,
                                const void* frame_state);

/* Container_Mesh_SetPerspectiveCorrection (FUN_00474e30). */
void container_mesh_perspective_correction_set(void* mesh,
                                               unsigned long group, int on);

/* Container_Mesh_SetEmissive (FUN_00474ff0). */
void scene_mesh_color_set(void* object, unsigned long block, float red, float green,
                          float blue);

/* Container_Mesh_SetColourAlpha (FUN_00475020). */
void scene_mesh_color_and_alpha_set(void* object, unsigned long block, float red,
                                    float green, float blue, float alpha);

/* MeshLOD_SwapTarget's container swap (FUN_00474760). */
void interface_array_forward_sync(void* target, void* origin, int restore);

/* Viewport_Transform (FUN_004772b0). */
void render_interface_transform_point(void* object, void* output,
                                      const void* input);

/* Material_Create (FUN_004894c0). */
void* scene_material_create(float color0, float color1, float color2,
                            float color3, float color4, float color5,
                            float power);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_MESH_H */
