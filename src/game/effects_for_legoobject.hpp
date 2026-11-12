// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what legoobject uses from effects.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GAME_EFFECTS_FOR_LEGOOBJECT_HPP
#define LRR_GAME_EFFECTS_FOR_LEGOOBJECT_HPP

extern KindEffectDispatchHooks s_kind_effect_dispatch_hooks;
extern EntityNodeScaleApplyHook s_entity_node_scale_apply_hook;
extern void* s_entity_node_scale_apply_context;

#endif /* LRR_GAME_EFFECTS_FOR_LEGOOBJECT_HPP */
