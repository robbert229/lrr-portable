// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; #496): what dependencies.cpp sees from
 * dependency_grid.cpp. A verbatim snapshot of dependency_grid_shared.hpp as it was when the
 * #496 migration froze it, so the unit's declarations no longer change
 * with later relocations. Never edited by tools/relocate_unit.py. */
#ifndef LRR_GAME_DEPENDENCY_GRID_FOR_DEPENDENCIES_HPP
#define LRR_GAME_DEPENDENCY_GRID_FOR_DEPENDENCIES_HPP

namespace unported_ledger_stubs {
typedef struct DependencyGridLevelStorage {
    DependencyGridState state;
    DependencyGridCategoryCandidates candidates;
    DependencyGridTuple* tuples[DEPENDENCY_GRID_ROWS]
        [DEPENDENCY_GRID_COLUMNS][DEPENDENCY_GRID_VARIANTS + 1u];
    uint32_t tuple_capacity[DEPENDENCY_GRID_ROWS]
        [DEPENDENCY_GRID_COLUMNS][DEPENDENCY_GRID_VARIANTS + 1u];
} DependencyGridLevelStorage;
} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {
LevelState* dependency_grid_current_level(void);

DependencyGridLevelStorage* dependency_grid_current_storage(void);

int dependency_grid_cell_entries_ensure_loaded( DependencyGridState* state, uint32_t row, uint32_t column, uint32_t variant);
} // namespace unported_ledger_stubs

#endif /* LRR_GAME_DEPENDENCY_GRID_FOR_DEPENDENCIES_HPP */
