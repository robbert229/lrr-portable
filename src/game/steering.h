// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-016 (original translation unit GAME-016, finding 505): steered
 * groups. A group is a stack of 0xa0-byte items, each of which turns and
 * moves towards its own target, plus one leader item whose position the
 * others wander around.
 *
 * C-compatible header (ADR 0005). steering.c is compiled as C in the matching
 * build and as C++ in the native port. Shared C++ headers do not include this
 * file: every include and declaration a header adds shifts MSVC's symbol
 * numbering (finding 506). src/game/test_c_boundaries.cpp includes it beside
 * the C++ headers that still name the unit's callees.
 *
 * The offsets are the original's Win32 layout. The native port widens the
 * two pointers at the end of an item and the three at the start of a group;
 * nothing outside the unit addresses them by offset. */
#ifndef LEGORRR_GAME_STEERING_H
#define LEGORRR_GAME_STEERING_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SteeringVector3 {
    float x, y, z;
} SteeringVector3;

/* One steered item (0xa0 bytes). Each tuning value has a current copy
 * (+0x38..+0x44) and a base copy (+0x48..+0x54) that the randomiser
 * spreads the current one around. */
typedef struct SteeringItem {
    SteeringVector3 target;     /* +0x00 where the item heads */
    SteeringVector3 position;   /* +0x0c */
    SteeringVector3 heading;    /* +0x18 unit forward axis (transform row 2) */
    float yaw;                  /* +0x24 rotation about y */
    float pitch;                /* +0x28 rotation about x */
    float roll;                 /* +0x2c rotation about z */
    float turnRate;             /* +0x30 yaw change per step */
    float wanderTime;           /* +0x34 */
    float wanderPeriod;         /* +0x38 time between new wander targets */
    float turnStep;             /* +0x3c pitch step and turn gain */
    float speed;                /* +0x40 distance per unit of time */
    float wanderRadius;         /* +0x44 spread of the wander targets */
    float baseWanderPeriod;     /* +0x48 */
    float baseTurnStep;         /* +0x4c */
    float baseSpeed;            /* +0x50 */
    float baseWanderRadius;     /* +0x54 */
    float transform[16];        /* +0x58 row-major; row 3 is the position */
    void* container;            /* +0x98 the scene object it drives */
    struct SteeringItem* next;  /* +0x9c the stack link */
} SteeringItem;

/* A group (0x28 bytes): the leader and the stack of followers. */
typedef struct SteeringGroup {
    SteeringItem* leader;       /* +0x00 */
    SteeringItem* head;         /* +0x04 */
    unsigned int count;         /* +0x08 */
    int hasPosition;            /* +0x0c the creator passed a position */
    int hasTarget;              /* +0x10 the creator passed a target */
    unsigned int reserved[5];   /* +0x14 never written by the unit */
} SteeringGroup;

/* The per-item callback of ptrstack_for_each: (group, item, data). */
typedef void (*SteeringItemCallback)(SteeringGroup* group, SteeringItem* item,
                                     void* data);

/* FUN_0040f6c0: a new item at position with target (NULL is the origin),
 * the four tuning values (current and base) and its container. */
SteeringItem* scene_node_record_create(const SteeringVector3* position,
                                       const SteeringVector3* target,
                                       float turnStep, float speed,
                                       float wanderRadius, float wanderPeriod,
                                       void* container);
/* FUN_0040f7b0: free an item, or nothing for NULL. */
void optional_resource_destroy(void* item);
/* FUN_0040f7d0: a new group whose leader is a new item (period 0). */
SteeringGroup* callback_state_record_create(const SteeringVector3* position,
                                            const SteeringVector3* target,
                                            float turnStep, float speed,
                                            float wanderRadius,
                                            void* container);
/* FUN_0040f850: free every follower, the leader and the group. */
void callback_state_list_dispose(SteeringGroup* group);
/* FUN_0040f890: nudge the pitch towards the direction's height. */
void direction_driven_heading_angle_step(SteeringItem* item,
                                         const SteeringVector3* direction);
/* FUN_0040f920: turn the heading towards direction (overwritten). */
void kind_two_steering_torque_apply(SteeringItem* item,
                                    SteeringVector3* direction);
/* FUN_0040fb00: transform = rotations from yaw, pitch and roll. */
void matrix_rotation_compose_xyz(SteeringItem* item);
/* FUN_0040fb90: one steering step of *(float*)data time units. */
void steering_physics_step(SteeringGroup* group, SteeringItem* item,
                           void* data);
/* FUN_0040fcc0: a random direction scaled per axis. */
void vector_scale_components_in_place(SteeringVector3* v, float sx, float sy,
                                      float sz);
/* FUN_0040fdb0: the leader's target, and flag the group as targeted. */
void three_value_record_copy_and_mark_ready(SteeringGroup* group,
                                            const SteeringVector3* target);
/* FUN_0040fde0: step one item outside a group. */
void set_vector_and_notify_listeners(SteeringItem* item, float elapsed);
/* FUN_0040fe00: step the leader and every follower, then wander. */
void scene_object_list_update_transforms(SteeringGroup* group, float elapsed);
/* FUN_0040fe40, FUN_0040fe60, FUN_0040fe90: the follower stack. */
void ptrstack_push(SteeringGroup* group, SteeringItem* item);
SteeringItem* ptrstack_pop(SteeringGroup* group);
void ptrstack_for_each(SteeringGroup* group, SteeringItemCallback callback,
                       void* data);
/* FUN_0040fec0: a random value in [0, 1]. */
float random_unit_float(void);
/* FUN_0040ff50: re-spread every follower's tuning by 1 / spread. */
void scene_object_list_apply_float_callback(SteeringGroup* group,
                                            float spread);
/* FUN_0040ff70: the leader's turn step, speed and wander radius. */
void object_vector3_set(SteeringGroup* group, float turnStep, float speed,
                        float wanderRadius);
/* FUN_0040ffe0: every follower's tuning, current and base. */
void global_vector_set_and_notify(SteeringGroup* group, float turnStep,
                                  float speed, float wanderRadius,
                                  float wanderPeriod);
/* FUN_00410020: the leader's position, and flag the group as placed. */
void scene_node_position_set(SteeringGroup* group,
                             const SteeringVector3* position);
/* FUN_00410070, FUN_004100b0: every follower's position or target. */
void scene_object_list_apply_position_callback(SteeringGroup* group,
                                              const SteeringVector3* position);
void scene_object_list_apply_second_position_callback(
    SteeringGroup* group, const SteeringVector3* target);
/* FUN_004100d0: a ptrstack_for_each callback (GAME-023's timed dispatch)
 * that steps an item and moves it straight at its old target, arriving
 * when it is within one step. */
void steering_item_follow_target(SteeringGroup* group, SteeringItem* item,
                                 void* data);
/* FUN_00410280: 1 when every follower is at its target. */
int callback_operation_run_and_return_status(SteeringGroup* group);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_STEERING_H */
