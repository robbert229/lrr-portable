// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what container_unit uses from surface.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GODS98_SURFACE_FOR_CONTAINER_UNIT_HPP
#define LRR_GODS98_SURFACE_FOR_CONTAINER_UNIT_HPP

extern MeshRecordAppendOps s_mesh_record_append_ops;
extern TextureSurfaceCreateHook s_texture_surface_create_hook;
extern void* s_texture_surface_create_context;

#endif /* LRR_GODS98_SURFACE_FOR_CONTAINER_UNIT_HPP */
