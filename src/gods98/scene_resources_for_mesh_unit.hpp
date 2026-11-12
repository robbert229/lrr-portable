// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what mesh_unit uses from scene_resources.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GODS98_SCENE_RESOURCES_FOR_MESH_UNIT_HPP
#define LRR_GODS98_SCENE_RESOURCES_FOR_MESH_UNIT_HPP

namespace lego {
extern AnimatedTextureFrameApplyHook s_texture_frame_apply_hook;
extern void* s_texture_frame_apply_context;
extern char s_shared_texture_root[1024];
extern CachedRenderResourceRendererHook s_cached_resource_renderer_hook;
extern void* s_cached_resource_renderer_context;
} // namespace lego

#endif /* LRR_GODS98_SCENE_RESOURCES_FOR_MESH_UNIT_HPP */
