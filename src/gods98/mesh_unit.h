// SPDX-License-Identifier: LGPL-3.0-or-later
/* C boundary header (ADR 0005) for the Mesh.c (G98-015) entries that
 * gods98/lws.c calls. G98-015 is relocated to gods98/mesh_unit.cpp (#543);
 * Mesh_Load (FUN_004809e0) is still model.cpp's. The port's contracts are
 * used where they have C linkage:
 * - Mesh_Load (FUN_004809e0) is lwo_model_entry_load;
 * - Mesh_GetGroupCount (FUN_00482210) is pointer_value_get.
 * Four have no original-ABI entry with C linkage (ADR 0005 step 6), so thin
 * unannotated adapters in gods98/lws_port.cpp stand in:
 * - mesh_group_material_values_set for Mesh_SetGroupMaterialValues
 *   (FUN_00483420, the port's screen_effect_item_opacity_set_target, which
 *   the frozen record_from_mesh_unit.hpp declares with C++ linkage);
 * - mesh_clone for Mesh_Clone (FUN_00480900, the port's
 *   lego::cached_render_resource_instance_create, which takes only the mesh,
 *   as a type in namespace lego);
 * - mesh_texture_time_set for Mesh_SetTextureTime2 (FUN_00482040, the
 *   port's ledger_stub_00482040_AnimationTrackApplyWrappedFrameOffset in
 *   mesh_unit.cpp, which takes an integer frame offset);
 * - mesh_remove for Mesh_Remove (FUN_004820d0), which the port does not
 *   model.
 * Shared C++ headers do not include this file; src/game/test_c_boundaries.cpp
 * checks it against model.hpp and record.hpp. */
#ifndef LEGORRR_GODS98_MESH_UNIT_H
#define LEGORRR_GODS98_MESH_UNIT_H

#include <stdint.h>

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
