// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what savegame.cpp uses from reward.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by savegame.cpp only. */
#ifndef LRR_GAME_SAVEGAME_FROM_REWARD_HPP
#define LRR_GAME_SAVEGAME_FROM_REWARD_HPP

namespace unported_ledger_stubs {
void ledger_stub_00460290_EntityType4IndexListBuild( EntityType4IndexNameList* output);
} // namespace unported_ledger_stubs

#endif /* LRR_GAME_SAVEGAME_FROM_REWARD_HPP */
