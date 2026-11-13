// SPDX-License-Identifier: LGPL-3.0-or-later
/* ScrollInfo.c, original translation unit GAME-039 (finding 505), rebuilt
 * in original shape (ADR 0005; finding 611): text 004636b0-00463f00, the
 * 0.5 at 0049e710, the .bss word 005013c0 and the common block 00552140.
 *
 * The information panel's scroll bar. Each scroll record sits on a panel
 * and has an image, two arrow buttons (each with a highlight and a pressed
 * image) and a slider over a range of positions. ScrollInfo_Update moves the
 * position by a button press or by dragging the slider, and lays the slider
 * out again; ScrollInfo_Draw draws the bar, the pressed or highlighted
 * buttons and the slider. Panel.c creates the one record the game uses,
 * for the scrolling message text, and Info.c sets and reads its range.
 *
 * The port's implementation stays in the port tail scrollinfo_port.cpp,
 * unannotated. In the matching build scrollinfo_link_names.h gives this
 * unit's functions the port's names, so every caller reaches them;
 * natively the port tail serves those names. */
#include "scrollinfo_link_names.h"

#include <math.h>
#include <stdarg.h>
#include <stdlib.h>

#include "crt/vc5names.h"

#include "scrollinfo.h"
#include "scrollinfo_imports.h"

/* The unit's functions have C linkage in the native C++ compile too. */
#ifdef __cplusplus
extern "C" {
#endif

/* The unit's state, 0x24 bytes at 00552140: a common block. */
typedef struct ScrollInfo_Globs {
    ScrollInfo_Struct structs[SCROLLINFO_MAXSTRUCTS]; /* 00552140 */
    unsigned int selection;                           /* 00552160 */
} ScrollInfo_Globs;

// GLOBAL: LEGORR 0x005013c0
static int scrollInfoOverSlider;

// GLOBAL: LEGORR 0x00552140
ScrollInfo_Globs scrollInfoGlobs;

// FUNCTION: LEGORR 0x004636b0
void scroll_info_initialise(const char* imageName, unsigned int index,
                            int horizontal, int panel, ...)
{
    va_list args;
    ScrollInfo_Struct* scrollInfo;

    va_start(args, panel);
    while (imageName != NULL) {
        scrollInfo = &scrollInfoGlobs.structs[index];
        scrollInfo->flags = SCROLLINFO_FLAG_ENABLED;
        if (horizontal) scrollInfo->flags |= SCROLLINFO_FLAG_HORIZONTAL;
        else scrollInfo->flags |= SCROLLINFO_FLAG_VERTICAL;
        if ((scrollInfo->image = image_load_bmp_scaled(imageName, 0, 0)) != NULL) {
            scrollInfo->flags |= SCROLLINFO_FLAG_IMAGE;
        }
        imageName = va_arg(args, const char*);
        scrollInfo->panel = panel;
        index = va_arg(args, unsigned int);
        horizontal = va_arg(args, int);
        panel = va_arg(args, int);
    }
    va_end(args);
}

// FUNCTION: LEGORR 0x00463730
void scroll_info_set_xy_offset(unsigned int index, unsigned int xOffset,
                               unsigned int yOffset)
{
    ScrollInfo_Struct* scrollInfo = &scrollInfoGlobs.structs[index];

    scrollInfo->xOffset = (float)xOffset;
    scrollInfo->yOffset = (float)yOffset;
}

// FUNCTION: LEGORR 0x00463770
void scroll_info_add_structure(unsigned int index, unsigned int type,
                               const Area2F* rect, const char* highlightName,
                               const char* pressedName)
{
    ScrollInfo_Struct* scrollInfo = &scrollInfoGlobs.structs[index];
    ScrollInfo_Button* button;

    if (type == 0) {
        button = scrollInfo->buttons[0] = (ScrollInfo_Button*)malloc(sizeof(ScrollInfo_Button));
    } else {
        button = scrollInfo->buttons[1] = (ScrollInfo_Button*)malloc(sizeof(ScrollInfo_Button));
    }
    button->x1 = rect->x;
    button->y1 = rect->y;
    button->x2 = rect->width + rect->x;
    button->y2 = rect->height + rect->y;
    button->highlightImage = NULL;
    button->pressedImage = NULL;
    if (highlightName != NULL) {
        button->highlightImage = image_load_bmp_scaled(highlightName, 0, 0);
    }
    if (pressedName != NULL) {
        button->pressedImage = image_load_bmp_scaled(pressedName, 0, 0);
    }
    button->flags = 0;
}

// FUNCTION: LEGORR 0x00463810
void scroll_info_add_create_structure(unsigned int index, const Area2F* rect,
                                      unsigned int count, unsigned int position,
                                      const char* imageName)
{
    ScrollInfo_Struct* scrollInfo = &scrollInfoGlobs.structs[index];

    scrollInfo->slider = (ScrollInfo_Slider*)malloc(sizeof(ScrollInfo_Slider));
    scrollInfo->slider->image = image_load_bmp_scaled(imageName, 0, 0);
    scrollInfo->slider->x1 = rect->x;
    scrollInfo->slider->y1 = rect->y;
    scrollInfo->slider->x2 = rect->width + rect->x;
    scrollInfo->slider->y2 = rect->height + rect->y;
    scrollInfo->slider->xOffset = 0.0f;
    scrollInfo->slider->yOffset = 0.0f;
    scrollInfo->slider->range = count - 1;
    scrollInfo->slider->position = position;
}

// FUNCTION: LEGORR 0x004638a0
int scroll_info_update(unsigned int* index, unsigned int mouseX,
                       unsigned int mouseY, int leftButton,
                       int leftButtonLast)
{
    ScrollInfo_Struct* scrollInfo;
    ScrollInfo_Button* button;
    ScrollInfo_Slider* slider;
    unsigned int i;
    unsigned int j;
    int handled = 0;
    float step;
    float offset;
    PanelData* panel;
    unsigned int position;

    for (i = 0; i < SCROLLINFO_MAXSTRUCTS; i++) {
        scrollInfo = &scrollInfoGlobs.structs[i];
        if ((scrollInfo->flags & SCROLLINFO_FLAG_IMAGE) &&
            !request_record_ready_flag4_target(scrollInfo->panel)) {
            for (j = 0; j < 2; j++) {
                if (j == 0) button = scrollInfo->buttons[0];
                else button = scrollInfo->buttons[1];

                if (!handled && scroll_info_mouse_over_structure(scrollInfo, button, mouseX, mouseY)) {
                    *index = i;
                    if (leftButton) {
                        if (scrollInfoGlobs.selection == SCROLLINFO_SELECT_NONE) {
                            if (!leftButtonLast) {
                                scrollInfoGlobs.selection = j;
                                button->flags &= ~SCROLLINFO_BUTTON_HIGHLIGHT;
                                button->flags |= SCROLLINFO_BUTTON_PRESSED;
                            }
                        } else if (j == scrollInfoGlobs.selection) {
                            button->flags &= ~SCROLLINFO_BUTTON_HIGHLIGHT;
                            button->flags |= SCROLLINFO_BUTTON_PRESSED;
                        }
                    } else if (scrollInfoGlobs.selection == SCROLLINFO_SELECT_NONE) {
                        button->flags &= ~SCROLLINFO_BUTTON_PRESSED;
                        button->flags |= SCROLLINFO_BUTTON_HIGHLIGHT;
                    } else if (j == scrollInfoGlobs.selection) {
                        if (j == 0) {
                            if (scrollInfo->slider->position < scrollInfo->slider->range) {
                                scrollInfo->slider->position++;
                            }
                        } else if (scrollInfo->slider->position > 0) {
                            scrollInfo->slider->position--;
                        }
                        scrollInfo->slider->dirty = 1;
                        button->flags &= ~SCROLLINFO_BUTTON_PRESSED;
                        button->flags |= SCROLLINFO_BUTTON_HIGHLIGHT;
                    }
                    handled = 1;
                } else {
                    button->flags &= ~(SCROLLINFO_BUTTON_HIGHLIGHT | SCROLLINFO_BUTTON_PRESSED);
                }
            }

            if (!handled && scroll_info_mouse_over_create_structure(scrollInfo, mouseX, mouseY)) {
                if (leftButton) {
                    if (scrollInfoGlobs.selection == SCROLLINFO_SELECT_SLIDER) {
                        panel = &panelGlobs.panelTable[scrollInfo->panel];
                        if (scrollInfo->flags & SCROLLINFO_FLAG_VERTICAL) {
                            slider = scrollInfo->slider;
                            step = (slider->y2 - slider->y1) / (float)slider->range;
                            offset = (float)mouseY - panel->yCurr - scrollInfo->yOffset - slider->y1;
                            position = scroll_info_round(offset / step);
                        } else {
                            slider = scrollInfo->slider;
                            step = (slider->x2 - slider->x1) / (float)slider->range;
                            offset = (float)mouseX - panel->xCurr - scrollInfo->xOffset - slider->x1;
                            position = scroll_info_round(offset / step);
                        }
                        scrollInfo->slider->position = position;
                        handled = 1;
                        scrollInfo->slider->dirty = 1;
                        *index = i;
                        goto layout;
                    }
                } else {
                    if (scrollInfoGlobs.selection == SCROLLINFO_SELECT_NONE) {
                        scrollInfoGlobs.selection = SCROLLINFO_SELECT_SLIDER;
                    }
                    scrollInfoOverSlider = 1;
                }
                handled = 1;
                *index = i;
            } else {
                scrollInfoOverSlider = 0;
            }

        layout:
            slider = scrollInfo->slider;
            if (slider->dirty) {
                if (scrollInfo->flags & SCROLLINFO_FLAG_VERTICAL) {
                    if (slider->range == 0) {
                        slider->yOffset = 0.0f;
                    } else {
                        slider->yOffset = (float)slider->position * ((slider->y2 - slider->y1 - (float)slider->image->height) / (float)slider->range);
                    }
                } else if (slider->range == 0) {
                    slider->xOffset = slider->x1 - (float)slider->image->width;
                } else {
                    slider->xOffset = (float)slider->position * ((slider->x2 - slider->x1 - (float)slider->image->width) / (float)slider->range);
                }
                scrollInfo->slider->dirty = 0;
            }
        }
    }

    if (!leftButton && !scrollInfoOverSlider) {
        scrollInfoGlobs.selection = SCROLLINFO_SELECT_NONE;
    }
    return handled;
}

// FUNCTION: LEGORR 0x00463c00
int scroll_info_mouse_over_structure(ScrollInfo_Struct* scrollInfo,
                                     ScrollInfo_Button* button, int mouseX,
                                     int mouseY)
{
    float x = panelGlobs.panelTable[scrollInfo->panel].xCurr + scrollInfo->xOffset;
    float y = panelGlobs.panelTable[scrollInfo->panel].yCurr + scrollInfo->yOffset;

    mouseX -= (int)x;
    mouseY -= (int)y;
    if ((float)mouseX >= button->x1 && (float)mouseY < button->y2 &&
        (float)mouseY >= button->y1 && (float)mouseX < button->x2) {
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x00463c90
int scroll_info_mouse_over_create_structure(ScrollInfo_Struct* scrollInfo,
                                            int mouseX, int mouseY)
{
    PanelData* panel = &panelGlobs.panelTable[scrollInfo->panel];

    mouseX -= (int)(panel->xCurr + scrollInfo->xOffset);
    mouseY -= (int)(panel->yCurr + scrollInfo->yOffset);
    if ((float)mouseX >= scrollInfo->slider->x1 && (float)mouseY < scrollInfo->slider->y2 &&
        (float)mouseY >= scrollInfo->slider->y1 && (float)mouseX < scrollInfo->slider->x2) {
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x00463d20
void scroll_info_draw(unsigned int index)
{
    PanelData* panel;
    ScrollInfo_Struct* scrollInfo = &scrollInfoGlobs.structs[index];
    ScrollInfo_Button* button;
    ScrollInfo_Slider* slider;
    Point2F origin;
    Point2F pos;
    unsigned int i;

    panel = &panelGlobs.panelTable[scrollInfo->panel];
    origin.x = panel->xCurr + scrollInfo->xOffset;
    origin.y = panel->yCurr + scrollInfo->yOffset;
    image_display_scaled(scrollInfo->image, NULL, &origin, NULL);

    for (i = 0; i < 2; i++) {
        if (i == 0) button = scrollInfo->buttons[0];
        else button = scrollInfo->buttons[1];

        pos = origin;
        if ((button->flags & SCROLLINFO_BUTTON_HIGHLIGHT) && button->highlightImage != NULL) {
            pos.x = button->x1 + origin.x;
            pos.y = button->y1 + origin.y;
            image_display_scaled(button->highlightImage, NULL, &pos, NULL);
        } else if ((button->flags & SCROLLINFO_BUTTON_PRESSED) && button->pressedImage != NULL) {
            pos.x = button->x1 + origin.x;
            pos.y = button->y1 + origin.y;
            image_display_scaled(button->pressedImage, NULL, &pos, NULL);
        }
    }

    slider = scrollInfo->slider;
    pos.x = slider->x1 + (slider->xOffset + origin.x);
    pos.y = slider->y1 + slider->yOffset + origin.y;
    image_display_scaled(slider->image, NULL, &pos, NULL);
}

// FUNCTION: LEGORR 0x00463e40
int scroll_info_round(float value)
{
    if (value - floor(value) < 0.5) {
        return (int)floor(value);
    }
    return (int)ceil(value);
}

// FUNCTION: LEGORR 0x00463ea0
void scroll_info_get_range(unsigned int index, unsigned int* range,
                           unsigned int* position)
{
    *range = scrollInfoGlobs.structs[index].slider->range;
    *position = scrollInfoGlobs.structs[index].slider->position;
}

// FUNCTION: LEGORR 0x00463ed0
void scroll_info_set_range(unsigned int index, unsigned int range,
                           unsigned int position)
{
    scrollInfoGlobs.structs[index].slider->range = range;
    scrollInfoGlobs.structs[index].slider->position = position;
    scrollInfoGlobs.structs[index].slider->dirty = 1;
}

#ifdef __cplusplus
}
#endif
