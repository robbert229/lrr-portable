// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what entity_stats uses from entity.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GAME_ENTITY_FOR_ENTITY_STATS_HPP
#define LRR_GAME_ENTITY_FOR_ENTITY_STATS_HPP

#define ENTITY_INDEXED_VALUE_GROUPS 11

#define ENTITY_INDEXED_VALUE_CAPACITY 3

typedef struct EntityIndexedValueGroup {
    uint32_t values[ENTITY_INDEXED_VALUE_CAPACITY];
    uint32_t count;
} EntityIndexedValueGroup;

#if defined(_MSC_VER) && defined(_M_IX86)
#define ENTITY_STATS_FLOAT_READ(e, off) \
    (*(const float*)((const uint8_t*)(e)->stats_record + (off)))
#define ENTITY_STATS_WORD_READ(e, off) \
    (*(const uint32_t*)((const uint8_t*)(e)->stats_record + (off)))
#else
#define ENTITY_STATS_FLOAT_READ(e, off) stats_rec_float((e), (off))
#define ENTITY_STATS_WORD_READ(e, off) stats_rec_word((e), (off))
#endif

extern int s_stats_cell_span[STATS_TYPE_TABLE_TYPES][STATS_TYPE_TABLE_ROWS];
uint32_t entity_indexed_record_word_get(int group, int index, int slot, uint32_t offset);
const uint8_t* stats_rec(const EntitySlot* e);
uint32_t stats_table_read(int type, int row, int off, int stride_index);
float stats_rec_float(const EntitySlot* e, int off);
uint32_t stats_rec_word(const EntitySlot* e, int off);

#endif /* LRR_GAME_ENTITY_FOR_ENTITY_STATS_HPP */
