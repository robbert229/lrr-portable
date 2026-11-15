// SPDX-License-Identifier: LGPL-3.0-or-later
/* AITask.c's (GAME-002) .bss, 004b31c8-004b8064: one struct, laid out as
 * the original. aitask.c defines it; the port tail (aitask_port.cpp) reads
 * the shared priority values (004b32e8) and states (004b7ff4) through it
 * (finding 570): priorities.c (GAME-034) sets them through this unit's
 * accessors, and the port's scheduler reads them.
 *
 * aitask.c includes this header where it declared the struct before, so its
 * symbol numbering is unchanged (finding 538). The tail's two accessors live
 * in aitask_port.cpp, so the unit ends at its last original function
 * (finding 675). */
#ifndef LEGORRR_GAME_AITASK_GLOBS_H
#define LEGORRR_GAME_AITASK_GLOBS_H

#ifdef __cplusplus
extern "C" {
#endif

struct AITask;
struct EntitySlot;

typedef struct AITask_Globs {
    struct AITask* poolBlocks[12];      /* 004b31c8 CmdPoolGrow block table */
    struct AITask* freeList;            /* 004b31f8 */
    unsigned int poolBlockCount;        /* 004b31fc */
    const char* taskNames[31];          /* 004b3200 AITask_Type_* */
    const char* priorityNames[27];      /* 004b327c AI_Priority_* */
    int priorityValues[27];             /* 004b32e8 */
    struct AITask* globalHead;          /* 004b3354 the head (pending) list */
    struct AITask* globalTail;          /* 004b3358 the second (timer) list */
    struct EntitySlot* assignCandidates[50]; /* 004b335c */
    unsigned int assignCandidateCount;  /* 004b3424 */
    struct EntitySlot* timerCandidates[50];  /* 004b3428 */
    unsigned int timerCandidateCount;   /* 004b34f0 */
    unsigned int commandCounts[4800];   /* 004b34f4 */
    int priorityState[27];              /* 004b7ff4 */
    unsigned int flags;                 /* 004b8060 */
    unsigned int unknown_4e9c[620];     /* 004b8064-004b8a14: never read */
} AITask_Globs;  /* 0x584c bytes */

extern AITask_Globs aiTaskGlobs;  /* defined in aitask.c */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_AITASK_GLOBS_H */
