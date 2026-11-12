// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what crt.cpp uses from frontend_unit.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by crt.cpp only. */
#ifndef LRR_CRT_CRT_FROM_FRONTEND_UNIT_HPP
#define LRR_CRT_CRT_FROM_FRONTEND_UNIT_HPP

extern void* s_string_registry_head;
void* string_cache_lookup_or_load_corpus(const char* key);

#endif /* LRR_CRT_CRT_FROM_FRONTEND_UNIT_HPP */
