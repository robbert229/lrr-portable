// SPDX-License-Identifier: LGPL-3.0-or-later
/* C boundary header (ADR 0005 step 6) for Reward.c (GAME-037, finding 560):
 * FrontEnd.c and Pointer.c entries declared in frontend.hpp.
 * Only C units and src/game/test_c_boundaries.cpp include it; the C++
 * declarations it pairs with have C linkage in place. */
#ifndef LEGORRR_GAME_FRONTEND_API_H
#define LEGORRR_GAME_FRONTEND_API_H

#include <stdint.h>
#include "gods98/config.h"

#ifdef __cplusplus
extern "C" {
#endif

struct FrontEnd;
void main_menu_text_item(void);                             /* 004103d0 */
int menu_enter(struct FrontEnd* fe, int mode);              /* 00416c80 */
int frontend_enabled(BlockFile* cfg);                       /* 00416d00 */
int save_slot_open(void);                                   /* 00416d50 */
int is_in_game(void);                                       /* 00416d60 */
int* active_object_list_word(void);                         /* 00416e00 */
void* current_selection_context_get(void);                  /* 00417fe0 */
void current_selection_context_set(void* context);          /* 00417ff0 */
void pointer_cursor_draw_target(uint32_t x, uint32_t y);    /* 0045cc70 */

struct SavListWindow;
int front_level_set_index_of(const struct SavListWindow* win,
                             const char* name);             /* 00416e20 */
/* Front_Save_CopySaveData (00418040) in the original ABI; unannotated
 * adapter (frontend_unit.cpp). */
int front_campaign_snapshot_create(void* snapshot);

/* FrontEnd.c state without a port owner. Port globals stand in (end of
 * frontend_unit.cpp); nothing in the port sets them yet. */
extern char* g_front_end_avi1;            /* DAT_00556fb0 */
extern char* g_front_end_avi2;            /* DAT_00556fb4 */
extern void* g_front_main_menu_set;       /* DAT_00556fc4 */
extern void* g_front_save_tree;           /* DAT_00556ff8 */
extern uint8_t g_front_save_slot_buffer[0xec]; /* DAT_00557418 */
extern int g_front_save_menu_result;      /* DAT_00557504 */
extern int g_front_save_menu_open;        /* DAT_00557508 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_FRONTEND_API_H */
