// SPDX-License-Identifier: LGPL-3.0-or-later
/* Priorities.c (original translation unit GAME-034, finding 505): the
 * priority-list panel. It loads the priority images, order and positions
 * from the config, draws the list, and handles hover, selection and
 * reordering.
 *
 * C-compatible header (ADR 0005). Priorities.c is compiled as C in the
 * matching build and as C++ in the native port. */
#ifndef LEGORRR_GAME_PRIORITIES_H
#define LEGORRR_GAME_PRIORITIES_H

#include "gods98/block.h"
#include "gods98/images.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The 27 AI priority types; the value also marks "no priority". */
#define PRIORITY_TYPE_COUNT 27

/* The priority-list panel (panel index 8). */
#define PRIORITIES_PANEL 8

/* PriorityImagesConfigLoad (FUN_0045cdc0): read `<game>::PriorityImages`,
 * one `Type  ToolTip[|Sfx]:Image:PressedImage:OffImage` row per type. */
void priorities_load_images(BlockFile* config, const char* game_name);

/* PriorityLabelLayoutApply (FUN_0045cf00): restore the list order, order
 * values and on/off states read by priorities_load. */
void priorities_reset(void);

/* PrioritiesConfigLoad (FUN_0045cf50): read `<game>::<level>::Priorities`.
 * Each `Type  TRUE|FALSE` row appends the type to the list; TRUE turns the
 * priority off. Always returns 1. */
int priorities_load(BlockFile* config, const char* game_name,
                    const char* level_name);

/* PrioritiesImagePositionsConfigLoad (FUN_0045d060): read the 27 list
 * offsets `<game>::PrioritiesImagePositions::<n>  x,y`. */
void priorities_load_positions(BlockFile* config, const char* game_name);

/* RegionContainmentScanUpdate (FUN_0045d110): hit-test the cursor at
 * (x, y) against the list images. On a hit, raise the hover outline and
 * tool tip, update the selection and, when `apply` is set and the
 * selection holds, reorder the list. Returns 1 on a hit. */
int priorities_handle_cursor(unsigned int x, unsigned int y, int pressed,
                             int released, int apply);

/* PriorityImageSelectionStateUpdate (FUN_0045d300): track the pressed list
 * row. Returns 1 when `index` was already the selected row. */
int priorities_select(unsigned int index, int pressed, int released);

/* RegionBoundsSet (FUN_0045d370): set the hover outline drawn next frame. */
void priorities_set_hover_area(const Point2F* position, unsigned int width,
                               unsigned int height);

/* PriorityPanelRowMoveDispatch (FUN_0045d3c0): reorder by the panel mode
 * in the shared flags word (bit 2 moves to the top, bit 4 moves up one). */
void priorities_change_priority(unsigned int index);

/* PriorityImageOrderMoveToFront (FUN_0045d3f0). */
void priorities_move_to_top(unsigned int index);

/* UIListEntriesSwap (FUN_0045d470): swap with the row above. */
void priorities_move_up(unsigned int index);

/* ConfigEntryEnabledToggleAtIndex (FUN_0045d4f0): turn a priority on or
 * off. */
void priorities_toggle(unsigned int index);

/* UiCursorMoveToChoiceEntry (FUN_0045d550): move the cursor to the
 * "move up" button of row `index` (row 0 has none: its image is used). */
void priorities_move_cursor_to_up_button(unsigned int index);

/* UiCursorMoveToElementCenter (FUN_0045d640): move the cursor to the
 * centre of row `index`'s image. */
void priorities_move_cursor_to_image(unsigned int index);

/* PositionTrackerUpdateAll (FUN_0045d6d0): draw the list and the hover
 * outline. */
void priorities_draw(void);

/* Port-only accessors for tests (not in the original unit). */
void priorities_selection_set(unsigned int index, unsigned int flags);
void priorities_selection_get(unsigned int* index, unsigned int* flags);
void priorities_hover_area_get(Area2F* area);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_PRIORITIES_H */
