// SPDX-License-Identifier: LGPL-3.0-or-later
/* Pointer.c, original translation unit GAME-033 (finding 505), rebuilt in
 * original shape (ADR 0005; finding 612): text 0045c830-0045cdc0, the
 * literals 004a8a14-004a8ed4 and the .bss block 00500a98-00500f00.
 *
 * The mouse pointer. Each of the 56 pointer types has a name
 * (Pointer_Standard to Pointer_Zoom) and, from Lego.cfg's Pointers block,
 * either a bitmap or an animated flic with a hotspot offset. A pointer set
 * with a timer (Pointer_SetCurrent) holds until the timer runs out; until
 * then Pointer_SetCurrent_IfTimerFinished does nothing.
 *
 * The port's implementation stays in the port tail pointer_port.cpp,
 * unannotated. In the matching build pointer_link_names.h gives this unit's
 * functions the port's names, so every caller reaches them; natively the
 * port tail serves those names. */
#include "pointer_link_names.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crt/vc5names.h"
#include "pointer.h"
#include "pointer_imports.h"

/* The unit's functions have C linkage in the native C++ compile too. */
#ifdef __cplusplus
extern "C" {
#endif

typedef enum Pointer_Type {
    Pointer_Standard = 0, /* 0 */
    Pointer_Blank, /* 1 */
    Pointer_Selected, /* 2 */
    Pointer_Drill, /* 3 */
    Pointer_CantDrill, /* 4 */
    Pointer_Clear, /* 5 */
    Pointer_Go, /* 6 */
    Pointer_CantGo, /* 7 */
    Pointer_Teleport, /* 8 */
    Pointer_CantTeleport, /* 9 */
    Pointer_Reinforce, /* 10 */
    Pointer_CantReinforce, /* 11 */
    Pointer_RadarPan, /* 12 */
    Pointer_TrackObject, /* 13 */
    Pointer_Help, /* 14 */
    Pointer_CantHelp, /* 15 */
    Pointer_PutDown, /* 16 */
    Pointer_GetIn, /* 17 */
    Pointer_GetOut, /* 18 */
    Pointer_TutorialBlockInfo, /* 19 */
    Pointer_Okay, /* 20 */
    Pointer_NotOkay, /* 21 */
    Pointer_CanBuild, /* 22 */
    Pointer_CannotBuild, /* 23 */
    Pointer_Dynamite, /* 24 */
    Pointer_CantDynamite, /* 25 */
    Pointer_PickUp, /* 26 */
    Pointer_CantPickUp, /* 27 */
    Pointer_PickUpOre, /* 28 */
    Pointer_LegoManCantDig, /* 29 */
    Pointer_VehicleCantDig, /* 30 */
    Pointer_LegoManDig, /* 31 */
    Pointer_VehicleDig, /* 32 */
    Pointer_LegoManCantPickUp, /* 33 */
    Pointer_VehicleCantPickUp, /* 34 */
    Pointer_LegoManPickUp, /* 35 */
    Pointer_VehiclePickUp, /* 36 */
    Pointer_LegoManCantGo, /* 37 */
    Pointer_VehicleCantGo, /* 38 */
    Pointer_LegoManGo, /* 39 */
    Pointer_VehicleGo, /* 40 */
    Pointer_LegoManClear, /* 41 */
    Pointer_VehicleClear, /* 42 */
    Pointer_SurfaceType_Immovable, /* 43 */
    Pointer_SurfaceType_Hard, /* 44 */
    Pointer_SurfaceType_Medium, /* 45 */
    Pointer_SurfaceType_Loose, /* 46 */
    Pointer_SurfaceType_Soil, /* 47 */
    Pointer_SurfaceType_Lava, /* 48 */
    Pointer_SurfaceType_Water, /* 49 */
    Pointer_SurfaceType_OreSeam, /* 50 */
    Pointer_SurfaceType_Lake, /* 51 */
    Pointer_SurfaceType_CrystalSeam, /* 52 */
    Pointer_SurfaceType_RechargeSeam, /* 53 */
    Pointer_CantZoom, /* 54 */
    Pointer_Zoom, /* 55 */
    Pointer_Type_Count /* 56 */
} Pointer_Type;

/* Main.c's flags: the '!' prefix skips a pointer while this bit is set. */
#define MAIN_FLAG_REDUCEFLICS 0x2000000

/* Lego.c's flags1: the game is paused, and animated pointers stop. */
#define GAME1_PAUSED 0x100000

typedef struct Pointer_Point2I {
    int x;
    int y;
} Pointer_Point2I;

typedef struct Pointer_Globs {
    int currType;                               /* +0x000 00500a98 */
    Image* images[Pointer_Type_Count];          /* +0x004 00500a9c: Flic* when isFlic */
    int isFlic[Pointer_Type_Count];             /* +0x0e4 00500b7c */
    Pointer_Point2I flicOffsets[Pointer_Type_Count]; /* +0x1c4 00500c5c */
    const char* names[Pointer_Type_Count];      /* +0x384 00500e1c */
    float timer;                                /* +0x464 00500efc */
} Pointer_Globs; /* 0x468 */

// GLOBAL: LEGORR 0x00500a98
static Pointer_Globs pointerGlobs;

// FUNCTION: LEGORR 0x0045c830
void pointer_initialise(void)
{
    // STRING: LEGORR 0x004a8ed4
    pointerGlobs.names[Pointer_Standard] = "Pointer_Standard";
    // STRING: LEGORR 0x004a8ec4
    pointerGlobs.names[Pointer_Blank] = "Pointer_Blank";
    // STRING: LEGORR 0x004a8eb0
    pointerGlobs.names[Pointer_Selected] = "Pointer_Selected";
    // STRING: LEGORR 0x004a8ea0
    pointerGlobs.names[Pointer_Drill] = "Pointer_Drill";
    // STRING: LEGORR 0x004a8e8c
    pointerGlobs.names[Pointer_CantDrill] = "Pointer_CantDrill";
    // STRING: LEGORR 0x004a8e7c
    pointerGlobs.names[Pointer_Clear] = "Pointer_Clear";
    // STRING: LEGORR 0x004a8e70
    pointerGlobs.names[Pointer_Go] = "Pointer_Go";
    // STRING: LEGORR 0x004a8e60
    pointerGlobs.names[Pointer_CantGo] = "Pointer_CantGo";
    // STRING: LEGORR 0x004a8e4c
    pointerGlobs.names[Pointer_Teleport] = "Pointer_Teleport";
    // STRING: LEGORR 0x004a8e34
    pointerGlobs.names[Pointer_CantTeleport] = "Pointer_CantTeleport";
    // STRING: LEGORR 0x004a8e20
    pointerGlobs.names[Pointer_Reinforce] = "Pointer_Reinforce";
    // STRING: LEGORR 0x004a8e08
    pointerGlobs.names[Pointer_CantReinforce] = "Pointer_CantReinforce";
    // STRING: LEGORR 0x004a8df4
    pointerGlobs.names[Pointer_RadarPan] = "Pointer_RadarPan";
    // STRING: LEGORR 0x004a8de0
    pointerGlobs.names[Pointer_TrackObject] = "Pointer_TrackObject";
    // STRING: LEGORR 0x004a8dcc
    pointerGlobs.names[Pointer_CantZoom] = "Pointer_CantZoom";
    // STRING: LEGORR 0x004a8dbc
    pointerGlobs.names[Pointer_Zoom] = "Pointer_Zoom";
    // STRING: LEGORR 0x004a8dac
    pointerGlobs.names[Pointer_Help] = "Pointer_Help";
    // STRING: LEGORR 0x004a8d98
    pointerGlobs.names[Pointer_CantHelp] = "Pointer_CantHelp";
    // STRING: LEGORR 0x004a8d88
    pointerGlobs.names[Pointer_PutDown] = "Pointer_PutDown";
    // STRING: LEGORR 0x004a8d78
    pointerGlobs.names[Pointer_GetIn] = "Pointer_GetIn";
    // STRING: LEGORR 0x004a8d68
    pointerGlobs.names[Pointer_GetOut] = "Pointer_GetOut";
    // STRING: LEGORR 0x004a8d4c
    pointerGlobs.names[Pointer_TutorialBlockInfo] = "Pointer_TutorialBlockInfo";
    // STRING: LEGORR 0x004a8d3c
    pointerGlobs.names[Pointer_Okay] = "Pointer_Okay";
    // STRING: LEGORR 0x004a8d2c
    pointerGlobs.names[Pointer_NotOkay] = "Pointer_NotOkay";
    // STRING: LEGORR 0x004a8d18
    pointerGlobs.names[Pointer_CanBuild] = "Pointer_CanBuild";
    // STRING: LEGORR 0x004a8d04
    pointerGlobs.names[Pointer_CannotBuild] = "Pointer_CannotBuild";
    // STRING: LEGORR 0x004a8cf0
    pointerGlobs.names[Pointer_Dynamite] = "Pointer_Dynamite";
    // STRING: LEGORR 0x004a8cd8
    pointerGlobs.names[Pointer_CantDynamite] = "Pointer_CantDynamite";
    // STRING: LEGORR 0x004a8cc8
    pointerGlobs.names[Pointer_PickUp] = "Pointer_PickUp";
    // STRING: LEGORR 0x004a8cb4
    pointerGlobs.names[Pointer_CantPickUp] = "Pointer_CantPickUp";
    // STRING: LEGORR 0x004a8ca0
    pointerGlobs.names[Pointer_PickUpOre] = "Pointer_PickUpOre";
    // STRING: LEGORR 0x004a8c88
    pointerGlobs.names[Pointer_LegoManCantDig] = "Pointer_LegoManCantDig";
    // STRING: LEGORR 0x004a8c70
    pointerGlobs.names[Pointer_VehicleCantDig] = "Pointer_VehicleCantDig";
    // STRING: LEGORR 0x004a8c5c
    pointerGlobs.names[Pointer_LegoManDig] = "Pointer_LegoManDig";
    // STRING: LEGORR 0x004a8c48
    pointerGlobs.names[Pointer_VehicleDig] = "Pointer_VehicleDig";
    // STRING: LEGORR 0x004a8c2c
    pointerGlobs.names[Pointer_LegoManCantPickUp] = "Pointer_LegoManCantPickUp";
    // STRING: LEGORR 0x004a8c10
    pointerGlobs.names[Pointer_VehicleCantPickUp] = "Pointer_VehicleCantPickUp";
    // STRING: LEGORR 0x004a8bf8
    pointerGlobs.names[Pointer_LegoManPickUp] = "Pointer_LegoManPickUp";
    // STRING: LEGORR 0x004a8be0
    pointerGlobs.names[Pointer_VehiclePickUp] = "Pointer_VehiclePickUp";
    // STRING: LEGORR 0x004a8bc8
    pointerGlobs.names[Pointer_LegoManCantGo] = "Pointer_LegoManCantGo";
    // STRING: LEGORR 0x004a8bb0
    pointerGlobs.names[Pointer_VehicleCantGo] = "Pointer_VehicleCantGo";
    // STRING: LEGORR 0x004a8b9c
    pointerGlobs.names[Pointer_LegoManGo] = "Pointer_LegoManGo";
    // STRING: LEGORR 0x004a8b88
    pointerGlobs.names[Pointer_VehicleGo] = "Pointer_VehicleGo";
    // STRING: LEGORR 0x004a8b70
    pointerGlobs.names[Pointer_LegoManClear] = "Pointer_LegoManClear";
    // STRING: LEGORR 0x004a8b58
    pointerGlobs.names[Pointer_VehicleClear] = "Pointer_VehicleClear";
    // STRING: LEGORR 0x004a8b38
    pointerGlobs.names[Pointer_SurfaceType_Immovable] = "Pointer_SurfaceType_Immovable";
    // STRING: LEGORR 0x004a8b1c
    pointerGlobs.names[Pointer_SurfaceType_Hard] = "Pointer_SurfaceType_Hard";
    // STRING: LEGORR 0x004a8b00
    pointerGlobs.names[Pointer_SurfaceType_Medium] = "Pointer_SurfaceType_Medium";
    // STRING: LEGORR 0x004a8ae4
    pointerGlobs.names[Pointer_SurfaceType_Loose] = "Pointer_SurfaceType_Loose";
    // STRING: LEGORR 0x004a8ac8
    pointerGlobs.names[Pointer_SurfaceType_Soil] = "Pointer_SurfaceType_Soil";
    // STRING: LEGORR 0x004a8aac
    pointerGlobs.names[Pointer_SurfaceType_Lava] = "Pointer_SurfaceType_Lava";
    // STRING: LEGORR 0x004a8a90
    pointerGlobs.names[Pointer_SurfaceType_Water] = "Pointer_SurfaceType_Water";
    // STRING: LEGORR 0x004a8a74
    pointerGlobs.names[Pointer_SurfaceType_OreSeam] = "Pointer_SurfaceType_OreSeam";
    // STRING: LEGORR 0x004a8a58
    pointerGlobs.names[Pointer_SurfaceType_Lake] = "Pointer_SurfaceType_Lake";
    // STRING: LEGORR 0x004a8a38
    pointerGlobs.names[Pointer_SurfaceType_CrystalSeam] = "Pointer_SurfaceType_CrystalSeam";
    // STRING: LEGORR 0x004a8a14
    pointerGlobs.names[Pointer_SurfaceType_RechargeSeam] = "Pointer_SurfaceType_RechargeSeam";}

// FUNCTION: LEGORR 0x0045ca70
void pointer_load_pointers(BlockRec* config)
{
    char* name;
    int ok;
    int pointerType;
    char* argv[10];
    char buff[1024];

    for (; config != NULL; config = block_next_equal_key(config)) {
        name = config->key;
        ok = 1;
        if (name != NULL && strlen(name) && name[0] == '!') {
            if (!(s_engine_feature_word & MAIN_FLAG_REDUCEFLICS)) name++;
            else ok = 0;
        }
        if (ok && pointer_get_type(name, &pointerType)) {
            sprintf(buff, "%s", config->value);
            if (sound_variant_split(buff, argv, ",") == 1) {
                pointerGlobs.images[pointerType] = image_load_bmp_scaled(argv[0], 0, 0);
                pointerGlobs.isFlic[pointerType] = 0;
            } else {
                flic_setup(argv[0], (void**)&pointerGlobs.images[pointerType], 3);
                pointerGlobs.isFlic[pointerType] = 1;
                pointerGlobs.flicOffsets[pointerType].x = atoi(argv[1]);
                pointerGlobs.flicOffsets[pointerType].y = atoi(argv[2]);
            }
            if (!pointerGlobs.isFlic[pointerType]) {
                image_surface_blit_transformed((struct TextureSurface*)pointerGlobs.images[pointerType],
                                               0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
            }
        }
    }
}

// FUNCTION: LEGORR 0x0045cbd0
int pointer_get_type(const char* name, int* pointerType)
{
    unsigned int i;

    for (i = 0; i < Pointer_Type_Count; i++) {
        if (!_strcmpi(pointerGlobs.names[i], name)) {
            *pointerType = i;
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x0045cc10
Image* pointer_get_image(int pointerType)
{
    return pointerGlobs.images[pointerType];
}

// FUNCTION: LEGORR 0x0045cc20
void pointer_set_current_if_timer_finished(int pointerType)
{
    if (pointerGlobs.timer <= 0.0f) pointerGlobs.currType = pointerType;
}

// FUNCTION: LEGORR 0x0045cc40
void pointer_set_current(int pointerType, float timer)
{
    pointerGlobs.currType = pointerType;
    pointerGlobs.timer = timer;
}

// FUNCTION: LEGORR 0x0045cc60
int pointer_get_current_type(void)
{
    return pointerGlobs.currType;
}

// FUNCTION: LEGORR 0x0045cc70
void pointer_draw_pointer(unsigned int mouseX, unsigned int mouseY)
{
    float x;
    float y;
    Point2F pos;
    Area2F destArea;
    Image* image;

    x = (float)mouseX;
    y = (float)mouseY;
    pos.x = x;
    pos.y = y;
    if (pointerGlobs.isFlic[pointerGlobs.currType] != 1) {
        image = pointerGlobs.images[pointerGlobs.currType];
        if (image == NULL) image = pointerGlobs.images[Pointer_Standard];
        image_display_scaled(image, NULL, &pos, NULL);
    } else {
        destArea.x = (float)pointerGlobs.flicOffsets[pointerGlobs.currType].x + x;
        destArea.y = (float)pointerGlobs.flicOffsets[pointerGlobs.currType].y + y;
        destArea.width = (float)(unsigned int)flic_width((struct Flic*)pointerGlobs.images[pointerGlobs.currType]);
        destArea.height = (float)(unsigned int)flic_height((struct Flic*)pointerGlobs.images[pointerGlobs.currType]);
        image_display_scaled(pointerGlobs.images[Pointer_Blank], NULL, &pos, NULL);
        flic_animate(pointerGlobs.images[pointerGlobs.currType], &destArea,
                     !(legoGlobs_flags1 & GAME1_PAUSED), 1);
    }
}

// FUNCTION: LEGORR 0x0045cd90
void pointer_update(float elapsed)
{
    if (pointerGlobs.timer > 0.0f) pointerGlobs.timer -= elapsed;
}

#ifdef __cplusplus
}
#endif
