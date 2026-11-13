// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-027 Message.c (original translation unit, finding 505), rebuilt in
 * original shape (ADR 0005, finding 631): the two-bank message queue, the
 * debug key actions and the selected-unit list.
 *
 * C-compatible header (ADR 0005). message_unit.c is compiled as C in the
 * matching build and as C++ in the native port. Shared C++ headers do not
 * include this file: every include and declaration a header adds shifts
 * MSVC's symbol numbering (finding 506). src/game/test_c_boundaries.cpp
 * checks it against the C++ declarations of the same functions.
 *
 * Natively the port's message code (message_port.cpp, cmd.cpp,
 * select.cpp) stays the live one and nothing calls the unit; the records
 * below are the original's Win32 layout. */
#ifndef LEGORRR_GAME_MESSAGE_UNIT_H
#define LEGORRR_GAME_MESSAGE_UNIT_H

#ifdef __cplusplus
extern "C" {
#endif

struct Lego_Object;

#define MESSAGE_MAXEVENTS 2048
#define MESSAGE_MAXSELECTED 100
#define MESSAGE_MAXHOTKEYS 10
#define MESSAGE_TYPE_COUNT 65

/* Message_Type, the event types the queue dispatches. */
#define MESSAGE_SELECT                   1
#define MESSAGE_CLEARSELECTION           3
#define MESSAGE_DESELECT                 4
#define MESSAGE_GOTO                     5
#define MESSAGE_ROCKMONSTERGOTO          6
#define MESSAGE_USERGOTO                 8
#define MESSAGE_FIRSTPERSON              9
#define MESSAGE_TRACKOBJECT              10
#define MESSAGE_TOPVIEW                  11
#define MESSAGE_PLAYSAMPLE               12
#define MESSAGE_ROCKFALL                 19
#define MESSAGE_GENERATECRYSTAL          21
#define MESSAGE_COLLECTCRYSTAL           23
#define MESSAGE_CRYSTALTOREFINERY        25
#define MESSAGE_GENERATEORE              27
#define MESSAGE_COLLECTORE               29
#define MESSAGE_GENERATEROCKMONSTER      31
#define MESSAGE_GATHERROCK               33
#define MESSAGE_PICKRANDOMFLOOR          35
#define MESSAGE_PICKRANDOMFLOORCOMPLETE  36
#define MESSAGE_ATTACKBUILDING           37
#define MESSAGE_FOLLOWATTACK             43
#define MESSAGE_REDUCESELECTION          45
#define MESSAGE_GENERATECRYSTALANDORE    52
#define MESSAGE_GENERATEFROMCRYORE       54
#define MESSAGE_DEBUG_DESTROYALL         64

/* One queued event (0x14 bytes). */
typedef struct Message_Event {
    unsigned int type;              /* +0x00 Message_Type */
    struct Lego_Object* argument1;  /* +0x04 */
    unsigned int argument2;         /* +0x08 */
    int blockPos[2];                /* +0x0c */
} Message_Event;

/* 00451cd0 */
void message_initialise(void);
/* 00451f60 */
void message_remove_events_with_object(struct Lego_Object* obj);
/* 00451fd0 */
int message_lookup_physical_name(const char* name);
/* 00452010 */
void message_add_message_action(unsigned char key, unsigned int type,
                                struct Lego_Object* argument1,
                                unsigned int argument2, const int* blockPos);
/* 00452060 */
void message_post_event(unsigned int type, struct Lego_Object* argument1,
                        unsigned int argument2, const int* blockPos);
/* 004520d0 */
void message_update(void);
/* 00452430 */
void message_pick_random_floor(struct Lego_Object* obj);
/* 004524b0 */
void message_remove_object_reference(struct Lego_Object* deadObj);
/* 00452520 */
int message_copy_selected_units(struct Lego_Object*** outList,
                                unsigned int* outCount);
/* 00452580 */
struct Lego_Object** message_get_selected_units2(unsigned int* outCount);
/* 004525b0 */
int message_any_unit_selected(void);
/* 004525c0 */
struct Lego_Object* message_get_primary_selected_unit(void);
/* 004525e0 */
struct Lego_Object** message_get_selected_units(void);
/* 004525f0 */
unsigned int message_get_num_selected_units(void);
/* 00452600 */
void message_cleanup_selected_units_count(void);
/* 00452610 */
unsigned int message_is_unit_selected(struct Lego_Object* obj,
                                      unsigned int* outIndex);
/* 00452650 */
int message_find_index_of_object(struct Lego_Object** list,
                                 unsigned int count, struct Lego_Object* obj,
                                 unsigned int* outIndex);
/* 00452690 */
int message_is_selected_or_flags4_2(struct Lego_Object* obj);
/* 004526c0 */
void message_select_object(struct Lego_Object* obj, int noDoubleSelect);
/* 004526e0 */
int message_select_object2(struct Lego_Object* obj, int noDoubleSelect,
                           int noFaceCamera);
/* 00452870 */
int message_can_select(struct Lego_Object* obj);
/* 004528c0 */
void message_reduce_selected_units(void);
/* 00452be0 */
void message_clear_selected_units(void);
/* 00452c50 */
int message_deselect_object(struct Lego_Object* obj);
/* 00452cc0 */
unsigned int message_kill_all_selected_units(void);
/* 00452d60 */
int message_enter_first_person_view(unsigned int cameraFrame);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_MESSAGE_UNIT_H */
