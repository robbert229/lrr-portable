// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-032 Panel.c's records and state (panelGlobs), shared with the units
 * that read them (Lego.c).
 * The pointer fields hold real pointers (ADR 0007 decision 4). */
#ifndef LEGORRR_GAME_PANEL_RECORD_H
#define LEGORRR_GAME_PANEL_RECORD_H

#include <stdint.h>

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
    Point2F outPos;               /* +0x08 */
    Point2F inPos;                /* +0x10 */
    Point2F currPos;              /* +0x18 */
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

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_PANEL_RECORD_H */
