// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private to src/gods98/resource.cpp and the units relocated out of it (ADR 0006
 * decision 4; tools/relocate_unit.py). Statics both sides use have
 * external linkage through the declarations below. Include it only from
 * those files: it is not a shared header. */
#ifndef LRR_GODS98_RESOURCE_SHARED_HPP
#define LRR_GODS98_RESOURCE_SHARED_HPP

extern void* s_numbered_bitmap_slots[11];
extern NumberedBitmapLoadHook s_numbered_bitmap_load_hook;
extern void* s_numbered_bitmap_context;

#endif /* LRR_GODS98_RESOURCE_SHARED_HPP */
