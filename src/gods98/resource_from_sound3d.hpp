// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what resource.cpp uses from sound3d.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by resource.cpp only. */
#ifndef LRR_GODS98_RESOURCE_FROM_SOUND3D_HPP
#define LRR_GODS98_RESOURCE_FROM_SOUND3D_HPP

namespace unported_ledger_stubs {
int nested_record_list_remove_matching_key_impl( void* owner, void* key, void* opaque_record);
} // namespace unported_ledger_stubs

#endif /* LRR_GODS98_RESOURCE_FROM_SOUND3D_HPP */
