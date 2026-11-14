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
/* Front_RunScreenMenuType (00416c80) takes only the menu type; in the
 * matching build the C callers pass the original's argument to
 * frontend_unit.c's definition (finding 628). The macro also rewrites
 * this declaration, one for one. */
#if defined(_MSC_VER) && _MSC_VER < 1200 && !defined(__cplusplus)
#define menu_enter(fe, mode) front_run_screen_menu_type(mode)
#endif
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
extern uint8_t g_front_save_slot_buffer[0xec]; /* DAT_00557418 */
/* frontGlobs (00556fc0) is one struct, defined by frontend_unit.c with its
 * fields' types; these name the fields this header's users read, by their
 * offsets (finding 649). */
extern struct Front_Globs frontGlobs;
#define FRONT_GLOBS_FIELD(type, offset) (*(type*)((char*)&frontGlobs + (offset)))
#define g_front_main_menu_set FRONT_GLOBS_FIELD(void*, 0x004)  /* 00556fc4 */
#if defined(_MSC_VER) && _MSC_VER < 1200
#define g_front_save_tree FRONT_GLOBS_FIELD(void*, 0x038)      /* 00556ff8 */
#else
/* Natively the port keeps its own (frontend_port.cpp). */
extern void* g_front_save_tree;           /* DAT_00556ff8 */
#endif
#define g_front_save_menu_result FRONT_GLOBS_FIELD(int, 0x544) /* 00557504 */
#define g_front_save_menu_open FRONT_GLOBS_FIELD(int, 0x548)   /* 00557508 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_FRONTEND_API_H */
