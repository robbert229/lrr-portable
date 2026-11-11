// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what resource.cpp uses from wad.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by resource.cpp only. */
#ifndef LRR_GODS98_RESOURCE_FROM_WAD_HPP
#define LRR_GODS98_RESOURCE_FROM_WAD_HPP

int wwad_archive_index_load(const char* path);
Resource* ares_open_wad_path(const char* name, int mode);
void* fixed_record_slot_resource_get_target(int slot);

#endif /* LRR_GODS98_RESOURCE_FROM_WAD_HPP */
