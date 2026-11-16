// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-001 AITask.c's command record, shared with the units that read an
 * object's task chain (LegoObject.c, Bubbles.c). The +0x... comments are
 * the Win32 offsets; the pointer fields hold real pointers (ADR 0007
 * decision 4). */
#ifndef LEGORRR_GAME_AITASK_RECORD_H
#define LEGORRR_GAME_AITASK_RECORD_H

#ifdef __cplusplus
extern "C" {
#endif

struct Lego_Object;

/* The original command record (AITask, 0x68 bytes; finding 10, finding 570).
 * This is the unit's own declaration of the original layout. The port's
 * CmdRec (cmd_for_aitask.hpp, a frozen header) keeps its port-only tail and
 * is used only by the port tail aitask_port.cpp. Unlike the port's CmdRec
 * before finding 570, there is one link: +0x60 serves both the entity chain
 * (entity +0x2f0) and the two global lists (0x4b3354, 0x4b3358). */
typedef struct AITask AITask;
struct AITask {
    int type;                   /* +0x00 task type (0 goto, 2 target, 3 task, ...) */
    int param;                  /* +0x04 */
    int blockX;                 /* +0x08 cell x (raw word; floats for some types) */
    int blockY;                 /* +0x0c cell y */
    struct Lego_Object* object;  /* +0x10 payload object / target */
    float time;                 /* +0x14 timeGetTime on alloc, 1500.0f on link */
    float timeIn;               /* +0x18 timer (task parameter) */
    int priorityValue;          /* +0x1c behaviour value = behaviour[priority] */
    int field_20;               /* +0x20 cell-command action id */
    int field_24;               /* +0x24 cell-command referenced entity */
    int field_28;               /* +0x28 cell-command entry count */
    int zone;                   /* +0x2c zone-command id */
    float field_30;             /* +0x30 random offset in [0.4, 0.6] (type 7) */
    float field_34;             /* +0x34 random offset in [0.4, 0.6] (type 7) */
    void* field_38;             /* +0x38 raw word / payload */
    int field_3c;               /* +0x3c task-kind selector (type 8 sets 8) */
    void* buffer;               /* +0x40 owned buffer (freed by the pool free) */
    unsigned int count;         /* +0x44 the buffer's entry count */
    void* entityRef;            /* +0x48 pending entity reference */
    int priority;               /* +0x4c priority (kind) index */
    unsigned int field_50;      /* +0x50 timeGetTime() at alloc; unsigned sort key of 00403f80 */
    void* eventPayload;         /* +0x54 event payload */
    AITask* ref;                /* +0x58 referenced record (type 0x13 pair) */
    unsigned int flags;         /* +0x5c state flags */
    AITask* next;               /* +0x60 entity-chain and global-list link */
    AITask* nextFree;           /* +0x64 pool free-list link */
};

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_AITASK_RECORD_H */
