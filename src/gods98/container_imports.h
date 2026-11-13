// SPDX-License-Identifier: LGPL-3.0-or-later
/* What Container.c (G98-002, finding 624) uses from units not yet rebuilt
 * as C, beyond the converted units' own headers (utils.h, config.h,
 * files.h, lws.h, animclone.h, mesh_unit.h, sound3d.h, g98_026.h,
 * directdraw.h, main.h), which container.c includes. Included only by
 * container.c and src/game/test_c_boundaries.cpp, so the other C units'
 * symbol numbering does not change (finding 568). Each declaration names a
 * definition with C linkage, by the name it has in the matching build. */
#ifndef LEGORRR_GODS98_CONTAINER_IMPORTS_H
#define LEGORRR_GODS98_CONTAINER_IMPORTS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct EngineTaskCallbackRecord;
struct MeshRecordPool;

/* Main.c (G98-005): lpDevice() (00505918; viewports_port.cpp's stand-in),
 * the fog method (00505920; view.cpp's word, which view.cpp and
 * container_port.cpp give C linkage before the frozen
 * view_for_container_unit.hpp declares it) and the flags (00505f84,
 * crt/crt.h). */
extern void* s_main_device;
extern uint32_t s_scene_fog_mode;
extern uint32_t s_engine_feature_word;

/* DirectDraw.c (G98-010): lpDD() (directDrawGlobs.lpDirectDraw,
 * 0076ac84). Windows builds read the unit's common (directdraw.h); the
 * native port keeps the port's word (crt.hpp), as aitask_imports.h does. */
#if defined(LEGORRR_BACKEND_MINIWIN)
extern unsigned int s_renderer_global_dword;                       /* 0076ac84 */
#define CONTAINER_LPDD() ((void*)s_renderer_global_dword)
#else
#define CONTAINER_LPDD() ((void*)directDrawGlobs.lpDirectDraw)
#endif

/* Mesh.c (G98-015, mesh_unit.cpp), by the port's names where they have
 * C linkage (mesh_unit.h has five more). */
struct EngineTaskCallbackRecord* engine_task_callback_record_allocate_and_initialize(
    void* owner, void* callback, uint32_t mode, void* context,
    uint32_t flag_value);                                         /* 00480870 */
int mesh_record_allocate_and_fill(struct MeshRecordPool* pool,
                                  uint32_t vertex_count,
                                  uint32_t index_rows,
                                  uint32_t index_columns,
                                  const uint16_t* source_indices); /* 00482220 */
void indexed_record_flag6c_bit0_set(void* record, int index, int set); /* 004827a0 */
void animated_texture_frame_surface_set(void* texture, int frame_index,
                                        const void* frame_state); /* 004830c0 */
uint32_t render_object_color_vector_set(void* record, int block, float red,
                                        float green, float blue,
                                        int slot);                /* 00483270 */

/* Five more Mesh.c entries, whose port definitions (record.hpp) return
 * nothing where the original returns a result. The matching build (C)
 * declares them with the original's results; other builds compile the
 * unit as C++ beside record.hpp, so they call PORT-ONLY adapters with the
 * same signatures at the end of container_port.cpp instead (the unit is
 * not linked natively). */
#if !defined(__cplusplus)
#define container_mesh_get_group_adapter indexed_record_entry_read
#define container_mesh_scale_adapter mesh_vertices_scale_apply
#define container_mesh_set_vertices_adapter terrain_cell_quad_vertices_write
#define container_mesh_get_vertices_adapter indexed_geometry_data_copy_to_output
#define container_mesh_is_group_hidden_adapter indexed_record_flag6c_bit0_get
#endif
int container_mesh_get_group_adapter(void* mesh, unsigned long groupID,
                                     unsigned long* vertexCount,
                                     unsigned long* faceCount,
                                     unsigned long* vPerFace,
                                     unsigned long* faceDataSize,
                                     unsigned long* faceData);    /* 004821a0 */
void container_mesh_scale_adapter(void* mesh, float x, float y,
                                  float z);                       /* 00482370 */
int container_mesh_set_vertices_adapter(void* mesh,
                                        unsigned long groupID,
                                        unsigned long index,
                                        unsigned long count,
                                        void* values);            /* 004823e0 */
int container_mesh_get_vertices_adapter(void* mesh,
                                        unsigned long groupID,
                                        unsigned long index,
                                        unsigned long count,
                                        void* retArray);          /* 00482470 */
int container_mesh_is_group_hidden_adapter(void* mesh,
                                           unsigned long groupID); /* 00482780 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_CONTAINER_IMPORTS_H */
