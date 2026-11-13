// SPDX-License-Identifier: LGPL-3.0-or-later
/* ScrollInfo.c (original translation unit GAME-039, finding 611): the
 * information panel's scroll bar.
 *
 * C-compatible header (ADR 0005). scrollinfo.c is compiled as C in the
 * matching build and as C++ in the native port. Shared C++ headers do not
 * include this file: every include and declaration a header adds shifts
 * MSVC's symbol numbering (finding 506). src/game/test_c_boundaries.cpp
 * checks it against the C++ declarations of the same functions.
 *
 * The names are inferred (OpenLRR's ScrollInfo_* in snake case); in the
 * matching build scrollinfo_link_names.h renames most definitions to the
 * port's names. */
#ifndef LEGORRR_GAME_SCROLLINFO_H
#define LEGORRR_GAME_SCROLLINFO_H

#include "gods98/images.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SCROLLINFO_MAXSTRUCTS 1

/* ScrollInfo_Struct.flags. */
#define SCROLLINFO_FLAG_ENABLED 0x1
#define SCROLLINFO_FLAG_IMAGE 0x4
#define SCROLLINFO_FLAG_VERTICAL 0x8
#define SCROLLINFO_FLAG_HORIZONTAL 0x10

/* ScrollInfo_Button.flags. */
#define SCROLLINFO_BUTTON_HIGHLIGHT 0x2
#define SCROLLINFO_BUTTON_PRESSED 0x4

/* scrollInfoGlobs.selection: the button being pressed (0 or 1), the
 * slider (2), or none (3). */
#define SCROLLINFO_SELECT_SLIDER 2
#define SCROLLINFO_SELECT_NONE 3

/* An arrow button, 0x1c bytes. */
typedef struct ScrollInfo_Button {
    float x1, y1;              /* +0x00 */
    float x2, y2;              /* +0x08 */
    Image* highlightImage;     /* +0x10 */
    Image* pressedImage;       /* +0x14 */
    unsigned int flags;        /* +0x18 */
} ScrollInfo_Button;

/* The slider, 0x28 bytes. */
typedef struct ScrollInfo_Slider {
    float x1, y1;              /* +0x00 */
    float x2, y2;              /* +0x08 */
    float xOffset, yOffset;    /* +0x10: the slider's place on the bar */
    Image* image;              /* +0x18 */
    unsigned int range;        /* +0x1c: the last position */
    unsigned int position;     /* +0x20 */
    int dirty;                 /* +0x24: lay the slider out again */
} ScrollInfo_Slider;

/* A scroll bar, 0x20 bytes. */
typedef struct ScrollInfo_Struct {
    Image* image;              /* +0x00 */
    float xOffset, yOffset;    /* +0x04: the bar's place on its panel */
    ScrollInfo_Button* buttons[2]; /* +0x0c */
    ScrollInfo_Slider* slider; /* +0x14 */
    int panel;                 /* +0x18 */
    unsigned int flags;        /* +0x1c */
} ScrollInfo_Struct;

void scroll_info_initialise(const char* imageName, unsigned int index,
                            int horizontal, int panel, ...);          /* 004636b0 */
void scroll_info_set_xy_offset(unsigned int index, unsigned int xOffset,
                               unsigned int yOffset);                 /* 00463730 */
void scroll_info_add_structure(unsigned int index, unsigned int type,
                               const Area2F* rect, const char* highlightName,
                               const char* pressedName);              /* 00463770 */
void scroll_info_add_create_structure(unsigned int index, const Area2F* rect,
                                      unsigned int count, unsigned int position,
                                      const char* imageName);        /* 00463810 */
int scroll_info_update(unsigned int* index, unsigned int mouseX,
                       unsigned int mouseY, int leftButton,
                       int leftButtonLast);                           /* 004638a0 */
int scroll_info_mouse_over_structure(ScrollInfo_Struct* scrollInfo,
                                     ScrollInfo_Button* button, int mouseX,
                                     int mouseY);                     /* 00463c00 */
int scroll_info_mouse_over_create_structure(ScrollInfo_Struct* scrollInfo,
                                            int mouseX, int mouseY); /* 00463c90 */
void scroll_info_draw(unsigned int index);                            /* 00463d20 */
int scroll_info_round(float value);                                   /* 00463e40 */
void scroll_info_get_range(unsigned int index, unsigned int* range,
                           unsigned int* position);                   /* 00463ea0 */
void scroll_info_set_range(unsigned int index, unsigned int range,
                           unsigned int position);                    /* 00463ed0 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_SCROLLINFO_H */
