// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what frontend_unit uses from savegame.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GAME_SAVEGAME_FOR_FRONTEND_UNIT_HPP
#define LRR_GAME_SAVEGAME_FOR_FRONTEND_UNIT_HPP

extern int s_active_object_ingame;
extern int s_active_object_outgame;
extern void* s_sav_tree_root[2];
extern void** s_sav_visited_outgame_items;
extern uint32_t* s_sav_visited_outgame_words;
extern void** s_sav_visited_ingame_items;
extern uint32_t* s_sav_visited_ingame_words;
extern int s_save_current_slot;
extern uint8_t s_sav_slot_context_blocks[SAVE_SLOTS * 0xb8];
extern uint8_t* s_sav_slot_records[SAVE_SLOTS];
extern int32_t s_sav_slot_record_counts[SAVE_SLOTS];
void save_slot_sidecar_release(int slot);
void sav_tree_owned_visit_flags_clear(int which);
int save_tree_level_count(int which, uint32_t* count_out);
int save_current_level_counts(uint32_t* campaign_count, uint32_t* tutorial_count);
void save_release_records(GameContext* ctx);
int save_bootstrap_records(GameContext* ctx, int count);
extern uint32_t s_save_state_event_context;

#endif /* LRR_GAME_SAVEGAME_FOR_FRONTEND_UNIT_HPP */
