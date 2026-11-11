// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; #496): what bubbles.cpp sees from
 * ai.cpp. A verbatim snapshot of ai_shared.hpp as it was when the
 * #496 migration froze it, so the unit's declarations no longer change
 * with later relocations. Never edited by tools/relocate_unit.py. */
#ifndef LRR_GAME_AI_FOR_BUBBLES_HPP
#define LRR_GAME_AI_FOR_BUBBLES_HPP

#define BUBBLE_ACTION_NAME_COUNT 39

#define BUBBLE_ACTION_NAME_CAPACITY 64

/* --- the object registry windows (see ai.hpp) ------------------------------ */
typedef struct {
    void*    record;
    uint32_t timer;
} AiRegSlot;

#define AI_REG_FLAGGED_SLOTS 20 /* DAT_005579e4 window */

#define AI_REG_TYPE4_SLOTS   20 /* DAT_00557b24 window */

#define AI_REG_TYPE2_SLOTS   50 /* DAT_00557bc4 window */

extern char s_bubble_action_names[BUBBLE_ACTION_NAME_COUNT] [BUBBLE_ACTION_NAME_CAPACITY];
extern BubbleTextureResourceLoadFn s_bubble_texture_load;
extern BubbleTextureInitializeFn s_bubble_texture_initialize;
extern void* s_bubble_texture_context;
extern uint32_t s_bubble_config_global_flags;
extern AiRegSlot s_reg_flagged[AI_REG_FLAGGED_SLOTS];
extern AiRegSlot s_reg_type4[AI_REG_TYPE4_SLOTS];
extern AiRegSlot s_reg_type2[AI_REG_TYPE2_SLOTS];

#endif /* LRR_GAME_AI_FOR_BUBBLES_HPP */
