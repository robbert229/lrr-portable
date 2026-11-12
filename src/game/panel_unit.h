// SPDX-License-Identifier: LGPL-3.0-or-later
/* Panel.c (original translation unit GAME-032, finding 505): the in-game
 * interface panels. Twelve panels (radar, messages, crystal side bar, top
 * panel, information, priority list, camera control, info dock,
 * encyclopedia, ...) slide between an "out" and an "in" position. Each owns
 * a list of buttons with normal, highlight and pressed images. The unit also
 * holds the panel text windows, the camera-control rotation widget, the
 * crystal side bar and the air meter.
 *
 * C-compatible header (ADR 0005). panel_unit.c is compiled as C in the
 * matching build and as C++ in the native port. Shared C++ headers do not
 * include this file: every include and declaration a header adds shifts
 * MSVC's symbol numbering (finding 506). src/game/test_c_boundaries.cpp
 * checks it against the C++ declarations of the same functions. */
#ifndef LEGORRR_GAME_PANEL_UNIT_H
#define LEGORRR_GAME_PANEL_UNIT_H

#include <stdint.h>

#include "gods98/config.h"
#include "gods98/images.h"

#ifdef __cplusplus
extern "C" {
#endif

struct TextureSurface;

#define PANEL_TYPE_COUNT 12
#define PANEL_MAX_BUTTONS 24

/* The panels the unit addresses by number. */
#define PANEL_RADAR 0
#define PANEL_TOP 6
#define PANEL_PRIORITY_LIST 8
#define PANEL_CAMERA_CONTROL 9
#define PANEL_INFO_DOCK 10
#define PANEL_ENCYCLOPEDIA 11

/* PanelData flags (+0x2c). */
#define PANEL_FLAG_OPEN    0x1  /* an image or flic is loaded */
#define PANEL_FLAG_OUT     0x2  /* moving to (or at) the "out" position */
#define PANEL_FLAG_IN      0x4  /* moving to (or at) the "in" position */
#define PANEL_FLAG_MOVING  0x8

/* PanelButton flags (+0x28). */
#define PANEL_BUTTON_FLAG_HIGHLIGHT 0x02
#define PANEL_BUTTON_FLAG_PRESSED   0x04
#define PANEL_BUTTON_FLAG_TOGGLED   0x08
#define PANEL_BUTTON_FLAG_HIDDEN    0x10
#define PANEL_BUTTON_FLAG_TOGGLE    0x20
#define PANEL_BUTTON_FLAG_BANNER    0x40

/* One panel button, 0x2c bytes on Win32. The area is stored as two
 * corners. */
typedef struct PanelButton {
    float x1, y1;                 /* +0x00 */
    float x2, y2;                 /* +0x08 */
    Image* highlightImage;        /* +0x10 */
    Image* pressedImage;          /* +0x14 */
    Image* normalImage;           /* +0x18 */
    unsigned int reserved1c;      /* +0x1c */
    int toolTip;                  /* +0x20 */
    int clickCount;               /* +0x24 */
    unsigned int flags;           /* +0x28 */
} PanelButton;

/* One panel, 0x30 bytes on Win32. */
typedef struct PanelData {
    void* image;                  /* +0x00: Image*, or the Flic* when isFlic */
    int isFlic;                   /* +0x04 */
    float xOut, yOut;             /* +0x08 */
    float xIn, yIn;               /* +0x10 */
    float xCurr, yCurr;           /* +0x18 */
    int reserved20;               /* +0x20 */
    PanelButton* buttonList;      /* +0x24 */
    unsigned int buttonCount;     /* +0x28 */
    unsigned int flags;           /* +0x2c */
} PanelData;

/* A text window placed on a panel, 0x10 bytes on Win32. */
typedef struct PanelTextWindow {
    void* textWindow;             /* +0x00: TextWindow (gods98/heap.hpp) */
    int x, y;                     /* +0x04: the offset on the panel */
    PanelData* panel;             /* +0x0c */
} PanelTextWindow;

/* Panel.c's state, 0x7b8 bytes at 005000e0 on Win32. */
typedef struct PanelGlobs {
    const char* panelName[PANEL_TYPE_COUNT];                    /* 005000e0 */
    PanelData panelTable[PANEL_TYPE_COUNT];                     /* 00500110 */
    const char* buttonName[PANEL_TYPE_COUNT][PANEL_MAX_BUTTONS];/* 00500350 */
    int selectedPanel;                                          /* 005007d0 */
    int selectedButton;                                         /* 005007d4 */
    PanelTextWindow* scrollTextWindow;                          /* 005007d8 */
    PanelTextWindow* infoDockTextWindow;                        /* 005007dc */
    Image* crystalSmallImage;                                   /* 005007e0 */
    Image* crystalNoSmallImage;                                 /* 005007e4 */
    Image* crystalUsedImage;                                    /* 005007e8 */
    unsigned int crystalQuota;                                  /* 005007ec */
    Image* airMeterJuiceImage;                                  /* 005007f0 */
    Image* airMeterNoAirImage;                                  /* 005007f4 */
    Point2F airMeterJuiceOffset;                                /* 005007f8 */
    unsigned int airMeterJuiceLength;                           /* 00500800 */
    Point2F airMeterNoAirOffset;                                /* 00500804 */
    int airMeterOxygenLow;                                      /* 0050080c */
    Image* sideBarMeterImage;                                   /* 00500810 */
    Point2F sideBarMeterOffset;                                 /* 00500814 */
    unsigned int sideBarMeterLength;                            /* 0050081c */
    float sideBarMeterLevel;                                    /* 00500820 */
    int rotationCenterX;                                        /* 00500824 */
    int rotationCenterY;                                        /* 00500828 */
    int rotationRadius;                                         /* 0050082c */
    Point2F rotationImagePositions[4];                          /* 00500830 */
    Image* rotationImages[4];                                   /* 00500850 */
    unsigned int flags;                                         /* 00500860 */
    unsigned int reserved864;                                   /* 00500864 */
    unsigned int crystalHistory[2][6];                          /* 00500868 */
} PanelGlobs;

/* PanelGlobs.flags (00500860). */
#define PANEL_GLOB_FLAG_ROTATION_DRAG 0x1
#define PANEL_GLOB_FLAG_ROTATION_UP    0x10
#define PANEL_GLOB_FLAG_ROTATION_DOWN  0x20
#define PANEL_GLOB_FLAG_ROTATION_LEFT  0x40
#define PANEL_GLOB_FLAG_ROTATION_RIGHT 0x80

extern PanelGlobs panelGlobs;

/* FUN_0045a030 Panel_Initialise: the panel and button name tables. */
void ui_panel_name_tables_init(void);
/* FUN_0045a240: the information panel's scroll controls and text windows. */
void legacy_scroll_panel_controls_initialize(void);
/* FUN_0045a270: every panel back to its "in" position, button state cleared. */
void legacy_panel_category_interaction_state_reset(void);
/* FUN_0045a2e0 Panel_LoadImage: a bitmap, or failing that a flic. */
void legacy_overlay_image_record_load(const char* fileName, int panel,
                                      unsigned int flags);
/* FUN_0045a370 Panel_GetPanelType. */
int ui_panel_name_table_lookup(const char* name, int* panel);
/* FUN_0045a3b0: whether the panel's image is transparent at the point. */
int legacy_panel_slot_coordinate_hit_test(int panel, int x, int y);
/* FUN_0045a410 Panel_GetButtonType. */
int ui_panel_indexed_name_lookup(int panel, const char* name,
                                 unsigned int* button);
/* FUN_0045a460 Panel_PrintF. */
void panel_print_f(int panel, const struct TextureSurface* font, int x,
                   int y, int center, const char* msg, ...);
/* FUN_0045a500 Panel_TextWindow_Create. */
void* ui_text_output_record_allocate(int panel, uintptr_t font,
                                     const float* rect, unsigned int size);
/* FUN_0045a590 Panel_TextWindow_Print. */
void panel_text_window_print_f(PanelTextWindow* window, const char* msg,
                               ...);
/* FUN_0045a5b0 Panel_TextWindow_Update. */
void ui_text_output_position_and_draw(void* window, unsigned int posFromEnd,
                                      float elapsed);
/* FUN_0045a620 Panel_TextWindow_Clear. */
void ui_text_output_object_release_target(void* const* window);
/* FUN_0045a630 Panel_TextWindow_GetInfo. */
void ui_record_callback_forward(const void* const* window,
                                uint32_t* outA, uint32_t* outB);
/* FUN_0045a650 Panel_SetArea. */
void panel_category_bounds_set(int panel, int xOut, int yOut, int xIn,
                               int yIn);
/* FUN_0045a6e0 Panel_SetCurrentPosition. */
void indexed_pair_set(int panel, float x, float y);
/* FUN_0045a700 Panel_GetPosition. */
void indexed_pair_get(int panel, float* x, float* y);
/* FUN_0045a730 Panel_Update: move and draw one panel. */
void simtick_manager_tick(int panel, float elapsed);
/* FUN_0045a9c0 Panel_Button_DisplayAll. */
void panel_texture_slots_blit_to(int panel);
/* FUN_0045aac0 Panel_Button_SetHidden. */
void panel_item_flag16_set(int panel, int button, int on);
/* FUN_0045ab00 Panel_Toggle. */
void panel_state_flags_toggle(int panel);
/* FUN_0045ab30 Panel_IsIn. */
int request_record_ready_flag4_target(int panel);
/* FUN_0045ab60 Panel_IsOut. */
int request_record_ready_flag2_target(int panel);
/* FUN_0045ab90 Panel_IsMoving. */
int request_record_flag8_is_set_target(int panel);
/* FUN_0045abb0 Panel_Button_SetToggle. */
void panel_item_flag32_set(int panel, int button, int on);
/* FUN_0045abf0 Panel_Button_SetToggled. */
void panel_item_flag8_set_if_enabled(int panel, int button, int on);
/* FUN_0045ac30 Panel_LoadButtons. `buttonTypes` gives each row's button. */
void panel_texture_slots_allocate(int panel, unsigned int count,
                                  const unsigned int* buttonTypes,
                                  const float* areas,
                                  char** normalNames,
                                  char** highlightNames,
                                  char** pressedNames,
                                  const int* toolTips);
/* FUN_0045adb0 Panel_CheckCollision: the mouse over the panels. */
int panel_cursor_interaction_update(float elapsed, int mouseX, int mouseY,
                                    int leftButton, int leftButtonLast,
                                    int* overPanel);
/* FUN_0045b310 Panel_CheckButtonCollision. */
int panel_button_collision_check(int* outPanel, int* outButton, int mouseX,
                                 int mouseY, int leftButton,
                                 int leftButtonLast, int* outClicked,
                                 int* overPanel);
/* FUN_0045b590: whether the point is inside the button. */
int panel_button_point_inside(const PanelData* panel,
                              const PanelButton* button, int x, int y);
/* FUN_0045b610 Panel_RotationControl_ClearDrag. */
void simtick_global_ui_flags_clear_bits12(void);
/* FUN_0045b620 Panel_PriorityList_Button. */
void ui_list_priority_button_action_dispatch(int button);
/* FUN_0045b740 Panel_InfoPanel_CreateScroll. */
void ui_scroll_info_panel_initialize(void);
/* FUN_0045b850 Panel_InfoDock_CreateTextWindow. */
void ui_overlay_create_configured_element(void);
/* FUN_0045b8a0: whether (x2, y2) is closer to (x1, y1) than `radius`. */
int point_distance_within_radius(int x1, int y1, int x2, int y2,
                                 int radius);
/* FUN_0045b900: (x, y) / divisor as floats. */
void integer_pair_divide_to_float_pair(int x, int y, int divisor,
                                       float* outX, float* outY);
/* FUN_0045b930: the point on the circle nearest (x, y). */
void float_pair_to_integer_pair_x87(int* x, int* y, int centerX,
                                    int centerY, int radius);
/* FUN_0045b9d0 Panel_RotationControl_Initialise. */
void frontend_panel_rotation_control_config_load(BlockFile* config,
                                                 const char* gameName);
/* FUN_0045bcd0 Panel_RotationControl_HandleInput. */
int panel_rotation_control_drag(int mouseX, int mouseY, float elapsed);
/* FUN_0045bf20 Panel_CameraZoom. */
void camera_zoom_step_adjust(int button, float elapsed);
/* FUN_0045bf70 Panel_Button_GetArea. */
void ui_entry_bounds_get(int panel, int button, float* area);
/* FUN_0045bfb0 Panel_Crystals_Initialise. */
void three_surface_resources_load(const char* smallCrystal,
                                  const char* usedCrystal,
                                  const char* noSmallCrystal);
/* FUN_0045c040 Panel_Crystals_LoadRewardQuota. */
void mission_crystal_quota_setup(BlockFile* config, const char* gameName,
                                 const char* levelName);
/* FUN_0045c0d0 Panel_Crystals_Draw. */
void panel_crystals_draw(unsigned int crystals, unsigned int usedCrystals);
/* FUN_0045c340 Panel_AirMeter_Initialise. */
void panel_resource_layout_store(const char* juiceName,
                                 unsigned int juiceX, unsigned int juiceY,
                                 unsigned int juiceLength,
                                 const char* noAirName, unsigned int noAirX,
                                 unsigned int noAirY);
/* FUN_0045c3f0 Panel_AirMeter_DrawJuice. */
void advisor_progress_bar_fill_draw(int panel, float air);
/* FUN_0045c4a0 Panel_AirMeter_SetOxygenLow. */
void global_value_0050080c_set(int on);
/* FUN_0045c4b0 Panel_AirMeter_DrawOxygenLow. */
void panel_resource_position_update(int panel);
/* FUN_0045c520 Panel_CryOreSideBar_Initialise. */
void panel_resource_handle_and_dimensions_store(const char* name,
                                                unsigned int x,
                                                unsigned int y,
                                                unsigned int length);
/* FUN_0045c580 Panel_CryOreSideBar_ChangeOreMeter. */
void ui_overlay_animation_advance(int up, unsigned int count);
/* FUN_0045c5f0 Panel_CryOreSideBar_Draw. */
void panel_side_bar_draw(void);
/* FUN_0045c670 Panel_PlayAdvisorAnim. */
int panel_advisor_animation_play(int panel, int button, int force);
/* FUN_0045c6b0 Panel_GetAdvisorType. */
int advisor_animation_slot_id_resolve(int panel, int button, int* advisor);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_PANEL_UNIT_H */
