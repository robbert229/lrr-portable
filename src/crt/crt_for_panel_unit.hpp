// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what panel_unit uses from crt.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_CRT_CRT_FOR_PANEL_UNIT_HPP
#define LRR_CRT_CRT_FOR_PANEL_UNIT_HPP

#include "crt_for_advisor.hpp"
#include "crt_for_bubbles.hpp"
#include "crt_for_building_model.hpp"
#include "crt_for_dependencies.hpp"
#include "crt_for_encyclopedia.hpp"
#include "crt_for_entity_cell_effects.hpp"
#include "crt_for_info_track_overlay.hpp"
#include "crt_for_objective_unit.hpp"
#include "crt_for_pointer.hpp"
#include "crt_for_record_chains.hpp"
#include "crt_for_rock_fall_styles.hpp"
#include "crt_for_screen_effect_quads.hpp"
#include "crt_for_sfx.hpp"
#include "crt_for_terrain_cell_objects.hpp"
#include "crt_for_tutorial_script.hpp"
#include "crt_shared.hpp"

/* The raw x86 body indexes the float table at record+0x34 with a 0x3190
 * stride, loads the float at +0x2fb8, and tail-jumps to __ftol. Ghidra
 * treated the x87 input as the whole function and lost both parameters. */
#if defined(_MSC_VER) && defined(_M_IX86)
extern "C" int __cdecl _ftol(void);
#endif

#if defined(_MSC_VER) && defined(_M_IX86)
extern "C" int __cdecl _ftol(void);
#endif

extern void* s_panel_records[DAT_WINDOW_COUNT];

#endif /* LRR_CRT_CRT_FOR_PANEL_UNIT_HPP */
