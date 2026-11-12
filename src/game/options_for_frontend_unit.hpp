// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what frontend_unit uses from options.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GAME_OPTIONS_FOR_FRONTEND_UNIT_HPP
#define LRR_GAME_OPTIONS_FOR_FRONTEND_UNIT_HPP

/* Typed OptionsWriteToSlot adapter. */
/* The corpus's 0xb8-byte slot window is carried in host-width module state. */
#define OPTIONS_SLOT_WINDOW_DWORDS 46

void options_load_corpus_defaults(GameOptions* o);
extern int s_options_frontend_selection;
extern uint32_t s_options_slot_window[OPTIONS_SLOT_WINDOW_DWORDS];
extern void* s_options_slot_resource;

#endif /* LRR_GAME_OPTIONS_FOR_FRONTEND_UNIT_HPP */
