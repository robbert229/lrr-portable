// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private to src/game/ai.cpp and the units relocated out of it (ADR 0006
 * decision 4; tools/relocate_unit.py). Statics both sides use have
 * external linkage through the declarations below. Include it only from
 * those files: it is not a shared header. */
#ifndef LRR_GAME_AI_SHARED_HPP
#define LRR_GAME_AI_SHARED_HPP

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

#endif /* LRR_GAME_AI_SHARED_HPP */
