// SPDX-License-Identifier: LGPL-3.0-or-later
/* Panel.c — original translation unit GAME-032 (text 0045a030-0045c830,
 * .rdata 0049e5d8-0049e644, literals 004a8378-004a8a08, .bss
 * 005000e0-00500898; finding 505).
 *
 * Rebuilt in the original's shape (ADR 0005): one file for the unit, its
 * functions in original order, its state in one typed globals struct laid
 * out as the original .bss, and the C-compatible subset so the matching
 * build compiles it as C. The native port compiles it as C++.
 *
 * Callee contracts that this reconstruction has not converted yet are used
 * as they are; finding 554 lists them. Port-only code (accessors for the
 * native HUD, and the few guards the native port needs because it reaches
 * these functions before the original's state exists) is compiled out of
 * the matching build (ADR 0006). */
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "panel_unit.h"
#include "frontend.h"
#include "interface.h"
#include "lego.h"
#include "message.h"
#include "tutorial_script.h"
#include "priorities.h"
#include "advisor.h"
#include "crt/vc5names.h"
#include "gods98/config.h"
#include "gods98/images.h"
#include "gods98/image_adapters.h"
#include "gods98/input.h"
#include "gods98/utils.h"
#include "gods98/textwindow.h"
#include "gods98/flic.h"
#include "gods98/engine.h"
#include "camera_control.h"
#include "info.h"
#include "ai.h"
#include "lego_globs.h"

#if !(defined(_MSC_VER) && _MSC_VER == 1100)
#define PANEL_PORT 1
#endif

/* Other units read these fields directly (Lego.c, Interface.c, Info.c,
 * Priorities.c), so the struct is external. The zero initialiser keeps it
 * in this object's .bss rather than the common region (finding 554). */
// GLOBAL: LEGORR 0x005000e0
PanelGlobs panelGlobs = { 0 };

// FUNCTION: LEGORR 0x0045a030
void ui_panel_name_tables_init(void)
{
    panelGlobs.selectedPanel = PANEL_TYPE_COUNT;
    panelGlobs.selectedButton = PANEL_MAX_BUTTONS;

    // STRING: LEGORR 0x004a8930
    panelGlobs.panelName[11] = "Panel_Encyclopedia";
    // STRING: LEGORR 0x004a8924
    panelGlobs.panelName[0] = "Panel_Radar";
    // STRING: LEGORR 0x004a8914
    panelGlobs.panelName[1] = "Panel_RadarFill";
    // STRING: LEGORR 0x004a8900
    panelGlobs.panelName[2] = "Panel_RadarOverlay";
    // STRING: LEGORR 0x004a88f0
    panelGlobs.panelName[3] = "Panel_Messages";
    // STRING: LEGORR 0x004a88dc
    panelGlobs.panelName[4] = "Panel_MessagesSide";
    // STRING: LEGORR 0x004a88c4
    panelGlobs.panelName[5] = "Panel_CrystalSideBar";
    // STRING: LEGORR 0x004a88b4
    panelGlobs.panelName[6] = "Panel_TopPanel";
    // STRING: LEGORR 0x004a88a0
    panelGlobs.panelName[7] = "Panel_Information";
    // STRING: LEGORR 0x004a888c
    panelGlobs.panelName[8] = "Panel_PriorityList";
    // STRING: LEGORR 0x004a8878
    panelGlobs.panelName[9] = "Panel_CameraControl";
    // STRING: LEGORR 0x004a8868
    panelGlobs.panelName[10] = "Panel_InfoDock";

    // STRING: LEGORR 0x004a884c
    panelGlobs.buttonName[0][0] = "PanelButton_Radar_Toggle";
    // STRING: LEGORR 0x004a8828
    panelGlobs.buttonName[0][1] = "PanelButton_Radar_TaggedObjectView";
    // STRING: LEGORR 0x004a880c
    panelGlobs.buttonName[0][2] = "PanelButton_Radar_ZoomIn";
    // STRING: LEGORR 0x004a87f0
    panelGlobs.buttonName[0][3] = "PanelButton_Radar_ZoomOut";
    // STRING: LEGORR 0x004a87d4
    panelGlobs.buttonName[0][4] = "PanelButton_Radar_MapView";
    // STRING: LEGORR 0x004a87b8
    panelGlobs.buttonName[10][0] = "PanelButton_InfoDock_Goto";
    // STRING: LEGORR 0x004a879c
    panelGlobs.buttonName[10][1] = "PanelButton_InfoDock_Close";
    // STRING: LEGORR 0x004a877c
    panelGlobs.buttonName[6][0] = "PanelButton_TopPanel_Options";
    // STRING: LEGORR 0x004a875c
    panelGlobs.buttonName[6][1] = "PanelButton_TopPanel_Priorities";
    // STRING: LEGORR 0x004a873c
    panelGlobs.buttonName[6][2] = "PanelButton_TopPanel_CallToArms";
    // STRING: LEGORR 0x004a871c
    panelGlobs.buttonName[7][0] = "PanelButton_Information_Toggle";
    // STRING: LEGORR 0x004a86f8
    panelGlobs.buttonName[7][1] = "PanelButton_Information_Function";
    // STRING: LEGORR 0x004a86d4
    panelGlobs.buttonName[8][0] = "PanelButton_PriorityList_Disable1";
    // STRING: LEGORR 0x004a86b0
    panelGlobs.buttonName[8][1] = "PanelButton_PriorityList_Disable2";
    // STRING: LEGORR 0x004a868c
    panelGlobs.buttonName[8][2] = "PanelButton_PriorityList_Disable3";
    // STRING: LEGORR 0x004a8668
    panelGlobs.buttonName[8][3] = "PanelButton_PriorityList_Disable4";
    // STRING: LEGORR 0x004a8644
    panelGlobs.buttonName[8][4] = "PanelButton_PriorityList_Disable5";
    // STRING: LEGORR 0x004a8620
    panelGlobs.buttonName[8][5] = "PanelButton_PriorityList_Disable6";
    // STRING: LEGORR 0x004a85fc
    panelGlobs.buttonName[8][6] = "PanelButton_PriorityList_Disable7";
    // STRING: LEGORR 0x004a85d8
    panelGlobs.buttonName[8][7] = "PanelButton_PriorityList_Disable8";
    // STRING: LEGORR 0x004a85b4
    panelGlobs.buttonName[8][8] = "PanelButton_PriorityList_Disable9";
    // STRING: LEGORR 0x004a8594
    panelGlobs.buttonName[8][9] = "PanelButton_PriorityList_UpOne1";
    // STRING: LEGORR 0x004a8574
    panelGlobs.buttonName[8][10] = "PanelButton_PriorityList_UpOne2";
    // STRING: LEGORR 0x004a8554
    panelGlobs.buttonName[8][11] = "PanelButton_PriorityList_UpOne3";
    // STRING: LEGORR 0x004a8534
    panelGlobs.buttonName[8][12] = "PanelButton_PriorityList_UpOne4";
    // STRING: LEGORR 0x004a8514
    panelGlobs.buttonName[8][13] = "PanelButton_PriorityList_UpOne5";
    // STRING: LEGORR 0x004a84f4
    panelGlobs.buttonName[8][14] = "PanelButton_PriorityList_UpOne6";
    // STRING: LEGORR 0x004a84d4
    panelGlobs.buttonName[8][15] = "PanelButton_PriorityList_UpOne7";
    // STRING: LEGORR 0x004a84b4
    panelGlobs.buttonName[8][16] = "PanelButton_PriorityList_UpOne8";
    // STRING: LEGORR 0x004a8494
    panelGlobs.buttonName[8][17] = "PanelButton_PriorityList_Close";
    // STRING: LEGORR 0x004a8474
    panelGlobs.buttonName[8][18] = "PanelButton_PriorityList_Reset";
    // STRING: LEGORR 0x004a8454
    panelGlobs.buttonName[11][0] = "PanelButton_Encyclopedia_Close";
    // STRING: LEGORR 0x004a8430
    panelGlobs.buttonName[9][0] = "PanelButton_CameraControl_ZoomIn";
    // STRING: LEGORR 0x004a840c
    panelGlobs.buttonName[9][1] = "PanelButton_CameraControl_ZoomOut";
    // STRING: LEGORR 0x004a83e0
    panelGlobs.buttonName[9][2] = "PanelButton_CameraControl_CycleBuildings";
    // STRING: LEGORR 0x004a83bc
    panelGlobs.buttonName[9][3] = "PanelButton_CameraControl_Rotate";
    // STRING: LEGORR 0x004a839c
    panelGlobs.buttonName[5][0] = "PanelButton_CrystalSideBar_Ore";
    // STRING: LEGORR 0x004a8378
    panelGlobs.buttonName[5][1] = "PanelButton_CrystalSideBar_Crystals";
}

// FUNCTION: LEGORR 0x0045a240
void legacy_scroll_panel_controls_initialize(void)
{
    ui_scroll_info_panel_initialize();
    ui_overlay_create_configured_element();
    panel_item_flag32_set(PANEL_TOP, 1, 1);
    panel_item_flag32_set(PANEL_TOP, 2, 1);
}

// FUNCTION: LEGORR 0x0045a270
void legacy_panel_category_interaction_state_reset(void)
{
    unsigned int panelType;
    unsigned int i;
    PanelButton* button;
    unsigned int flags;

    for (panelType = 0; panelType < PANEL_TYPE_COUNT; panelType++) {
        flags = panelGlobs.panelTable[panelType].flags;
        panelGlobs.panelTable[panelType].xCurr = panelGlobs.panelTable[panelType].xIn;
        panelGlobs.panelTable[panelType].flags = (flags & ~(PANEL_FLAG_OUT | PANEL_FLAG_MOVING)) | PANEL_FLAG_IN;
        panelGlobs.panelTable[panelType].yCurr = panelGlobs.panelTable[panelType].yIn;
        for (i = 0; i < panelGlobs.panelTable[panelType].buttonCount; i++) {
            button = &panelGlobs.panelTable[panelType].buttonList[i];
            button->clickCount = 0;
            button->flags &= ~(PANEL_BUTTON_FLAG_TOGGLED | PANEL_BUTTON_FLAG_BANNER);
        }
    }
    panelGlobs.sideBarMeterLevel = 0.0f;
}

// FUNCTION: LEGORR 0x0045a2e0
void legacy_overlay_image_record_load(const char* fileName, int panelType,
                                      unsigned int flags)
{
    PanelData* panel = &panelGlobs.panelTable[panelType];

    panel->buttonCount = 0;
    panel->flags = flags;
    if ((panel->image = image_load_bmp_scaled(fileName, 0, 0)) != NULL) {
        panel->isFlic = 0;
        image_setup_trans((Image*)panel->image, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        panel->flags |= PANEL_FLAG_OPEN;
    } else if (flic_setup(fileName, &panel->image, 3)) {
        panel->isFlic = 1;
        panel->flags |= PANEL_FLAG_OPEN;
    }
}

// FUNCTION: LEGORR 0x0045a370
int ui_panel_name_table_lookup(const char* name, int* panelType)
{
    unsigned int i;

    for (i = 0; i < PANEL_TYPE_COUNT; i++) {
        if (stricmp(name, panelGlobs.panelName[i]) == 0) {
            *panelType = i;
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x0045a3b0
int legacy_panel_slot_coordinate_hit_test(int panelType, int x, int y)
{
    PanelData* panel = &panelGlobs.panelTable[panelType];
    unsigned int pixel;

    if (!panel->isFlic) {
        if (image_get_pixel((Image*)panel->image, x - (int)panel->xCurr,
                            y - (int)panel->yCurr, &pixel)) {
            return pixel == 0;
        }
    }
    return 1;
}

// FUNCTION: LEGORR 0x0045a410
int ui_panel_indexed_name_lookup(int panelType, const char* name,
                                 unsigned int* button)
{
    unsigned int i;
    const char** names = panelGlobs.buttonName[panelType];

    for (i = 0; i < PANEL_MAX_BUTTONS; i++) {
        if (names[i] != NULL && stricmp(name, names[i]) == 0) {
            *button = i;
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x0045a460
void panel_print_f(int panelType, const struct TextureSurface* font, int x,
                   int y, int center, const char* msg, ...)
{
    PanelData* panel = &panelGlobs.panelTable[panelType];
    va_list args;
    uint32_t width;

    va_start(args, msg);
    if (center) {
        font_v_get_string_info(font, &width, NULL, msg, args);
        x -= width >> 1;
    }
    font_v_print_f(font, (int)floor(x + panel->xCurr),
                   (int)floor(y + panel->yCurr), msg, args);
    va_end(args);
}

// FUNCTION: LEGORR 0x0045a500
void* ui_text_output_record_allocate(int panelType, uintptr_t font,
                                     const float* rect, unsigned int size)
{
    PanelTextWindow* window;
    Area2F area;

    area.x = 0.0f;
    area.y = 0.0f;
    area.width = rect[2];
    area.height = rect[3];
    if ((window = (PanelTextWindow*)malloc(sizeof(PanelTextWindow))) != NULL) {
        memset(window, 0, sizeof(PanelTextWindow));
        window->textWindow = text_buffer_create(font, (const uint32_t*)&area, size);
        window->x = (int)rect[0];
        window->y = (int)rect[1];
        window->panel = &panelGlobs.panelTable[panelType];
    }
    return window;
}

// FUNCTION: LEGORR 0x0045a590
void panel_text_window_print_f(PanelTextWindow* window, const char* msg, ...)
{
    va_list args;

    va_start(args, msg);
    text_buffer_append_vformat(window->textWindow, 0, msg, args);
    va_end(args);
}

// FUNCTION: LEGORR 0x0045a5b0
void ui_text_output_position_and_draw(void* window, unsigned int posFromEnd,
                                      float elapsed)
{
    PanelTextWindow* textWindow = (PanelTextWindow*)window;

#ifdef PANEL_PORT
    if (textWindow == NULL) return; /* port: the native HUD may have no window */
#endif
    float_pair_at_offset4_set_from_ints(textWindow->textWindow,
        (int)floor(textWindow->x + textWindow->panel->xCurr),
        (int)floor(textWindow->y + textWindow->panel->yCurr));
    text_buffer_draw_wrapped(textWindow->textWindow, posFromEnd, elapsed, NULL);
}

// FUNCTION: LEGORR 0x0045a620
void ui_text_output_object_release_target(void* const* window)
{
    text_buffer_reset_target(*window);
}

// FUNCTION: LEGORR 0x0045a630
void ui_record_callback_forward(const void* const* window, uint32_t* outA,
                                uint32_t* outB)
{
#ifdef PANEL_PORT
    if (window == NULL) { *outA = 0; *outB = 0; return; } /* port */
#endif
    object_state_pair_get_target(*window, outA, outB);
}

// FUNCTION: LEGORR 0x0045a650
void panel_category_bounds_set(int panelType, int xOut, int yOut, int xIn,
                               int yIn)
{
    PanelData* panel = &panelGlobs.panelTable[panelType];
    Point2F out;
    Point2F in;

    out.x = (float)xOut;
    out.y = (float)yOut;
    in.x = (float)xIn;
    in.y = (float)yIn;
    panel->xOut = out.x;
    panel->yOut = out.y;
    panel->xIn = in.x;
    panel->yIn = in.y;
    panel->reserved20 = 0;
    if (panel->flags & PANEL_FLAG_OUT) {
        panel->xCurr = out.x;
        panel->yCurr = out.y;
    } else {
        panel->xCurr = in.x;
        panel->yCurr = in.y;
    }
}

// FUNCTION: LEGORR 0x0045a6e0
void indexed_pair_set(int panelType, float x, float y)
{
    panelGlobs.panelTable[panelType].xCurr = x;
    panelGlobs.panelTable[panelType].yCurr = y;
}

// FUNCTION: LEGORR 0x0045a700
void indexed_pair_get(int panelType, float* x, float* y)
{
    PanelData* panel = &panelGlobs.panelTable[panelType];

    *x = panel->xCurr;
    *y = panel->yCurr;
}

// FUNCTION: LEGORR 0x0045a730
void simtick_manager_tick(int panelType, float elapsed)
{
    PanelData* panel = &panelGlobs.panelTable[panelType];
    Point2F target;
    Point2F position;
    Area2F flicArea;

    if (panel->flags & PANEL_FLAG_IN) {
        target.x = panel->xIn;
        target.y = panel->yIn;
    } else {
        target.x = panel->xOut;
        target.y = panel->yOut;
    }

    if (elapsed != 0.0f) {
        if (fabs(panel->xCurr - target.x) > 1.0) {
            panel->xCurr = ((4.0f / elapsed) * panel->xCurr + target.x) / ((4.0f / elapsed) + 1.0f);
        } else {
            panel->xCurr = target.x;
        }
        if (fabs(panel->yCurr - target.y) > 1.0) {
            panel->yCurr = ((4.0f / elapsed) * panel->yCurr + target.y) / ((4.0f / elapsed) + 1.0f);
        } else {
            panel->yCurr = target.y;
        }
    }

    flicArea.x = position.x = (float)floor(panel->xCurr);
    flicArea.y = position.y = (float)floor(panel->yCurr);

    if (panel->isFlic == 0) {
        image_display_scaled((Image*)panel->image, NULL, &position, NULL);
    } else if (panel->isFlic == 1) {
        flicArea.width = (float)(unsigned int)flic_width((struct Flic*)panel->image);
        flicArea.height = (float)(unsigned int)flic_height((struct Flic*)panel->image);
        flic_animate(panel->image, &flicArea, !(*engine_shared_state_word() & 0x100000), 1);
    }

    if (panelType == PANEL_CAMERA_CONTROL) {
        if (panelGlobs.flags & PANEL_GLOB_FLAG_ROTATION_UP) {
            image_display_scaled(panelGlobs.rotationImages[0], NULL, &panelGlobs.rotationImagePositions[0], NULL);
            panelGlobs.flags &= ~PANEL_GLOB_FLAG_ROTATION_UP;
        }
        if (panelGlobs.flags & PANEL_GLOB_FLAG_ROTATION_DOWN) {
            image_display_scaled(panelGlobs.rotationImages[1], NULL, &panelGlobs.rotationImagePositions[1], NULL);
            panelGlobs.flags &= ~PANEL_GLOB_FLAG_ROTATION_DOWN;
        }
        if (panelGlobs.flags & PANEL_GLOB_FLAG_ROTATION_LEFT) {
            image_display_scaled(panelGlobs.rotationImages[2], NULL, &panelGlobs.rotationImagePositions[2], NULL);
            panelGlobs.flags &= ~PANEL_GLOB_FLAG_ROTATION_LEFT;
        }
        if (panelGlobs.flags & PANEL_GLOB_FLAG_ROTATION_RIGHT) {
            image_display_scaled(panelGlobs.rotationImages[3], NULL, &panelGlobs.rotationImagePositions[3], NULL);
            panelGlobs.flags &= ~PANEL_GLOB_FLAG_ROTATION_RIGHT;
        }
    }

    if (abs((int)(panel->xCurr - target.x)) <= 1 &&
        abs((int)(panel->yCurr - target.y)) <= 1) {
        panel->flags &= ~PANEL_FLAG_MOVING;
    }

    panel_texture_slots_blit_to(panelType);
}

// FUNCTION: LEGORR 0x0045a9c0
void panel_texture_slots_blit_to(int panelType)
{
    PanelData* panel = &panelGlobs.panelTable[panelType];
    PanelButton* button;
    unsigned int i;
    unsigned int flash;
    unsigned int toggled;
    Point2F position;

    for (i = 0; i < panel->buttonCount; i++) {
        button = &panel->buttonList[i];
        position.x = button->x1 + panel->xCurr;
        position.y = button->y1 + panel->yCurr;
        if (!(button->flags & PANEL_BUTTON_FLAG_HIDDEN)) {
            flash = button->flags & PANEL_BUTTON_FLAG_BANNER;
            toggled = button->flags & PANEL_BUTTON_FLAG_TOGGLED;
            if (!toggled && (button->flags & PANEL_BUTTON_FLAG_HIGHLIGHT)) {
                if (!flash || sim_status_banner_word_get()) {
                    if (button->highlightImage != NULL) image_display_scaled(button->highlightImage, NULL, &position, NULL);
                }
            } else if (!(button->flags & PANEL_BUTTON_FLAG_PRESSED) && !toggled) {
                if (flash && sim_status_banner_word_get()) {
                    if (button->highlightImage != NULL) image_display_scaled(button->highlightImage, NULL, &position, NULL);
                } else {
                    if (button->normalImage != NULL) image_display_scaled(button->normalImage, NULL, &position, NULL);
                }
            } else {
                if (flash && sim_status_banner_word_get()) {
                    if (button->highlightImage != NULL) image_display_scaled(button->highlightImage, NULL, &position, NULL);
                } else {
                    if (button->pressedImage != NULL) image_display_scaled(button->pressedImage, NULL, &position, NULL);
                }
            }
        }
    }
}

// FUNCTION: LEGORR 0x0045aac0
void panel_item_flag16_set(int panelType, int button, int on)
{
    PanelData* panel = &panelGlobs.panelTable[panelType];

    if (on) panel->buttonList[button].flags |= PANEL_BUTTON_FLAG_HIDDEN;
    else panel->buttonList[button].flags &= ~PANEL_BUTTON_FLAG_HIDDEN;
}

// FUNCTION: LEGORR 0x0045ab00
void panel_state_flags_toggle(int panelType)
{
    PanelData* panel = &panelGlobs.panelTable[panelType];

    if (panel->flags & PANEL_FLAG_OUT) {
        panel->flags &= ~PANEL_FLAG_OUT;
        panel->flags |= PANEL_FLAG_IN | PANEL_FLAG_MOVING;
    } else {
        panel->flags &= ~PANEL_FLAG_IN;
        panel->flags |= PANEL_FLAG_OUT | PANEL_FLAG_MOVING;
    }
}

// FUNCTION: LEGORR 0x0045ab30
int request_record_ready_flag4_target(int panelType)
{
    PanelData* panel = &panelGlobs.panelTable[panelType];

    if ((panel->flags & PANEL_FLAG_IN) && !(panel->flags & PANEL_FLAG_MOVING)) return 1;
    return 0;
}

// FUNCTION: LEGORR 0x0045ab60
int request_record_ready_flag2_target(int panelType)
{
    PanelData* panel = &panelGlobs.panelTable[panelType];

    if ((panel->flags & PANEL_FLAG_OUT) && !(panel->flags & PANEL_FLAG_MOVING)) return 1;
    return 0;
}

// FUNCTION: LEGORR 0x0045ab90
int request_record_flag8_is_set_target(int panelType)
{
    return panelGlobs.panelTable[panelType].flags & PANEL_FLAG_MOVING;
}

// FUNCTION: LEGORR 0x0045abb0
void panel_item_flag32_set(int panelType, int button, int on)
{
    PanelButton* b = &panelGlobs.panelTable[panelType].buttonList[button];

    if (on) b->flags |= PANEL_BUTTON_FLAG_TOGGLE;
    else b->flags &= ~PANEL_BUTTON_FLAG_TOGGLE;
}

// FUNCTION: LEGORR 0x0045abf0
void panel_item_flag8_set_if_enabled(int panelType, int button, int on)
{
    PanelButton* b = &panelGlobs.panelTable[panelType].buttonList[button];

    if (b->flags & PANEL_BUTTON_FLAG_TOGGLE) {
        if (on) b->flags |= PANEL_BUTTON_FLAG_TOGGLED;
        else b->flags &= ~(PANEL_BUTTON_FLAG_TOGGLED | PANEL_BUTTON_FLAG_PRESSED);
    }
}

// FUNCTION: LEGORR 0x0045ac30
void panel_texture_slots_allocate(int panelType, unsigned int count,
                                  const unsigned int* buttonTypes,
                                  const float* areas,
                                  char** normalNames,
                                  char** highlightNames,
                                  char** pressedNames,
                                  const int* toolTips)
{
    PanelData* panel;
    PanelButton* button;
    unsigned int i;

    if (count) {
        panel = &panelGlobs.panelTable[panelType];
        panel->buttonList = (PanelButton*)malloc(count * sizeof(PanelButton));
        panel->buttonCount = count;
        for (i = 0; i < count; i++) {
            button = &panel->buttonList[buttonTypes[i]];
            button->x1 = areas[i * 4 + 0];
            button->y1 = areas[i * 4 + 1];
            button->x2 = areas[i * 4 + 2] + areas[i * 4 + 0];
            button->y2 = areas[i * 4 + 3] + areas[i * 4 + 1];
            button->normalImage = NULL;
            button->highlightImage = NULL;
            button->pressedImage = NULL;
            button->toolTip = toolTips[i];
            if (normalNames != NULL && normalNames[i] != NULL) {
                if ((button->normalImage = image_load_bmp_scaled(normalNames[i], 0, 0)) != NULL) {
                    image_setup_trans(button->normalImage, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
                }
            }
            if (highlightNames != NULL && highlightNames[i] != NULL) {
                if ((button->highlightImage = image_load_bmp_scaled(highlightNames[i], 0, 0)) != NULL) {
                    image_setup_trans(button->highlightImage, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
                }
            }
            if (pressedNames != NULL && pressedNames[i] != NULL) {
                if ((button->pressedImage = image_load_bmp_scaled(pressedNames[i], 0, 0)) != NULL) {
                    image_setup_trans(button->pressedImage, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
                }
            }
            button->flags = 0;
        }
    }
}

// FUNCTION: LEGORR 0x0045adb0
int panel_cursor_interaction_update(float elapsed, int mouseX, int mouseY,
                                    int leftButton, int leftButtonLast,
                                    int* overPanel)
{
    int panelType;
    int button;
    int clicked;
    int infoReleased;
    unsigned int flags;

    if (!leftButton) panelGlobs.flags &= ~PANEL_GLOB_FLAG_ROTATION_DRAG;

    if (panel_button_collision_check(&panelType, &button, mouseX, mouseY,
                                     leftButton, leftButtonLast, &clicked,
                                     overPanel)) {
        *engine_shared_state_word() |= 0x200;
        if (panelType == PANEL_TYPE_COUNT || button == PANEL_MAX_BUTTONS) return panelType;

        flags = system_state_flags_get();
        if (clicked) {
            panelGlobs.panelTable[panelType].buttonList[button].clickCount++;
            if ((flags & 0x10) && panelType == PANEL_RADAR) {
                media_action_feedback_sound_dispatch(1);
                return 1;
            }
            if ((flags & 0x20) && panelType == PANEL_TOP && button == 0) {
                panel_item_flag8_set_if_enabled(PANEL_TOP, 0, 0);
                media_action_feedback_sound_dispatch(1);
                return 1;
            }
            if ((flags & 0x40) && panelType == PANEL_TOP && button == 1) {
                panel_item_flag8_set_if_enabled(PANEL_TOP, 1, 0);
                media_action_feedback_sound_dispatch(1);
                return 1;
            }
            if ((flags & 0x80) && panelType == PANEL_TOP && button == 2) {
                panel_item_flag8_set_if_enabled(PANEL_TOP, 2, 0);
                media_action_feedback_sound_dispatch(1);
                return 1;
            }
            if ((flags & 0x400) && panelType == PANEL_CAMERA_CONTROL && button == 2) {
                media_action_feedback_sound_dispatch(1);
                return 1;
            }
            if ((flags & 0x1000) && panelType == PANEL_CAMERA_CONTROL) {
                if (button == 0 || button == 1 || button == 3) {
                    media_action_feedback_sound_dispatch(1);
                    return 1;
                }
            }
        }

        if (leftButton) {
            if (panelType != PANEL_RADAR) {
                if (panelType == PANEL_CAMERA_CONTROL && !(system_state_flags_get() & 0x1000)) {
                camera_zoom_step_adjust(button, elapsed);
                if (button == 3) panel_rotation_control_drag(mouseX, mouseY, elapsed);
                }
            } else {
                if (*engine_shared_state_word() & 0x1000) {
                    if (button == 2) {
                        if (s_lego_radar_zoom < 20.0f) s_lego_radar_zoom += elapsed;
                        if (s_lego_radar_zoom > 20.0f) s_lego_radar_zoom = 20.0f;
                    } else if (button == 3) {
                        if (s_lego_radar_zoom > 10.0f) s_lego_radar_zoom -= elapsed;
                        if (s_lego_radar_zoom < 10.0f) s_lego_radar_zoom = 10.0f;
                    }
                }
            }
        }

        if (!clicked) return 1;

        if (*(int*)((char*)level_current_owner_get() + 0x27c) == 0) {
            media_action_feedback_sound_dispatch(0);
            switch (panelType) {
            case PANEL_CAMERA_CONTROL:
                if (button == 2) global_scatter_spawn_enable_and_drain();
                break;
            case PANEL_RADAR:
                if (button == 0) {
                    panel_state_flags_toggle(0);
                    panel_state_flags_toggle(1);
                } else if (button == 1) {
                    *engine_shared_state_word() = (*engine_shared_state_word() & ~0x1000) | 0x2000;
                    panel_item_flag16_set(PANEL_RADAR, 2, 1);
                    panel_item_flag16_set(PANEL_RADAR, 3, 1);
                } else if (button == 4) {
                    *engine_shared_state_word() = (*engine_shared_state_word() & ~0x2000) | 0x1000;
                    panel_item_flag16_set(PANEL_RADAR, 2, 0);
                    panel_item_flag16_set(PANEL_RADAR, 3, 0);
                }
                break;
            case PANEL_TOP:
                if (s_context_mode && button == 1) {
                    if (panelGlobs.panelTable[PANEL_PRIORITY_LIST].flags & PANEL_FLAG_IN) {
                        cmd_produce(3, NULL, 0, NULL);
                        command_context_post_to_hud(0x16, NULL);
                    } else {
                        selection_clear_request_and_hud_reset();
                    }
                }
                if (button == 0) {
                    ledger_stub_00425c30_TransitionControl(1, 0);
                    s_panel_highlight_flags ^= 0x100;
                } else if (button == 2) {
                    if (s_panel_highlight_flags & 1) lego_panel_selection_highlight_toggle(0);
                    else lego_panel_selection_highlight_toggle(1);
                }
                break;
            case PANEL_PRIORITY_LIST:
                if (button == 18) {
                    priorities_reset();
                } else if (button == 17) {
                    if (!(panelGlobs.panelTable[PANEL_PRIORITY_LIST].flags & PANEL_FLAG_IN)) {
                        selection_clear_request_and_hud_reset();
                    }
                } else {
                    ui_list_priority_button_action_dispatch(button);
                }
                break;
            case PANEL_INFO_DOCK:
                if (button == 1) {
                    pending_record_release_if_present();
                } else if (button == 0) {
                    pending_record_list_entry_move_to_front(0);
                    incident_message_panel_refresh_if_active();
                }
                break;
            case PANEL_ENCYCLOPEDIA:
                if (button == 0 && !(panelGlobs.panelTable[PANEL_ENCYCLOPEDIA].flags & PANEL_FLAG_IN)) {
                    selection_clear_request_and_hud_reset();
                }
                break;
            }
            return 1;
        }
    } else {
        if (!info_panel_check_collision(&infoReleased, mouseX, mouseY,
                                        leftButton, leftButtonLast)) {
            return 0;
        }
        if (!infoReleased) screen_right_edge_cache_refresh();
        if (!clicked) return 1;
    }

    media_action_feedback_sound_dispatch(1);
    return 1;
}

// FUNCTION: LEGORR 0x0045b310
int panel_button_collision_check(int* outPanel, int* outButton, int mouseX,
                                 int mouseY, int leftButton,
                                 int leftButtonLast, int* outClicked,
                                 int* overPanel)
{
    int found = 0;
    unsigned int panelType;
    unsigned int i;
    PanelButton* button;
    int inside;

    if (outClicked != NULL) *outClicked = 0;
    *outButton = PANEL_MAX_BUTTONS;
    *outPanel = PANEL_TYPE_COUNT;

    if (panelGlobs.flags & PANEL_GLOB_FLAG_ROTATION_DRAG) {
        *outPanel = PANEL_CAMERA_CONTROL;
        *outButton = 3;
        return 1;
    }

    for (panelType = 0; panelType < PANEL_TYPE_COUNT; panelType++) {
        if (panelGlobs.panelTable[panelType].flags & PANEL_FLAG_OPEN) {
            if (overPanel != NULL && panelType != 1 && panelType != 2 &&
                !legacy_panel_slot_coordinate_hit_test(panelType, mouseX, mouseY)) {
                *overPanel = 1;
            }
            for (i = 0; i < panelGlobs.panelTable[panelType].buttonCount; i++) {
                button = &panelGlobs.panelTable[panelType].buttonList[i];
                inside = 0;
                if (!found) {
                    if (panelType == PANEL_CAMERA_CONTROL && i == 3) {
                        inside = point_distance_within_radius(mouseX, mouseY,
                            panelGlobs.rotationCenterX, panelGlobs.rotationCenterY,
                            panelGlobs.rotationRadius);
                    } else {
                        inside = panel_button_point_inside(&panelGlobs.panelTable[panelType], button, mouseX, mouseY);
                    }
                }
                if (!found && inside) {
                    *outPanel = panelType;
                    if (leftButton) {
                        if (panelGlobs.selectedButton == PANEL_MAX_BUTTONS) {
                            if (!leftButtonLast) {
                                panelGlobs.selectedPanel = panelType;
                                panelGlobs.selectedButton = i;
                                found = 1;
                                *outButton = i;
                                button->flags = (button->flags & ~PANEL_BUTTON_FLAG_HIGHLIGHT) | PANEL_BUTTON_FLAG_PRESSED;
                            } else {
                                found = 1;
                            }
                        } else if (panelType == panelGlobs.selectedPanel && i == panelGlobs.selectedButton) {
                            found = 1;
                            *outButton = i;
                            button->flags = (button->flags & ~PANEL_BUTTON_FLAG_HIGHLIGHT) | PANEL_BUTTON_FLAG_PRESSED;
                        } else {
                            found = 1;
                        }
                    } else {
                        if (button->toolTip == 26 && s_lego_object_tool_tip_gate == 0 &&
                            (s_entity_state_flags & 0x40)) {
                            timed_effect_update_request_set_corpus(35);
                        } else {
                            timed_effect_update_request_set_corpus(button->toolTip);
                        }
                        if (panelGlobs.selectedButton == PANEL_MAX_BUTTONS) {
                            found = 1;
                            *outButton = i;
                            button->flags = (button->flags & ~PANEL_BUTTON_FLAG_PRESSED) | PANEL_BUTTON_FLAG_HIGHLIGHT;
                        } else if (panelType == panelGlobs.selectedPanel && i == panelGlobs.selectedButton) {
                            *outButton = i;
                            if (button->flags & PANEL_BUTTON_FLAG_TOGGLE) {
                                if (button->flags & PANEL_BUTTON_FLAG_TOGGLED) button->flags &= ~PANEL_BUTTON_FLAG_TOGGLED;
                                else button->flags |= PANEL_BUTTON_FLAG_TOGGLED;
                            }
                            if (outClicked != NULL) *outClicked = 1;
                            button->flags = (button->flags & ~PANEL_BUTTON_FLAG_PRESSED) | PANEL_BUTTON_FLAG_HIGHLIGHT;
                            found = 1;
                        } else {
                            found = 1;
                        }
                    }
                } else {
                    button->flags &= ~PANEL_BUTTON_FLAG_HIGHLIGHT;
                    if (button->flags & PANEL_BUTTON_FLAG_TOGGLED) button->flags |= PANEL_BUTTON_FLAG_PRESSED;
                    else button->flags &= ~PANEL_BUTTON_FLAG_PRESSED;
                }
            }
        }
    }

    if (!leftButton) {
        panelGlobs.selectedPanel = PANEL_TYPE_COUNT;
        panelGlobs.selectedButton = PANEL_MAX_BUTTONS;
    }
    return found;
}

// FUNCTION: LEGORR 0x0045b590
int panel_button_point_inside(const PanelData* panel,
                              const PanelButton* button, int x, int y)
{
    x -= (int)panel->xCurr;
    y -= (int)panel->yCurr;
    if ((float)x >= button->x1 && (float)y < button->y2 &&
        (float)y >= button->y1 && (float)x < button->x2) {
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0045b610
void simtick_global_ui_flags_clear_bits12(void)
{
    panelGlobs.flags &= ~0x6;
}

// FUNCTION: LEGORR 0x0045b620
void ui_list_priority_button_action_dispatch(int button)
{
    switch (button) {
    case 0: priorities_toggle(0); break;
    case 1: priorities_toggle(1); break;
    case 2: priorities_toggle(2); break;
    case 3: priorities_toggle(3); break;
    case 4: priorities_toggle(4); break;
    case 5: priorities_toggle(5); break;
    case 6: priorities_toggle(6); break;
    case 7: priorities_toggle(7); break;
    case 8: priorities_toggle(8); break;
    case 9: priorities_move_up(1); break;
    case 10: priorities_move_up(2); break;
    case 11: priorities_move_up(3); break;
    case 12: priorities_move_up(4); break;
    case 13: priorities_move_up(5); break;
    case 14: priorities_move_up(6); break;
    case 15: priorities_move_up(7); break;
    case 16: priorities_move_up(8); break;
    }
}

// FUNCTION: LEGORR 0x0045b740
void ui_scroll_info_panel_initialize(void)
{
    Area2F rect;

    // STRING: LEGORR 0x004a8968
    info_panel_initialise("Interface\\InfoPanel\\scrollInfo.bmp", 0, 0, 7, 0);
    indexed_float_pair_set(0, 8, 33);
    rect.x = 0.0f;
    rect.y = 0.0f;
    rect.width = 18.0f;
    rect.height = 11.0f;
    scroll_rect_resource_record_create(0, 1, &rect.x, 0, 0);
    rect.x = 0.0f;
    rect.y = 11.0f;
    rect.width = 18.0f;
    rect.height = 11.0f;
    scroll_rect_resource_record_create(0, 0, &rect.x, 0, 0);
    rect.x = 0.0f;
    rect.y = 0.0f;
    rect.width = 0.0f;
    rect.height = 0.0f;
    // STRING: LEGORR 0x004a8944
    scroll_rect_payload_record_create(0, &rect.x, 1, 0, "Interface\\Buttons\\scrollInfoSld.bmp");
    rect.x = 40.0f;
    rect.y = 12.0f;
    rect.width = 180.0f;
    rect.height = 43.0f;
    panelGlobs.scrollTextWindow = (PanelTextWindow*)ui_text_output_record_allocate(7, (uintptr_t)s_lego_text_window_font, &rect.x, 256);
}

// FUNCTION: LEGORR 0x0045b850
void ui_overlay_create_configured_element(void)
{
    Area2F rect;

    rect.x = 21.0f;
    rect.y = 11.0f;
    rect.width = 221.0f;
    rect.height = 227.0f;
    panelGlobs.infoDockTextWindow = (PanelTextWindow*)ui_text_output_record_allocate(11, (uintptr_t)s_lego_text_window_font, &rect.x, 1024);
}

/* The C body below compiles to the same operations, but MSVC 5.0 loads the
 * radius after the second difference rather than before it (78%); no
 * expression shape or counter phase reproduces the original's order, so the
 * matching build keeps the original instructions (as advisor.c's vec_cross
 * does, finding 550). */
// FUNCTION: LEGORR 0x0045b8a0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) int point_distance_within_radius(int x1, int y1, int x2,
                                                   int y2, int radius)
{
    __asm {
        sub esp, 8
        mov eax, dword ptr [esp + 0x14]
        mov edx, dword ptr [esp + 0x0c]
        mov ecx, dword ptr [esp + 0x18]
        sub eax, edx
        mov edx, dword ptr [esp + 0x10]
        mov dword ptr [esp + 0x14], eax
        fild dword ptr [esp + 0x14]
        fild dword ptr [esp + 0x1c]
        sub ecx, edx
        mov dword ptr [esp + 0x18], ecx
        fild dword ptr [esp + 0x18]
        fld st(2)
        fmul st, st(3)
        fxch st(1)
        fst dword ptr [esp + 4]
        fmul dword ptr [esp + 4]
        faddp st(1), st
        fsqrt
        fst dword ptr [esp + 0x14]
        fxch st(1)
        fcompp
        fnstsw ax
        test ah, 0x41
        fstp st(0)
        jne point_distance_outside_radius
        mov eax, 1
        add esp, 8
        ret

    point_distance_outside_radius:
        xor eax, eax
        add esp, 8
        ret
    }
}
#else
int point_distance_within_radius(int x1, int y1, int x2, int y2, int radius)
{
    Point2F d;
    float distance;

    d.x = (float)(x2 - x1);
    d.y = (float)(y2 - y1);
    distance = (float)sqrt(d.x * d.x + d.y * d.y);
    if ((float)radius > distance) return 1;
    return 0;
}
#endif

// FUNCTION: LEGORR 0x0045b900
void integer_pair_divide_to_float_pair(int x, int y, int divisor,
                                       float* outX, float* outY)
{
    *outX = (float)x / (float)divisor;
    *outY = (float)y / (float)divisor;
}

#include "panel_maths.h"

// FUNCTION: LEGORR 0x0045b930
void float_pair_to_integer_pair_x87(int* x, int* y, int centerX,
                                    int centerY, int radius)
{
    Point2F vector;
    Point2F centre;

    centre.x = (float)centerX;
    centre.y = (float)centerY;
    vector.x = (float)*x;
    vector.y = (float)*y;
    vector.x -= centre.x;
    vector.y -= centre.y;
    panel_vector2d_set_length(&vector, (float)radius - 1.0f);
    *x = (int)(vector.x + centre.x);
    *y = (int)(vector.y + centre.y);
}

// FUNCTION: LEGORR 0x0045b9d0
void frontend_panel_rotation_control_config_load(BlockFile* config,
                                                 const char* gameName)
{
    const char* str;
    char* copy;
    char* argv[10];

    if ((str = block_get_strptr(config, string_path_join(gameName,
        // STRING: LEGORR 0x004a89d0
        "PanelRotationControl",
        // STRING: LEGORR 0x004a89e8
        "CenterPositionX", NULL))) != NULL) {
        panelGlobs.rotationCenterX = atoi(str);
    } else {
        panelGlobs.rotationCenterX = 1;
    }
    if ((str = block_get_strptr(config, string_path_join(gameName,
        "PanelRotationControl",
        // STRING: LEGORR 0x004a89c0
        "CenterPositionY", NULL))) != NULL) {
        panelGlobs.rotationCenterY = atoi(str);
    } else {
        panelGlobs.rotationCenterY = 1;
    }
    if ((str = block_get_strptr(config, string_path_join(gameName,
        "PanelRotationControl",
        // STRING: LEGORR 0x004a89b8
        "Radius", NULL))) != NULL) {
        panelGlobs.rotationRadius = atoi(str);
    } else {
        panelGlobs.rotationRadius = 1;
    }
    if ((copy = block_get_str_copy(config, string_path_join(gameName,
        "PanelRotationControl",
        // STRING: LEGORR 0x004a89ac
        "LeftImage", NULL))) != NULL) {
        sound_variant_split(copy, argv, ",");
        panelGlobs.rotationImages[2] = image_load_bmp_scaled(argv[0], 0, 0);
        image_set_pen_zero_trans(panelGlobs.rotationImages[2]);
        panelGlobs.rotationImagePositions[2].x = (float)atof(argv[1]);
        panelGlobs.rotationImagePositions[2].y = (float)atof(argv[2]);
        free(copy);
    } else {
        panelGlobs.rotationImages[2] = NULL;
    }
    if ((copy = block_get_str_copy(config, string_path_join(gameName,
        "PanelRotationControl",
        // STRING: LEGORR 0x004a89a0
        "RightImage", NULL))) != NULL) {
        sound_variant_split(copy, argv, ",");
        panelGlobs.rotationImages[3] = image_load_bmp_scaled(argv[0], 0, 0);
        image_set_pen_zero_trans(panelGlobs.rotationImages[3]);
        panelGlobs.rotationImagePositions[3].x = (float)atof(argv[1]);
        panelGlobs.rotationImagePositions[3].y = (float)atof(argv[2]);
        free(copy);
    } else {
        panelGlobs.rotationImages[3] = NULL;
    }
    if ((copy = block_get_str_copy(config, string_path_join(gameName,
        "PanelRotationControl",
        // STRING: LEGORR 0x004a8998
        "UpImage", NULL))) != NULL) {
        sound_variant_split(copy, argv, ",");
        panelGlobs.rotationImages[0] = image_load_bmp_scaled(argv[0], 0, 0);
        image_set_pen_zero_trans(panelGlobs.rotationImages[0]);
        panelGlobs.rotationImagePositions[0].x = (float)atof(argv[1]);
        panelGlobs.rotationImagePositions[0].y = (float)atof(argv[2]);
        free(copy);
    } else {
        panelGlobs.rotationImages[0] = NULL;
    }
    if ((copy = block_get_str_copy(config, string_path_join(gameName,
        "PanelRotationControl",
        // STRING: LEGORR 0x004a898c
        "DownImage", NULL))) != NULL) {
        sound_variant_split(copy, argv, ",");
        panelGlobs.rotationImages[1] = image_load_bmp_scaled(argv[0], 0, 0);
        image_set_pen_zero_trans(panelGlobs.rotationImages[1]);
        panelGlobs.rotationImagePositions[1].x = (float)atof(argv[1]);
        panelGlobs.rotationImagePositions[1].y = (float)atof(argv[2]);
        free(copy);
    } else {
        panelGlobs.rotationImages[1] = NULL;
    }
}

// FUNCTION: LEGORR 0x0045bcd0
int panel_rotation_control_drag(int mouseX, int mouseY, float elapsed)
{
    float dx;
    float dy;
    int inside;

    inside = point_distance_within_radius(mouseX, mouseY, panelGlobs.rotationCenterX,
                                          panelGlobs.rotationCenterY, panelGlobs.rotationRadius);
    if (!inside && !(panelGlobs.flags & PANEL_GLOB_FLAG_ROTATION_DRAG)) return 0;
    if (!inside) {
        float_pair_to_integer_pair_x87(&mouseX, &mouseY, panelGlobs.rotationCenterX,
                                       panelGlobs.rotationCenterY, panelGlobs.rotationRadius);
        input_cursor_client_position_set(mouseX, mouseY);
    } else if (point_distance_within_radius(mouseX, mouseY, panelGlobs.rotationCenterX,
                                            panelGlobs.rotationCenterY, 4)) {
        return 1;
    }

    panelGlobs.flags |= PANEL_GLOB_FLAG_ROTATION_DRAG;
    integer_pair_divide_to_float_pair(panelGlobs.rotationCenterX - mouseX,
                                      mouseY - panelGlobs.rotationCenterY,
                                      panelGlobs.rotationRadius, &dx, &dy);

    if (point_distance_within_radius(mouseX, panelGlobs.rotationCenterY,
                                     panelGlobs.rotationCenterX,
                                     panelGlobs.rotationCenterY, 3)) {
        camera_pan_axis_update(s_zoom_step_context, dy * elapsed * 0.04f);
        if (dy < 0.0f) panelGlobs.flags |= PANEL_GLOB_FLAG_ROTATION_UP;
        else panelGlobs.flags |= PANEL_GLOB_FLAG_ROTATION_DOWN;
        return 1;
    }

    if (point_distance_within_radius(panelGlobs.rotationCenterX, mouseY,
                                     panelGlobs.rotationCenterX,
                                     panelGlobs.rotationCenterY, 3)) {
        camera_tilt_axis_update(s_zoom_step_context, dx * elapsed * 0.04f);
        if (dx < 0.0f) panelGlobs.flags |= PANEL_GLOB_FLAG_ROTATION_RIGHT;
        else panelGlobs.flags |= PANEL_GLOB_FLAG_ROTATION_LEFT;
        return 1;
    }

    camera_tilt_axis_update(s_zoom_step_context, dx * elapsed * 0.04f);
    camera_pan_axis_update(s_zoom_step_context, dy * elapsed * 0.04f);
    if (dy < 0.0f) panelGlobs.flags |= PANEL_GLOB_FLAG_ROTATION_UP;
    else panelGlobs.flags |= PANEL_GLOB_FLAG_ROTATION_DOWN;
    if (dx < 0.0f) panelGlobs.flags |= PANEL_GLOB_FLAG_ROTATION_RIGHT;
    else panelGlobs.flags |= PANEL_GLOB_FLAG_ROTATION_LEFT;
    return 1;
}

// FUNCTION: LEGORR 0x0045bf20
void camera_zoom_step_adjust(int button, float elapsed)
{
    if (button == 0) {
        camera_zoom_update(s_zoom_step_context, elapsed * -3.0f);
    } else if (button == 1) {
        camera_zoom_update(s_zoom_step_context, elapsed * 3.0f);
    }
}

// FUNCTION: LEGORR 0x0045bf70
void ui_entry_bounds_get(int panelType, int buttonType, float* area)
{
    PanelButton* button = &panelGlobs.panelTable[panelType].buttonList[buttonType];

    area[0] = button->x1;
    area[1] = button->y1;
    area[2] = button->x2 - button->x1;
    area[3] = button->y2 - button->y1;
}

// FUNCTION: LEGORR 0x0045bfb0
void three_surface_resources_load(const char* smallCrystal,
                                  const char* usedCrystal,
                                  const char* noSmallCrystal)
{
    if ((panelGlobs.crystalSmallImage = image_load_bmp_scaled(smallCrystal, 0, 0)) != NULL) {
        image_setup_trans(panelGlobs.crystalSmallImage, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    }
    if ((panelGlobs.crystalUsedImage = image_load_bmp_scaled(usedCrystal, 0, 0)) != NULL) {
        image_setup_trans(panelGlobs.crystalUsedImage, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    }
    if ((panelGlobs.crystalNoSmallImage = image_load_bmp_scaled(noSmallCrystal, 0, 0)) != NULL) {
        image_setup_trans(panelGlobs.crystalNoSmallImage, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    }
}

// FUNCTION: LEGORR 0x0045c040
void mission_crystal_quota_setup(BlockFile* config, const char* gameName,
                                 const char* levelName)
{
    const char* str;

    if (block_get_strptr(config, string_path_join(gameName, levelName,
            // STRING: LEGORR 0x004a89f8
            "Reward",
            // STRING: LEGORR 0x004a8a00
            "Quota", "Crystals", NULL)) != NULL) {
        str = block_get_strptr(config, string_path_join(gameName, levelName, "Reward", "Quota", "Crystals", NULL));
    } else {
        str = "";
    }
    panelGlobs.crystalQuota = atoi(str);
    if (panelGlobs.crystalQuota > 26) panelGlobs.crystalQuota = 26;
    else if (panelGlobs.crystalQuota == 0) panelGlobs.crystalQuota = 26;
}

// FUNCTION: LEGORR 0x0045c0d0
void panel_crystals_draw(unsigned int crystals, unsigned int usedCrystals)
{
    unsigned int counts[2] = { 0 };
    unsigned int pairs[2][6][2] = { 0 };
    unsigned int row;
    unsigned int column;
    unsigned int k;
    unsigned int best;
    unsigned int n;
    Point2F position;
    Image* image;

    for (column = 0; column < 5; column++) {
        for (row = 0; row < 2; row++) {
            panelGlobs.crystalHistory[row][column] = panelGlobs.crystalHistory[row][column + 1];
        }
    }
    panelGlobs.crystalHistory[0][5] = crystals;
    panelGlobs.crystalHistory[1][5] = usedCrystals;

    for (row = 0; row < 2; row++) {
        for (column = 0; column < 6; column++) {
            for (k = 0; k < counts[row]; k++) {
                if (pairs[row][k][0] == panelGlobs.crystalHistory[row][column]) {
                    pairs[row][k][1]++;
                    break;
                }
            }
            if (k == counts[row]) {
                pairs[row][k][0] = panelGlobs.crystalHistory[row][column];
                pairs[row][k][1] = 1;
                counts[row]++;
            }
        }
    }

    for (row = 0; row < 2; row++) {
        best = 0;
        for (k = 0; k < counts[row]; k++) {
            if (pairs[row][k][1] > best) {
                best = pairs[row][k][1];
                if (row == 0) crystals = pairs[row][k][0];
                else usedCrystals = pairs[row][k][0];
            }
        }
    }

    panel_print_f(5, s_lego_crystal_count_font, 41, 469, 1, "%i", crystals);

    if (panelGlobs.crystalNoSmallImage != NULL) {
        position.x = 624.0f;
        position.y = 429.0f;
        n = panelGlobs.crystalQuota;
        while (n--) {
            image_display_scaled(panelGlobs.crystalNoSmallImage, NULL, &position, NULL);
            position.y -= 17.0f;
        }
    }

    if ((image = panelGlobs.crystalSmallImage) != NULL) {
        n = 0;
        if (crystals > panelGlobs.crystalQuota) crystals = panelGlobs.crystalQuota;
        if (usedCrystals > panelGlobs.crystalQuota) usedCrystals = panelGlobs.crystalQuota;
        position.x = 624.0f;
        position.y = 429.0f;
        if (panelGlobs.crystalUsedImage != NULL) {
            for (n = 0; n < usedCrystals; n++) {
                image_display_scaled(panelGlobs.crystalSmallImage, NULL, &position, NULL);
                position.y -= 17.0f;
            }
            image = panelGlobs.crystalUsedImage;
        }
        for (; n < crystals; n++) {
            image_display_scaled(image, NULL, &position, NULL);
            position.y -= 17.0f;
        }
    }
}

// FUNCTION: LEGORR 0x0045c340
void panel_resource_layout_store(const char* juiceName,
                                 unsigned int juiceX, unsigned int juiceY,
                                 unsigned int juiceLength,
                                 const char* noAirName, unsigned int noAirX,
                                 unsigned int noAirY)
{
    panelGlobs.airMeterJuiceImage = image_load_bmp_scaled(juiceName, 0, 0);
    panelGlobs.airMeterJuiceOffset.x = (float)juiceX;
    panelGlobs.airMeterJuiceOffset.y = (float)juiceY;
    panelGlobs.airMeterJuiceLength = juiceLength;
    if ((panelGlobs.airMeterNoAirImage = image_load_bmp_scaled(noAirName, 0, 0)) != NULL) {
        image_setup_trans(panelGlobs.airMeterNoAirImage, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    }
    panelGlobs.airMeterNoAirOffset.x = (float)noAirX;
    panelGlobs.airMeterNoAirOffset.y = (float)noAirY;
}

// FUNCTION: LEGORR 0x0045c3f0
void advisor_progress_bar_fill_draw(int panelType, float air)
{
    Point2F position;
    Point2F size;

    if (panelGlobs.airMeterJuiceImage != NULL && air > 0.0f) {
        size.y = (float)(int)panelGlobs.airMeterJuiceImage->height;
        if (air > 100.0f) air = 100.0f;
        size.x = air * ((float)panelGlobs.airMeterJuiceLength * 0.01f);
        position.x = panelGlobs.panelTable[panelType].xCurr + panelGlobs.airMeterJuiceOffset.x;
        position.y = panelGlobs.panelTable[panelType].yCurr + panelGlobs.airMeterJuiceOffset.y;
        image_display_scaled(panelGlobs.airMeterJuiceImage, NULL, &position, &size);
    }
}

// FUNCTION: LEGORR 0x0045c4a0
void global_value_0050080c_set(int on)
{
    panelGlobs.airMeterOxygenLow = on;
}

// FUNCTION: LEGORR 0x0045c4b0
void panel_resource_position_update(int panelType)
{
    Image* image = panelGlobs.airMeterNoAirImage;
    Point2F position;

    if (image != NULL) {
        position.x = panelGlobs.panelTable[panelType].xCurr + panelGlobs.airMeterNoAirOffset.x;
        position.y = panelGlobs.panelTable[panelType].yCurr + panelGlobs.airMeterNoAirOffset.y;
        if (panelGlobs.airMeterOxygenLow && sim_status_banner_word_get()) {
            image_display_scaled(image, NULL, &position, NULL);
        }
    }
}

// FUNCTION: LEGORR 0x0045c520
void panel_resource_handle_and_dimensions_store(const char* name,
                                                unsigned int x,
                                                unsigned int y,
                                                unsigned int length)
{
    panelGlobs.sideBarMeterImage = image_load_bmp_scaled(name, 0, 0);
    panelGlobs.sideBarMeterOffset.x = (float)x;
    panelGlobs.sideBarMeterOffset.y = (float)y;
    panelGlobs.sideBarMeterLength = length;
}

// FUNCTION: LEGORR 0x0045c580
void ui_overlay_animation_advance(int up, unsigned int count)
{
    if (up) {
        for (; count > 0; count--) {
            panelGlobs.sideBarMeterLevel = panelGlobs.sideBarMeterLevel - (1.0f - panelGlobs.sideBarMeterLevel) * -0.05f;
        }
    } else {
        for (; count > 0; count--) {
            panelGlobs.sideBarMeterLevel = 0.05263158f * ((panelGlobs.sideBarMeterLevel - 0.05f) * 20.0f);
        }
    }
    if (panelGlobs.sideBarMeterLevel < 0.0f) panelGlobs.sideBarMeterLevel = 0.0f;
}

// FUNCTION: LEGORR 0x0045c5f0
void panel_side_bar_draw(void)
{
    Point2F size;
    Point2F position;

    if (panelGlobs.sideBarMeterImage != NULL && panelGlobs.sideBarMeterLevel > 0.0f) {
        size.x = (float)(int)panelGlobs.sideBarMeterImage->width;
        size.y = (float)panelGlobs.sideBarMeterLength * panelGlobs.sideBarMeterLevel;
        position.x = panelGlobs.sideBarMeterOffset.x;
        position.y = panelGlobs.sideBarMeterOffset.y - size.y;
        image_display_scaled(panelGlobs.sideBarMeterImage, NULL, &position, &size);
    }
}

// FUNCTION: LEGORR 0x0045c670
int panel_advisor_animation_play(int panelType, int button, int force)
{
    int advisor;

    if (advisor_animation_slot_id_resolve(panelType, button, &advisor)) {
        advisor_request_admit(advisor, force);
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0045c6b0
int advisor_animation_slot_id_resolve(int panelType, int button, int* advisor)
{
    *advisor = 21;
    switch (panelType) {
    case 0:
        if (button == 0) *advisor = 6;
        else if (button == 1) *advisor = 7;
        else if (button == 2) *advisor = 8;
        else if (button == 3) *advisor = 9;
        else if (button == 4) *advisor = 10;
        break;
    case 10:
        if (button == 0) *advisor = 11;
        else if (button == 1) *advisor = 12;
        break;
    case 6:
        if (button == 0) *advisor = 14;
        else if (button == 1) *advisor = 15;
        else if (button == 2) *advisor = 16;
        break;
    case 9:
        if (button == 0) *advisor = 18;
        else if (button == 1) *advisor = 19;
        break;
    }
    return *advisor != 21;
}

