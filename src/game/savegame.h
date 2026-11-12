// SPDX-License-Identifier: LGPL-3.0-or-later
/* C boundary header (ADR 0005 step 6) for Reward.c (GAME-037, finding 560):
 * FrontEnd.c's save entries (savegame.hpp, savegame_from_frontend_unit.hpp).
 * Only C units and src/game/test_c_boundaries.cpp include it; the C++
 * declarations it pairs with have C linkage in place. */
#ifndef LEGORRR_GAME_SAVEGAME_H
#define LEGORRR_GAME_SAVEGAME_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct GameContext;
struct EngineContext;
void sav_tree_load(void* node, void* window_base, void** blocks,
                   uint32_t window, uint32_t flag,
                   const struct EngineContext* eng);        /* 004173e0 */
int save_read_slot_target(uint32_t slot, void* slot_context, int mode); /* 00417a90 */
void* sav_slot_ctx_target(int slot);                        /* 00417e50 */
void* sav_build_ctx_target(void);                           /* 00417e70 */
int save_slot_get_target(void);                             /* 00417e90 */
void save_slot_set_target(int slot);                        /* 00417ea0 */
void save_mark_record(struct GameContext* ctx, int index);  /* 00417eb0 */
int save_write_record_target(int index, const uint8_t* blob); /* 00417f40 */
void* sav_record_ptr_target(int index);                     /* 00417f90 */
void save_commit(struct GameContext* ctx);                  /* 00417fc0 */
void show_level_overlay_target(void);                       /* 00418000 */
void save_state_event_context_set(uint32_t value);          /* 00418110 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_SAVEGAME_H */
