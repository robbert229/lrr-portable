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
 * The corpus couples the checker/announcer to the HUD/message/transition
 * lanes; those calls are stubbed here (each marked `(stub)`), while the word
 * bit-flow — the part other game systems and the save blob depend on — is
 * ported faithfully.
 */
#ifndef LEGORRR_GAME_OBJECTIVE_HPP
#define LEGORRR_GAME_OBJECTIVE_HPP

#include "level.hpp"

#ifdef __cplusplus
extern "C" {
#endif/* Objective-slot offsets inside the level block (finding 22). */
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
 * Sets the runtime word bits + result index in the block, and fills the
 * advisor stream text (DAT_004ffe38 analog: "Stream Objective_…"). */
void objective_announce(LevelState* s, int who);

/* The advisor stream text built by the announcer ("" when none). */
const char* objective_stream(const LevelState* s);

/* Advisor decision (finding 22): should the hint display for the current
 * outcome? 0 when the outcome's suppression bit (0x10 generic / 0x20
 * achieved / 0x80 failed) is set or no stream was produced. */
int objective_advisor_shown(const LevelState* s);

/* Per-frame objective state machine (corpus FUN_00458be0). Consumes the
 * pending-result word and drives checker + announcer when idle. `dt` is the
 * frame delta. Returns 0 when a goal resolves this frame, else 1. */
int objective_tick(LevelState* s, float dt);

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
 * region (base word + row * 0x24): {mode +0x00, pair dwords +0x04/+0x08,
 * read-out pair +0x0c/+0x10, flag word +0x14}. The port models the
 * region as a bounded module table (the widened-window convention); the
 * corpus's writers outside these two rows leave it zero.
 * AdvisorActionPositionAndModeSet (FUN_00401840): stores the mode and
 * the pair dwords, raising flag word bit 0x10000 when mode is 0xc and
 * clearing it otherwise. Bounds-guarded (deviation: blind stride). */
void advisor_action_position_and_mode_set(int row, int mode,
                                          uint32_t dword_a,
                                          uint32_t dword_b); /* FUN_00401840 */

/* AdvisorActionPositionGet (FUN_00401890): reads the record's +0x0c and
 * +0x10 dwords (the corpus's separate read-out window — this band's set
 * never writes them, a faithful quirk). NULL outputs skipped. */
void advisor_action_position_get(int row, uint32_t* dword_c,
                                 uint32_t* dword_d);          /* FUN_00401890 */

/* Observer hook for the flag word (test pin; not corpus API). */
uint32_t advisor_action_flags_peek(int row);

/* ObjectiveAdvisorTokenLookup (FUN_00453200): reads the advisor token
 * table (the corpus's unowned count word and table pointer); a
 * token index below the count answers the table entry, else 0. The
 * corpus's table writer stays unported, so the modeled table reads
 * empty (documented deviation). */
int objective_advisor_token_lookup(uint32_t index);

/* Test hook installing the token table (corpus writer unported). */
void objective_advisor_tokens_install(const uint32_t* table, uint32_t count);

#endif /* LEGORRR_GAME_OBJECTIVE_HPP */
