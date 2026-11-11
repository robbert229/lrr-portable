// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what wad uses from startup_config.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GAME_STARTUP_CONFIG_FOR_WAD_HPP
#define LRR_GAME_STARTUP_CONFIG_FOR_WAD_HPP

namespace unported_ledger_stubs {
extern BufferedCharReadHook s_buffered_char_read_hook;
extern void* s_buffered_char_read_context;
} // namespace unported_ledger_stubs

#endif /* LRR_GAME_STARTUP_CONFIG_FOR_WAD_HPP */
