// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what message_unit uses from cmd.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GAME_CMD_FOR_MESSAGE_UNIT_HPP
#define LRR_GAME_CMD_FOR_MESSAGE_UNIT_HPP

/* Cheat/debug keybinds (FUN_00452010). */
/* The byte key table at 0x004ff0bc has twelve slots before the five-dword
 * records beginning at 0x004ff0c8.  Keep the portable table within that
 * retail storage window. */
#define KEYBIND_MAX 12

typedef struct KeyBind {
    int key;
    int op, a1, a2, a3, a4;
} KeyBind;

void cmd_keybind_tick(int (*keyheld)(int key));
GameContext* pending_scan_context_get(void);
void selection_drag_area_command_create(EntitySlot* const* entities, unsigned int count, const int cell_xy[2], int replace);
extern KeyBind s_keybinds[KEYBIND_MAX];
extern int s_keybind_count;

#endif /* LRR_GAME_CMD_FOR_MESSAGE_UNIT_HPP */
