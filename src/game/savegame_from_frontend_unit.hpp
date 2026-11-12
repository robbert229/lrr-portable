// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what savegame.cpp uses from frontend_unit.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by savegame.cpp only. */
#ifndef LRR_GAME_SAVEGAME_FROM_FRONTEND_UNIT_HPP
#define LRR_GAME_SAVEGAME_FROM_FRONTEND_UNIT_HPP

namespace unported_ledger_stubs {
int ledger_stub_00418040_CampaignChainSnapshotCreate( const GameContext* ctx, void* snapshot, size_t capacity);
} // namespace unported_ledger_stubs
int sav_write_slot_file(GameContext* ctx);
int list_find_name_target( const SavListWindow* win, const char* name);

#endif /* LRR_GAME_SAVEGAME_FROM_FRONTEND_UNIT_HPP */
