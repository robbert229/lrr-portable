// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gods98/block.hpp"
#include "gods98/heap.hpp"
#include "gods98/message.hpp"
#include "gods98/string.hpp"
#include "crt/crt.hpp"
#include "entity.hpp"
#include "level.hpp"
#include "mission.hpp"
#include "select.hpp"
#include "frontend.hpp"
#include "objective.hpp"
#include "savegame.hpp"
#include "ui_list.hpp"
#include "effects.hpp"
#include "../gods98/sound.hpp"
#include "../gods98/texture.hpp"
#include "../gods98/surface.hpp"
#include "util.hpp"
#include "../gods98/input.hpp"
#include "../gods98/frameloop.hpp"
#include "../platform/platform.h"
#include "mission_shared.hpp"
#include "mission_for_reward.hpp"
#include "mission_from_reward.hpp"
#include "mission_from_panel_unit.hpp"

#define REWARD_TASK_DURATION_DEFAULT 76.9375f

/* The reference entry points carry no context pointers. The game/runtime
 * owner must bind its active reward state and engine callbacks before calling
 * those ABI adapters. */
RewardScreenState* s_reward_screen_state;
const RewardScreenOps* s_reward_screen_ops;
RewardScreenState s_reward_screen_default_state;
/* RewardInit owns three atlas records alongside its config globals. Keep
 * their host-width pointers in a sidecar; the reference task records only
 * have room for 32-bit handles. */
GameContext* s_reward_font_owner;
void* s_reward_font_base_record;
void* s_reward_font_title_record;
void* s_reward_font_back_record;
MissionTaskInitBinding s_task_init_binding;

/* Path helper: level-scoped `Reward[::<sub>][::<name>]` key, resolved via
 * util_level_cfg_path (Levels container first, flat fallback). */
void rw_path(char* out, size_t out_sz, BlockFile* cfg,
                    const char* level, const char* sub, const char* name)
{
    char suffix[400]; /* "Reward" + optional "::<sub>" + optional "::<name>" */
    size_t pos = 0;
    pos += snprintf(suffix + pos, sizeof(suffix) - pos, "Reward");
    if (sub != NULL) {
        pos += snprintf(suffix + pos, sizeof(suffix) - pos, "::%s", sub);
    }
    if (name != NULL) {
        pos += snprintf(suffix + pos, sizeof(suffix) - pos, "::%s", name);
    }
    (void)pos;
    if (util_level_cfg_path(cfg, level, suffix, out, out_sz) == NULL) {
        out[0] = '\0';
    }
}

static int rw_int(BlockFile* cfg, const char* root, const char* level,
                  const char* sub, const char* name, int dflt)
{
    (void)root;
    char path[512];
    rw_path(path, sizeof(path), cfg, level, sub, name);
    return block_get_int(cfg, path, dflt);
}

static float rw_float(BlockFile* cfg, const char* root, const char* level,
                      const char* sub, const char* name, float dflt)
{
    (void)root;
    char path[512];
    rw_path(path, sizeof(path), cfg, level, sub, name);
    return block_get_float(cfg, path, dflt);
}

void task_put_int(GameTask* t, int off, int32_t v)
{
    __builtin_memcpy(t->data + off, &v, 4);
}

int task_get_int(const GameTask* t, int off)
{
    int32_t v;
    __builtin_memcpy(&v, t->data + off, 4);
    return (int)v;
}

/* StrReplace (FUN_00413f00): replace one byte throughout a NUL-terminated
 * string and return the original pointer. */
// FUNCTION: LEGORR 0x00413f00
char* mission_replace_char(char* text, char from, char to)
{
    char* const original = text;
    while (*text != '\0') {
        if (*text == from) {
            *text = to;
        }
        ++text;
    }
    return original;
}

/* ------------------------------------------------------------------ *
 *  Field loaders (corpus order in TaskBlockInit).
 * ------------------------------------------------------------------ */

/* ------------------------------------------------------------------ *
 *  Entry points (finding 09).
 * ------------------------------------------------------------------ */

const char* mission_task_name_at(GameContext* ctx, int slot)
{
    if (ctx == NULL || slot < 0 || slot >= GAME_TASK_COUNT) {
        return NULL;
    }
    return task_name(&ctx->tasks[slot]);
}

/* TaskBlockInit @ 0x0045f030 is a no-argument global entry. The live port
 * supplies its current session/config owner through the explicit adapter. */
static void mission_task_config_values_refresh(GameContext* ctx,
                                               BlockFile* cfg,
                                               const char* level_name);

/* Explicit owner adapter. Save/restore the binding so nested menu/config
 * work cannot replace the outer caller's context. */
void mission_task_init(GameContext* ctx, BlockFile* cfg, const char* level_name)
{
    const MissionTaskInitBinding previous = s_task_init_binding;
    if (ctx == NULL) return;
    if (ctx->level != NULL) objective_active_level_bind(ctx->level);
    s_task_init_binding.game = ctx;
    s_task_init_binding.config = cfg;
    s_task_init_binding.level_name = level_name;
    (void)mission_task_init_target();
    /* CheckpointBlobBuild's retail path copies these records and refreshes
     * each task's Importance and Quota before progress aggregation. Keep the
     * context-owned task records current as soon as their names/resources
     * have been initialized, so the live mission tick and its checkpoint
     * snapshot consume the same configured values. */
    mission_task_config_values_refresh(ctx, cfg, level_name);
    s_task_init_binding = previous;
}

void mission_task_refresh(GameContext* ctx)
{
    if (ctx == NULL) return;
    mission_task_config_values_refresh(ctx, ctx->cfg,
        ctx->level != NULL ? (const char*)ctx->level->ptr_name : NULL);
}

static void mission_task_config_values_refresh(GameContext* ctx,
                                               BlockFile* cfg,
                                               const char* level)
{
    MissionTaskInitBinding previous;
    int i;
    if (ctx == NULL || cfg == NULL || level == NULL) {
        return;
    }
    previous = s_task_init_binding;
    s_task_init_binding.game = ctx;
    s_task_init_binding.config = cfg;
    s_task_init_binding.level_name = level;
    for (i = 0; i < GAME_TASK_COUNT; ++i) {
        GameTask* t = &ctx->tasks[i];
        mission_task_importance_refresh(t);
        mission_task_quota_refresh(t);
        /* Defaults: absence parses as 0; the corpus reads the same way. */
    }
    {
        /* string_path_join returns its single static scratch buffer, which
         * the nested join inside the setup would overwrite while reading
         * this argument; hand it a private copy. */
        char levels_path[256];
        strncpy(levels_path, string_path_join("Levels", level, (const char*)0),
                sizeof(levels_path) - 1u);
        levels_path[sizeof(levels_path) - 1u] = '\0';
        mission_crystal_quota_setup(cfg, util_root_name(cfg), levels_path);
    }
    ctx->crystal_quota = s_crystal_quota;
    s_task_init_binding = previous;
}

void mission_task_snapshot_refresh(GameContext* ctx, GameTask* tasks)
{
    MissionTaskInitBinding previous;
    const char* level;
    int i;
    if (ctx == NULL || tasks == NULL || ctx->cfg == NULL || ctx->level == NULL)
        return;
    previous = s_task_init_binding;
    s_task_init_binding.game = ctx;
    s_task_init_binding.config = ctx->cfg;
    level = (const char*)ctx->level->ptr_name;
    if (level == NULL) {
        s_task_init_binding = previous;
        return;
    }
    s_task_init_binding.level_name = level;
    for (i = 0; i < GAME_TASK_COUNT; ++i) {
        mission_task_importance_refresh(&tasks[i]);
        mission_task_quota_refresh(&tasks[i]);
    }
    s_task_init_binding = previous;
}

void mission_set_crystals(GameContext* ctx, int32_t v)
{
    if (ctx != NULL) ctx->mission.crystals = v;
}
void mission_set_ore(GameContext* ctx, int32_t v)
{
    if (ctx != NULL) ctx->mission.ore = v;
}
void mission_add_score(GameContext* ctx, int32_t v)
{
    if (ctx != NULL) {
        ctx->mission.score += v;
        reward_score_add((float)v);
    }
}

void mission_incident(GameContext* ctx, int id)
{
    (void)ctx;
    message_dispatch(id, 0, 0, NULL);
}

/* Live bridge: write the counters into the level's objective-read slots and
 * advance the task value/score slots; return task bits that completed. */
int mission_update(GameContext* ctx)
{
    if (ctx == NULL || ctx->level == NULL) {
        return 0;
    }
    LevelState* s = ctx->level;
    objective_active_level_bind(s);
    /* Crystal (task 0) / Ore (task 1) live counts — objective readers. */
    level_set_int(s, OBJ_OFF_CRYSTAL_LIVE, ctx->mission.crystals);
    level_set_int(s, OBJ_OFF_ORE_LIVE, ctx->mission.ore);
    task_set_value(&ctx->tasks[0], ctx->mission.crystals);
    task_set_value(&ctx->tasks[1], ctx->mission.ore);
    task_set_value(&ctx->tasks[3], ctx->mission.constructions);
    task_set_value(&ctx->tasks[9], ctx->mission.score);

    int completed = 0;
    for (int i = 0; i < GAME_TASK_COUNT; ++i) {
        GameTask* t = &ctx->tasks[i];
        int quota = task_get_int(t, TASK_OFF_QUOTA);
        if (quota <= 0) {
            continue;
        }
        int value = task_value(t);
        if (value >= quota && (task_flags(t) & TASK_FLAG_COMPLETED) == 0) {
            task_set_flags(t, task_flags(t) | TASK_FLAG_COMPLETED);
            completed |= (1 << i);
        }
    }
    return completed;
}

static float mission_blob_float_get(const uint8_t* blob, size_t offset)
{
    float value;
    __builtin_memcpy(&value, blob + offset, sizeof(value));
    return value;
}

static void mission_blob_float_set(uint8_t* blob, size_t offset, float value)
{
    __builtin_memcpy(blob + offset, &value, sizeof(value));
}

static void mission_blob_word_set(uint8_t* blob, size_t offset,
                                  uint32_t value)
{
    __builtin_memcpy(blob + offset, &value, sizeof(value));
}

uint32_t s_reward_success_words[8];
int s_reward_success_words_valid;

static int mission_reward_success_words_capture(GameContext* game)
{
    char width_path[256];
    char height_path[256];
    const char* root;
    int width;
    int height;
    if (game == NULL || game->cfg == NULL) return 0;
    root = util_root_name(game->cfg);
    if (root == NULL) return 0;
    snprintf(width_path, sizeof(width_path), "%s::Menu::SaveImage::BigWidth",
             root);
    snprintf(height_path, sizeof(height_path), "%s::Menu::SaveImage::BigHeight",
             root);
    width = block_get_int(game->cfg, width_path, 0);
    height = block_get_int(game->cfg, height_path, 0);
    if (width == 0) width = 0x50;
    if (height == 0) height = 0x3c;
    if (width <= 0 || height <= 0 ||
        !surface_reward_success_capture((uint32_t)width, (uint32_t)height,
                                        s_reward_success_words)) return 0;
    s_reward_success_words_valid = 1;
    return 1;
}

int mission_reward_success_snapshot_prepare(LevelState* level)
{
    GameContext* game = NULL;
    if (level == NULL) return 0;
    if (s_reward_screen_state != NULL &&
        s_reward_screen_state->game != NULL &&
        s_reward_screen_state->game->level == level) {
        game = s_reward_screen_state->game;
    }
    else if (s_reward_screen_default_state.game != NULL &&
             s_reward_screen_default_state.game->level == level) {
        game = s_reward_screen_default_state.game;
    }
    if (game == NULL) return 0;
    if (s_reward_success_words_valid) return 1;
    return mission_reward_success_words_capture(game);
}

/* Keep explicit-context callers on the same no-argument target path as the
 * retail entry point. Save/restore the temporary owner so callbacks may
 * re-enter the calculation without leaking another GameContext. */
GameContext* s_checkpoint_refresh_context_override;

int mission_checkpoint_task_progress_refresh(GameContext* ctx)
{
    GameContext* previous = s_checkpoint_refresh_context_override;
    int result;
    s_checkpoint_refresh_context_override = ctx;
    result = mission_checkpoint_task_progress_refresh_bound();
    s_checkpoint_refresh_context_override = previous;
    return result;
}

void mission_reward_font_records_release(void)
{
    font_atlas_record_release(s_reward_font_base_record);
    font_atlas_record_release(s_reward_font_title_record);
    font_atlas_record_release(s_reward_font_back_record);
    s_reward_font_base_record = NULL;
    s_reward_font_title_record = NULL;
    s_reward_font_back_record = NULL;
    s_reward_font_owner = NULL;
}

/* RewardInit owns the display config and the two four-state button resource
 * sets; their refs remain in GameContext until reinit or context disposal. */
void mission_reward_resources_release(GameContext* ctx)
{
    int i;
    if (ctx == NULL) return;
    for (i = 0; i < 4; ++i) {
        if (ctx->reward_screen.save_button_images[i] != NULL) {
            texture_release(ctx->reward_screen.save_button_images[i]);
            ctx->reward_screen.save_button_images[i] = NULL;
        }
        if (ctx->reward_screen.advance_button_images[i] != NULL) {
            texture_release(ctx->reward_screen.advance_button_images[i]);
            ctx->reward_screen.advance_button_images[i] = NULL;
        }
    }
    if (s_reward_font_owner == ctx) {
        mission_reward_font_records_release();
    }
}

void mission_reward_buttons_draw(GameContext* ctx, void* pixels, int pitch,
                                 int width, int height, int mouse_x,
                                 int mouse_y, int mouse_down)
{
    RewardScreen* rs;
    int result;
    int save_hover;
    int advance_hover;
    int save_state;
    int advance_state;
    if (ctx == NULL || pixels == NULL || width <= 0 || height <= 0) return;
    rs = &ctx->reward_screen;
    result = ctx->level != NULL ? level_int(ctx->level, LEVEL_OFF_OBJ_RESULT)
                                : 0;
    save_hover = mouse_x > rs->save_x &&
        mouse_x < rs->save_x + rs->save_w && mouse_y > rs->save_y &&
        mouse_y < rs->save_y + rs->save_h;
    advance_hover = mouse_x > rs->advance_x &&
        mouse_x < rs->advance_x + rs->advance_w && mouse_y > rs->advance_y &&
        mouse_y < rs->advance_y + rs->advance_h;

    if (result == 1) {
        save_state = save_hover
            ? ((mouse_down && rs->save_button_images[2] != NULL) ? 2
               : (rs->save_button_images[1] != NULL ? 1 : -1))
            : 0;
    } else {
        save_state = 3; /* RewardZoneCheck draws the disabled Save image. */
    }
    advance_state = advance_hover
        ? ((mouse_down && rs->advance_button_images[2] != NULL) ? 2
           : (rs->advance_button_images[1] != NULL ? 1 : -1))
        : 0;
    if (save_state >= 0 && rs->save_button_images[save_state] != NULL) {
        surface_blit_texture(pixels, pitch, width, height,
            rs->save_button_images[save_state], (int)rs->save_x,
            (int)rs->save_y);
    }
    if (advance_state >= 0 &&
        rs->advance_button_images[advance_state] != NULL) {
        surface_blit_texture(pixels, pitch, width, height,
            rs->advance_button_images[advance_state], (int)rs->advance_x,
            (int)rs->advance_y);
    }
}

int mission_reward_active(const GameContext* ctx)
{
    return ctx != NULL ? ctx->reward_active : 0;
}

void mission_reward_set_active(GameContext* ctx, int on)
{
    if (ctx != NULL) {
        ctx->reward_active = on;
        /* KeepRunning builds and refreshes the checkpoint snapshot immediately
         * before entering the reward display. Doing this in mission_update
         * scans every terrain cell and redraws the stats panel on every live
         * simulation tick, which is not the retail call path. */
        if (on && ctx->level != NULL) {
            /* The retail path rebuilds the 0x318c-byte blob on each entry,
             * copying the current task records before computing map-backed
             * progress. Reuse would retain stale task and result fields. */
            checkpoint_blob_build(ctx);
            if (ctx->checkpoint_blob != NULL)
                (void)mission_checkpoint_task_progress_refresh(ctx);
        }
    }
}

/* Advance to the parsed `nextlevel` (finding 07): checkpoint the current
 * progress (the corpus's advance branch calls CheckpointStep too), tear the
 * level down, and load + re-seed the next one. Returns the next level name
 * (owned by GameState, i.e. ctx->level after the swap) or NULL when the
 * chain ends. */
const char* mission_reward_advance(GameContext* ctx)
{
    if (ctx == NULL || ctx->level == NULL || ctx->cfg == NULL) {
        return NULL;
    }
    const char* next = level_next_name(ctx->level);
    char name[128];
    if (ctx->checkpoint_blob == NULL) checkpoint_blob_build(ctx);
    (void)mission_checkpoint_task_progress_refresh(ctx);
    checkpoint_step(ctx);
    if (next == NULL || next[0] == '\0') {
        ctx->reward_active = 0;
        return NULL;
    }
    snprintf(name, sizeof(name), "%s", next);
    objective_active_level_clear(ctx->level);
    level_destroy(ctx->level);
    ctx->level = NULL;
    ctx->selection_count = 0;
    selection_active_context_sync(ctx);
    ctx->reward_active = 0;
    __builtin_memset(&ctx->mission, 0, sizeof(ctx->mission));
    __builtin_memset(&ctx->tasks, 0, sizeof(ctx->tasks));

    ctx->level = level_load(ctx->cfg, name);
    if (ctx->level != NULL) {
        level_no_gather_config_setup(ctx->cfg, name);
        mission_reward_init(ctx, ctx->cfg, name);
        mission_task_init(ctx, ctx->cfg, name);
        /* LevelLoad parsed the object list before returning; StartLevelSelected
         * snapshots the resulting type-4 and Pilot pools here. */
        start_level_selected(ctx);
    }
    return ctx->level != NULL ? (const char*)ctx->level->ptr_name : NULL;
}

GameTask* reward_task_get(RewardScreenState* state, int index)
{
    if (state == NULL || state->game == NULL || index < 0 ||
        index >= GAME_TASK_COUNT) return NULL;
    if (state->game->checkpoint_blob != NULL) {
        return (GameTask*)(state->game->checkpoint_blob + 0x1cecu +
                           (size_t)index * GAME_TASK_STRIDE);
    }
    return &state->game->tasks[index];
}

void reward_task_flic_reload_record(RewardScreenState* state,
                                          GameTask* task, int index)
{
    Flic* flic;
    const char* path;
    if (state == NULL || task == NULL || index < 0 ||
        index >= GAME_TASK_COUNT ||
        (task_flags(task) & TASK_FLAG_FLIC) == 0u) return;
    if (state->task_flics[index] != NULL) {
        flic_close(state->task_flics[index]);
        state->task_flics[index] = NULL;
    }
    path = (const char*)task->data + TASK_OFF_FLICPATH2;
    flic = path[0] == '\0' ? NULL : flic_open(path);
    state->task_flics[index] = flic;
    task_put_int(task, TASK_OFF_FLIC_HANDLE,
                 (int32_t)(uint32_t)(uintptr_t)flic);
}

static void reward_task_flic_reload(RewardScreenState* state, int index)
{
    reward_task_flic_reload_record(state, reward_task_get(state, index), index);
}

void reward_task_flics_reload_all(RewardScreenState* state)
{
    int i;
    if (state == NULL) return;
    for (i = 0; i < GAME_TASK_COUNT; ++i)
        reward_task_flic_reload(state, i);
}

void reward_screen_state_init(RewardScreenState* state, GameContext* game,
                              PointerCursorState* pointer_cursor,
                              const PointerCursorHooks* pointer_hooks)
{
    if (state == NULL) return;
    memset(state, 0, sizeof(*state));
    state->game = game;
    state->pointer_cursor = pointer_cursor;
    state->pointer_cursor_hooks = pointer_hooks;
    state->task_duration = game != NULL ? game->reward.timer : 0.0f;
    if (state->task_duration == 0.0f)
        state->task_duration = REWARD_TASK_DURATION_DEFAULT;
    state->elapsed = state->task_duration;
}

void reward_screen_runtime_bind(RewardScreenState* state,
                                const RewardScreenOps* ops)
{
    s_reward_screen_state = state;
    s_reward_screen_ops = ops;
}

int reward_screen_tick_bound(void)
{
    return reward_screen_tick();
}

void reward_screen_transition_tick(RewardScreenState* state,
                                   const RewardScreenOps* ops)
{
    int return_to_game = 0;
    int reset_reward = 0;
    int events_available = 0;
    if (state == NULL || state->game == NULL ||
        (state->game->cheat & 0x10u) == 0u) return;
    if (ops != NULL && ops->transition_events != NULL) {
        events_available = ops->transition_events(
            ops->context, &return_to_game, &reset_reward);
    }
    else {
        /* RewardScreenTransitionTick reads the current/previous DirectInput
         * snapshots directly: Space + a newly pressed Escape requests the
         * ResetLevelAndExit path, while the new press at byte +0x13 resets
         * the reward sequence. Keep the target scan-code indexes and also
         * accept the portable event vocabulary used by native input. */
        const InputState* input = input_direct_state_get();
        if (input != NULL) {
            int escape_scan = input->cur[0x01] != 0 &&
                input->prev[0x01] != input->cur[0x01];
            int escape_event = input->cur[PLATFORM_KEY_ESC] != 0 &&
                input->prev[PLATFORM_KEY_ESC] !=
                    input->cur[PLATFORM_KEY_ESC];
            int space_down = input->cur[0x39] != 0 ||
                input->cur[' '] != 0;
            return_to_game = space_down &&
                (input->cur[0x01] != 0 ||
                 input->cur[PLATFORM_KEY_ESC] != 0) &&
                (escape_scan || escape_event);
            reset_reward = input->cur[0x13] != 0 &&
                input->prev[0x13] != input->cur[0x13];
            events_available = return_to_game || reset_reward;
        }
    }
    if (!events_available) return;
    if (return_to_game && ops != NULL && ops->return_to_game != NULL)
        ops->return_to_game(ops->context);
    if (!reset_reward) return;
    state->displayed_task_count = 0;
    state->transition_stage = 0;
    state->score_text_gate = 0;
    state->elapsed = state->task_duration;
    sound_buffer_stop_handle(0x29);
    sound_request_queue_submit(0x29, 1);
    reward_task_flics_reload_all(state);
}

void reward_pointer_override_expiry_tick(RewardScreenState* state,
                                         float delta,
                                         const RewardScreenOps* ops)
{
    if (state == NULL) return;
    ui_pointer_cursor_override_tick(1.0f);
    if (state->pointer_cursor != NULL &&
        state->pointer_cursor_hooks != NULL) {
        state->pointer_cursor->selected = 0;
        if (ops != NULL && ops->cursor_position_get != NULL) {
            ops->cursor_position_get(ops->context, &state->pointer_x,
                                     &state->pointer_y);
        }
        pointer_cursor_draw(state->pointer_cursor,
                            state->pointer_cursor_hooks,
                            state->pointer_x, state->pointer_y);
    }
    else {
        if (ops != NULL && ops->cursor_position_get != NULL) {
            ops->cursor_position_get(ops->context, &state->pointer_x,
                                     &state->pointer_y);
        }
        pointer_cursor_draw_target((uint32_t)state->pointer_x,
                                   (uint32_t)state->pointer_y);
    }
    if (ops != NULL && ops->cursor_input_update != NULL)
        ops->cursor_input_update(ops->context);
    state->pointer_override_active = 0;
    if (ops != NULL && ops->pointer_override_clear != NULL)
        ops->pointer_override_clear(ops->context);
    /* PointerOverrideExpiryTick @ 0x462700 advances the sound subsystem
     * after clearing the cursor override and before the generic effect tick
     * and frame pump. Keep that modal-frame boundary here. */
    sound_subsystem_frame_tick(delta);
    if (ops != NULL && ops->screen_effect_update != NULL)
        ops->screen_effect_update(ops->context, delta);
    if (ops != NULL && ops->pointer_effect_update != NULL)
        ops->pointer_effect_update(ops->context, state->pointer_x,
                                   state->pointer_y, delta * 40.0f);
    timed_effect_runtime_update(renderer_word_a_get(), renderer_word_b_get(),
                                delta * 40.0f);
    if (ops != NULL && ops->frame_pump != NULL)
        ops->frame_pump(ops->context, 1);
}

void reward_pointer_override_expiry_tick_bound(float delta)
{
    reward_pointer_override_expiry_tick(s_reward_screen_state, delta,
                                        s_reward_screen_ops);
}

/* Explicit-state adapter for portable callers. The retail entry point above
 * is global and no-argument; save and restore the binding so nested UI work
 * cannot replace the caller's reward state after returning. */
int reward_screen_tick_with_ops(RewardScreenState* state,
                                const RewardScreenOps* ops)
{
    RewardScreenState* const previous_state = s_reward_screen_state;
    const RewardScreenOps* const previous_ops = s_reward_screen_ops;
    int result;
    if (state == NULL) return 0;
    s_reward_screen_state = state;
    s_reward_screen_ops = ops;
    result = reward_screen_tick();
    s_reward_screen_state = previous_state;
    s_reward_screen_ops = previous_ops;
    return result;
}

namespace unported_ledger_stubs {

int ledger_stub_00460f00_RewardStatsPanelDraw(void)
{
    return reward_stats_panel_draw_bound();
}

int ledger_stub_00461070_RewardStatsTextFormat(void)
{
    return reward_stats_text_format_bound();
}

void ledger_stub_00461790_RewardDisplayTick(GameTask* task,
                                            uint32_t draw_mask,
                                            uint32_t task_number)
{
    reward_display_task_abi(task, draw_mask, task_number);
}

/* Portable framebuffer text adapter; the retail address expects task-count
 * and elapsed-time inputs and is implemented in mission.cpp. */
void ledger_stub_00461c90_RewardScoreTaskTextDraw(
    const TextureSurface* font, void* pixels, int pitch, int width, int height,
    int x, int y, const char* text)
{
    if (text != NULL)
        reward_text_draw(font, pixels, pitch, width, height, x, y, "%s", text);
}

void ledger_stub_00462390_RewardScreenTransitionTick(
    int* displayed_task_count, int* transition_stage, int* score_text_gate,
    float* elapsed)
{
    reward_screen_transition_tick_abi(displayed_task_count, transition_stage,
                                      score_text_gate, elapsed);
}

void ledger_stub_004624a0_RewardFlhContextTick(int* displayed_task_count,
                                                int* transition_stage,
                                                float* elapsed)
{
    reward_flic_context_tick_abi(displayed_task_count, transition_stage,
                                 elapsed);
}

} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {

void ledger_stub_00462700_PointerOverrideExpiryTick(
    float delta)
{
    pointer_override_expiry_tick(delta);
}

// RewardScrollTickerAdvanceAndDraw
int ledger_stub_00462ad0_RewardScrollTickerAdvanceAndDraw(
    RewardScrollTickerState* state)
{
    return reward_scroll_ticker_advance_and_draw(state);
}


} // namespace unported_ledger_stubs
