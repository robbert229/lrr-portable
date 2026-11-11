// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; #496): what building_model.cpp sees from
 * record.cpp. A verbatim snapshot of record_shared.hpp as it was when the
 * #496 migration froze it, so the unit's declarations no longer change
 * with later relocations. Never edited by tools/relocate_unit.py. */
#ifndef LRR_GODS98_RECORD_FOR_BUILDING_MODEL_HPP
#define LRR_GODS98_RECORD_FOR_BUILDING_MODEL_HPP

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

extern SurfaceVisibilityAdapter s_surface_visibility_adapter;
extern void* s_surface_visibility_context;

void rec_dword_put(void* base, size_t off, uint32_t v);

#endif /* LRR_GODS98_RECORD_FOR_BUILDING_MODEL_HPP */
