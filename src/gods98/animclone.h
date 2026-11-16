// SPDX-License-Identifier: LGPL-3.0-or-later
/* AnimClone.c (original translation unit G98-025, finding 616): clones of a
 * Direct3DRM frame hierarchy that is driven by an animation set or by a
 * LightWave scene (Lws.c), and Material_Create.
 *
 * The frame and animation-set types are miniwin's DirectX 6 interfaces
 * (ADR 0007 decision 3), so this header is C++. Shared C++ headers do not
 * include this file: every include and declaration a header adds shifts
 * MSVC's symbol numbering (finding 506). src/game/test_c_boundaries.cpp
 * includes it.
 *
 * In the matching build animclone_link_names.h, which animclone.c includes
 * first, gives three of these functions the port's names (mesh.h and
 * view.hpp), so terrain_map.c, container_unit.cpp and view.cpp reach them.
 * Natively nothing calls these names: the port tail com_object_tree_port.cpp
 * serves the port's names. The names are descriptive; the comments give the
 * gods98 names (OpenLRR's, inferred). */
#ifndef LEGORRR_GODS98_ANIMCLONE_H
#define LEGORRR_GODS98_ANIMCLONE_H

#include <miniwin/d3drm.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct AnimClone AnimClone;
struct LwsInfo;

/* AnimClone_WalkTree's callback: nonzero stops the walk. */
typedef int (*AnimCloneWalkTreeCallback)(LPDIRECT3DRMFRAME3 frame,
                                         void* data);

/* Material_Create (FUN_004894c0): an IDirect3DRMMaterial2 with the
 * emissive and specular colours and the power; NULL on failure. */
void* material_create(float emissiveRed, float emissiveGreen,
                      float emissiveBlue, float specularRed,
                      float specularGreen, float specularBlue, float power);

/* AnimClone_Register (FUN_00489520): the original of an animation-set
 * hierarchy rooted at `root`. */
AnimClone* anim_clone_register(LPDIRECT3DRMANIMATIONSET2 animSet,
                               LPDIRECT3DRMFRAME3 root,
                               unsigned long frameCount);

/* AnimClone_RegisterLws (FUN_004895c0): the original of an Lws scene. */
AnimClone* anim_clone_register_lws(struct LwsInfo* scene,
                                   LPDIRECT3DRMFRAME3 root,
                                   unsigned long frameCount);

/* AnimClone_Make (FUN_00489660): a clone of `orig` under `parent`. */
AnimClone* anim_clone_make(AnimClone* orig, LPDIRECT3DRMFRAME3 parent,
                           unsigned long* frameCount);

/* AnimClone_Remove (FUN_00489750). */
void anim_clone_remove(AnimClone* dead);

/* AnimClone_IsLws (FUN_004897d0). */
int anim_clone_is_lws(AnimClone* clone);

/* AnimClone_SetTime (FUN_004897e0): with `oldTime`, the original is put
 * back to that time after its pose is copied. */
void anim_clone_set_time(AnimClone* clone, float time, float* oldTime);

/* AnimClone_WalkTree (FUN_00489910): 1 when a callback stopped the walk. */
int anim_clone_walk_tree(LPDIRECT3DRMFRAME3 frame, unsigned long level,
                         AnimCloneWalkTreeCallback Callback, void* data);

/* AnimClone_CreateCopy (FUN_004899f0). */
void anim_clone_create_copy(LPDIRECT3DRMFRAME3 orig,
                            LPDIRECT3DRMFRAME3 clone, int lws);

/* AnimClone_CloneLwoMeshes (FUN_00489b30). */
void anim_clone_clone_lwo_meshes(LPDIRECT3DRMFRAME3 orig,
                                 LPDIRECT3DRMFRAME3 clone);

/* AnimClone_ReferenceVisuals (FUN_00489bc0). */
void anim_clone_reference_visuals(LPDIRECT3DRMFRAME3 orig,
                                  LPDIRECT3DRMFRAME3 clone);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_ANIMCLONE_H */
