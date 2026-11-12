// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what lego uses from cmd.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GAME_CMD_FOR_LEGO_HPP
#define LRR_GAME_CMD_FOR_LEGO_HPP

/* --- the upgrade-name table band (2026-10-25) ------------------------------
 * UpgradeNameEntryFind (FUN_0042cd50): a case-insensitive scan over the
 * loader-owned name table, answering the indexed record's address. The
 * loader (the config seam) has no ported owner, so the three globals — the
 * name-pointer array DAT_005561f8, the record block DAT_005561e4, and the
 * entry count DAT_0055628c — ride zero-init own storage behind an install
 * hook. Deviations: the region span (0x1f8..0x28c, 37 dwords) sizes the
 * window to 36 installable entries (the corpus strides its count bound
 * blindly); NULL names (the searched name or a table entry) skip the
 * compare. A cold table answers NULL like the corpus's zero-count return. */
#define UPGRADE_NAME_TABLE_MAX 36 /* span-derived window guard */

extern const char* s_upgrade_names[UPGRADE_NAME_TABLE_MAX];
extern void* s_upgrade_records;
extern unsigned s_upgrade_count;

#endif /* LRR_GAME_CMD_FOR_LEGO_HPP */
