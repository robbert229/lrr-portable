// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private to src/game/startup_config.cpp and the units relocated out of it (ADR 0006
 * decision 4; tools/relocate_unit.py). Statics both sides use have
 * external linkage through the declarations below. Include it only from
 * those files: it is not a shared header. */
#ifndef LRR_GAME_STARTUP_CONFIG_SHARED_HPP
#define LRR_GAME_STARTUP_CONFIG_SHARED_HPP

namespace unported_ledger_stubs {
extern GameFileHandle* s_startup_streams[4];

extern uint32_t s_startup_window_mode;

extern char s_startup_output_path[260];

extern char s_startup_token_list_path[260];
} // namespace unported_ledger_stubs

#endif /* LRR_GAME_STARTUP_CONFIG_SHARED_HPP */
