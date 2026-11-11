// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-015 GAME-015: the unit's functions in original address order and the
 * state they own, relocated verbatim by tools/relocate_unit.py (ADR 0006).
 * Body hashes: docs/coverage/relocations/GAME-015.csv. */
/* Hides level_shared.hpp's extern for s_erosion_delay_scale (see there). */
#define LRR_LAVA_EROSION_UNIT
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <limits.h>
#include "game.hpp"
#include "gods98/block.hpp"
#include "gods98/engine.hpp"
#include "gods98/media.hpp"
#include "effects.hpp"
#include "gods98/heap.hpp"
#include "gods98/message.hpp"
#include "gods98/record.hpp"
#include "gods98/resource.hpp"
#include "gods98/sound.hpp"
#include "gods98/simtick.hpp"
#include "gods98/string.hpp"
#include "gods98/surface.hpp"
#include "crt/crt.hpp"
#include "gods98/view.hpp"
#include "gods98/camera.hpp"
#include "terrain_grid.hpp"
#include "entity.hpp"
#include "type_config_ports.hpp"
#include "ai.hpp"
#include "cmd.hpp"
#include "upgrades.hpp"
#include "spawn.hpp"
#include "port_loop_ports.hpp"
#include "frontend.hpp"
#include "level.hpp"
#include "objective.hpp"
#include "util.hpp"
#include "gods98/flic.hpp"
#include "select.hpp"
#include "scene_render.hpp"
#include "tutorial_hud_ports.hpp"
#include "level_for_lava_erosion.hpp"
#include <float.h>
#include <ctype.h>
#include <errno.h>
#include "gods98/texture.hpp"
#include "crt/crt_for_lava_erosion.hpp"
#include "level_for_lava_erosion.hpp"
#include "crt/crt_for_lava_erosion.hpp"
#include "cmd_for_lava_erosion.hpp"

// GLOBAL: LEGORR 0x0049e124
static const float s_erosion_ticks_per_second = 25.0f;
// GLOBAL: LEGORR 0x0049e128
static const float s_erosion_delay_fraction = 0.2f;
// GLOBAL: LEGORR 0x0049e130
static const float s_erosion_stage_quanta = 4.0f;

// GLOBAL: LEGORR 0x004a1ee4
uint32_t s_config_word = 0u;        /* DAT_004a1ee4 */

// GLOBAL: LEGORR 0x004c7eb0
LavaErosionPendingCell s_lava_erosion_pending_cells[EROSION_MARK_COUNT];
// GLOBAL: LEGORR 0x004cbd30
uint32_t s_lava_erosion_pending_count;
// GLOBAL: LEGORR 0x004cbd34
LavaErosionPendingCell s_lava_erosion_stage_coordinates[
    EROSION_MARK_COUNT];
// GLOBAL: LEGORR 0x004cfbb4
uint32_t s_erosion_marks[EROSION_MARK_COUNT];
// GLOBAL: LEGORR 0x004d1af4
float s_lava_erosion_stage_timers[EROSION_MARK_COUNT];
// GLOBAL: LEGORR 0x004d3a34
CellCoordinateTimestamp s_ts_coordinates[LEVEL_TS_COUNT];
// GLOBAL: LEGORR 0x004d5974
int32_t  s_ts_stamps[LEVEL_TS_COUNT];
// GLOBAL: LEGORR 0x004d6914
uint32_t s_ts_active[LEVEL_TS_COUNT];

// GLOBAL: LEGORR 0x004d78b4
float s_erosion_trigger_accumulator;
// GLOBAL: LEGORR 0x004d78b8
float s_erosion_decay_scale;
// GLOBAL: LEGORR 0x004d78bc
float s_erosion_delay_scale;
// GLOBAL: LEGORR 0x004d78c0
int32_t  s_ts_now; /* _DAT_004d78c0: ErodeLockTime * 25 float bits */

/* --- the kind-six message sweep timer (2026-10-25) -------------------------
 * KindSixRandomMessageDispatchSweep (FUN_0040f0e0): subtract the frame delta
 * from the sweep timer, and once it reaches zero draw up to one hundred
 * random grid pairs modulo the current level's width and height; a pair the
 * erosion eligibility/mutation path accepts dispatches message 6 with the
 * coordinate pair and stops the search. Either way the sweep re-arms the timer
 * from one final draw at (draw % 50) + 750.0 and answers draw / 50. The
 * corpus's flag fold over the three timer comparisons pins the positive-
 * timer return to zero (a finite positive trips no comparison; NaN rides
 * the 1024 bit). Deviations: the corpus strides a missing current-level
 * handle blindly and would take a draw modulo zero — the port skips the
 * cell loop without the level (the re-arm draw still runs, the corpus's
 * re-arm is level-independent) and reads the dims through the word model. */
// GLOBAL: LEGORR 0x004d78c4
float s_sweep_timer;

/* Defined below (forward declarations added by tools/relocate_unit.py). */
static LevelState* lava_erosion_current_level_get(void);

// FUNCTION: LEGORR 0x0040e930
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void erosion_timing_state_init(float value_a,
                                                  float value_b,
                                                  float value_c)
{
    __asm {
        fld dword ptr [esp + 8]
        fmul dword ptr [s_erosion_ticks_per_second]
        fld dword ptr [esp + 0ch]
        fld dword ptr [esp + 4]
        fmul dword ptr [s_erosion_ticks_per_second]
        fxch st(2)
        fmul dword ptr [s_erosion_delay_fraction]
        fxch st(1)
        fmul dword ptr [s_erosion_ticks_per_second]
        fxch st(2)
        fstp dword ptr [s_erosion_decay_scale]
        push edi
        xor eax, eax
        fstp dword ptr [s_erosion_delay_scale]
        mov ecx, 7d0h
        mov edi, offset s_erosion_marks
        fstp dword ptr [s_ts_now]
        mov dword ptr [s_lava_erosion_pending_count], eax
        rep stosd
        mov ecx, 3e8h
        mov edi, offset s_ts_active
        rep stosd
        pop edi
        ret
    }
}
#else
void erosion_timing_state_init(float value_a, float value_b, float value_c)
{
    s_erosion_decay_scale = value_a * s_erosion_ticks_per_second;
    s_erosion_delay_scale = value_b * s_erosion_ticks_per_second *
                            s_erosion_delay_fraction;
    const float tick = value_c * s_erosion_ticks_per_second;
    s_lava_erosion_pending_count = 0u;
    memset(s_erosion_marks, 0, sizeof(s_erosion_marks));
    memset(s_ts_active, 0, sizeof(s_ts_active));
    s_erosion_trigger_accumulator = 0.0f;
    __builtin_memcpy(&s_ts_now, &tick, sizeof s_ts_now);
}
#endif

/* Find the first zero in the 2,000-dword DAT_004cfbb4 stage-active table.
 * Stage registration and the pending-cell ticker share this allocator. */
// FUNCTION: LEGORR 0x0040e990
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) int lava_erosion_stage_slot_find_free(int* slot_out)
{
    __asm {
        xor ecx, ecx
        mov eax, offset s_erosion_marks
    lava_erosion_stage_slot_find_free_scan:
        cmp dword ptr [eax], 0
        je lava_erosion_stage_slot_find_free_found
        add eax, 4
        inc ecx
        cmp eax, offset s_lava_erosion_stage_timers
        jb lava_erosion_stage_slot_find_free_scan
        xor eax, eax
        ret
    lava_erosion_stage_slot_find_free_found:
        mov eax, dword ptr [esp + 4]
        mov dword ptr [eax], ecx
        mov eax, 1
        ret
    }
}
#else
int lava_erosion_stage_slot_find_free(int* slot_out)
{
    volatile uint32_t* cursor = s_erosion_marks;
    int slot = 0;
    do {
        if (*cursor == 0u) {
            *slot_out = slot;
            return 1;
        }
        ++cursor;
        ++slot;
    } while (cursor < s_erosion_marks + EROSION_MARK_COUNT);
    return 0;
}
#endif

// FUNCTION: LEGORR 0x0040e9c0
float lava_erosion_delay_from_cell(const int xy[2])
{
#if defined(_MSC_VER) && defined(_M_IX86)
    LevelState* level = g_active_level_retail;
    const uint8_t* voxels =
        (const uint8_t*)level->word[LEVEL_OFF_VOXELS / 4u];
#else
    LevelState* level = lava_erosion_current_level_get();
    const uint8_t* voxels = level != NULL
        ? (const uint8_t*)level->ptr_voxels : NULL;
#endif
    const uint8_t* record;
    uint32_t width;
    uint32_t index;
    int steps;
#if !defined(_MSC_VER) || !defined(_M_IX86)
    if (xy == NULL || level == NULL) return 0.0f;
#endif
#if !defined(_MSC_VER) || !defined(_M_IX86)
    if (voxels == NULL) return 0.0f;
#endif
    width = level->word[LEVEL_OFF_WIDTH / 4u];
    index = (uint32_t)xy[1] * width + (uint32_t)xy[0];
    record = voxels + index * LEVEL_VOXEL_STRIDE;
    steps = 6 - (int)record[6];
    return (float)steps * s_erosion_delay_scale *
           s_erosion_ticks_per_second;
}

/* LavaErosionPendingCellTick (FUN_0040eab0): keep the engine-scale trigger
 * gate, walk the pending coordinates in their live insertion/swap order,
 * then advance all active stage timers. */
// FUNCTION: LEGORR 0x0040eab0
void lava_erosion_pending_cell_tick(float scale)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    LevelState* s = g_active_level_retail;
#else
    LevelState* s = lava_erosion_current_level_get();
#endif
    s_erosion_triggered_this_tick = 0;
    /* The retail thunk draws before it tests the interval, so even frames
     * with no pending trigger advance the shared random stream. */
    const int16_t random_start = (int16_t)rng_thunk_draw15();
    s_erosion_trigger_accumulator += scale;
    if (s_erosion_decay_scale < s_erosion_trigger_accumulator) {
        int free_slot;
        const int have_free_slot =
            lava_erosion_stage_slot_find_free(&free_slot);
        unsigned int n;
        s_erosion_triggered_this_tick = 1;
        if (s != NULL && s_lava_erosion_pending_count != 0u &&
            have_free_slot) {
            const unsigned int slot = (unsigned int)free_slot;
            const unsigned int start = (unsigned int)random_start;
            for (n = 0; n < s_lava_erosion_pending_count; ++n) {
                const unsigned int index =
                    (start + n) % s_lava_erosion_pending_count;
                const int seed_xy[2] = {
                    s_lava_erosion_pending_cells[index].x,
                    s_lava_erosion_pending_cells[index].y
                };
                int candidate_xy[2];
                if (lava_erosion_neighbor_check(seed_xy, candidate_xy) != 0) {
                    uint8_t* cell;
#if defined(_MSC_VER) && defined(_M_IX86)
                    const uint32_t cell_index =
                        (uint32_t)candidate_xy[1] *
                        s->word[LEVEL_OFF_WIDTH / 4u] +
                        (uint32_t)candidate_xy[0];
                    cell = (uint8_t*)s->word[LEVEL_OFF_VOXELS / 4u] +
                           cell_index * LEVEL_VOXEL_STRIDE;
#else
                    cell = level_voxel(s, candidate_xy[0], candidate_xy[1]);
                    if (cell == NULL) continue;
#endif
                    if (level_cell_occupied_at_current(candidate_xy) == 0u ||
                        (cell[0x0c] & 0x10u) != 0u) {
                        float delay;
                        uint32_t secondary;
                        __builtin_memcpy(&secondary, cell + 0x0c,
                                         sizeof secondary);
                        secondary &= ~0x10u;
                        __builtin_memcpy(cell + 0x0c, &secondary,
                                         sizeof secondary);
                        s_lava_erosion_stage_coordinates[slot].x =
                            candidate_xy[0];
                        s_lava_erosion_stage_coordinates[slot].y =
                            candidate_xy[1];
                        delay = lava_erosion_delay_from_cell(candidate_xy);
                        s_lava_erosion_stage_timers[slot] = delay;
                        s_erosion_marks[slot] = 1u;
                        cell[7] = 0u;
                        lava_erosion_pending_cell_add(candidate_xy, 1);
                        message_dispatch(0x1e, 0, 0, candidate_xy);
                        break;
                    }
                    cell_coordinate_timestamp_remember(candidate_xy);
                    cell[0x0c] |= 0x10u;
                }
            }
        }
        s_erosion_trigger_accumulator = 0.0f;
    }

    if (s != NULL) {
        unsigned int slot;
        for (slot = 0; slot < EROSION_MARK_COUNT; ++slot) {
            LavaErosionPendingCell* stage;
            uint8_t* cell;
            int8_t before, after;
            if (s_erosion_marks[slot] == 0u) continue;
            stage = &s_lava_erosion_stage_coordinates[slot];
            before = (int8_t)crt_float_to_long(
                (s_lava_erosion_stage_timers[slot] /
                 lava_erosion_delay_from_cell((const int*)stage)) *
                s_erosion_stage_quanta);
            s_lava_erosion_stage_timers[slot] -= scale;
            after = (int8_t)crt_float_to_long(
                (s_lava_erosion_stage_timers[slot] /
                 lava_erosion_delay_from_cell((const int*)stage)) *
                s_erosion_stage_quanta);
            if (after == before) continue;
 #if defined(_MSC_VER) && defined(_M_IX86)
            {
                const uint32_t cell_index =
                    (uint32_t)stage->y * s->word[LEVEL_OFF_WIDTH / 4u] +
                    (uint32_t)stage->x;
                cell = (uint8_t*)s->word[LEVEL_OFF_VOXELS / 4u] +
                       cell_index * LEVEL_VOXEL_STRIDE;
            }
#else
            cell = level_voxel(s, stage->x, stage->y);
            if (cell == NULL) {
                s_erosion_marks[slot] = 0u;
                continue;
            }
#endif
            cell[7] = (uint8_t)(4 - after);
            if (after == 0) {
                s_erosion_marks[slot] = 0u;
                cell[2] = 6u;
            }
            {
                float position[3];
                float xy[2] = { 0.0f, 0.0f };
                const int position_valid = terrain_cell_world_center_xy_get(
                    s->ptr_terrain, stage->x, stage->y, xy);
                position[0] = xy[0];
                position[1] = xy[1];
                position[2] = position_valid
                    ? terrain_surface_height_interpolate(
                        s->ptr_terrain, xy[0], xy[1]) : 0.0f;
                /* Retail consumes this random vector on every stage-byte
                 * transition, before its switch decides whether the stage
                 * has a visible erosion effect. */
                {
                    float rotation[3];
                    d3drm_vector_random(rotation);
                    if (position_valid && after >= 0 && after <= 3) {
                        lava_effect_select_and_place(after + 4, position,
                                                     rotation);
                    }
                }
            }
            cell_init_pass(s, (float)stage->x, (uint32_t)stage->y);
        }
    }

    /* The lock entries share the configured ErodeLockTime float word. */
    {
        unsigned int i;
        for (i = 0; i < EROSION_TICKET_COUNT; ++i) {
            if (s_ts_active[i] != 0u) {
                float remaining;
                __builtin_memcpy(&remaining, &s_ts_stamps[i],
                                 sizeof remaining);
                remaining -= scale;
                __builtin_memcpy(&s_ts_stamps[i], &remaining,
                                 sizeof remaining);
                if (remaining < 0.0f) s_ts_active[i] = 0u;
            }
        }
    }
}
// FUNCTION: LEGORR 0x0040ee00
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void cell_coordinate_timestamp_remember(const int xy[2])
{
    __asm {
        xor ecx, ecx
        mov eax, offset s_ts_active
    cell_coordinate_timestamp_remember_scan:
        cmp dword ptr [eax], 0
        je cell_coordinate_timestamp_remember_store
        add eax, 4
        inc ecx
        cmp eax, 004d78b4h
        jb cell_coordinate_timestamp_remember_scan
        ret
    cell_coordinate_timestamp_remember_store:
        mov eax, dword ptr [esp + 4]
        mov edx, dword ptr [eax]
        mov dword ptr [ecx*8 + s_ts_coordinates], edx
        mov eax, dword ptr [eax + 4]
        mov dword ptr [ecx*8 + s_ts_coordinates + 4], eax
        fld dword ptr [s_ts_now]
        fstp dword ptr [ecx*4 + s_ts_stamps]
        mov dword ptr [ecx*4 + s_ts_active], 1
        ret
    }
}
#else
void cell_coordinate_timestamp_remember(const int xy[2])
{
    volatile uint32_t* active = s_ts_active;
    int index = 0;
    do {
        if (*active == 0u) {
            s_ts_coordinates[index].x = xy[0];
            s_ts_coordinates[index].y = xy[1];
            s_ts_stamps[index] = s_ts_now;
            *active = 1u;
            return;
        }
        ++active;
        ++index;
    } while (active < s_ts_active + LEVEL_TS_COUNT);
    /* full table: stores nothing (faithful) */
}
#endif

/* LavaErosionPendingCellAdd (FUN_0040ee50): maintain the compact 2,000-cell
 * candidate list and mirror membership in voxel flag bit 31. Removal uses
 * swap-with-last and retires the matching stage slot, as in the reference. */
// FUNCTION: LEGORR 0x0040ee50
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void lava_erosion_pending_cell_add(const int xy[2], int add)
{
    __asm {
        mov eax, dword ptr [esp + 8]
        push ebx
        push ebp
        push esi
        push edi
        mov edi, dword ptr [g_active_level_retail]
        test eax, eax
        je lava_erosion_pending_cell_add_remove
        mov esi, dword ptr [esp + 14h]
        mov eax, dword ptr [edi + 14h]
        mov ecx, dword ptr [edi + 0b8h]
        imul eax, dword ptr [esi + 4]
        mov ebx, dword ptr [esi]
        add eax, ebx
        lea eax, dword ptr [eax + eax*8]
        test dword ptr [ecx + eax*8 + 8], 80000000h
        jne lava_erosion_pending_cell_add_done
        mov eax, dword ptr [s_lava_erosion_pending_count]
        cmp eax, 7d0h
        jae lava_erosion_pending_cell_add_done
        mov edx, ebx
        push esi
        mov dword ptr [eax*8 + s_lava_erosion_pending_cells], edx
        mov ecx, dword ptr [s_lava_erosion_pending_count]
        mov eax, dword ptr [esi + 4]
        mov dword ptr [ecx*8 + s_lava_erosion_pending_cells + 4], eax
        mov eax, dword ptr [s_lava_erosion_pending_count]
        inc eax
        mov dword ptr [s_lava_erosion_pending_count], eax
        call map_cell_tracking_flags_clear_and_refresh_current
        mov eax, dword ptr [edi + 14h]
        mov edx, dword ptr [esi]
        imul eax, dword ptr [esi + 4]
        add eax, edx
        add esp, 4
        lea edx, dword ptr [eax + eax*8]
        mov eax, dword ptr [edi + 0b8h]
        mov ecx, dword ptr [eax + edx*8 + 8]
        lea eax, dword ptr [eax + edx*8 + 8]
        or ecx, 80000000h
        mov dword ptr [eax], ecx
        pop edi
        pop esi
        pop ebp
        pop ebx
        ret
    lava_erosion_pending_cell_add_remove:
        mov edx, dword ptr [s_lava_erosion_pending_count]
        mov esi, dword ptr [esp + 14h]
        xor eax, eax
        test edx, edx
        jbe lava_erosion_pending_cell_add_stage_scan
        mov ebx, dword ptr [esi]
        mov ecx, offset s_lava_erosion_pending_cells
    lava_erosion_pending_cell_add_list_scan:
        cmp ebx, dword ptr [ecx]
        jne lava_erosion_pending_cell_add_list_next
        mov ebp, dword ptr [esi + 4]
        cmp ebp, dword ptr [ecx + 4]
        je lava_erosion_pending_cell_add_list_found
    lava_erosion_pending_cell_add_list_next:
        inc eax
        add ecx, 8
        cmp eax, edx
        jae lava_erosion_pending_cell_add_stage_scan
        jmp lava_erosion_pending_cell_add_list_scan
    lava_erosion_pending_cell_add_list_found:
        dec edx
        mov dword ptr [s_lava_erosion_pending_count], edx
        lea ecx, dword ptr [edx*8 + s_lava_erosion_pending_cells]
        mov edx, dword ptr [edx*8 + s_lava_erosion_pending_cells]
        mov dword ptr [eax*8 + s_lava_erosion_pending_cells], edx
        mov ecx, dword ptr [ecx + 4]
        mov dword ptr [eax*8 + s_lava_erosion_pending_cells + 4], ecx
        mov eax, dword ptr [edi + 14h]
        imul eax, dword ptr [esi + 4]
        add eax, dword ptr [esi]
        lea edx, dword ptr [eax + eax*8]
        mov eax, dword ptr [edi + 0b8h]
        mov ecx, dword ptr [eax + edx*8 + 8]
        lea eax, dword ptr [eax + edx*8 + 8]
        and ecx, 7fffffffh
        mov dword ptr [eax], ecx
    lava_erosion_pending_cell_add_stage_scan:
        mov edx, dword ptr [esi]
        xor ecx, ecx
        mov eax, offset s_lava_erosion_stage_coordinates
    lava_erosion_pending_cell_add_stage_loop:
        cmp edx, dword ptr [eax]
        jne lava_erosion_pending_cell_add_stage_next
        mov edi, dword ptr [esi + 4]
        mov ebx, dword ptr [eax + 4]
        cmp edi, ebx
        je lava_erosion_pending_cell_add_stage_found
    lava_erosion_pending_cell_add_stage_next:
        add eax, 8
        inc ecx
        cmp eax, offset s_erosion_marks
        jae lava_erosion_pending_cell_add_timestamp
        jmp lava_erosion_pending_cell_add_stage_loop
    lava_erosion_pending_cell_add_stage_found:
        mov dword ptr [ecx*4 + s_erosion_marks], 0
    lava_erosion_pending_cell_add_timestamp:
        push esi
        call cell_coordinate_timestamp_remember
        add esp, 4
    lava_erosion_pending_cell_add_done:
        pop edi
        pop esi
        pop ebp
        pop ebx
        ret
    }
}
#else
void lava_erosion_pending_cell_add(const int xy[2], int add)
{
    LevelState* level = lava_erosion_current_level_get();
    uint8_t* const cell = xy != NULL
        ? level_voxel(level, xy[0], xy[1]) : NULL;
    unsigned int i;
    uint32_t flags;
    if (cell == NULL) return;
    __builtin_memcpy(&flags, cell + 8, sizeof flags);
    if (add == 0) {
        for (i = 0; i < s_lava_erosion_pending_count; ++i) {
            if (s_lava_erosion_pending_cells[i].x == xy[0] &&
                s_lava_erosion_pending_cells[i].y == xy[1]) {
                --s_lava_erosion_pending_count;
                s_lava_erosion_pending_cells[i] =
                    s_lava_erosion_pending_cells[s_lava_erosion_pending_count];
                flags &= 0x7fffffffu;
                __builtin_memcpy(cell + 8, &flags, sizeof flags);
                break;
            }
        }
        for (i = 0; i < EROSION_MARK_COUNT; ++i) {
            if (s_erosion_marks[i] != 0u &&
                s_lava_erosion_stage_coordinates[i].x == xy[0] &&
                s_lava_erosion_stage_coordinates[i].y == xy[1]) {
                s_erosion_marks[i] = 0u;
                break;
            }
        }
        cell_coordinate_timestamp_remember(xy);
    }
    else if ((flags & 0x80000000u) == 0u &&
             s_lava_erosion_pending_count < EROSION_MARK_COUNT) {
        LavaErosionPendingCell* const pending =
            &s_lava_erosion_pending_cells[s_lava_erosion_pending_count++];
        pending->x = xy[0];
        pending->y = xy[1];
        /* The retail caller invokes the coordinate-only entry, which reads
         * DAT_005560d4. Use that mapped path whenever this is the live owner;
         * retain the explicit-level adapter only for offline level work. */
        if (terrain_current_level_resolve() == level) {
            map_cell_tracking_flags_clear_and_refresh_current(xy);
        }
        else {
            map_cell_tracking_flags_clear_and_refresh(level, xy);
        }
        __builtin_memcpy(&flags, cell + 8, sizeof flags);
        flags |= 0x80000000u;
        __builtin_memcpy(cell + 8, &flags, sizeof flags);
    }
}
#endif
// FUNCTION: LEGORR 0x0040efb0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) uint32_t cell_coordinate_timestamp_exists(const int xy[2])
{
    __asm {
        mov edx, dword ptr [esp + 4]
        push esi
        push edi
        mov ecx, offset s_ts_coordinates
        mov eax, offset s_ts_active
    cell_coordinate_timestamp_exists_scan:
        cmp dword ptr [eax], 0
        je cell_coordinate_timestamp_exists_next
        mov esi, dword ptr [edx]
        mov edi, dword ptr [ecx]
        cmp esi, edi
        jne cell_coordinate_timestamp_exists_next
        mov esi, dword ptr [edx + 4]
        mov edi, dword ptr [ecx + 4]
        cmp esi, edi
        je cell_coordinate_timestamp_exists_found
    cell_coordinate_timestamp_exists_next:
        add eax, 4
        add ecx, 8
        cmp eax, 004d78b4h
        jae cell_coordinate_timestamp_exists_missing
        jmp cell_coordinate_timestamp_exists_scan
    cell_coordinate_timestamp_exists_found:
        mov eax, 1
        pop edi
        pop esi
        ret
    cell_coordinate_timestamp_exists_missing:
        pop edi
        xor eax, eax
        pop esi
        ret
    }
}
#else
uint32_t cell_coordinate_timestamp_exists(const int xy[2])
{
    uint32_t* active = s_ts_active;
    CellCoordinateTimestamp* coordinate = s_ts_coordinates;
    if (xy == NULL) return 0u;
    while (*active == 0u || coordinate->x != xy[0] ||
           coordinate->y != xy[1]) {
        ++active;
        ++coordinate;
        if (active >= s_ts_active + LEVEL_TS_COUNT) return 0u;
    }
    return 1u;
}
#endif

/* The retail entry receives seed/output coordinate pairs and reads the
 * active level global. Keep the explicit-level helper above for tests and
 * noncurrent-level queries; the complete target walk lives in this body. */
// FUNCTION: LEGORR 0x0040f000
int lava_erosion_neighbor_check(const int seed_xy[2], int out_xy[2])
{
    LevelState* level = lava_erosion_current_level_get();
    static const int dx[4] = { 0, 1, 0, -1 };
    static const int dy[4] = { -1, 0, 1, 0 };
    const uint8_t* seed;
    int direction;
    if (seed_xy == NULL || level == NULL) return 0;
    seed = level_voxel(level, seed_xy[0], seed_xy[1]);
    if (seed == NULL || seed[2] != 6) return 0;
    for (direction = 0; direction < 4; ++direction) {
        const int nx = seed_xy[0] + dx[direction];
        const int ny = seed_xy[1] + dy[direction];
        const uint8_t* record;
        uint32_t flags = 0u;
        int candidate_xy[2];
        if (out_xy != NULL) {
            out_xy[0] = nx;
            out_xy[1] = ny;
        }
        record = level_voxel(level, nx, ny);
        if (record == NULL) continue;
        __builtin_memcpy(&flags, record + 8, sizeof(flags));
        if ((flags & 0x80000000u) != 0u || (flags & 8u) == 0u ||
            record[6] == 0u) continue;
        candidate_xy[0] = nx;
        candidate_xy[1] = ny;
        if (cell_coordinate_timestamp_exists(candidate_xy) == 0u)
            return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0040f0e0
uint32_t kind_six_random_sweep_dispatch(float delta)
{
    s_sweep_timer = s_sweep_timer - delta;
    const uint32_t neg = (s_sweep_timer < 0.0f) ? 1u : 0u;
    const uint32_t nan = (s_sweep_timer != s_sweep_timer) ? 1u : 0u;
    const uint32_t zero = (s_sweep_timer == 0.0f) ? 1u : 0u;
    uint32_t answer = (neg << 8) | (nan << 10) | (zero << 14);
    if (neg != 0u || zero != 0u) {
        answer = 0;
        const LevelState* lvl =
            (s_ctx != NULL) ? (const LevelState*)s_ctx->level : NULL;
        const int w = (lvl != NULL) ? level_int(lvl, LEVEL_OFF_WIDTH) : 0;
        const int h = (lvl != NULL) ? level_int(lvl, LEVEL_OFF_HEIGHT) : 0;
        if (w > 0 && h > 0) {
            for (uint32_t n = 0; n < 100; ++n) {
                int cell[2];
                cell[0] = (int)(short)rng_thunk_draw15() % w;
                cell[1] = (int)(short)rng_thunk_draw15() % h;
                const int spawn = lava_erosion_cell_eligible_and_mutate(
                    cell, 1);
                if (spawn != 0) {
                    message_dispatch(6, 0, 0, cell);
                    break;
                }
            }
        }
        const uint32_t draw = rng_thunk_draw15();
        answer = (uint32_t)((int)(short)draw / 0x32);
        s_sweep_timer = (float)((int)(short)draw % 0x32) - -750.0f;
    }
    return answer;
}

/* FUN_0040f190: gate an erosion candidate, form a four-direction mask from
 * its walkable neighbors, and mutate only when no neighbor carries the
 * protected state/flag and at least one has a clear 0xc0 flag mask. */
// FUNCTION: LEGORR 0x0040f190
int lava_erosion_cell_eligible_and_mutate(const int xy[2], int hint)
{
    static const int dx[4] = { 0, 1, 0, -1 };
    static const int dy[4] = { -1, 0, 1, 0 };
    unsigned int mask = 0;
    LevelState* level = lava_erosion_current_level_get();
    int valid = 1;
    int clear_mask_neighbors = 0;
    int i;
    if (xy == NULL || lava_erosion_neighbor_eligible_current(xy) == 0) {
        return 0;
    }
    for (i = 0; i < 4; ++i) {
        const int nx = xy[0] + dx[i];
        const int ny = xy[1] + dy[i];
        if (level_cell_walkable_flag(level, nx, ny)) {
            if (level_cell_state_is_1(level, nx, ny) ||
                level_cell_flag_20_bounded(level, nx, ny) != 0u) {
                valid = 0;
            }
            mask |= 1u << i;
            if (level_cell_mask_c0(level, nx, ny) == 0u) {
                clear_mask_neighbors++;
            }
        }
    }
    if (valid && mask != 0u && clear_mask_neighbors != 0) {
        lava_erosion_neighbor_mutate(xy, mask, hint);
        return 1;
    }
    return 0;
}
// FUNCTION: LEGORR 0x0040f2b0
int lava_erosion_neighbor_eligible_current(const int xy[2])
{
    LevelState* level;
    if (xy == NULL) return 0;
    level = lava_erosion_current_level_get();
    if (level == NULL) return 0;
    const uint32_t x = (uint32_t)xy[0];
    const uint32_t y = (uint32_t)xy[1];
    const uint32_t width = level->word[LEVEL_OFF_WIDTH / 4u];
    const uint32_t height = level->word[LEVEL_OFF_HEIGHT / 4u];
    const uint32_t index = y * width + x;
    const uint8_t* record;
    uint32_t flags;
    int8_t type;

    /* Retail ABI: one coordinate-pair pointer and the global current level.
     * FUN_00432b50 performs only these upper-bound tests and the bit-3 test;
     * FUN_00432dd0 then reads the selected record without bounds checks. */
    if (!(x < width - 1u && y < height - 1u)) return 0;
    record = (const uint8_t*)level->ptr_voxels +
             index * LEVEL_VOXEL_STRIDE;
    __builtin_memcpy(&flags, record + 8u, sizeof(flags));
    if ((flags & 8u) == 0u || (flags & 0x8000000u) != 0u) return 0;
    type = (int8_t)record[2];
    if (type == 6 || type == 9 || type == 7) return 0;
    if (level->word[LEVEL_OFF_SAFECAVERNS / 4u] != 0u &&
        (flags & 0x4000000u) != 0u) return 0;
    return 1;
}

/* LavaErosionNeighborMutate (FUN_0040f330). The available scene-slot API
 * claims the effect slot; voxel flags, repeat count, recursive propagation,
 * and CellInitPass stay in the level model. */
// FUNCTION: LEGORR 0x0040f330
void lava_erosion_neighbor_mutate(const int xy[2], unsigned int direction,
                                  int propagate)
{
    static const int edge_for_type[15] = {
        -1, 0, 1, 1, 2, 2, 2, 3, 3, 0, 1, 2, 3, 1, 0
    };
    static const int group_for_type[15] = {
        -1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0
    };
    LevelState* level = lava_erosion_current_level_get();
    uint8_t* cell;
    uint32_t flags;
    int type;
    int edge;
    int changed = 0;
    if (xy == NULL || level == NULL) return;
    cell = level_voxel(level, xy[0], xy[1]);
    if (cell == NULL) return;
    /* FUN_0040f330 drains the cell transition state through 0x432c90 before
     * clearing its work flag and attempting the directional mutation. */
    while (terrain_cell_state_update(level, xy) == 0) {
    }
    if (s_lava_erosion_work_clear != NULL) {
        s_lava_erosion_work_clear(xy[0], xy[1], 0x2f,
                                  s_lava_erosion_hook_context);
    } else {
        terrain_cell_work_flag_clear(level, xy, 0x2fu);
    }
    /* FUN_0040f330 switches on the complete direction mask.  The values
     * 1..14 map to one of four scene edges and one of two edge endpoints. */
    type = direction < 15u ? (int)direction : -1;
    edge = type < 15 ? edge_for_type[type] : -1;
    /* Retail reaches its repeat-count, propagation and CellInitPass tail for
     * every direction mask. Mask 15 has no recovered switch arm, so retain
     * those CPU effects while leaving only its unresolvable scene edge out. */
    if (edge >= 0) {
        if (s_lava_erosion_edge_apply != NULL) {
            changed = s_lava_erosion_edge_apply(xy[0], xy[1], direction, edge,
                                                 s_lava_erosion_hook_context);
        } else {
            float position[2];
            if (terrain_cell_world_center_xy_get(
                    level->ptr_terrain,
                    xy[0], xy[1], position)) {
                const float height = terrain_surface_height_interpolate(
                    level->ptr_terrain,
                    position[0], position[1]);
                changed = scene_slot_pose_scale_register(
                    group_for_type[type], (uint32_t)xy[0], (uint32_t)xy[1],
                    position[0], position[1], height, 0.0f, 0.0f);
            }
        }
    }
    if (changed != 0) {
        __builtin_memcpy(&flags, cell + 8, sizeof flags);
        flags |= 0x800u | 0x1000u;
        __builtin_memcpy(cell + 8, &flags, sizeof flags);
        level_tile_mark(level, xy[0], xy[1], 1);
        if (s_lava_erosion_cell_change != NULL) {
            s_lava_erosion_cell_change(xy[0], xy[1], 1,
                                       s_lava_erosion_hook_context);
        }
        if (s_lava_erosion_sound != NULL) {
            s_lava_erosion_sound(xy[0], xy[1], s_lava_erosion_hook_context);
        } else {
            float position[3];
            float cell_xy[2];
            if (terrain_cell_world_center_xy_get(
                    level->ptr_terrain,
                    xy[0], xy[1], cell_xy)) {
                position[0] = cell_xy[0];
                position[1] = cell_xy[1];
                position[2] = terrain_surface_height_interpolate(
                    level->ptr_terrain,
                    cell_xy[0], cell_xy[1]);
                sound_submission_route_wrapper(NULL, 0x28, 0, 0, position);
            }
        }
    }
    if (propagate != 0) {
        uint32_t count;
        __builtin_memcpy(&count, cell + 0x24, sizeof count);
        ++count;
        __builtin_memcpy(cell + 0x24, &count, sizeof count);
        if (count >= s_lava_erosion_mutation_threshold) {
            lava_erosion_eight_neighbor_propagate(xy, direction);
            if (s_lava_erosion_event != NULL) {
                s_lava_erosion_event(xy[0], xy[1],
                                     s_lava_erosion_hook_context);
            } else {
                message_dispatch(6, 0, 0, xy);
            }
        }
    }
    /* The retail routine ends its CPU mutation path with CellInitPass; it
     * does not reset the surface type or set the terrain block dirty bit. */
    cell_init_pass(level, (float)xy[0], (uint32_t)xy[1]);
}

// FUNCTION: LEGORR 0x0040f5e0
void global_config_value_set(uint32_t v)
{
    s_config_word = v;
}

// FUNCTION: LEGORR 0x0040f5f0
void lava_erosion_eight_neighbor_propagate(const int xy[2],
                                           unsigned int direction)
{
    static const int dx[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };
    static const int dy[8] = { 1, 1, 0, -1, -1, -1, 0, 1 };
    LevelState* level = lava_erosion_current_level_get();
    int i;
    if (xy == NULL || level == NULL) return;
    for (i = 0; i < 8; ++i) {
        int neighbor[2];
        neighbor[0] = xy[0] + dx[i];
        neighbor[1] = xy[1] + dy[i];
        if (level_voxel(level, neighbor[0], neighbor[1]) != NULL) {
            if (lava_erosion_cell_eligible_and_mutate(neighbor, 0) == 0 &&
                lava_erosion_neighbor_eligible_current(neighbor) != 0) {
                lava_erosion_neighbor_mutate(neighbor, direction, 0);
            }
        }
    }
}
/* Retail erosion entry points read DAT_005560d4 on every call. Keep the
 * level-load sidecar as a fallback for explicit offline work, while letting
 * address-mapped current-level paths follow the live game/map owner. */
static LevelState* lava_erosion_current_level_get(void)
{
    LevelState* level = level_current_owner_get();
    return level != NULL ? level : s_lava_erosion_level_context;
}
