// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-020 Interface.c's records and state (interfaceGlobs), shared with
 * the units that read them (Lego.c).
 * The pointer fields hold real pointers (ADR 0007 decision 4). */
#ifndef LEGORRR_GAME_INTERFACE_RECORD_H
#define LEGORRR_GAME_INTERFACE_RECORD_H

#include <stdint.h>

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

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_INTERFACE_RECORD_H */
