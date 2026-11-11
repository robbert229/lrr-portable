// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; #496): what flic_unit.cpp sees from
 * record.cpp. A verbatim snapshot of record_shared.hpp as it was when the
 * #496 migration froze it, so the unit's declarations no longer change
 * with later relocations. Never edited by tools/relocate_unit.py. */
#ifndef LRR_GODS98_RECORD_FOR_FLIC_UNIT_HPP
#define LRR_GODS98_RECORD_FOR_FLIC_UNIT_HPP

void* rec_ptr(const void* base, size_t off);

uint32_t rec_dword(const void* base, size_t off);

#endif /* LRR_GODS98_RECORD_FOR_FLIC_UNIT_HPP */
