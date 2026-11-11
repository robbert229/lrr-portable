// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private to src/gods98/record.cpp and the units relocated out of it (ADR 0006
 * decision 4; tools/relocate_unit.py). Statics both sides use have
 * external linkage through the declarations below. Include it only from
 * those files: it is not a shared header. */
#ifndef LRR_GODS98_RECORD_SHARED_HPP
#define LRR_GODS98_RECORD_SHARED_HPP

void* rec_ptr(const void* base, size_t off);

uint32_t rec_dword(const void* base, size_t off);

float rec_float(const void* base, size_t off);

void scene_visibility_hook_call(void* object, int flag);
extern uint32_t s_overlay_word;
extern uint32_t s_overlay_cache;
extern uint32_t s_indexed_surface_width;
extern uint32_t s_indexed_surface_entry_count;
extern const void* const* s_indexed_surface_entries;
extern uint32_t s_indexed_surface_hidden_count;

#endif /* LRR_GODS98_RECORD_SHARED_HPP */
