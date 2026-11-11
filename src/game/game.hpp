// SPDX-License-Identifier: LGPL-3.0-or-later
/* Game layer (C++).
 *
 * The game code (entities, missions, command queue, selection — findings
 * 03/06/10) lived outside the gods98 engine. Decompiled game functions land
 * here as they are produced. This milestone ports the *mission plumbing*:
 *
 *   level.h     level state + `.map` loading pipeline (finding 07)
 *   objective.h objective/advisor config + state machine (finding 22)
 *   savegame.h  `.sav`/`.osf` persistence + checkpoint blob (finding 08)
 *
 * GameContext is the reconstruction's home for the corpus globals these
 * modules touch — DAT_005560c4 (config DB), DAT_005560d4 (level block),
 * DAT_00556ed0 (cheat/feature word), DAT_004ffbc0 (objective word) and the
 * save/objective tables — so no file-scope mutable state leaks across tests.
 * The engine owns the frame tick; the game layer owns the mission state.
 *
 * Discipline (ADR 0003/0004): plain C-shaped code, no OS-facing headers
 * below the seam (src/platform/), no heavy STL.
 */
#ifndef LEGORRR_GAME_HPP
#define LEGORRR_GAME_HPP

#include <cstdint>

#include "gods98/block.hpp"
#include "level.hpp"
#include "options.hpp"

struct TextureSurface;

#ifdef __cplusplus
extern "C" {
#endif

/* Stub identity string — replaced by real init when decompiled. */
const char* game_layer_name(void);

/* The 10 named mission-task slots (finding 09 §"Task record").
 * CheckpointBlobBuild copies these into the blob verbatim (stride 0x210),
 * so the save module's checkpoint carries task progress state. */
#define GAME_TASK_COUNT 10
#define GAME_TASK_STRIDE 0x210

typedef struct GameTask {
    /* Raw record, laid out at the corpus's task offsets (finding 09):
     * +0x00 flag byte (&1 completed, &2 active), +0x04 name, +0x24
     * importance (float), +0x28 quota (int). Progress (reconstruction-side)
     * lives at +0x2c so the checkpoint blob's verbatim copy stays
     * layout-faithful. */
    uint8_t data[GAME_TASK_STRIDE];
} GameTask;

/* Typed accessors over the raw task record. */
static inline uint32_t task_flags(const GameTask* t)
{
    uint32_t v; __builtin_memcpy(&v, t->data + 0x00, 4); return v;
}
static inline void task_set_flags(GameTask* t, uint32_t v)
{
    __builtin_memcpy(t->data + 0x00, &v, 4);
}
static inline const char* task_name(const GameTask* t)
{
    return (const char*)t->data + 0x04;
}
static inline void task_set_name(GameTask* t, const char* n)
{
    char* dst = (char*)t->data + 0x04;
    dst[0] = '\0';
    if (n != NULL) {
        for (int i = 0; i < 63 && n[i] != '\0'; ++i) {
            dst[i] = n[i];
            dst[i + 1] = '\0';
        }
    }
}
static inline float task_importance(const GameTask* t)
{
    float v; __builtin_memcpy(&v, t->data + 0x24, 4); return v;
}
static inline void task_set_importance(GameTask* t, float v)
{
    __builtin_memcpy(t->data + 0x24, &v, 4);
}
static inline int32_t task_quota(const GameTask* t)
{
    int32_t v; __builtin_memcpy(&v, t->data + 0x28, 4); return v;
}
static inline void task_set_quota(GameTask* t, int32_t v)
{
    __builtin_memcpy(t->data + 0x28, &v, 4);
}
static inline int32_t task_value(const GameTask* t)
{
    int32_t v; __builtin_memcpy(&v, t->data + 0x2c, 4); return v;
}
static inline void task_set_value(GameTask* t, int32_t v)
{
    __builtin_memcpy(t->data + 0x2c, &v, 4);
}
static_assert(GAME_TASK_COUNT * GAME_TASK_STRIDE <= 0x318c,
              "task block fits the checkpoint blob's 0x318c budget");

/* Per-level Reward group snapshot (finding 09 §"Init / teardown"):
 * the values CheckpointBlobBuild reads/writes. */
typedef struct GameReward {
    int     enable;      /* tri-state, 2 -> 0 on read */
    int     modifier;
    float   timer;       /* default 76.9375 */
    int     display_text;
    int     display_images;
    int     display_flics;
} GameReward;

/* Live mission-progress counters (finding 09 §"RewardScoreAdd"; the corpus
 * keeps them in the DAT_005557xx score cluster). These feed the objective
 * checker (level +0x8c/+0xa0 live counts) and the task value slots. */
typedef struct GameMission {
    int32_t crystals;       /* live crystal count               */
    int32_t ore;            /* live ore count                   */
    int32_t score;          /* mission score                    */
    int32_t constructions;  /* constructions (ConstructionObjective) */
    int32_t blocks;         /* walls/blocks built (BlockObjective)     */
} GameMission;

/* Reward (post-mission) display config (finding 09 — RewardInit keys). The
 * font/image *handles* are the render lane; the config strings and the
 * Save/Advance button hit-zones are kept for the save trigger. */
typedef struct RewardScreen {
    char  wallpaper[128];
    int   display;          /* `Display` tri-state, 2 -> 0 */
    int   centre_text;
    float scroll_speed;
    float vert_spacing;
    char  font_base[64];    /* DAT_00555b3c/config name */
    char  font_title[64];   /* DAT_00555b44 */
    char  font_back[64];    /* DAT_00555b4c */
    char  complete_text[128];
    char  failed_text[128];
    char  quit_text[128];
    /* Formatted result label selected by RewardStatsPanelDraw for the
     * renderer callback; sourced from the three config strings above. */
    char  stats_result_text[256];
    int   text_pos[2];      /* `TextPos` token pair */
    /* SaveButton / AdvanceButton hit zones (corpus DAT_00555b60..b98). */
    float save_x, save_y;
    float advance_x, advance_y;
    /* Button hit/draw sizes — derived from the normal widget surfaces just
     * as the corpus reads the loaded bitmap dimensions. */
    float save_w, save_h;
    float advance_w, advance_h;
    /* RewardSaveButtonZonesInit loads four bitmap states per button. The
     * normal surfaces define the interactive dimensions; all refs belong to
     * this GameContext reward config and are released on reinit/disposal. */
    struct TextureSurface* save_button_images[4];
    struct TextureSurface* advance_button_images[4];
} RewardScreen;

/* Entity slot model (finding 10 §"EntityPointer"): the corpus keeps four
 * typed arrays with fixed strides (type 1 vehicle 0x1ec, 2/3 minifigure
 * 0x74, 4 building/object 0x14c — exposed via entity_stride). The
 * reconstruction stores a compact record per slot — enough for placement
 * data, the counts the objective checks consume, and the command-chain
 * head (entity +0x2f0) the command subsystem hangs off. `type`/`etype`
 * carry the corpus entity-type code (1..9; placement kinds 1..4 map to the
 * four tables, other codes share the last table). */
#define ENTITY_TYPES 5 /* 4 typed tables + the special-object table (5..19) */
#define ENTITY_CAP 256
/* Selection table bound (finding 06: adds cap at 100 entries). */
#define SELECTION_MAX 100
/* Selected flag word on EntitySlot.flags (corpus entity +0xfa & 0x200000). */
#define ENTITY_SELECTED 0x200000u
/* Path-list entry (corpus entity +0x24 buffer, stride 5 dwords/entry):
 * x/y are numeric CELL coordinates. +8/+c are subcell interpolation weights;
 * the flags at +0x10 may be zero on a valid route (0x441700). wp_index owns
 * progression; zero flags do not mean an entry was consumed. */
#define AI_WAYPOINTS_MAX 16 /* inline waypoint span; longer lists realloc-grow */
/* Permitted-activity list bound (corpus entity +0x370 id slots / +0x384 count):
 * the inline span +0x370..+0x384 is five dwords, so an entity holds at most
 * five permitted activity ids before the setter (FUN_0044b370) recycles
 * entries — the port models the stats-block capacity as
 * GameContext.stats_activity_cap (see below). */
#define AI_ACTIVITY_SLOTS 5
/* Bound of the legacy target/work rate adapter. Stats +0x24..34 are drill
 * times in reference ticks; ordinary walking reads RouteSpeed at +0x20. */
#define AI_STATS_TERRAIN_TYPES 16
typedef struct AiWaypoint {
    float   x, y;            /* cell route point, offsets 0x00/0x04 */
    float   weight_y;         /* interpolation weight, record offset 0x08 */
    float   weight_x;         /* interpolation weight, record offset 0x0c */
    uint8_t pending;          /* record flags at +0x10; arrival clears bit 0 */
    uint8_t step_kind;        /* record kind at +0x11 */
    uint8_t reserved[2];      /* complete the corpus's 0x14-byte stride */
} AiWaypoint;
typedef char AiWaypointSizeCheck[(sizeof(AiWaypoint) == 0x14) ? 1 : -1];

/* Portable owner for the renderer-backed ActivityRunner records at retail
 * entity offsets +0x10/+0x14. Source and chain values are dword handles
 * copied from a type-resource row; host pointers stay in sidecars so they
 * are never truncated on 64-bit builds. The retail +0x54 cache has eight
 * slots inside the 0x74-byte runner record. */
#define ENTITY_ACTIVITY_RUNNER_CACHE_MAX 8u
typedef struct EntityActivityRunnerSidecar {
    uint32_t scene_token;
    uint32_t source_token;
    uint32_t slot_count;
    uint32_t template_row;
    uint32_t component_gate_value;
    uint32_t chain_tokens[3];
    uint32_t activity_values[5];
    void* child_scene;
    void* resource_chains[3];
    void* indexed_cache[ENTITY_ACTIVITY_RUNNER_CACHE_MAX];
    uint32_t bound;
} EntityActivityRunnerSidecar;

/* Type-1 camera source record. Retail EntityActivityChangeDispatch reads its
 * scene resource at +0xe0 and caches indexed camera children from +0x10c;
 * the host keeps the scene instance and cache in native-width sidecars. */
#define ENTITY_SCENE_RESOURCE_INDEX_CACHE_MAX 56u
typedef struct EntitySceneResourceSidecar {
    uint32_t scene_token;
    uint32_t template_row;
    void* scene_resource;
    void* indexed_cache[ENTITY_SCENE_RESOURCE_INDEX_CACHE_MAX];
    uint32_t bound;
} EntitySceneResourceSidecar;

/* Host-width node for SceneEntitySlotTableRebuild's renderer-owned list.
 * The retail record stores its renderer at +4, optional resource payload at
 * +0xc, and the next record at +0x20. Keep the exact semantic links here so
 * 64-bit renderer pointers never pass through retail dwords. */
typedef struct EntitySceneSlotNode {
    void* renderer_record; /* retail node +4 */
    void* payload;         /* retail node +0xc */
    struct EntitySceneSlotNode* next; /* retail node +0x20 */
    void* slot_handles[4];
    uint32_t slot_handle_count;
    int32_t animation_mode;
} EntitySceneSlotNode;

typedef struct EntitySlot {
    int32_t type;       /* corpus entity-type code (1..9)         */
    int32_t etype;      /* same, stored for the cmd task mapping  */
    char    name[32];
    float   x, y, z;         /* world pose; horizontal Y is inverted vs grid */
    float   rot;
    float   movement_tick_scale; /* corpus entity +0xae, reset to 0.01 */
    float   velocity_x;
    float   velocity_y;
    float   velocity_z;
    float   facing_target_x;
    float   facing_target_y;
    float   facing_target_z;
    float   type5_finalize_timeout;
    uint32_t last_action_event;
    uint32_t last_action_event_flags;
    int32_t flags;      /* reconstruction-side flag word          */
    /* --- .ol grammar (decompile-verified, FUN_0042e020/e8b0) ----- */
    char    type_name[24]; /* .ol `type` name (e.g. "SmallDigger") */
    char    driving[32];   /* .ol `driving` target object (or "")  */
    int     health;        /* .ol `health` (default 100)           */
    /* --- model-aware rendering (finding 10/30) ------------------ */
    char    model_path[64]; /* .ol `Model` key (best-effort)       */
    const void* model;      /* parsed LWO Model* (entity_model cache) */
    /* --- command layer (finding 10) --- */
    void*   cmd_head;   /* chain-head command record (entity +0x2f0) */
    void*   route_partner_300; /* reciprocal partner link for step commands */
    uint32_t flags_3e0; /* mirror of entity +0x3e0 (bit 0x400 gates gated cmds) */
    uint32_t flags_3e8; /* mirror of entity +0x3e8 (type-6 high bit → kind) */
    /* --- entity-tick state (callback 0x43c810, finding 10) --------- */
    uint32_t flags_3e4; /* +0x3e4 (bit 0x8000000 = wake-timer running)  */
    uint32_t flags_3ec; /* +0x3ec (activity/behaviour flags)            */
    uint32_t flags_390; /* +0x390: BuildingTypes path-effect veto bits */
    int32_t command_mode_404; /* per-entity mode written by route commands */
    /* +0x3f0: level-state mask word. The corpus's two readers are this
     * module's selection gate (CurrentLevelStateFlagsContainMask) and the
     * unported sim-interaction update; no ported writer feeds it — a
     * zero-init mirror per the flags_3e0 precedent. */
    uint32_t flags_3f0;           /* +0x3f0 (level-state mask word)         */
    uint32_t activity_grant_state_3f4; /* +0x3f4: cleared by FUN_0044b370 */
    /* --- static task-state (spawn-dispatch switch 0x438150) -------- *
     * EntityCreateByType assigns one of three static +0x334 templates to
     * types 8, 9, 0xc..0xe, and 0x10..0x13. activity_cap mirrors +0xcc for
     * the permitted-activity appender. The port uses a same-name Stats row's
     * NumOfToolsCanCarry as a capacity adapter when available; retail's
     * static template word is zero and stays zero without that row. */
    int      task_state;    /* +0x334 role: static task-state id, or
                             * AI_TASK_STATE_NONE (= -1) for instance stats */
    int      activity_cap;  /* state +0xcc: permitted-activity capacity;
                             * task-state value or context fallback          */
    /* Retail +0x1c component. It occupies the slot stats_record left at
     * +0x114 in the 32-bit layout, so no other member moves (finding 520). */
    void* type_operation_fallback_1c;
    int      activity;       /* +0x2e8: activity-table index, -1 = none */
    int      activity_last;  /* +0xbb: last completed activity, -1 none */
    int      activity_param; /* +0x2d8 (activity argument)              */
    float    activity_timer; /* +0x2d4 (run since activity set)         */
    float    wake_timer;     /* +0x368 (counts down; early-returns tick) */
    void*    link_target;    /* +0x354: acquired target-acquire link     */
    void*    wake_target;    /* +0x364: attached wake target (FUN_0044c110;
                               * the corpus fills it via the attach-pair
                               * writer 0x44c0e2, which is not ported)      */
    uintptr_t field_300;      /* retail +0x300 pointer tested by the spatial
                               * filter; no portable producer is recovered */
    const void* attach_304;  /* +0x304: attached object the selection
                              * filter follows in place of the candidate   */
    const void* owner_36c;   /* +0x36c: owner pointer read by the
                              * context-selected predicate and the
                              * selection-command dispatch (pointer-sized
                              * mirror of the corpus's 32-bit word)         */
    /* --- carried-object pair (corpus +0x308/+0x324/+0x348) ---------------
     * A type-3 carrier links the object it carries through +0x308; +0x324
     * holds the carry state the detach clears, and +0x348 points at the
     * reference whose dword the detach folds into the level's field 0x9c. */
    void*    carry_link_308;  /* +0x308: carried object link            */
    int32_t  carry_state_324; /* +0x324: carry state word               */
    uint32_t field_328;       /* +0x328 counter read by tutorial 0x14/0x15 */
    void*    carry_ref_348;   /* +0x348: carried reference (dword read) */
    float    command_chain_idle_elapsed_338; /* +0x338 callback timer */
    float    field_340;      /* +0x340 float; the wake mirror sets 100.0
                              * (its general corpus readers are TODO)       */
    int32_t  positioned_render_state_34c; /* proximity-driven render state */
    /* --- route-progress advance (corpus +0x344/+0x2fc, finding 448) -----
     * The corpus advances a routing progress float by a fixed step and
     * clamps it to the entity's own +0x340 bound, clearing the +0x2fc
     * link field in passing. */
    float    progress_344;   /* +0x344: routing progress float            */
    uintptr_t link_2fc;    /* +0x2fc: link field the advance clears (corpus
                                 * dword link, widened for the port) */
    /* --- eligibility coordinate pair (corpus +0x2f4/+0x2f8) ---------------
     * The activity/terrain eligibility transition parks its last probed
     * cell pair here and blanks it to -1.0/-1.0 when it hands the cell off
     * to queued work. */
    int      drill_sound_handle_350; /* corpus +0x350 loop sound handle */
    float    pos_pair_2f4; /* +0x2f4: pair x (port-side mirror)             */
    float    pos_pair_2f8; /* +0x2f8: pair y                                */
    /* --- path list (corpus +0x24 buffer / +0x28 count / +0x2c index) --- */
    AiWaypoint wp[AI_WAYPOINTS_MAX];
    AiWaypoint* wp_dynamic; /* realloc-grown continuation of the inline list */
    unsigned int wp_capacity;
    int      wp_count;       /* +0x28: valid entries                      */
    int      wp_index;       /* +0x2c: entry being walked                 */
    /* --- permitted-activity list (corpus +0x370 ids / +0x384 count) ----- *
     * Activities the entity is permitted to run, as table indices. The
     * FUN_0044b480 scan and the FUN_0043c440 can-do probe read this list
     * (activities 4/5/7 = drill/teleport/reinforce), and FUN_0044b370 is
     * the appender/recycler that command and dispatch installs run. */
    int32_t  act_list[AI_ACTIVITY_SLOTS]; /* +0x370: valid entries first   */
    int      act_count;                   /* +0x384: valid entries, 0 = no
                                           * permission (probe false)       */
    struct GameContext* owner_ctx; /* port back-reference for queued spawns */
    uint32_t effect_counter_a; /* additive effects from record +4 */
    uint32_t effect_counter_b; /* additive effects from record +8 */
    /* Activity counters mirrored from corpus offsets +0x3d4/+0x3d8/+0x3dc.
     * These are advances driven by the entity frame delta. */
    float activity_counter_3d4;
    float activity_counter_3d8;
    float activity_counter_3dc;
    float auxiliary_state_388; /* +0x388: cursor-target timeout */
    uint32_t auxiliary_slot_38c;
    uint32_t indexed_record_flags;
    uint32_t indexed_record_flag_index;
    /* Scene-owner sidecar for the corpus's variable attachment array at
     * +0x308/+0x324. The pointer is borrowed from the scene owner; entity
     * creation zero-initializes it and the scene owner clears it on release. */
    struct EntitySlot** attached_children;
    uint32_t attached_child_count;
    void* attached_scene_object;
    uint32_t ddiscene_owner_token; /* stable 32-bit owner link for scene callbacks */
    int32_t attached_scene_slot_index; /* corpus child +0x320 */
    int32_t indexed_record_slot_330;   /* corpus selector at +0x330 */
    void* attached_direction_record_type1;
    void* attached_direction_record_type2;
    void* attached_direction_record_type3;
    void* attached_direction_record_type4;
    /* Direction state written by AttachedEntityDirectionVectorUpdate
     * (+0x2ac..+0x2b8 in the corpus image). */
    float attached_direction[3];
    /* Retail +0x334: pointer to the entity's selected 0x150-byte Stats
     * record, set by EntityBuildingIndexSet (0x469c10) from the level index
     * at retail +0x330, or to a static template by EntityCreateByType. The
     * 32-bit layout puts it at its original +0x334 (finding 520); the native
     * EntitySlot is otherwise not a retail image. NULL selects the zero
     * template on portable hosts. */
    const void* stats_record;
    float attached_direction_length;
    uint32_t action_consume_counters[6];
    uint32_t action_consume_counters_initialized;
    int32_t payload_state_8c;
    void* payload_handle_04;
    void* payload_aux_handle_7c;
    void* payload_resource_chains[3];
    void* visual_resource_handles[4]; /* per-type cached scene visual */
    void* type_component_handle; /* retail 0x4a200 result consumed by
                                  * SurfaceVectorNormalize */
    void* payload_owned_blocks[5];
    uint32_t payload_flags_70;
    void* scene_slot_cache[12];
    uint32_t scene_slot_generation;
    /* Retail +0xa0/+0xb8/+0xc4 windows are separate per-payload arrays:
     * two repeated +0x1c registrations, then one +0x24 and one +0x20. */
    void* scene_slot_owner_1c_handles[3][2];
    void* scene_slot_owner_24_handles[3];
    void* scene_slot_owner_20_handles[3];
    uint32_t scene_slot_owner_1c_counts[3];
    int32_t task_pair_3b4[2];
    int32_t task_scalar_3bc;
    uintptr_t pair_link_3c0; /* reciprocal entity link at corpus +0x3c0 */
    float field_33c;         /* +0x33c spawn timer mirror, stored at the
                              * end to avoid shifting other port fields. */
    uint32_t tutorial_unit_at_block_3f8; /* +0x3f8 tutorial cell membership
                                          * marker written by callback 0x4570d0. */
    /* Type-10 records retain their animation-resource record at retail
     * +0x20; the resource getter in EntityTypeOperationRelease reads it. */
    void* attached_direction_record_type10;
    /* Spawn-request child link from retail +0x3cc. The generic entity pool
     * can retain the relationship even when its renderer wrapper is absent. */
    struct EntitySlot* spawn_child_3cc;
    EntityActivityRunnerSidecar activity_runner_type2;
    EntityActivityRunnerSidecar activity_runner_type3;
    EntitySceneResourceSidecar scene_resource_type1;
    EntitySceneSlotNode* scene_slot_nodes;
    uint32_t scene_slot_node_count;
    uint32_t scene_slot_registration_count;
    uint32_t scene_slot_cache_count;
    void* scene_slot_payloads[12];
    uint32_t scene_slot_owner_keys[3]; /* retail owner +0x1c/+0x20/+0x24 */
    uintptr_t pool_link_408; /* +0x408 self-pointer while live, free-list link after teardown */
    /* Remaining reference +0x30c..+0x320 carrier slots. Append sidecars
     * to preserve existing member offsets; slot zero is carry_link_308.
     * Never index the native EntitySlot by reference byte offsets. */
    struct EntitySlot* carry_extra_links[6];
    /* Owned 0x44a310 inputs; native EntitySlot is not a retail byte image. */
    int32_t motion_input_2bc;
    int32_t motion_input_2c0;
    float motion_input_2c4;
    uint32_t motion_inputs_initialized;
    uint32_t activity_generation;
    uint32_t activity_advance_valid;
    uint32_t activity_advance_generation;
    int activity_advance_activity;
    float activity_advance_overflow;
} EntitySlot;

/* Waypoint consumers use this accessor so routes grown past the inline
 * bootstrap span remain contiguous to every reader. */
#define ENTITY_WAYPOINTS(entity) \
    (((entity)->wp_dynamic != NULL) ? (entity)->wp_dynamic : (entity)->wp)

/* Completed-objective `.osf` table (finding 08): 20-byte entries — id, ref,
 * then a 12-byte name slot (OsfAdd zeroes it, then strncpy's up to 0xb
 * characters; the file layout is 5×u32, not the "name[11]+u16 pad" the doc
 * table suggests). */
#define GAME_OSF_MAX 64
typedef struct GameOsfEntry {
    int32_t  id;
    int32_t  ref;
    char     name[12];
} GameOsfEntry;
static_assert(sizeof(GameOsfEntry) == 20, "osf entry size");

typedef struct GameContext {
    /* --- corpus globals ------------------------------------------------ */
    BlockFile*  cfg;          /* DAT_005560c4 — Lego.cfg */
    LevelState* level;        /* DAT_005560d4 — current level block */
    uint32_t    cheat;        /* DAT_00556ed0 — level cheat/feature bits */

    /* Legacy target/work-path fallback, in seconds/cell. Kept for that
     * adapter's existing behavior; ordinary kind-0 walking never uses it.
     * Loaded Stats are per-entity: RouteSpeed +0x20 is raw world units per
     * reference tick, while drill-time +0x24..34 is seconds multiplied by 25. */
    float stats_speed_sec[AI_STATS_TERRAIN_TYPES];

    /* --- permitted-activity list capacity (FUN_0046a060) ----------------- *
     * Corpus: the activity setter (FUN_0044b370) sizes and recycles each
     * entity's permitted-activity list from `*(state + 0xcc)` — the dword
     * the capacity probe FUN_0046a060 reads out of the entity's `+0x334`
     * stats block. The instance-stats fill is the documented stats-block
     * TODO (config-driven, values unrecorded), so the port models the
     * capacity game-wide here; game_context_init defaults it to
     * AI_ACTIVITY_SLOTS — the corpus's inline span — so list installs
     * behave like cap == span entities. A zero capacity makes grants a
     * no-op exactly as the corpus's cap-0 entities behave. */
    int stats_activity_cap;

    /* --- save system (DAT_005574d0 + per-slot ctx) ---------------------- */
    int   save_slot;              /* DAT_005574d0 — -1 = none (defaults 5) */
    char  saves_dir[260];         /* "Saves" directory path */
    GameOptions options;          /* DAT_00557030/34/3c — functional Options/
                                   * Advanced values (speed/sfx/music/bright/
                                   * help; issue #85), seeded by the active
                                   * save slot, persisted on commit          */
    int   saved_music_volume;     /* active slot +0x04, applied at level load */
    int   saved_sfx_volume;       /* active slot +0x08 */
    int   saved_scroll_index;     /* active slot +0x0c */
    int   saved_brightness;       /* active slot +0x10 */
    int   current_record;         /* checkpoint_step: level -> record index */
    int   active_slot;            /* slot of `records` (ctx +0x18 side) */
    int   record_count;           /* ctx +0x18 */
    uint8_t* records;             /* ctx +0x34 (record array) */
    uint8_t* checkpoint_blob;     /* DAT_00552994 (12 688 B) */

    /* --- completed-objectives table (DAT_004ffe50..) -------------------- */
    GameOsfEntry* osf;          /* DAT_004ffe50 — growable live table */
    int   osf_count;              /* DAT_004ffe54 */
    int   osf_capacity;           /* DAT_004ffe58 */
    int   osf_dirty;              /* DAT_004ffe64 */
    GameOsfEntry* osf_shadow;   /* DAT_004ffe5c — snapshot allocation */
    int   osf_shadow_count;       /* DAT_004ffe60 */
    int   osf_shadow_dirty;       /* DAT_004ffe64 (snapshot) */

    /* --- mission task table (DAT_00552990) + reward snapshot ------------ */
    GameTask tasks[GAME_TASK_COUNT]; /* stride 0x210, base self-ref at +0x1CE8 */
    GameReward reward;
    GameMission mission;         /* live progress counters                 */
    RewardScreen reward_screen;  /* Reward display keys (finding 09)        */
    int   crystal_quota;         /* DAT_005007ec (Quota.Crystals, <= 26)   */
    int   reward_active;         /* reward overlay shown (reconstruction)  */

    /* --- entity table (finding 10; object list placements) -------------- */
    EntitySlot   entity[ENTITY_TYPES][ENTITY_CAP]; /* typed entity slots  */
    int          entity_count[ENTITY_TYPES];
    int          entity_placed;  /* objects placed from the .ol this level */

    /* --- .ol TVCamera placement (FUN_0042e020 type -1 branch) ----------- */
    int   ol_camera_set;      /* a TVCamera record was placed (last wins) */
    float ol_camera_x, ol_camera_y, ol_camera_heading;

    /* --- selection manager (finding 06) ----------------------------------- */
    EntitySlot*  selection[SELECTION_MAX]; /* DAT_004fef2c array + count   */
    int          selection_count;           /* DAT_004ff298 */

    /* --- command-driven hooks -------------------------------------------- */
    /* Held-key probe for the executor's cheat-keybind pre-pass
     * (SelectionRoot step 1); set by the frame loop to read the input
     * tables. NULL disables the pre-pass. */
    int (*input_keyheld)(int key);
} GameContext;

/* Reset a GameContext to a known-fresh state (zeroed; cfg untouched). */
void game_context_init(GameContext* ctx);

/* Root identity string for the reconstruction log. */
const char* game_identity(void);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_HPP */
