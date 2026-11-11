// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private to src/game/dependency_grid.cpp and the units relocated out of it (ADR 0006
 * decision 4; tools/relocate_unit.py). Statics both sides use have
 * external linkage through the declarations below. Include it only from
 * those files: it is not a shared header. */
#ifndef LRR_GAME_DEPENDENCY_GRID_SHARED_HPP
#define LRR_GAME_DEPENDENCY_GRID_SHARED_HPP

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

#endif /* LRR_GAME_DEPENDENCY_GRID_SHARED_HPP */
