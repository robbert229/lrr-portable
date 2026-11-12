// SPDX-License-Identifier: LGPL-3.0-or-later
/* Interface.c (original translation unit GAME-020, finding 505): the in-game
 * icon menus on the right of the screen. Each of the 24 menus lists up to
 * eleven of the 74 menu items (dig, build, teleport, train, ...) or, for the
 * three object menus, one icon per vehicle or building type. A menu slides
 * in from the right edge, draws a surround image sized by its item count and
 * a back button, and shows the tool tip, advisor hint and dependency icons of
 * the item under the cursor.
 *
 * C-compatible header (ADR 0005). interface_unit.c is compiled as C in the
 * matching build and as C++ in the native port. Shared C++ headers do not
 * include this file: every include and declaration a header adds shifts
 * MSVC's symbol numbering (finding 506). src/game/test_c_boundaries.cpp
 * checks it against the C++ declarations of the same functions.
 *
 * The port's interface keeps its own state and functions in the port tail
 * game/interface_port.cpp (finding 564). The unit's functions are named
 * after what they do; the comments give the original's Interface_* name
 * where it is known. */
#ifndef LEGORRR_GAME_INTERFACE_UNIT_H
#define LEGORRR_GAME_INTERFACE_UNIT_H

#include <stdint.h>

#include "gods98/config.h"
#include "gods98/images.h"

#ifdef __cplusplus
extern "C" {
#endif

struct LegoObject;

#define INTERFACE_MENUITEM_COUNT 74   /* 0x4a */
#define INTERFACE_MENU_COUNT 24       /* 0x18 */
#define INTERFACE_SURROUND_COUNT 11

/* The item that stands for one icon of an object menu (vehicles,
 * buildings). */
#define INTERFACE_MENUITEM_OBJECT 0x49

/* Menus whose items are object types rather than menu items. */
#define INTERFACE_MENU_BUILDINGS 0x13
#define INTERFACE_MENU_SMALL_VEHICLES 0x14
#define INTERFACE_MENU_LARGE_VEHICLES 0x15

/* interfaceGlobs.flags */
#define INTERFACE_FLAG_OPEN          0x4
#define INTERFACE_FLAG_ADVISOR       0x20
#define INTERFACE_FLAG_ADVISOR_LOOP  0x40
#define INTERFACE_FLAG_IN            0x80
#define INTERFACE_FLAG_OUT           0x100
#define INTERFACE_FLAG_HOVER_ITEM    0x200
#define INTERFACE_FLAG_HOVER_OBJECT  0x400
#define INTERFACE_FLAG_DEPENDENCIES  0x800
#define INTERFACE_FLAG_HOVER_DEPENDENCY 0x1000
#define INTERFACE_FLAG_HOVER_BACK    0x4000

/* Menu item flags (menuItemFlags, and the object items' flag arrays). */
#define INTERFACE_ITEM_DISABLED      0x1
#define INTERFACE_ITEM_FORCE_ENABLED 0x2
#define INTERFACE_ITEM_FLASH         0x4
#define INTERFACE_ITEM_PRESSED       0x8

typedef struct Point2I {
    int x;
    int y;
} Point2I;

typedef struct InterfaceMenu {
    int* items;            /* +0 */
    unsigned int count;    /* +4 */
} InterfaceMenu;

/* Interface.c's .bss from 004dcd58, 0x14ac bytes. Lego.c (menu type,
 * flashing state) and Info.c (font) read it, so it is external. */
typedef struct InterfaceGlobs {
    struct TextureSurface* font;                                 /* +0x0000 004dcd58 */
    Image* surroundImages[INTERFACE_SURROUND_COUNT];             /* +0x0004 004dcd5c by item count - 1 */
    Point2F surroundOffsets[INTERFACE_SURROUND_COUNT];           /* +0x0030 004dcd88 */
    Image* objectSurroundImages[INTERFACE_SURROUND_COUNT];       /* +0x0088 004dcde0 */
    Point2F objectSurroundOffsets[INTERFACE_SURROUND_COUNT];     /* +0x00b4 004dce0c */
    Image* backButtonImageHighlight;                             /* +0x010c 004dce64 */
    Image* backButtonImagePressed;                               /* +0x0110 004dce68 */
    Point2F backButtonOffsets[INTERFACE_SURROUND_COUNT];         /* +0x0114 004dce6c */
    unsigned int backButtonWidth;                                      /* +0x016c 004dcec4 */
    unsigned int backButtonHeight;                                      /* +0x0170 004dcec8 */
    char* backButtonText;                                        /* +0x0174 004dcecc */
    const char* menuItemName[INTERFACE_MENUITEM_COUNT];          /* +0x0178 004dced0 */
    Image* menuItemIcons[INTERFACE_MENUITEM_COUNT];              /* +0x02a0 004dcff8 */
    Image* menuItemIconsDisabled[INTERFACE_MENUITEM_COUNT];      /* +0x03c8 004dd120 */
    Image* menuItemIconsPressed[INTERFACE_MENUITEM_COUNT];       /* +0x04f0 004dd248 */
    char* menuItemTooltips[INTERFACE_MENUITEM_COUNT];            /* +0x0618 004dd370 */
    int menuItemSFX[INTERFACE_MENUITEM_COUNT];                   /* +0x0740 004dd498 */
    unsigned char menuItemF2keys[INTERFACE_MENUITEM_COUNT];      /* +0x0868 004dd5c0 */
    unsigned int menuItemFlags[INTERFACE_MENUITEM_COUNT];        /* +0x08b4 004dd60c */
    Image** vehicleItemIcons;                                    /* +0x09dc 004dd734 */
    Image** buildingItemIcons;                                   /* +0x09e0 004dd738 */
    Image** vehicleItemIconsDisabled;                            /* +0x09e4 004dd73c */
    Image** buildingItemIconsDisabled;                           /* +0x09e8 004dd740 */
    Image** vehicleItemIconsPressed;                             /* +0x09ec 004dd744 */
    Image** buildingItemIconsPressed;                            /* +0x09f0 004dd748 */
    char* menuItemTooltipsDisabled[INTERFACE_MENUITEM_COUNT];    /* +0x09f4 004dd74c */
    int menuItemSFXDisabled[INTERFACE_MENUITEM_COUNT];           /* +0x0b1c 004dd874 */
    unsigned char* vehicleItemF2keys;                            /* +0x0c44 004dd99c */
    unsigned char* buildingItemF2keys;                           /* +0x0c48 004dd9a0 */
    unsigned int* vehicleItemFlags;                              /* +0x0c4c 004dd9a4 */
    unsigned int* buildingItemFlags;                             /* +0x0c50 004dd9a8 */
    unsigned int menuItemClicks[INTERFACE_MENUITEM_COUNT];       /* +0x0c54 004dd9ac */
    unsigned int* vehicleItemClicks;                             /* +0x0d7c 004ddad4 */
    unsigned int* buildingItemClicks;                            /* +0x0d80 004ddad8 */
    int menuItemTooltipsUseName[INTERFACE_MENUITEM_COUNT];       /* +0x0d84 004ddadc */
    InterfaceMenu menus[INTERFACE_MENU_COUNT];                   /* +0x0eac 004ddc04 */
    Point2I selBlockPos;                                         /* +0x0f6c 004ddcc4 */
    int currMenuType;                                            /* +0x0f74 004ddccc */
    Point2F currMenuPosition;                                    /* +0x0f78 004ddcd0 */
    int dependencyObjType;                                       /* +0x0f80 004ddcd8 */
    int dependencyObjIndex;                                      /* +0x0f84 004ddcdc */
    Point2F slideOutPosition;                                    /* +0x0f88 004ddce0 */
    Point2F slideInPosition;                                     /* +0x0f90 004ddce8 */
    float slideSpeed;                                            /* +0x0f98 004ddcf0 */
    Point2I nextBlockPos;                                        /* +0x0f9c 004ddcf4 */
    int nextMenuType;                                            /* +0x0fa4 004ddcfc */
    int pressedItemType;                                         /* +0x0fa8 004ddd00 */
    int pressedObjType;                                          /* +0x0fac 004ddd04 */
    int pressedObjIndex;                                         /* +0x0fb0 004ddd08 */
    Area2F dependencyArea;                                       /* +0x0fb4 004ddd0c */
    int flashingState;                                           /* +0x0fc4 004ddd1c */
    float flashingTimer;                                         /* +0x0fc8 004ddd20 */
    int advisorType;                                             /* +0x0fcc 004ddd24 */
    int advisorItemType;                                         /* +0x0fd0 004ddd28 */
    int advisorObjType;                                          /* +0x0fd4 004ddd2c */
    int advisorObjIndex;                                         /* +0x0fd8 004ddd30 */
    int blockPermissions[20][15];                                /* +0x0fdc 004ddd34 */
    Image* dependencyPlusImage;                                  /* +0x148c 004de1e4 */
    Image* dependencyEqualsImage;                                /* +0x1490 004de1e8 */
    float dependencyTimer;                                       /* +0x1494 004de1ec */
    int blockHighlight;                                          /* +0x1498 004de1f0 */
    int hoverSFX;                                                /* +0x149c 004de1f4 */
    unsigned int flags;                                          /* +0x14a0 004de1f8 */
    int lastHoverSFX;                                            /* +0x14a4 004de1fc */
    int hoverSFXHandle;                                          /* +0x14a8 004de200 */
} InterfaceGlobs;

extern InterfaceGlobs interfaceGlobs;

/* The point that interface_get_item_at_point hands to its menu-item
 * callback, and what the callback finds. */
typedef struct InterfaceItemSearch {
    Point2I point;
    Point2F pos;
    int type;
    int objType;
    int objIndex;
} InterfaceItemSearch;

typedef int (*InterfaceMenuItemCallback)(int type, int objType, int objIndex,
                                         void* data);
typedef int (*InterfaceObjectCallback)(struct LegoObject* obj, void* data);

/* 0041a300 Interface_Initialise */
void interface_initialise(unsigned int x, unsigned int y,
                          struct TextureSurface* font);
/* 0041a660 */
void interface_init_menus(void);
/* 0041a7d0: the items of one menu, as `count` int arguments. */
void interface_set_menu_items(int menu, unsigned int count, ...);
/* 0041a850 Interface_Shutdown */
void interface_shutdown(void);
/* 0041a920 */
void interface_reset(void);
/* 0041a990 */
void interface_reset_menu(void);
/* 0041a9c0 Interface_GetMenuItemType */
int interface_get_menu_item_type(const char* name, int* type);
/* 0041aa00 */
void interface_init_object_items(void);
/* 0041ab00 */
void interface_load_object_items(BlockFile* config, const char* gameName);
/* 0041ada0 Interface_LoadMenuItems */
void interface_load_menu_items(BlockFile* config, const char* gameName);
/* 0041afd0 Interface_LoadItemPanels */
void interface_load_item_panels(BlockFile* config, const char* gameName);
/* 0041b1b0 Interface_LoadBackButton */
void interface_load_back_button(BlockFile* config, const char* gameName);
/* 0041b270 Interface_LoadPlusMinusImages */
void interface_load_dependency_images(const char* plusName,
                                      const char* equalsName);
/* 0041b2d0 Interface_ChangeMenu. The entries that keep a port name keep the
 * port's spelling of their arguments; their callers outside the unit use
 * it (finding 564). */
void command_context_post_to_hud(int menu, const uint32_t blockPos[2]);
/* 0041b300 */
void interface_set_block_highlight(int menu, const Point2I* blockPos);
/* 0041b3c0 */
unsigned int interface_get_menu_item_count(int menu);
/* 0041b470 */
int interface_menu_has_back_button(int menu);
/* 0041b490 */
void interface_draw(void);
/* 0041b680 */
int interface_enumerate_menu_items(int menu,
                                   InterfaceMenuItemCallback callback,
                                   void* data);
/* 0041b930 Interface_Update */
void interface_update(float elapsed);
/* 0041b9b0 Interface_GetFlashingState */
uint32_t sim_status_banner_word_get(void);
/* 0041b9c0 */
int interface_resolve_teleporter_item(int* type);
/* 0041ba10 */
void interface_update_hover_sfx(float elapsed);
/* 0041ba90: the sound of the item under the cursor. */
void simulation_global_word_set(uint32_t sfx);
/* 0041baa0 Interface_DoMouse */
int interface_do_mouse(int x, int y, int leftButton, int unused,
                       int leftReleased);
/* 0041c1c0 */
int interface_get_item_at_point(unsigned int x, unsigned int y, int* type,
                                int* objType, int* objIndex);
/* 0041c440: the F2 hot keys of the open menu's items. */
void advisor_callback_schedule_gate(void);
/* 0041c4f0 */
void interface_update_advisor(int type, int objType, int objIndex,
                              const Point2F* position);
/* 0041c6e0 */
int interface_handle_item_press(int type, int objType, int objIndex,
                                int leftButton, int leftReleased);
/* 0041c7b0 */
void interface_update_item_enabled(int type);
/* 0041c800 */
void interface_update_object_item_enabled(int objType, int objIndex);
/* 0041c8f0 */
int interface_is_item_disabled(int type, int checkFlash);
/* 0041c950 */
int interface_is_object_item_disabled(int objType, int objIndex,
                                      int checkFlash);
/* 0041c9f0 */
void interface_set_item_pressed(int type, int on);
/* 0041ca20 */
void interface_set_object_item_pressed(int objType, int objIndex, int on);
/* 0041ca60 */
unsigned int interface_is_item_pressed(int type);
/* 0041ca70 */
unsigned int interface_is_object_item_pressed(int objType, int objIndex);
/* 0041cab0 */
Image* interface_get_item_image(int type);
/* 0041caf0 */
Image* interface_get_object_item_image(int objType, int objIndex);
/* 0041cb90: an object type's dependency icon. */
void* input_binding_default_value_get(int objType, int objIndex,
                                      int available);
/* 0041cc80 */
void interface_set_dependency_item(int type, int objType, int objIndex);
/* 0041cce0 */
void interface_set_dependency_area(const Point2F* position,
                                   unsigned int width, unsigned int height);
/* 0041cd30: the box around the item under the cursor (an Area2F). */
void position_tracker_box_draw(const float bounds[4]);
/* 0041cea0 */
void interface_object_menu_check(struct LegoObject* obj);
/* 0041cf20 */
void interface_driven_object_menu_check(struct LegoObject* obj);
/* 0041cf80, 0041cfb0, 0041cfe0: back to the main menu when the menu of
 * the given block (a Point2I) is open. */
void selection_mode7_cancel_at_point(const uint32_t coordinate[2]);
void cancel_action_for_matching_coordinates(const uint32_t coordinate[2]);
void selection_mode5_cancel_at_point(const uint32_t coordinate[2]);
/* 0041d010 */
void interface_back_to_main_if_selected(struct LegoObject* obj);
/* 0041d040 */
int interface_is_item_available(int type);
/* 0041dca0 Interface_DoAction */
int interface_do_action(int type);
/* 0041e750 Interface_BackToMain */
void selection_clear_request_and_hud_reset(void);
/* 0041e770 */
void interface_set_item_flash(int type, int on);
/* 0041e7a0 */
void interface_set_object_item_flash(int objType, int objIndex, int on);
/* 0041e7e0 */
struct LegoObject* interface_get_primary_selected(void);
/* 0041e7f0 */
unsigned int interface_set_item_clicks(int type, unsigned int clicks);
/* 0041e810 */
unsigned int interface_get_item_clicks(int type);
/* 0041e820 */
unsigned int interface_set_object_item_clicks(int objType, int objIndex,
                                              unsigned int clicks);
/* 0041e860 */
unsigned int interface_get_object_item_clicks(int objType, int objIndex);
/* 0041e890 */
int interface_set_advisor_item(int type, int loop);
/* 0041e8d0 */
int interface_get_advisor_type(int type);
/* 0041e990 */
int cell_action_permission_get(int x, int y);
/* 0041e9d0 */
void interface_set_slide_positions(unsigned int inX, unsigned int inY,
                                   unsigned int outX, unsigned int outY,
                                   float speed);
/* 0041ea50 */
void interface_set_menu(int menu, const Point2I* blockPos);
/* 0041eac0 */
void interface_update_slide(float elapsed);
/* 0041ec30 */
void interface_draw_object_count(int objType, int objIndex,
                                 const Point2F* position);
/* 0041eca0 */
void interface_draw_dependencies(float x, float y);
/* 0041ee60 */
void interface_reset_dependency_timer(void);
/* 0041ee80 */
int interface_get_dependency_at_point(int x, int y, float areaX, float areaY,
                                      int* type, int* objType, int* objIndex);
/* 0041f100 */
int interface_object_type_present(int objType, int objIndex);
/* 0041f230 */
int interface_any_object_of_type2(void);
/* 0041f270 */
int interface_any_object_with_flags3(unsigned int mask);
/* 0041f2b0 */
int interface_any_object_with_flags2_100000(void);
/* 0041f2f0 */
int interface_any_object_of_type(int objType, int objIndex);
/* 0041f340 */
int interface_any_object_with_ability(unsigned int mask);
/* 0041f3c0 */
int interface_any_object_can_reach(const Point2I* blockPos);
/* 0041f400 */
int interface_any_object_can_reach_alt(const Point2I* blockPos);
/* 0041f420 */
int interface_enumerate_selected(InterfaceObjectCallback callback, void* data);
/* The callbacks (menu items: 0041b800, 0041c310, 0041c470; objects:
 * 0041f190 onwards). */
int interface_callback_draw_item(int type, int objType, int objIndex,
                                 void* data);
int interface_callback_hit_item(int type, int objType, int objIndex,
                                void* data);
int interface_callback_hotkey(int type, int objType, int objIndex,
                              void* data);
int interface_callback_object_search(struct LegoObject* obj, void* data);
int interface_callback_can_reach(struct LegoObject* obj, void* data);
int interface_callback_can_enter(struct LegoObject* obj, void* data);
int interface_callback_can_unload(struct LegoObject* obj, void* data);
int interface_callback_hungry(struct LegoObject* obj, void* data);
int interface_callback_damaged(struct LegoObject* obj, void* data);
int interface_callback_can_pick_up(struct LegoObject* obj, void* data);
int interface_callback_delete(struct LegoObject* obj, void* data);
int interface_callback_go_feed(struct LegoObject* obj, void* data);
int interface_callback_repair(struct LegoObject* obj, void* data);
int interface_callback_lacks_tool(struct LegoObject* obj, void* data);
int interface_callback_get_tool(struct LegoObject* obj, void* data);
int interface_callback_detach(struct LegoObject* obj, void* data);
int interface_callback_has_tool(struct LegoObject* obj, void* data);
/* 0041f720 */
int interface_primary_has_ability(unsigned int mask);
/* 0041f740 */
int interface_can_lay_path(const Point2I* blockPos);
/* 0041f840 */
int interface_callback_remove_spider(struct LegoObject* obj, void* data);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_INTERFACE_UNIT_H */
