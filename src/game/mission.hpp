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
 * Image/font/sound *handles* are the render lane — the port keeps the
 * config strings and coords in the task payload exactly where the corpus
 * writes them, with the flag bits that gate the reward display.
 *
 * `mission_update` is the live-progress bridge: it feeds the live counters
 * into the level block's objective-read slots (+0x8c crystals, +0xa0 ore),
 * advances task values, and marks tasks complete when their quota is met.
 * The future entity/AI layer will drive those counters directly.
 */
#ifndef LEGORRR_GAME_MISSION_HPP
#define LEGORRR_GAME_MISSION_HPP

#include "game.hpp"

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
#define TASK_OFF_IMG_XY     0x15C /* 2 ints (FUN_0045f3e0 -> +0x57/0x58)   */
#define TASK_OFF_BOXIMG_XY  0x164 /* 2 ints (FUN_0045f290 -> +0x59/0x5a)   */
#define TASK_OFF_FLIC_RECT  0x1EC /* 4 ints (FUN_0045f5f0 -> +0x7b..0x7e)  */
#define TASK_OFF_FONT       0x204 /* font config token (FUN_0045f230)      */
#define TASK_OFF_SOUND      0x208 /* sound config token (FUN_0045f1f0)     */

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

/* Seed the ten named task slots + run the six field loaders + refreshers
 * (TaskBlockInit). Corpus lifetime is per game *session* (run-start callback
 * FUN_0041fb50 @ 0x422651, freed by TaskBlockFinish in the run-finish
 * callback — finding 08 §Ownership); the port re-seeds per level load as a
 * layout-faithful stand-in for the session block. */
void mission_task_init(GameContext* ctx, BlockFile* cfg, const char* level_name);

/* Re-run Importance/Quota refreshers + crystal quota over every slot
 * (TaskImportanceRefresh/TaskQuotaRefresh/CrystalQuotaSetup). */
void mission_task_refresh(GameContext* ctx);

/* Reward display config + Save/Advance button zones (RewardInit). */
void mission_reward_init(GameContext* ctx, BlockFile* cfg, const char* level_name);

/* Per-frame live-progress bridge (finding 09/22): write the live counters
 * into the level objective slots, advance task values, mark completions.
 * Returns a bitmask of tasks that completed this frame. */
int mission_update(GameContext* ctx);

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

/* Reward-screen button handler (FUN_00462780): on a saved/advanced zone
 * hit, commit the checkpoint (CheckpointStep). Returns the step result. */
int mission_reward_click(GameContext* ctx, int hit_save);

/* Advance to the parsed `nextlevel` (finding 07): checkpoints the current
 * progress, tears the level down, and loads + re-seeds the next one. */
const char* mission_reward_advance(GameContext* ctx);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_MISSION_HPP */
