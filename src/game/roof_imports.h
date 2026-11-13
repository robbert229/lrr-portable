// SPDX-License-Identifier: LGPL-3.0-or-later
/* Roof.c (GAME-038) imports: C declarations of the callees the unit uses
 * from other modules (ADR 0005 step 6; finding 619). Included only by
 * roof.c and test_c_boundaries.cpp, so the other C units' symbol numbering
 * does not change (finding 568, "Unit-private imports header"). Each
 * declaration names a definition with C linkage, by the name it has in the
 * matching build.
 *
 * Container.c: the mesh functions are gods98/mesh.h's. Container_Mesh_AddGroup
 * (004748f0) keeps the port's contract, a table of 16-bit indices; the
 * original passes 32-bit face data, so the unit casts it (the cast changes
 * no code). Maths.c: gods98/maths.h. */
#ifndef LEGORRR_GAME_ROOF_IMPORTS_H
#define LEGORRR_GAME_ROOF_IMPORTS_H

#include "gods98/maths.h"
#include "gods98/mesh.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Container.c (container_unit.cpp, record.cpp), by the names other C
 * units already call. */
void ledger_stub_00472a40_DDiSceneObjectDestroy(void* scene_object); /* 00472a40 */
void scene_render_lock_toggle(void* scene_object, int enabled);     /* 00473b40 */
int animated_texture_frame_count(const void* texture);              /* 00474a20 */

/* Container functions whose port entries have C++ linkage: Detail.c's
 * PORT-ONLY adapters with the original signature, at the end of
 * tile_variants_port.cpp. */
void* detail_container_load_texture2_adapter(const char* fname,
                                             int immediate,
                                             unsigned int* width,
                                             unsigned int* height); /* 004744f0 */
void detail_container_free_texture_adapter(void* texture);          /* 00474710 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_ROOF_IMPORTS_H */
