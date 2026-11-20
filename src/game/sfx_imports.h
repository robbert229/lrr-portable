// SPDX-License-Identifier: LGPL-3.0-or-later
/* SFX.c (GAME-041) imports: C declarations of the callees the unit uses
 * from other modules (ADR 0005 step 6; finding 585). Included only by sfx.c
 * and test_c_boundaries.cpp, so the other C units' symbol numbering does not
 * change (finding 568, "Unit-private imports header"). Each declaration
 * names a definition with C linkage, with the port's parameter types; sfx.c
 * casts its original-layout records to them. */
#ifndef LEGORRR_GAME_SFX_IMPORTS_H
#define LEGORRR_GAME_SFX_IMPORTS_H

#include <stdint.h>

#include "gods98/container.h"
#include "gods98/maths.h"
#include "gods98/mesh_unit.h"
#include "gods98/sound3d.h"
#include "gods98/utils.h"
#include "gods98/view.h"
#include "terrain_map.h"

#include "gods98/container_record.h"

#ifdef __cplusplus
extern "C" {
#endif

struct EngineTaskCallbackRecord;
struct MeshRecordPool;

/* GAME-021 Lego.c. 00431570 Lego_GetMap takes no arguments. */
void* level_current_terrain_get(void);           /* 00431570 */
#ifdef __cplusplus
#define SFX_LEGO_GET_MAP() ((TerrainMap*)level_current_terrain_get())
#else
#define SFX_LEGO_GET_MAP() \
    ((TerrainMap*)level_current_terrain_get())
#endif
/* 004295a0: the current camera's container (Lego.c). */
void* lego_context_selected_global_pointer_get(void);           /* 004295a0 */

/* gods98 Container.c (crt.hpp, view.hpp, record.hpp). The frame these
 * units hang meshes on is the container's activity frame (+0x04,
 * gods98/container_record.h). */
#define SFX_CONTAINER_FRAME(cont) ((void*)((struct Container*)(cont))->activityFrame)
void scene_sample_hook_register(void* hook, void* value);          /* 00472920 */
void* scene_root_object_get(void);                                 /* 00472940 */
/* 00472950 Container_Create: the port's entry has C++ linkage in namespace
 * lego, so natively the unit calls lego_port.cpp's existing adapter. */
/* Matching build: Container.c's own, under its link name
 * (container_link_names.h; #805, finding 674). */
#define lego_container_create_adapter ddiscene_object_allocate
void* lego_container_create_adapter(void* parent);                 /* 00472950 */
void* scene_object_wrapped_pointer_get(const void* wrapped);       /* 00473680 */
void ddiscene_object_position_get(void* object, void* alternate_frame,
                                 float position[3]);               /* 004754c0 */
void ddiscene_object_facing_pair_get(void* object, void* alternate_frame,
                                    float first[3], float second[3]); /* 00475500 */
void* scene_object_query_value_if_ready(void* object);             /* 00476f70 */

/* gods98 Mesh.c (view.hpp, surface.hpp, record.hpp, mesh_unit.h). */
struct EngineTaskCallbackRecord* engine_task_callback_record_allocate_and_initialize(
    void* owner, void* callback, unsigned long mode, void* context,
    unsigned long flag_value);                                          /* 00480870 */
long mesh_record_allocate_and_fill(struct MeshRecordPool* pool,
                                  unsigned long vertex_count,
                                  unsigned long index_rows,
                                  unsigned long index_columns,
                                  const unsigned int* source_indices); /* 00482220 */
void instance_quad_vertex_data_fill(void* record, unsigned long block,
                                    unsigned long first_vertex, unsigned long count,
                                    const float* positions,
                                    const float triple[3],
                                    const uint32_t* pairs);        /* 00482500 */
void indexed_record_flag6c_bit0_set(void* record, unsigned long index, int set); /* 004827a0 */
void record_flag2c_bit0_set(void* record, int set);                /* 004827d0 */
uint32_t render_object_color_vector_set(void* record, unsigned long block, float red,
                                        float green, float blue,
                                        unsigned long slot);                 /* 00483270 */
/* 00482e70 Mesh_LoadTexture and 004830c0 Mesh_SetGroupTexture have no
 * port entry with the original's contract and C linkage, so thin
 * unannotated adapters in sfx_port.cpp stand in. */
void* sfx_mesh_load_texture_adapter(const char* basePath, const char* name,
                                    unsigned int* width,
                                    unsigned int* height);         /* 00482e70 */
void sfx_mesh_set_group_texture_adapter(void* mesh, unsigned long group,
                                        void* texture);            /* 004830c0 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_SFX_IMPORTS_H */
