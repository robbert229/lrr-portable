// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what aitask uses from cmd.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GAME_CMD_FOR_AITASK_HPP
#define LRR_GAME_CMD_FOR_AITASK_HPP

/* Port-only identity for the demonstrated GetTool route. No retail offsets
 * are assigned to this tail; copies must start without a movement owner. */
typedef struct CmdGetToolRoute {
    EntitySlot* actor;
    EntitySlot* store;
    const AiWaypoint* waypoints;
    int count;
    int phase;                 /* 0 unissued, 1 moving, 2 terminal pending */
    float endpoint[2];
} CmdGetToolRoute;

typedef struct CmdRec {
    int32_t  type;       /* +0x00 record type (0 move/2 target/3 task/5 gated) */
    int32_t  param;      /* +0x04 (flag for move, ext args)                   */
    float    x;          /* +0x08 opaque word; kind 0 = signed integer cell   */
    float    y;          /* +0x0c                                             */
    void*    payload;    /* +0x10 entity / target pointer                      */
    float    now;        /* +0x14 timeGetTime on alloc, 1500.0 on link          */
    int32_t  param2;     /* +0x18 (task param from the queue)                  */
    int32_t  behaviour;  /* +0x1c behaviour id = behaviour[kind]                */
    int32_t  cell_act;   /* +0x20 cell-command action id (8/0xb/0xd)          */
    int32_t  cell_target;/* +0x24 cell-command referenced entity (-1 = unset)   */
    int32_t  cell_count; /* +0x28 cell-command entry count                      */
    int32_t  zone;       /* +0x2c zone-command id (kind-7; the corpus writer
                          * of this word is unported — zero-init)             */
    uint8_t  pad_30[8];  /* +0x30..+0x37 (keeps the corpus +0x38 slot)          */
    void*    arg38;      /* +0x38 raw word/payload (type 0x13 selector)        */
    uint32_t field_3c;   /* +0x3c task-kind selector (when supplied)          */
    void*    buffer;     /* +0x40 owned buffer (freed on cmd_free)             */
    uint32_t count;      /* +0x44 the buffer's entry count                     */
    void*    entity_ref; /* +0x48 pending entity reference (the corpus
                           * writers are the spawn-dispatch band, unported —
                           * zero until one installs a value)                   */
    int32_t  kind;       /* +0x4c kind index                                   */
    void*    global_next; /* +0x50 port-only global command-list link           */
    void*    event_payload; /* +0x54 event payload (the 0xc queue's word)       */
    void*    ref;        /* +0x58 referenced record (the 0x13 "ref" pair)      */
    uint32_t state;      /* +0x5c state flags (CMD_STATE_*)                    */
    void*    next;       /* +0x60 entity-chain link; port reuses it when free  */
    int32_t  grant_act;  /* port-only staged activity (deferred install 0x43da62) */
    int32_t  extra;      /* port-only task selector (type-8 tasks set 8)        */
    CmdGetToolRoute gettool; /* cleared before reset, requeue, free or copy */
} CmdRec;

#define CMD_POOL_MAX_BLOCKS 12

/* Pool bookkeeping is the corpus's DAT_004b31c8 band: CmdPoolGrow stores
 * each block at [level*4+0x4b31c8] and the level at DAT_004b31fc;
 * CmdAlloc/CmdFree pop and push the freelist through DAT_004b31f8
 * (records link at +0x64). All three are zero-init in the corpus.
 * On the 64-bit host test build, keep the pointer-sized `next` link because
 * the private +0x64 word is only 32 bits; the 32-bit target uses grant_act. */
#if defined(_M_IX86) || defined(__i386__)
#define CMD_POOL_LINK_GET(rec) \
    ((CmdRec*)(uintptr_t)(uint32_t)(rec)->grant_act)
#define CMD_POOL_LINK_SET(rec, value) \
    ((rec)->grant_act = (int32_t)(uintptr_t)(value))
#else
#define CMD_POOL_LINK_GET(rec) ((rec)->next)
#define CMD_POOL_LINK_SET(rec, value) ((rec)->next = (value))
#endif

#define PRIORITY_ENTRY_COUNT 27

#if !defined(_MSC_VER) || !defined(_M_IX86)
#define CMD_TASK_NAME_SET(index, value) \
    (((const char* volatile*)s_task_names)[index] = value)
#define CMD_PRIORITY_NAME_SET(index, value) \
    (((const char* volatile*)s_priority_names)[index] = value)
#endif

extern CommandKindRecordHooks s_command_kind_record_hooks;
extern CmdEntityChainExternalOps s_cmd_entity_chain_external_ops;
CmdRec* as_rec(void* p);
const CmdRec* cas_rec(const void* p);
extern uint8_t* s_block_live[CMD_POOL_MAX_BLOCKS];
extern CmdRec* s_global_tail;
int pending_command_cell_walkable(GameContext* context, const EntitySlot* entity, int x, int y, int mode);
int cmd_place_target_route_position_get(void*, const EntitySlot* entity, float* x, float* y);
int cmd_place_position_to_world(void*, int x, int y, float* world_x, float* world_y);
int pending_command_activity_gate(const EntitySlot* entity, int kind);
extern unsigned s_pending_assignment_count;
extern CmdPurgeCellPredicate s_cmd_purge_cell_usable;
extern CmdPurgeCellPredicate s_cmd_purge_neighbor_eligible;
extern CmdPurgeCompare s_cmd_purge_compare;
extern void* s_cmd_purge_context;
int cmd_gettool_store_live(const CmdGetToolRoute* route);
void cmd_gettool_record_release(CmdRec* record);
void cmd_gettool_all_owners_release(void);
int cmd_purge_compare_adapter(const void* left, const void* right);
int cmd_purge_compare_default(const void* left, const void* right);
extern CoordinatePairEntityCallback s_coordinate_pair_entity_callback;
extern ArgumentEntityCallback s_argument_entity_callback;
extern void* s_callback_dispatch_context;
extern TaskCallbackEntityFn s_task_callback_entity_fn;
extern void* s_task_callback_entity_context;
extern TaskCallbackEntityFn s_pending_record_callback_fn;
extern void* s_pending_record_callback_context;
extern EntityTaskQueueFlushHooks s_entity_task_queue_flush_hooks;
namespace unported_ledger_stubs {
extern ObjectTaskEligibilityOps* s_object_task_eligibility_ops;
} // namespace unported_ledger_stubs

#endif /* LRR_GAME_CMD_FOR_AITASK_HPP */
