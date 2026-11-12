// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what lego uses from view.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GODS98_VIEW_FOR_LEGO_HPP
#define LRR_GODS98_VIEW_FOR_LEGO_HPP

extern SinePhaseBlendState s_sine_phase_blend_state;
extern SinePhaseBlendHooks s_sine_phase_blend_hooks;

#endif /* LRR_GODS98_VIEW_FOR_LEGO_HPP */
