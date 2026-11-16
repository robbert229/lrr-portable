// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-016 Steering's (flocks') records, shared with LegoObject.c, which
 * keeps a flock per object (LegoObject +0x32c). The +0x... comments are the
 * Win32 offsets; the pointer fields hold real pointers (ADR 0007 decision
 * 4). */
#ifndef LEGORRR_GAME_STEERING_RECORD_H
#define LEGORRR_GAME_STEERING_RECORD_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SteeringVector3 {
    float x, y, z;
} SteeringVector3;

/* One steered item (0xa0 bytes on Win32). Each tuning value has a current copy
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

/* A group (0x28 bytes on Win32), LegoObject +0x32c's flock: the leader and the stack of followers. */
typedef struct SteeringGroup {
    SteeringItem* leader;       /* +0x00 */
    SteeringItem* head;         /* +0x04 */
    unsigned int count;         /* +0x08 */
    int hasPosition;            /* +0x0c the creator passed a position */
    int hasTarget;              /* +0x10 the creator passed a target */
    /* +0x14-+0x28: never written by the unit; LegoObject.c keeps a
     * flock's attack state and a bat's home position here. */
    float attackTime;           /* +0x14 */
    float x;                    /* +0x18 the bat's home position */
    float f_1c;                 /* +0x1c */
    float y;                    /* +0x20 */
    unsigned int flags;         /* +0x24 bit 0: attacking */
} SteeringGroup;

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_STEERING_RECORD_H */
