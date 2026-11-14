// SPDX-License-Identifier: LGPL-3.0-or-later
/* HelpWindow.c, original translation unit GAME-018 (finding 505), rebuilt in
 * original shape (ADR 0005; finding 595): text 00418190-00419230, .rdata
 * 0049e230-0049e258, literals 004a2798-004a283c, .bss 004db8e8-004dc654.
 *
 * The help window shows, for a vehicle, minifigure or building the player
 * has just made available, its HelpWindowInfo text from Lego.cfg and the
 * list of the object types it unlocks, each with its icon and name. The
 * window has two buttons (Continue and Cancel Future) and scrolls when the
 * list is longer than the text area.
 *
 * The port's implementation stays in the port tail helpwindow_port.cpp,
 * unannotated. In the matching build helpwindow_link_names.h gives seven of
 * this unit's functions the port's names, so lego_unit.c and
 * legoobject_unit.c reach them; natively the port tail serves those
 * names. */
#include "helpwindow_link_names.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crt/vc5names.h"
#include "helpwindow.h"
#include "helpwindow_imports.h"

/* The unit's functions have C linkage in the native C++ compile too. */
#ifdef __cplusplus
extern "C" {
#endif

#define HELPWINDOW_BUTTONCOUNT 2
#define HELPWINDOW_BUTTON_NONE 4
#define HELPWINDOW_MAXINFOTYPES 15
#define HELPWINDOW_MAXINFOLEVELS 16
#define HELPWINDOW_MAXDEPENDENCIES 30

#define HELPWINDOW_FLAG_SHOWING 0x1    /* a text is open */
#define HELPWINDOW_FLAG_ACTIVE 0x2
#define HELPWINDOW_FLAG_LOADED 0x4
#define HELPWINDOW_FLAG_NEWTEXT 0x8    /* the text window needs the text */
#define HELPWINDOW_FLAG_HILIGHT 0x10   /* the pointer is over a button */
#define HELPWINDOW_FLAG_PRESSED 0x20
#define HELPWINDOW_FLAG_ENABLED 0x40

#define HELPWINDOW_OBJECT_VEHICLE 1
#define HELPWINDOW_OBJECT_MINIFIGURE 2
#define HELPWINDOW_OBJECT_BUILDING 4

/* Tool tips 8 and 9 (the Continue and Cancel Future buttons) and 0x21 (the
 * window's Next button). */
#define HELPWINDOW_TOOLTIP_CONTINUE 8
#define HELPWINDOW_TOOLTIP_CANCELFUTURE 9
#define HELPWINDOW_TOOLTIP_WINDOW 0x21

typedef struct HelpWindow_Button {
    Area2F area;                /* +0x00 */
    Image* hiImage;             /* +0x10: under the pointer */
    Image* pressedImage;        /* +0x14 */
    char* toolTip;              /* +0x18 */
} HelpWindow_Button;            /* 0x1c */

/* Dependencies.c's requirement list (0040b000 fills it). */
typedef struct HelpWindow_Matches {
    int types[32];              /* +0x000 */
    int indices[32];            /* +0x080 */
    int levels[32];             /* +0x100 */
    int unused[32];             /* +0x180 */
    unsigned int count;         /* +0x200 */
} HelpWindow_Matches;

/* The unit's .bss, 004db8e8-004dc654. */
typedef struct HelpWindow_Globs {
    struct Font* font;                          /* +0x000 004db8e8 */
    float textX;                                /* +0x004 004db8ec */
    float textY;                                /* +0x008 004db8f0 */
    float textBottom;                           /* +0x00c 004db8f4 */
    Image* windowImage;                         /* +0x010 004db8f8 */
    Point2F windowPos;                          /* +0x014 004db8fc */
    HelpWindow_Button buttons[HELPWINDOW_BUTTONCOUNT]; /* +0x01c 004db904 */
    unsigned char unused_054[0x38];             /* +0x054 004db93c: no code uses it */
    void* textWindow;                           /* +0x08c 004db974 */
    char* vehicleInfo[HELPWINDOW_MAXINFOTYPES][HELPWINDOW_MAXINFOLEVELS];    /* +0x090 */
    char* miniFigureInfo[HELPWINDOW_MAXINFOTYPES][HELPWINDOW_MAXINFOLEVELS]; /* +0x450 */
    char* buildingInfo[HELPWINDOW_MAXINFOTYPES][HELPWINDOW_MAXINFOLEVELS];   /* +0x810 */
    char* canBuildMessage;                      /* +0xbd0 004dc4b8 */
    char* canTransportMessage;                  /* +0xbd4 004dc4bc */
    char* canBuildAndTransportMessage;          /* +0xbd8 004dc4c0 */
    char* currentInfo;                          /* +0xbdc 004dc4c4 */
    int objType;                                /* +0xbe0 004dc4c8 */
    int objIndex;                               /* +0xbe4 004dc4cc */
    int objLevel;                               /* +0xbe8 004dc4d0 */
    int depTypes[HELPWINDOW_MAXDEPENDENCIES];   /* +0xbec 004dc4d4 */
    int depIndices[HELPWINDOW_MAXDEPENDENCIES]; /* +0xc64 004dc54c */
    int depLevels[HELPWINDOW_MAXDEPENDENCIES];  /* +0xcdc 004dc5c4 */
    unsigned int depCount;                      /* +0xd54 004dc63c */
    float scroll;                               /* +0xd58 004dc640 */
    float scrollMax;                            /* +0xd5c 004dc644 */
    int hilightButton;                          /* +0xd60 004dc648 */
    int pressedButton;                          /* +0xd64 004dc64c */
    unsigned int flags;                         /* +0xd68 004dc650 */
} HelpWindow_Globs;                             /* 0xd6c */

// GLOBAL: LEGORR 0x004db8e8
static HelpWindow_Globs helpwindowGlobs;

// FUNCTION: LEGORR 0x00418190
void help_window_set_font(struct Font* font)
{
    helpwindowGlobs.font = font;
}

// FUNCTION: LEGORR 0x004181a0
void help_window_hide(void)
{
    helpwindowGlobs.flags &= ~HELPWINDOW_FLAG_SHOWING;
}

// FUNCTION: LEGORR 0x004181b0
void help_window_initialise(BlockFile* config, const char* gameName)
{
    Area2F area;
    char* stringParts[16];
    char* str;
    char* s;
    char c;

    help_window_load_infos(config, gameName);

    if ((str = block_get_str_copy(config, string_path_join(gameName,
        // STRING: LEGORR 0x004a27e8
        "HelpWindow",
        // STRING: LEGORR 0x004a27f4
        "BackGroundImage", 0))) != NULL) {
        sound_variant_split(str, stringParts, ",");
        if ((helpwindowGlobs.windowImage = image_load_bmp_scaled(stringParts[0], 0, 0)) != NULL) {
            image_surface_blit_transformed((struct TextureSurface*)helpwindowGlobs.windowImage,
                                           0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        }
        helpwindowGlobs.windowPos.x = (float)atoi(stringParts[1]);
        helpwindowGlobs.windowPos.y = (float)atoi(stringParts[2]);
        free(str);
    }

    help_window_load_buttons(config, gameName);

    if ((str = block_get_str_copy(config, string_path_join(gameName, "HelpWindow",
        // STRING: LEGORR 0x004a27d8
        "TextWindowArea", 0))) != NULL) {
        sound_variant_split(str, stringParts, ",");
        area.x = (float)atoi(stringParts[0]);
        area.y = (float)atoi(stringParts[1]);
        area.width = (float)atoi(stringParts[2]);
        area.height = (float)atoi(stringParts[3]);
        helpwindowGlobs.textWindow = text_buffer_create((uintptr_t)helpwindowGlobs.font,
                                                        (const uint32_t*)&area, 0x400);
        helpwindowGlobs.textX = area.x;
        helpwindowGlobs.textY = area.y;
        helpwindowGlobs.textBottom = area.y + area.height;
        free(str);
    }

    if ((helpwindowGlobs.canBuildMessage = block_get_str_copy(config, string_path_join(gameName, "HelpWindow",
        // STRING: LEGORR 0x004a27c8
        "CanBuildMessage", 0))) != NULL) {
        for (s = helpwindowGlobs.canBuildMessage; (c = *s) != '\0'; s++) {
            if (c == '_') *s = ' ';
        }
    }
    if ((helpwindowGlobs.canTransportMessage = block_get_str_copy(config, string_path_join(gameName, "HelpWindow",
        // STRING: LEGORR 0x004a27b4
        "CanTransportMessage", 0))) != NULL) {
        for (s = helpwindowGlobs.canTransportMessage; (c = *s) != '\0'; s++) {
            if (c == '_') *s = ' ';
        }
    }
    if ((helpwindowGlobs.canBuildAndTransportMessage = block_get_str_copy(config, string_path_join(gameName, "HelpWindow",
        // STRING: LEGORR 0x004a2798
        "CanBuildAndTransportMessage", 0))) != NULL) {
        for (s = helpwindowGlobs.canBuildAndTransportMessage; (c = *s) != '\0'; s++) {
            if (c == '_') *s = ' ';
        }
    }

    helpwindowGlobs.flags |= HELPWINDOW_FLAG_LOADED;
    helpwindowGlobs.flags |= HELPWINDOW_FLAG_ACTIVE;
    help_window_toggle(0, 0);
}

/* legoGlobs' fields (lego_globs.h, finding 653), included before their
 * first use so that the functions above keep their symbol counter
 * (finding 538). */
#include "lego_globs.h"

// FUNCTION: LEGORR 0x00418450
void help_window_load_infos(BlockFile* config, const char* gameName)
{
    char buff[1024];
    unsigned int i, j;
    char* s;

    for (i = 0; i < helpWindow_typeCounts[0]; i++) {
        for (j = 0; j < HELPWINDOW_MAXINFOLEVELS; j++) {
            // STRING: LEGORR 0x004a2804
            sprintf(buff, "HelpWindow::HelpWindowInfo::%sL%i", legoGlobs_vehicleName[i], j);
            if ((s = helpwindowGlobs.vehicleInfo[i][j] = block_get_str_copy(config, string_path_join(gameName, buff, 0))) != NULL) {
                for (; *s != '\0'; s++) {
                    if (*s == '_') *s = ' ';
                }
            }
        }
    }
    for (i = 0; i < helpWindow_typeCounts[1]; i++) {
        for (j = 0; j < HELPWINDOW_MAXINFOLEVELS; j++) {
            sprintf(buff, "HelpWindow::HelpWindowInfo::%sL%i", legoGlobs_miniFigureName[i], j);
            if ((s = helpwindowGlobs.miniFigureInfo[i][j] = block_get_str_copy(config, string_path_join(gameName, buff, 0))) != NULL) {
                for (; *s != '\0'; s++) {
                    if (*s == '_') *s = ' ';
                }
            }
        }
    }
    for (i = 0; i < helpWindow_typeCounts[3]; i++) {
        for (j = 0; j < HELPWINDOW_MAXINFOLEVELS; j++) {
            sprintf(buff, "HelpWindow::HelpWindowInfo::%sL%i", legoGlobs_buildingName[i], j);
            if ((s = helpwindowGlobs.buildingInfo[i][j] = block_get_str_copy(config, string_path_join(gameName, buff, 0))) != NULL) {
                for (; *s != '\0'; s++) {
                    if (*s == '_') *s = ' ';
                }
            }
        }
    }
}

// FUNCTION: LEGORR 0x004185f0
void help_window_load_buttons(BlockFile* config, const char* gameName)
{
    char* stringParts[16];
    char* str;

    if ((str = block_get_str_copy(config, string_path_join(gameName, "HelpWindow",
        // STRING: LEGORR 0x004a283c
        "ContinueButton", 0))) != NULL) {
        sound_variant_split(str, stringParts, ",");
        helpwindowGlobs.buttons[0].area.x = (float)atoi(stringParts[0]) + helpwindowGlobs.windowPos.x;
        helpwindowGlobs.buttons[0].area.y = (float)atoi(stringParts[1]) + helpwindowGlobs.windowPos.y;
        helpwindowGlobs.buttons[0].area.width = (float)atoi(stringParts[2]);
        helpwindowGlobs.buttons[0].area.height = (float)atoi(stringParts[3]);
        helpwindowGlobs.buttons[0].hiImage = image_load_bmp_scaled(stringParts[4], 0, 0);
        helpwindowGlobs.buttons[0].pressedImage = image_load_bmp_scaled(stringParts[5], 0, 0);
        helpwindowGlobs.buttons[0].toolTip = formatted_text_escape_decode_and_emit(stringParts[6]);
        tooltip_set_content(HELPWINDOW_TOOLTIP_CONTINUE, helpwindowGlobs.buttons[0].toolTip);
    }
    if ((str = block_get_str_copy(config, string_path_join(gameName, "HelpWindow",
        // STRING: LEGORR 0x004a2828
        "CancelFutureButton", 0))) != NULL) {
        sound_variant_split(str, stringParts, ",");
        helpwindowGlobs.buttons[1].area.x = (float)atoi(stringParts[0]) + helpwindowGlobs.windowPos.x;
        helpwindowGlobs.buttons[1].area.y = (float)atoi(stringParts[1]) + helpwindowGlobs.windowPos.y;
        helpwindowGlobs.buttons[1].area.width = (float)atoi(stringParts[2]);
        helpwindowGlobs.buttons[1].area.height = (float)atoi(stringParts[3]);
        helpwindowGlobs.buttons[1].hiImage = image_load_bmp_scaled(stringParts[4], 0, 0);
        helpwindowGlobs.buttons[1].pressedImage = image_load_bmp_scaled(stringParts[5], 0, 0);
        helpwindowGlobs.buttons[1].toolTip = formatted_text_escape_decode_and_emit(stringParts[6]);
        tooltip_set_content(HELPWINDOW_TOOLTIP_CANCELFUTURE, helpwindowGlobs.buttons[1].toolTip);
    }
}

// FUNCTION: LEGORR 0x00418800
void help_window_show_text(int show)
{
    if (helpwindowGlobs.flags & HELPWINDOW_FLAG_LOADED) {
        if (show) helpwindowGlobs.flags |= HELPWINDOW_FLAG_ACTIVE;
        else helpwindowGlobs.flags &= ~HELPWINDOW_FLAG_ACTIVE;
        helpwindowGlobs.flags &= ~HELPWINDOW_FLAG_SHOWING;
    }
}

// FUNCTION: LEGORR 0x00418830
void help_window_recall_dependencies(int objType, int objIndex, int objLevel, int noDisplay)
{
    help_window_dependencies_pass_adapter();
    if (!help_window_dependencies_flag100_adapter(objType, objIndex, objLevel)) {
        help_window_dependencies_flag100_set_adapter(objType, objIndex, objLevel);
        if (!noDisplay) {
            helpwindowGlobs.objType = objType;
            helpwindowGlobs.objIndex = objIndex;
            helpwindowGlobs.objLevel = objLevel;
            switch (objType) {
            case HELPWINDOW_OBJECT_VEHICLE:
                helpwindowGlobs.currentInfo = helpwindowGlobs.vehicleInfo[objIndex][objLevel];
                break;
            case HELPWINDOW_OBJECT_MINIFIGURE:
                helpwindowGlobs.currentInfo = helpwindowGlobs.miniFigureInfo[objIndex][objLevel];
                break;
            case HELPWINDOW_OBJECT_BUILDING:
                helpwindowGlobs.currentInfo = helpwindowGlobs.buildingInfo[objIndex][objLevel];
                break;
            }
            text_buffer_reset_target(helpwindowGlobs.textWindow);
            helpwindowGlobs.flags |= HELPWINDOW_FLAG_NEWTEXT;
            help_window_list_dependencies(objType, objIndex, objLevel);
            helpwindowGlobs.scroll = 0.0f;
            helpwindowGlobs.scrollMax = 0.0f;
            helpwindowGlobs.flags |= HELPWINDOW_FLAG_SHOWING;
        }
    }
}

// FUNCTION: LEGORR 0x00418920
void help_window_list_dependencies(int objType, int objIndex, int objLevel)
{
    HelpWindow_Matches matches;
    unsigned int i;

    helpwindowGlobs.depCount = 0;
    help_window_dependencies_matches_adapter(objType, objIndex, objLevel, &matches);
    for (i = 0; i < matches.count; i++) {
        if (help_window_dependencies_flag4_adapter(matches.types[i], matches.indices[i], matches.levels[i])) {
            helpwindowGlobs.depTypes[helpwindowGlobs.depCount] = matches.types[i];
            helpwindowGlobs.depIndices[helpwindowGlobs.depCount] = matches.indices[i];
            helpwindowGlobs.depLevels[helpwindowGlobs.depCount] = matches.levels[i];
            helpwindowGlobs.depCount++;
        }
    }
}

// FUNCTION: LEGORR 0x004189d0
void help_window_close(void)
{
    if ((helpwindowGlobs.flags & HELPWINDOW_FLAG_ACTIVE) && (helpwindowGlobs.flags & HELPWINDOW_FLAG_SHOWING)) {
        lego_transition_control(0, 0);
        objective_score_text_origin_set(NULL);
        helpwindowGlobs.flags &= ~HELPWINDOW_FLAG_SHOWING;
    }
}

// FUNCTION: LEGORR 0x00418a00
void help_window_update(void)
{
    unsigned int linesUsed;
    Point2F pos;
    unsigned int i;
    unsigned int vehicleCount, buildingCount;
    Image* image;

    if (!help_window_is_enabled()) help_window_close();

    if (help_window_is_enabled() && (helpwindowGlobs.flags & HELPWINDOW_FLAG_ACTIVE) &&
        (helpwindowGlobs.flags & HELPWINDOW_FLAG_SHOWING) && !(nerpfunc_get_tutorial_flags(NULL) & 0x800)) {

        lego_transition_control(0, 1);

        if (helpwindowGlobs.currentInfo == NULL && helpwindowGlobs.depCount <= 0) {
            help_window_close();
            return;
        }

        if (helpwindowGlobs.windowImage) {
            image_display_scaled(helpwindowGlobs.windowImage, NULL, &helpwindowGlobs.windowPos, NULL);
        }

        if (helpwindowGlobs.flags & HELPWINDOW_FLAG_NEWTEXT) {
            if (helpwindowGlobs.currentInfo) {
                text_buffer_append_format(helpwindowGlobs.textWindow, "%s", helpwindowGlobs.currentInfo);
            }
            helpwindowGlobs.flags &= ~HELPWINDOW_FLAG_NEWTEXT;
        }

        if (helpwindowGlobs.currentInfo) {
            text_buffer_draw_wrapped(helpwindowGlobs.textWindow, 0, 0.0f, (int*)&linesUsed);
        } else {
            linesUsed = (unsigned int)helpwindowGlobs.textY;
        }

        if (helpwindowGlobs.depCount > 0) {
            pos.x = helpwindowGlobs.textX;
            pos.y = (float)linesUsed + 10.0f;

            vehicleCount = 0;
            buildingCount = 0;
            for (i = 0; i < helpwindowGlobs.depCount; i++) {
                switch (helpwindowGlobs.depTypes[i]) {
                case HELPWINDOW_OBJECT_VEHICLE:
                    vehicleCount++;
                    break;
                case HELPWINDOW_OBJECT_BUILDING:
                    buildingCount++;
                    break;
                }
            }

            if (!vehicleCount) {
                if (helpwindowGlobs.canBuildMessage) {
                    font_print_f((const struct TextureSurface*)helpwindowGlobs.font, (int)pos.x, (int)pos.y, "%s", helpwindowGlobs.canBuildMessage);
                }
            } else if (!buildingCount) {
                if (helpwindowGlobs.canTransportMessage) {
                    font_print_f((const struct TextureSurface*)helpwindowGlobs.font, (int)pos.x, (int)pos.y, "%s", helpwindowGlobs.canTransportMessage);
                }
            } else if (helpwindowGlobs.canBuildAndTransportMessage) {
                font_print_f((const struct TextureSurface*)helpwindowGlobs.font, (int)pos.x, (int)pos.y, "%s", helpwindowGlobs.canBuildAndTransportMessage);
            }

            pos.y += (float)(object_field_be4_dword_get(helpwindowGlobs.font) + 10);
            pos.y -= (helpwindowGlobs.scroll - (float)floor(helpwindowGlobs.scroll)) * 45.0f;

            for (i = (unsigned int)floor(helpwindowGlobs.scroll); i < helpwindowGlobs.depCount; i++) {
                pos.x = helpwindowGlobs.textX;
                if ((image = (Image*)input_binding_default_value_get(helpwindowGlobs.depTypes[i], helpwindowGlobs.depIndices[i], 1)) != NULL) {
                    if ((float)(int)image->height + pos.y >= helpwindowGlobs.textBottom) {
                        if (0.0f == helpwindowGlobs.scrollMax) {
                            helpwindowGlobs.scrollMax = (float)(helpwindowGlobs.depCount - i - 1);
                        }
                        if (helpwindowGlobs.scroll < helpwindowGlobs.scrollMax) {
                            Point2F pointerPos;
                            pointerPos.x = 345.0f;
                            pointerPos.y = 350.0f;
                            objective_score_text_origin_set(&pointerPos.x);
                        }
                        break;
                    }
                    image_display_scaled(image, NULL, &pos, NULL);
                }
                pos.x += 50.0f;
                pos.y += 10.0f;
                font_print_f((const struct TextureSurface*)helpwindowGlobs.font, (int)pos.x, (int)pos.y, "%s",
                             legoobject_the_name_get(helpwindowGlobs.depTypes[i], helpwindowGlobs.depIndices[i]));
                pos.y += 35.0f;
            }
        }

        help_window_draw_buttons();
    }
}

// FUNCTION: LEGORR 0x00418da0
int help_window_check_buttons(unsigned int mouseX, unsigned int mouseY, int leftButton,
                              int leftButtonLast, int leftReleased, float elapsed)
{
    float msx;
    Area2F area;
    unsigned int i;

    if ((helpwindowGlobs.flags & HELPWINDOW_FLAG_ACTIVE) && (helpwindowGlobs.flags & HELPWINDOW_FLAG_SHOWING)) {
        helpwindowGlobs.flags &= ~HELPWINDOW_FLAG_HILIGHT;
        msx = (float)mouseX;
        helpwindowGlobs.flags &= ~HELPWINDOW_FLAG_PRESSED;
        if (msx >= 345.0f && msx <= (float)(int)legoGlobs_NextButtonImage->width + 345.0f &&
            (float)mouseY >= 350.0f && (float)mouseY <= (float)(int)legoGlobs_NextButtonImage->height + 350.0f) {
            timed_effect_update_request_set_corpus(HELPWINDOW_TOOLTIP_WINDOW);
            if (leftReleased) {
                help_window_button_action(3, 5.0f);
                media_action_feedback_sound_dispatch(0);
            }
        }

        for (i = 0; i < HELPWINDOW_BUTTONCOUNT; i++) {
            area = helpwindowGlobs.buttons[i].area;
            if (msx >= area.x && msx <= (area.x + area.width) &&
                (float)mouseY >= area.y && (float)mouseY <= (area.y + area.height)) {
                helpwindowGlobs.flags |= HELPWINDOW_FLAG_HILIGHT;
                helpwindowGlobs.hilightButton = i;
                if (help_window_press_button(i, leftButton, leftButtonLast) && leftReleased) {
                    help_window_button_action(i, elapsed);
                    media_action_feedback_sound_dispatch(0);
                }
                if (!leftButton && !leftReleased) help_window_button_tooltip(i);
                return 1;
            }
        }
        help_window_press_button(HELPWINDOW_BUTTON_NONE, leftButton, leftButtonLast);
    }
    return 0;
}

// FUNCTION: LEGORR 0x00418f80
void help_window_button_tooltip(int button)
{
    switch (button) {
    case 0:
        timed_effect_update_request_set_corpus(HELPWINDOW_TOOLTIP_CONTINUE);
        break;
    case 1:
        timed_effect_update_request_set_corpus(HELPWINDOW_TOOLTIP_CANCELFUTURE);
        break;
    case 2:
    case 3:
        /* Deviation (scoring only): the original's table has entries for
         * buttons 2 and 3 that do nothing; MSVC 5.0 keeps them only when
         * the cases hold a statement (an empty case gives a compare
         * chain). */
        button = 0;
        break;
    }
}

// FUNCTION: LEGORR 0x00418fc0
int help_window_press_button(int button, int leftButton, int leftButtonLast)
{
    int pressed = helpwindowGlobs.pressedButton;

    if (leftButton) {
        if (pressed == HELPWINDOW_BUTTON_NONE) {
            if (!leftButtonLast) {
                helpwindowGlobs.pressedButton = button;
                helpwindowGlobs.flags |= HELPWINDOW_FLAG_PRESSED;
            }
        } else if (pressed == button) {
            helpwindowGlobs.flags |= HELPWINDOW_FLAG_PRESSED;
        }
    } else {
        helpwindowGlobs.pressedButton = HELPWINDOW_BUTTON_NONE;
    }
    return pressed == button;
}

// FUNCTION: LEGORR 0x00419030
void help_window_button_action(int button, float elapsed)
{
    switch (button) {
    case 0:
        help_window_close();
        break;
    case 1:
        help_window_toggle(1, 0);
        help_window_close();
        break;
    case 2:
        if (helpwindowGlobs.scroll > 0.0f) {
            helpwindowGlobs.scroll -= elapsed * 0.2f;
            if (helpwindowGlobs.scroll < 0.0f) helpwindowGlobs.scroll = 0.0f;
        }
        break;
    case 3:
        if (helpwindowGlobs.scroll < helpwindowGlobs.scrollMax) {
            helpwindowGlobs.scroll += elapsed * 0.2f;
            if (helpwindowGlobs.scroll >= helpwindowGlobs.scrollMax) {
                helpwindowGlobs.scroll = helpwindowGlobs.scrollMax;
                objective_score_text_origin_set(NULL);
            }
        }
        break;
    }
}

// FUNCTION: LEGORR 0x00419100
void help_window_draw_buttons(void)
{
    Point2F pos;
    Image* image;
    unsigned int i;

    for (i = 0; i < HELPWINDOW_BUTTONCOUNT; i++) {
        pos.x = helpwindowGlobs.buttons[i].area.x;
        pos.y = helpwindowGlobs.buttons[i].area.y;
        if ((image = help_window_get_button_image(i)) != NULL) {
            image_display_scaled(image, NULL, &pos, NULL);
        }
    }
}

// FUNCTION: LEGORR 0x00419150
Image* help_window_get_button_image(int button)
{
    if (button == 1 && !(helpwindowGlobs.flags & HELPWINDOW_FLAG_LOADED)) {
        return helpwindowGlobs.buttons[1].pressedImage;
    }
    if ((helpwindowGlobs.flags & HELPWINDOW_FLAG_HILIGHT) && helpwindowGlobs.hilightButton == button) {
        if (helpwindowGlobs.flags & HELPWINDOW_FLAG_PRESSED) return helpwindowGlobs.buttons[button].pressedImage;
        return helpwindowGlobs.buttons[button].hiImage;
    }
    return NULL;
}

// FUNCTION: LEGORR 0x004191a0
int help_window_is_showing(void)
{
    int enabled = help_window_is_enabled();
    unsigned int active = helpwindowGlobs.flags & HELPWINDOW_FLAG_ACTIVE;
    unsigned int showing = helpwindowGlobs.flags & HELPWINDOW_FLAG_SHOWING;
    int noTutorial = !nerpfunc_get_tutorial_flags(NULL);

    if (enabled && active && showing && noTutorial) return 1;
    return 0;
}

// FUNCTION: LEGORR 0x004191f0
int help_window_is_enabled(void)
{
    return helpwindowGlobs.flags & HELPWINDOW_FLAG_ENABLED;
}

// FUNCTION: LEGORR 0x00419200
void help_window_toggle(int toggle, int disable)
{
    if (toggle) disable = helpwindowGlobs.flags & HELPWINDOW_FLAG_ENABLED;
    if (disable) helpwindowGlobs.flags &= ~HELPWINDOW_FLAG_ENABLED;
    else helpwindowGlobs.flags |= HELPWINDOW_FLAG_ENABLED;
}

#ifdef __cplusplus
}
#endif
