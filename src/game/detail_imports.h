// SPDX-License-Identifier: LGPL-3.0-or-later
/* Detail.c (GAME-011) imports: C declarations of the callees the unit uses
 * from other modules (ADR 0005 step 6; finding 615). Included only by
 * detail.c and test_c_boundaries.cpp, so the other C units' symbol
 * numbering does not change (finding 568, "Unit-private imports header").
 * Each declaration names a definition with C linkage, by the name it has in
 * the matching build. */
#ifndef LEGORRR_GAME_DETAIL_IMPORTS_H
#define LEGORRR_GAME_DETAIL_IMPORTS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Container.c (container_unit.cpp, entity.cpp, record.cpp), by the names
 * other C units already call. */
void* container_load(void* parent, const char* filename,
                     const char* typeString, int looseLoad);        /* 00472cd0 */
void ledger_stub_00472a40_DDiSceneObjectDestroy(void* scene_object); /* 00472a40 */
void* entity_visual_resource_load(void* scene_object);              /* 00473690 */
void scene_render_lock_toggle(void* scene_object, int enabled);     /* 00473b40 */
int animated_texture_frame_count(const void* texture);              /* 00474a20 */
unsigned int lwo_layer_polygon_vertex_data_read(void* object, int block,
                                                int first_vertex, int count,
                                                void* output);      /* 00474cc0 */
void container_mesh_set_vertices(void* mesh, unsigned int group,
                                 unsigned int index, unsigned int count,
                                 const void* vertices);             /* 00474d30 */
void animated_texture_frame_set(void* texture, unsigned int frame_index,
                                const void* frame_state);           /* 00474da0 */
void container_mesh_perspective_correction_set(void* mesh,
                                               unsigned int group, int on); /* 00474e30 */
void render_object_set_packed_rgba_from_float_channels(
    void* scene_object, float red, float green, float blue, float alpha); /* 004750b0 */
int ddiscene_object_position_set(void* object, void* alternate_frame,
                                 float x, float y, float z);        /* 00475430 */
int ddiscene_object_pose_method_dispatch(void* object,
                                         void* alternate_frame,
                                         float dir_x, float dir_y,
                                         float dir_z, float up_x,
                                         float up_y, float up_z);   /* 00475470 */
void render_object_rotate_axis(void* object, unsigned int mode, float x,
                               float y, float z, float angle);      /* 00475580 */
void scene_frame_scale_set(void* object, unsigned int mode, float x,
                           float y, float z);                       /* 004755b0 */
void scene_object_camera_teleport_or_offset(void* object,
                                            unsigned int mode, float x,
                                            float y, float z);      /* 004755e0 */

/* Container functions whose port entries have C++ linkage: PORT-ONLY
 * adapters with the original signature. 004756b0's is legoobject_port.cpp's
 * (legoobject_unit.c calls it too); the others are at the end of
 * tile_variants_port.cpp. */
void* detail_container_load_texture2_adapter(const char* fname,
                                             int immediate,
                                             unsigned int* width,
                                             unsigned int* height); /* 004744f0 */
void detail_container_free_texture_adapter(void* texture);          /* 00474710 */
void detail_container_mesh_get_group_adapter(void* cont, unsigned int group,
                                             unsigned int* vertexCount,
                                             unsigned int* faceCount,
                                             unsigned int* vPerFace,
                                             unsigned int* faceDataSize,
                                             unsigned int* faceData); /* 00474c40 */
void detail_container_mesh_scale_adapter(void* cont, float x, float y,
                                         float z);                  /* 00474e90 */
void detail_container_inverse_transform_adapter(void* cont, void* dest,
                                                const void* src);   /* 00475090 */
void legoobject_gt_container_matrix_set_adapter(void* cont,
                                                unsigned int mode,
                                                const float* matrix); /* 004756b0 */

/* Viewports.c (viewports.c), by its link name: Viewport_Transform. */
void render_interface_transform_point(void* object, void* output,
                                      const void* input);           /* 004772b0 */

/* Maths.c (maths.c): Maths_TriangleAreaZ. */
float surface_triangle_area_calculate(const float* p1, const float* p2,
                                      const float* p3, int bfc);    /* 00479630 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_DETAIL_IMPORTS_H */
