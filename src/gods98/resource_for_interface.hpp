// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what interface uses from resource.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GODS98_RESOURCE_FOR_INTERFACE_HPP
#define LRR_GODS98_RESOURCE_FOR_INTERFACE_HPP

/* --- resource-handle release band (2026-10-25; corpus .data band) ---------
 * ResourceHandleSetRelease (FUN_0041a850) frees the corpus's global handle
 * set: the DAT_004ddc04 stride-8 record band (the first DWORD of each pair,
 * while the walker stays below DAT_004ddcc4) followed by twelve named
 * globals, freed in corpus order. Deviations: the band lands as
 * own storage (install/peek hooks for the named globals), the
 * corpus frees without clearing, and the port nulls each freed slot. */
struct ResourceHandleSetBandSlot {
    void* handle;      /* corpus band dword (stride 8; the word behind rides) */
    uint32_t word2;
};

extern void* s_handle_set_named[12];

#endif /* LRR_GODS98_RESOURCE_FOR_INTERFACE_HPP */
