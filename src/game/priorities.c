// SPDX-License-Identifier: LGPL-3.0-or-later
/* Priorities.c — original translation unit GAME-034 (text 0045cdc0-0045d7e0,
 * .data 004a8ee8-004a8f24, .bss 00500f00-005013c0; finding 505).
 *
 * Rebuilt in the original's shape (issue #384, ADR 0005): one file for the
 * unit, its functions in original order, its state in one typed globals
 * struct laid out as the original .bss, and the C-compatible subset so the
 * matching build compiles it as C. The native port compiles it as C++.
 *
 * Callee contracts that this reconstruction has not yet converted are used
 * as they are (finding 506 lists the differences from the original):
 * - the config layer returns an array's container record, not its first
 *   item, and its records are read through accessors;
 * - the panel position (indexed_pair_get) is an integer pair, not floats. */
#include <stdio.h>
#include <stdlib.h>

#include "priorities.h"
#include "cmd.h"
#include "frontend.h"
#include "panel.h"
#include "crt/vc5names.h"
#include "gods98/block.h"
#include "gods98/images.h"
#include "gods98/input.h"
#include "gods98/media.hpp"
#include "gods98/simtick.h"
#include "gods98/sound.h"
#include "gods98/string.hpp"

/* Tool-tip slot and message used by the panel. */
#define PRIORITIES_TOOLTIP 7
#define PRIORITIES_MESSAGE_ON 0x10

/* First "move up" button of the priority panel; row n uses button n + 8. */
#define PRIORITIES_BUTTON_UP 9

#define PRIORITIES_FLAG_HOVER   0x1
#define PRIORITIES_FLAG_PRESSED 0x2

/* Order values: the top row is 80, each row below is 5 less. */
#define PRIORITIES_TOP_VALUE 80
#define PRIORITIES_VALUE_STEP 5

typedef struct PrioritiesGlobs {
    char* tool_tips[PRIORITY_TYPE_COUNT];          /* 00500f00 */
    Image* images[PRIORITY_TYPE_COUNT];            /* 00500f6c */
    Image* pressed_images[PRIORITY_TYPE_COUNT];    /* 00500fd8 */
    Image* off_images[PRIORITY_TYPE_COUNT];        /* 00501044 */
    int sounds[PRIORITY_TYPE_COUNT];               /* 005010b0 */
    int list[PRIORITY_TYPE_COUNT];                 /* 0050111c */
    Point2F offsets[PRIORITY_TYPE_COUNT];          /* 00501188 */
    int initial_list[PRIORITY_TYPE_COUNT];         /* 00501260 */
    int initial_values[PRIORITY_TYPE_COUNT];       /* 005012cc */
    int initial_states[PRIORITY_TYPE_COUNT];       /* 00501338 */
    unsigned int count;                            /* 005013a4 */
    Area2F hover_area;                             /* 005013a8 */
    unsigned int selected;                         /* 005013b8 */
    unsigned int flags;                            /* 005013bc */
} PrioritiesGlobs;

/* The row and type whose tool tip was last offered. */
// GLOBAL: LEGORR 0x004a8ee8
static unsigned int s_tool_tip_index = PRIORITY_TYPE_COUNT;
// GLOBAL: LEGORR 0x004a8eec
static int s_tool_tip_type = PRIORITY_TYPE_COUNT;

// GLOBAL: LEGORR 0x00500f00
static PrioritiesGlobs priorities;

// FUNCTION: LEGORR 0x0045cdc0
void priorities_load_images(BlockFile* config, const char* game_name)
{
    BlockRec* item;
    int type;
    char* parts[10];
    char* names[10];
    char line[512];

    for (item = block_first_child(block_subgroup(config,
             string_path_join(game_name, "PriorityImages", NULL)));
         item != NULL; item = block_next_equal_key(item)) {
        if (cmd_kind_by_name(block_name(item), &type)) {
            int count;
            sprintf(line, "%s", block_value(item));
            sound_variant_split(line, parts, ":");
            count = sound_variant_split(parts[0], names, "|");
            priorities.tool_tips[type] = string_duplicate(names[0]);
            if (count == 2) sound_alias_lookup(names[1], &priorities.sounds[type]);
            priorities.images[type] = image_load_bmp_scaled(parts[1], 0, 0);
            priorities.pressed_images[type] = image_load_bmp_scaled(parts[2], 0, 0);
            priorities.off_images[type] = image_load_bmp_scaled(parts[3], 0, 0);
        }
    }
}

// FUNCTION: LEGORR 0x0045cf00
void priorities_reset(void)
{
    unsigned int i;

    for (i = 0; i < priorities.count; i++) {
        priorities.list[i] = priorities.initial_list[i];
        priority_entry_order_value_set(priorities.initial_list[i],
                                       priorities.initial_values[i]);
        priority_entry_state_set(priorities.list[i],
                                 priorities.initial_states[i]);
    }
}

// FUNCTION: LEGORR 0x0045cf50
int priorities_load(BlockFile* config, const char* game_name,
                    const char* level_name)
{
    BlockRec* item;
    int type;

    priorities.count = 0;
    for (item = block_first_child(block_subgroup(config,
             string_path_join(game_name, level_name, "Priorities", NULL)));
         item != NULL; item = block_next_equal_key(item)) {
        if (cmd_kind_by_name(block_name(item), &type)) {
            priorities.list[priorities.count] = type;
            priorities.initial_list[priorities.count] = type;
            priorities.initial_values[priorities.count] =
                PRIORITIES_TOP_VALUE - priorities.count * PRIORITIES_VALUE_STEP;
            priority_entry_order_value_set(type,
                priorities.initial_values[priorities.count]);
            if (block_get_bool(block_value(item)) == 1) {
                priority_entry_state_set(type, 0);
                priorities.initial_states[priorities.count] = 0;
            }
            else {
                priority_entry_state_set(type, 1);
                priorities.initial_states[priorities.count] = 1;
            }
            priorities.count++;
        }
    }
    return 1;
}

// FUNCTION: LEGORR 0x0045d060
void priorities_load_positions(BlockFile* config, const char* game_name)
{
    unsigned int i;
    char number[4];
    const char* value;
    char* parts[16];

    for (i = 0; i < PRIORITY_TYPE_COUNT; i++) {
        _itoa(i + 1, number, 10);
        value = block_get_strptr(config, string_path_join(game_name,
            "PrioritiesImagePositions", number, NULL));
        if (value != NULL) {
            /* The original tokenises the config's own value in place. */
            sound_variant_split((char*)value, parts, ",");
            priorities.offsets[i].x = (float)atoi(parts[0]);
            priorities.offsets[i].y = (float)atoi(parts[1]);
        }
    }
}

// FUNCTION: LEGORR 0x0045d110
int priorities_handle_cursor(unsigned int x, unsigned int y, int pressed,
                             int released, int apply)
{
    unsigned int panel_x;
    unsigned int panel_y;
    unsigned int i;
    Point2F position;

    if (!request_record_ready_flag4_target(PRIORITIES_PANEL) &&
        !request_record_flag8_is_set_target(PRIORITIES_PANEL)) {
        indexed_pair_get(PRIORITIES_PANEL, &panel_x, &panel_y);
        for (i = 0; i < priorities.count; i++) {
            int type = priorities.list[i];
            Image* image = priorities.images[type];
            if (image != NULL) {
                position.x = priorities.offsets[i].x + (float)panel_x;
                position.y = priorities.offsets[i].y + (float)panel_y;
                if (position.x <= (float)x &&
                    (float)x <= position.x + (float)image->width &&
                    position.y <= (float)y &&
                    (float)y <= position.y + (float)image->height) {
                    priorities.flags |= PRIORITIES_FLAG_HOVER;
                    priorities_set_hover_area(&position, image->width - 1,
                                              image->height - 1);
                    if (!pressed) {
                        if (s_tool_tip_index == i && s_tool_tip_type == type) {
                            tooltip_set_content(PRIORITIES_TOOLTIP,
                                                priorities.tool_tips[type]);
                            timed_effect_update_request_set_corpus(
                                PRIORITIES_TOOLTIP);
                            simulation_global_word_set(
                                (uint32_t)priorities.sounds[type]);
                        }
                        else {
                            timed_effect_update_request_set_corpus(0);
                            s_tool_tip_index = i;
                            s_tool_tip_type = type;
                        }
                    }
                    if (priorities_select(i, pressed, released) && apply) {
                        priorities_change_priority(i);
                    }
                    return 1;
                }
            }
        }
        priorities_select(PRIORITY_TYPE_COUNT, pressed, released);
    }
    return 0;
}

// FUNCTION: LEGORR 0x0045d300
int priorities_select(unsigned int index, int pressed, int released)
{
    unsigned int selected = priorities.selected;

    if (pressed) {
        if (priorities.selected == PRIORITY_TYPE_COUNT) {
            if (!released) {
                priorities.selected = index;
                priorities.flags |= PRIORITIES_FLAG_PRESSED;
            }
        }
        else if (priorities.selected == index) {
            priorities.flags |= PRIORITIES_FLAG_PRESSED;
        }
    }
    else priorities.selected = PRIORITY_TYPE_COUNT;
    return selected == index;
}

// FUNCTION: LEGORR 0x0045d370
void priorities_set_hover_area(const Point2F* position, unsigned int width,
                               unsigned int height)
{
    priorities.hover_area.x = position->x;
    priorities.hover_area.y = position->y;
    priorities.hover_area.width = (float)width;
    priorities.hover_area.height = (float)height;
}

// FUNCTION: LEGORR 0x0045d3c0
void priorities_change_priority(unsigned int index)
{
    if (simtick_timer_word & 2) priorities_move_to_top(index);
    else if (simtick_timer_word & 4) priorities_move_up(index);
}

// FUNCTION: LEGORR 0x0045d3f0
void priorities_move_to_top(unsigned int index)
{
    if (index != 0 && index < priorities.count) {
        int type = priorities.list[index];
        unsigned int i;
        for (i = 0; i < index; i++) {
            priorities.list[index - i] = priorities.list[index - i - 1];
            priority_entry_order_value_set(priorities.list[index - i],
                PRIORITIES_TOP_VALUE - (index - i) * PRIORITIES_VALUE_STEP);
        }
        priorities.list[0] = type;
        priority_entry_order_value_set(type, PRIORITIES_TOP_VALUE);
    }
    media_action_feedback_sound_dispatch(3);
}

// FUNCTION: LEGORR 0x0045d470
void priorities_move_up(unsigned int index)
{
    if (index != 0 && index < priorities.count) {
        int type = priorities.list[index];
        priorities.list[index] = priorities.list[index - 1];
        priorities.list[index - 1] = type;
        priority_entry_order_value_set(priorities.list[index],
            PRIORITIES_TOP_VALUE - index * PRIORITIES_VALUE_STEP);
        priority_entry_order_value_set(priorities.list[index - 1],
            PRIORITIES_TOP_VALUE - (index - 1) * PRIORITIES_VALUE_STEP);
        priorities_move_cursor_to_up_button(index - 1);
    }
    media_action_feedback_sound_dispatch(0);
}

// FUNCTION: LEGORR 0x0045d4f0
void priorities_toggle(unsigned int index)
{
    if (index < priorities.count) {
        int on = !priority_entry_state_get(priorities.list[index]);
        priority_entry_state_set(priorities.list[index], on);
        if (on && (priorities.list[index] == 0 || priorities.list[index] == 1)) {
            advisor_text_animation_update(PRIORITIES_MESSAGE_ON, 1, 0);
        }
    }
}

// FUNCTION: LEGORR 0x0045d550
void priorities_move_cursor_to_up_button(unsigned int index)
{
    int button;
    unsigned int panel_x;
    unsigned int panel_y;
    Area2F area;

    switch (index) {
    case 0:
        priorities_move_cursor_to_image(0);
        return;
    case 1: button = PRIORITIES_BUTTON_UP; break;
    case 2: button = PRIORITIES_BUTTON_UP + 1; break;
    case 3: button = PRIORITIES_BUTTON_UP + 2; break;
    case 4: button = PRIORITIES_BUTTON_UP + 3; break;
    case 5: button = PRIORITIES_BUTTON_UP + 4; break;
    case 6: button = PRIORITIES_BUTTON_UP + 5; break;
    case 7: button = PRIORITIES_BUTTON_UP + 6; break;
    case 8: button = PRIORITIES_BUTTON_UP + 7; break;
    default: return;
    }
    indexed_pair_get(PRIORITIES_PANEL, &panel_x, &panel_y);
    ui_entry_bounds_get(PRIORITIES_PANEL, button, &area.x);
    input_cursor_client_position_set(
        (int)(area.x + area.width * 0.5f + (float)panel_x),
        (int)(area.y + area.height * 0.5f + (float)panel_y));
}

// FUNCTION: LEGORR 0x0045d640
void priorities_move_cursor_to_image(unsigned int index)
{
    int type = priorities.list[index];
    unsigned int panel_x;
    unsigned int panel_y;

    if (priorities.images[type] != NULL) {
        Image* image;
        indexed_pair_get(PRIORITIES_PANEL, &panel_x, &panel_y);
        image = priorities.images[type];
        input_cursor_client_position_set(
            (int)(priorities.offsets[index].x + (float)panel_x +
                  (float)(image->width >> 1)),
            (int)(priorities.offsets[index].y + (float)panel_y +
                  (float)(image->height >> 1)));
    }
}

// FUNCTION: LEGORR 0x0045d6d0
void priorities_draw(void)
{
    unsigned int panel_x;
    unsigned int panel_y;
    unsigned int i;
    Point2F position;

    if (!request_record_ready_flag4_target(PRIORITIES_PANEL)) {
        indexed_pair_get(PRIORITIES_PANEL, &panel_x, &panel_y);
        for (i = 0; i < priorities.count; i++) {
            int type;
            Image* image;
            position.x = priorities.offsets[i].x + (float)panel_x;
            position.y = priorities.offsets[i].y + (float)panel_y;
            type = priorities.list[i];
            if (priority_entry_state_get(type)) image = priorities.off_images[type];
            else if ((priorities.flags & PRIORITIES_FLAG_PRESSED) &&
                     priorities.selected == i) {
                image = priorities.pressed_images[type];
            }
            else image = priorities.images[type];
            if (image != NULL) image_display_scaled(image, NULL, &position, NULL);
        }
        if (priorities.flags & PRIORITIES_FLAG_HOVER) {
            position_tracker_box_draw(&priorities.hover_area.x);
            priorities.flags &= ~PRIORITIES_FLAG_HOVER;
        }
        priorities.flags &= ~PRIORITIES_FLAG_PRESSED;
    }
}

void priorities_selection_set(unsigned int index, unsigned int flags)
{
    priorities.selected = index;
    priorities.flags = flags;
}

void priorities_selection_get(unsigned int* index, unsigned int* flags)
{
    *index = priorities.selected;
    *flags = priorities.flags;
}

void priorities_hover_area_get(Area2F* area)
{
    *area = priorities.hover_area;
}
