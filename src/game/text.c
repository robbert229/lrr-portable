// SPDX-License-Identifier: LGPL-3.0-or-later
/* Text.c, original translation unit GAME-045 (finding 505), rebuilt in
 * original shape (ADR 0005; finding 607): text 0046a7f0-0046b1d0, the
 * literals 004aa460-004aa640 and the .bss block 00503190-0050366c.
 *
 * The in-game message panel. Each message type (Text_Walk to
 * Text_UnitUpgraded) has a text, an optional image and an optional sound,
 * loaded from Lego.cfg's TextMessages and TextMessagesWithImages.
 * Text_DisplayMessage selects a type; Text_Update prints it into the
 * text-only window, or into the image window beside its image, and counts
 * down its display time. A NERPs message line (Text_SetNERPsMessage) takes
 * precedence. When the text overflows the window, the panel grows a line
 * at a time, and shrinks back once the text fits again.
 *
 * The port's implementation stays in the port tail text_port.cpp,
 * unannotated. In the matching build text_link_names.h gives this unit's
 * functions the port's names, so every caller reaches them; natively the
 * port tail serves those names. */
#include "text_link_names.h"

#include <stdio.h>
#include <string.h>

#include "crt/vc5names.h"
#include "text.h"
#include "text_imports.h"

/* The unit's functions have C linkage in the native C++ compile too. */
#ifdef __cplusplus
extern "C" {
#endif

typedef enum Text_Type {
    Text_Walk,
    Text_Drill,
    Text_NoDrill,
    Text_CantDrill,
    Text_Reinforce,
    Text_NoReinforce,
    Text_CantReinforce,
    Text_PickupCrystal,
    Text_Encyclopedia,
    Text_RadarHelp,
    Text_BuildHelp,
    Text_SelectionHelp,
    Text_Drive,
    Text_CantDrive,
    Text_MakeTeleporter,
    Text_CannotPlaceBuilding,
    Text_CrystalOreDisabled,
    Text_CrystalFound,
    Text_CavernDiscovered,
    Text_OreFound,
    Text_AirSupplyLow,
    Text_AirSupplyRunningOut,
    Text_SpaceToContinue,
    Text_GameCompleted,
    Text_ManTrained,
    Text_UnitUpgraded,
    Text_Type_Count
} Text_Type;

/* textGlobs.flags. NERPSMESSAGE is set by a new NERPs message line and
 * cleared once the panel has stopped growing or shrinking for it. */
#define TEXT_FLAG_NERPSMESSAGE 0x1
#define TEXT_FLAG_FLASH 0x4

#define TEXT_COUNTER_PERMANENT 0x8000
#define TEXT_COUNTER_NERPS 0x10000

/* Panel.c's message panel. */
#define PANEL_MESSAGES 3

/* Font.c's record, as far as the unit reads it: Font_GetHeight is an
 * inline read of +0xbe4. */
typedef struct Font {
    unsigned char reserved[0xbe4];
    unsigned int height; /* +0xbe4 */
} Font;

/* TextWindow.c's record, as far as the unit reads it. */
typedef struct TextWindow {
    Font* font; /* +0x000 */
} TextWindow;

/* Lego.c's level record, as far as the unit reads it. */
typedef struct Lego_Level {
    unsigned char reserved[0x1fc];
    float oxygenLevel; /* +0x1fc */
} Lego_Level;

typedef struct Text_Globs {
    const char* typeName[Text_Type_Count];      /* +0x000 00503190 */
    char* messages[Text_Type_Count];            /* +0x068 005031f8 */
    Image* images[Text_Type_Count];             /* +0x0d0 00503260 */
    char soundNames[Text_Type_Count][32];       /* +0x138 005032c8 */
    Text_Type currType;                         /* +0x478 00503608 */
    unsigned int typeCount;                     /* +0x47c 0050360c */
    unsigned int counter;                       /* +0x480 00503610 */
    char* currText;                             /* +0x484 00503614 */
    float timer;                                /* +0x488 00503618 */
    TextWindow* textOnlyWindow;                 /* +0x48c 0050361c */
    TextWindow* textImageWindow;                /* +0x490 00503620 */
    unsigned int reserved494;                   /* +0x494 00503624: no code reference */
    unsigned int panelY;                        /* +0x498 00503628 */
    Area2F messageArea;                         /* +0x49c 0050362c */
    float messageBottom;                        /* +0x4ac 0050363c */
    float panelTimer;                           /* +0x4b0 00503640: no reader */
    float panelX;                               /* +0x4b4 00503644 */
    float currPanelY;                           /* +0x4b8 00503648 */
    Area2F currArea;                            /* +0x4bc 0050364c */
    unsigned int imageX;                        /* +0x4cc 0050365c */
    unsigned int imageY;                        /* +0x4d0 00503660 */
    unsigned int flags;                         /* +0x4d4 00503664 */
    float pauseTime;                            /* +0x4d8 00503668 */
} Text_Globs; /* 0x4dc */

// GLOBAL: LEGORR 0x00503190
Text_Globs textGlobs;

// FUNCTION: LEGORR 0x0046a7f0
void text_load(TextWindow* textOnlyWindow, TextWindow* textImageWindow,
               unsigned int imageX, unsigned int imageY, float pauseTime)
{
    // STRING: LEGORR 0x004aa640
    textGlobs.typeName[Text_Walk] = "Text_Walk";
    // STRING: LEGORR 0x004aa634
    textGlobs.typeName[Text_Drill] = "Text_Drill";
    // STRING: LEGORR 0x004aa624
    textGlobs.typeName[Text_NoDrill] = "Text_NoDrill";
    // STRING: LEGORR 0x004aa614
    textGlobs.typeName[Text_CantDrill] = "Text_CantDrill";
    // STRING: LEGORR 0x004aa604
    textGlobs.typeName[Text_Reinforce] = "Text_Reinforce";
    // STRING: LEGORR 0x004aa5f0
    textGlobs.typeName[Text_NoReinforce] = "Text_NoReinforce";
    // STRING: LEGORR 0x004aa5dc
    textGlobs.typeName[Text_CantReinforce] = "Text_CantReinforce";
    // STRING: LEGORR 0x004aa5c8
    textGlobs.typeName[Text_PickupCrystal] = "Text_PickupCrystal";
    // STRING: LEGORR 0x004aa5b4
    textGlobs.typeName[Text_Encyclopedia] = "Text_Encyclopedia";
    // STRING: LEGORR 0x004aa5a4
    textGlobs.typeName[Text_RadarHelp] = "Text_RadarHelp";
    // STRING: LEGORR 0x004aa594
    textGlobs.typeName[Text_BuildHelp] = "Text_BuildHelp";
    // STRING: LEGORR 0x004aa580
    textGlobs.typeName[Text_SelectionHelp] = "Text_SelectionHelp";
    // STRING: LEGORR 0x004aa574
    textGlobs.typeName[Text_Drive] = "Text_Drive";
    // STRING: LEGORR 0x004aa564
    textGlobs.typeName[Text_CantDrive] = "Text_CantDrive";
    // STRING: LEGORR 0x004aa550
    textGlobs.typeName[Text_MakeTeleporter] = "Text_MakeTeleporter";
    // STRING: LEGORR 0x004aa534
    textGlobs.typeName[Text_CannotPlaceBuilding] = "Text_CannotPlaceBuilding";
    // STRING: LEGORR 0x004aa51c
    textGlobs.typeName[Text_CrystalOreDisabled] = "Text_CrystalOreDisabled";
    // STRING: LEGORR 0x004aa508
    textGlobs.typeName[Text_CrystalFound] = "Text_CrystalFound";
    // STRING: LEGORR 0x004aa4f0
    textGlobs.typeName[Text_CavernDiscovered] = "Text_CavernDiscovered";
    // STRING: LEGORR 0x004aa4e0
    textGlobs.typeName[Text_OreFound] = "Text_OreFound";
    // STRING: LEGORR 0x004aa4cc
    textGlobs.typeName[Text_AirSupplyLow] = "Text_AirSupplyLow";
    // STRING: LEGORR 0x004aa4b0
    textGlobs.typeName[Text_AirSupplyRunningOut] = "Text_AirSupplyRunningOut";
    // STRING: LEGORR 0x004aa498
    textGlobs.typeName[Text_SpaceToContinue] = "Text_SpaceToContinue";
    // STRING: LEGORR 0x004aa484
    textGlobs.typeName[Text_GameCompleted] = "Text_GameCompleted";
    // STRING: LEGORR 0x004aa474
    textGlobs.typeName[Text_ManTrained] = "Text_ManTrained";
    // STRING: LEGORR 0x004aa460
    textGlobs.typeName[Text_UnitUpgraded] = "Text_UnitUpgraded";

    textGlobs.textOnlyWindow = textOnlyWindow;
    textGlobs.textImageWindow = textImageWindow;
    textGlobs.imageX = imageX;
    textGlobs.imageY = imageY;
    textGlobs.typeCount = Text_Type_Count;
    textGlobs.currType = Text_Type_Count;

    /* 0.0f first: MSVC loads the literal and compares the parameter with
     * it, as the original does. */
    if (0.0f == pauseTime) textGlobs.pauseTime = 100.0f;
    else textGlobs.pauseTime = pauseTime;
}

// FUNCTION: LEGORR 0x0046a950
void text_initialise(const char* panelImageFile, unsigned int panelX,
                     unsigned int panelY, unsigned int panelOpenY,
                     const Area2F* messageArea, const Area2F* imageArea,
                     float panelTimer)
{
    textGlobs.panelY = panelY;
    textGlobs.messageArea = *messageArea;
    textGlobs.messageBottom = textGlobs.messageArea.y + textGlobs.messageArea.height -
        (float)textGlobs.textOnlyWindow->font->height;
    textGlobs.panelTimer = panelTimer;
    textGlobs.panelX = (float)panelX;

    text_clear();
}

// FUNCTION: LEGORR 0x0046a9e0
void text_clear(void)
{
    textGlobs.currPanelY = (float)textGlobs.panelY;
    float_pair_at_offset4_set_from_ints(textGlobs.textOnlyWindow,
        (int)textGlobs.messageArea.x, (int)textGlobs.messageArea.y);
    float_pair_at_offsetc_set_from_uints(textGlobs.textOnlyWindow,
        (unsigned int)textGlobs.messageArea.width,
        (unsigned int)textGlobs.messageArea.height);
    textGlobs.currArea = textGlobs.messageArea;
    textGlobs.flags &= ~TEXT_FLAG_NERPSMESSAGE;
}

// FUNCTION: LEGORR 0x0046aa90
void text_restart(void)
{
    text_clear();
    textGlobs.currType = Text_Type_Count;
    textGlobs.counter = 0;
    textGlobs.currText = NULL;
    textGlobs.flags = 0;
    text_buffer_reset_target(textGlobs.textOnlyWindow);
    text_buffer_reset_target(textGlobs.textImageWindow);
}

// FUNCTION: LEGORR 0x0046aad0
int text_get_type_from_name(const char* name, int* type)
{
    unsigned int loop;

    for (loop = 0; loop < Text_Type_Count; loop++) {
        if (!_strcmpi(textGlobs.typeName[loop], name)) {
            *type = loop;
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x0046ab10
void text_set_message(int textType, const char* message)
{
    char* s;
    const char* t;

    if (textGlobs.messages[textType] != NULL) free(textGlobs.messages[textType]);
    textGlobs.messages[textType] = (char*)malloc(strlen(message) + 1);
    for (s = textGlobs.messages[textType], t = message; *t != '\0'; s++, t++) {
        if (*t == '_') *s = ' ';
        else if (*t == '\\' && t[1] == 'n') {
            *s = '\n';
            t++;
        } else *s = *t;
    }
    *s = '\0';

    if (textGlobs.images[textType] != NULL) {
        image_surface_release(textGlobs.images[textType]);
        textGlobs.images[textType] = NULL;
    }
}

// FUNCTION: LEGORR 0x0046abb0
void text_set_message_with_image(int textType, const char* message,
                                 const char* imageFile, const char* soundName)
{
    text_set_message(textType, message);
    sprintf(textGlobs.soundNames[textType], "%s", soundName);
    if ((textGlobs.images[textType] = image_load_bmp_scaled(imageFile, 0, 0)) != NULL) {
        image_surface_blit_transformed((struct TextureSurface*)textGlobs.images[textType],
                                       0, 0, 0, 0, 0, 0);
    }
}

// FUNCTION: LEGORR 0x0046ac20
void text_set_nerps_message(char* message, unsigned int counter)
{
    textGlobs.currText = message;
    if (counter == 0) counter = TEXT_COUNTER_PERMANENT;
    textGlobs.counter = counter | TEXT_COUNTER_NERPS;
    nerps_parse_message(message, NULL, 1);
    textGlobs.flags |= TEXT_FLAG_NERPSMESSAGE;
}

// FUNCTION: LEGORR 0x0046ac60
void text_display_message(int textType, int changeTiming, int setFlag4)
{
    int sfxID;

    if (changeTiming) {
        textGlobs.currType = (Text_Type)textType;
        textGlobs.timer = textGlobs.pauseTime;
    } else if (textGlobs.timer <= 0.0f) {
        textGlobs.currType = (Text_Type)textType;
    }

    if (!nerpfunc_get_tutorial_flags(NULL)) {
        if (sound_alias_lookup(textGlobs.soundNames[textType], &sfxID)) {
            sound_submit_with_global_cooldown(sfxID, NULL);
        }
    }

    if (textGlobs.currType == (Text_Type)textType && setFlag4) textGlobs.flags |= TEXT_FLAG_FLASH;
    else textGlobs.flags &= ~TEXT_FLAG_FLASH;
}

// FUNCTION: LEGORR 0x0046ad00
void text_update(float elapsed)
{
    Text_Type currType = textGlobs.currType;
    Image* image = NULL;
    int noImage = 1;
    TextWindow* window;
    char* message;
    int flash;
    int lowestPoint;
    float height;
    Point2F imagePos;
    char buffer[512];

    if (textGlobs.counter) {
        if (textGlobs.currText == NULL) textGlobs.counter = 0;
        else {
            nerps_parse_message(textGlobs.currText, buffer, 0);
            objective_advisor_text_copy(textGlobs.textOnlyWindow, 0, buffer);
            if (!(textGlobs.counter & TEXT_COUNTER_PERMANENT)) textGlobs.counter--;
        }
    } else if (nerps_get_message_permit() && textGlobs.currType != Text_Type_Count) {
        image = textGlobs.images[textGlobs.currType];
        message = textGlobs.messages[textGlobs.currType];
        if (image != NULL) {
            imagePos.x = (float)textGlobs.imageX;
            imagePos.y = (float)textGlobs.imageY;
            window = textGlobs.textImageWindow;
            noImage = 0;
        } else window = textGlobs.textOnlyWindow;

        if (message != NULL) {
            flash = 1;
            if (textGlobs.flags & TEXT_FLAG_FLASH) flash = sim_status_banner_word_get();
            if (flash) objective_advisor_text_copy(window, 1, message);
        }

        if (textGlobs.timer <= 0.0f) textGlobs.currType = Text_Type_Count;
        textGlobs.timer -= elapsed;
    }

    if ((legoGlobs_flags1 & 0x100000) && currType != Text_SpaceToContinue) {
        text_clear();
        indexed_pair_set(PANEL_MESSAGES, textGlobs.panelX, textGlobs.currPanelY);
        simtick_manager_tick(PANEL_MESSAGES, 0.0f);
        advisor_progress_bar_fill_draw(PANEL_MESSAGES, ((Lego_Level*)g_active_level_retail)->oxygenLevel);
        panel_resource_position_update(PANEL_MESSAGES);
        return;
    }

    if (noImage) {
        indexed_pair_set(PANEL_MESSAGES, textGlobs.panelX, textGlobs.currPanelY);
        simtick_manager_tick(PANEL_MESSAGES, 0.0f);
        advisor_progress_bar_fill_draw(PANEL_MESSAGES, ((Lego_Level*)g_active_level_retail)->oxygenLevel);
        panel_resource_position_update(PANEL_MESSAGES);

        if (text_buffer_draw_wrapped(textGlobs.textOnlyWindow, 0, elapsed, &lowestPoint)) {
            height = (float)textGlobs.textOnlyWindow->font->height;
            textGlobs.currPanelY -= height;
            textGlobs.currArea.y -= height;
            textGlobs.currArea.height += height;
            float_pair_at_offset4_set_from_ints(textGlobs.textOnlyWindow,
                (int)textGlobs.currArea.x, (int)textGlobs.currArea.y);
            float_pair_at_offsetc_set_from_uints(textGlobs.textOnlyWindow,
                (unsigned int)textGlobs.currArea.width,
                (unsigned int)textGlobs.currArea.height);
            textGlobs.flags &= ~TEXT_FLAG_NERPSMESSAGE;
        } else if (textGlobs.flags & TEXT_FLAG_NERPSMESSAGE) {
            if ((float)lowestPoint < textGlobs.messageBottom) {
                height = (float)textGlobs.textOnlyWindow->font->height;
                textGlobs.currPanelY += height;
                textGlobs.currArea.y += height;
                textGlobs.currArea.height -= height;
                float_pair_at_offset4_set_from_ints(textGlobs.textOnlyWindow,
                    (int)textGlobs.currArea.x, (int)textGlobs.currArea.y);
                float_pair_at_offsetc_set_from_uints(textGlobs.textOnlyWindow,
                    (unsigned int)textGlobs.currArea.width,
                    (unsigned int)textGlobs.currArea.height);
                if (textGlobs.currPanelY >= (float)textGlobs.panelY) {
                    textGlobs.currPanelY = (float)textGlobs.panelY;
                    textGlobs.flags &= ~TEXT_FLAG_NERPSMESSAGE;
                    textGlobs.currArea = textGlobs.messageArea;
                    float_pair_at_offset4_set_from_ints(textGlobs.textOnlyWindow,
                        (int)textGlobs.messageArea.x, (int)textGlobs.messageArea.y);
                    float_pair_at_offsetc_set_from_uints(textGlobs.textOnlyWindow,
                        (unsigned int)textGlobs.currArea.width,
                        (unsigned int)textGlobs.currArea.height);
                }
            } else textGlobs.flags &= ~TEXT_FLAG_NERPSMESSAGE;
        }
    } else {
        text_clear();
        indexed_pair_set(PANEL_MESSAGES, textGlobs.panelX, textGlobs.currPanelY);
        simtick_manager_tick(PANEL_MESSAGES, 0.0f);
        advisor_progress_bar_fill_draw(PANEL_MESSAGES, ((Lego_Level*)g_active_level_retail)->oxygenLevel);
        panel_resource_position_update(PANEL_MESSAGES);
        text_buffer_draw_wrapped(textGlobs.textImageWindow, 0, elapsed, NULL);
        image_display_scaled(image, NULL, &imagePos, NULL);
    }
}

#ifdef __cplusplus
}
#endif
