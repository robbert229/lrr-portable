// SPDX-License-Identifier: LGPL-3.0-or-later
/* Game layer — terrain grid route search (finding 447).
 *
 * Ports the corpus's route-search cluster and the terrain predicates it
 * runs on the current level's voxel grid:
 *
 *   GridRouteSearchAdapter            @ 0x00440c30 -> path_route_adapter
 *   TerrainGridPathSearch             @ 0x00440c70 -> path_grid_search
 *   TerrainNeighborPathCheck          @ 0x004410f0 -> path_neighbor_search
 *   TerrainCellRouteAndOccupancyCheck @ 0x00431da0 -> path_route_check
 *   TerrainCoordinateAndOccupancyCheck@ 0x00433090 -> path_coord_probe
 *   EntityRouteSetupAndMarkerAssociate@ 0x00441700 -> path_route_install
 *
 * The two searches return *freshly allocated* parallel x/y coordinate
 * arrays plus a count (0 = no route); the caller owns the arrays and frees
 * them with free(), and routes them into an entity with path_route_install.
 * TerrainGridPathSearch is a breadth-first expansion over the four cardinal
 * neighbours storing per-cell visit depth; TerrainNeighborPathCheck is a
 * cost-relaxation expansion (1.0 per step, 0.5 when the cell one further
 * step in the same direction passes path_coord_probe's 0x20000000 probe)
 * that reconstructs by walking the decreasing-cost gradient and falls back
 * to TerrainGridPathSearch when its first step is not cardinally adjacent.
 *
 * Both searches read grid state through path_route_check, which applies the
 * corpus's terrain-cell eligibility rules (cell type/flags plus the
 * entity's +0x334 stats-block words, mirrored on EntitySlot as
 * stats_word/stats_state/stats_capability, and the +0x36c linked target as
 * EntitySlot.route_link). The masks that lack an established meaning in the
 * findings are kept literal, corpus-faithful, exactly like the rest of the
 * repo (finding 317's discipline).
 *
 * Corpus deviations, all documented at each site:
 *  - the corpus work queues hold 100 five-word records with no capacity
 *    guard (finding 447 open item); the port sizes its queues to the grid
 *    cell count and returns 0 on overflow instead of writing out of bounds;
 *  - the corpus returns 1 without filling its output pointers when the
 *    paired route arrays cannot be allocated; the port returns 0,
 *  - the corpus reads OOB at the grid edges in places; the port bounds-
 *    guards those reads (level_tile_type precedent),
 *  - path_route_check's beyond-cell probe, and the west/north neighbour
 *    type probes, are the only probe reads; every other corpus mask is a
 *    literal.
 *
 * The module is not wired into a move command yet: the port's move path
 * (ai_path_build) runs the wave-flood builder (finding 10), while this
 * cluster feeds the corpus's *route* command family (ObjectPathSubmit
 * ToGridTarget, EntityNeighborCellMoveCommandHandle) — an unported seam,
 * kept at the search boundary (same pattern as the port's other leaf
 * ports).
 */
#ifndef LEGORRR_GAME_PATH_HPP
#define LEGORRR_GAME_PATH_HPP

#include <stdint.h>

#include "game.hpp"
#include "level.hpp"

#ifdef __cplusplus
extern "C" {
#endif

/* Cell flag word (voxel record +8) bits the route predicates read. The
 * meanings that the findings do not assign stay literal; 0x10 and 0x40 are
 * the level.hpp tile flags, 0x8 and 0x20000000 are the finding 07/447
 * predug/probe bits. */
#define PATH_FLAG_WALLED        0x00000008u /* route pass req. + corner-cut check */
#define PATH_FLAG_TRAVERSABLE   0x00000010u /* TILE_FLAG_WALKABLE (minion result 3) */
#define PATH_FLAG_BLOCKING      0x00000040u /* TILE_FLAG_BLOCKING (mode != 0 only) */
#define PATH_FLAG_0x200         0x00000200u /* type-6-gate companion (literal) */
#define PATH_FLAG_BLOCKED       0x00000400u /* hard reject */
#define PATH_FLAG_0x2000        0x00002000u /* NULL-entity result-2 marker (literal) */
#define PATH_FLAG_PROBE         0x20000000u /* TerrainCoordinateAndOccupancyCheck */
#define PATH_TYPE_LAVA          6           /* type byte: '\x06' gate */
#define PATH_TYPE_WALL          7           /* type byte: '\a' corner wall */
#define PATH_TYPE_IDLE          '\t'        /* type byte: idle-registration tile */

/* TerrainCoordinateAndOccupancyCheck @ 0x00433090: bounds-guarded read of
 * the cell's 0x20000000 flag — the beyond-cell probe the half-cost rule of
 * path_neighbor_search and the tie-break of its reconstruction consult. */
int path_coord_probe(const LevelState* grid, int x, int y);

/* TerrainCellRouteAndOccupancyCheck @ 0x00431da0: the terrain-cell
 * eligibility predicate both searches expand through. `ent` may be NULL
 * (route with no owner — the corpus's NULL-entity default gate words
 * apply). (cx,cy) is the candidate cell, (px,py) the probe cell the
 * caller supplies (candidate + direction for the searches), `mode`
 * param_6 (the searches pass 0). Returns 0 = blocked, 1 = pass,
 * 2 = corner-cut pass, 3 = traversable-flagged minion pass. */
int path_route_check(const LevelState* grid, const EntitySlot* ent,
                     int cx, int cy, int px, int py, int mode);

/* Route-search callback (corpus `code*` param_9): called with the
 * candidate cell when it passes path_route_check; return nonzero to
 * terminate the search at that cell. When a callback is supplied the
 * search does not require the target cell itself to be routable. */
typedef int (*PathCheckFn)(const EntitySlot* ent, const int cell[2],
                           void* data);

/* TerrainGridPathSearch @ 0x00440c70: BFS expansion over the four
 * cardinal neighbours (N,E,S,W) storing per-cell visit depth. On success
 * allocates `*out_xroad`/`*out_yroad` (both `*out_count` ints, start at
 * [0], target at [count-1], 4-adjacent) and returns 1; returns 0 and
 * leaves the outputs as NULL/0 when the start or target is off-grid, the
 * target fails path_route_check (no callback), the target is unreachable,
 * or the work state cannot be allocated. */
int path_grid_search(const LevelState* grid, const EntitySlot* ent,
                     int sx, int sy, int tx, int ty,
                     int** out_xroad, int** out_yroad, int* out_count,
                     PathCheckFn cb, void* cb_data);

/* TerrainNeighborPathCheck @ 0x004410f0: cost-relaxation expansion
 * (cost 1.0 per step, 0.5 when the beyond-cell probe flag is set) with
 * reconstruction by decreasing cost; falls back through path_grid_search
 * when the first step is not cardinally adjacent. Same contract and
 * failure returns as path_grid_search. */
int path_neighbor_search(const LevelState* grid, const EntitySlot* ent,
                         int sx, int sy, int tx, int ty,
                         int** out_xroad, int** out_yroad, int* out_count,
                         PathCheckFn cb, void* cb_data);

/* GridRouteSearchAdapter @ 0x00440c30: the eight-argument convenience
 * route into path_neighbor_search with a NULL callback/context. */
int path_route_adapter(const LevelState* grid, const EntitySlot* ent,
                       int sx, int sy, int tx, int ty,
                       int** out_xroad, int** out_yroad, int* out_count);

/* EntityRouteSetupAndMarkerAssociate @ 0x00441700: install a route-search
 * result into the entity's waypoint list (corpus +0x24 buffer; the port's
 * AiWaypoint mirror). Clear/raise the same entity flag mirrors the corpus
 * writes (+0x3ec bit 0x40 clear, +0x3e8 bit 0x400 clear, +0x3e0 bit 1).
 * The corpus's per-entry random offsets, ground-height last point, route
 * marker direction byte (finding 270) and action-sequence
 * reset are TODO(corpus) — the installed list is plain {x,y,pending=1}
 * rows the existing mover walks. Returns 1 on success; 0 when the entity
 * is NULL, the FUN_0043baf0 gate blocks (any mask bit set across +0x3e0/
 * +0x3e4), or the count exceeds AI_WAYPOINTS_MAX. */
int path_route_install(GameContext* ctx, EntitySlot* e, const int* xroad,
                       const int* yroad, int count);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_PATH_HPP */
