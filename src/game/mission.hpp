// SPDX-License-Identifier: LGPL-3.0-or-later
/* Game layer — mission task table + reward config (finding 09).
 *
 * Reconstructs TaskBlockInit @ 0x45f030, the six task field loaders
 * (0x45f1f0 / 0x45f230 / 0x45f290 / 0x45f3e0 / 0x45f530 / 0x45f5f0),
 * TaskImportanceRefresh @ 0x4600a0 / TaskQuotaRefresh @ 0x460140,
 * CrystalQuotaSetup @ 0x45c040 and the RewardInit display keys
 * (@ 0x45f7f0, corpus engine-region.c).
 *
 * The ten named task slots (Crystals, Ore, Diggable, Constructions, Caverns,
 * Figures, RockMonsters, Oxygen, Timer, Score — stride 0x210) live in
 * GameContext.tasks; CheckpointBlobBuild copies them into the save blob
 * verbatim, with the Importance/Quota refreshers re-run over them every
 * capture (finding 09 §"Per-level reward config").
 *
 * Image/font/sound handles are the render lane. Image and font handles are
 * mirrored in a host-width resource sidecar for safe release; their low
 * 32-bit words occupy the corpus task payload slots, with the flag bits that
 * gate the reward display. The sound token remains a config string because
 * this path has no portable sound-resource handle owner yet.
 *
 * `mission_update` is the live-progress bridge: it feeds the live counters
 * into the level block's objective-read slots (+0x8c crystals, +0xa0 ore),
 * advances task values, and marks tasks complete when their quota is met.
 * The future entity/AI layer will drive those counters directly.
 */
#ifndef LEGORRR_GAME_MISSION_HPP
#define LEGORRR_GAME_MISSION_HPP

#include "game.hpp"
#include "pointer_cursor.hpp"
#include "port_loop_ports.hpp"
#include "../gods98/flic.hpp"

#ifdef __cplusplus
extern "C" {
#endif

/* Task payload offsets (stride 0x210; corpus FUN_0045fxx offsets recorded
 * per finding 09 §"Task record" and the loader pseudocode). */
#define TASK_OFF_NAME       0x04  /* char name[0x20] (TaskBlockInit)       */
#define TASK_OFF_IMPORTANCE 0x24  /* float (TaskImportanceRefresh)         */
#define TASK_OFF_QUOTA      0x28  /* int32 (TaskQuotaRefresh)              */
#define TASK_OFF_VALUE      0x2C  /* live progress (reconstruction-side;   */
                                  /* corpus tracks it in the level block   */
                                  /* objective slots, task +0x30 is a      */
                                  /* derived float re-computed from quota) */
#define TASK_OFF_TEXT       0x54  /* display text (FUN_0045f530 reads      */
                                  /* `Reward Text.<name>` field 0, with    */
                                  /* '_' mapped to ' ' over the first      */
                                  /* 0x20 bytes)                           */
#define TASK_OFF_TEXT_XY    0x154 /* 2 int text-draw x/y (FUN_0045f530     */
                                  /* fields 1/2 -> record dwords 0x55/0x56)*/
#define TASK_OFF_FLICPATH2  0x16C /* flic path (FUN_0045f5f0 -> +0x5b)     */
#define TASK_OFF_FLIC_HANDLE 0x1FC /* opened FLIC player (FUN_00461790)     */
#define TASK_OFF_IMG_XY     0x15C /* 2 ints (FUN_0045f3e0 -> +0x57/0x58)   */
#define TASK_OFF_IMAGE      0x200 /* loaded image surface (FUN_0045f3e0)    */
#define TASK_OFF_BOXIMG_XY  0x164 /* 2 ints (FUN_0045f290 -> +0x59/0x5a)   */
#define TASK_OFF_FLIC_RECT  0x1EC /* 4 ints (FUN_0045f5f0 -> +0x7b..0x7e)  */
#define TASK_OFF_FONT       0x204 /* font-atlas handle (FUN_0045f230)      */
#define TASK_OFF_SOUND      0x208 /* sound config token (FUN_0045f1f0)     */
#define TASK_OFF_BOX_IMAGE  0x20C /* loaded BoxImages surface handle        */

/* Task flag bits (dword +0x00). The display tick tests `flags & bit`
 * paired with the blob-side display gates (finding 09 §Display). */
#define TASK_FLAG_COMPLETED  1u    /* drawn as achieved + text overlay on   */
#define TASK_FLAG_IMG        2u    /* Images entry loaded (FUN_0045f3e0)    */
#define TASK_FLAG_FLIC       4u    /* Flics entry loaded (FUN_0045f5f0)     */
#define TASK_FLAG_TEXT       0x10u /* text overlay drawn over the record    */
#define TASK_FLAG_BOXTEXT    0x20u /* boxed-text variant of the overlay     */
#define TASK_FLAG_BOXIMG     0x40u /* BoxImages entry loaded (FUN_0045f290) */
#define TASK_FLAG_MASK       0x31u /* FUN_0045f530 ORs 1|0x10|0x20 when a   */
                                   /* `Reward Text.<name>` entry exists     */

/* Target-shaped, no-argument TaskBlockInit entry. The current runtime owner
 * is supplied by the explicit adapter below. */
int mission_task_init_target(void);
/* Seed ten named task slots and run the six corpus-order field-loader passes
 * for the supplied live GameContext. */
void mission_task_init(GameContext* ctx, BlockFile* cfg, const char* level_name);

/* Re-run Importance/Quota refreshers + crystal quota over every slot
 * (TaskImportanceRefresh/TaskQuotaRefresh/CrystalQuotaSetup). */
void mission_task_refresh(GameContext* ctx);

/* Reward display config + Save/Advance button zones (RewardInit). */
int mission_reward_init(GameContext* ctx, BlockFile* cfg, const char* level_name);
void mission_reward_resources_release(GameContext* ctx);
void mission_reward_buttons_draw(GameContext* ctx, void* pixels, int pitch,
                                 int width, int height, int mouse_x,
                                 int mouse_y, int mouse_down);

/* Per-frame live-progress bridge (finding 09/22): write the live counters
 * into the level objective slots, advance task values, mark completions.
 * Returns a bitmask of tasks that completed this frame. */
int mission_update(GameContext* ctx);

/* Per-level checkpoint/reward snapshot refresh (corpus 0x460360): derive
 * map-backed task totals, clamp task progress, recalculate aggregate
 * percentages, and refresh the reward statistics display. The caller must
 * have built the temporary checkpoint blob first. */
int mission_checkpoint_task_progress_refresh(GameContext* ctx);
/* No-argument adapter for the original global-context call site at
 * 0x435940; it resolves the currently bound mission GameContext. */
int mission_checkpoint_task_progress_refresh_bound(void);
/* Capture the eight renderer-owned success words when ObjectiveVoiceAliasPrepare
 * enters its achieved branch. The active mission/config supplies dimensions. */
int mission_reward_success_snapshot_prepare(LevelState* level);

/* The ten task names in slot order (s_Crystals … s_Score). */
const char* mission_task_name_at(GameContext* ctx, int slot);

/* Set/read live progress counters (the values the entity layer will own). */
void mission_set_crystals(GameContext* ctx, int32_t v);
void mission_set_ore(GameContext* ctx, int32_t v);
void mission_add_score(GameContext* ctx, int32_t v);

/* Dispatch a message-bus incident id (finding 22 §1): the game layer's path
 * into the engine's 40-id Info bus (sound/cooldown handled by the bus). */
void mission_incident(GameContext* ctx, int id);

/* --- reward screen + save zone (finding 09 display half) ---------------- */
typedef struct RewardScreenState {
    GameContext* game;
    int displayed_task_count;  /* FUN_00461410 local_34 */
    int transition_stage;       /* local_30: intro, task, completion, exit */
    int score_text_gate;        /* local_28 */
    int transition_state;       /* local_1c */
    float elapsed;              /* local_2c */
    float task_duration;        /* DAT_0055298c */
    uint32_t last_clock_ms;
    int pointer_override_active;
    uint32_t pointer_x;
    uint32_t pointer_y;
    Flic* task_flics[GAME_TASK_COUNT];
    PointerCursorState* pointer_cursor;
    const PointerCursorHooks* pointer_cursor_hooks;
    RewardScrollTickerState* scroll_ticker;
} RewardScreenState;

/* The reward loop owns its state transitions, task timing, FLIC lifetime,
 * and callback order. These operations bridge state owned by the engine or
 * renderer that is not represented by GameContext. */
typedef struct RewardScreenOps {
    void* context;
    uint32_t (*time_get_ms)(void* context);
    /* RewardSaveButtonZoneCheck input/readiness gate. Returns nonzero when
     * the reward display may run; mission code queues the accepted sound. */
    int (*save_button_check)(void* context, GameContext* game);
    int (*loop_should_continue)(void* context,
                                const RewardScreenState* state);
    /* Hard cancellation is separate from loop_should_continue, whose retail
     * counterpart only extends the reveal loop. It still runs normal cleanup. */
    int (*loop_should_abort)(void* context);
    int (*fast_forward_pressed)(void* context);
    int (*transition_events)(void* context, int* return_to_game,
                             int* reset_reward);
    int (*display_mode_get)(void* context, GameContext* game);
    int (*score_text_gate_ready)(void* context, GameContext* game);
    int (*reward_zone_check)(void* context, RewardScreenState* state);
    void (*reward_scroll_cleanup)(void* context);
    void (*return_to_game)(void* context);
    void (*task_visual_draw)(void* context, GameTask* task, int task_index,
                             uint32_t draw_mask, int displayed_count,
                             float elapsed, Flic* flic);
    void (*task_text_draw)(void* context, GameTask* task, int task_index,
                           int displayed_count, float elapsed);
    int (*score_text_draw)(void* context, GameContext* game,
                           int displayed_count, float elapsed);
    /* Renderer receives formatted blob strings and
     * game->reward_screen.stats_result_text (config label decoded like the
     * retail escape/underscore helper). */
    void (*reward_stats_draw)(void* context, GameContext* game);
    void (*render_finish)(void* context);
    void (*input_restore)(void* context, int mode);
    void (*cursor_position_get)(void* context, uint32_t* x, uint32_t* y);
    void (*cursor_input_update)(void* context);
    void (*pointer_override_clear)(void* context);
    void (*screen_effect_update)(void* context, float delta);
    void (*pointer_effect_update)(void* context, uint32_t x, uint32_t y,
                                  float elapsed_ms);
    void (*frame_pump)(void* context, int mode);
    void (*completion_audio)(void* context, GameContext* game);
} RewardScreenOps;

void reward_screen_state_init(RewardScreenState* state, GameContext* game,
                              PointerCursorState* pointer_cursor,
                              const PointerCursorHooks* pointer_hooks);
/* Bind before entering the no-argument reference ABI adapters below. The
 * frame loop installs its operations before level setup; mission_reward_init
 * then binds the active GameContext state while retaining those operations. */
void reward_screen_runtime_bind(RewardScreenState* state,
                               const RewardScreenOps* ops);
int reward_screen_tick(void); /* FUN_00461410: active global reward screen */
int reward_screen_tick_with_ops(RewardScreenState* state,
                                const RewardScreenOps* ops);
int reward_screen_tick_bound(void);
/* Reward display cluster ABI adapters. These use the bound screen state and
 * renderer callbacks installed by reward_screen_runtime_bind. */
int reward_stats_panel_draw_bound(void);
int reward_stats_text_format_bound(void);
void reward_display_task_abi(GameTask* task, uint32_t draw_mask,
                             uint32_t displayed_count);
int reward_score_task_text_draw(int displayed_task_count, float elapsed);
void reward_flic_context_tick_abi(int* displayed_task_count,
                                  int* transition_stage, float* elapsed);
void reward_screen_transition_tick(RewardScreenState* state,
                                   const RewardScreenOps* ops);
void reward_screen_transition_tick_abi(int* displayed_task_count,
                                       int* transition_stage,
                                       int* score_text_gate, float* elapsed);
void reward_pointer_override_expiry_tick(RewardScreenState* state,
                                         float delta,
                                         const RewardScreenOps* ops);
void reward_pointer_override_expiry_tick_bound(float delta);

/* The corpus gates the overlay on the Reward/Enable blob + the button
 * surfaces; the demo uses a reconstruction-side `reward_active` flag. */
int  mission_reward_active(const GameContext* ctx);
void mission_reward_set_active(GameContext* ctx, int on);

/* RewardZoneCheck @ 0x461dd0 port: hit-test the SaveButton (when the level
 * result index is 1, "achieved") or the AdvanceButton rect against the
 * mouse position. Returns 1 when the pressed button was hit; *hit_save /
 * *hit_advance report which. */
int mission_reward_zone(GameContext* ctx, int mx, int my,
                        int* hit_save, int* hit_advance);

/* Reward button adapter: Save opens the frame-driven save picker (which may
 * be cancelled without changing the reward state); Advance commits the
 * checkpoint and follows nextlevel. Returns 1 when either route is handled. */
int mission_reward_click(GameContext* ctx, int hit_save);

/* Advance to the parsed `nextlevel` (finding 07): checkpoints the current
 * progress, tears the level down, and loads + re-seeds the next one. */
const char* mission_reward_advance(GameContext* ctx);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_MISSION_HPP */
