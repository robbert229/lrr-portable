// SPDX-License-Identifier: LGPL-3.0-or-later
/* C boundary header (ADR 0005 step 6) for Reward.c (GAME-037, finding 560):
 * FrontEnd.c's save entries (savegame.hpp, savegame_from_frontend_unit.hpp).
 * Only C units and src/game/test_c_boundaries.cpp include it; the C++
 * declarations it pairs with have C linkage in place. */
#ifndef LEGORRR_GAME_SAVEGAME_H
#define LEGORRR_GAME_SAVEGAME_H

/* FrontEnd.c part 2 (frontend_unit.c, finding 628). In the matching build
 * Front_Levels_UpdateAvailable (004173e0) and
 * Front_Save_WriteCurrentSaveFile (00417fc0) answer these names' callers;
 * the port's prototypes add a trailing argument the original ignores, so
 * the callers take the unit's names (frontend_unit.c's own declarations
 * keep theirs). A macro renames a declaration one for one, so MSVC's
 * symbol numbering does not change. */
#if defined(_MSC_VER) && _MSC_VER < 1200 && !defined(LEGORRR_FRONTEND_UNIT_C)
#define sav_tree_load front_levels_update_available /* 004173e0 */
#define save_commit front_save_write_current_save_file /* 00417fc0 */
#endif

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
/* Front_Save_SetLevelCompleted (00417eb0) takes only the index; in the
 * matching build the C callers pass the original's argument to
 * frontend_unit.c's definition (finding 628). The macro also rewrites
 * this declaration, one for one. */
#if defined(_MSC_VER) && _MSC_VER < 1200 && !defined(__cplusplus) && \
    !defined(LEGORRR_FRONTEND_UNIT_C)
#define save_mark_record(ctx, index) front_save_set_level_completed(index)
#endif
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
