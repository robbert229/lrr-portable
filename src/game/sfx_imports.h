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

#ifdef __cplusplus
extern "C" {
#endif

struct EngineTaskCallbackRecord;
struct MeshRecordPool;

/* GAME-021 Lego.c. 00431570 Lego_GetMap takes no arguments; lego_unit.c
 * defines it with the port's two ignored cell arguments, so the C matching
 * build calls it through an unprototyped function type, which pushes none,
 * as the original does; the native C++ compile goes through lego_port.cpp's
 * adapter (as weapons_imports.h). */
void* level_current_terrain_get(int cell_x, int cell_y);           /* 00431570 */
void* level_current_terrain_get_original(void);
#ifdef __cplusplus
#define SFX_LEGO_GET_MAP() ((TerrainMap*)level_current_terrain_get_original())
#else
#define SFX_LEGO_GET_MAP() \
    ((TerrainMap*)((void* (*)())level_current_terrain_get)())
#endif
/* 004295a0: the current camera's container (Lego.c). */
uint32_t lego_context_selected_global_pointer_get(void);           /* 004295a0 */

/* gods98 Container.c (crt.hpp, view.hpp, record.hpp). The container's
 * master frame is at +0x04. */
#define SFX_CONTAINER_FRAME(cont) (*(void**)((char*)(cont) + 0x4))
void scene_sample_hook_register(void* hook, void* value);          /* 00472920 */
void* scene_root_object_get(void);                                 /* 00472940 */
/* 00472950 Container_Create: the port's entry has C++ linkage in namespace
 * lego, so the unit calls lego_port.cpp's existing adapter. */
void* lego_container_create_adapter(void* parent);                 /* 00472950 */
uint32_t scene_object_wrapped_pointer_get(const void* wrapped);    /* 00473680 */
int ddiscene_object_position_get(void* object, void* alternate_frame,
                                 float position[3]);               /* 004754c0 */
int ddiscene_object_facing_pair_get(void* object, void* alternate_frame,
                                    float first[3], float second[3]); /* 00475500 */
uint32_t scene_object_query_value_if_ready(void* object);          /* 00476f70 */

/* gods98 Mesh.c (view.hpp, surface.hpp, record.hpp, mesh_unit.h). */
struct EngineTaskCallbackRecord* engine_task_callback_record_allocate_and_initialize(
    void* owner, void* callback, uint32_t mode, void* context,
    uint32_t flag_value);                                          /* 00480870 */
int mesh_record_allocate_and_fill(struct MeshRecordPool* pool,
                                  uint32_t vertex_count,
                                  uint32_t index_rows,
                                  uint32_t index_columns,
                                  const uint16_t* source_indices); /* 00482220 */
void instance_quad_vertex_data_fill(void* record, int block,
                                    int first_vertex, int count,
                                    const float* positions,
                                    const float triple[3],
                                    const uint32_t* pairs);        /* 00482500 */
void indexed_record_flag6c_bit0_set(void* record, int index, int set); /* 004827a0 */
void record_flag2c_bit0_set(void* record, int set);                /* 004827d0 */
uint32_t render_object_color_vector_set(void* record, int block, float red,
                                        float green, float blue,
                                        int slot);                 /* 00483270 */
/* 00482e70 Mesh_LoadTexture and 004830c0 Mesh_SetGroupTexture have no
 * port entry with the original's contract and C linkage, so thin
 * unannotated adapters in sfx_port.cpp stand in. */
void* sfx_mesh_load_texture_adapter(const char* basePath, const char* name,
                                    unsigned int* width,
                                    unsigned int* height);         /* 00482e70 */
void sfx_mesh_set_group_texture_adapter(void* mesh, int group,
                                        void* texture);            /* 004830c0 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_SFX_IMPORTS_H */
