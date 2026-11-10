// SPDX-License-Identifier: LGPL-3.0-or-later
/* Game layer — level objectives / advisor (finding 22).
 *
 * Reconstruction of FUN_00457d40 (per-level objective config loader),
 * FUN_00458580/5c0/5f0/620 (target setters), FUN_00459050 (objective
 * checker), FUN_00458670 (objective-result announcer) and the ObjectiveTick
 * state machine FUN_00458be0 (corpus, doc 22 §2).
 *
 * The objective *word* (LVL_OBJECTIVE_*) arms which goals the level wants
 * enforced and records their outcome:
 *
 *   config side (objective_load):
 *     0x10/0x20/0x80  advisor-hint suppression keys
 *     0x100 crystals   target level+0xf4
 *     0x200 ore       target level+0xf8
 *     0x400 block     targets level+0xfc/+0x100
 *     0x800 timer     target level+0x104 (seconds*25)
 *     0x1000 construction targets level+0x108/+0x10c
 *   runtime side:
 *     0x1 pending result, 0x2 achieved, 0x4 failed, 0x104 failed-with-
 *     crystals, 0x8 result announced (shown once, then cleared).
 *
 * The corpus couples the checker/announcer to HUD, message, checkpoint, and
 * scene-transition lanes. Objective/result state and the available
 * checkpoint-selection effects are modeled here; renderer-owned advisor
 * callbacks and several global transition inputs remain explicit gaps.
 */
#ifndef LEGORRR_GAME_OBJECTIVE_HPP
#define LEGORRR_GAME_OBJECTIVE_HPP

#include "level.hpp"
#include "gods98/block.hpp"

struct EntitySlot;
struct TextureSurface;

#ifdef __cplusplus
extern "C" {
#endif/* Objective-slot offsets inside the level block (finding 22). */
typedef struct RewardTaskResourceRecord {
    uint32_t flags;
    void* image_surface;
    void* font_record;
    void* box_image_surface;
} RewardTaskResourceRecord;
void reward_task_resource_slot_set(unsigned int slot, uint32_t flags,
                                   void* image_surface, void* font_record);
void reward_task_font_record_set(unsigned int slot, void* font_record);
void* reward_task_font_record_get(unsigned int slot);
void* reward_task_image_surface_get(unsigned int slot);
void reward_task_box_image_surface_set(unsigned int slot, void* surface);
void* reward_task_box_image_surface_get(unsigned int slot);
void reward_task_flush_images(RewardTaskResourceRecord* task);
void reward_task_flush_font(RewardTaskResourceRecord* task);
void reward_task_block_finish(void);
typedef void (*ObjectiveDeferredCallback)(void);
void objective_deferred_callback_set(ObjectiveDeferredCallback callback);
void objective_deferred_callback_invoke_if_set(void);
typedef struct AdvisorTimerState {
    float timers[4];
    float countdown;
    int stored_toggle;
    int hold_countdown;
    int update_pending;
    int finalize_pending;
    int advisor_active;
    int advisor_scroll;
    float final_timer;
} AdvisorTimerState;
typedef void (*AdvisorTimerActionFn)(int value, int mode, void* user);
typedef void (*AdvisorTimerFinalizeFn)(void* user);
void advisor_timer_hooks_set(AdvisorTimerActionFn action,
                             AdvisorTimerFinalizeFn finalize, void* user);
void advisor_timer_array_advance_and_advisor_clear(AdvisorTimerState* state,
                                                   float delta);

#define OBJ_SLOT_HANDLE       0xBC  /* 0x54 B of image handles/dims + AVI */
#define OBJ_OFF_ACH_IMG       0xC8  /* "achieved" image handle (+0xc8)      */
#define OBJ_OFF_ACH_DIM       0xCC  /* "achieved" image dims (2 floats)    */
#define OBJ_OFF_FAIL_IMG      0xD4  /* "failed" image handle               */
#define OBJ_OFF_FAIL_DIM      0xD8  /* "failed" image dims (2 floats)      */
#define OBJ_OFF_AVI           0xE0  /* achieved-AVI handle (+0xe0)          */
#define OBJ_OFF_AVI_DIM       0xE4  /* achieved-AVI dims (2 floats)        */
#define OBJ_OFF_AVI_SECOND    0xEC  /* 0 or 1: AVI vs second slot          */
#define OBJ_OFF_CRYSTAL_TARGET 0xF4 /* crystal-count target (+0xf4)        */
#define OBJ_OFF_ORE_TARGET    0xF8  /* ore-count target (+0xf8)            */
#define OBJ_OFF_BLOCK_A       0xFC  /* block objective target A (+0xfc)    */
#define OBJ_OFF_BLOCK_B       0x100 /* block objective target B (+0x100)   */
#define OBJ_OFF_TIMER_TARGET  0x104 /* timer objective, ticks left (+0x104) */
#define OBJ_OFF_CONSTRUCT_A   0x108 /* construction targets (+0x108/0x10c) */
#define OBJ_OFF_CONSTRUCT_B   0x10C
#define OBJ_OFF_CRYSTAL_LIVE  0x8C  /* live crystal count (read)           */
#define OBJ_OFF_ORE_LIVE      0xA0  /* live ore count (read)               */

/* Level block +0x9f: objective result index stored by the announcer
 * (0 = active / 1 = achieved / 2 = failed). Also the save module's
 * "completed here" marker (finding 08 reads `state+0x27c == 1`). */
#define OBJ_OFF_RESULT_INDEX 0x27C

/* Load `<level>`'s objective config into the block + objective word
 * (corpus FUN_00457d40, called from LevelLoad at 0x42ab0a). */
void objective_load(BlockFile* cfg, const char* level_name, LevelState* s);

/* Objective checker (corpus FUN_00459050). Returns 1 when every armed goal
 * is satisfied (or the timer expired), 0 while still un-met. `*mark` is set
 * to 0 for a timer success under the HitTimeFail variant else 1. `dt` is
 * the frame delta consumed by the timer objective. */
int objective_check(LevelState* s, int* mark, float dt);

/* Objective-result announcer (corpus FUN_00458670). `who`:
 *   0 pending / 1 achieved / 2 failed / 3 failed-with-crystals.
 * The mapped entry uses the active level, like the reference current-level
 * getter. Local port callers can use the explicit-level adapter below. */
void objective_announce_current(int who); /* FUN_00458670, one argument */
void objective_announce(LevelState* s, int who);
void objective_active_level_bind(LevelState* s);
void objective_active_level_clear(LevelState* s);
/* Presentation ownership, distinct from an achieved/failed result. The UI
 * acknowledges only after the user's final authored briefing page. This
 * performs the pending branch of 0x4588e0 and publishes FFE34 to the NERP
 * consume-on-read switch, as in 0x4589c0. Returns 1 on acknowledgment. */
/* Public kinds represent the page sets selected by 0x4589c0. ObjectiveTick
 * gates terminal entry; final-page acknowledgment checks that gate again. */
typedef enum ObjectivePresentationKind {
    OBJECTIVE_PRESENTATION_NONE = 0,
    OBJECTIVE_PRESENTATION_BRIEFING = 1,
    OBJECTIVE_PRESENTATION_ACHIEVED = 2,
    OBJECTIVE_PRESENTATION_FAILED = 3,
    OBJECTIVE_PRESENTATION_CRYSTAL_FAILED = 4
} ObjectivePresentationKind;
int objective_presentation_kind(const LevelState* s);
/* Returns 1 when a presentation was accepted. The caller retains its kind
 * to distinguish briefing resume from the subsequent mission transition. */
int objective_presentation_acknowledge(LevelState* s);
int objective_briefing_active(const LevelState* s);
int objective_briefing_acknowledge(LevelState* s);
const char* objective_voice_alias_get(void);
int objective_voice_alias_pending_take(void);

/* Port-side string used by the objective/help overlay ("" when none). */
const char* objective_stream(const LevelState* s);

/* Advisor decision (finding 22): should the hint display for the current
 * outcome? 0 when the outcome's suppression bit (0x10 generic / 0x20
 * achieved / 0x80 failed) is set or no stream was produced. */
int objective_advisor_shown(const LevelState* s);

/* Target-shaped ObjectiveTick (FUN_00458be0): the opaque first argument is
 * DAT_00556140, `s` is DAT_005560d4, checker_dt is passed to ObjectiveCheck,
 * and frame_dt advances result-stream timing. The renderer/UI lanes whose
 * owner is the engine are represented only where this module has a real API. */
void objective_tick_target(void* engine, LevelState* s,
                           float checker_dt, float frame_dt);

/* Explicit-level adapter used by the portable frame loop. Returns 0 only
 * for an achieved/failed result whose 0x40b entity gate has passed. Pending
 * briefing presentation and unresolved gameplay return 1. The mapped target
 * above retains its void ABI. */
int objective_tick(LevelState* s, float dt);

/* ObjectiveSidecarAppendGate (FUN_0043bc40), applied to the live entity
 * record. Returns 1 when a represented state transition was applied. */
int objective_sidecar_append_gate(struct EntitySlot* entity);

#ifdef __cplusplus
}
#endif

/* ObjectiveVoiceAliasRaiseCommit (FUN_00454b70): the corpus's commit-half
 * raise calls the announcer with 1 and answers 1 — deviation: the
 * port's announcer needs the threaded level block the corpus reads
 * blindly. */
int objective_voice_alias_raise_commit(LevelState* s);

/* ObjectiveVoiceAliasRaiseGateBit27 (FUN_00454b80): when the engine-state
 * word's bit 27 is clear the corpus raises the failed-result announcer and
 * answers 1. Bit 27's corpus writer is the CDAudio/voice state feeder —
 * unported — so the gate reads through the port's modeled state word and
 * stays clear (the announcer therefore always fires, the corpus's unset
 * case, deviation documented). */
int objective_voice_alias_raise_gate_bit27(LevelState* s);

/* --- the advisor action window band (2026-10-25) ---------------------------
 * The corpus keeps the advisor's per-action records in an unowned word
 * region (base word + row * 0x24). The port models the region as a bounded
 * module table (the widened-window convention).
 * The scene request model exposes render/audio effects through an action
 * callback and scene readiness through a small owner-side state bit. */
enum {
    ADVISOR_SCENE_ACTION_RESET = 0,
    ADVISOR_SCENE_ACTION_HIDE = 1,
    ADVISOR_SCENE_ACTION_SOUND = 2,
    ADVISOR_SCENE_ACTION_VISUAL_INIT = 3,
    ADVISOR_SCENE_ACTION_CONTROLLER_INIT = 4,
    ADVISOR_SCENE_ACTION_NOTIFY = 5,
    ADVISOR_SCENE_ACTION_ATTACH = 6
};
typedef void (*AdvisorSceneActionHook)(int action, int row,
                                       uint32_t animation, void* user);
/* Creates the type-3 companion DDiScene, applies its recovered transform to
 * the base scene, enables it, and returns its handle. */
typedef uint32_t (*AdvisorCompanionSceneCreateHook)(uint32_t base_scene,
                                                    void* user);
typedef struct AdvisorAnimationHooks {
    void* user;
    float (*update)(uint32_t handle, float delta, void* user);
    float (*time_get)(uint32_t handle, void* user);
    void (*seek)(uint32_t handle, float time, void* user);
} AdvisorAnimationHooks;
typedef void (*AdvisorTransformApplyHook)(int row, uint32_t animation,
                                          float x, float y, float scale,
                                          void* user);
/* Stores DAT_004a6768, a configured sound-bank slot (not an alias index). */
void advisor_sound_alias_set(int slot);
void advisor_sound_alias_play_global(void); /* FUN_00456810 */
typedef int (*CoordinateListVisitCallback)(void* record, int x, int y,
                                           void* user);
typedef void* (*CoordinateListHeadGetHook)(void* list_key, void* context);
void coordinate_list_visit_hooks_set(CoordinateListHeadGetHook head_get,
                                     void* context);
void coordinate_list_callback_visit(void* list_key,
                                   CoordinateListVisitCallback callback,
                                   void* user); /* FUN_00456cb0 */
typedef struct AdvisorPositionConfigEntry {
    /* Parsed position rows retained for callers that already decoded config. */
    const char* position_name;
    const char* animation_name;
    int position_type;
    uint32_t position_mode;
    int action_mode;
    float x;
    float y;
} AdvisorPositionConfigEntry;
/* Renderer-owned animation lookup supplies the scene handle and configured
 * start/end times; the loader converts times to 25 Hz frame counts. */
typedef int (*AdvisorAnimationConfigLookupHook)(const char* name,
                                                uint32_t* handle,
                                                float* start_time,
                                                float* end_time,
                                                void* user);
/* Target loader adapter for the config lookup and renderer resource path.
 * The target calls it first with the animation name and, when that fails and
 * feature bit 0x400000 is clear, again with the `!<name>` fallback key. The
 * hook owns the Advisor config lookup and LWS creation/initialization. Return
 * true when the config value exists, even if `handle` is zero. */
typedef int (*AdvisorAnimationTargetConfigLookupHook)(
    void* scene_context, void* config_context, const char* animation_name,
    uint32_t* handle, float* start_time, float* end_time, void* user);
void advisor_scene_ready_set(int ready);
void advisor_scene_action_hook_set(AdvisorSceneActionHook hook, void* user);
void advisor_companion_scene_create_hook_set(
    AdvisorCompanionSceneCreateHook hook, void* user);
void advisor_scene_controller_initialize(void* config_context,
                                         const uint32_t* scene_context_words,
                                         uint32_t transform_context_handle);
void advisor_transition_worker(float delta); /* FUN_00401990 */
void advisor_scene_visual_state_initialize(void); /* FUN_00401210 */
float advisor_scene_scale_get(void);
void advisor_transform_apply_hook_set(AdvisorTransformApplyHook hook,
                                      void* user);
void advisor_apply_slot_transform(int row); /* FUN_00401b30 */
void advisor_position_config_load(BlockFile* config,
                                  int resolution_width,
                                  int resolution_height); /* FUN_00401530 */
int advisor_position_config_entries_load(
    const AdvisorPositionConfigEntry* entries, uint32_t count);
int advisor_config_slots_load(AdvisorAnimationConfigLookupHook lookup,
                              void* user); /* portable adapter */
void advisor_animation_target_config_lookup_set(
    AdvisorAnimationTargetConfigLookupHook lookup, void* user);
void advisor_config_slots_load_target(void* scene_context,
                                      void* config_context); /* 0x00401270 */
int advisor_request_admit(int row, int immediate); /* FUN_004018c0 */
void advisor_request_reset_and_submit_sound(int row); /* FUN_00401930 */
void advisor_animation_end_cleanup(void); /* FUN_004011c0 */
void advisor_animation_hooks_set(const AdvisorAnimationHooks* hooks);
void advisor_animation_slot_install(int slot, uint32_t handle,
                                    float start_time, float end_time);
int advisor_scene_advance_completion_check(int animation_slot,
                                           float delta); /* FUN_00401a70 */

/* AdvisorPositionRecordInitialize (FUN_004017b0) initializes the action
 * record and flags. */
void advisor_position_record_initialize(int row, uint32_t animation_index,
                                        int position_type,
                                        uint32_t position_mode,
                                        int action_mode, uint32_t x,
                                        uint32_t y);
/* AdvisorActionPositionAndModeSet (FUN_00401840): stores the mode and
 * position pair, raising flag word bit 0x10000 when mode is 0xc and
 * clearing it otherwise. */
void advisor_action_position_and_mode_set(int row, int mode,
                                          uint32_t dword_a,
                                          uint32_t dword_b); /* FUN_00401840 */

/* AdvisorActionPositionGet (FUN_00401890): reads the record's +0x18 and
 * +0x1c dwords. The setter writes its second coordinate to +0x18; +0x1c
 * remains the initialized current-position value. NULL outputs are skipped. */
void advisor_action_position_get(int row, uint32_t* dword_c,
                                 uint32_t* dword_d);          /* FUN_00401890 */

/* AdvisorAnimationPositionAndRequestUpdate (FUN_0041c4f0). The controller
 * uses live frontend mode/interaction state, while the target's indicator
 * row, trigger tuple, point table and vertical origin have no source owner.
 * Keep those unowned inputs explicit and bounded instead of inventing them. */
#define ADVISOR_POSITION_UPDATE_MAX_POINTS 32
typedef struct AdvisorPositionUpdatePoint {
    float x;
    float y;
} AdvisorPositionUpdatePoint;
typedef struct AdvisorAnimationPositionUpdateState {
    int action_row;
    int trigger_selector;
    int cached_input_a;
    int cached_input_b;
    float vertical_origin;
    uint32_t point_count;
    AdvisorPositionUpdatePoint points[ADVISOR_POSITION_UPDATE_MAX_POINTS];
} AdvisorAnimationPositionUpdateState;
typedef int (*AdvisorPositionUpdateValueResolveFn)(
    int selector, int input_a, int input_b, int* value, void* user);
void advisor_animation_position_update_state_set(
    const AdvisorAnimationPositionUpdateState* state);
void advisor_animation_position_update_value_resolver_set(
    AdvisorPositionUpdateValueResolveFn resolver, void* user);
/* FlashGeneric advisor producer (FUN_0041e890). */
int advisor_action_indicator_request(int action, int request_mode);
/* Typed 0x41c4f0 path: image is the freshly resolved current image. */
void advisor_animation_position_and_request_update_image(
    int position_mode, int input_a, int input_b, const float* position,
    const struct TextureSurface* image);
void advisor_animation_position_and_request_update(
    int position_mode, int input_a, int input_b, const float* position);

/* Observer hook for the flag word (test pin; not corpus API). */
uint32_t advisor_action_flags_peek(int row);

/* ObjectiveAdvisorTokenLookup (FUN_00453200): reads the advisor token
 * table (the corpus's unowned count word and table pointer); a
 * token index below the count answers the table entry, else 0. The
 * corpus's table writer stays unported, so the modeled table reads
 * empty (documented deviation). */
int objective_advisor_token_lookup(uint32_t index);

/* ObjectiveAdvisorTokenLoad (FUN_00452e70) owns a mutable script copy and
 * builds the ordinary text table plus separate image/sound name tables.
 * The resolver receives kind 1 for images and 2 for sounds; a returned zero
 * omits that resource token, matching the reference's failed-load gate. */
typedef uintptr_t (*ObjectiveAdvisorTokenResolveFn)(int kind,
                                                     const char* value,
                                                     void* user);
typedef void (*ObjectiveAdvisorTokenReleaseFn)(int kind, uintptr_t handle,
                                               void* user);
int objective_advisor_tokens_parse(const char* source, uint32_t source_length,
                                   ObjectiveAdvisorTokenResolveFn resolve,
                                   ObjectiveAdvisorTokenReleaseFn release,
                                   void* user);
/* Objective/advisor stream loader: RESOURCE_AUTO path lookup plus the
 * native bitmap and sound-bank loaders used by the retail token tables. */
int objective_advisor_token_file_load(const char* path);
void objective_advisor_tokens_release(void);
const char* objective_advisor_token_text_get(uint32_t index);
uint32_t objective_advisor_image_token_count(void);
const char* objective_advisor_image_token_name_get(uint32_t index);
uintptr_t objective_advisor_image_token_handle_get(uint32_t index);
void* objective_advisor_image_token_resource_get(uint32_t index);
uint32_t objective_advisor_sound_token_count(void);
const char* objective_advisor_sound_token_name_get(uint32_t index);
uintptr_t objective_advisor_sound_token_handle_get(uint32_t index);
uint32_t objective_advisor_token_generation_get(void);

/* Optional raw-table installer for callers that already own a legacy table. */
void objective_advisor_tokens_install(const uint32_t* table, uint32_t count);

/* Advisor name tables (FUN_004013e0/FUN_004014c0). */
int advisor_position_name_to_index(const char* name, int* index);
int advisor_animation_name_to_index(const char* name, int* index);

#endif /* LEGORRR_GAME_OBJECTIVE_HPP */
