// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what record.cpp uses from container_unit.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by record.cpp only. */
#ifndef LRR_GODS98_RECORD_FROM_CONTAINER_UNIT_HPP
#define LRR_GODS98_RECORD_FROM_CONTAINER_UNIT_HPP

void entity_flag_80_set_corpus(void* record, int set);

#endif /* LRR_GODS98_RECORD_FROM_CONTAINER_UNIT_HPP */
