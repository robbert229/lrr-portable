// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; #496): what grid_cell_records.cpp sees from
 * engine.cpp. A verbatim snapshot of engine_shared.hpp as it was when the
 * #496 migration froze it, so the unit's declarations no longer change
 * with later relocations. Never edited by tools/relocate_unit.py. */
#ifndef LRR_GODS98_ENGINE_FOR_GRID_CELL_RECORDS_HPP
#define LRR_GODS98_ENGINE_FOR_GRID_CELL_RECORDS_HPP

extern int32_t s_application_mode;

extern uint32_t s_window_mode_flag;

extern uint32_t s_overlay_word_03610;
extern uint32_t s_overlay_word_03614;

extern void* s_field_grid_buf;
extern EngineState* s_field_grid_desc;

#endif /* LRR_GODS98_ENGINE_FOR_GRID_CELL_RECORDS_HPP */
