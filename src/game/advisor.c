// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-001 Advisor.c — original translation unit (text 00401000-00401cb0,
 * .rdata 0049e004-0049e00c, literals 004a1030-004a13fc, .bss
 * 004b2db0-004b31c0; finding 505).
 *
 * The in-game advisor: a figure that plays one of eleven LWS animations at
 * one of 21 configured screen positions, beside a light of its own. The
 * unit also holds the out-of-line copies of gods98 Maths.h's vector helpers
 * that other units call, and four header inlines (Main.h, Font.h) that
 * nothing in the unit calls (finding 550).
 *
 * Rebuilt in the original's shape (issue #512, ADR 0005): its functions in
 * original order, its state in one globals struct laid out as the original
 * .bss, the C-compatible subset. The matching build compiles it as C; the
 * native port compiles it as C++. The exported functions keep the port's
 * names. Callee contracts that this reconstruction has not converted are
 * used as they are; finding 550 lists them. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "advisor.h"
#include "frontend.h"
#include "gods98/font.h"
#include "panel.h"
#include "crt/crt.h"
#include "crt/vc5names.h"
#include "gods98/config.h"
#include "gods98/engine.h"
#include "gods98/entity.h"
#include "gods98/message.h"
#include "gods98/record.h"
#include "gods98/scene_resources.h"
#include "gods98/sound.h"
#include "gods98/utils.h"
#include "gods98/view.h"

#define ADVISOR_TYPE_COUNT 21
#define ADVISOR_ANIM_COUNT 11

/* AdvisorPositionData flags. */
#define ADVISOR_FLAG_NOPANEL 0x10000 /* no panel offset */
#define ADVISOR_FLAG_USED    0x20000 /* configured */
#define ADVISOR_FLAG_HASTEXT 0x40000 /* shows a message */

/* AdvisorGlobs flags. */
#define ADVISOR_GLOB_FLAG_ANIMATING 0x1
#define ADVISOR_GLOB_FLAG_LOOPING   0x2

/* Text_Type_Count and Panel_Type_Count: "NULL" in the config. */
#define ADVISOR_TEXT_NONE  0x1a
#define ADVISOR_PANEL_NONE 0xc

/* SFX_Type: the advisor's own sound. */
#define ADVISOR_SFX 0x2a

/* Main.c's flags (DAT_00505f84): reduced animation. */
#define ADVISOR_MAIN_FLAG_REDUCEANIMATION 0x400000

/* The prefix of the game camera (LegoCamera) that Advisor_Initialise
 * reads: its camera container is the word at +0x24. */
typedef struct AdvisorCamera {
    void* fields[9];
    void* contCam;
} AdvisorCamera;

typedef struct AdvisorPositionData {
    int animType;       /* +00 */
    int textType;       /* +04 */
    int sfxID;          /* +08 */
    int panelType;      /* +0c */
    float x;            /* +10 */
    float y;            /* +14 */
    float origX;        /* +18 */
    float origY;        /* +1c */
    unsigned int flags; /* +20 */
} AdvisorPositionData;

typedef struct AdvisorAnimData {
    void* cont;          /* +0 */
    float loopStartTime; /* +4 */
    float loopEndTime;   /* +8 */
} AdvisorAnimData;

typedef struct AdvisorGlobs {
    const char* positionName[ADVISOR_TYPE_COUNT];     /* 004b2db0 */
    AdvisorPositionData positions[ADVISOR_TYPE_COUNT]; /* 004b2e04 */
    const char* animName[ADVISOR_ANIM_COUNT];          /* 004b30f8 */
    AdvisorAnimData anims[ADVISOR_ANIM_COUNT];         /* 004b3124 */
    int currType;                                      /* 004b31a8 */
    void* cameraCont;                                  /* 004b31ac */
    void* viewMain;                                    /* 004b31b0 */
    float scalar;                                      /* 004b31b4 */
    void* lightCont;                                   /* 004b31b8 */
    unsigned int flags;                                /* 004b31bc */
} AdvisorGlobs;

/* The unit's .rdata (0049e004-0049e00c). The 1.0 (0049e008) that
 * vec_normalize divides and the 0.0 (0049e00c) that
 * advisor_scene_advance_completion_check compares stay literals: MSVC pools
 * them at the same addresses, and as annotated statics they move vec_dot's
 * counter phase (finding 550). */
// GLOBAL: LEGORR 0x0049e004
static const float s_advisor_frames_per_second = 25.0f;

// GLOBAL: LEGORR 0x004b2db0
static AdvisorGlobs advisorGlobs;

// FUNCTION: LEGORR 0x00401000
void advisor_scene_controller_initialize(const char* gameName,
                                         void* camera,
                                         void* viewMain)
{
    // STRING: LEGORR 0x004a13cc
    advisorGlobs.positionName[0] = "Advisor_Objective";
    // STRING: LEGORR 0x004a13b0
    advisorGlobs.positionName[1] = "Advisor_ObjectiveAcheived";
    // STRING: LEGORR 0x004a1398
    advisorGlobs.positionName[2] = "Advisor_ObjectiveFailed";
    // STRING: LEGORR 0x004a137c
    advisorGlobs.positionName[3] = "Advisor_IconPoint_Normal";
    // STRING: LEGORR 0x004a135c
    advisorGlobs.positionName[4] = "Advisor_IconPoint_BackButton";
    // STRING: LEGORR 0x004a133c
    advisorGlobs.positionName[5] = "Advisor_IconPoint_TopButtons";
    // STRING: LEGORR 0x004a131c
    advisorGlobs.positionName[6] = "Advisor_PanelPoint_RadarToggle";
    // STRING: LEGORR 0x004a12f0
    advisorGlobs.positionName[7] = "Advisor_PanelPoint_RadarTaggedObjectView";
    // STRING: LEGORR 0x004a12d0
    advisorGlobs.positionName[8] = "Advisor_PanelPoint_RadarZoomIn";
    // STRING: LEGORR 0x004a12b0
    advisorGlobs.positionName[9] = "Advisor_PanelPoint_RadarZoomOut";
    // STRING: LEGORR 0x004a1290
    advisorGlobs.positionName[10] = "Advisor_PanelPoint_RadarMapView";
    // STRING: LEGORR 0x004a1270
    advisorGlobs.positionName[11] = "Advisor_PanelPoint_InfoDockGoto";
    // STRING: LEGORR 0x004a124c
    advisorGlobs.positionName[12] = "Advisor_PanelPoint_InfoDockClose";
    // STRING: LEGORR 0x004a122c
    advisorGlobs.positionName[13] = "Advisor_PanelPoint_TopPanelInfo";
    // STRING: LEGORR 0x004a1208
    advisorGlobs.positionName[14] = "Advisor_PanelPoint_TopPanelOptions";
    // STRING: LEGORR 0x004a11e0
    advisorGlobs.positionName[15] = "Advisor_PanelPoint_TopPanelPriorities";
    // STRING: LEGORR 0x004a11b8
    advisorGlobs.positionName[16] = "Advisor_PanelPoint_TopPanelCallToArms";
    // STRING: LEGORR 0x004a1194
    advisorGlobs.positionName[17] = "Advisor_PanelPoint_TopPanelGoBack";
    // STRING: LEGORR 0x004a1170
    advisorGlobs.positionName[18] = "Advisor_PanelPoint_ControlZoomIn";
    // STRING: LEGORR 0x004a114c
    advisorGlobs.positionName[19] = "Advisor_PanelPoint_ControlZoomOut";
    // STRING: LEGORR 0x004a1138
    advisorGlobs.positionName[20] = "Advisor_TalkInGame";

    // STRING: LEGORR 0x004a1120
    advisorGlobs.animName[0] = "Advisor_Anim_PointToMap";
    // STRING: LEGORR 0x004a1108
    advisorGlobs.animName[1] = "Advisor_Anim_Point_N";
    // STRING: LEGORR 0x004a10f0
    advisorGlobs.animName[2] = "Advisor_Anim_Point_NE";
    // STRING: LEGORR 0x004a10d8
    advisorGlobs.animName[3] = "Advisor_Anim_Point_E";
    // STRING: LEGORR 0x004a10c0
    advisorGlobs.animName[4] = "Advisor_Anim_Point_SE";
    // STRING: LEGORR 0x004a10a8
    advisorGlobs.animName[5] = "Advisor_Anim_Point_S";
    // STRING: LEGORR 0x004a1090
    advisorGlobs.animName[6] = "Advisor_Anim_Point_SW";
    // STRING: LEGORR 0x004a1078
    advisorGlobs.animName[7] = "Advisor_Anim_Point_W";
    // STRING: LEGORR 0x004a1060
    advisorGlobs.animName[8] = "Advisor_Anim_Point_NW";
    // STRING: LEGORR 0x004a1048
    advisorGlobs.animName[9] = "Advisor_Anim_Point_Up";
    // STRING: LEGORR 0x004a1030
    advisorGlobs.animName[10] = "Advisor_Anim_Talk_Top";

    advisorGlobs.cameraCont = ((AdvisorCamera*)camera)->contCam;
    advisorGlobs.viewMain = viewMain;
    advisorGlobs.lightCont = ddiscene_light_create(((AdvisorCamera*)camera)->contCam, 3,
                                                   0x3f800000, 0x3f800000,
                                                   0x3f800000);
    ddiscene_object_pose_method_dispatch(advisorGlobs.lightCont,
                                         ((AdvisorCamera*)camera)->contCam, 0.0f, 0.0f, -1.0f,
                                         0.0f, 1.0f, 0.0f);
    scene_render_lock_toggle(advisorGlobs.lightCont, 1);
    advisorGlobs.flags = 0;
}

// FUNCTION: LEGORR 0x004011c0
void advisor_animation_end_cleanup(void)
{
    if (advisorGlobs.anims[advisorGlobs.positions[advisorGlobs.currType].animType].cont)
        scene_render_lock_toggle(advisorGlobs.anims[advisorGlobs.positions[advisorGlobs.currType].animType].cont, 1);
    advisorGlobs.flags &= ~ADVISOR_GLOB_FLAG_ANIMATING;
    scene_render_lock_toggle(advisorGlobs.lightCont, 1);
}

// FUNCTION: LEGORR 0x00401210
void advisor_scene_visual_state_initialize(float distance)
{
    float frontClip;

    resource_interface_call_slot8c(advisorGlobs.viewMain);
    frontClip = resource_interface_call_slot90(advisorGlobs.viewMain);
    advisorGlobs.scalar = 0.96f;
}

// FUNCTION: LEGORR 0x00401240
float vec2_dist(const float* a, const float* b)
{
    return (float)sqrt((a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]));
}

// FUNCTION: LEGORR 0x00401270
void advisor_config_slots_load_target(BlockFile* config, const char* gameName)
{
    unsigned int loop;
    const char* str;
    char* argv[5];
    char name[64];

    for (loop = 0; loop < ADVISOR_ANIM_COUNT; loop++) {
        // STRING: LEGORR 0x004a13ec
        if ((str = block_get_strptr(config, string_path_join(gameName, "Advisor", advisorGlobs.animName[loop], 0))) == NULL) {
            if (!(s_engine_feature_word & ADVISOR_MAIN_FLAG_REDUCEANIMATION)) {
                // STRING: LEGORR 0x004a13e8
                sprintf(name, "!%s", advisorGlobs.animName[loop]);
                str = block_get_strptr(config, string_path_join(gameName, "Advisor", advisorGlobs.animName[loop], 0));
            }
        }
        if (str) {
            /* "," (004a13e4) is pooled with other units' uses, like
             * "NULL" below: no STRING marker. */
            sound_variant_split((char*)str, argv, ",");
            // STRING: LEGORR 0x004a13e0
            advisorGlobs.anims[loop].cont = container_load(advisorGlobs.cameraCont, argv[0], "LWS", 1);
            if (advisorGlobs.anims[loop].cont) {
                ddiscene_object_pose_method_dispatch(advisorGlobs.anims[loop].cont,
                                                     advisorGlobs.cameraCont,
                                                     0.0f, 0.0f, 1.0f,
                                                     0.0f, 1.0f, 0.0f);
                scene_render_lock_toggle(advisorGlobs.anims[loop].cont, 1);
            }
            advisorGlobs.anims[loop].loopStartTime = (float)(atof(argv[1]) * s_advisor_frames_per_second);
            advisorGlobs.anims[loop].loopEndTime = (float)(atof(argv[2]) * s_advisor_frames_per_second);
        }
    }
}

// FUNCTION: LEGORR 0x004013a0
float vec_len(const float* a)
{
    return (float)sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
}

// FUNCTION: LEGORR 0x004013e0
int advisor_position_name_to_index(const char* name, int* index)
{
    unsigned int loop;

    for (loop = 0; loop < ADVISOR_TYPE_COUNT; loop++) {
        if (_strcmpi(advisorGlobs.positionName[loop], name) == 0) {
            *index = loop;
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00401420
float vec_dist(const float* a, const float* b)
{
    return (float)sqrt((a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]) + (a[2] - b[2]) * (a[2] - b[2]));
}

// FUNCTION: LEGORR 0x00401470
#if defined(_MSC_VER) && defined(_M_IX86)
/* The C body below compiles to the same operations with the operands of the
 * last two products loaded the other way round (68%), in every source form
 * and counter phase tried (finding 550); the matching build keeps the
 * port's instruction-for-instruction body. */
__declspec(naked) float* vec_cross(float* dst, const float* b, const float* c)
{
    __asm {
        mov edx, dword ptr [esp + 8]
        mov ecx, dword ptr [esp + 0x0c]
        mov eax, dword ptr [esp + 4]
        fld dword ptr [edx + 8]
        fld dword ptr [ecx + 8]
        fmul dword ptr [edx + 4]
        fxch st(1)
        fmul dword ptr [ecx + 4]
        fsubp st(1), st
        fstp dword ptr [eax]
        fld dword ptr [ecx + 8]
        fld dword ptr [edx + 8]
        fmul dword ptr [ecx]
        fxch st(1)
        fmul dword ptr [edx]
        fsubp st(1), st
        fstp dword ptr [eax + 4]
        fld dword ptr [edx + 4]
        fld dword ptr [ecx + 4]
        fmul dword ptr [edx]
        fxch st(1)
        fmul dword ptr [ecx]
        fsubp st(1), st
        fstp dword ptr [eax + 8]
        ret
    }
}
#else
float* vec_cross(float* dst, const float* b, const float* c)
{
    dst[0] = b[1] * c[2] - b[2] * c[1];
    dst[1] = b[2] * c[0] - b[0] * c[2];
    dst[2] = b[0] * c[1] - b[1] * c[0];
    return dst;
}
#endif


// FUNCTION: LEGORR 0x004014c0
int advisor_animation_name_to_index(const char* name, int* index)
{
    unsigned int loop;

    for (loop = 0; loop < ADVISOR_ANIM_COUNT; loop++) {
        if (_strcmpi(advisorGlobs.animName[loop], name) == 0) {
            *index = loop;
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00401500
float* vec_add(float* dst, const float* a, const float* b)
{
    dst[0] = a[0] + b[0];
    dst[1] = a[1] + b[1];
    dst[2] = a[2] + b[2];
    return dst;
}

// FUNCTION: LEGORR 0x00401530
void advisor_position_config_load(BlockFile* config, const char* gameName,
                                  int screenWidth, int screenHeight)
{
    BlockRec* item;
    char* argv[10];
    char name[128];
    int advisorType;
    int animType;
    int textType;
    int sfxType;
    int panelType;
    float pos[2];

    // STRING: LEGORR 0x004a13fc
    sprintf(name, "AdvisorPositions%ix%i", screenWidth, screenHeight);
    for (item = block_subgroup(config, string_path_join(gameName, name, 0)); item; item = block_next_equal_key(item)) {
        textType = ADVISOR_TEXT_NONE;
        panelType = ADVISOR_PANEL_NONE;
        if (advisor_position_name_to_index(item->key, &advisorType)) {
            sound_variant_split(item->value, argv, ",");
            if (advisor_animation_name_to_index(argv[0], &animType)) {
                /* "NULL" (004a13f4) is pooled with GAME-032's and GAME-020's
                 * uses; a STRING marker here relabels their operands. */
                if (_strcmpi("NULL", argv[1]) == 0 || ui_message_label_index_find(argv[1], &textType)) {
                    if (sound_alias_lookup(argv[2], &sfxType)) {
                        pos[0] = (float)atof(argv[3]);
                        pos[1] = (float)atof(argv[4]);
                        if (_strcmpi("NULL", argv[5]) == 0 || ui_panel_name_table_lookup(argv[5], &panelType)) {
                            advisor_position_record_initialize(advisorType, animType, textType, sfxType, panelType, pos[0], pos[1]);
                        }
                    }
                }
            }
        }
    }
}

// FUNCTION: LEGORR 0x004016c0
float* vec_sub(float* dst, const float* a, const float* b)
{
    dst[0] = a[0] - b[0];
    dst[1] = a[1] - b[1];
    dst[2] = a[2] - b[2];
    return dst;
}

// FUNCTION: LEGORR 0x004016f0
float vec_dot(const float* a, const float* b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

// FUNCTION: LEGORR 0x00401720
float* vec_scale(float* dst, const float* src, float s)
{
    dst[0] = src[0] * s;
    dst[1] = src[1] * s;
    dst[2] = src[2] * s;
    return dst;
}

#include "advisor_maths.h"

// FUNCTION: LEGORR 0x00401750
void vec_normalize(float* v)
{
    advisor_vector3d_scale(v, v, 1.0f / advisor_vector3d_modulus(v));
}

// FUNCTION: LEGORR 0x004017b0
void advisor_position_record_initialize(int advisorType, int animType,
                                        int textType, int sfxID,
                                        int panelType, float x, float y)
{
    advisorGlobs.positions[advisorType].flags = ADVISOR_FLAG_USED;
    advisorGlobs.positions[advisorType].animType = animType;
    advisorGlobs.positions[advisorType].textType = textType;
    advisorGlobs.positions[advisorType].sfxID = sfxID;
    advisorGlobs.positions[advisorType].x = x;
    advisorGlobs.positions[advisorType].y = y;
    advisorGlobs.positions[advisorType].origX = x;
    advisorGlobs.positions[advisorType].origY = y;
    advisorGlobs.positions[advisorType].panelType = panelType;
    if (panelType == ADVISOR_PANEL_NONE)
        advisorGlobs.positions[advisorType].flags |= ADVISOR_FLAG_NOPANEL;
    if (textType != ADVISOR_TEXT_NONE)
        advisorGlobs.positions[advisorType].flags |= ADVISOR_FLAG_HASTEXT;
}

// FUNCTION: LEGORR 0x00401840
void advisor_action_position_and_mode_set(int advisorType, int panelType,
                                          float x, float y)
{
    advisorGlobs.positions[advisorType].x = x;
    advisorGlobs.positions[advisorType].y = y;
    advisorGlobs.positions[advisorType].panelType = panelType;
    if (panelType == ADVISOR_PANEL_NONE)
        advisorGlobs.positions[advisorType].flags |= ADVISOR_FLAG_NOPANEL;
    else
        advisorGlobs.positions[advisorType].flags &= ~ADVISOR_FLAG_NOPANEL;
}

// FUNCTION: LEGORR 0x00401890
void advisor_action_position_get(int advisorType, float* x, float* y)
{
    *x = advisorGlobs.positions[advisorType].origX;
    *y = advisorGlobs.positions[advisorType].origY;
}

// FUNCTION: LEGORR 0x004018c0
int advisor_request_admit(int advisorType, int loop)
{
    if ((advisorGlobs.positions[advisorType].flags & ADVISOR_FLAG_USED) &&
        s_context_mode && !(advisorGlobs.flags & ADVISOR_GLOB_FLAG_ANIMATING)) {
        advisorGlobs.currType = advisorType;
        advisor_request_reset_and_submit_sound(advisorType);
        advisorGlobs.flags |= ADVISOR_GLOB_FLAG_ANIMATING;
        if (loop)
            advisorGlobs.flags |= ADVISOR_GLOB_FLAG_LOOPING;
        else
            advisorGlobs.flags &= ~ADVISOR_GLOB_FLAG_LOOPING;
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x00401930
void advisor_request_reset_and_submit_sound(int advisorType)
{
    void* cont = advisorGlobs.anims[advisorGlobs.positions[advisorType].animType].cont;

    if (cont) {
        scene_render_lock_toggle(cont, 0);
        ddiscene_frame_update_and_callbacks(cont, 0.0f);
        sound_alias_play(advisorGlobs.positions[advisorType].sfxID, 0);
        sound_request_queue_submit(ADVISOR_SFX, 0);
    }
}

// FUNCTION: LEGORR 0x00401990
void advisor_transition_worker(float elapsed)
{
    int advisorType = advisorGlobs.currType;
    void* cont = advisorGlobs.anims[advisorGlobs.positions[advisorType].animType].cont;

    if (cont) {
        if ((advisorGlobs.flags & ADVISOR_GLOB_FLAG_ANIMATING) && s_context_mode) {
            if (advisorGlobs.positions[advisorType].flags & ADVISOR_FLAG_HASTEXT)
                advisor_text_animation_update(advisorGlobs.positions[advisorType].textType, 1, 0);
            scene_render_lock_toggle(cont, 0);
            scene_render_lock_toggle(advisorGlobs.lightCont, 0);
            advisor_companion_transform_apply(advisorGlobs.lightCont, (void**)cont);
            advisor_apply_slot_transform(advisorType);
            if (advisor_scene_advance_completion_check(advisorGlobs.positions[advisorType].animType, elapsed))
                return;
            advisorGlobs.flags &= ~ADVISOR_GLOB_FLAG_ANIMATING;
        }
        scene_render_lock_toggle(cont, 1);
    }
    scene_render_lock_toggle(advisorGlobs.lightCont, 1);
}


// FUNCTION: LEGORR 0x00401a70
int advisor_scene_advance_completion_check(int animType, float elapsed)
{
    void* cont = advisorGlobs.anims[animType].cont;
    float overrun = ddiscene_advance_by_time_target(cont, elapsed);
    float time;

    if (advisorGlobs.flags & ADVISOR_GLOB_FLAG_LOOPING) {
        if (overrun > 0.0f) {
            ddiscene_frame_update_and_callbacks(cont, advisorGlobs.anims[animType].loopStartTime + overrun);
            return 1;
        }
        time = ddiscene_object_length_get(cont);
        if (time >= advisorGlobs.anims[animType].loopEndTime) {
            ddiscene_frame_update_and_callbacks(cont, (time - advisorGlobs.anims[animType].loopEndTime) + advisorGlobs.anims[animType].loopStartTime);
            return 1;
        }
    } else if (overrun > 0.0f) {
        return 0;
    }
    return 1;
}

// FUNCTION: LEGORR 0x00401b20
void advisor_clear_active_state(void)
{
    advisorGlobs.flags &= ~ADVISOR_GLOB_FLAG_LOOPING;
}

// FUNCTION: LEGORR 0x00401b30
void advisor_apply_slot_transform(int advisorType)
{
    void* cont = advisorGlobs.anims[advisorGlobs.positions[advisorType].animType].cont;
    float panelX, panelY;
    float screenPt[4];
    float wPos[3];

    if (advisorGlobs.positions[advisorType].flags & ADVISOR_FLAG_NOPANEL) {
        panelX = 0.0f;
        panelY = 0.0f;
    } else {
        indexed_pair_get(advisorGlobs.positions[advisorType].panelType, &panelX, &panelY);
    }
    screenPt[0] = advisorGlobs.positions[advisorType].x + panelX;
    screenPt[1] = advisorGlobs.positions[advisorType].y + panelY;
    screenPt[2] = advisorGlobs.scalar;
    screenPt[3] = 1.0f;
    camera_screen_coordinate_project((const ViewProjectionAdapter*)advisorGlobs.viewMain, wPos, screenPt);
    ddiscene_object_position_set(cont, 0, wPos[0], wPos[1], wPos[2]);
}


// FUNCTION: LEGORR 0x00401bf0
int engine_app_mode_get(void)
{
    return s_application_mode;
}

/* 0x00401c00 (Main_GetStartLevel, a Main.h inline) is not written: it
 * returns Main.c's start-level buffer (0x00505f8c), which the port keeps
 * only in the Win32 entry point (platform/winapi/entry.cpp). */

// FUNCTION: LEGORR 0x00401c20
uint32_t global_flag_bit0_is_set(void)
{
    return advisorGlobs.flags & ADVISOR_GLOB_FLAG_ANIMATING;
}

// FUNCTION: LEGORR 0x00401c30
int display_width_get(void)
{
    return s_display_w;
}

// FUNCTION: LEGORR 0x00401c40
int display_height_get(void)
{
    return s_display_h;
}

/* The two Font.h inlines keep the port's text contract: its renderer
 * (formatted_text_format_draw, G98-008) draws into a software target
 * rather than taking a position and a va_list. */
// FUNCTION: LEGORR 0x00401c50
uint32_t formatted_text_printf_width(const struct TextureSurface* font,
                                     const char* text, ...)
{
    uint32_t width;
    va_list args;

    width = 0;
    va_start(args, text);
    formatted_text_format_draw(font, &width, 0, 0, 0, 0, 0, 0, 0, text, args);
    va_end(args);
    return width;
}

// FUNCTION: LEGORR 0x00401c80
void formatted_text_printf(const struct TextureSurface* font,
                           uint32_t* width_out, int* line_count,
                           void* px, int pitch, int w, int h,
                           int x, int y, const char* format, ...)
{
    va_list args;

    va_start(args, format);
    formatted_text_format_draw(font, width_out, line_count, px, pitch, w, h,
                               x, y, format, args);
    va_end(args);
}

#if !(defined(_MSC_VER) && _MSC_VER == 1100)
/* Port-only test accessors, compiled out of the matching build (ADR 0006). */
uint32_t advisor_state_word_peek(void)
{
    return advisorGlobs.flags;
}

void advisor_state_word_set(uint32_t v)
{
    advisorGlobs.flags = v;
}

uint32_t advisor_action_flags_peek(int row)
{
    return advisorGlobs.positions[row].flags;
}
#endif
