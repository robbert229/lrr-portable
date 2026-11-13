// SPDX-License-Identifier: LGPL-3.0-or-later
/* Bubbles.c, original translation unit GAME-005 (finding 505), rebuilt in
 * original shape (ADR 0005; finding 598): text 004070b0-00407d60, .rdata
 * 0049e060-0049e06c, literals 004a19cc-004a1cb4, .bss 004b8a18, and the
 * state in the common region from 00557860.
 *
 * The objects' info overlays: the thought bubble over each minifigure
 * (its current task, or "Idle"), the health bar of an object that was
 * damaged, and the blinking power-off icon over an unpowered building.
 * Each kind is a table of objects with a timer; Bubble_DrawAllObjInfos
 * draws and counts down the tables every frame. With the object UIs
 * always visible, every minifigure shows its bubble and health bar.
 *
 * The port's implementation stays in the port tail bubbles_port.cpp,
 * unannotated. In the matching build bubbles_link_names.h gives this
 * unit's functions the port's names, so every caller reaches them;
 * natively the port tail serves those names. */
#include "bubbles_link_names.h"

#include <string.h>

#include "crt/vc5names.h"
#include "bubbles.h"
#include "bubbles_imports.h"

/* The unit's functions have C linkage in the native C++ compile too. */
#ifdef __cplusplus
extern "C" {
#endif

typedef enum Bubble_Type {
    Bubble_CantDo,
    Bubble_Idle,
    Bubble_CollectCrystal,
    Bubble_CollectOre,
    Bubble_CollectStud,
    Bubble_CollectDynamite,
    Bubble_CollectBarrier,
    Bubble_CollectElecFence,
    Bubble_CollectDrill,
    Bubble_CollectSpade,
    Bubble_CollectHammer,
    Bubble_CollectSpanner,
    Bubble_CollectLaser,
    Bubble_CollectPusher,
    Bubble_CollectFreezer,
    Bubble_CollectBirdScarer,
    Bubble_CarryCrystal,
    Bubble_CarryOre,
    Bubble_CarryStud,
    Bubble_CarryDynamite,
    Bubble_CarryBarrier,
    Bubble_CarryElecFence,
    Bubble_Goto,
    Bubble_Dynamite,
    Bubble_Reinforce,
    Bubble_Drill,
    Bubble_Repair,
    Bubble_Dig,
    Bubble_Flee,
    Bubble_PowerOff,
    Bubble_CallToArms,
    Bubble_ElecFence,
    Bubble_Eat,
    Bubble_Drive,
    Bubble_Upgrade,
    Bubble_BuildPath,
    Bubble_Train,
    Bubble_Recharge,
    Bubble_Request,
    Bubble_Type_Count
} Bubble_Type;

#define BUBBLE_MAXHEALTHBARS 20
#define BUBBLE_MAXBUBBLES 20
#define BUBBLE_MAXPOWEROFFS 20
#define BUBBLE_MAXCALLTOARMS 50

/* LegoObject_Type and the flags the unit reads. */
#define LEGOOBJECT_MINIFIGURE 2
#define LEGOOBJECT_BUILDING 4
#define LIVEOBJ1_HIDDEN 0x600000             /* flags1: removed or teleported */
#define LIVEOBJ1_RUNNINGAWAY 0x800000        /* flags1 */
#define LIVEOBJ1_CANTDO 0x80000000           /* flags1 */
#define LIVEOBJ3_POWEROFF 0x20000000         /* flags3: the icon goes */
#define LIVEOBJ4_CALLTOARMS 0x20             /* flags4 */
#define STATS3_SHOWHEALTHBAR 0x80            /* StatsFlags3 */
#define MAIN_FLAG_REDUCEIMAGES 0x2000000     /* mainGlobs.flags */

/* AITask types, named here by the bubble each one shows (the original
 * AITask_Type names are not recovered), and the tool types of 0x13. */
#define AITASK_TYPE_GOTO 0x00
#define AITASK_TYPE_COLLECT 0x03
#define AITASK_TYPE_GATHER 0x05
#define AITASK_TYPE_REQUEST 0x07
#define AITASK_TYPE_DIG 0x08
#define AITASK_TYPE_DYNAMITE 0x09
#define AITASK_TYPE_REPAIR 0x0a
#define AITASK_TYPE_REINFORCE 0x0b
#define AITASK_TYPE_CLEAR 0x0c
#define AITASK_TYPE_WAIT 0x0e
#define AITASK_TYPE_ELECFENCE 0x0f
#define AITASK_TYPE_EAT 0x10
#define AITASK_TYPE_GOTOEAT 0x11
#define AITASK_TYPE_DRIVE 0x12
#define AITASK_TYPE_GETTOOL 0x13
#define AITASK_TYPE_UPGRADE 0x15
#define AITASK_TYPE_BUILDPATH 0x16
#define AITASK_TYPE_TRAIN 0x17
#define AITASK_TYPE_RECHARGE 0x1b

/* LegoObject_Type of a collected or carried object. */
#define LEGOOBJECT_POWERCRYSTAL 6
#define LEGOOBJECT_ORE 7
#define LEGOOBJECT_DYNAMITE 8
#define LEGOOBJECT_BARRIER 9
#define LEGOOBJECT_ELECTRICFENCE 11

typedef struct Bubble_Vector3F {
    float x;
    float y;
    float z;
} Bubble_Vector3F;

/* The original command record's fields the unit reads (AITask, 0x68
 * bytes; aitask.h). */
typedef struct Bubble_AITask {
    int taskType;                    /* +0x00 */
    unsigned char pad_04[0x10 - 0x04];
    Lego_Object* targetObject;       /* +0x10 */
    unsigned char pad_14[0x38 - 0x14];
    int toolType;                    /* +0x38 */
    unsigned char pad_3c[0x60 - 0x3c];
    struct Bubble_AITask* next;      /* +0x60 */
} Bubble_AITask;

/* One table entry: the object and the time it has left. */
typedef struct BubbleTimer {
    Lego_Object* object;
    float timer;
} BubbleTimer;

/* The unit's state (0x4f4 bytes). */
typedef struct Bubble_Globs {
    int alwaysVisible;                           /* 00557860 */
    Image* bubbleImage[Bubble_Type_Count];       /* 00557864 */
    const char* bubbleName[Bubble_Type_Count];   /* 00557900 */
    unsigned int reserved[18];                   /* 0055799c: no reference */
    BubbleTimer healthBarTable[BUBBLE_MAXHEALTHBARS]; /* 005579e4 */
    BubbleTimer bubbleTable[BUBBLE_MAXBUBBLES];  /* 00557a84 */
    BubbleTimer powerOffTable[BUBBLE_MAXPOWEROFFS]; /* 00557b24 */
    BubbleTimer callToArmsTable[BUBBLE_MAXCALLTOARMS]; /* 00557bc4 */
} Bubble_Globs;

// GLOBAL: LEGORR 0x00557860
Bubble_Globs bubbleGlobs;

// FUNCTION: LEGORR 0x004070b0
void bubble_initialise(void)
{
    // STRING: LEGORR 0x004a1ca4
    bubbleGlobs.bubbleName[Bubble_CantDo] = "Bubble_CantDo";
    // STRING: LEGORR 0x004a1c98
    bubbleGlobs.bubbleName[Bubble_Idle] = "Bubble_Idle";
    // STRING: LEGORR 0x004a1c80
    bubbleGlobs.bubbleName[Bubble_CollectCrystal] = "Bubble_CollectCrystal";
    // STRING: LEGORR 0x004a1c6c
    bubbleGlobs.bubbleName[Bubble_CollectOre] = "Bubble_CollectOre";
    // STRING: LEGORR 0x004a1c58
    bubbleGlobs.bubbleName[Bubble_CollectStud] = "Bubble_CollectStud";
    // STRING: LEGORR 0x004a1c40
    bubbleGlobs.bubbleName[Bubble_CollectDynamite] = "Bubble_CollectDynamite";
    // STRING: LEGORR 0x004a1c28
    bubbleGlobs.bubbleName[Bubble_CollectElecFence] = "Bubble_CollectElecFence";
    // STRING: LEGORR 0x004a1c10
    bubbleGlobs.bubbleName[Bubble_CollectBarrier] = "Bubble_CollectBarrier";
    // STRING: LEGORR 0x004a1bfc
    bubbleGlobs.bubbleName[Bubble_CollectDrill] = "Bubble_CollectDrill";
    // STRING: LEGORR 0x004a1be8
    bubbleGlobs.bubbleName[Bubble_CollectSpade] = "Bubble_CollectSpade";
    // STRING: LEGORR 0x004a1bd0
    bubbleGlobs.bubbleName[Bubble_CollectHammer] = "Bubble_CollectHammer";
    // STRING: LEGORR 0x004a1bb8
    bubbleGlobs.bubbleName[Bubble_CollectSpanner] = "Bubble_CollectSpanner";
    // STRING: LEGORR 0x004a1ba4
    bubbleGlobs.bubbleName[Bubble_CollectLaser] = "Bubble_CollectLaser";
    // STRING: LEGORR 0x004a1b8c
    bubbleGlobs.bubbleName[Bubble_CollectPusher] = "Bubble_CollectPusher";
    // STRING: LEGORR 0x004a1b74
    bubbleGlobs.bubbleName[Bubble_CollectFreezer] = "Bubble_CollectFreezer";
    // STRING: LEGORR 0x004a1b58
    bubbleGlobs.bubbleName[Bubble_CollectBirdScarer] = "Bubble_CollectBirdScarer";
    // STRING: LEGORR 0x004a1b44
    bubbleGlobs.bubbleName[Bubble_CarryCrystal] = "Bubble_CarryCrystal";
    // STRING: LEGORR 0x004a1b34
    bubbleGlobs.bubbleName[Bubble_CarryOre] = "Bubble_CarryOre";
    // STRING: LEGORR 0x004a1b20
    bubbleGlobs.bubbleName[Bubble_CarryStud] = "Bubble_CarryStud";
    // STRING: LEGORR 0x004a1b08
    bubbleGlobs.bubbleName[Bubble_CarryDynamite] = "Bubble_CarryDynamite";
    // STRING: LEGORR 0x004a1af4
    bubbleGlobs.bubbleName[Bubble_CarryBarrier] = "Bubble_CarryBarrier";
    // STRING: LEGORR 0x004a1adc
    bubbleGlobs.bubbleName[Bubble_CarryElecFence] = "Bubble_CarryElecFence";
    // STRING: LEGORR 0x004a1ad0
    bubbleGlobs.bubbleName[Bubble_Goto] = "Bubble_Goto";
    // STRING: LEGORR 0x004a1ac0
    bubbleGlobs.bubbleName[Bubble_Dynamite] = "Bubble_Dynamite";
    // STRING: LEGORR 0x004a1aac
    bubbleGlobs.bubbleName[Bubble_Reinforce] = "Bubble_Reinforce";
    // STRING: LEGORR 0x004a1a9c
    bubbleGlobs.bubbleName[Bubble_Drill] = "Bubble_Drill";
    // STRING: LEGORR 0x004a1a8c
    bubbleGlobs.bubbleName[Bubble_Repair] = "Bubble_Repair";
    // STRING: LEGORR 0x004a1a80
    bubbleGlobs.bubbleName[Bubble_Dig] = "Bubble_Dig";
    // STRING: LEGORR 0x004a1a74
    bubbleGlobs.bubbleName[Bubble_Flee] = "Bubble_Flee";
    // STRING: LEGORR 0x004a1a64
    bubbleGlobs.bubbleName[Bubble_PowerOff] = "Bubble_PowerOff";
    // STRING: LEGORR 0x004a1a50
    bubbleGlobs.bubbleName[Bubble_CallToArms] = "Bubble_CallToArms";
    // STRING: LEGORR 0x004a1a3c
    bubbleGlobs.bubbleName[Bubble_ElecFence] = "Bubble_ElecFence";
    // STRING: LEGORR 0x004a1a30
    bubbleGlobs.bubbleName[Bubble_Eat] = "Bubble_Eat";
    // STRING: LEGORR 0x004a1a20
    bubbleGlobs.bubbleName[Bubble_Drive] = "Bubble_Drive";
    // STRING: LEGORR 0x004a1a10
    bubbleGlobs.bubbleName[Bubble_Upgrade] = "Bubble_Upgrade";
    // STRING: LEGORR 0x004a19fc
    bubbleGlobs.bubbleName[Bubble_BuildPath] = "Bubble_BuildPath";
    // STRING: LEGORR 0x004a19ec
    bubbleGlobs.bubbleName[Bubble_Train] = "Bubble_Train";
    // STRING: LEGORR 0x004a19dc
    bubbleGlobs.bubbleName[Bubble_Recharge] = "Bubble_Recharge";
    // STRING: LEGORR 0x004a19cc
    bubbleGlobs.bubbleName[Bubble_Request] = "Bubble_Request";
}

// FUNCTION: LEGORR 0x00407240
void bubble_load_bubbles(BlockRec* config)
{
    BlockRec* arrayItem;
    char* name;
    int use;
    int bubbleType;

    // STRING: LEGORR 0x004a1cb4
    for (arrayItem = block_subgroup(config, string_path_join(s_engine_config_root, "Bubbles", 0));
         arrayItem != NULL; arrayItem = block_next_equal_key(arrayItem)) {
        name = arrayItem->key;
        use = 1;
        if (name != NULL && strlen(name) != 0 && *name == '!') {
            if (!(s_engine_feature_word & MAIN_FLAG_REDUCEIMAGES)) name++;
            else use = 0;
        }
        if (use) {
            bubbleType = bubble_get_bubble_type(name);
            if ((bubbleGlobs.bubbleImage[bubbleType] = image_load_bmp_scaled(arrayItem->value, 0, 0)) != NULL) {
                image_surface_blit_transformed((struct TextureSurface*)bubbleGlobs.bubbleImage[bubbleType],
                                               0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
            }
        }
    }
}

// FUNCTION: LEGORR 0x00407300
int bubble_get_bubble_type(const char* name)
{
    int i;
    int result;

    for (i = 0; i < Bubble_Type_Count; i++) {
        if ((result = _strcmpi(bubbleGlobs.bubbleName[i], name)) == 0) return i;
    }
    return result;
}

// FUNCTION: LEGORR 0x00407340
void bubble_toggle_object_uis_always_visible(void)
{
    bubbleGlobs.alwaysVisible = !bubbleGlobs.alwaysVisible;
}

// FUNCTION: LEGORR 0x00407360
int bubble_get_object_uis_always_visible(void)
{
    return bubbleGlobs.alwaysVisible;
}

// FUNCTION: LEGORR 0x00407370
void bubble_reset_object_bubble_image(Lego_Object* liveObj)
{
    if (bubbleGlobs.alwaysVisible) {
        liveObj->bubbleTimer = 0.0f;
        bubble_live_object_get_bubble_image(liveObj, (Image**)&liveObj->bubbleImage);
    }
}

// FUNCTION: LEGORR 0x004073a0
void bubble_remove_object(Lego_Object* liveObj)
{
    unsigned int i;

    for (i = 0; i < BUBBLE_MAXHEALTHBARS; i++) {
        if (liveObj == bubbleGlobs.healthBarTable[i].object) bubbleGlobs.healthBarTable[i].object = NULL;
    }
    for (i = 0; i < BUBBLE_MAXBUBBLES; i++) {
        if (liveObj == bubbleGlobs.bubbleTable[i].object) bubbleGlobs.bubbleTable[i].object = NULL;
    }
    for (i = 0; i < BUBBLE_MAXPOWEROFFS; i++) {
        if (liveObj == bubbleGlobs.powerOffTable[i].object) bubbleGlobs.powerOffTable[i].object = NULL;
    }
    for (i = 0; i < BUBBLE_MAXCALLTOARMS; i++) {
        if (liveObj == bubbleGlobs.callToArmsTable[i].object) bubbleGlobs.callToArmsTable[i].object = NULL;
    }
}

// FUNCTION: LEGORR 0x00407410
void bubble_show_power_off(Lego_Object* liveObj)
{
    unsigned int i;
    BubbleTimer* bubbleTimer;

    if (liveObj->type == LEGOOBJECT_BUILDING) {
        for (i = 0; i < BUBBLE_MAXPOWEROFFS; i++) {
            if (liveObj == bubbleGlobs.powerOffTable[i].object) break;
        }
        if (i == BUBBLE_MAXPOWEROFFS) {
            for (i = 0; i < BUBBLE_MAXPOWEROFFS; i++) {
                bubbleTimer = &bubbleGlobs.powerOffTable[i];
                if (bubbleTimer->object == NULL) {
                    bubbleTimer->object = liveObj;
                    break;
                }
            }
        }
    }
}

// FUNCTION: LEGORR 0x00407450
void bubble_show_bubble(Lego_Object* liveObj)
{
    unsigned int i;
    BubbleTimer* bubbleTimer;

    if (liveObj->type == LEGOOBJECT_MINIFIGURE) {
        for (i = 0; i < BUBBLE_MAXBUBBLES; i++) {
            bubbleTimer = &bubbleGlobs.bubbleTable[i];
            if (liveObj == bubbleTimer->object) break;
        }
        if (i == BUBBLE_MAXBUBBLES) {
            for (i = 0; i < BUBBLE_MAXBUBBLES; i++) {
                bubbleTimer = &bubbleGlobs.bubbleTable[i];
                if (bubbleTimer->object == NULL) {
                    bubbleTimer->object = liveObj;
                    break;
                }
            }
            if (i == BUBBLE_MAXBUBBLES) return;
        }
        bubbleTimer->timer = 12.5f;
    }
}

// FUNCTION: LEGORR 0x004074b0
void bubble_show_call_to_arms(Lego_Object* liveObj)
{
    unsigned int i;
    BubbleTimer* bubbleTimer;

    if (liveObj->type == LEGOOBJECT_MINIFIGURE) {
        for (i = 0; i < BUBBLE_MAXCALLTOARMS; i++) {
            bubbleTimer = &bubbleGlobs.callToArmsTable[i];
            if (liveObj == bubbleTimer->object) break;
        }
        if (i == BUBBLE_MAXCALLTOARMS) {
            for (i = 0; i < BUBBLE_MAXCALLTOARMS; i++) {
                bubbleTimer = &bubbleGlobs.callToArmsTable[i];
                if (bubbleTimer->object == NULL) {
                    bubbleTimer->object = liveObj;
                    break;
                }
            }
            if (i == BUBBLE_MAXCALLTOARMS) return;
        }
        bubbleTimer->timer = 2250000.0f;
    }
}

// FUNCTION: LEGORR 0x00407510
void bubble_set_call_to_arms_timer(Lego_Object* liveObj, float timer)
{
    unsigned int i;

    if (liveObj->type == LEGOOBJECT_MINIFIGURE) {
        for (i = 0; i < BUBBLE_MAXCALLTOARMS; i++) {
            if (bubbleGlobs.callToArmsTable[i].object != NULL) bubbleGlobs.callToArmsTable[i].timer = timer;
        }
    }
}

// FUNCTION: LEGORR 0x00407540
void bubble_show_health_bar(Lego_Object* liveObj)
{
    unsigned int i;
    BubbleTimer* bubbleTimer;

    if (entity_capability_flags_get((struct EntitySlot*)liveObj) & STATS3_SHOWHEALTHBAR) {
        for (i = 0; i < BUBBLE_MAXHEALTHBARS; i++) {
            bubbleTimer = &bubbleGlobs.healthBarTable[i];
            if (liveObj == bubbleTimer->object) break;
        }
        if (i == BUBBLE_MAXHEALTHBARS) {
            for (i = 0; i < BUBBLE_MAXHEALTHBARS; i++) {
                bubbleTimer = &bubbleGlobs.healthBarTable[i];
                if (bubbleTimer->object == NULL) {
                    bubbleTimer->object = liveObj;
                    break;
                }
            }
            if (i == BUBBLE_MAXHEALTHBARS) return;
        }
        bubbleTimer->timer = 37.5f;
    }
}

// FUNCTION: LEGORR 0x004075a0
void bubble_draw_all_obj_infos(float elapsedGame)
{
    // GLOBAL: LEGORR 0x004b8a18
    static float bubble_powerOffTimer;
    unsigned int i;
    Lego_Object* liveObj;

    if (bubbleGlobs.alwaysVisible && !current_engine_mode_is_zero()) {
        legoobject_run_through_lists_skip_upgrade_parts(bubble_callback_draw_obj_info, &elapsedGame);
    }

    for (i = 0; i < BUBBLE_MAXHEALTHBARS; i++) {
        if ((liveObj = bubbleGlobs.healthBarTable[i].object) != NULL) {
            if ((liveObj->type != LEGOOBJECT_MINIFIGURE || !bubbleGlobs.alwaysVisible) &&
                !current_engine_mode_is_zero() && !(liveObj->flags1 & LIVEOBJ1_HIDDEN)) {
                Point2F screenPt;
                Bubble_Vector3F wPos;
                ddiscene_object_position_get(legoobject_type_component_get(liveObj), NULL, (float*)&wPos);
                wPos.z -= entity_stats_float_at74((struct EntitySlot*)liveObj);
                viewport_world_to_screen((struct Viewport*)legoGlobs_viewMain, &screenPt, (struct Vector3F*)&wPos);
                legacy_entity_screen_ring_render(liveObj, (int)screenPt.x, (int)screenPt.y);
            }
            bubbleGlobs.healthBarTable[i].timer -= elapsedGame;
            if (bubbleGlobs.healthBarTable[i].timer < 0.0f) {
                bubbleGlobs.healthBarTable[i].object = NULL;
            }
        }
    }

    if (!current_engine_mode_is_zero()) {
        if (bubble_powerOffTimer < 12.5f) {
            for (i = 0; i < BUBBLE_MAXPOWEROFFS; i++) {
                if ((liveObj = bubbleGlobs.powerOffTable[i].object) != NULL) {
                    if (liveObj->flags3 & LIVEOBJ3_POWEROFF) {
                        bubbleGlobs.powerOffTable[i].object = NULL;
                    } else {
                        Image* image = bubbleGlobs.bubbleImage[Bubble_PowerOff];
                        Point2F screenPt;
                        Bubble_Vector3F wPos;
                        ddiscene_object_position_get(legoobject_type_component_get(liveObj), NULL, (float*)&wPos);
                        wPos.z -= entity_stats_float_at74((struct EntitySlot*)liveObj);
                        viewport_world_to_screen((struct Viewport*)legoGlobs_viewMain, &screenPt, (struct Vector3F*)&wPos);
                        screenPt.y -= (float)(int)image->height;
                        screenPt.x -= (float)(int)image->width * 0.5f;
                        image_display_scaled(image, NULL, &screenPt, NULL);
                    }
                }
            }
        } else if (bubble_powerOffTimer >= 25.0f) {
            bubble_powerOffTimer -= 25.0f;
        }
        bubble_powerOffTimer += elapsedGame;
    }

    for (i = 0; i < BUBBLE_MAXBUBBLES; i++) {
        if ((liveObj = bubbleGlobs.bubbleTable[i].object) != NULL) {
            if ((liveObj->type != LEGOOBJECT_MINIFIGURE || !bubbleGlobs.alwaysVisible) &&
                !(liveObj->flags1 & LIVEOBJ1_HIDDEN) && !current_engine_mode_is_zero()) {
                Image* image = NULL;
                Point2F screenPt;
                bubble_live_object_update_bubble_image(liveObj, elapsedGame, &image, &screenPt);
                if (image != NULL && image != bubbleGlobs.bubbleImage[Bubble_Idle]) {
                    panel_resource_position_apply_if_enabled(image, (int)screenPt.x, (int)screenPt.y);
                }
            }
            bubbleGlobs.bubbleTable[i].timer -= elapsedGame;
            if (bubbleGlobs.bubbleTable[i].timer < 0.0f) {
                bubbleGlobs.bubbleTable[i].object = NULL;
            }
        }
    }

    for (i = 0; i < BUBBLE_MAXCALLTOARMS; i++) {
        if ((liveObj = bubbleGlobs.callToArmsTable[i].object) != NULL) {
            if (!bubbleGlobs.alwaysVisible && !(liveObj->flags1 & LIVEOBJ1_HIDDEN) &&
                !current_engine_mode_is_zero()) {
                Image* image = NULL;
                Point2F screenPt;
                bubble_live_object_update_bubble_image(liveObj, elapsedGame, &image, &screenPt);
                if (image != NULL) {
                    panel_resource_position_apply_if_enabled(image, (int)screenPt.x, (int)screenPt.y);
                }
            }
            if (bubbleGlobs.callToArmsTable[i].timer <= 0.0f) {
                bubbleGlobs.callToArmsTable[i].object = NULL;
            }
        }
    }
}

// FUNCTION: LEGORR 0x004078c0
void bubble_live_object_update_bubble_image(Lego_Object* liveObj, float elapsedGame,
                                            Image** outImage, Point2F* screenPt)
{
    Bubble_Vector3F wPos;

    ddiscene_object_position_get(legoobject_type_component_get(liveObj), NULL, (float*)&wPos);
    wPos.z -= entity_stats_float_at74((struct EntitySlot*)liveObj);
    viewport_world_to_screen((struct Viewport*)legoGlobs_viewMain, screenPt, (struct Vector3F*)&wPos);

    liveObj->bubbleTimer -= elapsedGame;
    if (liveObj->bubbleTimer > 0.0f) {
        *outImage = (Image*)liveObj->bubbleImage;
    } else {
        bubble_live_object_get_bubble_image(liveObj, outImage);
    }
    if (*outImage == NULL) *outImage = bubbleGlobs.bubbleImage[Bubble_Idle];
}

// FUNCTION: LEGORR 0x00407960
int bubble_callback_draw_obj_info(Lego_Object* liveObj, void* pElapsedGame)
{
    float elapsedGame = *(float*)pElapsedGame;
    Image* image;
    Point2F screenPt;

    if (liveObj->type == LEGOOBJECT_MINIFIGURE && !(liveObj->flags1 & LIVEOBJ1_HIDDEN)) {
        image = NULL;
        bubble_live_object_update_bubble_image(liveObj, elapsedGame, &image, &screenPt);
        legacy_entity_screen_ring_render(liveObj, (int)screenPt.x, (int)screenPt.y);
        objinfo_draw_hunger_image_adapter(liveObj, (int)screenPt.x, (int)screenPt.y);
        if (image != NULL) {
            panel_resource_position_apply_if_enabled(image, (int)screenPt.x, (int)screenPt.y);
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00407a10
void bubble_live_object_get_bubble_image(Lego_Object* liveObj, Image** outImage)
{
    Bubble_AITask* aiTask = (Bubble_AITask*)liveObj->aiTask;

    if (liveObj->flags4 & LIVEOBJ4_CALLTOARMS) {
        *outImage = bubbleGlobs.bubbleImage[Bubble_CallToArms];
        return;
    }
    if (liveObj->flags1 & LIVEOBJ1_RUNNINGAWAY) {
        *outImage = bubbleGlobs.bubbleImage[Bubble_Flee];
        return;
    }
    if (liveObj->flags1 & LIVEOBJ1_CANTDO) {
        *outImage = bubbleGlobs.bubbleImage[Bubble_CantDo];
        return;
    }
    if (aiTask == NULL) return;

    while (aiTask->taskType == AITASK_TYPE_WAIT && aiTask->next != NULL) aiTask = aiTask->next;

    switch (aiTask->taskType) {
    case AITASK_TYPE_GATHER:
        if (liveObj->carriedObjects[0] != NULL) {
            if (liveObj->carriedObjects[0]->type == LEGOOBJECT_POWERCRYSTAL) *outImage = bubbleGlobs.bubbleImage[Bubble_CarryCrystal];
            else if (liveObj->carriedObjects[0]->type == LEGOOBJECT_ORE) {
                if (liveObj->carriedObjects[0]->id == 0) *outImage = bubbleGlobs.bubbleImage[Bubble_CarryOre];
                else *outImage = bubbleGlobs.bubbleImage[Bubble_CarryStud];
            }
            else if (liveObj->carriedObjects[0]->type == LEGOOBJECT_DYNAMITE) *outImage = bubbleGlobs.bubbleImage[Bubble_CarryDynamite];
            else if (liveObj->carriedObjects[0]->type == LEGOOBJECT_BARRIER) *outImage = bubbleGlobs.bubbleImage[Bubble_CarryBarrier];
            else if (liveObj->carriedObjects[0]->type == LEGOOBJECT_ELECTRICFENCE) *outImage = bubbleGlobs.bubbleImage[Bubble_CarryElecFence];
        }
        break;
    case AITASK_TYPE_DIG:
        *outImage = bubbleGlobs.bubbleImage[Bubble_Drill];
        break;
    case AITASK_TYPE_REPAIR:
        *outImage = bubbleGlobs.bubbleImage[Bubble_Repair];
        break;
    case AITASK_TYPE_CLEAR:
        *outImage = bubbleGlobs.bubbleImage[Bubble_Dig];
        break;
    case AITASK_TYPE_GOTO:
        *outImage = bubbleGlobs.bubbleImage[Bubble_Goto];
        break;
    case AITASK_TYPE_COLLECT:
        if (aiTask->targetObject->type == LEGOOBJECT_POWERCRYSTAL) *outImage = bubbleGlobs.bubbleImage[Bubble_CollectCrystal];
        else if (aiTask->targetObject->type == LEGOOBJECT_ORE) {
            if (aiTask->targetObject->id == 0) *outImage = bubbleGlobs.bubbleImage[Bubble_CollectOre];
            else *outImage = bubbleGlobs.bubbleImage[Bubble_CollectStud];
        }
        else if (aiTask->targetObject->type == LEGOOBJECT_DYNAMITE) *outImage = bubbleGlobs.bubbleImage[Bubble_CollectDynamite];
        else if (aiTask->targetObject->type == LEGOOBJECT_BARRIER) *outImage = bubbleGlobs.bubbleImage[Bubble_CollectBarrier];
        else if (aiTask->targetObject->type == LEGOOBJECT_ELECTRICFENCE) *outImage = bubbleGlobs.bubbleImage[Bubble_CollectElecFence];
        break;
    case AITASK_TYPE_DYNAMITE:
        *outImage = bubbleGlobs.bubbleImage[Bubble_Dynamite];
        break;
    case AITASK_TYPE_GETTOOL:
        if (aiTask->toolType == 0) *outImage = bubbleGlobs.bubbleImage[Bubble_CollectDrill];
        else if (aiTask->toolType == 1) *outImage = bubbleGlobs.bubbleImage[Bubble_CollectSpade];
        else if (aiTask->toolType == 2) *outImage = bubbleGlobs.bubbleImage[Bubble_CollectHammer];
        else if (aiTask->toolType == 3) *outImage = bubbleGlobs.bubbleImage[Bubble_CollectSpanner];
        else if (aiTask->toolType == 4) *outImage = bubbleGlobs.bubbleImage[Bubble_CollectLaser];
        else if (aiTask->toolType == 5) *outImage = bubbleGlobs.bubbleImage[Bubble_CollectPusher];
        else if (aiTask->toolType == 7) *outImage = bubbleGlobs.bubbleImage[Bubble_CollectFreezer];
        else if (aiTask->toolType == 6) *outImage = bubbleGlobs.bubbleImage[Bubble_CollectBirdScarer];
        break;
    case AITASK_TYPE_REINFORCE:
        *outImage = bubbleGlobs.bubbleImage[Bubble_Reinforce];
        break;
    case AITASK_TYPE_ELECFENCE:
        *outImage = bubbleGlobs.bubbleImage[Bubble_ElecFence];
        break;
    case AITASK_TYPE_EAT:
    case AITASK_TYPE_GOTOEAT:
        *outImage = bubbleGlobs.bubbleImage[Bubble_Eat];
        break;
    case AITASK_TYPE_DRIVE:
        *outImage = bubbleGlobs.bubbleImage[Bubble_Drive];
        break;
    case AITASK_TYPE_BUILDPATH:
        *outImage = bubbleGlobs.bubbleImage[Bubble_BuildPath];
        break;
    case AITASK_TYPE_TRAIN:
        *outImage = bubbleGlobs.bubbleImage[Bubble_Train];
        break;
    case AITASK_TYPE_RECHARGE:
        *outImage = bubbleGlobs.bubbleImage[Bubble_Recharge];
        break;
    case AITASK_TYPE_UPGRADE:
        *outImage = bubbleGlobs.bubbleImage[Bubble_Upgrade];
        break;
    case AITASK_TYPE_REQUEST:
        *outImage = bubbleGlobs.bubbleImage[Bubble_Request];
        break;
    }
}

#ifdef __cplusplus
}
#endif
