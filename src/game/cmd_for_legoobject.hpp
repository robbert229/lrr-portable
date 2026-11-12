// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what legoobject uses from cmd.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GAME_CMD_FOR_LEGOOBJECT_HPP
#define LRR_GAME_CMD_FOR_LEGOOBJECT_HPP

/* DeferredOListEntityQueueAdd (FUN_00437c20) — see cmd.hpp. The bounded
 * portable band retains the eleven-word record and owns duplicated names.
 * Its consumer transfers those names before removing a matching entry. */
#define DEFERRED_OLIST_SLOTS 200

struct DeferredOListEntry {
    uint32_t w[9];    /* entry +0x00 .. +0x20 (the 11-dword stride)   */
    char*    names[2]; /* widened mirrors of entry +0x24 / +0x28       */
};

extern DeferredOListEntry s_deferred_olist[DEFERRED_OLIST_SLOTS];
extern uint32_t s_deferred_olist_next;

#endif /* LRR_GAME_CMD_FOR_LEGOOBJECT_HPP */
