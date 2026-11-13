// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-022 (the game camera, camera_control.c) imports: C declarations of
 * the callees the unit uses from other modules (ADR 0005 step 6; finding
 * 587). Included only by camera_control.c and test_c_boundaries.cpp, so the
 * other C units' symbol numbering does not change (finding 568,
 * "Unit-private imports header"). Each declaration names a definition with
 * C linkage, with the port's parameter types; camera_control.c casts its
 * original-layout records to them. */
#ifndef LEGORRR_GAME_CAMERA_CONTROL_IMPORTS_H
#define LEGORRR_GAME_CAMERA_CONTROL_IMPORTS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct EntitySlot;
struct TerrainMap;

/* GAME-001 Advisor.c: the out-of-line copies of Maths.h's inline vector
 * functions. The camera unit was compiled without optimisation, so it calls
 * them instead of expanding them (finding 587). */
float vec_len(const float* a);                                    /* 004013a0 */
float vec_dist(const float* a, const float* b);                   /* 00401420 */
float* vec_add(float* dst, const float* a, const float* b);       /* 00401500 */
float* vec_sub(float* dst, const float* a, const float* b);       /* 004016c0 */
float vec_dot(const float* a, const float* b);                    /* 004016f0 */
float* vec_scale(float* dst, const float* src, float s);          /* 00401720 */
void vec_normalize(float* v);                                     /* 00401750 */

/* GAME-002 AITask.c: Mem_Alloc and Mem_Free. */
void* engine_memory_allocate(long size);                          /* 004021e0 */
void engine_memory_free(void* ptr);                               /* 00402210 */

/* GAME-021 Lego.c. */
void lego_flags1_bit17_set(int on);                               /* 0041f940 */
/* 00431570 Lego_GetMap takes no arguments. lego_unit.c defines it with the
 * port's two ignored cell arguments, so the C matching build calls it
 * through an unprototyped function type, which pushes none, as the original
 * does; the native C++ compile goes through lego.cpp's adapter (as
 * weapons_imports.h). */
void* level_current_terrain_get(int cell_x, int cell_y);          /* 00431570 */
void* level_current_terrain_get_original(void);
#ifdef __cplusplus
#define CAMERA_LEGO_GET_MAP() \
    ((struct TerrainMap*)level_current_terrain_get_original())
#else
#define CAMERA_LEGO_GET_MAP() \
    ((struct TerrainMap*)((void* (*)())level_current_terrain_get)())
#endif

/* GAME-023 LegoObject.c (entity.hpp). */
/* In the matching build these name legoobject_unit.c's functions at the
 * same addresses (GAME-023 part 3, issue #615); one declaration each. */
#if defined(_MSC_VER) && _MSC_VER < 1200
#define entity_follow_target_pose_get legoobject_follow_target_pose_get
#define entity_type_component_get legoobject_type_component_get
#endif
void entity_follow_target_pose_get(struct EntitySlot* entity, int index,
                                   float position[3],
                                   float facing[3]);              /* 0044a070 */
void* entity_type_component_get(const void* entity_record);       /* 0044a200 */

/* GAME-026 Map3D.c (terrain_map.h). */
int terrain_map_world_to_block(struct TerrainMap* map, float x, float y,
                               int* bx, int* by);                 /* 0044f6d0 */
float terrain_map_world_z_get(struct TerrainMap* map, float x,
                              float y);                           /* 0044f940 */
float terrain_map_world_z_unmodified_get(struct TerrainMap* map, float x,
                                         float y);                /* 0044fe70 */

/* G98-002 Container.c (container.h, view.h, view.hpp). Container_Create
 * (00472950) is declared in camera_control.c: the port types its argument
 * as the parent's scene-object descriptor. */
void ledger_stub_00472a40_DDiSceneObjectDestroy(void* scene_object); /* 00472a40 */
int ddiscene_object_position_set(void* object, void* alternate_frame,
                                 float x, float y, float z);      /* 00475430 */
int ddiscene_object_pose_method_dispatch(void* object,
                                         void* alternate_frame,
                                         float parameter_3,
                                         float parameter_4,
                                         float parameter_5,
                                         float parameter_6,
                                         float parameter_7,
                                         float parameter_8);      /* 00475470 */
int ddiscene_object_position_get(void* object, void* alternate_frame,
                                 float position[3]);              /* 004754c0 */
int ddiscene_object_facing_pair_get(void* object, void* alternate_frame,
                                    float first[3],
                                    float second[3]);             /* 00475500 */
void render_object_rotate_axis(void* object, uint32_t mode, float x, float y,
                               float z, float angle);             /* 00475580 */
void scene_object_camera_teleport_or_offset(void* object, uint32_t mode,
                                            float x, float y,
                                            float z);             /* 004755e0 */
void ddiscene_scene_object_attach_or_unlink(void* pair_record,
                                            void* source_pair_record); /* 00475710 */

/* G98-007 Maths.c (maths.h). */
float* d3drm_vector_random(float* r);                             /* 00479500 */
float random_float_range(float low, float high);                  /* 004798b0 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_CAMERA_CONTROL_IMPORTS_H */
