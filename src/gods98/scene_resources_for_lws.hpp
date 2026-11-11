// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what lws uses from scene_resources.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GODS98_SCENE_RESOURCES_FOR_LWS_HPP
#define LRR_GODS98_SCENE_RESOURCES_FOR_LWS_HPP

namespace lego {
extern SceneResourceReleaseHook s_release_hook;
extern void* s_release_context;
extern void** s_string_table_a;
extern void** s_string_table_b;
extern unsigned int s_string_count_a;
extern unsigned int s_string_count_b;
extern SceneFloatTrackApplyHook s_float_track_apply_hook;
extern void* s_float_track_apply_context;
} // namespace lego

#endif /* LRR_GODS98_SCENE_RESOURCES_FOR_LWS_HPP */
