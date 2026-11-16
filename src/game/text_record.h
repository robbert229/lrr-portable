// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-045 Text.c's state (textGlobs, 00503190; finding 607), shared with
 * the units that write its message counter, line and timer (Lego.c,
 * NERPs). The +0x... comments are the Win32 offsets; the pointer fields
 * hold real pointers (ADR 0007 decision 4). */
#ifndef LEGORRR_GAME_TEXT_RECORD_H
#define LEGORRR_GAME_TEXT_RECORD_H

#include "gods98/textwindow_record.h"

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
} Text_Globs; /* 0x4dc on Win32 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_TEXT_RECORD_H */
