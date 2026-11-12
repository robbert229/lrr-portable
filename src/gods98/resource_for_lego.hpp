// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what lego uses from resource.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GODS98_RESOURCE_FOR_LEGO_HPP
#define LRR_GODS98_RESOURCE_FOR_LEGO_HPP

namespace unported_ledger_stubs {
extern MiscObjectResourceHooks s_misc_resource_hooks;
extern void* s_misc_resource_handles[22];
} // namespace unported_ledger_stubs

#endif /* LRR_GODS98_RESOURCE_FOR_LEGO_HPP */
