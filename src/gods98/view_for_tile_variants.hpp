// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what tile_variants uses from view.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GODS98_VIEW_FOR_TILE_VARIANTS_HPP
#define LRR_GODS98_VIEW_FOR_TILE_VARIANTS_HPP

extern SceneFrameApplyHooks s_scene_frame_apply_hooks;
extern SceneRotationApplyHooks s_scene_rotation_apply_hooks;

#endif /* LRR_GODS98_VIEW_FOR_TILE_VARIANTS_HPP */
