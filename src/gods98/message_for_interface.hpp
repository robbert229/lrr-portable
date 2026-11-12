// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what interface uses from message.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GODS98_MESSAGE_FOR_INTERFACE_HPP
#define LRR_GODS98_MESSAGE_FOR_INTERFACE_HPP

#define CATALOG_SLOT_CAPACITY 256

extern CatalogSlotPredicateHook s_catalog_slot_eligible;
extern CatalogSlotPredicateHook s_catalog_slot_available;
extern CatalogSlotRankHook s_catalog_slot_rank;
extern void* s_catalog_slot_context;
int catalog_slot_table_index(int category);

#endif /* LRR_GODS98_MESSAGE_FOR_INTERFACE_HPP */
