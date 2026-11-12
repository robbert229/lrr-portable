// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what legoobject uses from view.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GODS98_VIEW_FOR_LEGOOBJECT_HPP
#define LRR_GODS98_VIEW_FOR_LEGOOBJECT_HPP

extern SceneMatrixApplyHook s_scene_matrix_apply_hook;
extern void* s_scene_matrix_apply_context;
extern SceneObjectDisposeHooks s_scene_object_dispose_hooks;

#endif /* LRR_GODS98_VIEW_FOR_LEGOOBJECT_HPP */
