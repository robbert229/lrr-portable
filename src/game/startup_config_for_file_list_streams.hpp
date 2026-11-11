// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; #496): what file_list_streams.cpp sees from
 * startup_config.cpp. A verbatim snapshot of startup_config_shared.hpp as it was when the
 * #496 migration froze it, so the unit's declarations no longer change
 * with later relocations. Never edited by tools/relocate_unit.py. */
#ifndef LRR_GAME_STARTUP_CONFIG_FOR_FILE_LIST_STREAMS_HPP
#define LRR_GAME_STARTUP_CONFIG_FOR_FILE_LIST_STREAMS_HPP

namespace unported_ledger_stubs {
extern GameFileHandle* s_startup_streams[4];

extern uint32_t s_startup_window_mode;

extern char s_startup_output_path[260];

extern char s_startup_token_list_path[260];
} // namespace unported_ledger_stubs

#endif /* LRR_GAME_STARTUP_CONFIG_FOR_FILE_LIST_STREAMS_HPP */
