// SPDX-License-Identifier: LGPL-3.0-or-later
/* AITask.c, original translation unit GAME-002 (finding 505), rebuilt in
 * original shape (ADR 0005, finding 570): text 00401cb0-004065f0, .rdata
 * 0049e014-0049e02c, literals 004a1414-004a1918, .bss 004b31c8-004b8064.
 *
 * The AI task (command record) pool, the two global task lists, the AITask
 * type and priority name tables, and the per-object task chains. The record
 * is AITask (aitask.h), with one +0x60 link shared by the object chains and
 * the global lists.
 *
 * Compiled as C in the matching build and as C++ natively. The port keeps
 * its own command-record implementation, unannotated, in the port tail
 * aitask_port.cpp; native callers use that. Seven functions keep their port
 * names and serve both (the priority accessors, the engine memory wrappers
 * and the two renderer getters). */
#include "legoobject_port_aliases.h"
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "crt/vc5names.h"
#include "lego_unit.h"
#include "aitask.h"
#include "aitask_imports.h"

/* AITask.c's .bss, 004b31c8-004b8064: one struct, laid out as the
 * original. Its priority values (004b32e8) and states (004b7ff4) are shared
 * live with the port tail, which reads them through the port-only accessors
 * at the end of this file (finding 570): priorities.c (GAME-034) sets them
 * through this unit's accessors, and the port's scheduler reads them. */
typedef struct AITask_Globs {
    AITask* poolBlocks[12];             /* 004b31c8 CmdPoolGrow block table */
    AITask* freeList;                   /* 004b31f8 */
    unsigned int poolBlockCount;        /* 004b31fc */
    const char* taskNames[31];          /* 004b3200 AITask_Type_* */
    const char* priorityNames[27];      /* 004b327c AI_Priority_* */
    int priorityValues[27];             /* 004b32e8 */
    AITask* globalHead;                 /* 004b3354 the head (pending) list */
    AITask* globalTail;                 /* 004b3358 the second (timer) list */
    struct EntitySlot* assignCandidates[50]; /* 004b335c */
    unsigned int assignCandidateCount;  /* 004b3424 */
    struct EntitySlot* timerCandidates[50];  /* 004b3428 */
    unsigned int timerCandidateCount;   /* 004b34f0 */
    unsigned int commandCounts[4800];   /* 004b34f4 */
    int priorityState[27];              /* 004b7ff4 */
    unsigned int flags;                 /* 004b8060 */
} AITask_Globs;  /* 0x4e9c bytes */

/* x87 constants. MSVC pools float literals itself, so a literal in the code
 * may replace an item (priorities.c does); an annotated static (config.c)
 * binds the address explicitly. Keep whichever matches. */
// GLOBAL: LEGORR 0x0049e014
static const float s_rdata_0049e014 = 0.0f;
// GLOBAL: LEGORR 0x0049e018
static const float s_rdata_0049e018 = 100.0f;
// GLOBAL: LEGORR 0x0049e01c
static const float s_rdata_0049e01c = 130.0f;
// GLOBAL: LEGORR 0x0049e020
static const float s_rdata_0049e020 = 6.0f;
// GLOBAL: LEGORR 0x0049e024
static const float s_rdata_0049e024 = -6.0f;
// GLOBAL: LEGORR 0x0049e028
static const float s_rdata_0049e028 = 250.0f;
// GLOBAL: LEGORR 0x0049e02c
static const float s_rdata_0049e02c = 0.00999999978f;

// GLOBAL: LEGORR 0x004b31c8
static AITask_Globs aiTaskGlobs;

/* String literals. Annotate each where the code uses it, as
 * `// STRING: LEGORR 0x...` on the line before the literal (utils.c),
 * one literal per marked line. The matching build pools literals the way
 * the original did (finding 569), so mark each address once in the whole
 * tree: Reccmp drops a second marker for it. Mark this unit's own
 * literals here, even when other units reuse them. Leave the ones pooled
 * with another unit unmarked; that unit's conversion marks them. */
/*   STRING: LEGORR 0x004a1414  "AI_Priority_AttackObject" */
/*   STRING: LEGORR 0x004a1430  "AI_Priority_Congregate" */
/*   STRING: LEGORR 0x004a1448  "AI_Priority_AttackPath" */
/*   STRING: LEGORR 0x004a1460  "AI_Priority_Depart" */
/*   STRING: LEGORR 0x004a1474  "AI_Priority_Punch" */
/*   STRING: LEGORR 0x004a1488  "AI_Priority_Steal" */
/*   STRING: LEGORR 0x004a149c  "AI_Priority_Gather" */
/*   STRING: LEGORR 0x004a14b0  "AI_Priority_UpgradeBuilding" */
/*   STRING: LEGORR 0x004a14cc  "AI_Priority_Recharge" */
/*   STRING: LEGORR 0x004a14e4  "AI_Priority_FindLoad" */
/*   STRING: LEGORR 0x004a14fc  "AI_Priority_Barrier" */
/*   STRING: LEGORR 0x004a1510  "AI_Priority_AttackRockMonster" */
/*   STRING: LEGORR 0x004a1530  "AI_Priority_BuildPath" */
/*   STRING: LEGORR 0x004a1548  "AI_Priority_Upgrade" */
/*   STRING: LEGORR 0x004a155c  "AI_Priority_GetIn" */
/*   STRING: LEGORR 0x004a1570  "AI_Priority_HealthLow" */
/*   STRING: LEGORR 0x004a1588  "AI_Priority_Refining" */
/*   STRING: LEGORR 0x004a15a0  "AI_Priority_Storage" */
/*   STRING: LEGORR 0x004a15b4  "AI_Priority_Clearing" */
/*   STRING: LEGORR 0x004a15cc  "AI_Priority_Repair" */
/*   STRING: LEGORR 0x004a15e0  "AI_Priority_Reinforce" */
/*   STRING: LEGORR 0x004a15f8  "AI_Priority_Request" */
/*   STRING: LEGORR 0x004a160c  "AI_Priority_Construction" */
/*   STRING: LEGORR 0x004a1628  "AI_Priority_Destruction" */
/*   STRING: LEGORR 0x004a1640  "AI_Priority_DefaultCollect" */
/*   STRING: LEGORR 0x004a165c  "AI_Priority_Ore" */
/*   STRING: LEGORR 0x004a166c  "AI_Priority_Crystal" */
/*   STRING: LEGORR 0x004a1680  "AITask_Type_FindLoad" */
/*   STRING: LEGORR 0x004a1698  "AITask_Type_AttackObject" */
/*   STRING: LEGORR 0x004a16b4  "AITask_Type_Dock" */
/*   STRING: LEGORR 0x004a16c8  "AITask_Type_Recharge" */
/*   STRING: LEGORR 0x004a16e0  "AITask_Type_AttackRockMonster" */
/*   STRING: LEGORR 0x004a1700  "AITask_Type_AttackPath" */
/*   STRING: LEGORR 0x004a1718  "AITask_Type_Depart" */
/*   STRING: LEGORR 0x004a172c  "AITask_Type_Train" */
/*   STRING: LEGORR 0x004a1740  "AITask_Type_BuildPath" */
/*   STRING: LEGORR 0x004a1758  "AITask_Type_Upgrade" */
/*   STRING: LEGORR 0x004a176c  "AITask_Type_BirdScarer" */
/*   STRING: LEGORR 0x004a1784  "AITask_Type_GetTool" */
/*   STRING: LEGORR 0x004a1798  "AITask_Type_FindDriver" */
/*   ... and 18 more up to 0x004a1918 (strings in LegoRR.exe; the pseudocode shows the ones the code uses) */

/* 0x00401cb0 cmd_kind_by_name (corpus name CmdKindByName), 64 bytes.
 * Current definition: src/game/aitask.cpp:284
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * undefined4 FUN_00401cb0(char *param_1,int *param_2)
 *
 * {
 *   int iVar1;
 *   undefined4 *puVar2;
 *   int iVar3;
 *
 *   iVar3 = 0;
 *   puVar2 = &DAT_004b327c;
 *   do {
 *     iVar1 = __strcmpi(param_1,(char *)*puVar2);
 *     if (iVar1 == 0) {
 *       *param_2 = iVar3;
 *       return 1;
 *     }
 *     puVar2 = puVar2 + 1;
 *     iVar3 = iVar3 + 1;
 *   } while (puVar2 < &DAT_004b32e8);
 *   return 0;
 * }
 */
// FUNCTION: LEGORR 0x00401cb0
int aitask_cmd_kind_by_name(const char* name, int* priority)
{
    const char** entry;
    int i;

    i = 0;
    for (entry = aiTaskGlobs.priorityNames;
         entry < aiTaskGlobs.priorityNames + 27; entry++, i++) {
        if (_strcmpi(name, *entry) == 0) {
            *priority = i;
            return 1;
        }
    }
    return 0;
}

/* 0x00401cf0 cmd_registry_init_target (corpus name CmdRegistryInit), 784 bytes.
 * Current definition: src/game/aitask.cpp:303
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * / * WARNING: Globals starting with '_' overlap smaller symbols at the same address * /
 *
 * void FUN_00401cf0(void)
 *
 * {
 *   int iVar1;
 *   undefined4 *puVar2;
 *
 *   puVar2 = &DAT_004b31c8;
 *   for (iVar1 = 0xc; iVar1 != 0; iVar1 = iVar1 + -1) {
 *     *puVar2 = 0;
 *     puVar2 = puVar2 + 1;
 *   }
 *   _DAT_004b3200 = s_AITask_Type_Goto_004a1918;
 *   _DAT_004b3204 = s_AITask_Type_Follow_004a1904;
 *   _DAT_004b3208 = s_AITask_Type_FollowAttack_004a18e8;
 *   _DAT_004b320c = s_AITask_Type_Collect_004a18d4;
 *   _DAT_004b3210 = s_AITask_Type_Gather_004a18c0;
 *   _DAT_004b3230 = s_AITask_Type_Clear_004a18ac;
 *   _DAT_004b3214 = s_AITask_Type_Deposit_004a1898;
 *   _DAT_004b3218 = s_AITask_Type_Dump_004a1884;
 *   _DAT_004b321c = s_AITask_Type_Request_004a1870;
 *   _DAT_004b3228 = s_AITask_Type_Repair_004a185c;
 *   _DAT_004b3220 = s_AITask_Type_Dig_004a184c;
 *   _DAT_004b3224 = s_AITask_Type_Dynamite_004a1834;
 *   _DAT_004b322c = s_AITask_Type_Reinforce_004a181c;
 *   _DAT_004b3234 = s_AITask_Type_Wait_004a1808;
 *   _DAT_004b3238 = s_AITask_Type_AnimationWait_004a17ec;
 *   _DAT_004b323c = s_AITask_Type_ElecFence_004a17d4;
 *   _DAT_004b3240 = s_AITask_Type_Eat_004a17c4;
 *   _DAT_004b3244 = s_AITask_Type_GotoEat_004a17b0;
 *   _DAT_004b3248 = s_AITask_Type_FindDriver_004a1798;
 *   _DAT_004b324c = s_AITask_Type_GetTool_004a1784;
 *   _DAT_004b3250 = s_AITask_Type_BirdScarer_004a176c;
 *   _DAT_004b3254 = s_AITask_Type_Upgrade_004a1758;
 *   _DAT_004b3258 = s_AITask_Type_BuildPath_004a1740;
 *   _DAT_004b325c = s_AITask_Type_Train_004a172c;
 *   _DAT_004b3260 = s_AITask_Type_Depart_004a1718;
 *   _DAT_004b3264 = s_AITask_Type_AttackPath_004a1700;
 *   _DAT_004b3268 = s_AITask_Type_AttackRockMonster_004a16e0;
 *   _DAT_004b326c = s_AITask_Type_Recharge_004a16c8;
 *   _DAT_004b3270 = s_AITask_Type_Dock_004a16b4;
 *   _DAT_004b3274 = s_AITask_Type_AttackObject_004a1698;
 *   _DAT_004b3278 = s_AITask_Type_FindLoad_004a1680;
 *   DAT_004b327c = s_AI_Priority_Crystal_004a166c;
 *   DAT_004b3280 = s_AI_Priority_Ore_004a165c;
 *   _DAT_004b3284 = s_AI_Priority_DefaultCollect_004a1640;
 *   _DAT_004b3288 = s_AI_Priority_Destruction_004a1628;
 *   _DAT_004b328c = s_AI_Priority_Construction_004a160c;
 *   _DAT_004b3290 = s_AI_Priority_Request_004a15f8;
 *   _DAT_004b3294 = s_AI_Priority_Reinforce_004a15e0;
 *   _DAT_004b3298 = s_AI_Priority_Repair_004a15cc;
 *   _DAT_004b329c = s_AI_Priority_Clearing_004a15b4;
 *   _DAT_004b32a0 = s_AI_Priority_Storage_004a15a0;
 *   _DAT_004b32a4 = s_AI_Priority_Refining_004a1588;
 *   _DAT_004b32a8 = s_AI_Priority_HealthLow_004a1570;
 *   _DAT_004b32ac = s_AI_Priority_GetIn_004a155c;
 *   _DAT_004b32b0 = s_AI_Priority_Upgrade_004a1548;
 *   _DAT_004b32b4 = s_AI_Priority_BuildPath_004a1530;
 *   _DAT_004b32b8 = s_AI_Priority_AttackRockMonster_004a1510;
 *   _DAT_004b32bc = s_AI_Priority_Barrier_004a14fc;
 *   _DAT_004b32c0 = s_AI_Priority_FindLoad_004a14e4;
 *   _DAT_004b32c4 = s_AI_Priority_Recharge_004a14cc;
 *   _DAT_004b32c8 = s_AI_Priority_UpgradeBuilding_004a14b0;
 *   _DAT_004b32cc = s_AI_Priority_Gather_004a149c;
 *   _DAT_004b32d0 = s_AI_Priority_Steal_004a1488;
 *   _DAT_004b32d4 = s_AI_Priority_Punch_004a1474;
 *   _DAT_004b32d8 = s_AI_Priority_Depart_004a1460;
 *   _DAT_004b32dc = s_AI_Priority_AttackPath_004a1448;
 *   _DAT_004b32e4 = s_AI_Priority_Congregate_004a1430;
 *   _DAT_004b32e0 = s_AI_Priority_AttackObject_004a1414;
 *   puVar2 = &DAT_004b32e8;
 *   for (iVar1 = 0x1b; iVar1 != 0; iVar1 = iVar1 + -1) {
 *     *puVar2 = 0x31;
 *     puVar2 = puVar2 + 1;
 *   }
 *   DAT_004b32f8 = 0x3c;
 *   _DAT_004b32f4 = 0x28;
 *   DAT_004b32e8 = 0x37;
 *   _DAT_004b330c = 5;
 *   _DAT_004b3308 = 0x14;
 *   _DAT_004b3310 = 0xf;
 *   _DAT_004b32fc = 0x14;
 *   _DAT_004b3300 = 0x46;
 *   _DAT_004b3324 = 0x38;
 *   _DAT_004b3334 = 99;
 *   _DAT_004b333c = 0x5a;
 *   _DAT_004b3340 = 0x31;
 *   _DAT_004b3338 = 0x31;
 *   _DAT_004b3344 = 0;
 *   _DAT_004b3348 = 0x14;
 *   _DAT_004b3350 = 99;
 *   _DAT_004b334c = 0x1e;
 *   DAT_004b31f8 = 0;
 *   DAT_004b31fc = 0;
 *   DAT_004b8060 = 1;
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00401cf0
void aitask_cmd_registry_init_target(void)
{
    int i;

    for (i = 0; i < 12; i++)
        aiTaskGlobs.poolBlocks[i] = NULL;

    // STRING: LEGORR 0x004a1918
    aiTaskGlobs.taskNames[0] = "AITask_Type_Goto";
    // STRING: LEGORR 0x004a1904
    aiTaskGlobs.taskNames[1] = "AITask_Type_Follow";
    // STRING: LEGORR 0x004a18e8
    aiTaskGlobs.taskNames[2] = "AITask_Type_FollowAttack";
    // STRING: LEGORR 0x004a18d4
    aiTaskGlobs.taskNames[3] = "AITask_Type_Collect";
    // STRING: LEGORR 0x004a18c0
    aiTaskGlobs.taskNames[4] = "AITask_Type_Gather";
    // STRING: LEGORR 0x004a18ac
    aiTaskGlobs.taskNames[12] = "AITask_Type_Clear";
    // STRING: LEGORR 0x004a1898
    aiTaskGlobs.taskNames[5] = "AITask_Type_Deposit";
    // STRING: LEGORR 0x004a1884
    aiTaskGlobs.taskNames[6] = "AITask_Type_Dump";
    // STRING: LEGORR 0x004a1870
    aiTaskGlobs.taskNames[7] = "AITask_Type_Request";
    // STRING: LEGORR 0x004a185c
    aiTaskGlobs.taskNames[10] = "AITask_Type_Repair";
    // STRING: LEGORR 0x004a184c
    aiTaskGlobs.taskNames[8] = "AITask_Type_Dig";
    // STRING: LEGORR 0x004a1834
    aiTaskGlobs.taskNames[9] = "AITask_Type_Dynamite";
    // STRING: LEGORR 0x004a181c
    aiTaskGlobs.taskNames[11] = "AITask_Type_Reinforce";
    // STRING: LEGORR 0x004a1808
    aiTaskGlobs.taskNames[13] = "AITask_Type_Wait";
    // STRING: LEGORR 0x004a17ec
    aiTaskGlobs.taskNames[14] = "AITask_Type_AnimationWait";
    // STRING: LEGORR 0x004a17d4
    aiTaskGlobs.taskNames[15] = "AITask_Type_ElecFence";
    // STRING: LEGORR 0x004a17c4
    aiTaskGlobs.taskNames[16] = "AITask_Type_Eat";
    // STRING: LEGORR 0x004a17b0
    aiTaskGlobs.taskNames[17] = "AITask_Type_GotoEat";
    // STRING: LEGORR 0x004a1798
    aiTaskGlobs.taskNames[18] = "AITask_Type_FindDriver";
    // STRING: LEGORR 0x004a1784
    aiTaskGlobs.taskNames[19] = "AITask_Type_GetTool";
    // STRING: LEGORR 0x004a176c
    aiTaskGlobs.taskNames[20] = "AITask_Type_BirdScarer";
    // STRING: LEGORR 0x004a1758
    aiTaskGlobs.taskNames[21] = "AITask_Type_Upgrade";
    // STRING: LEGORR 0x004a1740
    aiTaskGlobs.taskNames[22] = "AITask_Type_BuildPath";
    // STRING: LEGORR 0x004a172c
    aiTaskGlobs.taskNames[23] = "AITask_Type_Train";
    // STRING: LEGORR 0x004a1718
    aiTaskGlobs.taskNames[24] = "AITask_Type_Depart";
    // STRING: LEGORR 0x004a1700
    aiTaskGlobs.taskNames[25] = "AITask_Type_AttackPath";
    // STRING: LEGORR 0x004a16e0
    aiTaskGlobs.taskNames[26] = "AITask_Type_AttackRockMonster";
    // STRING: LEGORR 0x004a16c8
    aiTaskGlobs.taskNames[27] = "AITask_Type_Recharge";
    // STRING: LEGORR 0x004a16b4
    aiTaskGlobs.taskNames[28] = "AITask_Type_Dock";
    // STRING: LEGORR 0x004a1698
    aiTaskGlobs.taskNames[29] = "AITask_Type_AttackObject";
    // STRING: LEGORR 0x004a1680
    aiTaskGlobs.taskNames[30] = "AITask_Type_FindLoad";
    // STRING: LEGORR 0x004a166c
    aiTaskGlobs.priorityNames[0] = "AI_Priority_Crystal";
    // STRING: LEGORR 0x004a165c
    aiTaskGlobs.priorityNames[1] = "AI_Priority_Ore";
    // STRING: LEGORR 0x004a1640
    aiTaskGlobs.priorityNames[2] = "AI_Priority_DefaultCollect";
    // STRING: LEGORR 0x004a1628
    aiTaskGlobs.priorityNames[3] = "AI_Priority_Destruction";
    // STRING: LEGORR 0x004a160c
    aiTaskGlobs.priorityNames[4] = "AI_Priority_Construction";
    // STRING: LEGORR 0x004a15f8
    aiTaskGlobs.priorityNames[5] = "AI_Priority_Request";
    // STRING: LEGORR 0x004a15e0
    aiTaskGlobs.priorityNames[6] = "AI_Priority_Reinforce";
    // STRING: LEGORR 0x004a15cc
    aiTaskGlobs.priorityNames[7] = "AI_Priority_Repair";
    // STRING: LEGORR 0x004a15b4
    aiTaskGlobs.priorityNames[8] = "AI_Priority_Clearing";
    // STRING: LEGORR 0x004a15a0
    aiTaskGlobs.priorityNames[9] = "AI_Priority_Storage";
    // STRING: LEGORR 0x004a1588
    aiTaskGlobs.priorityNames[10] = "AI_Priority_Refining";
    // STRING: LEGORR 0x004a1570
    aiTaskGlobs.priorityNames[11] = "AI_Priority_HealthLow";
    // STRING: LEGORR 0x004a155c
    aiTaskGlobs.priorityNames[12] = "AI_Priority_GetIn";
    // STRING: LEGORR 0x004a1548
    aiTaskGlobs.priorityNames[13] = "AI_Priority_Upgrade";
    // STRING: LEGORR 0x004a1530
    aiTaskGlobs.priorityNames[14] = "AI_Priority_BuildPath";
    // STRING: LEGORR 0x004a1510
    aiTaskGlobs.priorityNames[15] = "AI_Priority_AttackRockMonster";
    // STRING: LEGORR 0x004a14fc
    aiTaskGlobs.priorityNames[16] = "AI_Priority_Barrier";
    // STRING: LEGORR 0x004a14e4
    aiTaskGlobs.priorityNames[17] = "AI_Priority_FindLoad";
    // STRING: LEGORR 0x004a14cc
    aiTaskGlobs.priorityNames[18] = "AI_Priority_Recharge";
    // STRING: LEGORR 0x004a14b0
    aiTaskGlobs.priorityNames[19] = "AI_Priority_UpgradeBuilding";
    // STRING: LEGORR 0x004a149c
    aiTaskGlobs.priorityNames[20] = "AI_Priority_Gather";
    // STRING: LEGORR 0x004a1488
    aiTaskGlobs.priorityNames[21] = "AI_Priority_Steal";
    // STRING: LEGORR 0x004a1474
    aiTaskGlobs.priorityNames[22] = "AI_Priority_Punch";
    // STRING: LEGORR 0x004a1460
    aiTaskGlobs.priorityNames[23] = "AI_Priority_Depart";
    // STRING: LEGORR 0x004a1448
    aiTaskGlobs.priorityNames[24] = "AI_Priority_AttackPath";
    // STRING: LEGORR 0x004a1430
    aiTaskGlobs.priorityNames[26] = "AI_Priority_Congregate";
    // STRING: LEGORR 0x004a1414
    aiTaskGlobs.priorityNames[25] = "AI_Priority_AttackObject";

    for (i = 0; i < 27; i++)
        aiTaskGlobs.priorityValues[i] = 49;
    aiTaskGlobs.priorityValues[4] = 60;
    aiTaskGlobs.priorityValues[3] = 40;
    aiTaskGlobs.priorityValues[0] = 55;
    aiTaskGlobs.priorityValues[9] = 5;
    aiTaskGlobs.priorityValues[8] = 20;
    aiTaskGlobs.priorityValues[10] = 15;
    aiTaskGlobs.priorityValues[5] = 20;
    aiTaskGlobs.priorityValues[6] = 70;
    aiTaskGlobs.priorityValues[15] = 56;
    aiTaskGlobs.priorityValues[19] = 99;
    aiTaskGlobs.priorityValues[21] = 90;
    aiTaskGlobs.priorityValues[22] = 49;
    aiTaskGlobs.priorityValues[20] = 49;
    aiTaskGlobs.priorityValues[23] = 0;
    aiTaskGlobs.priorityValues[24] = 20;
    aiTaskGlobs.priorityValues[26] = 99;
    aiTaskGlobs.priorityValues[25] = 30;

    aiTaskGlobs.freeList = NULL;
    aiTaskGlobs.poolBlockCount = 0;
    aiTaskGlobs.flags = 1;
}

/* 0x00402000 cmd_mark_matching (corpus name CmdMarkMatching), 96 bytes.
 * Current definition: src/game/aitask.cpp:496
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00402000(int param_1,int param_2,int *param_3)
 *
 * {
 *   int *piVar1;
 *
 *   for (piVar1 = DAT_004b3354; piVar1 != (int *)0x0; piVar1 = (int *)piVar1[0x18]) {
 *     if (*piVar1 == param_1) {
 *       if ((param_2 != 0) && (param_2 == piVar1[4])) {
 *         piVar1[0x17] = piVar1[0x17] | 0x10000;
 *       }
 *       if (((param_3 != (int *)0x0) && (*param_3 == piVar1[2])) && (param_3[1] == piVar1[3])) {
 *         piVar1[0x17] = piVar1[0x17] | 0x10000;
 *       }
 *     }
 *   }
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00402000
void aitask_cmd_mark_matching(int type, struct EntitySlot* object,
                              const int* blockPos)
{
    AITask* task;

    for (task = aiTaskGlobs.globalHead; task != NULL; task = task->next) {
        if (task->type == type) {
            if (object != NULL && object == task->object)
                task->flags |= 0x10000;
            if (blockPos != NULL && blockPos[0] == task->blockX &&
                blockPos[1] == task->blockY)
                task->flags |= 0x10000;
        }
    }
}

/* 0x00402060 cmd_set_kind (corpus name CmdSetKind), 48 bytes.
 * Current definition: src/game/aitask.cpp:518
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00402060(int param_1,int param_2)
 *
 * {
 *   undefined4 uVar1;
 *
 *   if ((*(uint *)(param_1 + 0x5c) & 0x30000) == 0) {
 *     uVar1 = (&DAT_004b32e8)[param_2];
 *     *(int *)(param_1 + 0x4c) = param_2;
 *     *(undefined4 *)(param_1 + 0x1c) = uVar1;
 *     return;
 *   }
 *   *(undefined4 *)(param_1 + 0x1c) = 99;
 *   *(int *)(param_1 + 0x4c) = param_2;
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00402060
void aitask_cmd_set_kind(AITask* task, int priority)
{
    if (!(task->flags & 0x30000)) {
        task->priorityValue = aiTaskGlobs.priorityValues[priority];
    } else {
        task->priorityValue = 99;
    }
    task->priority = priority;
}

/* 0x00402090 priority_entry_order_value_set (corpus name PriorityEntryOrderValueSet), 16 bytes.
 * Current definition: src/game/aitask.cpp:532
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00402090(int param_1,undefined4 param_2)
 *
 * {
 *   (&DAT_004b32e8)[param_1] = param_2;
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00402090
void priority_entry_order_value_set(int kind, int value)
{
    aiTaskGlobs.priorityValues[kind] = value;
}

/* 0x004020a0 priority_entry_state_set (corpus name PriorityEntryStateSet), 16 bytes.
 * Current definition: src/game/aitask.cpp:539
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_004020a0(int param_1,undefined4 param_2)
 *
 * {
 *   (&DAT_004b7ff4)[param_1] = param_2;
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x004020a0
void priority_entry_state_set(int kind, int value)
{
    aiTaskGlobs.priorityState[kind] = value;
}

/* 0x004020b0 priority_entry_state_get (corpus name PriorityEntryStateGet), 16 bytes.
 * Current definition: src/game/aitask.cpp:545
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * undefined4 FUN_004020b0(int param_1)
 *
 * {
 *   return (&DAT_004b7ff4)[param_1];
 * }
 */
// FUNCTION: LEGORR 0x004020b0
int priority_entry_state_get(int kind)
{
    return aiTaskGlobs.priorityState[kind];
}

/* 0x004020c0 handle_array_release_and_reset (corpus name HandleArrayReleaseAndReset), 64 bytes.
 * Current definition: src/game/aitask.cpp:556
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_004020c0(void)
 *
 * {
 *   int *piVar1;
 *
 *   piVar1 = &DAT_004b31c8;
 *   do {
 *     if (*piVar1 != 0) {
 *       FUN_0048db80(*piVar1);
 *     }
 *     piVar1 = piVar1 + 1;
 *   } while (piVar1 < &DAT_004b31f8);
 *   DAT_004b31f8 = 0;
 *   DAT_004b8060 = 0;
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x004020c0
void aitask_handle_array_release_and_reset(void)
{
    unsigned int i;

    for (i = 0; i < 12; i++) {
        if (aiTaskGlobs.poolBlocks[i] != NULL)
            free(aiTaskGlobs.poolBlocks[i]);
    }
    aiTaskGlobs.freeList = NULL;
    aiTaskGlobs.flags = 0;
}

/* 0x00402100 cmd_flush_all (corpus name CmdFlushAll), 112 bytes.
 * Current definition: src/game/aitask.cpp:580
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00402100(int param_1)
 *
 * {
 *   int iVar1;
 *   undefined4 *puVar2;
 *
 *   if (param_1 != 0) {
 *     DAT_004b8060 = DAT_004b8060 & 0xfffffffd;
 *     return;
 *   }
 *   FUN_00406540(&LAB_004021f0,0);
 *   DAT_004b8060 = DAT_004b8060 | 2;
 *   DAT_004b3354 = 0;
 *   DAT_004b3358 = 0;
 *   puVar2 = &DAT_004b34f4;
 *   for (iVar1 = 0x12c0; iVar1 != 0; iVar1 = iVar1 + -1) {
 *     *puVar2 = 0;
 *     puVar2 = puVar2 + 1;
 *   }
 *   puVar2 = &DAT_004b7ff4;
 *   for (iVar1 = 0x1b; iVar1 != 0; iVar1 = iVar1 + -1) {
 *     *puVar2 = 0;
 *     puVar2 = puVar2 + 1;
 *   }
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00402100
void aitask_cmd_flush_all(int disable)
{
    int i;

    if (disable) {
        aiTaskGlobs.flags &= ~2;
    } else {
        aitask_entity_pool_callback_search(aitask_callback_004021f0, NULL);
        aiTaskGlobs.flags |= 2;
        aiTaskGlobs.globalHead = NULL;
        aiTaskGlobs.globalTail = NULL;
        for (i = 0; i < 4800; i++)
            aiTaskGlobs.commandCounts[i] = 0;
        for (i = 0; i < 27; i++)
            aiTaskGlobs.priorityState[i] = 0;
    }
}

/* 0x00402170 level_no_gather_object_setup (corpus name LevelNoGatherObjectSetup), 112 bytes.
 * Current definition: src/game/aitask.cpp:605
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00402170(int param_1)
 *
 * {
 *   int iVar1;
 *
 *   if (param_1 == 0) {
 *     FUN_00402800(0);
 *   }
 *   iVar1 = FUN_00406440(0x18);
 *   FUN_00402060(iVar1,0x17);
 *   *(uint *)(iVar1 + 0x5c) = *(uint *)(iVar1 + 0x5c) | 0x40;
 *   *(int *)(iVar1 + 0x60) = DAT_004b3358;
 *   DAT_004b3358 = iVar1;
 *   iVar1 = FUN_00406440(0x1b);
 *   FUN_00404030(iVar1,0);
 *   *(uint *)(iVar1 + 0x5c) = *(uint *)(iVar1 + 0x5c) & 0xffffffef | 0x440;
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00402170
void aitask_level_no_gather_object_setup(int noGather)
{
    AITask* task;

    if (!noGather)
        aitask_entity_command14_batch_issue(0);

    task = aitask_cmd_alloc(0x18);
    aitask_cmd_set_kind(task, 0x17);
    task->flags |= 0x40;
    task->next = aiTaskGlobs.globalTail;
    aiTaskGlobs.globalTail = task;

    task = aitask_cmd_alloc(0x1b);
    aitask_cmd_link_head(task, 0);
    task->flags &= ~0x10;
    task->flags |= 0x440;
}

/* 0x004021e0 engine_memory_allocate (corpus name EngineMemoryAllocate), 48 bytes.
 * Current definition: src/game/aitask.cpp:625
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_004021e0(undefined4 param_1)
 *
 * {
 *   FUN_0048dbd0(param_1);
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x004021e0
void* engine_memory_allocate(long size)
{
    return malloc(size);
}

/* 0x004021f0: an enumeration callback or entry that Ghidra merged into the
 * preceding function; the unit pushes or calls its address. Read it from
 * the disassembly of reference/LegoRR.exe. */
// FUNCTION: LEGORR 0x004021f0
int aitask_callback_004021f0(AITask* task, void* data)
{
    aitask_cmd_free(task, 1);
    return 0;
}

/* 0x00402210 engine_memory_free (corpus name EngineMemoryFree), 16 bytes.
 * Current definition: src/game/aitask.cpp:635
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00402210(undefined4 param_1)
 *
 * {
 *   FUN_0048db80(param_1);
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00402210
void engine_memory_free(void* ptr)
{
    free(ptr);
}

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

/* 0x00402220 cmd_tick_global_delta (corpus name CmdTickGlobal), 240 bytes.
 * Current definition: src/game/aitask.cpp:644
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * / * WARNING: Globals starting with '_' overlap smaller symbols at the same address * /
 *
 * void FUN_00402220(void)
 *
 * {
 *   int *piVar1;
 *   int iVar2;
 *
 *   iVar2 = DAT_004b32ec;
 *   if (DAT_004b32ec <= DAT_004b32e8) {
 *     iVar2 = DAT_004b32e8;
 *   }
 *   _DAT_004b3328 = DAT_004b32f8;
 *   if ((DAT_004b32f8 < iVar2) && (_DAT_004b3328 = DAT_004b32e8, DAT_004b32e8 < DAT_004b32ec)) {
 *     _DAT_004b3328 = DAT_004b32ec;
 *   }
 *   _DAT_004b3328 = _DAT_004b3328 + 1;
 *   if ((DAT_004b8060 & 0x80000000) == 0) {
 *     FUN_00406540(&LAB_00404250,&stack0x00000004);
 *     for (piVar1 = DAT_004b3354; piVar1 != (int *)0x0; piVar1 = (int *)piVar1[0x18]) {
 *       if (((*piVar1 == 10) && ((piVar1[0x17] & 0x200000U) == 0)) &&
 *          ((*(float *)(piVar1[4] + 0x340) <= 0.0 || (100.0 <= *(float *)(piVar1[4] + 0x340))))) {
 *         piVar1[0x17] = piVar1[0x17] | 0x20;
 *       }
 *     }
 *     FUN_00402310(&DAT_004b3354);
 *     FUN_00402310(&DAT_004b3358);
 *     DAT_004b3424 = 0;
 *     DAT_004b34f0 = 0;
 *     FUN_004377b0(&LAB_00404280,&stack0x00000004);
 *     FUN_00405c10();
 *     FUN_00405950();
 *   }
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00402220
void aitask_cmd_tick_global_delta(float elapsed)
{
    AITask* task;

    aiTaskGlobs.priorityValues[16] = max(max(aiTaskGlobs.priorityValues[1], aiTaskGlobs.priorityValues[0]), aiTaskGlobs.priorityValues[4]) + 1;

    if (!(aiTaskGlobs.flags & 0x80000000)) {
        aitask_entity_pool_callback_search(aitask_cmd_pool_timer_delta_apply, &elapsed);

        for (task = aiTaskGlobs.globalHead; task != NULL; task = task->next) {
            if (task->type == 10 && !(task->flags & 0x200000)) {
                if (LEGO_FIELD(task->object, float, 0x340) <= s_rdata_0049e014 ||
                    LEGO_FIELD(task->object, float, 0x340) >= s_rdata_0049e018)
                    task->flags |= 0x20;
            }
        }

        aitask_cmd_global_list_purge(&aiTaskGlobs.globalHead);
        aitask_cmd_global_list_purge(&aiTaskGlobs.globalTail);
        aiTaskGlobs.assignCandidateCount = 0;
        aiTaskGlobs.timerCandidateCount = 0;
        entity_pool_find_by_callback_filtered_current(
            aitask_cmd_entity_command_chain_tick, &elapsed);
        aitask_pending_command_entity_assignment_tick();
        aitask_pending_command_timer_fire_scan();
    }
}

/* 0x00402310 cmd_global_list_purge (corpus name CmdChainPurge), 512 bytes.
 * Current definition: src/game/aitask.cpp:681
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00402310(int *param_1)
 *
 * {
 *   int *piVar1;
 *   uint uVar2;
 *   bool bVar3;
 *   int *piVar4;
 *   int iVar5;
 *   int iVar6;
 *   int *piVar7;
 *   int iVar8;
 *   int *piVar9;
 *   int *local_4dc;
 *   int local_4d0 [308];
 *
 *   local_4dc = (int *)0x0;
 *   iVar8 = *param_1;
 *   iVar6 = 0;
 *   while (iVar5 = iVar6, iVar6 = iVar8, iVar6 != 0) {
 *     iVar8 = *(int *)(iVar6 + 0x60);
 *     if ((*(byte *)(iVar6 + 0x5c) & 0x20) != 0) {
 *       if (iVar5 == 0) {
 *         *param_1 = iVar8;
 *         FUN_00406480(iVar6,0);
 *         iVar6 = iVar5;
 *       }
 *       else {
 *         *(int *)(iVar5 + 0x60) = iVar8;
 *         FUN_00406480(iVar6,0);
 *         iVar6 = iVar5;
 *       }
 *     }
 *   }
 *   iVar8 = 0;
 *   if ((int *)*param_1 != (int *)0x0) {
 *     piVar4 = local_4d0 + 8;
 *     piVar7 = (int *)*param_1;
 *     do {
 *       piVar1 = (int *)piVar7[0x18];
 *       if (iVar8 == 300) {
 *         iVar8 = 300;
 *         local_4dc = piVar7;
 *         break;
 *       }
 *       *piVar4 = (int)piVar7;
 *       piVar4 = piVar4 + 1;
 *       iVar8 = iVar8 + 1;
 *       if (((&DAT_004b7ff4)[piVar7[0x13]] == 0) || ((piVar7[0x17] & 0x10000U) != 0)) {
 *         uVar2 = piVar7[0x17];
 *         piVar7[0x17] = uVar2 & 0xfff7ffff;
 *         if ((uVar2 & 0x30000) == 0) {
 *           piVar7[7] = (&DAT_004b32e8)[piVar7[0x13]];
 *         }
 *         else {
 *           piVar7[7] = 99;
 *         }
 *       }
 *       else {
 *         piVar7[0x17] = piVar7[0x17] | 0x80000;
 *       }
 *       if (*piVar7 == 8) {
 *         if ((*(byte *)(piVar7 + 0x17) & 8) == 0) {
 *           iVar6 = FUN_00432f40(piVar7[2],piVar7[3]);
 *           if (iVar6 != 0) {
 *             piVar7[7] = 0;
 *           }
 *         }
 *         else {
 *           local_4d0[0] = 0;
 *           local_4d0[1] = 0;
 *           local_4d0[2] = 0;
 *           local_4d0[7] = 0;
 *           local_4d0[3] = 0xffffffff;
 *           local_4d0[4] = 0xffffffff;
 *           local_4d0[5] = 0xffffffff;
 *           local_4d0[6] = 0xffffffff;
 *           bVar3 = false;
 *           piVar9 = local_4d0;
 *           iVar6 = 4;
 *           do {
 *             iVar5 = FUN_00432bd0(*piVar9 + piVar7[2],piVar9[1] + piVar7[3]);
 *             if (iVar5 != 0) {
 *               bVar3 = true;
 *             }
 *             piVar9 = piVar9 + 2;
 *             iVar6 = iVar6 + -1;
 *           } while (iVar6 != 0);
 *           if (bVar3) {
 *             if (piVar7[7] == 0) {
 *               piVar7[6] = 0x42c80000;
 *             }
 *           }
 *           else {
 *             piVar7[7] = 0;
 *           }
 *         }
 *       }
 *       piVar7 = piVar1;
 *     } while (piVar1 != (int *)0x0);
 *   }
 *   if (iVar8 != 0) {
 *     FUN_0048de20(local_4d0 + 8,iVar8,4,&LAB_00403f80);
 *     iVar8 = iVar8 + -1;
 *     iVar6 = 0;
 *     if (iVar8 != 0) {
 *       piVar4 = local_4d0 + 8;
 *       iVar5 = iVar8;
 *       do {
 *         iVar6 = *piVar4;
 *         piVar7 = piVar4 + 1;
 *         piVar4 = piVar4 + 1;
 *         iVar5 = iVar5 + -1;
 *         *(int *)(iVar6 + 0x60) = *piVar7;
 *         iVar6 = iVar8;
 *       } while (iVar5 != 0);
 *     }
 *     *(int **)(local_4d0[iVar6 + 8] + 0x60) = local_4dc;
 *     *param_1 = local_4d0[8];
 *   }
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00402310
void aitask_cmd_global_list_purge(AITask** list)
{
    AITask* task;
    AITask* next;
    AITask* prev;
    AITask* overflow;
    unsigned int count;
    unsigned int i;
    int found;
    int offsets[4][2];
    AITask* sortList[300];

    overflow = NULL;
    prev = NULL;
    for (task = *list; task != NULL; task = next) {
        next = task->next;
        if (task->flags & 0x20) {
            if (prev != NULL)
                prev->next = next;
            else
                *list = next;
            aitask_cmd_free(task, 0);
        } else {
            prev = task;
        }
    }

    count = 0;
    for (task = *list; task != NULL; task = next) {
        next = task->next;
        if (count == 300) {
            overflow = task;
            break;
        }
        sortList[count++] = task;

        if (aiTaskGlobs.priorityState[task->priority] && !(task->flags & 0x10000)) {
            task->flags |= 0x80000;
        } else {
            task->flags &= ~0x80000;
            if (!(task->flags & 0x30000))
                task->priorityValue = aiTaskGlobs.priorityValues[task->priority];
            else
                task->priorityValue = 99;
        }

        if (task->type == 8) {
            if (task->flags & 8) {
                offsets[0][0] = 0;
                offsets[0][1] = 0;
                offsets[1][0] = 0;
                offsets[1][1] = -1;
                offsets[2][0] = -1;
                offsets[2][1] = -1;
                offsets[3][0] = -1;
                offsets[3][1] = 0;
                found = 0;
                for (i = 0; i < 4; i++) {
                    if (terrain_cell_walkable_flag_current(
                            task->blockX + offsets[i][0],
                            task->blockY + offsets[i][1]))
                        found = 1;
                }
                if (found) {
                    if (task->priorityValue == 0)
                        task->timeIn = 100.0f;
                } else {
                    task->priorityValue = 0;
                }
            } else {
                if (terrain_cell_mask18_clear_current(task->blockX, task->blockY))
                    task->priorityValue = 0;
            }
        }
    }

    if (count) {
        qsort(sortList, count, sizeof(AITask*), aitask_callback_00403f80);
        for (i = 0; i < count - 1; i++)
            sortList[i]->next = sortList[i + 1];
        sortList[i]->next = overflow;
        *list = sortList[0];
    }
}

/* 0x00402510 terrain_cell_command_create (corpus name TerrainCellCommandCreate), 240 bytes.
 * Current definition: src/game/aitask.cpp:776
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00402510(int *param_1,int param_2,int param_3)
 *
 * {
 *   uint uVar1;
 *   int iVar2;
 *   int iVar3;
 *
 *   iVar2 = FUN_00431560();
 *   iVar3 = FUN_00432ec0(param_1);
 *   if (iVar3 == 0) {
 *     iVar3 = FUN_00432f00(param_1);
 *     if (iVar3 == 0) {
 *       iVar3 = FUN_00432f90(*param_1,param_1[1]);
 *       if (iVar3 != 0) {
 *         iVar3 = FUN_00406440(8);
 *         *(int *)(iVar3 + 8) = *param_1;
 *         *(int *)(iVar3 + 0xc) = param_1[1];
 *         if (param_2 != 0) {
 *           *(uint *)(iVar3 + 0x5c) = *(uint *)(iVar3 + 0x5c) | 8;
 *         }
 *         FUN_00402600(iVar3,1);
 *         FUN_00404030(iVar3,3);
 *         uVar1 = *(uint *)(iVar3 + 0x5c);
 *         *(uint *)(iVar3 + 0x5c) = uVar1 | 0x80;
 *         if (param_3 != 0) {
 *           *(uint *)(iVar3 + 0x5c) = uVar1 | 0x20080;
 *         }
 *         if (param_2 == 0) {
 *           FUN_0044f4e0(*(undefined4 *)(iVar2 + 4),*param_1,param_1[1],1);
 *           iVar3 = *(int *)(iVar2 + 0x14) * param_1[1] + *param_1;
 *           *(uint *)(*(int *)(iVar2 + 0xb8) + 8 + iVar3 * 0x48) =
 *                *(uint *)(*(int *)(iVar2 + 0xb8) + 8 + iVar3 * 0x48) | 0x800000;
 *         }
 *       }
 *     }
 *   }
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00402510
void aitask_terrain_cell_command_create(const int* blockPos, int noHighlight,
                                        int cancel)
{
    Lego_Level* level;
    AITask* task;

    level = (Lego_Level*)level_current_owner_get();
    if (!terrain_cell_state_equals_one_current(blockPos) &&
        !terrain_cell_state_equals_six_current(blockPos) &&
        level_cell_flag_4_current(blockPos[0], blockPos[1])) {
        task = aitask_cmd_alloc(8);
        task->blockX = blockPos[0];
        task->blockY = blockPos[1];
        if (noHighlight)
            task->flags |= 8;
        aitask_entity_record_array_apply_zero_state(task, 1);
        aitask_cmd_link_head(task, 3);
        task->flags |= 0x80;
        if (cancel)
            task->flags |= 0x20000;
        if (!noHighlight) {
            terrain_map_block_highlight_set(
                (struct TerrainMap*)LEGO_LEVEL_PTR(void, level->map),
                blockPos[0], blockPos[1], 1);
            LEGO_LEVEL_PTR(Lego_Block, level->blocks)
                [level->width * blockPos[1] + blockPos[0]].flags1 |= 0x800000;
        }
    }
}

/* 0x00402600 entity_record_array_apply_zero_state (corpus name EntityRecordArrayApplyZeroState), 112 bytes.
 * Current definition: src/game/aitask.cpp:828
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00402600(int param_1,int param_2)
 *
 * {
 *   uint *puVar1;
 *   int *piVar2;
 *   int iVar3;
 *   uint uVar4;
 *
 *   iVar3 = FUN_004525b0();
 *   if (iVar3 != 0) {
 *     puVar1 = (uint *)(param_1 + 0x44);
 *     piVar2 = (int *)(param_1 + 0x40);
 *     *(uint *)(param_1 + 0x5c) = *(uint *)(param_1 + 0x5c) | 0x140;
 *     FUN_00452520(piVar2,puVar1);
 *     uVar4 = 0;
 *     if (*puVar1 != 0) {
 *       do {
 *         FUN_00441940(*(undefined4 *)(*piVar2 + uVar4 * 4),0);
 *         if (param_2 != 0) {
 *           FUN_00439e70(*(undefined4 *)(*piVar2 + uVar4 * 4),0);
 *         }
 *         uVar4 = uVar4 + 1;
 *       } while (uVar4 < *puVar1);
 *     }
 *   }
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00402600
void aitask_entity_record_array_apply_zero_state(AITask* task, int detach)
{
    unsigned int i;

    if (selection_active_has_any_get()) {
        task->flags |= 0x140;
        selection_snapshot_create(&task->buffer, &task->count);
        for (i = 0; i < task->count; i++) {
            entity_action_sequence_reset(
                ((struct EntitySlot**)task->buffer)[i], 0);
            if (detach)
                entity_attached_object_list_detach(
                    ((struct EntitySlot**)task->buffer)[i], 0);
        }
    }
}

/* 0x00402670 entity_command15_validated_record_queue (corpus name EntityCommand15ValidatedRecordQueue), 80 bytes.
 * Current definition: src/game/aitask.cpp:855
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00402670(int param_1)
 *
 * {
 *   int iVar1;
 *
 *   FUN_00431560();
 *   if (param_1 != 0) {
 *     iVar1 = FUN_00437cc0(param_1);
 *     if (iVar1 != 0) {
 *       iVar1 = FUN_00406440(0x1a);
 *       FUN_00404030(iVar1,0xf);
 *       *(int *)(iVar1 + 0x10) = param_1;
 *       *(uint *)(iVar1 + 0x5c) = *(uint *)(iVar1 + 0x5c) & 0xffffffef | 0x40040;
 *     }
 *   }
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00402670
void aitask_entity_command15_validated_record_queue(struct EntitySlot* object)
{
    AITask* task;

    level_current_owner_get();
    if (object != NULL && entity_flag40000_threshold_compare(object)) {
        task = aitask_cmd_alloc(0x1a);
        aitask_cmd_link_head(task, 0xf);
        task->object = object;
        task->flags &= ~0x10;
        task->flags |= 0x40040;
    }
}

/* 0x004026c0 entity_deferred_work_schedule (corpus name EntityDeferredWorkSchedule), 96 bytes.
 * Current definition: src/game/aitask.cpp:875
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_004026c0(int param_1)
 *
 * {
 *   int iVar1;
 *
 *   if ((*(int *)(param_1 + 0x304) == 0) && ((*(uint *)(param_1 + 0x3ec) & 0x100) == 0)) {
 *     iVar1 = FUN_00406540(&LAB_00402700,param_1);
 *     if (iVar1 == 0) {
 *       FUN_00402720(param_1,0);
 *     }
 *   }
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x004026c0
void aitask_entity_deferred_work_schedule(struct EntitySlot* object)
{
    if (LEGO_FIELD(object, void*, 0x304) == NULL &&
        !(LEGO_FIELD(object, unsigned int, 0x3ec) & 0x100)) {
        if (!aitask_entity_pool_callback_search(aitask_callback_00402700, object))
            aitask_cmd_create_task(object, 0.0f);
    }
}

/* 0x00402700: an enumeration callback or entry that Ghidra merged into the
 * preceding function; the unit pushes or calls its address. Read it from
 * the disassembly of reference/LegoRR.exe. */
// FUNCTION: LEGORR 0x00402700
int aitask_callback_00402700(AITask* task, void* data)
{
    if (task->type == 3 && task->object == (struct EntitySlot*)data)
        return 1;
    return 0;
}

/* 0x00402720 cmd_create_task (corpus name CmdCreateTask), 128 bytes.
 * Current definition: src/game/aitask.cpp:890
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00402720(int *param_1,undefined4 param_2)
 *
 * {
 *   int iVar1;
 *   int iVar2;
 *   byte bVar3;
 *
 *   bVar3 = 2;
 *   iVar2 = FUN_00406440(3);
 *   *(int **)(iVar2 + 0x10) = param_1;
 *   *(undefined4 *)(iVar2 + 0x18) = param_2;
 *   iVar1 = *param_1;
 *   if (iVar1 == 6) {
 *     bVar3 = -((param_1[0xfa] & 0x80000000U) != 0) & 0x12;
 *   }
 *   else if (iVar1 == 7) {
 *     bVar3 = 1;
 *   }
 *   else if (iVar1 == 9) {
 *     bVar3 = 0x10;
 *   }
 *   else if (iVar1 == 8) {
 *     *(undefined4 *)(iVar2 + 0x3c) = 8;
 *     bVar3 = 3;
 *   }
 *   FUN_00404030(iVar2,bVar3);
 *   FUN_00402850(param_1);
 *   return;
 * }
 *
 *
 * // ==== FUN_004027a0 @ 004027A0 ====
 *
 *
 * void FUN_004027a0(undefined4 *param_1)
 *
 * {
 *   int iVar1;
 *
 *   iVar1 = FUN_00433090(param_1);
 *   if (iVar1 == 0) {
 *     iVar1 = FUN_00432c50(param_1);
 *     if (iVar1 == 0) {
 *       iVar1 = FUN_0040aea0(0xf,0,0);
 *       if (iVar1 != 0) {
 *         iVar1 = FUN_00406440(0x16);
 *         *(undefined4 *)(iVar1 + 8) = *param_1;
 *         *(undefined4 *)(iVar1 + 0xc) = param_1[1];
 *         FUN_00404030(iVar1,0xe);
 *       }
 *     }
 *   }
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00402720
void aitask_cmd_create_task(struct EntitySlot* object, float timeIn)
{
    AITask* task;
    int priority;

    priority = 2;
    task = aitask_cmd_alloc(3);
    task->object = object;
    task->timeIn = timeIn;
    if (LEGO_FIELD(object, int, 0) == 6) {
        priority = (LEGO_FIELD(object, unsigned int, 0x3e8) & 0x80000000) ? 0x12 : 0;
    } else if (LEGO_FIELD(object, int, 0) == 7) {
        priority = 1;
    } else if (LEGO_FIELD(object, int, 0) == 9) {
        priority = 0x10;
    } else if (LEGO_FIELD(object, int, 0) == 8) {
        task->field_3c = 8;
        priority = 3;
    }
    aitask_cmd_link_head(task, priority);
    aitask_cmd_minion_companion(object);
}

/* 0x004027a0 bobbin_clear_delay (corpus name FUN_004027a0), 96 bytes.
 * Current definition: src/game/aitask.cpp:910
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * (not in the corpus)
 */
// FUNCTION: LEGORR 0x004027a0
void aitask_bobbin_clear_delay(const int* blockPos)
{
    AITask* task;

    if (!level_cell_occupied_at_current(blockPos) &&
        !terrain_cell_speed_class_current(blockPos) &&
        ensure_cell_entries_loaded(0xf, 0, 0)) {
        task = aitask_cmd_alloc(0x16);
        task->blockX = blockPos[0];
        task->blockY = blockPos[1];
        aitask_cmd_link_head(task, 0xe);
    }
}

/* 0x00402800 entity_command14_batch_issue (corpus name EntityCommand14BatchIssue), 80 bytes.
 * Current definition: src/game/aitask.cpp:942
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00402800(int param_1)
 *
 * {
 *   int iVar1;
 *   bool bVar2;
 *
 *   bVar2 = param_1 == 0;
 *   if (bVar2) {
 *     param_1 = 1;
 *   }
 *   do {
 *     iVar1 = FUN_00406440(4);
 *     FUN_00402060(iVar1,0x14);
 *     if (bVar2) {
 *       *(uint *)(iVar1 + 0x5c) = *(uint *)(iVar1 + 0x5c) | 0x40;
 *     }
 *     FUN_00404070(iVar1);
 *     param_1 = param_1 + -1;
 *   } while (param_1 != 0);
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00402800
void aitask_entity_command14_batch_issue(unsigned int count)
{
    AITask* task;
    unsigned int i;
    int single;

    single = 0;
    if (count == 0) {
        single = 1;
        count = 1;
    }
    for (i = 0; i < count; i++) {
        task = aitask_cmd_alloc(4);
        aitask_cmd_set_kind(task, 0x14);
        if (single)
            task->flags |= 0x40;
        aitask_cmd_link_tail(task);
    }
}

/* 0x00402850 cmd_minion_companion (corpus name MinionCompanionCmd), 64 bytes.
 * Current definition: src/game/aitask.cpp:971
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00402850(int *param_1)
 *
 * {
 *   int iVar1;
 *
 *   if (*param_1 == 6) {
 *     iVar1 = FUN_00406440(3);
 *     *(int **)(iVar1 + 0x10) = param_1;
 *     FUN_00402060(iVar1,0x15);
 *     FUN_00404070(iVar1);
 *     *(uint *)(iVar1 + 0x5c) = *(uint *)(iVar1 + 0x5c) | 0x2000;
 *   }
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00402850
void aitask_cmd_minion_companion(struct EntitySlot* object)
{
    AITask* task;

    if (LEGO_FIELD(object, int, 0) == 6) {
        task = aitask_cmd_alloc(3);
        task->object = object;
        aitask_cmd_set_kind(task, 0x15);
        aitask_cmd_link_tail(task);
        task->flags |= 0x2000;
    }
}

/* 0x00402890 task_type23_create_for_category_mask (corpus name TaskType23CreateForCategoryMask), 208 bytes.
 * Current definition: src/game/aitask.cpp:992
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00402890(undefined4 param_1,undefined4 param_2,int param_3)
 *
 * {
 *   int iVar1;
 *   undefined4 uVar2;
 *
 *   iVar1 = FUN_00438ed0(param_1,param_2);
 *   if (iVar1 != 0) {
 *     iVar1 = FUN_00406440(0x17);
 *     *(undefined4 *)(iVar1 + 0x10) = param_1;
 *     *(undefined4 *)(iVar1 + 0x3c) = param_2;
 *     switch(param_2) {
 *     case 1:
 *     case 2:
 *     case 4:
 *       uVar2 = 0xc;
 *       break;
 *     default:
 *       uVar2 = 0xd;
 *       break;
 *     case 8:
 *       uVar2 = 3;
 *       break;
 *     case 0x10:
 *       uVar2 = 7;
 *       break;
 *     case 0x20:
 *       uVar2 = 4;
 *     }
 *     if (param_3 != 0) {
 *       FUN_00402600(iVar1,1);
 *     }
 *     FUN_00404030(iVar1,uVar2);
 *     *(uint *)(iVar1 + 0x5c) = *(uint *)(iVar1 + 0x5c) & 0xffffffbf;
 *   }
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00402890
void aitask_task_type23_create_for_category_mask(struct EntitySlot* object,
                                                 int mask, int detach)
{
    AITask* task;
    int priority;

    if (entity_flags_contain_requested_single_bit(object, mask)) {
        task = aitask_cmd_alloc(0x17);
        task->object = object;
        task->field_3c = mask;
        switch (mask) {
        case 1:
            priority = 0xc;
            break;
        case 2:
            priority = 0xc;
            break;
        case 4:
            priority = 0xc;
            break;
        case 8:
            priority = 3;
            break;
        case 0x10:
            priority = 7;
            break;
        case 0x20:
            priority = 4;
            break;
        default:
            priority = 0xd;
            break;
        }
        if (detach)
            aitask_entity_record_array_apply_zero_state(task, 1);
        aitask_cmd_link_head(task, priority);
        task->flags &= ~0x40;
    }
}

/* 0x00402960 entity_kind_command_issue (corpus name EntityKindCommandIssue), 224 bytes.
 * Current definition: src/game/aitask.cpp:1021
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00402960(int param_1)
 *
 * {
 *   int iVar1;
 *   uint uVar2;
 *
 *   if ((*(byte *)(param_1 + 0x3e4) & 0x10) == 0) {
 *     iVar1 = FUN_00406440(0x12);
 *     *(int *)(iVar1 + 0x10) = param_1;
 *     *(uint *)(param_1 + 0x3e4) = *(uint *)(param_1 + 0x3e4) | 0x10;
 *     uVar2 = FUN_00469ea0(param_1);
 *     if ((uVar2 & 0x100) == 0) {
 *       uVar2 = FUN_00469e60(param_1);
 *       if ((uVar2 & 0x2000000) == 0) {
 *         uVar2 = FUN_00469e60(param_1);
 *         if ((uVar2 & 0x1000000) != 0) {
 *           *(undefined4 *)(iVar1 + 0x3c) = 2;
 *         }
 *       }
 *       else {
 *         uVar2 = FUN_00469e60(param_1);
 *         if ((uVar2 & 0x1000000) == 0) {
 *           *(undefined4 *)(iVar1 + 0x3c) = 4;
 *         }
 *         else {
 *           *(undefined4 *)(iVar1 + 0x3c) = 1;
 *         }
 *       }
 *     }
 *     else {
 *       *(undefined4 *)(iVar1 + 0x3c) = 1;
 *     }
 *     uVar2 = FUN_00469ea0(param_1);
 *     if (((uVar2 & 0x40) != 0) && ((*(uint *)(param_1 + 0x3e0) & 0x400) == 0)) {
 *       *(uint *)(iVar1 + 0x5c) = *(uint *)(iVar1 + 0x5c) | 0x100000;
 *     }
 *     FUN_00402600(iVar1,~*(uint *)(iVar1 + 0x5c) >> 0x14 & 1);
 *     *(uint *)(iVar1 + 0x5c) = *(uint *)(iVar1 + 0x5c) & 0xffffffbf;
 *     FUN_00404030(iVar1,0xc);
 *   }
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00402960
void aitask_entity_kind_command_issue(struct EntitySlot* object)
{
    AITask* task;

    if (!(LEGO_FIELD(object, unsigned int, 0x3e4) & 0x10)) {
        task = aitask_cmd_alloc(0x12);
        task->object = object;
        LEGO_FIELD(object, unsigned int, 0x3e4) |= 0x10;
        if (entity_capability_flags_get(object) & 0x100) {
            task->field_3c = 1;
        } else if (entity_stats_word_get(object) & 0x2000000) {
            if (entity_stats_word_get(object) & 0x1000000)
                task->field_3c = 1;
            else
                task->field_3c = 4;
        } else if (entity_stats_word_get(object) & 0x1000000) {
            task->field_3c = 2;
        }
        if ((entity_capability_flags_get(object) & 0x40) &&
            !(LEGO_FIELD(object, unsigned int, 0x3e0) & 0x400))
            task->flags |= 0x100000;
        aitask_entity_record_array_apply_zero_state(task, !(task->flags & 0x100000));
        task->flags &= ~0x40;
        aitask_cmd_link_head(task, 0xc);
    }
}

/* The search record the block-position dispatchers (0x00402a40 to 0x00402b60)
 * pass to the pool search: the block position and a mode word. */
typedef struct AITask_BlockSearch {
    const int* blockPos;
    int mode;
} AITask_BlockSearch;

/* 0x00402a40 coordinate_pair_callback_dispatch (corpus name CoordinatePairCallbackDispatch), 96 bytes.
 * Current definition: src/game/aitask.cpp:1062
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00402a40(undefined4 param_1)
 *
 * {
 *   undefined4 local_8;
 *   undefined4 local_4;
 *
 *   local_8 = param_1;
 *   local_4 = 0;
 *   FUN_00406540(&LAB_00402a70,&local_8);
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00402a40
void aitask_coordinate_pair_callback_dispatch(const int* blockPos)
{
    AITask_BlockSearch search;

    search.blockPos = blockPos;
    search.mode = 0;
    aitask_entity_pool_callback_search(aitask_callback_00402a70, &search);
}

/* 0x00402a70: an enumeration callback or entry that Ghidra merged into the
 * preceding function; the unit pushes or calls its address. Read it from
 * the disassembly of reference/LegoRR.exe. */
// FUNCTION: LEGORR 0x00402a70
int aitask_callback_00402a70(AITask* task, void* data)
{
    AITask_BlockSearch* search = (AITask_BlockSearch*)data;

    if (task->type == 0x19 && search->blockPos[0] == task->blockX &&
        search->blockPos[1] == task->blockY)
        task->flags |= 0x20;
    return 0;
}

/* 0x00402aa0 argument_callback_dispatch (corpus name ArgumentCallbackDispatch), 64 bytes.
 * Current definition: src/game/aitask.cpp:1071
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00402aa0(undefined4 param_1)
 *
 * {
 *   FUN_00406540(&LAB_00402ac0,param_1);
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00402aa0
void aitask_argument_callback_dispatch(struct EntitySlot* object)
{
    aitask_entity_pool_callback_search(aitask_callback_00402ac0, object);
}

/* 0x00402ac0: an enumeration callback or entry that Ghidra merged into the
 * preceding function; the unit pushes or calls its address. Read it from
 * the disassembly of reference/LegoRR.exe. */
// FUNCTION: LEGORR 0x00402ac0
int aitask_callback_00402ac0(AITask* task, void* data)
{
    if (task->type == 0x1a && task->object == (struct EntitySlot*)data)
        task->flags |= 0x20;
    return 0;
}

/* 0x00402ae0 terrain_cell_refresh_dispatch_current (corpus name TerrainCellRefreshDispatch), 80 bytes.
 * Current definition: src/game/aitask.cpp:1080
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00402ae0(undefined4 *param_1,undefined4 param_2)
 *
 * {
 *   int iVar1;
 *   undefined4 *local_8;
 *   undefined4 local_4;
 *
 *   iVar1 = FUN_00431560();
 *   local_8 = param_1;
 *   local_4 = param_2;
 *   FUN_00406540(&LAB_00402bb0,&local_8);
 *   FUN_0044f570(*(undefined4 *)(iVar1 + 4),*param_1,param_1[1]);
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00402ae0
void aitask_terrain_cell_refresh_dispatch_current(const int* blockPos, int mode)
{
    Lego_Level* level;
    AITask_BlockSearch search;

    level = (Lego_Level*)level_current_owner_get();
    search.blockPos = blockPos;
    search.mode = mode;
    aitask_entity_pool_callback_search(aitask_callback_00402bb0, &search);
    terrain_map_block_highlight_clear(
        (struct TerrainMap*)LEGO_LEVEL_PTR(void, level->map),
        blockPos[0], blockPos[1]);
}

/* 0x00402b30 coordinate_callback_dispatch_current (corpus name CoordinateCallbackDispatch), 48 bytes.
 * Current definition: src/game/aitask.cpp:1103
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00402b30(undefined4 param_1)
 *
 * {
 *   undefined4 local_8;
 *   undefined4 local_4;
 *
 *   local_8 = param_1;
 *   local_4 = 0;
 *   FUN_00406540(&LAB_00402c20,&local_8);
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00402b30
void aitask_coordinate_callback_dispatch_current(const int* blockPos)
{
    AITask_BlockSearch search;

    search.blockPos = blockPos;
    search.mode = 0;
    aitask_entity_pool_callback_search(aitask_callback_00402c20, &search);
}

/* 0x00402b60 coordinate_callback_dispatch_by_mode (corpus name CoordinateCallbackDispatchByMode), 368 bytes.
 * Current definition: src/game/aitask.cpp:1113
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00402b60(undefined4 param_1,int param_2)
 *
 * {
 *   undefined4 local_8;
 *   undefined4 local_4;
 *
 *   local_4 = 0;
 *   local_8 = param_1;
 *   if (param_2 != 0) {
 *     FUN_00406540(&LAB_00402ca0,&local_8);
 *     return;
 *   }
 *   FUN_00406540(&LAB_00402c70,&local_8);
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00402b60
void aitask_coordinate_callback_dispatch_by_mode(const int* blockPos, int mode)
{
    AITask_BlockSearch search;

    search.mode = 0;
    search.blockPos = blockPos;
    if (mode)
        aitask_entity_pool_callback_search(aitask_callback_00402ca0, &search);
    else
        aitask_entity_pool_callback_search(aitask_callback_00402c70, &search);
}

/* 0x00402bb0: an enumeration callback or entry that Ghidra merged into the
 * preceding function; the unit pushes or calls its address. Read it from
 * the disassembly of reference/LegoRR.exe. */
// FUNCTION: LEGORR 0x00402bb0
int aitask_callback_00402bb0(AITask* task, void* data)
{
    AITask_BlockSearch* search = (AITask_BlockSearch*)data;
    int match;

    match = 0;
    if (task->type == 8) {
        if ((search->mode && (task->flags & 8)) ||
            (!search->mode && !(task->flags & 8)))
            match = 1;
    } else if (task->type == 9 || task->type == 0xb) {
        match = 1;
    }
    if (match && search->blockPos[0] == task->blockX &&
        search->blockPos[1] == task->blockY) {
        task->flags |= 0x20;
        level_tile_mark_current_target(search->blockPos, 0);
    }
    return 0;
}

/* 0x00402c20: an enumeration callback or entry that Ghidra merged into the
 * preceding function; the unit pushes or calls its address. Read it from
 * the disassembly of reference/LegoRR.exe. */
// FUNCTION: LEGORR 0x00402c20
int aitask_callback_00402c20(AITask* task, void* data)
{
    AITask_BlockSearch* search = (AITask_BlockSearch*)data;

    if (task->type == 0xb && search->blockPos[0] == task->blockX &&
        search->blockPos[1] == task->blockY) {
        task->flags |= 0x20;
        terrain_map_block_highlight_clear(
            AITASK_LEGO_GET_MAP(),
            task->blockX, task->blockY);
    }
    return 0;
}

/* 0x00402c70: an enumeration callback or entry that Ghidra merged into the
 * preceding function; the unit pushes or calls its address. Read it from
 * the disassembly of reference/LegoRR.exe. */
// FUNCTION: LEGORR 0x00402c70
int aitask_callback_00402c70(AITask* task, void* data)
{
    AITask_BlockSearch* search = (AITask_BlockSearch*)data;

    if (task->type == 0xc && search->blockPos[0] == task->blockX &&
        search->blockPos[1] == task->blockY)
        task->flags |= 0x20;
    return 0;
}

/* 0x00402ca0: an enumeration callback or entry that Ghidra merged into the
 * preceding function; the unit pushes or calls its address. Read it from
 * the disassembly of reference/LegoRR.exe. */
// FUNCTION: LEGORR 0x00402ca0
int aitask_callback_00402ca0(AITask* task, void* data)
{
    AITask_BlockSearch* search = (AITask_BlockSearch*)data;

    if (task->type == 0xc && search->blockPos[0] == task->blockX &&
        search->blockPos[1] == task->blockY)
        task->timeIn = 500.0f;
    return 0;
}

/* 0x00402cd0 cmd_cancel_ref (corpus name CmdCancelRef), 128 bytes.
 * Current definition: src/game/aitask.cpp:1129
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00402cd0(int *param_1)
 *
 * {
 *   int *piVar1;
 *
 *   if (*param_1 == 0x13) {
 *     if (param_1[0x16] != 0) {
 *       FUN_00406540(&LAB_00402d30,&param_1);
 *       return;
 *     }
 *   }
 *   else {
 *     piVar1 = DAT_004b3354;
 *     if (param_1[0x16] != 0) {
 *       for (; piVar1 != (int *)0x0; piVar1 = (int *)piVar1[0x18]) {
 *         if ((*piVar1 == 0x13) && ((int *)param_1[0x16] == piVar1)) {
 *           piVar1[0x17] = piVar1[0x17] | 0x20;
 *         }
 *       }
 *     }
 *   }
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00402cd0
void aitask_cmd_cancel_ref(AITask* task)
{
    AITask* cur;

    if (task->type == 0x13) {
        if (task->ref != NULL)
            aitask_entity_pool_callback_search(aitask_callback_00402d30, &task);
    } else if (task->ref != NULL) {
        for (cur = aiTaskGlobs.globalHead; cur != NULL; cur = cur->next) {
            if (cur->type == 0x13 && task->ref == cur)
                cur->flags |= 0x20;
        }
    }
}

/* 0x00402d30: an enumeration callback or entry that Ghidra merged into the
 * preceding function; the unit pushes or calls its address. Read it from
 * the disassembly of reference/LegoRR.exe. */
// FUNCTION: LEGORR 0x00402d30
int aitask_callback_00402d30(AITask* task, void* data)
{
    return task->ref == (AITask*)data;
}

/* 0x00402d50 kind_nine_cell_command_issue (corpus name KindNineCellCommandIssue), 240 bytes.
 * Current definition: src/game/aitask.cpp:1156
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * undefined4 FUN_00402d50(int *param_1,int param_2)
 *
 * {
 *   int iVar1;
 *   undefined4 uVar2;
 *
 *   if (*(char *)(*(int *)(DAT_005560d4 + 0xb8) + 2 +
 *                (*(int *)(DAT_005560d4 + 0x14) * param_1[1] + *param_1) * 0x48) != '\x01') {
 *     iVar1 = FUN_00432bd0(*param_1,param_1[1]);
 *     if (iVar1 != 0) {
 *       iVar1 = FUN_00432dd0(param_1);
 *       if (iVar1 == 0) {
 *         iVar1 = FUN_00406440(9);
 *         *(int *)(iVar1 + 8) = *param_1;
 *         *(int *)(iVar1 + 0xc) = param_1[1];
 *         *(undefined4 *)(iVar1 + 0x20) = 8;
 *         *(undefined4 *)(iVar1 + 0x24) = 0xffffffff;
 *         *(undefined4 *)(iVar1 + 0x28) = 0;
 *         *(uint *)(iVar1 + 0x5c) = *(uint *)(iVar1 + 0x5c) | 0x400;
 *         if (param_2 == 0) {
 *           FUN_00402600(iVar1,1);
 *         }
 *         FUN_00404030(iVar1,3);
 *         FUN_00403810(param_1,8,0,0);
 *         FUN_00432e00(param_1,1);
 *         uVar2 = FUN_00431570(*param_1,param_1[1],2);
 *         FUN_0044f4e0(uVar2);
 *         return 1;
 *       }
 *     }
 *   }
 *   return 0;
 * }
 */
// FUNCTION: LEGORR 0x00402d50
int aitask_kind_nine_cell_command_issue(const int* blockPos, int noDetach)
{
    AITask* task;

    if (LEGO_LEVEL_PTR(Lego_Block, legoGlobs_currLevel->blocks)
            [legoGlobs_currLevel->width * blockPos[1] + blockPos[0]].f_02 != 1 &&
        terrain_cell_walkable_flag_current(blockPos[0], blockPos[1]) &&
        !terrain_cell_flag_reserved_current(blockPos)) {
        task = aitask_cmd_alloc(9);
        task->blockX = blockPos[0];
        task->blockY = blockPos[1];
        task->field_20 = 8;
        task->field_24 = -1;
        task->field_28 = 0;
        task->flags |= 0x400;
        if (!noDetach)
            aitask_entity_record_array_apply_zero_state(task, 1);
        aitask_cmd_link_head(task, 3);
        aitask_terrain_cell_record_count_update(blockPos, 8, 0, 0);
        level_tile_mark_current_target(blockPos, 1);
        terrain_map_block_highlight_set(
            AITASK_LEGO_GET_MAP(),
            blockPos[0], blockPos[1], 2);
        return 1;
    }
    return 0;
}

/* 0x00402e40 cell_command_type13_issue (corpus name CellCommandType13Issue), 128 bytes.
 * Current definition: src/game/aitask.cpp:1193
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00402e40(undefined4 *param_1)
 *
 * {
 *   int iVar1;
 *
 *   iVar1 = FUN_00432b50(*param_1,param_1[1]);
 *   if (iVar1 != 0) {
 *     iVar1 = FUN_00406440(0x14);
 *     *(undefined4 *)(iVar1 + 8) = *param_1;
 *     *(undefined4 *)(iVar1 + 0xc) = param_1[1];
 *     *(undefined4 *)(iVar1 + 0x20) = 0xd;
 *     *(undefined4 *)(iVar1 + 0x24) = 0xffffffff;
 *     *(undefined4 *)(iVar1 + 0x28) = 0;
 *     *(uint *)(iVar1 + 0x5c) = *(uint *)(iVar1 + 0x5c) | 0x400;
 *     FUN_00402600(iVar1,1);
 *     FUN_00404030(iVar1,3);
 *     FUN_00403810(param_1,0xd,0,0);
 *   }
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00402e40
void aitask_cell_command_type13_issue(const int* blockPos)
{
    AITask* task;

    if (terrain_cell_flag_8_bounded_current(blockPos[0], blockPos[1])) {
        task = aitask_cmd_alloc(0x14);
        task->blockX = blockPos[0];
        task->blockY = blockPos[1];
        task->field_20 = 0xd;
        task->field_24 = -1;
        task->field_28 = 0;
        task->flags |= 0x400;
        aitask_entity_record_array_apply_zero_state(task, 1);
        aitask_cmd_link_head(task, 3);
        aitask_terrain_cell_record_count_update(blockPos, 0xd, 0, 0);
    }
}

/* 0x00402ec0 entity_conditional_task_schedule (corpus name EntityConditionalTaskSchedule), 112 bytes.
 * Current definition: src/game/aitask.cpp:1221
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00402ec0(int param_1)
 *
 * {
 *   uint uVar1;
 *   int iVar2;
 *
 *   uVar1 = FUN_00469ea0(param_1);
 *   if ((((uVar1 & 1) != 0) && ((*(uint *)(param_1 + 0x3e0) & 0x400) == 0)) &&
 *      ((*(uint *)(param_1 + 0x3ec) & 0x10000) == 0)) {
 *     iVar2 = FUN_00406440(0x1e);
 *     *(int *)(iVar2 + 0x10) = param_1;
 *     *(uint *)(param_1 + 0x3ec) = *(uint *)(param_1 + 0x3ec) | 0x10000;
 *     FUN_00404030(iVar2,0x11);
 *     *(uint *)(iVar2 + 0x5c) = *(uint *)(iVar2 + 0x5c) & 0xffffffbf | 0x100010;
 *   }
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00402ec0
void aitask_entity_conditional_task_schedule(struct EntitySlot* object)
{
    AITask* task;

    if ((entity_capability_flags_get(object) & 1) &&
        !(LEGO_FIELD(object, unsigned int, 0x3e0) & 0x400) &&
        !(LEGO_FIELD(object, unsigned int, 0x3ec) & 0x10000)) {
        task = aitask_cmd_alloc(0x1e);
        task->object = object;
        LEGO_FIELD(object, unsigned int, 0x3ec) |= 0x10000;
        aitask_cmd_link_head(task, 0x11);
        task->flags &= ~0x40;
        task->flags |= 0x100010;
    }
}

/* 0x00402f30 entity_threshold_progress_event_queue (corpus name EntityThresholdProgressEventQueue), 64 bytes.
 * Current definition: src/game/aitask.cpp:1240
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00402f30(undefined4 param_1,int param_2)
 *
 * {
 *   int iVar1;
 *
 *   iVar1 = FUN_00406440(10);
 *   *(undefined4 *)(iVar1 + 0x10) = param_1;
 *   *(undefined4 *)(iVar1 + 0x3c) = 0x10;
 *   if (param_2 != 0) {
 *     *(uint *)(iVar1 + 0x5c) = *(uint *)(iVar1 + 0x5c) | 0x200000;
 *   }
 *   FUN_00404030(iVar1,(-(param_2 != 0) & 0xcU) + 7);
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00402f30
void aitask_entity_threshold_progress_event_queue(struct EntitySlot* object,
                                                  int urgent)
{
    AITask* task;

    task = aitask_cmd_alloc(10);
    task->object = object;
    task->field_3c = 0x10;
    if (urgent)
        task->flags |= 0x200000;
    aitask_cmd_link_head(task, urgent ? 0x13 : 7);
}

/* 0x00402f70 entity_target_command_queue (corpus name EntityTargetCommandQueue), 64 bytes.
 * Current definition: src/game/aitask.cpp:1256
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00402f70(int param_1,undefined4 param_2)
 *
 * {
 *   int iVar1;
 *   int iVar2;
 *
 *   iVar1 = FUN_00406440(0x11);
 *   *(undefined4 *)(iVar1 + 0x10) = param_2;
 *   iVar2 = FUN_00404190(param_1);
 *   if (iVar2 != 0) {
 *     *(int *)(iVar2 + 0x60) = iVar1;
 *     return;
 *   }
 *   *(int *)(param_1 + 0x2f0) = iVar1;
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00402f70
void aitask_entity_target_command_queue(struct EntitySlot* object,
                                        struct EntitySlot* target)
{
    AITask* task;
    AITask* tail;

    task = aitask_cmd_alloc(0x11);
    task->object = target;
    if ((tail = aitask_cmd_chain_walk(object)) != NULL)
        tail->next = task;
    else
        LEGO_FIELD(object, AITask*, 0x2f0) = task;
}

/* 0x00402fb0 cell_command_type11_issue (corpus name CellCommandType11Issue), 128 bytes.
 * Current definition: src/game/aitask.cpp:1272
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00402fb0(undefined4 *param_1)
 *
 * {
 *   int iVar1;
 *
 *   iVar1 = FUN_00406440(0xf);
 *   *(undefined4 *)(iVar1 + 8) = *param_1;
 *   *(undefined4 *)(iVar1 + 0xc) = param_1[1];
 *   *(undefined4 *)(iVar1 + 0x20) = 0xb;
 *   *(undefined4 *)(iVar1 + 0x24) = 0xffffffff;
 *   *(undefined4 *)(iVar1 + 0x28) = 0;
 *   *(uint *)(iVar1 + 0x5c) = *(uint *)(iVar1 + 0x5c) | 0x400;
 *   FUN_00402600(iVar1,1);
 *   FUN_00404030(iVar1,4);
 *   FUN_00403810(param_1,0xb,0,0);
 *   FUN_004324d0(*param_1,param_1[1],1);
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00402fb0
void aitask_cell_command_type11_issue(const int* blockPos)
{
    AITask* task;

    task = aitask_cmd_alloc(0xf);
    task->blockX = blockPos[0];
    task->blockY = blockPos[1];
    task->field_20 = 0xb;
    task->field_24 = -1;
    task->field_28 = 0;
    task->flags |= 0x400;
    aitask_entity_record_array_apply_zero_state(task, 1);
    aitask_cmd_link_head(task, 4);
    aitask_terrain_cell_record_count_update(blockPos, 0xb, 0, 0);
    terrain_cell_secondary_flag_8_set_current(blockPos[0], blockPos[1], 1);
}

/* 0x00403030 cell_command_type3_issue (corpus name CellCommandType3Issue), 128 bytes.
 * Current definition: src/game/aitask.cpp:1298
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00403030(undefined4 *param_1)
 *
 * {
 *   int iVar1;
 *   undefined4 uVar2;
 *
 *   iVar1 = FUN_00432bd0(*param_1,param_1[1]);
 *   if (iVar1 != 0) {
 *     iVar1 = FUN_00432d90(*param_1,param_1[1]);
 *     if (iVar1 == 0) {
 *       iVar1 = FUN_00406440(0xb);
 *       *(undefined4 *)(iVar1 + 8) = *param_1;
 *       *(undefined4 *)(iVar1 + 0xc) = param_1[1];
 *       FUN_00402600(iVar1,1);
 *       FUN_00404030(iVar1,6);
 *       uVar2 = FUN_00431570(*param_1,param_1[1],3);
 *       FUN_0044f4e0(uVar2);
 *     }
 *   }
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00403030
void aitask_cell_command_type3_issue(const int* blockPos)
{
    AITask* task;

    if (terrain_cell_walkable_flag_current(blockPos[0], blockPos[1]) &&
        !terrain_cell_flag_20_bounded_current(blockPos[0], blockPos[1])) {
        task = aitask_cmd_alloc(0xb);
        task->blockX = blockPos[0];
        task->blockY = blockPos[1];
        aitask_entity_record_array_apply_zero_state(task, 1);
        aitask_cmd_link_head(task, 6);
        terrain_map_block_highlight_set(
            AITASK_LEGO_GET_MAP(),
            blockPos[0], blockPos[1], 3);
    }
}

/* 0x004030b0 coordinate_pair_event_queue (corpus name CoordinatePairEventQueue), 48 bytes.
 * Current definition: src/game/aitask.cpp:1328
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_004030b0(undefined4 *param_1,undefined4 param_2)
 *
 * {
 *   int iVar1;
 *
 *   iVar1 = FUN_00406440(0xc);
 *   *(undefined4 *)(iVar1 + 8) = *param_1;
 *   *(undefined4 *)(iVar1 + 0xc) = param_1[1];
 *   *(undefined4 *)(iVar1 + 0x54) = param_2;
 *   FUN_00404030(iVar1,8);
 *   return;
 * }
 *
 *
 * // ==== FUN_004030e0 @ 004030E0 ====
 *
 *
 * void FUN_004030e0(undefined4 *param_1)
 *
 * {
 *   undefined4 *puVar1;
 *   int iVar2;
 *   undefined4 *puVar3;
 *   uint uVar4;
 *   int iVar5;
 *   undefined4 *puVar6;
 *
 *   puVar1 = param_1;
 *   iVar2 = FUN_0046a120(*param_1,&param_1);
 *   if (iVar2 != 0) {
 *     puVar1[0x17] = puVar1[0x17] | 0x1000;
 *     iVar2 = FUN_00406440(0x13);
 *     if ((puVar1[0x17] & 0x100) != 0) {
 *       puVar3 = (undefined4 *)FUN_0048dbd0(puVar1[0x11] << 2);
 *       *(undefined4 **)(iVar2 + 0x40) = puVar3;
 *       puVar6 = (undefined4 *)puVar1[0x10];
 *       for (uVar4 = puVar1[0x11] & 0x3fffffff; uVar4 != 0; uVar4 = uVar4 - 1) {
 *         *puVar3 = *puVar6;
 *         puVar6 = puVar6 + 1;
 *         puVar3 = puVar3 + 1;
 *       }
 *       for (iVar5 = 0; iVar5 != 0; iVar5 = iVar5 + -1) {
 *         *(undefined1 *)puVar3 = *(undefined1 *)puVar6;
 *         puVar6 = (undefined4 *)((int)puVar6 + 1);
 *         puVar3 = (undefined4 *)((int)puVar3 + 1);
 *       }
 *       *(undefined4 *)(iVar2 + 0x44) = puVar1[0x11];
 *       *(uint *)(iVar2 + 0x5c) = *(uint *)(iVar2 + 0x5c) | 0x100;
 *     }
 *     *(uint *)(iVar2 + 0x5c) = *(uint *)(iVar2 + 0x5c) | 0x10;
 *     *(undefined4 **)(iVar2 + 0x38) = param_1;
 *     *(undefined4 **)(iVar2 + 0x58) = puVar1;
 *     puVar1[0x16] = iVar2;
 *     FUN_00404030(iVar2,puVar1[0x13]);
 *   }
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x004030b0
void aitask_coordinate_pair_event_queue(const int* blockPos, void* payload)
{
    AITask* task;

    task = aitask_cmd_alloc(0xc);
    task->blockX = blockPos[0];
    task->blockY = blockPos[1];
    task->eventPayload = payload;
    aitask_cmd_link_head(task, 8);
}

/* 0x004030e0 cmd_validated_record_queue (corpus name FUN_004030e0), 176 bytes.
 * Current definition: src/game/aitask.cpp:1344
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * (not in the corpus)
 */
// FUNCTION: LEGORR 0x004030e0
void aitask_cmd_validated_record_queue(AITask* source)
{
    AITask* task;
    int group;

    if (entity_grouped_code_table_find_value_index(source->type, &group)) {
        source->flags |= 0x1000;
        task = aitask_cmd_alloc(0x13);
        if (source->flags & 0x100) {
            task->buffer = malloc(source->count * 4);
            memcpy(task->buffer, source->buffer, source->count * 4);
            task->count = source->count;
            task->flags |= 0x100;
        }
        task->flags |= 0x10;
        task->field_38 = (void*)(long)group;
        task->ref = source;
        source->ref = task;
        aitask_cmd_link_head(task, source->priority);
    }
}

/* 0x00403190 object_event_type2_queue (corpus name ObjectEventType2Queue), 80 bytes.
 * Current definition: src/game/aitask.cpp:1385
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * undefined4 FUN_00403190(undefined4 param_1)
 *
 * {
 *   int iVar1;
 *
 *   iVar1 = FUN_00406440(0x13);
 *   *(undefined4 *)(iVar1 + 0x38) = param_1;
 *   *(undefined4 *)(iVar1 + 0x58) = 0;
 *   FUN_00402600(iVar1,1);
 *   *(uint *)(iVar1 + 0x5c) = *(uint *)(iVar1 + 0x5c) & 0xffffffbf | 0x10;
 *   FUN_00404030(iVar1,2);
 *   return 1;
 * }
 */
// FUNCTION: LEGORR 0x00403190
int aitask_object_event_type2_queue(void* payload)
{
    AITask* task;

    task = aitask_cmd_alloc(0x13);
    task->field_38 = payload;
    task->ref = NULL;
    aitask_entity_record_array_apply_zero_state(task, 1);
    task->flags &= ~0x40;
    task->flags |= 0x10;
    aitask_cmd_link_head(task, 2);
    return 1;
}

/* 0x004031e0 active_entity_list_contains_code (corpus name ActiveEntityListContainsCode), 144 bytes.
 * Current definition: src/game/aitask.cpp:1398
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * undefined4 FUN_004031e0(undefined4 param_1)
 *
 * {
 *   int iVar1;
 *   uint uVar2;
 *   undefined4 *puVar3;
 *   uint uVar4;
 *   int *piVar5;
 *
 *   iVar1 = FUN_004525b0();
 *   if (iVar1 == 0) {
 *     uVar2 = 0;
 *     if (DAT_004b3424 != 0) {
 *       piVar5 = &DAT_004b335c;
 *       while ((*piVar5 == 0 || (iVar1 = FUN_0044b480(*piVar5,param_1), iVar1 == 0))) {
 *         uVar2 = uVar2 + 1;
 *         piVar5 = piVar5 + 1;
 *         if (DAT_004b3424 <= uVar2) {
 *           return 0;
 *         }
 *       }
 *       return 1;
 *     }
 *   }
 *   else {
 *     uVar2 = FUN_004525f0();
 *     puVar3 = (undefined4 *)FUN_004525e0();
 *     uVar4 = 0;
 *     if (uVar2 != 0) {
 *       do {
 *         iVar1 = FUN_0044b480(*puVar3,param_1);
 *         if (iVar1 != 0) {
 *           return 1;
 *         }
 *         uVar4 = uVar4 + 1;
 *         puVar3 = puVar3 + 1;
 *       } while (uVar4 < uVar2);
 *       return 0;
 *     }
 *   }
 *   return 0;
 * }
 */
// FUNCTION: LEGORR 0x004031e0
int aitask_active_entity_list_contains_code(int code)
{
    unsigned int count;
    struct EntitySlot* const* list;
    unsigned int i;

    if (selection_active_has_any_get()) {
        count = selection_active_count_get();
        list = selection_active_records_get();
        for (i = 0; i < count; i++) {
            if (ai_activity_scan(list[i], code))
                return 1;
        }
    } else {
        for (i = 0; i < aiTaskGlobs.assignCandidateCount; i++) {
            if (aiTaskGlobs.assignCandidates[i] &&
                ai_activity_scan(aiTaskGlobs.assignCandidates[i], code))
                return 1;
        }
    }
    return 0;
}

/* 0x00403270 selection_drag_area_command_create (corpus name SelectionDragAreaCommandCreate), 176 bytes.
 * Current definition: src/game/aitask.cpp:1433
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00403270(int *param_1,int param_2,undefined4 *param_3,int param_4)
 *
 * {
 *   int iVar1;
 *   int iVar2;
 *   uint uVar3;
 *   int iVar4;
 *
 *   for (; param_2 != 0; param_2 = param_2 + -1) {
 *     iVar2 = FUN_00406440(0);
 *     *(undefined4 *)(iVar2 + 8) = *param_3;
 *     *(undefined4 *)(iVar2 + 0xc) = param_3[1];
 *     *(undefined4 *)(iVar2 + 0x5c) = 4;
 *     iVar1 = *param_1;
 *     uVar3 = FUN_00469e60(iVar1);
 *     if (((((uVar3 & 0x20000) == 0) || (*(int *)(iVar1 + 0x36c) != 0)) &&
 *         (*(int *)(iVar1 + 0x304) == 0)) && ((*(byte *)(iVar1 + 1000) & 0x80) != 0)) {
 *       if (param_4 != 0) {
 *         FUN_00404100(iVar1);
 *       }
 *       iVar4 = FUN_00404190(iVar1);
 *       if (iVar4 == 0) {
 *         *(int *)(iVar1 + 0x2f0) = iVar2;
 *       }
 *       else {
 *         *(int *)(iVar4 + 0x60) = iVar2;
 *       }
 *     }
 *     param_1 = param_1 + 1;
 *   }
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00403270
void aitask_selection_drag_area_command_create(struct EntitySlot** objects,
                                               unsigned int count,
                                               const int* blockPos,
                                               int replace)
{
    AITask* task;
    AITask* tail;
    struct EntitySlot* object;
    unsigned int i;

    if (count) {
        for (i = 0; i < count; i++) {
            task = aitask_cmd_alloc(0);
            task->blockX = blockPos[0];
            task->blockY = blockPos[1];
            task->flags = 4;
            object = objects[i];
            if ((!(entity_stats_word_get(object) & 0x20000) ||
                 LEGO_FIELD(object, void*, 0x36c) != NULL) &&
                LEGO_FIELD(object, void*, 0x304) == NULL &&
                (LEGO_FIELD(object, unsigned int, 0x3e8) & 0x80)) {
                if (replace)
                    aitask_command_queue_sweep_relink(object);
                if ((tail = aitask_cmd_chain_walk(object)) != NULL)
                    tail->next = task;
                else
                    LEGO_FIELD(object, AITask*, 0x2f0) = task;
            }
        }
    }
}

/* 0x00403320 object_command_task_create (corpus name ObjectCommandTaskCreate), 160 bytes.
 * Current definition: src/game/aitask.cpp:1459
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00403320(int *param_1,undefined4 param_2)
 *
 * {
 *   int iVar1;
 *   int iVar2;
 *   undefined1 local_8 [4];
 *   undefined1 local_4 [4];
 *
 *   iVar2 = 0;
 *   if ((*param_1 == 2) && ((param_1[0xf8] & 0x400U) == 0)) {
 *     FUN_00442260(param_1,local_8,local_4);
 *     iVar2 = FUN_00438df0(local_8);
 *   }
 *   else if (*param_1 == 1) {
 *     iVar2 = FUN_00438bf0(param_1);
 *   }
 *   if (iVar2 != 0) {
 *     iVar1 = FUN_00406440(0x15);
 *     *(int *)(iVar1 + 0x10) = iVar2;
 *     *(undefined4 *)(iVar1 + 0x28) = param_2;
 *     FUN_00404100(param_1);
 *     iVar2 = FUN_00404190(param_1);
 *     if (iVar2 != 0) {
 *       *(int *)(iVar2 + 0x60) = iVar1;
 *       return;
 *     }
 *     param_1[0xbc] = iVar1;
 *   }
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00403320
void aitask_object_command_task_create(struct EntitySlot* object, int param)
{
    AITask* task;
    AITask* tail;
    struct EntitySlot* target;
    float pos[2];

    target = NULL;
    if (LEGO_FIELD(object, int, 0) == 2 &&
        !(LEGO_FIELD(object, unsigned int, 0x3e0) & 0x400)) {
        entity_transform_position_get_outputs(object, &pos[0], &pos[1]);
        target = entity_lookup_at_world_position(pos);
    } else if (LEGO_FIELD(object, int, 0) == 1) {
        target = (struct EntitySlot*)
            ledger_stub_00438bf0_SpatialObjectQueryCenterMask80(object);
    }
    if (target != NULL) {
        task = aitask_cmd_alloc(0x15);
        task->object = target;
        task->field_28 = param;
        aitask_command_queue_sweep_relink(object);
        if ((tail = aitask_cmd_chain_walk(object)) != NULL)
            tail->next = task;
        else
            LEGO_FIELD(object, AITask*, 0x2f0) = task;
    }
}

/* 0x004033c0 object_attached_record_allocate (corpus name ObjectAttachedRecordAllocate), 48 bytes.
 * Current definition: src/game/aitask.cpp:1491
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_004033c0(int param_1)
 *
 * {
 *   undefined4 uVar1;
 *   int iVar2;
 *
 *   uVar1 = FUN_00406440(0x1c);
 *   iVar2 = FUN_00404190(param_1);
 *   if (iVar2 != 0) {
 *     *(undefined4 *)(iVar2 + 0x60) = uVar1;
 *     return;
 *   }
 *   *(undefined4 *)(param_1 + 0x2f0) = uVar1;
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x004033c0
void aitask_object_attached_record_allocate(struct EntitySlot* object)
{
    AITask* task;
    AITask* tail;

    task = aitask_cmd_alloc(0x1c);
    if ((tail = aitask_cmd_chain_walk(object)) != NULL)
        tail->next = task;
    else
        LEGO_FIELD(object, AITask*, 0x2f0) = task;
}

/* 0x004033f0 coordinate_pair_task_record_queue (corpus name CoordinatePairTaskRecordQueue), 64 bytes.
 * Current definition: src/game/aitask.cpp:1506
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_004033f0(undefined4 *param_1)
 *
 * {
 *   int iVar1;
 *
 *   iVar1 = FUN_00406440(0);
 *   *(undefined4 *)(iVar1 + 8) = *param_1;
 *   *(undefined4 *)(iVar1 + 0xc) = param_1[1];
 *   *(uint *)(iVar1 + 0x5c) = *(uint *)(iVar1 + 0x5c) & 0xffffffef | 0x40;
 *   FUN_00402060(iVar1,0x1a);
 *   FUN_00404070(iVar1);
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x004033f0
void aitask_coordinate_pair_task_record_queue(const int* blockPos)
{
    AITask* task;

    task = aitask_cmd_alloc(0);
    task->blockX = blockPos[0];
    task->blockY = blockPos[1];
    task->flags &= ~0x10;
    task->flags |= 0x40;
    aitask_cmd_set_kind(task, 0x1a);
    aitask_cmd_link_tail(task);
}

/* 0x00403430 task_registry_remove_type0_entry_by_pair (corpus name TaskRegistryRemoveType0EntryByPair), 112 bytes.
 * Current definition: src/game/aitask.cpp:1519
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00403430(int *param_1)
 *
 * {
 *   int *piVar1;
 *   int *piVar2;
 *   int *piVar3;
 *   int *piVar4;
 *
 *   piVar4 = (int *)0x0;
 *   piVar1 = DAT_004b3358;
 *   do {
 *     piVar3 = piVar1;
 *     piVar2 = piVar4;
 *     if (piVar3 == (int *)0x0) {
 *       return;
 *     }
 *     piVar1 = (int *)piVar3[0x18];
 *     piVar4 = piVar3;
 *   } while (((*piVar3 != 0) || (piVar3[2] != *param_1)) || (piVar3[3] != param_1[1]));
 *   if (piVar2 == (int *)0x0) {
 *     DAT_004b3358 = piVar1;
 *     FUN_00406480(piVar3,0);
 *     return;
 *   }
 *   piVar2[0x18] = (int)piVar1;
 *   FUN_00406480(piVar3,0);
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00403430
void aitask_task_registry_remove_type0_entry_by_pair(const int* blockPos)
{
    AITask* task;
    AITask* next;
    AITask* prev;

    prev = NULL;
    if ((task = aiTaskGlobs.globalTail) != NULL) {
        do {
            next = task->next;
            if (task->type == 0 && task->blockX == blockPos[0] &&
                task->blockY == blockPos[1]) {
                if (prev != NULL)
                    prev->next = next;
                else
                    aiTaskGlobs.globalTail = next;
                aitask_cmd_free(task, 0);
                return;
            }
            prev = task;
        } while ((task = next) != NULL);
    }
}

/* 0x004034a0 scalar_task_record_queue (corpus name ScalarTaskRecordQueue), 64 bytes.
 * Current definition: src/game/aitask.cpp:1543
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_004034a0(undefined4 param_1)
 *
 * {
 *   int iVar1;
 *
 *   iVar1 = FUN_00406440(0x1d);
 *   *(undefined4 *)(iVar1 + 0x10) = param_1;
 *   FUN_00402060(iVar1,0x19);
 *   FUN_00404070(iVar1);
 *   *(uint *)(iVar1 + 0x5c) = *(uint *)(iVar1 + 0x5c) | 0x40;
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x004034a0
void aitask_scalar_task_record_queue(struct EntitySlot* object)
{
    AITask* task;

    task = aitask_cmd_alloc(0x1d);
    task->object = object;
    aitask_cmd_set_kind(task, 0x19);
    aitask_cmd_link_tail(task);
    task->flags |= 0x40;
}

/* 0x004034e0 task_pair_record_enqueue (corpus name TaskPairRecordEnqueue), 64 bytes.
 * Current definition: src/game/aitask.cpp:1558
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_004034e0(undefined4 *param_1)
 *
 * {
 *   int iVar1;
 *
 *   iVar1 = FUN_00406440(0x19);
 *   *(undefined4 *)(iVar1 + 8) = *param_1;
 *   *(undefined4 *)(iVar1 + 0xc) = param_1[1];
 *   FUN_00402060(iVar1,0x18);
 *   FUN_00404070(iVar1);
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x004034e0
void aitask_task_pair_record_enqueue(const int* blockPos)
{
    AITask* task;

    task = aitask_cmd_alloc(0x19);
    task->blockX = blockPos[0];
    task->blockY = blockPos[1];
    aitask_cmd_set_kind(task, 0x18);
    aitask_cmd_link_tail(task);
}

/* 0x00403520 task_record_kind10_queue (corpus name TaskRecordKind10Queue), 64 bytes.
 * Current definition: src/game/aitask.cpp:1569
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00403520(undefined4 param_1)
 *
 * {
 *   int iVar1;
 *
 *   iVar1 = FUN_00406440(10);
 *   *(undefined4 *)(iVar1 + 0x10) = param_1;
 *   FUN_00402060(iVar1,0x16);
 *   FUN_00404070(iVar1);
 *   *(uint *)(iVar1 + 0x5c) = *(uint *)(iVar1 + 0x5c) | 0x40;
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00403520
void aitask_task_record_kind10_queue(struct EntitySlot* object)
{
    AITask* task;

    task = aitask_cmd_alloc(10);
    task->object = object;
    aitask_cmd_set_kind(task, 0x16);
    aitask_cmd_link_tail(task);
    task->flags |= 0x40;
}

/* 0x00403560 task_registry_remove_type10_entries_for_entity (corpus name TaskRegistryRemoveType10EntriesForEntity), 96 bytes.
 * Current definition: src/game/aitask.cpp:1585
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00403560(int param_1)
 *
 * {
 *   int *piVar1;
 *   int *piVar2;
 *   int *piVar3;
 *   int *piVar4;
 *
 *   piVar4 = (int *)0x0;
 *   piVar1 = DAT_004b3358;
 *   while (piVar3 = piVar1, piVar2 = piVar4, piVar3 != (int *)0x0) {
 *     piVar1 = (int *)piVar3[0x18];
 *     piVar4 = piVar3;
 *     if ((*piVar3 == 10) && (param_1 == piVar3[4])) {
 *       piVar4 = piVar2;
 *       if (piVar2 == (int *)0x0) {
 *         DAT_004b3358 = piVar1;
 *         FUN_00406480(piVar3,0);
 *       }
 *       else {
 *         piVar2[0x18] = (int)piVar1;
 *         FUN_00406480(piVar3,0);
 *       }
 *     }
 *   }
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00403560
void aitask_task_registry_remove_type10_entries_for_entity(
    struct EntitySlot* object)
{
    AITask* task;
    AITask* next;
    AITask* prev;

    prev = NULL;
    for (task = aiTaskGlobs.globalTail; task != NULL; task = next) {
        next = task->next;
        if (task->type == 10 && object == task->object) {
            if (prev != NULL)
                prev->next = next;
            else
                aiTaskGlobs.globalTail = next;
            aitask_cmd_free(task, 0);
        } else {
            prev = task;
        }
    }
}

/* 0x004035c0 cmd_create_move (corpus name CmdCreateMove), 80 bytes.
 * Current definition: src/game/aitask.cpp:1605
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_004035c0(int param_1,undefined4 *param_2,undefined4 param_3)
 *
 * {
 *   int iVar1;
 *   int iVar2;
 *
 *   iVar1 = FUN_00406440(0);
 *   *(undefined4 *)(iVar1 + 8) = *param_2;
 *   *(undefined4 *)(iVar1 + 0xc) = param_2[1];
 *   *(undefined4 *)(iVar1 + 4) = param_3;
 *   iVar2 = FUN_00404190(param_1);
 *   if (iVar2 != 0) {
 *     *(int *)(iVar2 + 0x60) = iVar1;
 *     return;
 *   }
 *   *(int *)(param_1 + 0x2f0) = iVar1;
 *   FUN_00407450(param_1);
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x004035c0
void aitask_cmd_create_move(struct EntitySlot* object, const int* blockPos,
                            int param)
{
    AITask* task;
    AITask* tail;

    task = aitask_cmd_alloc(0);
    task->blockX = blockPos[0];
    task->blockY = blockPos[1];
    task->param = param;
    if ((tail = aitask_cmd_chain_walk(object)) != NULL) {
        tail->next = task;
    } else {
        LEGO_FIELD(object, AITask*, 0x2f0) = task;
        idle_register(object);
    }
}

/* 0x00403610 cmd_create_gated (corpus name CmdCreateGated), 64 bytes.
 * Current definition: src/game/aitask.cpp:1625
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00403610(int param_1,undefined4 param_2)
 *
 * {
 *   int iVar1;
 *   int iVar2;
 *
 *   if ((*(uint *)(param_1 + 0x3e0) & 0x400) != 0) {
 *     iVar1 = FUN_00406440(5);
 *     *(undefined4 *)(iVar1 + 0x10) = param_2;
 *     iVar2 = FUN_00404190(param_1);
 *     if (iVar2 != 0) {
 *       *(int *)(iVar2 + 0x60) = iVar1;
 *       return;
 *     }
 *     *(int *)(param_1 + 0x2f0) = iVar1;
 *   }
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00403610
void aitask_cmd_create_gated(struct EntitySlot* object,
                             struct EntitySlot* target)
{
    AITask* task;
    AITask* tail;

    if (LEGO_FIELD(object, unsigned int, 0x3e0) & 0x400) {
        task = aitask_cmd_alloc(5);
        task->object = target;
        if ((tail = aitask_cmd_chain_walk(object)) != NULL)
            tail->next = task;
        else
            LEGO_FIELD(object, AITask*, 0x2f0) = task;
    }
}

/* 0x00403650 entity_task_batch_append (corpus name EntityTaskBatchAppend), 112 bytes.
 * Current definition: src/game/aitask.cpp:1644
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00403650(int *param_1,int param_2,undefined4 param_3)
 *
 * {
 *   int iVar1;
 *   int iVar2;
 *   int iVar3;
 *
 *   for (; param_2 != 0; param_2 = param_2 + -1) {
 *     iVar2 = FUN_00406440(1);
 *     *(undefined4 *)(iVar2 + 0x10) = param_3;
 *     *(undefined4 *)(iVar2 + 0x14) = 0;
 *     iVar1 = *param_1;
 *     FUN_00404100(iVar1);
 *     iVar3 = FUN_00404190(iVar1);
 *     if (iVar3 == 0) {
 *       *(int *)(iVar1 + 0x2f0) = iVar2;
 *     }
 *     else {
 *       *(int *)(iVar3 + 0x60) = iVar2;
 *     }
 *     param_1 = param_1 + 1;
 *   }
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00403650
void aitask_entity_task_batch_append(struct EntitySlot** objects,
                                     unsigned int count,
                                     struct EntitySlot* target)
{
    AITask* task;
    AITask* tail;
    struct EntitySlot* object;
    unsigned int i;

    if (count) {
        for (i = 0; i < count; i++) {
            task = aitask_cmd_alloc(1);
            task->object = target;
            task->time = 0.0f;
            object = objects[i];
            aitask_command_queue_sweep_relink(object);
            if ((tail = aitask_cmd_chain_walk(object)) != NULL)
                tail->next = task;
            else
                LEGO_FIELD(object, AITask*, 0x2f0) = task;
        }
    }
}

/* 0x004036c0 cmd_create_target (corpus name CmdCreateTarget), 64 bytes.
 * Current definition: src/game/aitask.cpp:1669
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_004036c0(int param_1,undefined4 param_2)
 *
 * {
 *   int iVar1;
 *   int iVar2;
 *
 *   iVar1 = FUN_00406440(2);
 *   *(undefined4 *)(iVar1 + 0x10) = param_2;
 *   *(undefined4 *)(iVar1 + 0x14) = 0;
 *   iVar2 = FUN_00404190(param_1);
 *   if (iVar2 != 0) {
 *     *(int *)(iVar2 + 0x60) = iVar1;
 *     return;
 *   }
 *   *(int *)(param_1 + 0x2f0) = iVar1;
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x004036c0
void aitask_cmd_create_target(struct EntitySlot* object,
                              struct EntitySlot* target)
{
    AITask* task;
    AITask* tail;

    task = aitask_cmd_alloc(2);
    task->object = target;
    task->time = 0.0f;
    if ((tail = aitask_cmd_chain_walk(object)) != NULL)
        tail->next = task;
    else
        LEGO_FIELD(object, AITask*, 0x2f0) = task;
}

/* 0x00403700 cmd_cancel_by_coords (corpus name CmdCancelByCoords), 80 bytes.
 * Current definition: src/game/aitask.cpp:1697
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00403700(int param_1,int param_2,int param_3,int param_4)
 *
 * {
 *   int *piVar1;
 *
 *   for (piVar1 = DAT_004b3354; piVar1 != (int *)0x0; piVar1 = (int *)piVar1[0x18]) {
 *     if (((*piVar1 == 7) && (piVar1[0xb] == param_1)) &&
 *        ((param_2 == 0 || ((param_3 == piVar1[8] && (param_4 == piVar1[9])))))) {
 *       piVar1[0x17] = piVar1[0x17] | 0x20;
 *     }
 *   }
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00403700
void aitask_cmd_cancel_by_coords(int zone, int matchCell, int field20,
                                 int field24)
{
    AITask* task;

    for (task = aiTaskGlobs.globalHead; task != NULL; task = task->next) {
        if (task->type == 7 && task->zone == zone &&
            (!matchCell || (field20 == task->field_20 && field24 == task->field_24)))
            task->flags |= 0x20;
    }
}

/* 0x00403750 cmd_command_kind_record_issue (corpus name CommandKindRecordIssueAndLink), 192 bytes.
 * Current definition: src/game/aitask.cpp:1710
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00403750(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 *param_4,
 *                  undefined4 param_5,undefined4 *param_6,int param_7)
 *
 * {
 *   undefined4 uVar1;
 *   int iVar2;
 *   float10 fVar3;
 *
 *   iVar2 = FUN_00406440(7);
 *   *(undefined4 *)(iVar2 + 0x20) = param_1;
 *   *(undefined4 *)(iVar2 + 0x24) = param_2;
 *   *(undefined4 *)(iVar2 + 0x28) = param_3;
 *   *(undefined4 *)(iVar2 + 8) = *param_4;
 *   uVar1 = param_4[1];
 *   *(undefined4 *)(iVar2 + 0x2c) = param_5;
 *   *(undefined4 *)(iVar2 + 0xc) = uVar1;
 *   if (param_6 == (undefined4 *)0x0) {
 *     fVar3 = (float10)FUN_004798b0(0x3ecccccd,0x3f19999a);
 *     *(float *)(iVar2 + 0x30) = (float)fVar3;
 *     fVar3 = (float10)FUN_004798b0(0x3ecccccd,0x3f19999a);
 *     *(float *)(iVar2 + 0x34) = (float)fVar3;
 *   }
 *   else {
 *     *(undefined4 *)(iVar2 + 0x30) = *param_6;
 *     *(undefined4 *)(iVar2 + 0x34) = param_6[1];
 *   }
 *   if (param_7 != 0) {
 *     *(uint *)(iVar2 + 0x5c) = *(uint *)(iVar2 + 0x5c) | 0x800;
 *   }
 *   *(undefined4 *)(iVar2 + 0x10) = 0;
 *   *(undefined4 *)(iVar2 + 0x14) = 0;
 *   FUN_00404030(iVar2,5);
 *   *(uint *)(iVar2 + 0x5c) = *(uint *)(iVar2 + 0x5c) | 0x400;
 *   FUN_00403810(param_4,param_1,param_2,param_3);
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00403750
void aitask_cmd_command_kind_record_issue(int field20, int field24, int field28,
                                          const int* blockPos, int zone,
                                          const float* randPair, int flag800)
{
    AITask* task;

    task = aitask_cmd_alloc(7);
    task->field_20 = field20;
    task->field_24 = field24;
    task->field_28 = field28;
    task->blockX = blockPos[0];
    task->blockY = blockPos[1];
    task->zone = zone;
    if (randPair != NULL) {
        task->field_30 = randPair[0];
        task->field_34 = randPair[1];
    } else {
        task->field_30 = random_float_range(0.4f, 0.6f);
        task->field_34 = random_float_range(0.4f, 0.6f);
    }
    if (flag800)
        task->flags |= 0x800;
    task->object = NULL;
    task->time = 0.0f;
    aitask_cmd_link_head(task, 5);
    task->flags |= 0x400;
    aitask_terrain_cell_record_count_update(blockPos, field20, field24, field28);
}

/* 0x00403810 terrain_cell_record_count_update (corpus name TerrainCellRecordCountUpdate), 176 bytes.
 * Current definition: src/game/aitask.cpp:1762
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00403810(undefined4 param_1,int param_2,int param_3,int param_4)
 *
 * {
 *   int iVar1;
 *   undefined4 uVar2;
 *   int iVar3;
 *
 *   iVar1 = FUN_00438a60(param_1,param_2,param_3,param_4);
 *   if (iVar1 == 0) {
 *     if (((param_3 != 0) && (param_2 == 7)) &&
 *        (iVar1 = FUN_00438a60(param_1,7,0,param_4), iVar1 != 0)) {
 *       uVar2 = FUN_00438ae0(0,param_1,7,0);
 *       iVar3 = FUN_00469bb0(uVar2);
 *       if (iVar3 == 0) {
 *         return;
 *       }
 *       do {
 *         FUN_0043a650(iVar1,7,0,param_4);
 *         iVar3 = iVar3 + -1;
 *       } while (iVar3 != 0);
 *       return;
 *     }
 *   }
 *   else {
 *     iVar1 = FUN_0043a650(iVar1,param_2,param_3,param_4);
 *     if (iVar1 != 0) {
 *       return;
 *     }
 *   }
 *   param_4 = param_3 * 0x10 + param_2 * 0xf0 + param_4;
 *   (&DAT_004b34f4)[param_4] = (&DAT_004b34f4)[param_4] + 1;
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00403810
void aitask_terrain_cell_record_count_update(const int* blockPos, int a, int b,
                                             int c)
{
    struct EntitySlot* object;
    unsigned int count;
    unsigned int i;

    if ((object = (struct EntitySlot*)ledger_stub_00438a60_SpatialObjectQueryCenterMask01(
             blockPos, a, b, c)) != NULL) {
        /* The goto keeps more of the original's block order (76.83% against
         * 62.34% for a plain else-if fall-through). */
        if (entity_action_kind_entity_create_and_command_issue(object, a, b, c) == NULL)
            goto count_it;
        return;
    } else if (b != 0 && a == 7 &&
               (object = (struct EntitySlot*)ledger_stub_00438a60_SpatialObjectQueryCenterMask01(
                    blockPos, 7, 0, c)) != NULL) {
        count = stats_instance_word_at_ec(
            ledger_stub_00438ae0_SpatialObjectQueryCenterMask02(NULL, blockPos, 7, 0));
        for (i = 0; i < count; i++)
            entity_action_kind_entity_create_and_command_issue(object, 7, 0, c);
        return;
    }
count_it:
    aiTaskGlobs.commandCounts[a * 0xf0 + b * 0x10 + c]++;
}

/* 0x004038c0 timed_task_record_initialize_and_link (corpus name TimedTaskRecordInitializeAndLink), 80 bytes.
 * Current definition: src/game/aitask.cpp:1805
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * int FUN_004038c0(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)
 *
 * {
 *   int iVar1;
 *
 *   iVar1 = FUN_00406440(5);
 *   *(undefined4 *)(iVar1 + 0x10) = param_1;
 *   *(undefined4 *)(iVar1 + 0x14) = 0;
 *   *(undefined4 *)(iVar1 + 0x20) = param_2;
 *   *(undefined4 *)(iVar1 + 0x24) = param_3;
 *   *(undefined4 *)(iVar1 + 0x28) = param_4;
 *   FUN_00404030(iVar1,10);
 *   *(uint *)(iVar1 + 0x5c) = *(uint *)(iVar1 + 0x5c) & 0xffffffef | 0x4c0;
 *   return iVar1;
 * }
 */
// FUNCTION: LEGORR 0x004038c0
AITask* aitask_timed_task_record_initialize_and_link(struct EntitySlot* object,
                                                     int field20, int field24,
                                                     int field28)
{
    AITask* task;

    task = aitask_cmd_alloc(5);
    task->object = object;
    task->time = 0.0f;
    task->field_20 = field20;
    task->field_24 = field24;
    task->field_28 = field28;
    aitask_cmd_link_head(task, 10);
    task->flags = (task->flags & ~0x10) | 0x4c0;
    return task;
}

/* 0x00403910 terrain_cell_record_counts_prune (corpus name CellRecordCountsPruneForEntity), 144 bytes.
 * Current definition: src/game/aitask.cpp:1824
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00403910(undefined4 param_1)
 *
 * {
 *   int iVar1;
 *   uint uVar2;
 *   int iVar3;
 *   uint uVar4;
 *   uint *puVar5;
 *   int local_8;
 *   uint local_4;
 *
 *   iVar3 = 0;
 *   puVar5 = &DAT_004b34f4;
 *   do {
 *     uVar2 = 0;
 *     do {
 *       uVar4 = 0;
 *       do {
 *         local_8 = 0;
 *         local_4 = 0;
 *         if (*puVar5 != 0) {
 *           do {
 *             iVar1 = FUN_00404f10(iVar3,uVar2,uVar4);
 *             if ((iVar1 == 0) || (iVar1 = FUN_0043a650(param_1,iVar3,uVar2,uVar4), iVar1 != 0)) {
 *               local_8 = local_8 + 1;
 *             }
 *             local_4 = local_4 + 1;
 *           } while (local_4 < *puVar5);
 *         }
 *         uVar4 = uVar4 + 1;
 *         *puVar5 = *puVar5 - local_8;
 *         puVar5 = puVar5 + 1;
 *       } while (uVar4 < 0x10);
 *       uVar2 = uVar2 + 1;
 *     } while (uVar2 < 0xf);
 *     iVar3 = iVar3 + 1;
 *   } while (puVar5 < &DAT_004b7ff4);
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00403910
void aitask_terrain_cell_record_counts_prune(struct EntitySlot* object)
{
    unsigned int* count;
    int a;
    unsigned int b;
    unsigned int c;
    unsigned int removed;
    unsigned int i;

    a = 0;
    count = aiTaskGlobs.commandCounts;
    do {
        for (b = 0; b < 15; b++) {
            for (c = 0; c < 16; c++, count++) {
                removed = 0;
                for (i = 0; i < *count; i++) {
                    if (!aitask_object_task_target_filter_find(a, b, c) ||
                        entity_action_kind_entity_create_and_command_issue(object, a, b, c) != NULL)
                        removed++;
                }
                *count -= removed;
            }
        }
        a++;
    } while (count < &aiTaskGlobs.commandCounts[4800]);
}

/* 0x004039a0 task_record_create_and_scan_grid (corpus name TaskRecordCreateAndScanGrid), 48 bytes.
 * Current definition: src/game/aitask.cpp:1850
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_004039a0(undefined4 param_1)
 *
 * {
 *   undefined4 uVar1;
 *
 *   uVar1 = FUN_004038c0(param_1,0,0xffffffff,0);
 *   FUN_00402060(uVar1,9);
 *   FUN_00403910(param_1);
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x004039a0
void aitask_task_record_create_and_scan_grid(struct EntitySlot* object)
{
    aitask_cmd_set_kind(aitask_timed_task_record_initialize_and_link(object, 0, -1, 0), 9);
    aitask_terrain_cell_record_counts_prune(object);
}

/* 0x004039d0 task_record_kind11_timed_queue (corpus name TaskRecordKind11TimedQueue), 64 bytes.
 * Current definition: src/game/aitask.cpp:1862
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_004039d0(undefined4 param_1)
 *
 * {
 *   int iVar1;
 *
 *   iVar1 = FUN_00406440(0x11);
 *   *(undefined4 *)(iVar1 + 0x10) = param_1;
 *   *(undefined4 *)(iVar1 + 0x14) = 0;
 *   FUN_00404030(iVar1,0);
 *   *(uint *)(iVar1 + 0x5c) = *(uint *)(iVar1 + 0x5c) & 0xffffffef | 0xc0;
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x004039d0
void aitask_task_record_kind11_timed_queue(struct EntitySlot* object)
{
    AITask* task;

    task = aitask_cmd_alloc(0x11);
    task->object = object;
    task->time = 0.0f;
    aitask_cmd_link_head(task, 0);
    task->flags = (task->flags & ~0x10) | 0xc0;
}

/* 0x00403a10 task_callback_dispatch (corpus name TaskCallbackDispatch), 64 bytes.
 * Current definition: src/game/aitask.cpp:1875
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00403a10(undefined4 param_1)
 *
 * {
 *   FUN_00406540(&LAB_00403a30,param_1);
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00403a10
void aitask_task_callback_dispatch(struct EntitySlot* object)
{
    aitask_entity_pool_callback_search(aitask_callback_00403a30, object);
}

/* 0x00403a30: an enumeration callback or entry that Ghidra merged into the
 * preceding function; the unit pushes or calls its address. Read it from
 * the disassembly of reference/LegoRR.exe. */
// FUNCTION: LEGORR 0x00403a30
int aitask_callback_00403a30(AITask* task, void* data)
{
    if (task->type == 5 && (struct EntitySlot*)data == task->object)
        task->flags |= 0x20;
    return 0;
}

/* 0x00403a50 entity_drain_command_chain (corpus name EntityDrainCommandChain), 160 bytes.
 * Current definition: src/game/aitask.cpp:1881
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00403a50(int param_1)
 *
 * {
 *   uint uVar1;
 *   int iVar2;
 *   int iVar3;
 *
 *   if (((byte)DAT_004b8060 & 2) == 0) {
 *     iVar3 = *(int *)(param_1 + 0x2f0);
 *     while (iVar3 != 0) {
 *       uVar1 = *(uint *)(iVar3 + 0x5c);
 *       iVar2 = *(int *)(iVar3 + 0x60);
 *       if (((uVar1 & 0x10) == 0) || ((uVar1 & 0x20) != 0)) {
 *         if (((uVar1 & 0x2000) == 0) || ((uVar1 & 0x20) != 0)) {
 *           FUN_00406480(iVar3,0);
 *           iVar3 = iVar2;
 *         }
 *         else {
 *           FUN_00404070(iVar3);
 *           iVar3 = iVar2;
 *         }
 *       }
 *       else {
 *         FUN_00404030(iVar3,*(undefined4 *)(iVar3 + 0x4c));
 *         iVar3 = iVar2;
 *       }
 *     }
 *     *(undefined4 *)(param_1 + 0x2f0) = 0;
 *     FUN_00406540(&LAB_00403ad0,param_1);
 *   }
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00403a50
void aitask_entity_drain_command_chain(struct EntitySlot* object)
{
    AITask* task;
    AITask* next;

    if (!(aiTaskGlobs.flags & 2)) {
        task = LEGO_FIELD(object, AITask*, 0x2f0);
        while (task != NULL) {
            next = task->next;
            if ((task->flags & 0x10) && !(task->flags & 0x20))
                aitask_cmd_link_head(task, task->priority);
            else if ((task->flags & 0x2000) && !(task->flags & 0x20))
                aitask_cmd_link_tail(task);
            else
                aitask_cmd_free(task, 0);
            task = next;
        }
        LEGO_FIELD(object, AITask*, 0x2f0) = NULL;
        aitask_entity_pool_callback_search(aitask_callback_00403ad0, object);
    }
}

/* 0x00403ad0: an enumeration callback or entry that Ghidra merged into the
 * preceding function; the unit pushes or calls its address. Read it from
 * the disassembly of reference/LegoRR.exe. */
// FUNCTION: LEGORR 0x00403ad0
int aitask_callback_00403ad0(AITask* task, void* data)
{
    if ((struct EntitySlot*)data == task->object)
        task->flags |= 0x20;
    return 0;
}

/* 0x00403af0 entity_task_state_update_after_cleanup (corpus name EntityTaskStateUpdateAfterCleanup), 80 bytes.
 * Current definition: src/game/aitask.cpp:1911
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00403af0(int *param_1,int param_2)
 *
 * {
 *   int *piVar1;
 *
 *   piVar1 = (int *)param_1[0xbc];
 *   if (piVar1 != (int *)0x0) {
 *     if (param_2 == 0) {
 *       piVar1[0x17] = piVar1[0x17] & 0xfffffffe;
 *     }
 *     else if (*piVar1 == 0) {
 *       FUN_004041e0(param_1);
 *       if (*param_1 == 3) {
 *         FUN_00452060(7,param_1,0,0);
 *         return;
 *       }
 *     }
 *   }
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00403af0
void aitask_entity_task_state_update_after_cleanup(struct EntitySlot* object,
                                                   int remove)
{
    AITask* task;

    if ((task = LEGO_FIELD(object, AITask*, 0x2f0)) != NULL) {
        if (remove) {
            if (task->type == 0) {
                aitask_entity_command_chain_head_remove(object);
                if (LEGO_FIELD(object, int, 0x0) == 3)
                    cmd_produce(7, object, 0, NULL);
            }
        } else {
            task->flags &= ~1;
        }
    }
}

/* 0x00403b40 entity_pending_task_flag_clear (corpus name EntityPendingTaskFlagClear), 32 bytes.
 * Current definition: src/game/aitask.cpp:1930
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00403b40(int param_1)
 *
 * {
 *   uint *puVar1;
 *
 *   if (*(int *)(param_1 + 0x2f0) != 0) {
 *     puVar1 = (uint *)(*(int *)(param_1 + 0x2f0) + 0x5c);
 *     *puVar1 = *puVar1 & 0xfffffffe;
 *   }
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00403b40
void aitask_entity_pending_task_flag_clear(struct EntitySlot* object)
{
    AITask* task;

    if ((task = LEGO_FIELD(object, AITask*, 0x2f0)) != NULL)
        task->flags &= ~1;
}

/* 0x00403b60 entity_activity_records_refresh (corpus name EntityActivityRecordsRefresh), 160 bytes.
 * Current definition: src/game/aitask.cpp:1938
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00403b60(int param_1)
 *
 * {
 *   undefined4 uVar1;
 *   uint uVar2;
 *   undefined4 *puVar3;
 *
 *   if ((((*(uint *)(param_1 + 0x3e0) & 0x400) != 0) && ((*(uint *)(param_1 + 0x3e0) & 0x2000) == 0))
 *      && (uVar2 = 0, *(int *)(param_1 + 0x324) != 0)) {
 *     puVar3 = (undefined4 *)(param_1 + 0x308);
 *     do {
 *       uVar1 = *puVar3;
 *       FUN_00403c00(param_1,5,uVar1);
 *       FUN_00403c00(param_1,6,uVar1);
 *       FUN_00403c00(param_1,9,uVar1);
 *       FUN_00403c00(param_1,0xf,uVar1);
 *       FUN_00403c00(param_1,7,uVar1);
 *       FUN_00403c00(param_1,4,0);
 *       FUN_00403c00(param_1,0x14,uVar1);
 *       uVar2 = uVar2 + 1;
 *       puVar3 = puVar3 + 1;
 *     } while (uVar2 < *(uint *)(param_1 + 0x324));
 *   }
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00403b60
void aitask_entity_activity_records_refresh(struct EntitySlot* object)
{
    unsigned int i;
    struct EntitySlot* other;

    if ((LEGO_FIELD(object, unsigned int, 0x3e0) & 0x400) &&
        !(LEGO_FIELD(object, unsigned int, 0x3e0) & 0x2000)) {
        for (i = 0; i < LEGO_FIELD(object, unsigned int, 0x324); i++) {
            other = (&LEGO_FIELD(object, struct EntitySlot*, 0x308))[i];
            aitask_entity_activity_record_remove_by_type(object, 5, other);
            aitask_entity_activity_record_remove_by_type(object, 6, other);
            aitask_entity_activity_record_remove_by_type(object, 9, other);
            aitask_entity_activity_record_remove_by_type(object, 0xf, other);
            aitask_entity_activity_record_remove_by_type(object, 7, other);
            aitask_entity_activity_record_remove_by_type(object, 4, NULL);
            aitask_entity_activity_record_remove_by_type(object, 0x14, other);
        }
    }
}

/* 0x00403c00 entity_activity_record_remove_by_type (corpus name EntityActivityRecordRemoveByType), 272 bytes.
 * Current definition: src/game/aitask.cpp:1970
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00403c00(int param_1,int param_2,undefined4 param_3)
 *
 * {
 *   int *piVar1;
 *   int iVar2;
 *   undefined4 uVar3;
 *   int *piVar4;
 *   int *piVar5;
 *
 *   if (((byte)DAT_004b8060 & 2) != 0) {
 *     return;
 *   }
 *   piVar4 = (int *)0x0;
 *   piVar5 = *(int **)(param_1 + 0x2f0);
 *   if (piVar5 == (int *)0x0) {
 *     return;
 *   }
 *   while (param_2 != *piVar5) {
 *     piVar1 = piVar5 + 0x18;
 *     piVar4 = piVar5;
 *     piVar5 = (int *)*piVar1;
 *     if ((int *)*piVar1 == (int *)0x0) {
 *       return;
 *     }
 *   }
 *   iVar2 = *piVar5;
 *   if (iVar2 == 5) {
 * LAB_00403c60:
 *     if (iVar2 == 7) goto LAB_00403c65;
 *   }
 *   else {
 *     if (iVar2 != 7) {
 *       if ((((iVar2 != 6) && (iVar2 != 3)) && (iVar2 != 9)) && ((iVar2 != 0xf && (iVar2 != 0x14))))
 *       goto LAB_00403cd1;
 *       goto LAB_00403c60;
 *     }
 * LAB_00403c65:
 *     FUN_00403750(piVar5[8],piVar5[9],piVar5[10],piVar5 + 2,piVar5[0xb],piVar5 + 0xc,
 *                  piVar5[0x17] & 0x800);
 *   }
 *   if (*piVar5 == 9) {
 *     FUN_00432e00(piVar5 + 2,0);
 *     uVar3 = FUN_00431570(piVar5[2],piVar5[3]);
 *     FUN_0044f570(uVar3);
 *   }
 *   FUN_00402720(param_3,0x43160000);
 *   FUN_00402850(param_3);
 * LAB_00403cd1:
 *   if (piVar4 == (int *)0x0) {
 *     *(int *)(param_1 + 0x2f0) = piVar5[0x18];
 *     FUN_00406480(piVar5,0);
 *     return;
 *   }
 *   piVar4[0x18] = piVar5[0x18];
 *   FUN_00406480(piVar5,0);
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00403c00
void aitask_entity_activity_record_remove_by_type(struct EntitySlot* object,
                                                  int type,
                                                  struct EntitySlot* target)
{
    AITask* task;
    AITask* prev;

    if (aiTaskGlobs.flags & 2)
        return;
    prev = NULL;
    for (task = LEGO_FIELD(object, AITask*, 0x2f0); task != NULL; task = task->next) {
        if (type == task->type) {
            if (task->type == 5 || task->type == 7 || task->type == 6 ||
                task->type == 3 || task->type == 9 || task->type == 0xf ||
                task->type == 0x14) {
                if (task->type == 7)
                    aitask_cmd_command_kind_record_issue(
                        task->field_20, task->field_24, task->field_28,
                        &task->blockX, task->zone, &task->field_30,
                        task->flags & 0x800);
                if (task->type == 9) {
                    level_tile_mark_current_target(&task->blockX, 0);
                    terrain_map_block_highlight_clear(AITASK_LEGO_GET_MAP(), task->blockX,
                                                      task->blockY);
                }
                aitask_cmd_create_task(target, 150.0f);
                aitask_cmd_minion_companion(target);
            }
            if (prev != NULL)
                prev->next = task->next;
            else
                LEGO_FIELD(object, AITask*, 0x2f0) = task->next;
            aitask_cmd_free(task, 0);
            return;
        }
        prev = task;
    }
}

/* 0x00403d10 entity_command_unlink_by_kind (corpus name EntityCommandUnlinkByKind), 480 bytes.
 * Current definition: src/game/aitask.cpp:2144
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00403d10(int *param_1,int param_2,undefined4 *param_3,int param_4)
 *
 * {
 *   int *piVar1;
 *   bool bVar2;
 *   int *piVar3;
 *   int iVar4;
 *   int iVar5;
 *   uint uVar6;
 *   int *piVar7;
 *   int *piVar8;
 *
 *   piVar1 = param_1;
 *   if (((byte)DAT_004b8060 & 2) == 0) {
 *     if ((param_2 == 3) && (iVar4 = FUN_00438ae0(param_1,0,*param_3,param_3[0xcc]), iVar4 != 0)) {
 *       FUN_00452060(0x18,param_1,iVar4,0);
 *     }
 *     piVar3 = (int *)param_1[0xbc];
 *     piVar8 = (int *)0x0;
 *     if ((int *)param_1[0xbc] != (int *)0x0) {
 * LAB_00403d6a:
 *       piVar7 = piVar3;
 *       if (param_2 != *piVar7) goto code_r0x00403d74;
 *       param_1 = (undefined4 *)0x0;
 *       bVar2 = false;
 *       if (param_4 != 0) {
 *         if (piVar8 == (int *)0x0) {
 *           piVar1[0xbc] = piVar7[0x18];
 *         }
 *         else {
 *           piVar8[0x18] = piVar7[0x18];
 *         }
 *       }
 *       if (*piVar7 == 7) {
 *         iVar4 = piVar7[0xb];
 *         bVar2 = true;
 *         param_1 = param_3;
 *         iVar5 = FUN_00408d70(iVar4,*param_3,param_3[1]);
 *         if (iVar5 == 0) {
 *           FUN_00403700(iVar4,1,*param_3,param_3[1]);
 *           bVar2 = false;
 *         }
 *       }
 *       else {
 *         iVar4 = param_2;
 *         if (*piVar7 == 0xc) {
 *           if (*piVar1 == 1) {
 *             for (uVar6 = *(uint *)(*(int *)(DAT_005560d4 + 0xb8) + 8 +
 *                                   (piVar7[3] * *(int *)(DAT_005560d4 + 0x14) + piVar7[2]) * 0x48) &
 *                          3; uVar6 != 0; uVar6 = uVar6 - 1) {
 *               FUN_00432c90(piVar7 + 2);
 *               FUN_00452060(piVar7[0x15],0,0,piVar7 + 2);
 *             }
 *           }
 *           piVar1 = piVar7 + 2;
 *           FUN_00432c90(piVar1);
 *           FUN_00452060(piVar7[0x15],0,0,piVar1);
 *           FUN_00432710(piVar1,0);
 *           FUN_00433120(piVar1,0);
 *         }
 *         else if (param_2 == 0x17) {
 *           FUN_0043f5b0(piVar1,piVar7[0xf]);
 *           FUN_0046ac60(0x18,1,0);
 *         }
 *       }
 *       if (param_4 != 0) {
 *         FUN_00406480(piVar7,0);
 *       }
 *       if (bVar2) {
 *         FUN_00408ce0(iVar4,param_1);
 *       }
 *     }
 * LAB_00403ecc:
 *     if (param_2 == 3) {
 *       FUN_00403ef0(param_3);
 *     }
 *   }
 *   return;
 * code_r0x00403d74:
 *   piVar3 = (int *)piVar7[0x18];
 *   piVar8 = piVar7;
 *   if ((int *)piVar7[0x18] == (int *)0x0) goto LAB_00403ecc;
 *   goto LAB_00403d6a;
 * }
 */
// FUNCTION: LEGORR 0x00403d10
void aitask_entity_command_unlink_by_kind(struct EntitySlot* object, int type,
                                          struct EntitySlot* target, int unlink)
{
    AITask* task;
    AITask* prev;
    struct EntitySlot* found;
    struct EntitySlot* appendObject;
    int append;
    int zone;
    unsigned int count;
    unsigned int i;

    if (aiTaskGlobs.flags & 2)
        return;
    prev = NULL;
    if (type == 3 &&
        (found = (struct EntitySlot*)ledger_stub_00438ae0_SpatialObjectQueryCenterMask02(
             object, NULL, LEGO_FIELD(target, int, 0x0),
             LEGO_FIELD(target, int, 0x330))) != NULL)
        cmd_produce(0x18, object, (int)(size_t)found, NULL);
    for (task = LEGO_FIELD(object, AITask*, 0x2f0); task != NULL; task = task->next) {
        if (type == task->type) {
            appendObject = NULL;
            append = 0;
            if (unlink) {
                if (prev != NULL)
                    prev->next = task->next;
                else
                    LEGO_FIELD(object, AITask*, 0x2f0) = task->next;
            }
            if (task->type == 7) {
                zone = task->zone;
                append = 1;
                appendObject = target;
                if (!terrain_descriptor_coordinate_count_limit_check(
                        zone, LEGO_FIELD(target, int, 0x0), LEGO_FIELD(target, int, 0x4))) {
                    aitask_cmd_cancel_by_coords(zone, 1, LEGO_FIELD(target, int, 0x0),
                                                LEGO_FIELD(target, int, 0x4));
                    append = 0;
                }
            } else if (task->type == 0xc) {
                if (LEGO_FIELD(object, int, 0x0) == 1) {
                    count = LEGO_LEVEL_PTR(Lego_Block, legoGlobs_currLevel->blocks)
                                [task->blockY * legoGlobs_currLevel->width + task->blockX]
                                    .flags1 & 3;
                    for (i = 0; i < count; i++) {
                        terrain_cell_state_update_current(&task->blockX);
                        cmd_produce((int)(size_t)task->eventPayload, NULL, 0, &task->blockX);
                    }
                }
                terrain_cell_state_update_current(&task->blockX);
                cmd_produce((int)(size_t)task->eventPayload, NULL, 0, &task->blockX);
                terrain_cell_flag_80000_clear_or_set_current(&task->blockX, 0);
                terrain_cell_flag_10000000_set_current(&task->blockX, 0);
            } else if (type == 0x17) {
                entity_linked_activity_event_dispatch(object, task->field_3c);
                advisor_text_animation_update(0x18, 1, 0);
            }
            if (unlink)
                aitask_cmd_free(task, 0);
            if (append)
                cmd_operation_group_append_object(zone, appendObject);
            break;
        }
        prev = task;
    }
    if (type == 3)
        aitask_pending_record_callback_dispatch(target);
}

/* 0x00403ef0 pending_record_callback_dispatch (corpus name PendingRecordCallbackDispatch), 64 bytes.
 * Current definition: src/game/aitask.cpp:2231
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00403ef0(undefined4 param_1)
 *
 * {
 *   FUN_00406540(&LAB_00403f10,param_1);
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00403ef0
void aitask_pending_record_callback_dispatch(struct EntitySlot* object)
{
    aitask_entity_pool_callback_search(aitask_callback_00403f10, object);
}

/* 0x00403f10: an enumeration callback or entry that Ghidra merged into the
 * preceding function; the unit pushes or calls its address. Read it from
 * the disassembly of reference/LegoRR.exe. */
// FUNCTION: LEGORR 0x00403f10
int aitask_callback_00403f10(AITask* task, void* data)
{
    if (task->type == 3 && task->object == (struct EntitySlot*)data)
        task->flags |= 0x20;
    return 0;
}

/* 0x00403f30 ai_idle_chain_record_attach (corpus name EntityActivityListRegister), 48 bytes.
 * Current definition: src/game/aitask.cpp:2262
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00403f30(int param_1)
 *
 * {
 *   int iVar1;
 *
 *   if ((*(int **)(param_1 + 0x2f0) == (int *)0x0) || (**(int **)(param_1 + 0x2f0) != 0xe)) {
 *     iVar1 = FUN_00406440(0xe);
 *     *(undefined4 *)(iVar1 + 0x60) = *(undefined4 *)(param_1 + 0x2f0);
 *     *(int *)(param_1 + 0x2f0) = iVar1;
 *   }
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00403f30
void aitask_ai_idle_chain_record_attach(struct EntitySlot* object)
{
    AITask* task;

    if (LEGO_FIELD(object, AITask*, 0x2f0) == NULL ||
        LEGO_FIELD(object, AITask*, 0x2f0)->type != 0xe) {
        task = aitask_cmd_alloc(0xe);
        task->next = LEGO_FIELD(object, AITask*, 0x2f0);
        LEGO_FIELD(object, AITask*, 0x2f0) = task;
    }
}

/* 0x00403f60 task_record_kind14_remove_if_head (corpus name TaskRecordKind14RemoveIfHead), 208 bytes.
 * Current definition: src/game/aitask.cpp:2274
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00403f60(int param_1)
 *
 * {
 *   if ((*(int **)(param_1 + 0x2f0) != (int *)0x0) && (**(int **)(param_1 + 0x2f0) == 0xe)) {
 *     FUN_004041e0(param_1);
 *   }
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00403f60
void aitask_task_record_kind14_remove_if_head(struct EntitySlot* object)
{
    if (LEGO_FIELD(object, AITask*, 0x2f0) != NULL &&
        LEGO_FIELD(object, AITask*, 0x2f0)->type == 0xe)
        aitask_entity_command_chain_head_remove(object);
}

/* 0x00403f80: an enumeration callback or entry that Ghidra merged into the
 * preceding function; the unit pushes or calls its address. Read it from
 * the disassembly of reference/LegoRR.exe. */
// FUNCTION: LEGORR 0x00403f80
int aitask_callback_00403f80(const void* a, const void* b)
{
    AITask* taskA;
    AITask* taskB;
    unsigned int selA;
    unsigned int selB;

    taskA = *(AITask* const*)a;
    taskB = *(AITask* const*)b;
    selA = taskA->flags & 0x100;
    if (selA && !(taskB->flags & 0x100))
        return -1;
    selB = taskB->flags & 0x100;
    if (selB && !selA)
        return 1;
    if (selA && selB) {
        if (taskA->field_50 > taskB->field_50)
            return -1;
        if (taskA->field_50 < taskB->field_50)
            return 1;
    }
    if (taskA->priorityValue < 0)
        taskA->priorityValue = 0;
    if (taskB->priorityValue < 0)
        taskB->priorityValue = 0;
    if (taskA->priorityValue > 99)
        taskA->priorityValue = 99;
    if (taskB->priorityValue > 99)
        taskB->priorityValue = 99;
    if (taskA->priorityValue > taskB->priorityValue)
        return -1;
    return taskB->priorityValue > taskA->priorityValue;
}

/* 0x00404030 cmd_link_head (corpus name CmdLinkHead), 64 bytes.
 * Current definition: src/game/aitask.cpp:2282
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00404030(int param_1,int param_2)
 *
 * {
 *   *(int *)(param_1 + 0x60) = DAT_004b3354;
 *   DAT_004b3354 = param_1;
 *   *(undefined4 *)(param_1 + 0x14) = 0x44bb8000;
 *   *(uint *)(param_1 + 0x5c) = *(uint *)(param_1 + 0x5c) & 0xfffffffe | 0x10;
 *   *(undefined4 *)(param_1 + 0x1c) = (&DAT_004b32e8)[param_2];
 *   *(int *)(param_1 + 0x4c) = param_2;
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00404030
void aitask_cmd_link_head(AITask* task, int priority)
{
    task->next = aiTaskGlobs.globalHead;
    aiTaskGlobs.globalHead = task;
    task->time = 1500.0f;
    task->flags = (task->flags & ~1) | 0x10;
    task->priorityValue = aiTaskGlobs.priorityValues[priority];
    task->priority = priority;
}

/* 0x00404070 cmd_link_tail (corpus name CmdLinkTail), 48 bytes.
 * Current definition: src/game/aitask.cpp:2292
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00404070(int param_1)
 *
 * {
 *   *(int *)(param_1 + 0x60) = DAT_004b3358;
 *   DAT_004b3358 = param_1;
 *   *(undefined4 *)(param_1 + 0x14) = 0x44bb8000;
 *   *(uint *)(param_1 + 0x5c) = *(uint *)(param_1 + 0x5c) & 0xffffffee;
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00404070
void aitask_cmd_link_tail(AITask* task)
{
    task->next = aiTaskGlobs.globalTail;
    aiTaskGlobs.globalTail = task;
    task->time = 1500.0f;
    task->flags &= ~0x11;
}

/* The LegoObject fields (LegoObject.c, GAME-023) that groups C and D read,
 * at their Win32 offsets. The port's EntitySlot has another layout; the unit
 * receives and passes these records as EntitySlot pointers and views them
 * through AITASK_OBJ. Only the matching and Win32 reference builds run this
 * code. A struct member, not an offset cast, is what lets MSVC 5.0 use the
 * field as an x87 memory operand as the original does. */
typedef struct AITaskObjectStats AITaskObjectStats;
struct AITaskObjectStats {
    unsigned char pad_000[0xe8];
    float field_e8;                     /* +0x0e8 health threshold */
};
typedef struct AITaskObject AITaskObject;
struct AITaskObject {
    int type;                           /* +0x000 */
    int field_4;                        /* +0x004 object id within the type */
    unsigned char pad_008[0x018 - 0x008];
    void* field_18;                     /* +0x018 */
    unsigned char pad_01c[0x2f0 - 0x01c];
    AITask* aiTask;                     /* +0x2f0 the entity's task chain */
    unsigned char pad_2f4[0x2fc - 0x2f4];
    AITaskObject* field_2fc;            /* +0x2fc */
    unsigned char pad_300[0x304 - 0x300];
    void* field_304;                    /* +0x304 */
    AITaskObject* interactObject;       /* +0x308 */
    unsigned char pad_30c[0x330 - 0x30c];
    int field_330;                      /* +0x330 object level */
    AITaskObjectStats* stats;           /* +0x334 */
    float field_338;                    /* +0x338 idle time */
    unsigned char pad_33c[0x340 - 0x33c];
    float health;                       /* +0x340 */
    float energy;                       /* +0x344 */
    unsigned char pad_348[0x364 - 0x348];
    void* field_364;                    /* +0x364 */
    unsigned char pad_368[0x36c - 0x368];
    AITaskObject* driveObject;          /* +0x36c */
    unsigned char pad_370[0x3d4 - 0x370];
    float field_3d4;                    /* +0x3d4 */
    unsigned char pad_3d8[0x3e0 - 0x3d8];
    unsigned int flags1;                /* +0x3e0 */
    unsigned int flags2;                /* +0x3e4 */
    unsigned int flags3;                /* +0x3e8 */
    unsigned int flags4;                /* +0x3ec */
    unsigned int abilityFlags;          /* +0x3f0 */
};
#define AITASK_OBJ(ptr) ((AITaskObject*)(ptr))

/* Maths.h's Point2I and Point2F: block and world positions. */
typedef struct AITaskPoint2I {
    int x, y;
} AITaskPoint2I;
typedef struct AITaskPoint2F {
    float x, y;
} AITaskPoint2F;

/* Map3D's vertex position (Vector3F; terrain_map.h's TerrainMapVector3). */
struct TerrainMapVector3 {
    float x, y, z;
};

/* 0x004040a0 entity_task_queue_flush (corpus name EntityTaskQueueFlush), 96 bytes.
 * Current definition: src/game/aitask.cpp:2302
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_004040a0(int param_1)
 *
 * {
 *   if (((byte)DAT_004b8060 & 6) == 0) {
 *     FUN_00403c00(param_1,0,0);
 *     if (((*(uint *)(param_1 + 0x3e0) & 0x400) != 0) && (**(int **)(param_1 + 0x308) == 8)) {
 *       FUN_00403c00(param_1,9,*(int **)(param_1 + 0x308));
 *     }
 *     FUN_00404100(param_1);
 *     FUN_004063e0(param_1);
 *   }
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x004040a0
void aitask_entity_task_queue_flush(struct EntitySlot* object)
{
    if (!(aiTaskGlobs.flags & 6)) {
        aitask_entity_activity_record_remove_by_type(object, 0, NULL);
        if ((AITASK_OBJ(object)->flags1 & 0x400) &&
            AITASK_OBJ(object)->interactObject->type == 8) {
            aitask_entity_activity_record_remove_by_type(
                object, 9, (struct EntitySlot*)AITASK_OBJ(object)->interactObject);
        }
        aitask_command_queue_sweep_relink(object);
        aitask_pending_record_entity_reference_clear(object);
    }
}

/* 0x00404100 command_queue_sweep_relink (corpus name CommandQueueSweepRelink), 144 bytes.
 * Current definition: src/game/aitask.cpp:2338
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00404100(int param_1)
 *
 * {
 *   uint uVar1;
 *   int iVar2;
 *   int iVar3;
 *   int iVar4;
 *
 *   iVar3 = 0;
 *   iVar2 = *(int *)(param_1 + 0x2f0);
 *   while (iVar4 = iVar2, iVar4 != 0) {
 *     uVar1 = *(uint *)(iVar4 + 0x5c);
 *     iVar2 = *(int *)(iVar4 + 0x60);
 *     if ((uVar1 & 4) == 0) {
 *       if (((uVar1 & 0x10) == 0) || ((uVar1 & 0x20) != 0)) {
 *         if (((uVar1 & 0x2000) == 0) || ((uVar1 & 0x20) != 0)) {
 *           if (iVar3 == 0) {
 *             *(undefined4 *)(iVar4 + 0x60) = 0;
 *             iVar3 = iVar4;
 *           }
 *           else {
 *             *(int *)(iVar3 + 0x60) = iVar4;
 *             *(undefined4 *)(iVar4 + 0x60) = 0;
 *           }
 *         }
 *         else {
 *           *(int *)(iVar4 + 0x60) = DAT_004b3358;
 *           DAT_004b3358 = iVar4;
 *         }
 *       }
 *       else {
 *         *(int *)(iVar4 + 0x60) = DAT_004b3354;
 *         DAT_004b3354 = iVar4;
 *       }
 *     }
 *     else {
 *       FUN_00406480(iVar4,0);
 *     }
 *   }
 *   *(int *)(param_1 + 0x2f0) = iVar3;
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00404100
void aitask_command_queue_sweep_relink(struct EntitySlot* object)
{
    AITask* task;
    AITask* next;
    AITask* kept;
    unsigned int flags;

    kept = NULL;
    task = AITASK_OBJ(object)->aiTask;
    while (task != NULL) {
        flags = task->flags;
        next = task->next;
        if (!(flags & 4)) {
            if ((flags & 0x10) && !(flags & 0x20)) {
                task->next = aiTaskGlobs.globalHead;
                aiTaskGlobs.globalHead = task;
            } else if ((flags & 0x2000) && !(flags & 0x20)) {
                task->next = aiTaskGlobs.globalTail;
                aiTaskGlobs.globalTail = task;
            } else if (kept != NULL) {
                kept->next = task;
                task->next = NULL;
            } else {
                kept = task;
                task->next = NULL;
            }
        } else {
            aitask_cmd_free(task, 0);
        }
        task = next;
    }
    AITASK_OBJ(object)->aiTask = kept;
}

/* 0x00404190 cmd_chain_walk (corpus name CmdChainWalk), 32 bytes.
 * Current definition: src/game/aitask.cpp:2371
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00404190(int param_1)
 *
 * {
 *   int iVar1;
 *
 *   if (*(int *)(param_1 + 0x2f0) != 0) {
 *     for (iVar1 = *(int *)(*(int *)(param_1 + 0x2f0) + 0x60); iVar1 != 0;
 *         iVar1 = *(int *)(iVar1 + 0x60)) {
 *     }
 *   }
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00404190
AITask* aitask_cmd_chain_walk(struct EntitySlot* object)
{
    AITask* task;

    task = AITASK_OBJ(object)->aiTask;
    if (task != NULL) {
        while (task->next != NULL)
            task = task->next;
    }
    return task;
}

/* 0x004041b0 entity_task_chain_contains_kind (corpus name EntityTaskChainContainsKind), 48 bytes.
 * Current definition: src/game/aitask.cpp:2386
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * undefined4 FUN_004041b0(int param_1,int param_2)
 *
 * {
 *   int *piVar1;
 *
 *   piVar1 = *(int **)(param_1 + 0x2f0);
 *   if (piVar1 == (int *)0x0) {
 *     return 0;
 *   }
 *   do {
 *     if (param_2 == *piVar1) {
 *       return 1;
 *     }
 *     piVar1 = (int *)piVar1[0x18];
 *   } while (piVar1 != (int *)0x0);
 *   return 0;
 * }
 *
 *
 * // ==== FUN_004041e0 @ 004041E0 ====
 *
 *
 * undefined4 FUN_004041e0(int param_1)
 *
 * {
 *   int iVar1;
 *   uint uVar2;
 *
 *   iVar1 = *(int *)(param_1 + 0x2f0);
 *   if (iVar1 != 0) {
 *     *(undefined4 *)(param_1 + 0x2f0) = *(undefined4 *)(iVar1 + 0x60);
 *     uVar2 = *(uint *)(iVar1 + 0x5c);
 *     if (((uVar2 & 0x10) != 0) && ((uVar2 & 0x20) == 0)) {
 *       *(int *)(iVar1 + 0x60) = DAT_004b3354;
 *       DAT_004b3354 = iVar1;
 *       return *(undefined4 *)(param_1 + 0x2f0);
 *     }
 *     if (((uVar2 & 0x2000) != 0) && ((uVar2 & 0x20) == 0)) {
 *       *(int *)(iVar1 + 0x60) = DAT_004b3358;
 *       DAT_004b3358 = iVar1;
 *       return *(undefined4 *)(param_1 + 0x2f0);
 *     }
 *     FUN_00406480(iVar1,0);
 *   }
 *   return *(undefined4 *)(param_1 + 0x2f0);
 * }
 */
// FUNCTION: LEGORR 0x004041b0
int aitask_entity_task_chain_contains_kind(struct EntitySlot* object, int type)
{
    AITask* task;

    for (task = AITASK_OBJ(object)->aiTask; task != NULL; task = task->next) {
        if (task->type == type)
            return 1;
    }
    return 0;
}

/* 0x004041e0 entity_command_chain_head_remove (corpus name FUN_004041e0), 3104 bytes.
 * Current definition: src/game/aitask.cpp:2400
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * (not in the corpus)
 */
// FUNCTION: LEGORR 0x004041e0
AITask* aitask_entity_command_chain_head_remove(struct EntitySlot* object)
{
    AITask* task;
    unsigned int flags;

    task = AITASK_OBJ(object)->aiTask;
    if (task != NULL) {
        AITASK_OBJ(object)->aiTask = task->next;
        flags = task->flags;
        if ((flags & 0x10) && !(flags & 0x20)) {
            task->next = aiTaskGlobs.globalHead;
            aiTaskGlobs.globalHead = task;
            return AITASK_OBJ(object)->aiTask;
        }
        if ((flags & 0x2000) && !(flags & 0x20)) {
            task->next = aiTaskGlobs.globalTail;
            aiTaskGlobs.globalTail = task;
            return AITASK_OBJ(object)->aiTask;
        }
        aitask_cmd_free(task, 0);
    }
    return AITASK_OBJ(object)->aiTask;
}

/* 0x00404250: an enumeration callback or entry that Ghidra merged into the
 * preceding function; the unit pushes or calls its address. Read it from
 * the disassembly of reference/LegoRR.exe. */
// FUNCTION: LEGORR 0x00404250
int aitask_cmd_pool_timer_delta_apply(AITask* task, void* data)
{
    float* elapsed = (float*)data;

    if (!(task->flags & 0x80))
        task->time -= *elapsed;
    task->timeIn -= *elapsed;
    return 0;
}

/* 0x00404280: an enumeration callback or entry that Ghidra merged into the
 * preceding function; the unit pushes or calls its address. Read it from
 * the disassembly of reference/LegoRR.exe. */
// FUNCTION: LEGORR 0x00404280
int aitask_cmd_entity_command_chain_tick(struct EntitySlot* object, void* data)
{
    AITask* task;
    AITask* prev;
    int remove;
    int result;

    aiTaskGlobs.flags |= 4;
    if ((AITASK_OBJ(object)->flags3 & 0x400000) &&
        AITASK_OBJ(object)->health > s_rdata_0049e014 &&
        !lego_entity_matches_current_selection_or_owner(object) &&
        !(AITASK_OBJ(object)->driveObject != NULL && AITASK_OBJ(object)->type == 2) &&
        AITASK_OBJ(object)->field_304 == NULL &&
        !(AITASK_OBJ(object)->flags2 & 0x8004000) &&
        !(AITASK_OBJ(object)->flags1 & 0x800000)) {
        remove = 0;
        task = AITASK_OBJ(object)->aiTask;
        if (task != NULL) {
            AITASK_OBJ(object)->field_338 = 0.0f;
            if (task->flags & 0x20) {
                aitask_entity_command_chain_head_remove(object);
                entity_action_sequence_reset(object, 0);
            } else {
                if (task->type == 0xd && task->time <= s_rdata_0049e014) {
                    aitask_entity_command_chain_head_remove(object);
                } else if (task->type == 1) {
                    if (task->time <= s_rdata_0049e014) {
                        AITaskPoint2I pos;
                        AITaskPoint2I targetPos;
                        entity_position_to_terrain_cell_adapter(object, &pos.x, &pos.y);
                        entity_position_to_terrain_cell_adapter(task->object, &targetPos.x,
                                                                &targetPos.y);
                        if (pos.x != targetPos.x || pos.y != targetPos.y)
                            aitask_object_path_submit_to_grid_target(object, &targetPos.x, NULL);
                        task->time = 50.0f;
                    }
                } else if (task->type == 2) {
                    if (task->time <= s_rdata_0049e014) {
                        if (AITASK_OBJ(task->object)->health >
                            AITASK_OBJ(task->object)->stats->field_e8) {
                            AITaskPoint2F wPos;
                            AITaskPoint2F targetWPos;
                            float dx, dy;
                            entity_transform_position_get_outputs(object, &wPos.x, &wPos.y);
                            entity_transform_position_get_outputs(task->object, &targetWPos.x,
                                                                  &targetWPos.y);
                            dx = wPos.x - targetWPos.x;
                            dy = wPos.y - targetWPos.y;
                            if (sqrt(dx * dx + dy * dy) > s_rdata_0049e01c) {
                                AITaskPoint2I pos;
                                AITaskPoint2I targetPos;
                                entity_position_to_terrain_cell_adapter(object, &pos.x,
                                                                        &pos.y);
                                entity_position_to_terrain_cell_adapter(
                                    task->object, &targetPos.x, &targetPos.y);
                                if (pos.x != targetPos.x || pos.y != targetPos.y)
                                    aitask_object_path_submit_to_grid_target(object, &targetPos.x,
                                                                             NULL);
                                task->time = 50.0f;
                            } else if (entity_face_target_action_start(object, task->object, 2)) {
                                task->time = 50.0f;
                            } else {
                                task->time = 25.0f;
                            }
                        } else {
                            task->flags |= 0x20;
                        }
                    }
                } else if (!(task->flags & 1) &&
                           !(AITASK_OBJ(object)->flags3 & 0x200000)) {
                    result = 1;
                    switch (task->type) {
                    case 3:
                        result = entity_gated_step_command_issue(object, task->object);
                        break;
                    case 5:
                        result = entity_deposit_activity_gate(object, task->object);
                        break;
                    case 7:
                        result = aitask_cell_step_command_issue_adapter(
                            object, &task->blockX, &task->field_30, 1, task->flags & 0x800);
                        break;
                    case 6:
                        result = aitask_cell_step_command_issue_adapter(
                            object, &task->blockX, &task->field_30, 1, task->flags & 0x800);
                        break;
                    case 9:
                        result = aitask_move_to_cell_adapter(object, &task->blockX);
                        break;
                    case 10:
                        result = aitask_proximity_step_link_adapter(object, task->object, 1,
                                                                    task->flags & 0x200000);
                        break;
                    case 4:
                        result = aitask_type_five_spawn_adapter(object);
                        break;
                    case 15:
                        result = aitask_neighbor_cell_move_adapter(object, &task->blockX);
                        break;
                    case 11:
                        result = aitask_walkable_cell_route_adapter(object, &task->blockX);
                        break;
                    case 12:
                        result = aitask_cell_step_mark_adapter(object, &task->blockX);
                        break;
                    case 16:
                        result = aitask_proximity_step_clearing_adapter(object, task->object);
                        break;
                    case 17:
                        result = aitask_proximity_step_clearing_adapter(object, task->object);
                        break;
                    case 18:
                        result = level_debug_cell_retarget_adapter(object, task->object);
                        break;
                    case 20:
                        result = aitask_gated_step_record_adapter(object, &task->blockX);
                        break;
                    case 21:
                        result = aitask_cardinal_step_link_adapter(object, task->object,
                                                                   task->field_28);
                        break;
                    case 22:
                        result = entity_set_activity_place(object);
                        break;
                    case 23:
                        result = aitask_proximity_step_jitter_adapter(object, task->object, 0);
                        break;
                    case 24:
                        result = aitask_route_step_adapter(object);
                        break;
                    case 27:
                        result = aitask_kind6_approach_route_adapter(object);
                        break;
                    case 25:
                        result = aitask_coordinate_route_mark_adapter(object, &task->blockX);
                        break;
                    case 29:
                        result = aitask_target_cell_route_marker14_adapter(object, task->object);
                        break;
                    case 30:
                        result = entity_route_to_explicit_target(object, task->object);
                        break;
                    case 28:
                        result = entity_route_marker_setup(object);
                        break;
                    case 26:
                        if (task->object != NULL && AITASK_OBJ(task->object)->field_364 != NULL) {
                            if (AITASK_OBJ(task->object)->flags2 & 0x8000000) {
                                result = 0;
                                break;
                            }
                            if (system_state_flags_get_original(NULL) &&
                                AITASK_OBJ(task->object)->health <
                                    AITASK_OBJ(task->object)->stats->field_e8) {
                                result = 0;
                                break;
                            }
                        }
                        result = aitask_target_cell_route_associate_adapter(object, task->object);
                        break;
                    case 19:
                        result = aitask_position_step_link_adapter(object, task->field_38);
                        entity_reset_auxiliary_state(object);
                        break;
                    case 8:
                        if (((task->flags & 8) && (entity_stats_word_get(object) & 1)) ||
                            !(task->flags & 8)) {
                            result = aitask_nearest_cell_route_commit_adapter(
                                object, task->blockX, task->blockY, task->flags & 8);
                        }
                        break;
                    case 0: {
                        AITaskPoint2I pos;
                        entity_position_to_terrain_cell_adapter(object, &pos.x, &pos.y);
                        if (pos.x != task->blockX || pos.y != task->blockY) {
                            struct TerrainMapVector3 vertices[4];
                            AITaskPoint2F wPos;
                            terrain_map_block_vertices_get(AITASK_LEGO_GET_MAP(), task->blockX,
                                                           task->blockY, vertices);
                            wPos.x = random_float_range(vertices[0].x - s_rdata_0049e024,
                                                         vertices[2].x - s_rdata_0049e020);
                            wPos.y = random_float_range(vertices[2].y - s_rdata_0049e024,
                                                         vertices[0].y - s_rdata_0049e020);
                            result = aitask_object_path_submit_to_grid_target(
                                object, &task->blockX,
                                (entity_stats_word_get(object) & 0x20000000) ? &wPos.x : NULL);
                            break;
                        }
                        result = 1;
                        aitask_entity_command_chain_head_remove(object);
                        break;
                    }
                    }
                    if (result)
                        task->flags |= 1;
                    else
                        remove = 1;
                }
                if (remove) {
                    if (task->type == 8 && (task->flags & 8))
                        task->timeIn = 100.0f;
                    if ((task->flags & 0x10) && task->time < s_rdata_0049e014)
                        task->time = 1500.0f;
                    aitask_entity_command_chain_head_remove(object);
                }
            }
        } else {
            float* elapsed = (float*)data;
            AITaskPoint2I blockPos;
            AITaskPoint2I candidate;

            candidate.x = 0;
            candidate.y = 0;
            AITASK_OBJ(object)->field_338 += *elapsed;
            entity_position_to_terrain_cell_adapter(object, &blockPos.x, &blockPos.y);
            if (AITASK_OBJ(object)->field_338 != *elapsed &&
                !(AITASK_OBJ(object)->flags4 & 0xc0) &&
                aitask_adjacent_position_find_adapter(object, &blockPos.x, &candidate.x, 0)) {
                if (candidate.x != 0 && candidate.y != 0) {
                    task = aitask_cmd_alloc(0);
                    AITASK_OBJ(object)->aiTask = task;
                    task->blockX = candidate.x;
                    task->blockY = candidate.y;
                }
            } else if (AITASK_OBJ(object)->field_338 > s_rdata_0049e028) {
                if (AITASK_OBJ(object)->type == 2 &&
                    !lego_entity_matches_current_selection_or_owner(object))
                    entity_random_activity_attempt(object);
                AITASK_OBJ(object)->field_338 = 0.0f;
            } else {
                prev = NULL;
                for (task = aiTaskGlobs.globalHead; task != NULL;
                     prev = task, task = task->next) {
                    if (!(task->flags & 0x80000) &&
                        ((task->flags & 0x100000) ||
                         (!entity_flag400_and_health_threshold_check(object) &&
                          !(task->flags & 0x400))) &&
                        ((task->flags & 0x100) ||
                         !selection_contains_entity_active_target(object, NULL)) &&
                        (!(task->flags & 0x100) ||
                         cmd_array_find_value_index((const int*)task->buffer, task->count,
                                                    (int)(size_t)object, NULL))) {
                        AITaskPoint2I taskPos;
                        int noRoute;
                        taskPos.x = task->blockX;
                        taskPos.y = task->blockY;
                        if (aitask_place_command_eligibility_check(task, object, NULL, &taskPos.x,
                                                                   &noRoute, 1, 1) &&
                            !noRoute) {
                            AITaskPoint2I pos;
                            if (task->flags & 0x100000)
                                break;
                            entity_position_to_terrain_cell_adapter(object, &pos.x, &pos.y);
                            if (task->type == 0xa || task->type == 0xb || task->type == 0x17 ||
                                task->type == 0x15 ||
                                (task->type == 8 && !(task->flags & 8))) {
                                if (taskPos.x == pos.x && abs(taskPos.y - pos.y) == 1)
                                    break;
                                if (taskPos.y == pos.y && abs(taskPos.x - pos.x) == 1)
                                    break;
                            } else if (taskPos.x == pos.x && taskPos.y == pos.y) {
                                break;
                            }
                        }
                    }
                    if ((task->flags & 0x400) && entity_flag400_and_health_threshold_check(object) &&
                        !(AITASK_OBJ(object)->flags3 & 0x200000) &&
                        AITASK_OBJ(object)->type != 3) {
                        AITaskObject* driver = AITASK_OBJ(object)->interactObject;
                        if (driver->flags3 & 0x80000000) {
                            if (task->type == 0x1b)
                                break;
                        } else if (task->field_20 == 0 ||
                                   (driver->field_330 == task->field_28 &&
                                    (task->field_24 == -1 || driver->field_4 == task->field_24) &&
                                    driver->type == task->field_20)) {
                            int route = 0;
                            int failed = 0;
                            int type = task->type;
                            if (type == 7 || type == 9 || type == 0xf || type == 0x14) {
                                route = 1;
                            } else if (type == 5) {
                                if (task->object == NULL) {
                                    route = 1;
                                } else if (entity_shared_eligibility_check(task->object, 0)) {
                                    if (entity_deposit_activity_gate(object, task->object))
                                        break;
                                    failed = 1;
                                }
                            }
                            if (route) {
                                AITaskPoint2I goal;
                                if (task->type == 9) {
                                    if (!first_free_neighbor_cell_pick_current(object, &task->blockX,
                                                                               &goal.x, 1))
                                        failed = 1;
                                } else {
                                    goal.x = task->blockX;
                                    goal.y = task->blockY;
                                }
                                if (!failed) {
                                    AITaskPoint2I pos;
                                    unsigned int routeCount;
                                    int* routeY;
                                    int* routeX;
                                    entity_position_to_terrain_cell_adapter(object, &pos.x,
                                                                            &pos.y);
                                    if (aitask_grid_route_search_adapter(object, pos.x, pos.y,
                                                                         goal.x, goal.y, &routeX,
                                                                         &routeY, &routeCount)) {
                                        free(routeX);
                                        free(routeY);
                                        break;
                                    }
                                    failed = 1;
                                }
                            }
                            if (failed)
                                task->timeIn = 75.0f;
                        }
                    }
                    if ((task->flags & 0x100) &&
                        cmd_array_find_value_index((const int*)task->buffer, task->count,
                                                   (int)(size_t)object, NULL)) {
                        task = NULL;
                        break;
                    }
                }
                if (task != NULL) {
                    aitask_entity_record_attach_and_relink(task, prev, object);
                } else if (!entity_flag400_and_health_threshold_check(object)) {
                    if (aiTaskGlobs.assignCandidateCount < 50)
                        aiTaskGlobs.assignCandidates[aiTaskGlobs.assignCandidateCount++] = object;
                    if (aiTaskGlobs.timerCandidateCount < 50 && AITASK_OBJ(object)->type == 3 &&
                        !(AITASK_OBJ(object)->flags3 & 0x80000000))
                        aiTaskGlobs.timerCandidates[aiTaskGlobs.timerCandidateCount++] = object;
                }
            }
        }
    }
    aiTaskGlobs.flags &= ~4;
    return 0;
}

/* 0x00404e00 object_path_submit_to_grid_target (corpus name ObjectPathSubmitToGridTarget), 208 bytes.
 * Current definition: src/game/aitask.cpp:2696
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * undefined4 FUN_00404e00(int *param_1,int *param_2,undefined4 param_3)
 *
 * {
 *   int *piVar1;
 *   int iVar2;
 *   int *local_10;
 *   undefined4 local_c;
 *   int local_8;
 *   int local_4;
 *
 *   piVar1 = param_1;
 *   FUN_004424f0(param_1,&local_8,&local_4);
 *   if ((local_8 == *param_2) && (local_4 == param_2[1])) {
 *     param_1 = &local_8;
 *     local_10 = &local_4;
 *     local_c = 1;
 *   }
 *   else {
 *     iVar2 = FUN_00440c30(piVar1,local_8,local_4,*param_2,param_2[1],&param_1,&local_10,&local_c);
 *     if (iVar2 == 0) {
 *       return 0;
 *     }
 *   }
 *   FUN_00441700(piVar1,local_c,param_1,local_10,param_3);
 *   if (param_1 != &local_8) {
 *     FUN_0048db80(param_1);
 *     FUN_0048db80(local_10);
 *   }
 *   return 1;
 * }
 */
// FUNCTION: LEGORR 0x00404e00
int aitask_object_path_submit_to_grid_target(struct EntitySlot* object, int* blockPos,
                                             float* wPos)
{
    AITaskPoint2I pos;
    int* routeX;
    int* routeY;
    unsigned int count;

    entity_position_to_terrain_cell_adapter(object, &pos.x, &pos.y);
    if (pos.x == blockPos[0] && pos.y == blockPos[1]) {
        count = 1;
        routeX = &pos.x;
        routeY = &pos.y;
    } else if (!aitask_grid_route_search_adapter(object, pos.x, pos.y, blockPos[0], blockPos[1],
                                                 &routeX, &routeY, &count)) {
        return 0;
    }
    aitask_route_setup_adapter(object, count, routeX, routeY, wPos);
    if (routeX != &pos.x) {
        free(routeX);
        free(routeY);
    }
    return 1;
}

/* 0x00404ed0 ledger_stub_00404ed0_CheckObjectCandidate (corpus name CheckObjectCandidate), 64 bytes.
 * Current definition: src/game/aitask.cpp:2767
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00404ed0(int *param_1)
 *
 * {
 *   if ((*param_1 == 6) && ((param_1[0xfa] & 0x80000000U) != 0)) {
 *     FUN_0044a3d0(param_1,0);
 *     return;
 *   }
 *   FUN_00404f10(*param_1,param_1[1],param_1[0xcc]);
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00404ed0
int aitask_check_object_candidate(struct EntitySlot* object)
{
    if (AITASK_OBJ(object)->type == 6 && (AITASK_OBJ(object)->flags3 & 0x80000000))
        return aitask_type11_nearest_resolve_adapter(object, NULL);
    return aitask_object_task_target_filter_find(AITASK_OBJ(object)->type,
                                                 AITASK_OBJ(object)->field_4,
                                                 AITASK_OBJ(object)->field_330);
}

/* 0x00404f10 object_task_target_filter_find (corpus name FindMatchingObjectRecord), 64 bytes.
 * Current definition: src/game/aitask.cpp:2781
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00404f10(undefined4 param_1,undefined4 param_2,undefined4 param_3)
 *
 * {
 *   undefined4 local_c;
 *   undefined4 local_8;
 *   undefined4 local_4;
 *
 *   local_c = param_1;
 *   local_8 = param_2;
 *   local_4 = param_3;
 *   FUN_00406540(FUN_00404f50,&local_c);
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00404f10
int aitask_object_task_target_filter_find(int type, int id, int level)
{
    int search[3];

    search[0] = type;
    search[1] = id;
    search[2] = level;
    return aitask_entity_pool_callback_search(aitask_object_filter_matches_action_target, search);
}

/* 0x00404f50 object_filter_matches_action_target (corpus name ObjectFilterMatchesActionTarget), 112 bytes.
 * Current definition: src/game/aitask.cpp:2803
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * undefined4 FUN_00404f50(int *param_1,int *param_2)
 *
 * {
 *   int iVar1;
 *
 *   iVar1 = *param_1;
 *   if (iVar1 != 5) {
 *     if (((iVar1 != 7) && (iVar1 != 9)) && (iVar1 != 0xf)) {
 *       return 0;
 *     }
 *     if (((iVar1 != 5) && (param_1[4] != 0)) && (iVar1 = FUN_004385b0(param_1[4],0), iVar1 == 0)) {
 *       return 0;
 *     }
 *   }
 *   if ((param_1[8] != 0) &&
 *      (((*param_2 != param_1[8] || ((param_1[9] != -1 && (param_2[1] != param_1[9])))) ||
 *       (param_2[2] != param_1[10])))) {
 *     return 0;
 *   }
 *   return 1;
 * }
 */
// FUNCTION: LEGORR 0x00404f50
int aitask_object_filter_matches_action_target(AITask* task, void* data)
{
    int* search = (int*)data;

    if ((task->type == 5 || task->type == 7 || task->type == 9 || task->type == 0xf) &&
        (task->type == 5 || task->object == NULL ||
         entity_shared_eligibility_check(task->object, 0))) {
        if (task->field_20 == 0 ||
            (search[0] == task->field_20 &&
             (task->field_24 == -1 || search[1] == task->field_24) &&
             search[2] == task->field_28))
            return 1;
    }
    return 0;
}

/* 0x00404fc0 ledger_stub_00404fc0_PlaceCommandEligibilityCheck (corpus name PlaceCommandEligibilityCheck), 2448 bytes.
 * Current definition: src/game/aitask.cpp:2837
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * / * WARNING: Globals starting with '_' overlap smaller symbols at the same address * /
 *
 * undefined4
 * FUN_00404fc0(int *param_1,int *param_2,int *param_3,int *param_4,undefined4 *param_5,int param_6,
 *             int param_7)
 *
 * {
 *   int *piVar1;
 *   uint uVar2;
 *   int iVar3;
 *   undefined4 uVar4;
 *   int iVar5;
 *   float10 fVar6;
 *   int local_18;
 *   int local_14 [2];
 *   int local_c;
 *   int local_8;
 *
 *   if (param_5 != (undefined4 *)0x0) {
 *     *param_5 = 0;
 *   }
 *   if (((*param_1 != 3) || (*(int *)param_1[4] != 8)) && (0.0 < (float)param_1[6])) {
 *     return 0;
 *   }
 *   uVar2 = FUN_00469e60(param_2);
 *   if (((uVar2 & 0x20000) != 0) && (param_2[0xdb] == 0)) {
 *     return 0;
 *   }
 *   if ((param_2[0xf8] & 0x800000U) != 0) {
 *     return 0;
 *   }
 *   if ((param_2[0xfb] & 0x400U) != 0) {
 *     return 0;
 *   }
 *   if (((((byte)DAT_00556ed0 & 1) != 0) && (iVar3 = FUN_0044c530(param_2), iVar3 != 0)) &&
 *      (*param_1 != 0x1a)) {
 *     return 0;
 *   }
 *   iVar3 = *param_1;
 *   if (iVar3 == 0x1a) {
 *     iVar3 = FUN_004493e0(param_2);
 *     if (iVar3 == 0) {
 *       return 0;
 *     }
 *     iVar3 = FUN_00472130(param_2,param_1[4]);
 *     if (iVar3 == 0) {
 *       return 0;
 *     }
 *     iVar3 = FUN_00471960(param_2,param_1[4]);
 *     if (iVar3 != 0) {
 *       return 0;
 *     }
 *     iVar3 = FUN_004385b0(param_1[4],1);
 *     if (iVar3 == 0) {
 *       return 0;
 *     }
 *     if (((byte)DAT_00556ed0 & 1) == 0) {
 *       return 0;
 *     }
 *     if (param_3 != (int *)0x0) {
 *       FUN_00442260(param_1[4],param_3,param_3 + 1);
 *     }
 *     if (param_4 != (int *)0x0) {
 *       FUN_004424f0(param_1[4],param_4,param_4 + 1);
 *       return 1;
 *     }
 *   }
 *   else if (iVar3 == 0x1e) {
 *     iVar3 = FUN_0043a5f0(param_1[4]);
 *     if ((iVar3 == 0) || (uVar2 = FUN_00469ea0(param_2), (uVar2 & 2) == 0)) {
 *       return 0;
 *     }
 *     FUN_004754c0(iVar3,0,&local_18);
 *     if (param_3 != (int *)0x0) {
 *       *param_3 = local_18;
 *       param_3[1] = local_14[0];
 *     }
 *     if (param_4 != (int *)0x0) {
 *       uVar4 = FUN_00431570(local_18,local_14[0],param_4,param_4 + 1);
 *       FUN_0044f6d0(uVar4);
 *       return 1;
 *     }
 *   }
 *   else if (iVar3 == 3) {
 *     if ((*(byte *)(param_2 + 0xfa) & 0x40) == 0) {
 *       return 0;
 *     }
 *     iVar3 = FUN_00439bd0(param_2,param_1[4],1);
 *     if (iVar3 == 0) {
 *       return 0;
 *     }
 *     if ((*param_2 == 1) && (iVar3 = FUN_00469bb0(param_2), iVar3 == 0)) {
 *       return 0;
 *     }
 *     piVar1 = (int *)param_1[4];
 *     iVar3 = *piVar1;
 *     if (((iVar3 == 8) && ((*(byte *)(param_2 + 0xfc) & 8) == 0)) && (param_7 != 0)) {
 *       return 0;
 *     }
 *     if ((*param_2 == 3) && (iVar3 != 6)) {
 *       return 0;
 *     }
 *     if (*param_2 == 1) {
 *       if (iVar3 == 9) {
 *         return 0;
 *       }
 *       if ((iVar3 == 6) && ((piVar1[0xfa] & 0x80000000U) != 0)) {
 *         return 0;
 *       }
 *       if (((param_2[0xf8] & 0x400U) != 0) && (*(int *)param_2[0xc2] == 0xb)) {
 *         return 0;
 *       }
 *     }
 *     iVar3 = FUN_00404ed0(piVar1);
 *     if (iVar3 == 0) {
 *       return 0;
 *     }
 *     iVar3 = FUN_00439b80(param_2,param_1[4]);
 *     if (iVar3 == 0) {
 *       return 0;
 *     }
 *     if (param_3 != (int *)0x0) {
 *       FUN_00442260(param_1[4],param_3,param_3 + 1);
 *     }
 *     if (param_4 != (int *)0x0) {
 *       FUN_004424f0(param_1[4],param_4,param_4 + 1);
 *       return 1;
 *     }
 *   }
 *   else if ((((iVar3 == 8) || (iVar3 == 0)) || (iVar3 == 0xb)) || (iVar3 == 0xc)) {
 *     iVar5 = 1;
 *     if (iVar3 == 8) {
 *       iVar5 = FUN_00432770(param_2,param_1[2],param_1[3],param_1[0x17] & 8,1);
 *     }
 *     if (((param_6 != 0) && (iVar3 = *param_1, iVar3 != 0)) &&
 *        ((iVar3 != 9 && (iVar3 = FUN_0044b4c0(param_2,iVar3), iVar3 == 0)))) {
 *       return 0;
 *     }
 *     if ((*param_1 == 0xb) && (*param_2 != 2)) {
 *       return 0;
 *     }
 *     if ((*param_1 == 0xc) && (uVar2 = FUN_00469e60(param_2), (uVar2 & 0x10000) == 0)) {
 *       return 0;
 *     }
 *     if (*param_1 == 8) {
 *       if ((*(byte *)(param_2 + 0xfa) & 2) == 0) {
 *         return 0;
 *       }
 *       if (iVar5 == 0) {
 *         return 0;
 *       }
 *       if (((*(byte *)(param_1 + 0x17) & 8) != 0) &&
 *          (uVar2 = FUN_00469e60(param_2), (uVar2 & 1) == 0)) {
 *         return 0;
 *       }
 *     }
 *     if (param_3 != (int *)0x0) {
 *       uVar4 = FUN_00431570(param_1[2],param_1[3],param_3,param_3 + 1);
 *       FUN_0044f640(uVar4);
 *     }
 *     if (param_4 != (int *)0x0) {
 *       *param_4 = param_1[2];
 *       param_4[1] = param_1[3];
 *     }
 *   }
 *   else {
 *     if (iVar3 == 10) {
 *       if ((param_6 != 0) && (iVar3 = FUN_0044b4c0(param_2,10), iVar3 == 0)) {
 *         return 0;
 *       }
 *       if (((param_7 != 0) && ((*(byte *)(param_2 + 0xfc) & 0x10) == 0)) &&
 *          ((param_1[0x17] & 0x200000U) == 0)) {
 *         return 0;
 *       }
 *       if (*param_2 != 2) {
 *         return 0;
 *       }
 *       if (param_3 != (int *)0x0) {
 *         FUN_00442260(param_1[4],param_3,param_3 + 1);
 *         if (param_4 == (int *)0x0) {
 *           return 1;
 *         }
 *         FUN_004424f0(param_1[4],param_4,param_4 + 1);
 *         return 1;
 *       }
 *     }
 *     else {
 *       if (iVar3 == 0x11) {
 *         iVar3 = FUN_004385b0(param_1[4],0);
 *         if (iVar3 == 0) {
 *           return 0;
 *         }
 *         if (((*param_2 == 2) && ((param_2[0xf9] & 0x800U) == 0)) &&
 *            ((float)param_2[0xd1] <= (float)param_2[0xd0] * 0.01 * _DAT_00556458)) {
 *           if ((param_2[0xf8] & 0x400U) == 0) {
 *             return 1;
 *           }
 *           return 0;
 *         }
 *         return 0;
 *       }
 *       if (iVar3 == 0x12) {
 *         if ((((*param_2 != 2) || (param_2[0xbc] != 0)) || (param_2[0xdb] != 0)) ||
 *            (((param_1[0xf] & param_2[0xfc]) == 0 && (param_7 != 0)))) {
 *           return 0;
 *         }
 *         uVar2 = FUN_00469e60(param_1[4]);
 *         if ((uVar2 & 0x2000000) == 0) {
 *           if ((((*(byte *)(param_1[4] + 0x3ec) & 0x40) != 0) &&
 *               (piVar1 = *(int **)(param_1[4] + 0x2fc), piVar1 != (int *)0x0)) &&
 *              ((*piVar1 == 4 && (iVar3 = FUN_004087e0(piVar1[6]), iVar3 != 0)))) {
 *             FUN_004754c0(iVar3,0,&local_c);
 *             if (param_3 != (int *)0x0) {
 *               *param_3 = local_c;
 *               param_3[1] = local_8;
 *             }
 *             if (param_4 == (int *)0x0) {
 *               return 1;
 *             }
 *             uVar4 = FUN_00431570(local_c,local_8,param_4,param_4 + 1);
 *             FUN_0044f6d0(uVar4);
 *             return 1;
 *           }
 *           iVar3 = FUN_00439e40(param_1[4],&local_18);
 *           if (iVar3 == 0) {
 *             return 0;
 *           }
 *           if (param_3 != (int *)0x0) {
 *             uVar4 = FUN_00431570(local_18,local_14[0],param_3,param_3 + 1);
 *             FUN_0044f640(uVar4);
 *           }
 *           if (param_4 == (int *)0x0) {
 *             return 1;
 *           }
 *           *param_4 = local_18;
 *           param_4[1] = local_14[0];
 *           return 1;
 *         }
 *         if (param_3 != (int *)0x0) {
 *           FUN_00442260(param_1[4],param_3,param_3 + 1);
 *           if (param_4 == (int *)0x0) {
 *             return 1;
 *           }
 *           FUN_004424f0(param_1[4],param_4,param_4 + 1);
 *           return 1;
 *         }
 *       }
 *       else {
 *         if (iVar3 == 0x15) {
 *           if (*param_2 != 1) {
 *             return 0;
 *           }
 *           if (param_2[0xdb] != 0) {
 *             uVar4 = FUN_0043a5f0(param_1[4]);
 *             FUN_004754c0(uVar4,0,&local_c);
 *             if (param_3 != (int *)0x0) {
 *               *param_3 = local_c;
 *               param_3[1] = local_8;
 *             }
 *             if (param_4 == (int *)0x0) {
 *               return 1;
 *             }
 *             uVar4 = FUN_00431570(local_c,local_8,param_4,param_4 + 1);
 *             FUN_0044f6d0(uVar4);
 *             return 1;
 *           }
 *           return 0;
 *         }
 *         if (iVar3 != 0x17) {
 *           if (iVar3 == 0x16) {
 *             if (*param_2 != 2) {
 *               return 0;
 *             }
 *             piVar1 = param_1 + 2;
 *             iVar3 = FUN_00432550(piVar1);
 *             if (iVar3 != 0) {
 *               if (param_3 != (int *)0x0) {
 *                 uVar4 = FUN_00431570(*piVar1,param_1[3],param_3,param_3 + 1);
 *                 FUN_0044f640(uVar4);
 *               }
 *               if (param_4 == (int *)0x0) {
 *                 return 1;
 *               }
 *               *param_4 = *piVar1;
 *               param_4[1] = param_1[3];
 *               return 1;
 *             }
 *             return 0;
 *           }
 *           if (iVar3 == 0x13) {
 *             if (((*param_2 == 2) && (iVar3 = FUN_0046a060(param_2), iVar3 != 0)) &&
 *                (iVar3 = FUN_0044b480(param_2,param_1[0xe]), iVar3 == 0)) {
 *               FUN_00442260(param_2,&local_18,local_14);
 *               iVar3 = FUN_00438df0(&local_18);
 *               if (iVar3 != 0) {
 *                 if (param_3 == (int *)0x0) {
 *                   return 1;
 *                 }
 *                 FUN_00442260(iVar3,param_3,param_3 + 1);
 *                 return 1;
 *               }
 *             }
 *             return 0;
 *           }
 *           if (param_5 == (undefined4 *)0x0) {
 *             return 1;
 *           }
 *           *param_5 = 1;
 *           return 1;
 *         }
 *         if (*param_2 != 2) {
 *           return 0;
 *         }
 *         uVar2 = param_2[0xfc];
 *         if (((uVar2 & 1) != 0) && (param_1[0xf] == 1)) {
 *           return 0;
 *         }
 *         if (((uVar2 & 2) != 0) && (param_1[0xf] == 2)) {
 *           return 0;
 *         }
 *         if (((uVar2 & 4) != 0) && (param_1[0xf] == 4)) {
 *           return 0;
 *         }
 *         if (((uVar2 & 8) != 0) && (param_1[0xf] == 8)) {
 *           return 0;
 *         }
 *         if (((uVar2 & 0x10) != 0) && (param_1[0xf] == 0x10)) {
 *           return 0;
 *         }
 *         if (((uVar2 & 0x20) != 0) && (param_1[0xf] == 0x20)) {
 *           return 0;
 *         }
 *         fVar6 = (float10)FUN_00429880();
 *         if ((float10)(float)param_2[0xf5] < fVar6) {
 *           return 0;
 *         }
 *         if (param_3 != (int *)0x0) {
 *           FUN_00442260(param_1[4],param_3,param_3 + 1);
 *         }
 *       }
 *     }
 *     if (param_4 != (int *)0x0) {
 *       FUN_004424f0(param_1[4],param_4,param_4 + 1);
 *       return 1;
 *     }
 *   }
 *   return 1;
 * }
 */
// FUNCTION: LEGORR 0x00404fc0
int aitask_place_command_eligibility_check(AITask* task, struct EntitySlot* object,
                                           float* outWPos, int* outBPos, int* outFlag,
                                           int checkCommand, int checkAbility)
{
    int result = 1;
    float pos[3];

    if (outFlag != NULL) *outFlag = 0;

    if ((task->type != 3 || AITASK_OBJ(task->object)->type != 8) && task->timeIn > 0.0f)
        return 0;
    if ((entity_stats_word_get(object) & 0x20000) && AITASK_OBJ(object)->driveObject == NULL)
        return 0;
    if (AITASK_OBJ(object)->flags1 & 0x800000)
        return 0;
    if (AITASK_OBJ(object)->flags4 & 0x400)
        return 0;
    if ((s_panel_highlight_flags & 1) && ai_can_do(object) && task->type != 0x1a)
        return 0;

    if (task->type == 0x1a) {
        if (scene_object_has_task_type_4_5_7(object) &&
            entity_centers_within_object_radius(object, task->object) &&
            !see_through_walls_ray_check(object, task->object) &&
            entity_shared_eligibility_check(task->object, 1) &&
            (s_panel_highlight_flags & 1)) {
            if (outWPos != NULL)
                entity_transform_position_get_outputs(task->object, &outWPos[0], &outWPos[1]);
            if (outBPos != NULL)
                entity_position_to_terrain_cell_adapter(task->object, &outBPos[0], &outBPos[1]);
        } else {
            result = 0;
        }
    } else if (task->type == 0x1e) {
        void* cont = aitask_object_container_get_adapter(task->object);
        if (cont != NULL && (entity_capability_flags_get(object) & 2)) {
            float cPos[3];
            ddiscene_object_position_get(cont, NULL, cPos);
            if (outWPos != NULL) {
                outWPos[0] = cPos[0];
                outWPos[1] = cPos[1];
            }
            if (outBPos != NULL)
                terrain_map_world_to_block(AITASK_LEGO_GET_MAP(), cPos[0], cPos[1], &outBPos[0], &outBPos[1]);
        } else {
            return 0;
        }
    } else if (task->type == 3) {
        if ((AITASK_OBJ(object)->flags3 & 0x40) &&
            entity_accepts_target_for_task_check(object, task->object, 1) &&
            (AITASK_OBJ(object)->type != 1 || stats_instance_word_at_ec(object))) {
            struct EntitySlot* target = task->object;
            int targetType = AITASK_OBJ(target)->type;
            if (targetType == 8 && !(AITASK_OBJ(object)->abilityFlags & 8) && checkAbility)
                result = 0;
            else if (AITASK_OBJ(object)->type == 3 && targetType != 6)
                result = 0;
            else if (AITASK_OBJ(object)->type == 1 && targetType == 9)
                result = 0;
            else if (AITASK_OBJ(object)->type == 1 && targetType == 6 &&
                     (AITASK_OBJ(target)->flags3 & 0x80000000))
                result = 0;
            else if (AITASK_OBJ(object)->type == 1 &&
                     (AITASK_OBJ(object)->flags1 & 0x400) &&
                     AITASK_OBJ(object)->interactObject->type == 0xb)
                return 0;
            else if (!aitask_check_object_candidate(target))
                result = 0;
            else if (!entity_task_target_cell_eligibility_check(object, task->object))
                result = 0;
            else {
                if (outWPos != NULL)
                    entity_transform_position_get_outputs(task->object, &outWPos[0], &outWPos[1]);
                if (outBPos != NULL)
                    entity_position_to_terrain_cell_adapter(task->object, &outBPos[0], &outBPos[1]);
            }
        } else {
            result = 0;
        }
    } else if (task->type == 8 || task->type == 0 || task->type == 0xb || task->type == 0xc) {
        int walkable = 1;
        if (task->type == 8)
            walkable = terrain_cell_walkability_query_current(object, task->blockX, task->blockY,
                                                              task->flags & 8, 1);
        if (checkCommand && task->type != 0 && task->type != 9 &&
            !aitask_object_command_gate_adapter(object, task->type))
            result = 0;
        else if (task->type == 0xb && AITASK_OBJ(object)->type != 2)
            result = 0;
        else if (task->type == 0xc && !(entity_stats_word_get(object) & 0x10000))
            result = 0;
        else if (task->type == 8 &&
                 (!(AITASK_OBJ(object)->flags3 & 2) || !walkable ||
                  ((task->flags & 8) && !(entity_stats_word_get(object) & 1))))
            result = 0;
        else {
            if (outWPos != NULL)
                terrain_map_block_to_world(AITASK_LEGO_GET_MAP(), task->blockX, task->blockY,
                                           &outWPos[0], &outWPos[1]);
            if (outBPos != NULL) {
                outBPos[0] = task->blockX;
                outBPos[1] = task->blockY;
            }
        }
    } else if (task->type == 10) {
        if (checkCommand && !aitask_object_command_gate_adapter(object, 10))
            result = 0;
        else if (checkAbility && !(AITASK_OBJ(object)->abilityFlags & 0x10) &&
                 !(task->flags & 0x200000))
            result = 0;
        else if (AITASK_OBJ(object)->type == 2) {
            if (outWPos != NULL)
                entity_transform_position_get_outputs(task->object, &outWPos[0], &outWPos[1]);
            if (outBPos != NULL)
                entity_position_to_terrain_cell_adapter(task->object, &outBPos[0], &outBPos[1]);
        } else
            result = 0;
    } else if (task->type == 0x11) {
        if (!entity_shared_eligibility_check(task->object, 0))
            result = 0;
        else if (AITASK_OBJ(object)->type == 2 &&
                 !(AITASK_OBJ(object)->flags2 & 0x800) &&
                 AITASK_OBJ(object)->energy <=
                     AITASK_OBJ(object)->health * s_rdata_0049e02c * legoGlobs_MinEnergyForEat) {
            if (AITASK_OBJ(object)->flags1 & 0x400)
                result = 0;
        } else {
            result = 0;
        }
    } else if (task->type == 0x12) {
        if (AITASK_OBJ(object)->type == 2 && AITASK_OBJ(object)->aiTask == NULL &&
            AITASK_OBJ(object)->driveObject == NULL &&
            ((task->field_3c & AITASK_OBJ(object)->abilityFlags) || !checkAbility)) {
            if (!(entity_stats_word_get(task->object) & 0x2000000)) {
                AITaskObject* carried;
                void* cont;
                if ((AITASK_OBJ(task->object)->flags4 & 0x40) &&
                    (carried = AITASK_OBJ(task->object)->field_2fc) != NULL &&
                    carried->type == 4 &&
                    (cont = scene_object_secondary_handle_get_or_create(carried->field_18)) != NULL) {
                    ddiscene_object_position_get(cont, NULL, pos);
                    if (outWPos != NULL) {
                        outWPos[0] = pos[0];
                        outWPos[1] = pos[1];
                    }
                    if (outBPos != NULL)
                        terrain_map_world_to_block(AITASK_LEGO_GET_MAP(), pos[0], pos[1], &outBPos[0], &outBPos[1]);
                } else {
                    int bPos[2];
                    if (entity_record_flag20_query(task->object, bPos)) {
                        if (outWPos != NULL)
                            terrain_map_block_to_world(AITASK_LEGO_GET_MAP(), bPos[0], bPos[1],
                                                       &outWPos[0], &outWPos[1]);
                        if (outBPos != NULL) {
                            outBPos[0] = bPos[0];
                            outBPos[1] = bPos[1];
                        }
                    } else {
                        result = 0;
                    }
                }
            } else {
                if (outWPos != NULL)
                    entity_transform_position_get_outputs(task->object, &outWPos[0], &outWPos[1]);
                if (outBPos != NULL)
                    entity_position_to_terrain_cell_adapter(task->object, &outBPos[0], &outBPos[1]);
            }
        } else {
            result = 0;
        }
    } else if (task->type == 0x15) {
        if (AITASK_OBJ(object)->type == 1 && AITASK_OBJ(object)->driveObject != NULL) {
            void* cont = aitask_object_container_get_adapter(task->object);
            ddiscene_object_position_get(cont, NULL, pos);
            if (outWPos != NULL) {
                outWPos[0] = pos[0];
                outWPos[1] = pos[1];
            }
            if (outBPos != NULL)
                terrain_map_world_to_block(AITASK_LEGO_GET_MAP(), pos[0], pos[1], &outBPos[0], &outBPos[1]);
        } else {
            result = 0;
        }
    } else if (task->type == 0x17) {
        unsigned int ability;
        if (AITASK_OBJ(object)->type != 2 ||
            (((ability = AITASK_OBJ(object)->abilityFlags) & 0x1) && task->field_3c == 0x1) ||
            ((ability & 0x2) && task->field_3c == 0x2) ||
            ((ability & 0x4) && task->field_3c == 0x4) ||
            ((ability & 0x8) && task->field_3c == 0x8) ||
            ((ability & 0x10) && task->field_3c == 0x10) ||
            ((ability & 0x20) && task->field_3c == 0x20))
            result = 0;
        else if (engine_float_field260_get_current_target() > AITASK_OBJ(object)->field_3d4)
            result = 0;
        else {
            if (outWPos != NULL)
                entity_transform_position_get_outputs(task->object, &outWPos[0], &outWPos[1]);
            if (outBPos != NULL)
                entity_position_to_terrain_cell_adapter(task->object, &outBPos[0], &outBPos[1]);
        }
    } else if (task->type == 0x16) {
        if (AITASK_OBJ(object)->type != 2)
            result = 0;
        else if (!aitask_place_command_cell_accept_adapter(&task->blockX))
            result = 0;
        else {
            if (outWPos != NULL)
                terrain_map_block_to_world(AITASK_LEGO_GET_MAP(), task->blockX, task->blockY,
                                           &outWPos[0], &outWPos[1]);
            if (outBPos != NULL) {
                outBPos[0] = task->blockX;
                outBPos[1] = task->blockY;
            }
        }
    } else if (task->type == 0x13) {
        struct EntitySlot* found;
        float wPos[2];
        if (AITASK_OBJ(object)->type == 2 && (unsigned int)ai_activity_cap(object) > 0 &&
            !ai_activity_scan(object, (int)(size_t)task->field_38)) {
            entity_transform_position_get_outputs(object, &wPos[0], &wPos[1]);
            if ((found = entity_lookup_at_world_position(wPos)) != NULL) {
                if (outWPos != NULL)
                    entity_transform_position_get_outputs(found, &outWPos[0], &outWPos[1]);
            } else {
                result = 0;
            }
        } else {
            result = 0;
        }
    } else {
        if (outFlag != NULL) *outFlag = 1;
    }

    return result;
}

/* 0x00405950 pending_command_timer_fire_scan (corpus name PendingCommandTimerFireScan), 704 bytes.
 * Current definition: src/game/aitask.cpp:3548
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00405950(void)
 *
 * {
 *   int iVar1;
 *   float fVar2;
 *   float fVar3;
 *   bool bVar4;
 *   int *piVar5;
 *   int iVar6;
 *   uint uVar7;
 *   undefined4 uVar8;
 *   uint uVar9;
 *   float10 fVar10;
 *   int *local_24;
 *   int *local_1c;
 *   float local_18;
 *   float local_14;
 *   float local_10;
 *   float local_c;
 *   int local_8;
 *   int local_4;
 *
 *   local_1c = (int *)0x0;
 *   piVar5 = DAT_004b3358;
 *   do {
 *     if (piVar5 == (int *)0x0) {
 *       return;
 *     }
 *     if ((float)piVar5[6] <= 0.0) {
 *       bVar4 = false;
 *       uVar9 = 0;
 *       if (DAT_004b34f0 != 0) {
 *         local_24 = &DAT_004b3428;
 *         do {
 *           iVar1 = *local_24;
 *           FUN_00442260(iVar1,&local_18,&local_14);
 *           FUN_004424f0(iVar1,&local_8,&local_4);
 *           fVar10 = (float10)FUN_00469d70(iVar1);
 *           if (fVar10 < (float10)*(float *)(iVar1 + 0x340)) {
 *             iVar6 = *piVar5;
 *             if (iVar6 == 4) {
 *               iVar6 = FUN_004436f0(iVar1);
 *               if ((iVar6 != 0) && (iVar6 = FUN_00437100(4,0), iVar6 != 0)) {
 *                 bVar4 = true;
 *               }
 *             }
 *             else if (iVar6 == 10) {
 *               uVar7 = FUN_00469e80(iVar1);
 *               if (((uVar7 & 0x80000) == 0) ||
 *                  ((iVar6 = FUN_004385b0(piVar5[4],0), iVar6 != 0 &&
 *                   (uVar7 = FUN_00469e80(piVar5[4]), (uVar7 & 0x400) == 0)))) {
 *                 FUN_00442260(piVar5[4],&local_10,&local_c);
 *                 fVar3 = local_18 - local_10;
 *                 fVar2 = local_14 - local_c;
 *                 fVar10 = (float10)FUN_0046a020(iVar1);
 *                 if ((float10)SQRT(fVar2 * fVar2 + fVar3 * fVar3) < fVar10) {
 *                   bVar4 = true;
 *                   uVar7 = FUN_00469e80(iVar1);
 *                   if ((uVar7 & 0x80000) == 0) {
 *                     piVar5[6] = 0x443b8000;
 *                   }
 *                 }
 *               }
 *             }
 *             else if (iVar6 == 3) {
 *               iVar6 = FUN_0043c650(iVar1);
 * joined_r0x00405b46:
 *               if (iVar6 != 0) {
 *                 bVar4 = true;
 *                 piVar5[6] = 0x437a0000;
 *               }
 *             }
 *             else if (iVar6 == 0x19) {
 *               uVar7 = FUN_00469e80(iVar1);
 *               if ((uVar7 & 0x4000) != 0) {
 *                 bVar4 = true;
 *               }
 *             }
 *             else if (iVar6 == 0) {
 *               iVar6 = FUN_004436f0(iVar1);
 *               if ((iVar6 != 0) && ((piVar5[2] != local_8 || (piVar5[3] != local_4)))) {
 *                 bVar4 = true;
 *               }
 *             }
 *             else if (iVar6 == 0x1d) {
 *               iVar6 = FUN_004436f0(iVar1);
 *               goto joined_r0x00405b46;
 *             }
 *           }
 *           if ((*piVar5 == 0x18) &&
 *              ((iVar6 = FUN_004436f0(iVar1), iVar6 != 0 ||
 *               (uVar7 = FUN_00469e80(iVar1), (uVar7 & 0x100000) != 0)))) {
 *             bVar4 = true;
 *           }
 *           if (bVar4) {
 *             DAT_004b34f0 = DAT_004b34f0 - 1;
 *             (&DAT_004b3428)[uVar9] = (&DAT_004b3428)[DAT_004b34f0];
 *             if ((*(byte *)(piVar5 + 0x17) & 0x40) == 0) {
 *               if (local_1c == (int *)0x0) {
 *                 DAT_004b3358 = (int *)piVar5[0x18];
 *               }
 *               else {
 *                 local_1c[0x18] = piVar5[0x18];
 *               }
 *               piVar5[0x18] = 0;
 *               *(int **)(iVar1 + 0x2f0) = piVar5;
 *             }
 *             else {
 *               uVar8 = FUN_00406400(piVar5);
 *               *(undefined4 *)(iVar1 + 0x2f0) = uVar8;
 *             }
 *             break;
 *           }
 *           uVar9 = uVar9 + 1;
 *           local_24 = local_24 + 1;
 *         } while (uVar9 < DAT_004b34f0);
 *       }
 *     }
 *     local_1c = piVar5;
 *     piVar5 = (int *)piVar5[0x18];
 *   } while( true );
 * }
 */
// FUNCTION: LEGORR 0x00405950
void aitask_pending_command_timer_fire_scan(void)
{
    AITask* prev = NULL;
    AITask* task;

    for (task = aiTaskGlobs.globalTail; task != NULL; task = task->next) {
        if (task->timeIn <= 0.0f) {
            int fire = 0;
            unsigned int i;
            for (i = 0; i < aiTaskGlobs.timerCandidateCount; i++) {
                struct EntitySlot* object = aiTaskGlobs.timerCandidates[i];
                float wPos[2];
                int bPos[2];
                entity_transform_position_get_outputs(object, &wPos[0], &wPos[1]);
                entity_position_to_terrain_cell_adapter(object, &bPos[0], &bPos[1]);
                if (entity_stats_float_ate8(object) < AITASK_OBJ(object)->health) {
                    if (task->type == 4) {
                        if (entity_type3_flag04000000_test(object) && grid_record_count_difference_sum(4, 0))
                            fire = 1;
                    } else if (task->type == 10) {
                        if (!(entity_state_flags_get(object) & 0x80000) ||
                            (entity_shared_eligibility_check(task->object, 0) &&
                             !(entity_state_flags_get(task->object) & 0x400))) {
                            float tPos[2];
                            float dx, dy, dist;
                            entity_transform_position_get_outputs(task->object, &tPos[0], &tPos[1]);
                            dx = wPos[0] - tPos[0];
                            dy = wPos[1] - tPos[1];
                            dist = (float)sqrt(dx * dx + dy * dy);
                            if (dist < entity_stats_float_at98(object)) {
                                fire = 1;
                                if (!(entity_state_flags_get(object) & 0x80000))
                                    task->timeIn = 750.0f;
                            }
                        }
                    } else if (task->type == 3) {
                        if (entity_capacity_or_state_predicate(object)) {
                            fire = 1;
                            task->timeIn = 250.0f;
                        }
                    } else if (task->type == 0x19) {
                        if (entity_state_flags_get(object) & 0x4000)
                            fire = 1;
                    } else if (task->type == 0) {
                        if (entity_type3_flag04000000_test(object) &&
                            (task->blockX != bPos[0] || task->blockY != bPos[1]))
                            fire = 1;
                    } else if (task->type == 0x1d) {
                        if (entity_type3_flag04000000_test(object)) {
                            fire = 1;
                            task->timeIn = 250.0f;
                        }
                    }
                }
                if (task->type == 0x18 &&
                    (entity_type3_flag04000000_test(object) || (entity_state_flags_get(object) & 0x100000)))
                    fire = 1;
                if (fire) {
                    aiTaskGlobs.timerCandidates[i] = aiTaskGlobs.timerCandidates[--aiTaskGlobs.timerCandidateCount];
                    if (task->flags & 0x40) {
                        AITASK_OBJ(object)->aiTask = aitask_cmd_pending_record_copy(task);
                    } else {
                        if (prev != NULL) prev->next = task->next;
                        else aiTaskGlobs.globalTail = task->next;
                        task->next = NULL;
                        AITASK_OBJ(object)->aiTask = task;
                    }
                    break;
                }
            }
        }
        prev = task;
    }
}

/* 0x00405c10 PendingCommandEntityAssignmentTick (corpus name PendingCommandEntityAssignmentTick), 1872 bytes.
 * Current definition: src/game/aitask.cpp:3672
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00405c10(void)
 *
 * {
 *   float fVar1;
 *   bool bVar2;
 *   int *piVar3;
 *   int iVar4;
 *   undefined4 uVar5;
 *   int iVar6;
 *   uint uVar7;
 *   int *piVar8;
 *   int *piVar9;
 *   int local_94;
 *   int *local_90;
 *   int *local_8c;
 *   undefined4 local_88;
 *   uint local_84;
 *   int local_80;
 *   int local_7c;
 *   undefined4 local_78;
 *   float local_74;
 *   float local_70;
 *   int local_6c;
 *   int local_68;
 *   int local_64;
 *   int local_60;
 *   uint local_5c;
 *   int local_58;
 *   int local_54;
 *   int *local_50;
 *   float local_4c;
 *   float local_48;
 *   float local_44;
 *   float local_40;
 *   float local_3c;
 *   undefined4 local_38;
 *   int local_34;
 *   undefined4 local_30;
 *   undefined1 local_2c [8];
 *   int local_24;
 *   int local_20;
 *   float local_18;
 *   undefined1 local_14 [4];
 *   undefined1 local_10 [8];
 *   undefined1 local_8 [8];
 *
 *   local_50 = (int *)0x0;
 *   local_44 = 99999.0;
 *   piVar3 = DAT_004b3354;
 *   do {
 *     if (piVar3 == (int *)0x0) {
 *       return;
 *     }
 *     if (((float)piVar3[6] <= 0.0) && ((piVar3[0x17] & 0x80400U) == 0)) {
 *       piVar8 = (int *)0x0;
 *       local_90 = (int *)0x0;
 *       local_84 = 0;
 *       if (DAT_004b3424 != 0) {
 *         local_8c = &DAT_004b335c;
 *         piVar9 = piVar8;
 *         do {
 *           piVar8 = (int *)*local_8c;
 *           if (piVar8 != (int *)0x0) {
 *             if ((piVar3[0x17] & 0x100U) == 0) {
 *               iVar4 = FUN_00452610(piVar8,0);
 *               piVar9 = local_90;
 *               if (iVar4 == 0) {
 *                 if ((piVar3[0x17] & 0x100U) != 0) goto LAB_00405caa;
 *                 goto LAB_00405cc5;
 *               }
 *             }
 *             else {
 * LAB_00405caa:
 *               iVar4 = FUN_00452650(piVar3[0x10],piVar3[0x11],piVar8,0);
 *               piVar9 = local_90;
 *               if (iVar4 != 0) {
 * LAB_00405cc5:
 *                 FUN_00442260(piVar8,&local_4c,&local_48);
 *                 iVar4 = FUN_00404fc0(piVar3,piVar8,&local_74,0,&local_34,1,1);
 *                 if (iVar4 != 0) {
 *                   uVar5 = FUN_00431570(local_74,local_70,&local_6c,&local_68);
 *                   iVar6 = FUN_0044f6d0(uVar5);
 *                   if (iVar6 != 0) {
 *                     uVar5 = FUN_00431570(local_4c,local_48,&local_64,&local_60);
 *                     iVar6 = FUN_0044f6d0(uVar5);
 *                     if (iVar6 != 0) {
 *                       iVar6 = *piVar3;
 *                       bVar2 = true;
 *                       if ((((((iVar6 == 8) && ((*(byte *)(piVar3 + 0x17) & 8) == 0)) ||
 *                             (iVar6 == 10)) || ((iVar6 == 0xb || (iVar6 == 0x17)))) ||
 *                           (iVar6 == 0x13)) &&
 *                          (((local_64 != local_6c ||
 *                            (uVar7 = local_60 - local_68 >> 0x1f,
 *                            (local_60 - local_68 ^ uVar7) - uVar7 != 1)) &&
 *                           ((local_60 != local_68 ||
 *                            (uVar7 = local_64 - local_6c >> 0x1f,
 *                            (local_64 - local_6c ^ uVar7) - uVar7 != 1)))))) {
 *                         local_24 = local_6c;
 *                         local_20 = local_68;
 *                         iVar6 = FUN_00431c70(local_90,&local_24,&local_6c,1);
 *                         if (iVar6 == 0) {
 *                           bVar2 = false;
 *                         }
 *                       }
 *                       if (bVar2) {
 *                         iVar4 = FUN_00440c30(piVar8,local_64,local_60,local_6c,local_68,&local_30,
 *                                              &local_38,local_14);
 *                         if (iVar4 == 0) {
 *                           piVar3[6] = 0x42960000;
 *                         }
 *                         else {
 *                           FUN_0048db80(local_30);
 *                           FUN_0048db80(local_38);
 *                         }
 *                       }
 *                     }
 *                   }
 *                 }
 *                 if (local_34 != 0) {
 *                   local_5c = local_84;
 *                   local_40 = local_74;
 *                   local_3c = local_70;
 *                   break;
 *                 }
 *                 piVar9 = local_90;
 *                 if (iVar4 != 0) {
 *                   local_18 = local_70 - local_48;
 *                   fVar1 = SQRT(local_18 * local_18 + (local_74 - local_4c) * (local_74 - local_4c));
 *                   FUN_004422a0(piVar8,local_8);
 *                   if ((local_90 == (int *)0x0) || (fVar1 < local_44)) {
 *                     local_5c = local_84;
 *                     local_40 = local_74;
 *                     local_3c = local_70;
 *                     piVar9 = piVar8;
 *                     local_90 = piVar8;
 *                     local_44 = fVar1;
 *                   }
 *                 }
 *               }
 *             }
 *           }
 *           piVar8 = piVar9;
 *           local_84 = local_84 + 1;
 *           local_8c = local_8c + 1;
 *           piVar9 = piVar8;
 *         } while (local_84 < DAT_004b3424);
 *       }
 *       if (piVar8 == (int *)0x0) {
 *         if (((piVar3[0x17] & 0x1000U) == 0) && (uVar7 = 0, DAT_004b3424 != 0)) {
 *           piVar8 = &DAT_004b335c;
 *           do {
 *             iVar4 = *piVar8;
 *             if (iVar4 != 0) {
 *               if ((piVar3[0x17] & 0x100U) == 0) {
 *                 iVar6 = FUN_00452610(iVar4,0);
 *                 if (iVar6 == 0) {
 *                   if ((piVar3[0x17] & 0x100U) != 0) goto LAB_00406186;
 *                   goto LAB_0040619d;
 *                 }
 *               }
 *               else {
 * LAB_00406186:
 *                 iVar6 = FUN_00452650(piVar3[0x10],piVar3[0x11],iVar4,0);
 *                 if (iVar6 != 0) {
 * LAB_0040619d:
 *                   iVar4 = FUN_00404fc0(piVar3,iVar4,0,0,0,0,1);
 *                   if (iVar4 != 0) {
 *                     FUN_004030e0(piVar3);
 *                     break;
 *                   }
 *                 }
 *               }
 *             }
 *             uVar7 = uVar7 + 1;
 *             piVar8 = piVar8 + 1;
 *           } while (uVar7 < DAT_004b3424);
 *         }
 *         if (((*piVar3 != 0x17) && (piVar3[0xf] != 0)) && ((piVar3[0x17] & 0x8000U) == 0)) {
 *           uVar7 = 0;
 *           local_94 = 0;
 *           bVar2 = false;
 *           if (DAT_004b3424 != 0) {
 *             piVar8 = &DAT_004b335c;
 *             do {
 *               iVar4 = *piVar8;
 *               if (iVar4 != 0) {
 *                 if ((piVar3[0x17] & 0x100U) == 0) {
 *                   iVar6 = FUN_00452610(iVar4,0);
 *                   if (iVar6 == 0) {
 *                     if ((piVar3[0x17] & 0x100U) != 0) goto LAB_0040622f;
 *                     goto LAB_00406246;
 *                   }
 *                 }
 *                 else {
 * LAB_0040622f:
 *                   iVar6 = FUN_00452650(piVar3[0x10],piVar3[0x11],iVar4,0);
 *                   if (iVar6 != 0) {
 * LAB_00406246:
 *                     iVar6 = FUN_00404fc0(piVar3,iVar4,local_2c,0,0,0,0);
 *                     if ((iVar6 != 0) &&
 *                        (iVar6 = FUN_00404fc0(piVar3,iVar4,local_2c,0,0,0,1), local_94 = iVar4,
 *                        iVar6 != 0)) {
 *                       bVar2 = true;
 *                       break;
 *                     }
 *                   }
 *                 }
 *               }
 *               uVar7 = uVar7 + 1;
 *               piVar8 = piVar8 + 1;
 *             } while (uVar7 < DAT_004b3424);
 *           }
 *           if (((local_94 != 0) && (!bVar2)) &&
 *              (iVar4 = FUN_00438e50(0,local_2c,piVar3[0xf]), iVar4 != 0)) {
 *             FUN_00402890(iVar4,piVar3[0xf],piVar3[0xf] != 0x10);
 *             piVar3[0x17] = piVar3[0x17] | 0x8000;
 *           }
 *         }
 *       }
 *       else {
 *         bVar2 = true;
 *         FUN_004424f0(piVar8,&local_58,&local_54);
 *         uVar5 = FUN_00431570(local_40,local_3c,&local_80,&local_7c);
 *         FUN_0044f6d0(uVar5);
 *         iVar4 = *piVar3;
 *         if (((((iVar4 == 0xc) || (iVar4 == 3)) || (iVar4 == 0x16)) || (iVar4 == 0x15)) &&
 *            ((local_58 != local_80 || (local_54 != local_7c)))) {
 *           if (piVar3[0x12] == 0) {
 *             switch(iVar4) {
 *             case 3:
 *               local_78 = 0x17;
 *               break;
 *             case 0xc:
 *               local_78 = 0x27;
 *               break;
 *             case 0x15:
 *               local_78 = 0x38;
 *               break;
 *             case 0x16:
 *               local_78 = 0x30;
 *             }
 *             FUN_00452060(local_78,piVar8,piVar3,&local_80);
 *             if ((piVar3[0x17] & 0x100U) == 0) {
 *               piVar3[0x12] = (int)piVar8;
 *             }
 *             if ((*piVar3 == 0xc) && (*piVar8 == 1)) {
 *               FUN_00402b60(piVar3 + 2,1);
 *               piVar3[6] = 0;
 *             }
 *           }
 * LAB_00406112:
 *           bVar2 = false;
 *         }
 *         else if ((((iVar4 == 8) && ((*(byte *)(piVar3 + 0x17) & 8) == 0)) ||
 *                  (((iVar4 == 10 || (iVar4 == 0xb)) || ((iVar4 == 0x17 || (iVar4 == 0x13)))))) &&
 *                 (((local_58 != local_80 ||
 *                   (uVar7 = local_54 - local_7c >> 0x1f, (local_54 - local_7c ^ uVar7) - uVar7 != 1))
 *                  && ((local_54 != local_7c ||
 *                      (uVar7 = local_58 - local_80 >> 0x1f,
 *                      (local_58 - local_80 ^ uVar7) - uVar7 != 1)))))) {
 *           if (piVar3[0x12] == 0) {
 *             switch(iVar4) {
 *             case 8:
 *               local_88 = 0xd;
 *               break;
 *             case 10:
 *               local_88 = 0xf;
 *               break;
 *             case 0xb:
 *               local_88 = 0x11;
 *               break;
 *             case 0x13:
 *               local_88 = 0x2c;
 *               break;
 *             case 0x17:
 *               local_88 = 0x32;
 *             }
 *             iVar4 = FUN_00431c70(piVar8,&local_80,local_10,1);
 *             if (iVar4 == 0) {
 *               piVar3[7] = piVar3[7] + -1;
 *             }
 *             else {
 *               FUN_00452060(local_88,piVar8,piVar3,local_10);
 *               if ((piVar3[0x17] & 0x100U) == 0) {
 *                 piVar3[0x12] = (int)piVar8;
 *               }
 *             }
 *           }
 *           goto LAB_00406112;
 *         }
 *         if (bVar2) {
 *           FUN_00406360(piVar3,local_50,piVar8);
 *         }
 *         DAT_004b3424 = DAT_004b3424 - 1;
 *         (&DAT_004b335c)[local_5c] = (&DAT_004b335c)[DAT_004b3424];
 *       }
 *     }
 *     local_50 = piVar3;
 *     piVar3 = (int *)piVar3[0x18];
 *   } while( true );
 * }
 */
// FUNCTION: LEGORR 0x00405c10
void aitask_pending_command_entity_assignment_tick(void)
{
    AITask* prev = NULL;
    AITask* task;
    float bestDist = 99999.0f;

    for (task = aiTaskGlobs.globalHead; task != NULL; task = task->next) {
        if (task->timeIn <= 0.0f && !(task->flags & 0x80400)) {
            struct EntitySlot* best = NULL;
            struct EntitySlot* object;
            unsigned int bestIndex;
            unsigned int i;
            float bestWPos[2];

            for (i = 0; i < aiTaskGlobs.assignCandidateCount; i++) {
                object = aiTaskGlobs.assignCandidates[i];
                if (object != NULL &&
                    ((task->flags & 0x100) || !selection_contains_entity_active_target(object, NULL)) &&
                    (!(task->flags & 0x100) ||
                     cmd_array_find_value_index((const int*)task->buffer, task->count,
                                                (int)(size_t)object, NULL))) {
                    float oPos[2];
                    float wPos[2];
                    int bPos[2];
                    int oBPos[2];
                    int nearBPos[2];
                    int flag;
                    int ok;
                    entity_transform_position_get_outputs(object, &oPos[0], &oPos[1]);
                    ok = aitask_place_command_eligibility_check(task, object, wPos, NULL, &flag, 1, 1);
                    if (ok) {
                        if (terrain_map_world_to_block(AITASK_LEGO_GET_MAP(), wPos[0], wPos[1], &bPos[0], &bPos[1]) &&
                            terrain_map_world_to_block(AITASK_LEGO_GET_MAP(), oPos[0], oPos[1], &oBPos[0], &oBPos[1])) {
                            int type = task->type;
                            int route = 1;
                            if (((type == 8 && !(task->flags & 8)) || type == 10 || type == 11 ||
                                 type == 0x17 || type == 0x13) &&
                                (oBPos[0] != bPos[0] || abs(oBPos[1] - bPos[1]) != 1) &&
                                (oBPos[1] != bPos[1] || abs(oBPos[0] - bPos[0]) != 1)) {
                                nearBPos[0] = bPos[0];
                                nearBPos[1] = bPos[1];
                                if (!first_free_neighbor_cell_pick_current(best, nearBPos, bPos, 1))
                                    route = 0;
                            }
                            if (route) {
                                int* routeX;
                                int* routeY;
                                unsigned int routeCount;
                                ok = aitask_grid_route_search_adapter(object, oBPos[0], oBPos[1],
                                                                      bPos[0], bPos[1], &routeX,
                                                                      &routeY, &routeCount);
                                if (ok) {
                                    free(routeX);
                                    free(routeY);
                                } else {
                                    task->timeIn = 75.0f;
                                }
                            }
                        }
                    }
                    if (flag) {
                        bestIndex = i;
                        bestWPos[0] = wPos[0];
                        bestWPos[1] = wPos[1];
                        best = object;
                        break;
                    }
                    if (ok) {
                        float dir[2];
                        float dx = wPos[0] - oPos[0];
                        float dy = wPos[1] - oPos[1];
                        float dist = (float)sqrt(dy * dy + dx * dx);
                        entity_surface_vector_normalize(object, dir);
                        if (best == NULL || dist < bestDist) {
                            bestIndex = i;
                            bestWPos[0] = wPos[0];
                            bestWPos[1] = wPos[1];
                            best = object;
                            bestDist = dist;
                        }
                    }
                }
            }

            if (best != NULL) {
                int attach = 1;
                int objBPos[2];
                int bPos[2];
                int type;
                entity_position_to_terrain_cell_adapter(best, &objBPos[0], &objBPos[1]);
                terrain_map_world_to_block(AITASK_LEGO_GET_MAP(), bestWPos[0], bestWPos[1], &bPos[0], &bPos[1]);
                type = task->type;
                if ((type == 0xc || type == 3 || type == 0x16 || type == 0x15) &&
                    (bPos[0] != objBPos[0] || bPos[1] != objBPos[1])) {
                    if (task->entityRef == NULL) {
                        int message;
                        switch (type) {
                        case 3:
                            message = 0x17;
                            break;
                        case 0x15:
                            message = 0x38;
                            break;
                        case 0xc:
                            message = 0x27;
                            break;
                        case 0x16:
                            message = 0x30;
                            break;
                        }
                        cmd_produce(message, best, (int)(size_t)task, bPos);
                        if (!(task->flags & 0x100))
                            task->entityRef = best;
                        if (task->type == 0xc && AITASK_OBJ(best)->type == 1) {
                            aitask_coordinate_callback_dispatch_by_mode(&task->blockX, 1);
                            task->timeIn = 0.0f;
                        }
                    }
                    attach = 0;
                } else if (((type == 8 && !(task->flags & 8)) || type == 10 || type == 11 ||
                            type == 0x17 || type == 0x13) &&
                           (objBPos[0] != bPos[0] || abs(objBPos[1] - bPos[1]) != 1) &&
                           (objBPos[1] != bPos[1] || abs(objBPos[0] - bPos[0]) != 1)) {
                    if (task->entityRef == NULL) {
                        int message;
                        int nearBPos[2];
                        switch (type) {
                        case 8:
                            message = 0xd;
                            break;
                        case 10:
                            message = 0xf;
                            break;
                        case 11:
                            message = 0x11;
                            break;
                        case 0x13:
                            message = 0x2c;
                            break;
                        case 0x17:
                            message = 0x32;
                            break;
                        }
                        if (first_free_neighbor_cell_pick_current(best, bPos, nearBPos, 1)) {
                            cmd_produce(message, best, (int)(size_t)task, nearBPos);
                            if (!(task->flags & 0x100))
                                task->entityRef = best;
                        } else {
                            task->priorityValue--;
                        }
                    }
                    attach = 0;
                }
                if (attach)
                    aitask_entity_record_attach_and_relink(task, prev, best);
                aiTaskGlobs.assignCandidates[bestIndex] =
                    aiTaskGlobs.assignCandidates[--aiTaskGlobs.assignCandidateCount];
            } else {
                if (!(task->flags & 0x1000)) {
                    for (i = 0; i < aiTaskGlobs.assignCandidateCount; i++) {
                        object = aiTaskGlobs.assignCandidates[i];
                        if (object != NULL &&
                            ((task->flags & 0x100) || !selection_contains_entity_active_target(object, NULL)) &&
                            (!(task->flags & 0x100) ||
                             cmd_array_find_value_index((const int*)task->buffer, task->count,
                                                        (int)(size_t)object, NULL))) {
                            if (aitask_place_command_eligibility_check(task, object, NULL, NULL, NULL, 0, 1)) {
                                aitask_cmd_validated_record_queue(task);
                                break;
                            }
                        }
                    }
                }
                if (task->type != 0x17 && task->field_3c != 0 && !(task->flags & 0x8000)) {
                    struct EntitySlot* found = NULL;
                    int done = 0;
                    float wPos[2];
                    for (i = 0; i < aiTaskGlobs.assignCandidateCount; i++) {
                        object = aiTaskGlobs.assignCandidates[i];
                        if (object != NULL &&
                            ((task->flags & 0x100) || !selection_contains_entity_active_target(object, NULL)) &&
                            (!(task->flags & 0x100) ||
                             cmd_array_find_value_index((const int*)task->buffer, task->count,
                                                        (int)(size_t)object, NULL))) {
                            if (aitask_place_command_eligibility_check(task, object, wPos, NULL, NULL, 0, 0)) {
                                found = object;
                                if (aitask_place_command_eligibility_check(task, object, wPos, NULL, NULL, 0, 1)) {
                                    done = 1;
                                    break;
                                }
                            }
                        }
                    }
                    if (found != NULL && !done) {
                        struct EntitySlot* trainer = aitask_object_find_trainer_adapter(NULL, wPos, task->field_3c);
                        if (trainer != NULL) {
                            aitask_task_type23_create_for_category_mask(trainer, task->field_3c,
                                                                        task->field_3c != 0x10);
                            task->flags |= 0x8000;
                        }
                    }
                }
            }
        }
        prev = task;
    }
}

/* 0x00406360 entity_record_attach_and_relink (corpus name EntityRecordAttachAndRelink), 128 bytes.
 * Current definition: src/game/aitask.cpp:4257
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00406360(int param_1,int param_2,int param_3)
 *
 * {
 *   undefined4 uVar1;
 *
 *   if ((*(byte *)(param_1 + 0x5c) & 0x40) == 0) {
 *     if (param_2 == 0) {
 *       DAT_004b3354 = *(undefined4 *)(param_1 + 0x60);
 *     }
 *     else {
 *       *(undefined4 *)(param_2 + 0x60) = *(undefined4 *)(param_1 + 0x60);
 *     }
 *     *(undefined4 *)(param_1 + 0x60) = 0;
 *     *(int *)(param_3 + 0x2f0) = param_1;
 *   }
 *   else {
 *     uVar1 = FUN_00406400(param_1);
 *     *(undefined4 *)(param_3 + 0x2f0) = uVar1;
 *     if ((*(uint *)(param_1 + 0x5c) & 0x40000) != 0) {
 *       *(undefined4 *)(param_1 + 0x18) = 0x41c80000;
 *     }
 *   }
 *   FUN_004063e0(param_3);
 *   FUN_00407450(param_3);
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00406360
void aitask_entity_record_attach_and_relink(AITask* task, AITask* prev, struct EntitySlot* object)
{
    if (task->flags & 0x40) {
        AITASK_OBJ(object)->aiTask = aitask_cmd_pending_record_copy(task);
        if (task->flags & 0x40000)
            task->timeIn = 25.0f;
    } else {
        if (prev != NULL)
            prev->next = task->next;
        else
            aiTaskGlobs.globalHead = task->next;
        task->next = NULL;
        AITASK_OBJ(object)->aiTask = task;
    }
    aitask_pending_record_entity_reference_clear(object);
    idle_register(object);
}

/* 0x004063e0 pending_record_entity_reference_clear (corpus name PendingRecordEntityReferenceClear), 32 bytes.
 * Current definition: src/game/aitask.cpp:4294
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_004063e0(int param_1)
 *
 * {
 *   int iVar1;
 *
 *   for (iVar1 = DAT_004b3354; iVar1 != 0; iVar1 = *(int *)(iVar1 + 0x60)) {
 *     if (*(int *)(iVar1 + 0x48) == param_1) {
 *       *(undefined4 *)(iVar1 + 0x48) = 0;
 *     }
 *   }
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x004063e0
void aitask_pending_record_entity_reference_clear(struct EntitySlot* object)
{
    AITask* task;
    for (task = aiTaskGlobs.globalHead; task != NULL; task = task->next) {
        if (task->entityRef == object)
            task->entityRef = NULL;
    }
}

/* 0x00406400 cmd_pending_record_copy (corpus name PendingRecordCopyToEntityAndFlag), 64 bytes.
 * Current definition: src/game/aitask.cpp:4309
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00406400(undefined4 *param_1)
 *
 * {
 *   undefined4 uVar1;
 *   undefined4 *puVar2;
 *   int iVar3;
 *   undefined4 *puVar4;
 *
 *   puVar2 = (undefined4 *)FUN_00406440(*param_1);
 *   uVar1 = puVar2[0x19];
 *   puVar4 = puVar2;
 *   for (iVar3 = 0x1a; iVar3 != 0; iVar3 = iVar3 + -1) {
 *     *puVar4 = *param_1;
 *     param_1 = param_1 + 1;
 *     puVar4 = puVar4 + 1;
 *   }
 *   puVar2[0x19] = uVar1;
 *   puVar2[0x17] = puVar2[0x17] | 0x200;
 *   puVar2[0x18] = 0;
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00406400
AITask* aitask_cmd_pending_record_copy(AITask* task)
{
    AITask* copy = aitask_cmd_alloc(task->type);
    AITask* nextFree = copy->nextFree;
    *copy = *task;
    copy->nextFree = nextFree;
    copy->flags |= 0x200;
    copy->next = NULL;
    return copy;
}

/* 0x00406440 cmd_alloc (corpus name CmdAlloc), 64 bytes.
 * Current definition: src/game/aitask.cpp:4328
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * undefined4 * FUN_00406440(undefined4 param_1)
 *
 * {
 *   undefined4 *puVar1;
 *   DWORD DVar2;
 *   int iVar3;
 *   undefined4 *puVar4;
 *
 *   if (DAT_004b31f8 == (undefined4 *)0x0) {
 *     FUN_004064c0();
 *   }
 *   puVar1 = DAT_004b31f8;
 *   puVar4 = DAT_004b31f8;
 *   DAT_004b31f8 = (undefined4 *)DAT_004b31f8[0x19];
 *   for (iVar3 = 0x1a; iVar3 != 0; iVar3 = iVar3 + -1) {
 *     *puVar4 = 0;
 *     puVar4 = puVar4 + 1;
 *   }
 *   puVar1[0x19] = puVar1;
 *   DVar2 = timeGetTime();
 *   puVar1[0x14] = DVar2;
 *   *puVar1 = param_1;
 *   return puVar1;
 * }
 */
// FUNCTION: LEGORR 0x00406440
AITask* aitask_cmd_alloc(int type)
{
    AITask* task;
    if (aiTaskGlobs.freeList == NULL)
        aitask_cmd_pool_grow();
    task = aiTaskGlobs.freeList;
    aiTaskGlobs.freeList = task->nextFree;
    memset(task, 0, sizeof(AITask));
    task->nextFree = task;
    task->field_50 = timeGetTime();
    task->type = type;
    return task;
}

/* 0x00406480 cmd_free (corpus name CmdFree), 64 bytes.
 * Current definition: src/game/aitask.cpp:4346
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_00406480(int param_1,int param_2)
 *
 * {
 *   if (param_2 == 0) {
 *     FUN_00402cd0(param_1);
 *   }
 *   if ((*(int *)(param_1 + 0x40) != 0) && ((*(uint *)(param_1 + 0x5c) & 0x200) == 0)) {
 *     FUN_0048db80(*(int *)(param_1 + 0x40));
 *   }
 *   *(int *)(param_1 + 100) = DAT_004b31f8;
 *   DAT_004b31f8 = param_1;
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x00406480
void aitask_cmd_free(AITask* task, int noCancel)
{
    if (!noCancel)
        aitask_cmd_cancel_ref(task);
    if (task->buffer != NULL && !(task->flags & 0x200))
        free(task->buffer);
    task->nextFree = aiTaskGlobs.freeList;
    aiTaskGlobs.freeList = task;
}

/* 0x004064c0 cmd_pool_grow (corpus name CmdPoolGrow), 128 bytes.
 * Current definition: src/game/aitask.cpp:4370
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * void FUN_004064c0(void)
 *
 * {
 *   int iVar1;
 *   int iVar2;
 *   undefined4 uVar3;
 *   int iVar4;
 *   uint uVar5;
 *
 *   uVar5 = 1 << ((byte)DAT_004b31fc & 0x1f);
 *   uVar3 = FUN_0048dbd0(uVar5 * 0x68);
 *   (&DAT_004b31c8)[DAT_004b31fc] = uVar3;
 *   iVar1 = (&DAT_004b31c8)[DAT_004b31fc];
 *   if (iVar1 != 0) {
 *     DAT_004b31fc = DAT_004b31fc + 1;
 *     if (1 < uVar5) {
 *       iVar4 = uVar5 - 1;
 *       iVar2 = iVar1;
 *       do {
 *         *(int *)(iVar2 + 100) = iVar2 + 0x68;
 *         iVar4 = iVar4 + -1;
 *         iVar2 = iVar2 + 0x68;
 *       } while (iVar4 != 0);
 *     }
 *     *(int *)(iVar1 + -4 + uVar5 * 0x68) = DAT_004b31f8;
 *     DAT_004b31f8 = iVar1;
 *   }
 *   return;
 * }
 */
// FUNCTION: LEGORR 0x004064c0
void aitask_cmd_pool_grow(void)
{
    unsigned int count = 1 << aiTaskGlobs.poolBlockCount;
    AITask* block;
    unsigned int i;

    aiTaskGlobs.poolBlocks[aiTaskGlobs.poolBlockCount] = (AITask*)malloc(sizeof(AITask) * count);
    block = aiTaskGlobs.poolBlocks[aiTaskGlobs.poolBlockCount];
    if (block != NULL) {
        aiTaskGlobs.poolBlockCount++;
        for (i = 1; i < count; i++)
            block[i - 1].nextFree = &block[i];
        block[count - 1].nextFree = aiTaskGlobs.freeList;
        aiTaskGlobs.freeList = block;
    }
}

/* 0x00406540 EntityPoolCallbackSearch (corpus name EntityPoolCallbackSearch), 144 bytes.
 * Current definition: none
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * undefined4 FUN_00406540(code *param_1,undefined4 param_2)
 *
 * {
 *   int iVar1;
 *   int iVar2;
 *   int *piVar3;
 *   uint uVar4;
 *   uint uVar5;
 *   uint local_4;
 *
 *   local_4 = 0;
 *   if (DAT_004b31fc == 0) {
 *     return 0;
 *   }
 *   piVar3 = &DAT_004b31c8;
 *   do {
 *     if (*piVar3 != 0) {
 *       uVar5 = 0;
 *       uVar4 = 1 << ((byte)local_4 & 0x1f);
 *       if (uVar4 != 0) {
 *         iVar2 = 0;
 *         do {
 *           iVar1 = *piVar3 + iVar2;
 *           if (((iVar1 != 0) && (*(int *)(iVar1 + 100) == iVar1)) &&
 *              (iVar1 = (*param_1)(iVar1,param_2), iVar1 != 0)) {
 *             return 1;
 *           }
 *           uVar5 = uVar5 + 1;
 *           iVar2 = iVar2 + 0x68;
 *         } while (uVar5 < uVar4);
 *       }
 *     }
 *     local_4 = local_4 + 1;
 *     piVar3 = piVar3 + 1;
 *     if (DAT_004b31fc <= local_4) {
 *       return 0;
 *     }
 *   } while( true );
 * }
 */
// FUNCTION: LEGORR 0x00406540
int aitask_entity_pool_callback_search(int (*callback)(AITask* task, void* data), void* data)
{
    unsigned int list, i, count;
    for (list = 0; list < aiTaskGlobs.poolBlockCount; list++) {
        if (aiTaskGlobs.poolBlocks[list] != NULL) {
            count = 1 << list;
            for (i = 0; i < count; i++) {
                AITask* task = &aiTaskGlobs.poolBlocks[list][i];
                if (task != NULL && task->nextFree == task) {
                    if (callback(task, data))
                        return 1;
                }
            }
        }
    }
    return 0;
}

/* 0x004065d0 renderer_global_dword_get (corpus name GlobalDwordGet), 16 bytes.
 * Current definition: src/game/aitask.cpp:4400
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * undefined4 FUN_004065d0(void)
 *
 * {
 *   return DAT_0076ac84;
 * }
 */
// FUNCTION: LEGORR 0x004065d0
unsigned int renderer_global_dword_get(void)
{
    return s_renderer_global_dword;
}

/* 0x004065e0 render_surface_secondary_get (corpus name RenderSurfaceSecondaryGet), 16 bytes.
 * Current definition: src/game/aitask.cpp:4403
 *
 * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):
 * undefined4 FUN_004065e0(void)
 *
 * {
 *   return DAT_0076ac8c;
 * }
 */
// FUNCTION: LEGORR 0x004065e0
unsigned int render_surface_secondary_get(void)
{
    return s_render_surface_secondary;
}

/* Port-only accessors (finding 570): the port tail (aitask_port.cpp,
 * cmd.cpp) reads and writes the shared priority tables through these. */
// PORT-ONLY: the port tail's view of the shared priority values (004b32e8)
int* aitask_port_priority_values(void)
{
    return aiTaskGlobs.priorityValues;
}

// PORT-ONLY: the port tail's view of the shared priority states (004b7ff4)
int* aitask_port_priority_states(void)
{
    return aiTaskGlobs.priorityState;
}
