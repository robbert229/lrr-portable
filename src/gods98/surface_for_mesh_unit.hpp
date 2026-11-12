// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what mesh_unit uses from surface.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GODS98_SURFACE_FOR_MESH_UNIT_HPP
#define LRR_GODS98_SURFACE_FOR_MESH_UNIT_HPP

typedef struct PortableModelRenderEntry {
    struct PortableModelRenderEntry* next;
    Model* model;
    uint32_t entry_flags;
    float parameters[16];
    char path_prefix[260];
} PortableModelRenderEntry;

extern ModelTextureBindOps s_model_texture_bind_ops;
extern MaterialPassOps s_material_pass_ops;
int model_texture_frame_name(const char* name, uint32_t frame, char output[260]);
extern RenderStateApplyOps s_render_state_apply_ops;
extern Direct3DMatrixOps s_direct3d_matrix_ops;
extern ModelRenderRecordStateOps s_model_render_record_state_ops;
extern ModelRenderChildPassOps s_model_render_child_pass_ops;
extern ModelRenderPassDispatchOps s_model_render_pass_dispatch_ops;
extern PortableModelRenderEntry* s_portable_model_render_head;
extern PortableModelRenderEntry* s_portable_model_render_active;
extern Direct3DRenderStateSnapshotOps s_d3d_render_state_snapshot_ops;
extern Direct3DDeviceStateOps s_direct3d_device_state_ops;
extern int s_direct3d_device_texture_dirty;
extern ModelMaterialUpdateHook s_model_material_update_hook;
extern void* s_model_material_update_context;
extern ModelChildMaterialOps s_model_child_material_ops;
extern Direct3DInterfaceOps s_direct3d_interface_ops;

#endif /* LRR_GODS98_SURFACE_FOR_MESH_UNIT_HPP */
