// SPDX-License-Identifier: LGPL-3.0-or-later
/* Terrain grid route search (finding 447) — see path.hpp.
 *
 * Every routine cites its corpus address; masks that the findings do not
 * assign a meaning to are kept literal (finding 317's discipline). The
 * corpus work state for both searches (DAT_004dee04 / DAT_004dee00) is
 * process-global and reallocated per map size; the port scopes the state
 * inside each call for testability, with the same zeroed-cell semantics.
 */
#include <stdlib.h>
#include <string.h>

#include "path.hpp"

#define PATH_BEST_INIT 10004.0f /* corpus 0x461c4000 bit pattern */

/* The four cardinal expansion directions in corpus order (TerrainGrid
 * PathSearch's local_fc0 table and TerrainNeighborPathCheck's inline
 * offsets): N, E, S, W. */
static const int s_dir[4][2] = { { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 } };

static int iabs(int v)
{
    return v < 0 ? -v : v;
}

/* TerrainCoordinateAndOccupancyCheck @ 0x00433090. */
int path_coord_probe(const LevelState* grid, int x, int y)
{
    if (grid == NULL || x < 0 || y < 0 || x >= level_width(grid) ||
        y >= level_height(grid)) {
        return 0;
    }
    uint32_t flags;
    __builtin_memcpy(&flags, level_voxel(grid, x, y) + 8, sizeof(flags));
    return (flags & PATH_FLAG_PROBE) != 0;
}

/* Read the voxel record's +8 flag word (0 when the cell cannot be
 * addressed — the corpus reads OOB at the edges; the port guards). */
static uint32_t cell_flags(const LevelState* grid, int x, int y)
{
    const uint8_t* rec = level_voxel(grid, x, y);
    if (rec == NULL) {
        return 0;
    }
    uint32_t flags;
    __builtin_memcpy(&flags, rec + 8, sizeof(flags));
    return flags;
}

/* TerrainCellRouteAndOccupancyCheck @ 0x00431da0.
 *
 * Corpus shape (entity flag words from the +0x334 stats block via
 * EntityStatsWordGet/EntityStateFlagsGet/EntityCapabilityFlagsGet @
 * 0x469e60/0x469e80/0x469ea0, mirrored as EntitySlot.stats_word/state/
 * capability; the +0x36c linked target as EntitySlot.route_link):
 *
 *  - entity type 0xb -> blocked; stats_word bit 0x20000 with no linked
 *    target -> blocked; mode 0 on the entity's own cell -> pass;
 *  - cell type == '\t' (idle tile) needs stats bit 0x1000000, any other
 *    type needs bit 0x2000000, else blocked;
 *  - cell type 6 needs state bit 0x200000 clear AND cell flag 0x200 clear;
 *  - cell flag 0x400 (hard) -> blocked; mode != 0 with flag 0x40 (blocking)
 *    -> blocked; capability bit 0x200 clear with record byte +0xc bit 4
 *    -> blocked;
 *  - stats bit 0 clear: cell flag 0x8 required (NULL entity keeps the
 *    corpus's zero defaults for all four gate words); stats bit 0 set:
 *    cell flag 0x10 raises the result to 3, a zero record byte blocks,
 *    and a traversable cell whose west or north neighbour has type 6/7
 *    blocks;
 *  - NULL entity: candidate or probe flag 0x2000 raises the result to 2;
 *  - a probe differing in both axes (diagonal) needs both corner cells'
 *    flag 0x8, else pass with the current result — the searches always
 *    probe one cardinal step further, so the diagonal branch only fires
 *    for direct callers.
 */
int path_route_check(const LevelState* grid, const EntitySlot* ent,
                     int cx, int cy, int px, int py, int mode)
{
    if (grid == NULL) {
        return 0;
    }
    const uint8_t* crec = level_voxel(grid, cx, cy);
    if (crec == NULL) {
        return 0; /* port guard: the corpus reads OOB for a bad candidate */
    }
    uint32_t cflags = cell_flags(grid, cx, cy);

    /* Corpus gate words for the entity branch — NULL keeps the initializer
     * values (uVar10 = 0, local_3c = 1, local_40 = 0, local_38 = 0,
     * local_34 = 0 in the decompile). */
    uint32_t ent_walk_idle = 0;    /* stats_word & 0x1000000          */
    uint32_t ent_walk_normal = 1;  /* stats_word & 0x2000000          */
    uint32_t ent_state_200000 = 0;  /* stats_state & 0x200000          */
    uint32_t ent_cap_200 = 0;      /* stats_capability & 0x200        */
    uint32_t ent_stats_bit0 = 0;   /* stats_word & 1                  */

    int result = 1; /* the corpus's param_3 default */

    if (ent != NULL) {
        if (ent->type == 0xb) {
            return 0;
        }
        ent_walk_idle = ent->stats_word & 0x1000000u;
        ent_walk_normal = ent->stats_word & 0x2000000u;
        ent_state_200000 = ent->stats_state & 0x200000u;
        ent_cap_200 = ent->stats_capability & 0x200u;
        ent_stats_bit0 = ent->stats_word & 1u;
        if (((ent->stats_word & 0x20000u) != 0) && (ent->route_link == NULL)) {
            return 0;
        }
        if ((mode == 0) && (level_cell_of(ent->x) == cx) &&
            (level_cell_of(ent->y) == cy)) {
            return 1; /* the entity's own cell always passes: FUN_004424f0 */
        }
    }

    if (crec[2] == PATH_TYPE_IDLE) { /* '\t' */
        if (ent_walk_idle == 0) {
            return 0;
        }
    } else if (ent_walk_normal == 0) {
        return 0;
    }
    if ((crec[2] == PATH_TYPE_LAVA) && (ent_state_200000 == 0) &&
        ((cflags & PATH_FLAG_0x200) == 0)) {
        return 0;
    }
    if ((cflags & PATH_FLAG_BLOCKED) != 0) {
        return 0;
    }
    if ((mode != 0) && ((cflags & PATH_FLAG_BLOCKING) != 0)) {
        return 0;
    }
    if ((ent_cap_200 == 0) && ((crec[0xc] & 4u) != 0)) {
        return 0;
    }
    if (ent_stats_bit0 == 0) {
        if ((cflags & PATH_FLAG_WALLED) == 0) {
            return 0;
        }
    } else {
        if ((cflags & PATH_FLAG_TRAVERSABLE) != 0) {
            result = 3;
        }
        if (crec[0] == '\0') {
            return 0;
        }
        if ((cflags & PATH_FLAG_TRAVERSABLE) != 0) {
            /* edge-corner probe: west/north neighbours of a traversable
             * cell must not be lava (6) or wall (7); the corpus reads the
             * neighbouring records' +2 byte — the port bounds-guards. */
            int wt = level_tile_type(grid, cx - 1, cy);
            int nt = level_tile_type(grid, cx, cy - 1);
            if (wt == PATH_TYPE_LAVA || nt == PATH_TYPE_LAVA ||
                wt == PATH_TYPE_WALL || nt == PATH_TYPE_WALL) {
                return 0;
            }
        }
    }
    if (ent == NULL) {
        if ((cflags & PATH_FLAG_0x2000) != 0) {
            result = 2;
        }
        uint32_t pflags = cell_flags(grid, px, py);
        if ((pflags & PATH_FLAG_0x2000) != 0) {
            result = 2;
        }
    }
    if ((px != cx) && (py != cy)) {
        /* diagonal probe: both axis-corner cells need the flag-0x8 bit */
        uint32_t f1 = cell_flags(grid, px, cy);
        uint32_t f2 = cell_flags(grid, cx, py);
        if ((f1 & PATH_FLAG_WALLED) != 0 && (f2 & PATH_FLAG_WALLED) != 0) {
            return 2;
        }
    }
    return result;
}

/* TerrainGridPathSearch @ 0x00440c70. */
int path_grid_search(const LevelState* grid, const EntitySlot* ent,
                     int sx, int sy, int tx, int ty,
                     int** out_xroad, int** out_yroad, int* out_count,
                     PathCheckFn cb, void* cb_data)
{
    if (out_xroad == NULL || out_yroad == NULL || out_count == NULL) {
        return 0;
    }
    *out_xroad = NULL;
    *out_yroad = NULL;
    *out_count = 0;
    if (grid == NULL) {
        return 0;
    }
    int w = level_width(grid);
    int h = level_height(grid);
    if ((unsigned)sx >= (unsigned)w || (unsigned)sy >= (unsigned)h) {
        return 0;
    }
    if (cb == NULL) {
        if ((unsigned)tx >= (unsigned)w || (unsigned)ty >= (unsigned)h) {
            return 0;
        }
        if (path_route_check(grid, ent, tx, ty, tx, ty, 0) == 0) {
            return 0;
        }
    }

    /* The corpus's DAT_004dee04 visit-depth map, sized to the grid and
     * zeroed per call; the alternating work queues are the corpus's two
     * 100-record buffers, bound to the grid size here (finding 447). */
    int size = w * h;
    uint32_t* visited =
        (uint32_t*)calloc((size_t)size, sizeof(uint32_t));
    int* qx0 = (int*)malloc((size_t)size * sizeof(int));
    int* qy0 = (int*)malloc((size_t)size * sizeof(int));
    int* qx1 = (int*)malloc((size_t)size * sizeof(int));
    int* qy1 = (int*)malloc((size_t)size * sizeof(int));
    if (visited == NULL || qx0 == NULL || qy0 == NULL || qx1 == NULL ||
        qy1 == NULL) {
        free(visited);
        free(qx0);
        free(qy0);
        free(qx1);
        free(qy1);
        return 0;
    }
    int* qx[2] = { qx0, qx1 };
    int* qy[2] = { qy0, qy1 };
    int qcount[2] = { 0, 0 };
    int active = 0;
    qx[0][0] = sx;
    qy[0][0] = sy;
    qcount[0] = 1;
    visited[sy * w + sx] = 1;

    int depth = 1;
    int found = 0;
    int fx = tx, fy = ty;
    while (found == 0) {
        ++depth;
        int in = qcount[active];
        int outi = active ^ 1;
        int wrote = 0;
        for (int i = 0; i < in; ++i) {
            int cx = qx[active][i];
            int cy = qy[active][i];
            for (int d = 0; d < 4; ++d) {
                int nx = cx + s_dir[d][0];
                int ny = cy + s_dir[d][1];
                if ((unsigned)nx >= (unsigned)w || (unsigned)ny >= (unsigned)h) {
                    continue;
                }
                int idx = ny * w + nx;
                if (visited[idx] != 0) {
                    continue;
                }
                if (path_route_check(grid, ent, nx, ny,
                                     nx + s_dir[d][0], ny + s_dir[d][1],
                                     0) == 0) {
                    continue;
                }
                if (wrote >= size) {
                    free(visited);
                    free(qx0);
                    free(qy0);
                    free(qx1);
                    free(qy1);
                    return 0; /* queue bound (finding 447 open item) */
                }
                qx[outi][wrote] = nx;
                qy[outi][wrote] = ny;
                ++wrote;
                visited[idx] = (uint32_t)depth;
                if (cb == NULL) {
                    if (nx == tx && ny == ty) {
                        found = 1;
                        fx = nx;
                        fy = ny;
                    }
                } else {
                    int cell[2] = { nx, ny };
                    if (cb(ent, cell, cb_data) != 0) {
                        found = 1;
                        fx = nx;
                        fy = ny;
                    }
                }
            }
        }
        qcount[active] = 0;
        active = outi;
        qcount[active] = wrote;
        if (qcount[active] == 0) {
            free(visited);
            free(qx0);
            free(qy0);
            free(qx1);
            free(qy1);
            return 0; /* both queues drained: the target is unreachable */
        }
    }

    /* Reconstruct: parallel x/y arrays, start at [0], target at [count-1],
     * walking back by decreasing visit depth (corpus `local_fe0` walk). */
    int count = depth; /* corpus uVar11 */
    int* xroad = (int*)malloc((size_t)count * sizeof(int));
    int* yroad = (int*)malloc((size_t)count * sizeof(int));
    if (xroad == NULL || yroad == NULL) {
        free(xroad);
        free(yroad);
        free(visited);
        free(qx0);
        free(qy0);
        free(qx1);
        free(qy1);
        return 0; /* the corpus returns 1 with the outputs unset here */
    }
    xroad[0] = sx;
    yroad[0] = sy;
    xroad[count - 1] = fx;
    yroad[count - 1] = fy;
    if (count > 2) {
        int cx = fx, cy = fy;
        for (int k = count - 2; k >= 1; --k) {
            int bx = -1, by = -1;
            for (int d = 0; d < 4 && bx < 0; ++d) {
                int nx = cx + s_dir[d][0];
                int ny = cy + s_dir[d][1];
                if ((unsigned)nx >= (unsigned)w || (unsigned)ny >= (unsigned)h) {
                    continue;
                }
                if (visited[ny * w + nx] == (uint32_t)(k + 1)) {
                    bx = nx;
                    by = ny;
                }
            }
            if (bx < 0) {
                /* the corpus's direction-residue walk can fall into stale
                 * neighbours; the port aborts instead of emitting a broken
                 * route (finding 447 open item). */
                free(xroad);
                free(yroad);
                free(visited);
                free(qx0);
                free(qy0);
                free(qx1);
                free(qy1);
                return 0;
            }
            xroad[k] = bx;
            yroad[k] = by;
            cx = bx;
            cy = by;
        }
    }

    *out_xroad = xroad;
    *out_yroad = yroad;
    *out_count = count;
    free(visited);
    free(qx0);
    free(qy0);
    free(qx1);
    free(qy1);
    return 1;
}

/* TerrainNeighborPathCheck @ 0x004410f0.
 *
 * Cost-relaxation over the same four cardinal neighbours: each accepted
 * cell accumulates 1.0 per step, reduced to 0.5 when the cell one further
 * step in the same direction carries the probe flag; a cell is re-queued
 * whenever its cost drops. The path counter is the corpus's float `iter`
 * (relaxation rounds), so `count` can exceed the true hop count — that is
 * why the reconstruction's first-step adjacency check falls back to
 * path_grid_search. The reconstruction picks the neighbour with the
 * smallest strictly-positive cost delta, breaking ties with the probe, and
 * keeps the previous pick when none qualifies (the corpus's stale stack
 * values, initialised {1, 0}). */
int path_neighbor_search(const LevelState* grid, const EntitySlot* ent,
                         int sx, int sy, int tx, int ty,
                         int** out_xroad, int** out_yroad, int* out_count,
                         PathCheckFn cb, void* cb_data)
{
    if (out_xroad == NULL || out_yroad == NULL || out_count == NULL) {
        return 0;
    }
    *out_xroad = NULL;
    *out_yroad = NULL;
    *out_count = 0;
    if (grid == NULL) {
        return 0;
    }
    int w = level_width(grid);
    int h = level_height(grid);
    if ((unsigned)sx >= (unsigned)w || (unsigned)sy >= (unsigned)h) {
        return 0;
    }
    if (cb == NULL) {
        if ((unsigned)tx >= (unsigned)w || (unsigned)ty >= (unsigned)h) {
            return 0;
        }
        if (path_route_check(grid, ent, tx, ty, tx, ty, 0) == 0) {
            return 0;
        }
    }

    int size = w * h;
    float* cost = (float*)calloc((size_t)size, sizeof(float)); /* 0x4dee00 */
    int* qx0 = (int*)malloc((size_t)size * sizeof(int));
    int* qy0 = (int*)malloc((size_t)size * sizeof(int));
    int* qx1 = (int*)malloc((size_t)size * sizeof(int));
    int* qy1 = (int*)malloc((size_t)size * sizeof(int));
    if (cost == NULL || qx0 == NULL || qy0 == NULL || qx1 == NULL ||
        qy1 == NULL) {
        free(cost);
        free(qx0);
        free(qy0);
        free(qx1);
        free(qy1);
        return 0;
    }
    int* qx[2] = { qx0, qx1 };
    int* qy[2] = { qy0, qy1 };
    int qcount[2] = { 0, 0 };
    int active = 0;
    qx[0][0] = sx;
    qy[0][0] = sy;
    qcount[0] = 1;
    cost[sy * w + sx] = 1.0f; /* corpus 0x3f800000 */

    float iter = 1.0f;
    int stop = 0;
    int found = 0;
    float found_iter = 0.0f;
    do {
        iter += 1.0f;
        int in = qcount[active];
        int outi = active ^ 1;
        int wrote = 0;
        for (int i = 0; i < in; ++i) {
            int cx = qx[active][i];
            int cy = qy[active][i];
            float base = cost[cy * w + cx];
            for (int d = 0; d < 4; ++d) {
                int nx = cx + s_dir[d][0];
                int ny = cy + s_dir[d][1];
                if ((unsigned)nx >= (unsigned)w || (unsigned)ny >= (unsigned)h) {
                    continue;
                }
                int idx = ny * w + nx;
                float step = 1.0f;
                if (path_coord_probe(grid, nx + s_dir[d][0],
                                     ny + s_dir[d][1]) != 0) {
                    step = 0.5f; /* the half-cost probe rule */
                }
                float nc = step + base;
                if ((path_route_check(grid, ent, nx, ny, nx + s_dir[d][0],
                                      ny + s_dir[d][1], 0) != 0) &&
                    (cost[idx] == 0.0f || nc < cost[idx])) {
                    cost[idx] = nc;
                    if (wrote >= size) {
                        free(cost);
                        free(qx0);
                        free(qy0);
                        free(qx1);
                        free(qy1);
                        return 0; /* queue bound (finding 447 open item) */
                    }
                    qx[outi][wrote] = nx;
                    qy[outi][wrote] = ny;
                    ++wrote;
                    if (cb == NULL) {
                        if (nx == tx && ny == ty) {
                            found = 1;
                            found_iter = iter;
                        }
                    } else {
                        int cell[2] = { nx, ny };
                        if (cb(ent, cell, cb_data) != 0) {
                            stop = 1;
                            tx = nx;
                            ty = ny;
                        }
                    }
                }
            }
        }
        qcount[active] = 0;
        active = outi;
        qcount[active] = wrote;
        if (qcount[active] == 0) {
            stop = -1;
        }
    } while (stop == 0);
    if (found) {
        stop = 1;
        iter = found_iter;
    }
    if (stop == -1) {
        free(cost);
        free(qx0);
        free(qy0);
        free(qx1);
        free(qy1);
        return 0;
    }

    int count = (int)iter;
    if (count < 2) {
        free(cost);
        free(qx0);
        free(qy0);
        free(qx1);
        free(qy1);
        return 0; /* cannot hold start + target */
    }
    int* xroad = (int*)malloc((size_t)count * sizeof(int));
    int* yroad = (int*)malloc((size_t)count * sizeof(int));
    if (xroad == NULL || yroad == NULL) {
        free(xroad);
        free(yroad);
        free(cost);
        free(qx0);
        free(qy0);
        free(qx1);
        free(qy1);
        return 0;
    }
    xroad[0] = sx;
    yroad[0] = sy;
    xroad[count - 1] = tx;
    yroad[count - 1] = ty;
    if (count > 2) {
        int cx = tx, cy = ty;
        int bx = 1, by = 0; /* corpus stack residue (in_stack_0x30/0x34) */
        for (int k = count - 2; k >= 1; --k) {
            float best = PATH_BEST_INIT;
            for (int d = 0; d < 4; ++d) {
                int nx = cx + s_dir[d][0];
                int ny = cy + s_dir[d][1];
                if ((unsigned)nx >= (unsigned)w || (unsigned)ny >= (unsigned)h) {
                    continue;
                }
                float delta = cost[cy * w + cx] - cost[ny * w + nx];
                if (delta > 0.0f &&
                    (delta < best ||
                     (delta == best && path_coord_probe(grid, nx, ny) != 0))) {
                    best = delta;
                    bx = nx;
                    by = ny;
                }
            }
            xroad[k] = bx;
            yroad[k] = by;
            cx = bx;
            cy = by;
        }
    }

    /* The corpus frees both arrays and retries through TerrainGridPathSearch
     * when the first reconstructed step is not cardinally adjacent. */
    if ((xroad[0] != xroad[1] || iabs(yroad[1] - yroad[0]) != 1) &&
        (yroad[0] != yroad[1] || iabs(xroad[1] - xroad[0]) != 1)) {
        free(xroad);
        free(yroad);
        free(cost);
        free(qx0);
        free(qy0);
        free(qx1);
        free(qy1);
        return path_grid_search(grid, ent, sx, sy, tx, ty, out_xroad,
                                out_yroad, out_count, cb, cb_data);
    }

    *out_xroad = xroad;
    *out_yroad = yroad;
    *out_count = count;
    free(cost);
    free(qx0);
    free(qy0);
    free(qx1);
    free(qy1);
    return 1;
}

/* GridRouteSearchAdapter @ 0x00440c30. */
int path_route_adapter(const LevelState* grid, const EntitySlot* ent,
                       int sx, int sy, int tx, int ty, int** out_xroad,
                       int** out_yroad, int* out_count)
{
    return path_neighbor_search(grid, ent, sx, sy, tx, ty, out_xroad,
                                out_yroad, out_count, NULL, NULL);
}

/* EntityRouteSetupAndMarkerAssociate @ 0x00441700. */
int path_route_install(GameContext* ctx, EntitySlot* e, const int* xroad,
                       const int* yroad, int count)
{
    (void)ctx;
    if (e == NULL || count < 0 ||
        (count > 0 && (xroad == NULL || yroad == NULL))) {
        return 0;
    }
    if ((e->flags_3e0 & 0x107c33deu) != 0 ||
        (e->flags_3e4 & 0x01c00203u) != 0) {
        return 0; /* FUN_0043baf0 gate (finding 471) */
    }
    if (count > AI_WAYPOINTS_MAX) {
        return 0; /* port bound — the corpus list is heap-grown (0x441a50) */
    }
    e->flags_3ec &= ~0x40u;
    e->wp_count = count;
    e->wp_index = 0;
    for (int i = 0; i < count; ++i) {
        e->wp[i].x = (float)xroad[i];
        e->wp[i].y = (float)yroad[i];
        e->wp[i].pending = 1;
    }
    /* Corpus also writes +0x2fc = 0, per-entry random offsets (0.3..0.7 or
     * 0.5), the ground-height last point, the route-marker direction byte
     * (finding 270) and runs the entity action-sequence reset — the port
     * models the list, the mover flags, and the FUN_0043baf0 gate; the
     * rest stay TODO(corpus). */
    e->flags_3e8 &= ~0x400u;
    e->flags_3e0 |= 0x1u;
    return 1;
}
