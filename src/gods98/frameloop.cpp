// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "game/game.hpp"
#include "game/ai.hpp"
#include "game/cmd.hpp"
#include "game/entity.hpp"
#include "game/building_scene.hpp"
#include "game/actor_scene.hpp"
#include "game/effects.hpp"
#include "game/frontend.hpp"
#include "game/level.hpp"
#include "game/mission.hpp"
#include "game/options.hpp"
#include "game/objective.hpp"
#include "game/port_loop_ports.hpp"
#include "game/render.hpp"
#include "game/savegame.hpp"
#include "game/select.hpp"
#include "game/tutorial_hud_ports.hpp"
#include "game/tutorial_world_arrows.hpp"
#include "game/scene_render.hpp"
#include "game/ui_action_dispatch.hpp"
#include "game/upgrades.hpp"
#include "game/util.hpp"
#include "platform/platform.h"
#include "audio.hpp"
#include "engine.hpp"
#include "frameloop.hpp"
#include "input.hpp"
#include "movie.hpp"
#include "media.hpp"
#include "flic.hpp"
#include "message.hpp"
#include "resource.hpp"
#include "surface.hpp"
#include "sound.hpp"

namespace unported_ledger_stubs {
int ledger_stub_004401b0_TerrainCellToWorldPosition(EntitySlot* entity,
                                                    int force);
void entity_attached_object_list_detach(EntitySlot* entity, int reset_mode);
int ledger_stub_0043fc20_RecordPairPoseCopyAndActivityRaise(
    EntitySlot* entity);
}
#include "string.hpp"
#include "texture.hpp"
#include "tickers.hpp"
#include "simtick.hpp"
#include "view.hpp"
#include "camera.hpp"
#include "crt/crt.hpp"
#include "frameloop_shared.hpp"

static void menu_sound_tick_clock_init(MenuSoundTickClock* clock)
{
    if (clock == NULL) return;
    clock->last_ms = platform_time_milliseconds();
    clock->dt = 1.0f;
}

static void menu_sound_tick_clock_advance(MenuSoundTickClock* clock)
{
    uint32_t now;
    if (clock == NULL) return;
    now = platform_time_milliseconds();
    clock->dt = (float)(now - clock->last_ms) * 0.025f;
    clock->last_ms = now;
}

static MainLoopFramePumpHooks s_main_loop_frame_pump_hooks;
static WindowRuntimeOps s_window_runtime_ops;
WindowRuntimeState s_window_runtime_state;
// GLOBAL: LEGORR 0x00505808
uint32_t s_window_application_active;
// GLOBAL: LEGORR 0x0050580c
uint32_t s_window_close_requested;
// GLOBAL: LEGORR 0x00505f80
static int32_t s_window_runtime_style;
static WindowDefaultProc s_window_default_proc;
static void* s_window_default_proc_context;
static char s_command_line_level_override[FE_LEVEL_NAME_MAX];
static int s_command_line_sound_disabled;

#define FRAMELOOP_PHASE_MENU 1
#define FRAMELOOP_PHASE_GAMEPLAY_START 2
#define FRAMELOOP_PHASE_GAMEPLAY 3
#define FRAMELOOP_PHASE_DONE 4

FrameloopApplicationState s_frameloop_application;

/* A close can be delivered directly to WndProc, without a converted event.
 * Keep both routes sticky; only the outer callback owner performs shutdown. */
static int frameloop_application_quit_ingest(FrameloopApplicationState* app,
    const PlatformEvent* events, int count)
{
    for (int i = 0; i < count; ++i) {
        if (events[i].kind == PLATFORM_EVENT_QUIT)
            s_window_close_requested = 1u;
    }
    if (s_window_close_requested != 0u) {
        s_window_runtime_state.close_requested = 1u;
        app->quit = 1;
        app->stop_reason = "closed";
    }
    return app->quit;
}

/* The event backend clears cur each frame, so a repeated key-down can look
 * like a fresh edge. Every modal capture tracks the full removed batch before
 * any skip/leave decision, retaining both quit and Escape release state. */
int frameloop_application_events_track(FrameloopApplicationState* app,
    const PlatformEvent* events, int count)
{
    int pressed = 0;
    (void)frameloop_application_quit_ingest(app, events, count);
    for (int i = 0; i < count; ++i) {
        if (events[i].kind == PLATFORM_EVENT_MOUSE)
            app->pause_mouse_down = events[i].held != 0;
        if (events[i].code != PLATFORM_KEY_ESC) continue;
        if (events[i].kind == PLATFORM_EVENT_KEYDOWN) {
            if (!app->pause_escape_down) pressed = 1;
            app->pause_escape_down = 1;
        } else if (events[i].kind == PLATFORM_EVENT_KEYUP) {
            app->pause_escape_down = 0;
        }
    }
    return pressed;
}

/* The menu owns its closing mouse gesture through its physical release.
 * Filter that gesture before any gameplay consumer sees the input state. */
static void frameloop_gameplay_input_capture(FrameloopApplicationState* app,
    const PlatformEvent* events, int count)
{
    for (int i = 0; i < count; ++i) {
        const PlatformEvent* event = &events[i];
        if (app->gameplay_mouse_suppressed &&
            event->kind == PLATFORM_EVENT_MOUSE) {
            /* Track the suppressed physical release, but admit no samples
             * or aggregate press to any gameplay consumer. */
            input_capture_events(&app->input, event, 1);
            input_pointer_rearm(&app->input);
            app->input.mouse_down = 0;
            if (!event->held) app->gameplay_mouse_suppressed = 0;
            continue;
        }
        input_capture_events(&app->input, event, 1);
    }
}

void frameloop_application_start_level_override_set(const char* level_name)
{
    size_t length = 0;
    if (level_name == NULL) {
        s_command_line_level_override[0] = '\0';
        return;
    }
    while (level_name[length] != '\0' &&
           length + 1u < sizeof(s_command_line_level_override)) {
        s_command_line_level_override[length] = level_name[length];
        ++length;
    }
    s_command_line_level_override[length] = '\0';
}

void frameloop_application_sound_disabled_set(int disabled)
{
    s_command_line_sound_disabled = disabled != 0;
}

static void* frameloop_indexed_texture_load(const char* name, void*)
{
    return texture_get_or_load(NULL, name);
}

static void frameloop_indexed_texture_release(void* texture, void*)
{
    texture_release((TextureSurface*)texture);
}

static uint32_t reward_frame_time_get(void* context)
{
    (void)context;
    return platform_time_milliseconds();
}

static int reward_frame_loop_abort(void* opaque)
{
    RewardFramePumpContext* context = (RewardFramePumpContext*)opaque;
    (void)frameloop_application_quit_ingest(&s_frameloop_application, NULL, 0);
    return context != NULL && context->quit != NULL && *context->quit != 0;
}

static void reward_task_visual_queue(void* opaque, GameTask* task,
                                     int task_index, uint32_t draw_mask,
                                     int displayed_count, float elapsed,
                                     Flic* flic)
{
    RewardFramePumpContext* context = (RewardFramePumpContext*)opaque;
    (void)task;
    (void)displayed_count;
    (void)elapsed;
    if (context == NULL || task_index < 0 || task_index >= GAME_TASK_COUNT)
        return;
    context->task_draw_masks[task_index] |= draw_mask;
    if (draw_mask == 4u) context->task_flics[task_index] = flic;
}

static void reward_task_text_queue(void* opaque, GameTask* task,
                                   int task_index, int displayed_count,
                                   float elapsed)
{
    RewardFramePumpContext* context = (RewardFramePumpContext*)opaque;
    (void)task;
    (void)displayed_count;
    (void)elapsed;
    if (context == NULL || task_index < 0 || task_index >= GAME_TASK_COUNT)
        return;
    context->task_draw_masks[task_index] |= 1u;
}

static int reward_score_text_queue(void* opaque, GameContext* game,
                                   int displayed_count, float elapsed)
{
    RewardFramePumpContext* context = (RewardFramePumpContext*)opaque;
    (void)game;
    (void)displayed_count;
    (void)elapsed;
    if (context == NULL) return 0;
    context->score_text_requested = 1;
    return 1;
}

/* RewardStatsPanelDraw selects and formats stats_result_text in the mission
 * module. The frame owner already has the configured text position, task
 * font atlas, and software surface renderer used by the score text path;
 * queue that result through the same concrete renderer. */
static void reward_stats_result_queue(void* opaque, GameContext* game)
{
    RewardFramePumpContext* context = (RewardFramePumpContext*)opaque;
    if (context == NULL || context->game != game || game == NULL) return;
    context->stats_result_requested = 1;
}

static int reward_zone_dispatch(void* opaque, RewardScreenState* state)
{
    RewardFramePumpContext* context = (RewardFramePumpContext*)opaque;
    int hit_save = 0;
    int hit_advance = 0;
    if (context == NULL || state == NULL || state->game == NULL ||
        context->input == NULL || !input_mouse_pressed(context->input) ||
        !mission_reward_zone(state->game, context->input->mouse_x,
                             context->input->mouse_y, &hit_save,
                             &hit_advance)) return 0;
    if (hit_save) {
        if (context->frontend != NULL)
            (void)save_menu_open_from_reward(context->frontend);
        state->transition_state = 1;
        state->transition_stage = 3; /* hand the save menu to its frame owner */
        sound_request_queue_submit(0x29, 1);
        context->zone_action_consumed = 1;
        return 1;
    }
    if (hit_advance) {
        if (!s_frameloop_application.picker_return_pending)
            checkpoint_step(state->game);
        state->transition_stage = 3;
        context->zone_action_consumed = 2;
        return 1;
    }
    return 0;
}

static void reward_frame_draw_task(RewardFramePumpContext* context,
                                   int index, uint32_t draw_pass,
                                   void* pixels, int pitch)
{
    GameTask* task;
    uint32_t masks;
    int x;
    int y;
    if (context == NULL || context->game == NULL || index < 0 ||
        index >= GAME_TASK_COUNT) return;
    task = &context->game->tasks[index];
    masks = context->task_draw_masks[index];
    if (draw_pass == 2u && (masks & 2u) != 0u) {
        TextureSurface* image = (TextureSurface*)
            reward_task_image_surface_get((unsigned int)index);
        memcpy(&x, task->data + TASK_OFF_IMG_XY, sizeof(x));
        memcpy(&y, task->data + TASK_OFF_IMG_XY + 4, sizeof(y));
        if (image != NULL)
            surface_blit_texture(pixels, pitch, WINDOW_W, WINDOW_H,
                                 image, x, y);
    }
    if (draw_pass == 0x40u && (masks & 0x40u) != 0u) {
        TextureSurface* image = (TextureSurface*)
            reward_task_box_image_surface_get((unsigned int)index);
        memcpy(&x, task->data + TASK_OFF_BOXIMG_XY, sizeof(x));
        memcpy(&y, task->data + TASK_OFF_BOXIMG_XY + 4, sizeof(y));
        if (image != NULL)
            surface_blit_texture(pixels, pitch, WINDOW_W, WINDOW_H,
                                 image, x, y);
    }
    if (draw_pass == 4u && (masks & 4u) != 0u &&
        context->task_flics[index] != NULL) {
        Flic* flic = context->task_flics[index];
        const uint8_t* rgba = flic_frame_rgba(flic);
        if (rgba != NULL) {
            TextureSurface frame;
            int32_t rect_x;
            int32_t rect_y;
            memset(&frame, 0, sizeof(frame));
            frame.width = flic_width(flic);
            frame.height = flic_height(flic);
            frame.pixels = (uint32_t*)rgba;
            memcpy(&rect_x, task->data + TASK_OFF_FLIC_RECT,
                   sizeof(rect_x));
            memcpy(&rect_y, task->data + TASK_OFF_FLIC_RECT + 4,
                   sizeof(rect_y));
            surface_blit_texture(pixels, pitch, WINDOW_W, WINDOW_H,
                                 &frame, rect_x, rect_y);
        }
    }
    if (draw_pass == 1u && (masks & 1u) != 0u) {
        void* font_record = reward_task_font_record_get((unsigned int)index);
        TextureSurface* font = font_atlas_record_surface_get(font_record);
        const char* text = (const char*)task->data + TASK_OFF_TEXT;
        memcpy(&x, task->data + TASK_OFF_TEXT_XY, sizeof(x));
        memcpy(&y, task->data + TASK_OFF_TEXT_XY + 4, sizeof(y));
        if (font != NULL && text[0] != '\0')
            reward_text_draw(font, pixels, pitch, WINDOW_W, WINDOW_H,
                             x, y, "%s", text);
    }
}

static void reward_frame_pump(void* opaque, int mode)
{
    RewardFramePumpContext* context = (RewardFramePumpContext*)opaque;
    PlatformEvent events[32];
    int event_count;
    int i;
    void* pixels = NULL;
    int pitch = 0;
    (void)mode;
    if (context == NULL || context->window == NULL ||
        context->surface == NULL || context->input == NULL ||
        context->game == NULL) return;

    if (context->quit != NULL && *context->quit == 0 &&
        context->frame_cap > 0 && context->frames != NULL &&
        *context->frames >= context->frame_cap) {
        *context->quit = 1;
        if (context->stop != NULL) *context->stop = "autostop";
    }
    if (context->quit != NULL && *context->quit != 0) {
        renderer_word_c_set(0u);
        return;
    }
    /* Preserve this frame's edge through the next RewardScreenTick body;
     * its zone check runs before the next pump, then this boundary advances
     * `prev` before capturing the following events. */
    input_begin_frame(context->input);
    event_count = platform_pump_events(context->window, events, 32);
    (void)frameloop_application_events_track(&s_frameloop_application,
                                            events, event_count);
    if (context->quit != NULL && *context->quit != 0) {
        renderer_word_c_set(0u);
        return;
    }
    input_capture_events(context->input, events, event_count);
    input_cursor_position_update();
    input_cursor_position_sync(context->input);
    renderer_word_c_set(context->input->mouse_down != 0 ? 1u : 0u);
    for (i = 0; i < event_count; ++i) {
        if (events[i].kind == PLATFORM_EVENT_QUIT) {
            if (context->quit != NULL) *context->quit = 1;
            if (context->stop != NULL) *context->stop = "closed";
        }
    }

    renderer_begin_frame_once_reset();
    (void)renderer_begin_frame_once();
    if (surface_lock(context->surface, &pixels, &pitch) == 0) {
        if (context->game->level != NULL) {
            level_render_scene(context->game->level, context->game,
                               WINDOW_W, WINDOW_H, pixels, pitch);
            render_apply_brightness(pixels, pitch, WINDOW_W, WINDOW_H,
                                    context->game->options.brightness);
            frontend_help_overlay(pixels, pitch, WINDOW_W, WINDOW_H,
                                  objective_stream(context->game->level),
                                  &context->game->options);
        } else {
            surface_fill(pixels, pitch, WINDOW_W, WINDOW_H, 8, 16, 48);
            render_apply_brightness(pixels, pitch, WINDOW_W, WINDOW_H,
                                    context->game->options.brightness);
        }
        {
            static const uint32_t reward_draw_passes[] = { 2u, 0x40u, 4u, 1u };
            unsigned int pass;
            for (pass = 0; pass < 4u; ++pass) {
                for (i = 0; i < GAME_TASK_COUNT; ++i)
                    reward_frame_draw_task(context, i,
                        reward_draw_passes[pass], pixels, pitch);
            }
        }
        if ((context->score_text_requested || context->stats_result_requested) &&
            context->game->reward_screen.stats_result_text[0] != '\0') {
            TextureSurface* font = (TextureSurface*)
                font_atlas_record_surface_get(
                    reward_task_font_record_get(GAME_TASK_COUNT - 1));
            if (font != NULL)
                reward_text_draw(font, pixels, pitch, WINDOW_W, WINDOW_H,
                    context->game->reward_screen.text_pos[0],
                    context->game->reward_screen.text_pos[1], "%s",
                    context->game->reward_screen.stats_result_text);
        }
        /* RewardZoneCheck follows the task and score text passes in the
         * retail modal loop, keeping the button art in the foreground. */
        mission_reward_buttons_draw(context->game, pixels, pitch,
                                    WINDOW_W, WINDOW_H,
                                    context->input->mouse_x,
                                    context->input->mouse_y,
                                    context->input->mouse_down);
        platform_present_pixels(context->window, pixels, pitch,
                                WINDOW_W, WINDOW_H);
        surface_unlock(context->surface, pixels);
    }
    memset(context->task_draw_masks, 0, sizeof(context->task_draw_masks));
    memset(context->task_flics, 0, sizeof(context->task_flics));
    context->score_text_requested = 0;
    context->stats_result_requested = 0;
    if (context->audio != NULL) audio_update(context->audio);
    Sleep((DWORD)FRAME_MS);
    if (context->frames != NULL) ++*context->frames;
}

/* RewardSaveButtonZoneCheck @ 0x00462780 gates the modal reward loop on the
 * configured Display value and checkpoint Reward/Enable word. It then polls
 * the DirectInput Space byte (current and previous snapshots) until release,
 * clears the per-frame mouse-press latch, and only then queues cue 0x29 in
 * mission.cpp. The portable event adapter keeps held keys until KEYUP, with
 * Space represented by its canonical ASCII code. */
static int reward_save_button_check(void* opaque, GameContext* game)
{
    RewardFramePumpContext* context = (RewardFramePumpContext*)opaque;
    InputState* input;
    uint32_t enabled;
    int release_tail = 0;
    if (context == NULL || game == NULL || game->level == NULL ||
        game->checkpoint_blob == NULL || game->reward_screen.display == 0 ||
        context->input == NULL) return 0;
    memcpy(&enabled, game->checkpoint_blob, sizeof(enabled));
    if (enabled == 0u) return 0;
    input = context->input;
    for (;;) {
        const int space_down = input_keyboard_key_held(' ') ||
            input_keyboard_key_held(0x39) || input->cur[' '] != 0u ||
            input->prev[' '] != 0u || input->cur[0x39] != 0u ||
            input->prev[0x39] != 0u;
        if (!space_down) {
            if (!release_tail) break;
            /* Retail checks both snapshots, so perform one final poll after
             * the held-key table observes KEYUP. */
            release_tail = 0;
        } else {
            release_tail = 1;
        }
        input->mouse_press = 0;
        if (context->quit != NULL && *context->quit != 0) break;
        reward_frame_pump(context, 1);
        if (context->quit != NULL && *context->quit != 0) break;
    }
    input->mouse_press = 0;
    return 1;
}

typedef struct FramePointerDispatchContext {
    GameContext* game;
    LevelState* level;
    EngineContext* engine;
    int screen_width;
    int screen_height;
    UiActionDispatchState* ui_action_state;
} FramePointerDispatchContext;

static void* frame_ui_action_current_selection(void* context)
{
    FramePointerDispatchContext* state =
        (FramePointerDispatchContext*)context;
    return state != NULL ? selection_first(state->game) : NULL;
}

static int frame_ui_action_already_held(void* context, int action, int mode)
{
    FramePointerDispatchContext* state =
        (FramePointerDispatchContext*)context;
    return input_action_pressed_get(state != NULL ? state->engine : NULL,
                                    action, mode);
}

/* CatalogSlotFlagUpdate (FUN_0041c800) uses these exact helpers for its
 * category-1/category-4 eligibility and cell-resource checks. The category-1
 * rank read is retained even though the retail comparison's second operand
 * is the return residue of a void no-op. */
static int frame_catalog_slot_eligible(int category, int slot,
                                       void* context)
{
    return object_interaction_flags_allow((GameContext*)context, category,
                                          slot);
}

static int frame_catalog_slot_available(int category, int slot,
                                        void* context)
{
    (void)context;
    return ensure_cell_entries_loaded((uint32_t)category, slot, 0);
}

static int frame_catalog_slot_rank(int category, int slot, void* context)
{
    (void)context;
    if (category != 1) return 0;
    return (int)entity_indexed_record_word_at_b4_get(1, slot, 0);
}

static int frame_action_catalog_query(void* context, int action)
{
    FrameloopApplicationState* app = (FrameloopApplicationState*)context;
    if (app == NULL || app->game_storage == NULL) return 0;
    /* The reference predicate reads the current selected tile, not the
     * cursor or pending tile. Retrieve the live contexts on every call;
     * a frame-local pointer-dispatch record does not outlive its scope. */
    return tutorial_path_menu_action(app->game_storage, &app->engine_context,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        action, (int)map_store_pair_word(0), (int)map_store_pair_word(1));
}

static int frame_action_catalog_hit(void* context, float x, float y,
    float offset_x, float offset_y, uint32_t* action, uint32_t* kind,
    uint32_t* entry)
{
    FramePointerDispatchContext* frame = (FramePointerDispatchContext*)context;
    if (frame == NULL) return 0;
    return frontend_action_catalog_hit(frame->engine, frame->game,
        x, y, offset_x, offset_y, action, kind, entry);
}

static uint32_t frame_ui_action_entity_flags(void* context, void* entity,
                                             int offset)
{
    EntitySlot* slot = (EntitySlot*)entity;
    (void)context;
    if (slot == NULL) return 0u;
    if (offset == 0x3e8) return entity_stats_word_get(slot);
    if (offset == 0x3ec) return slot->flags_3ec;
    return 0u;
}

static uint8_t frame_ui_action_entity_byte_flags(void* context, void* entity,
                                                 int offset)
{
    EntitySlot* slot = (EntitySlot*)entity;
    (void)context;
    if (slot == NULL || offset != 0x3ec) return 0u;
    return (uint8_t)slot->flags_3ec;
}

static int frame_ui_action_entity_type(void* context, void* entity)
{
    (void)context;
    return entity != NULL ? ((EntitySlot*)entity)->type : 0;
}

/* UiActionDispatch's case 0x25 selects one of three targets according to the
 * exact stats-word tests at 0x439cf0/0x439d10/0x439d30. These data and helpers
 * are available in the live EntitySlot, so bind the predicates directly. */
static int frame_ui_action_selected_entity_predicate(void* context,
        void* entity, int predicate_id)
{
    const EntitySlot* slot = (const EntitySlot*)entity;
    (void)context;
    if (slot == NULL) return 0;
    switch (predicate_id) {
    case 0x39cf0: return entity_stats_flag10_test(slot) != 0u;
    case 0x39d10: return entity_stats_flag20_test(slot) != 0u;
    case 0x39d30: return entity_stats_flag40_test(slot) != 0u;
    default: return 0;
    }
}

static uint32_t frame_ui_action_cell_flags(void* context, int x, int y)
{
    FramePointerDispatchContext* state =
        (FramePointerDispatchContext*)context;
    uint8_t* voxel;
    uint32_t flags = 0u;
    if (state == NULL || state->level == NULL) return 0u;
    voxel = level_voxel(state->level, x, y);
    if (voxel != NULL) memcpy(&flags, voxel + 8, sizeof(flags));
    return flags;
}

/* UiActionDispatch supplies these exact arguments at its cell-action cases:
 * 0x0b calls 0x402510(pair, 0, 1), 0x0c calls 0x403030(pair), 0x0d calls
 * 0x402d50(pair, 0), 0x0e calls 0x402fb0(pair), and 0x0f calls
 * 0x402ae0(pair, 0). */
static void frame_ui_action_cell_command_02510(void* context, int x, int y)
{
    (void)context;
    terrain_cell_command_create(x, y, 0, 1);
}

static void frame_ui_action_cell_command_03030(void* context, int x, int y)
{
    (void)context;
    cell_command_type3_issue(x, y);
}

static void frame_ui_action_cell_refresh_02ae0(void* context, int x, int y)
{
    int cell_xy[2];
    (void)context;
    cell_xy[0] = x;
    cell_xy[1] = y;
    terrain_cell_refresh_dispatch_current(cell_xy, 0);
}

static void frame_ui_action_terrain_cell_to_world(void* context,
                                                  void* entity, int mode)
{
    (void)context;
    (void)unported_ledger_stubs::
        ledger_stub_004401b0_TerrainCellToWorldPosition(
            (EntitySlot*)entity, mode);
}

static void frame_ui_action_cell_command_02d50(void* context, int x, int y)
{
    (void)context;
    (void)kind_nine_cell_command_issue(x, y, 0);
}

static void frame_ui_action_cell_command_02fb0(void* context, int x, int y)
{
    (void)context;
    cell_command_type11_issue(x, y);
}

static void frame_ui_action_mode_request(void* context, int mode,
                                         int argument)
{
    (void)context;
    (void)argument;
    command_context_post_to_hud(mode, NULL);
}

static void frame_ui_action_input_request(void* context, int kind,
                                           void* entity, int argument_a,
                                           int argument_b)
{
    int pair[2];
    (void)context;
    pair[0] = argument_a;
    pair[1] = argument_b;
    cmd_produce(kind, entity, argument_a, pair);
}

static void frame_ui_action_map_request(void* context, int action, int mode,
                                         int x, int y)
{
    int pair[2];
    (void)context;
    pair[0] = x;
    pair[1] = y;
    cmd_produce(action, NULL, mode, pair);
}

static void frame_ui_action_selection_clear(void* context)
{
    (void)context;
    selection_clear_request_and_hud_reset();
}

static void frame_ui_action_helper_00452be0(void* context)
{
    FramePointerDispatchContext* state =
        (FramePointerDispatchContext*)context;
    if (state != NULL && state->game != NULL) selection_clear(state->game);
}

static void frame_ui_action_entity_kind_command(void* context, void* entity)
{
    (void)context;
    entity_kind_command_issue((EntitySlot*)entity);
}

static void frame_ui_action_entity_action_kind_create(void* context,
                                                       void* entity,
                                                       int kind)
{
    (void)context;
    (void)entity_action_kind_entity_create_and_command_issue(
        (EntitySlot*)entity, kind, 0, 0);
}

static void frame_ui_action_entity_flag_80000000(void* context, void* entity,
                                                  int set)
{
    FramePointerDispatchContext* state =
        (FramePointerDispatchContext*)context;
    if (state != NULL && state->game != NULL)
        entity_flag80000000_set_or_clear(state->game,
            (EntitySlot*)entity, set);
}

static void frame_ui_action_conditional_task_schedule(void* context,
                                                       void* entity)
{
    (void)context;
    entity_conditional_task_schedule((EntitySlot*)entity);
}

static void* frame_ui_action_record_find(void* context, void* entity,
                                          int mode, int mask)
{
    (void)context;
    if (mode != 0) return NULL;
    return unported_ledger_stubs::
        ledger_stub_00438e50_SpatialObjectQueryCenterMask100(
            (const EntitySlot*)entity, NULL, (uint32_t)mask);
}

static void frame_ui_action_record_apply(void* context, void* record,
                                          int mask, int enabled)
{
    (void)context;
    if (record != NULL)
        task_type23_create_for_category_mask((EntitySlot*)record, mask,
                                             enabled);
}

static void frame_ui_action_object_command_task_create(void* context,
                                                        void* entity,
                                                        int mode)
{
    (void)context;
    object_command_task_create((EntitySlot*)entity, mode);
}

static void frame_ui_action_entity_field_f6_clear(void* context,
                                                   void* entity)
{
    (void)context;
    if (entity != NULL)
        ((EntitySlot*)entity)->activity_counter_3d8 = 0.0f;
}

static int frame_ui_action_cursor_action_rejected(void* context, int first,
                                                   int second, int mode)
{
    FramePointerDispatchContext* state =
        (FramePointerDispatchContext*)context;
    return input_binding_pressed_get(state != NULL ? state->engine : NULL,
                                     first, second, mode);
}

/* TutorialPathDescriptorCreate (FUN_00409300): when the selected cell does
 * not carry 0x20000000, allocate/link its coordinate record, raise the
 * cell's 0x40000000 flag (including CellInitPass), set record +0xd0 bit 2,
 * and cancel an action matching the coordinate. */
static void frame_ui_action_tutorial_path_descriptor_create(void* context,
                                                              int x, int y)
{
    FramePointerDispatchContext* state =
        (FramePointerDispatchContext*)context;
    uint8_t* voxel;
    uint32_t flags;
    uint32_t pair[2];
    void* record;
    if (state == NULL || state->level == NULL) return;
    voxel = level_voxel(state->level, x, y);
    if (voxel == NULL) return;
    memcpy(&flags, voxel + 8, sizeof(flags));
    if ((flags & 0x20000000u) != 0u) return;
    pair[0] = (uint32_t)x;
    pair[1] = (uint32_t)y;
    record = path_record_allocate_and_link(pair);
    if (record == NULL) return;
    terrain_cell_flag40000000_clear_or_set(state->level, x, y, 1);
    *(uint32_t*)((uint8_t*)record + 0xd0) |= 2u;
    cancel_action_for_matching_coordinates(pair);
}

static void frame_ui_action_selection_group_clear(void* context, int x,
                                                  int y)
{
    uint32_t pair[2];
    (void)context;
    pair[0] = (uint32_t)x;
    pair[1] = (uint32_t)y;
    simtick_selection_group_clear(pair);
}

static void frame_ui_action_cell_mask_clear(void* context, int x, int y,
                                             uint32_t mask)
{
    FramePointerDispatchContext* state =
        (FramePointerDispatchContext*)context;
    uint8_t* voxel;
    uint32_t flags;
    if (state == NULL || state->level == NULL) return;
    voxel = level_voxel(state->level, x, y);
    if (voxel == NULL) return;
    memcpy(&flags, voxel + 8, sizeof(flags));
    flags &= ~mask;
    memcpy(voxel + 8, &flags, sizeof(flags));
    terrain_cell_changed_flag_set(state->level->ptr_terrain, x, y);
}

static void frame_ui_action_cell_update(void* context, int x, int y)
{
    FramePointerDispatchContext* state =
        (FramePointerDispatchContext*)context;
    if (state != NULL && state->level != NULL)
        level_cell_init_pass(state->level, x, (uint32_t)y);
}

static void frame_ui_action_terrain_descriptor_reset(void* context,
                                                      int x, int y)
{
    FramePointerDispatchContext* state =
        (FramePointerDispatchContext*)context;
    int cell_xy[2];
    if (state == NULL || state->level == NULL) return;
    cell_xy[0] = x;
    cell_xy[1] = y;
    unported_ledger_stubs::ledger_stub_00409600_TerrainDescriptorTilesResetAndFree(
        cell_xy);
}

static void frame_ui_action_global_flags_clear(void* context)
{
    (void)context;
    global_state_low_byte_clear_and_refresh();
}

static void frame_ui_action_cursor_request(void* context, int action,
                                            int value)
{
    (void)context;
    (void)interface_spawn_request_enqueue(action, (uint32_t)value);
}

static void frame_ui_action_building_upgrade_commit(void* context,
                                                    void* entity)
{
    (void)context;
    building_upgrade_commit((EntitySlot*)entity);
}

/* Cases 0x16/0x17 pass FUN_0041f6c0 to SelectedEntitiesAnyMatch. The
 * callback always returns zero, so every selected entity is visited. */
static void frame_ui_action_selected_entities_detach_or_pose(void* context)
{
    FramePointerDispatchContext* state =
        (FramePointerDispatchContext*)context;
    EntitySlot* const* entities;
    int count, i;
    if (state == NULL || state->game == NULL) return;
    count = selection_count(state->game);
    entities = selection_record_array(state->game);
    for (i = 0; entities != NULL && i < count; ++i) {
        EntitySlot* entity = entities[i];
        EntitySlot* record;
        uint32_t capabilities;
        if (entity == NULL) continue;
        capabilities = entity_capability_flags_get(entity);
        record = (EntitySlot*)entity->carry_link_308;
        if ((capabilities & 1u) == 0u) {
            if ((entity->flags_3e0 & 0x400u) == 0u || record == NULL ||
                record->type != 9) {
                unported_ledger_stubs::
                    entity_attached_object_list_detach(entity, 0);
            }
        }
        else if ((entity->flags_3e0 & 0x400u) != 0u && record != NULL) {
            (void)unported_ledger_stubs::
                ledger_stub_0043fc20_RecordPairPoseCopyAndActivityRaise(record);
        }
    }
}

/* Action 0x21 walks the selection with FUN_0041f840. That target returns
 * zero for every entity after applying its class-6 marker behavior. */
static void frame_ui_action_selected_entity_action_21(void* context)
{
    FramePointerDispatchContext* state =
        (FramePointerDispatchContext*)context;
    EntitySlot* const* entities;
    int count, i;
    if (state == NULL || state->game == NULL) return;
    count = selection_count(state->game);
    entities = selection_record_array(state->game);
    for (i = 0; entities != NULL && i < count; ++i)
        (void)entity_remove_object_if_class6(state->game, entities[i]);
}

static void frame_ui_action_count(void* context, int action, int entry)
{
    (void)context;
    input_action_completion_record(action,
        (int)objective_selection_word_peek(), entry);
}

static void frame_ui_action_complete(void* context, int action, int kind,
                                    int entry)
{
    FramePointerDispatchContext* frame =
        (FramePointerDispatchContext*)context;
    UiActionDispatchState* state;
    int result;
    if (frame == NULL || frame->ui_action_state == NULL) return;
    state = frame->ui_action_state;
    state->current_mode = (int)objective_selection_word_peek();
    state->selected_cell_x = (int)map_store_pair_word(0);
    state->selected_cell_y = (int)map_store_pair_word(1);
    state->cursor_action_x = (int)cursor_tile_pair_peek(0);
    state->cursor_action_y = (int)cursor_tile_pair_peek(1);
    state->action_flags = global_state_low_word_get();
    state->cursor_flags = terrain_choice_flags_peek();
    tutorial_nerp_pointer_release_record(action, kind, entry);
    ui_action_dispatch_state_install(state);
    result = ui_action_dispatch(action);
    global_state_low_word_set(state->action_flags);
    terrain_choice_flags_set(state->cursor_flags);
    ui_action_dispatch_state_install(NULL);
    media_action_feedback_sound_dispatch(result == 0 ? 1 : 0);
}

#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
static TutorialWorldArrows s_frame_world_arrows;
static LevelState* s_frame_world_arrow_level;
static uint32_t s_frame_world_arrow_level_generation;
static uint32_t s_frame_world_arrow_frame;
static int s_frame_world_arrow_lists_active;
static int s_frame_world_arrow_pending;
static int s_frame_world_arrow_display_ready = 1;

static void frame_world_arrows_level_releasing(void)
{
    terrain_camera_detail_restore(s_frame_world_arrow_level);
    scene_render_terrain_surface_display_discard();
    s_frame_world_arrow_display_ready = 1;
    s_frame_world_arrow_pending = 0;
    s_frame_world_arrow_lists_active = 0;
    s_frame_world_arrow_level = NULL;
    tutorial_world_arrows_discard(&s_frame_world_arrows);
}

static void frame_world_arrows_level_activated(LevelState* level)
{
    frame_world_arrows_level_releasing();
    ++s_frame_world_arrow_level_generation;
    if (s_frame_world_arrow_level_generation == 0u)
        ++s_frame_world_arrow_level_generation;
    s_frame_world_arrow_level = level;
    /* Prerequisite: level_load now calls tutorial_path_coordinate_index
     * after OList/counters and clears every raw group byte before returning.
     * Never set this flag against the uncorrected base level loader. */
    s_frame_world_arrow_lists_active = level != NULL;
}

static void frame_world_arrows_begin_frame(void)
{
    scene_render_terrain_surface_display_discard();
    s_frame_world_arrow_display_ready = 1;
    s_frame_world_arrow_pending = 0;
    ++s_frame_world_arrow_frame;
}

static void frame_world_arrows_capture(EngineContext* engine,
                                       float raw_seconds, void* context)
{
    GameContext* game = (GameContext*)context;
    if (game == NULL || engine == NULL || game->level == NULL ||
        game->level != s_frame_world_arrow_level) return;
    if (engine->context_mode == 1 &&
        (engine->state & ENGINE_STATE_TIME_FROZEN) == 0u &&
        s_frame_world_arrow_lists_active &&
        !scene_render_terrain_surface_display_begin(game->level))
        s_frame_world_arrow_display_ready = 0;
    s_frame_world_arrow_pending = tutorial_world_arrows_capture(
        &s_frame_world_arrows, game->level, s_frame_world_arrow_level_generation,
        s_frame_world_arrow_frame, engine->context_mode,
        (engine->state & ENGINE_STATE_TIME_FROZEN) != 0u,
        s_frame_world_arrow_lists_active,
        tutorial_world_arrows_delta_from_seconds(raw_seconds));
}

typedef struct FrameWorldArrowDisplay {
    const GameContext* game;
    const LevelState* level;
    int width, height;
    unsigned int generation;
} FrameWorldArrowDisplay;

static int frame_world_arrows_project(void* context, const float world[3],
                                       float screen[2])
{
    FrameWorldArrowDisplay* display = (FrameWorldArrowDisplay*)context;
    return scene_render_displayed_world_point_project(display->level,
        display->game, display->width, display->height, display->generation,
        world, screen);
}

static void frame_world_arrows_draw(GameContext* game, void* pixels,
                                     int pitch, int width, int height)
{
    FrameWorldArrowDisplay display;
    ViewCameraProjection projection;
    /* Terrain has finished; later draws may use its current logical colors. */
    scene_render_terrain_surface_display_discard();
    if (!s_frame_world_arrow_pending || game == NULL || game->level == NULL ||
        game->reward_active || game->level != s_frame_world_arrow_level ||
        !view_primary_projection_get(engine_current_primary_view_get(),
            game->level, width, height, &projection)) return;
    display.game = game;
    display.level = game->level;
    display.width = width;
    display.height = height;
    display.generation = projection.generation;
    tutorial_world_arrows_draw(&s_frame_world_arrows, game->level,
        s_frame_world_arrow_level_generation, frame_world_arrows_project,
        &display, pixels, pitch, width, height);
}
#endif

typedef struct FrameTooltipDrawContext {
    void* pixels;
    int pitch;
    int width;
    int height;
} FrameTooltipDrawContext;

static void frame_tooltip_pixel(FrameTooltipDrawContext* ctx, int x, int y,
                                const uint32_t color[3])
{
    unsigned char* pixel;
    if (x < 0 || y < 0 || x >= ctx->width || y >= ctx->height) return;
    pixel = (unsigned char*)ctx->pixels + y * ctx->pitch + x * 4;
    pixel[0] = (unsigned char)color[0];
    pixel[1] = (unsigned char)color[1];
    pixel[2] = (unsigned char)color[2];
    pixel[3] = 255;
}

static void frame_tooltip_fill(void* context,
                               const RectangularGridOverlayState* state, int mode)
{
    FrameTooltipDrawContext* ctx = (FrameTooltipDrawContext*)context;
    int x, y;
    (void)mode;
    for (y = (int)state->y; y < (int)(state->y + state->height); ++y)
        for (x = (int)state->x; x < (int)(state->x + state->width); ++x)
            frame_tooltip_pixel(ctx, x, y, state->fill_color);
}

static void frame_tooltip_lines(void* context, const float points[10],
                                unsigned int count, const uint32_t color[3],
                                int mode)
{
    FrameTooltipDrawContext* ctx = (FrameTooltipDrawContext*)context;
    unsigned int i;
    (void)mode;
    for (i = 1; i < count; ++i) {
        int x = (int)points[(i - 1) * 2], y = (int)points[(i - 1) * 2 + 1];
        int end_x = (int)points[i * 2], end_y = (int)points[i * 2 + 1];
        int dx = end_x > x ? 1 : (end_x < x ? -1 : 0);
        int dy = end_y > y ? 1 : (end_y < y ? -1 : 0);
        for (;;) {
            frame_tooltip_pixel(ctx, x, y, color);
            if (x == end_x && y == end_y) break;
            x += dx; y += dy;
        }
    }
}

static void frame_tooltip_text(void* context, void* font, const char* text,
                               float x, float y)
{
    FrameTooltipDrawContext* ctx = (FrameTooltipDrawContext*)context;
    formatted_text_draw((const TextureSurface*)font, ctx->pixels, ctx->pitch,
                        ctx->width, ctx->height, (int)x, (int)y, text);
}

static void frame_tutorial_script_dispatch(float delta, void* context)
{
    (void)context;
    /* Frame seconds -> reference 25-Hz units -> milliseconds in 0x453bb0. */
    tutorial_frame_timer_array_advance(delta * 25.0f);
    tutorial_script_command_dispatch(delta);
}

/* 0x454600's first side effect belongs to the authored record pointer:
 * resolve the current OList slot each frame and mark the actual actor. The
 * indexed world-arrow draw still needs the renderer owner's projection API. */
static void frame_tutorial_message_owner_update(GameContext* game,
                                                 InputState* input)
{
    int active = game != NULL && game->level != NULL &&
                 !mission_reward_active(game);
    int changed = advisor_message_ui_gameplay_set(active);
    if (changed) input_pointer_rearm(input);
}

static void frame_tutorial_record_pointer_mark(void)
{
    uint32_t slot = tutorial_record_object_pointer_get();
    EntitySlot* entity = slot != 0u
        ? object_pointer_registry_entity_get(slot - 1u) : NULL;
    if (entity != NULL) entity->tutorial_unit_at_block_3f8 = 1u;
}

/* MainSim 0x426c1e..0x426ca0 rejects entity clicks for system bit 8 or
 * unmarked actors in a scripted mode, then checks the real type/id table.
 * Ground commands use 0x428780, including the authored voxel +4 marker.
 * This runs BEFORE both early input and EngineTick's late press consumer. */
static int frame_tutorial_world_pointer_admit(GameContext* game,
    EngineContext* engine, int x, int y, int width, int height)
{
    uint32_t flags = system_state_flags_get();
    EntitySlot* hit;
    float cell_x, cell_y;
    uint8_t* voxel;
    uint32_t clicks;
    if (game == NULL || game->level == NULL || engine == NULL ||
        mission_reward_active(game)) return 0;
    if ((engine->state & (0x100u | 0x100000u | 0x08000000u)) != 0u ||
        (engine->debug & 0x1000u) != 0u) return 0;
    hit = level_scene_pick_entity(game, game->level, width, height, x, y, 30.0f);
    if (hit != NULL) {
        if ((flags & 8u) != 0u ||
            (flags != 0u && hit->tutorial_unit_at_block_3f8 == 0u)) return 0;
        return cell_action_permission_get(hit->type, hit->etype) != 0 &&
            (hit->flags_3e4 & 2u) == 0u &&
            (hit->type != 1 || (hit->flags_3e0 & 0x40000u) == 0u) &&
            ((engine->state & 0x10000000u) == 0u ||
             (hit->flags_3ec & 1u) != 0u);
    }
    /* The real terrain-hit counter precedes action permission in 0x426520
     * (0x426f79..0x426f85). UI gestures never reach this producer. */
    if (level_scene_mouse_cell(game, game->level, width, height,
                              x, y, &cell_x, &cell_y)) {
        voxel = level_voxel(game->level, (int)cell_x, (int)cell_y);
        if (voxel != NULL) {
            memcpy(&clicks, voxel + 0x28, sizeof(clicks));
            ++clicks;
            memcpy(voxel + 0x28, &clicks, sizeof(clicks));
        }
    }
    if (!level_scene_mouse_cell(game, game->level, width, height,
                               x, y, &cell_x, &cell_y)) return 0;
    /* The floor owner tests route before command permission. Admission
     * records the genuine release hit without pre-empting that ordering. */
    return 1;
}

/* SimCursorInteractionTick (0x45adb0) zooms while the button stays held on
 * a captured CameraControl ZoomIn/ZoomOut item (panel 9, items 0/1), unless
 * system-state bit 0x1000 locks the camera. The pointer pass records the
 * held item; the frame applies it with the frame's scale (see below). */
static int s_frame_panel_zoom_item = -1;

static void frame_panel_zoom_hold_capture(int held,
                                          const FrontendPanelPointerHit* hit)
{
    s_frame_panel_zoom_item = -1;
    if (held && hit != NULL && hit->category == 9 &&
        (hit->item == 0 || hit->item == 1) &&
        (system_state_flags_get() & 0x1000u) == 0u)
        s_frame_panel_zoom_item = hit->item;
}

/* CameraZoomStepAdjust's step is the 25-Hz frame scale; the port's frame
 * delta is in seconds. Frozen time adds no zoom, as in MainSimStep. */
static void frame_panel_zoom_hold_apply(const EngineContext* engine, float dt)
{
    if (s_frame_panel_zoom_item < 0) return;
    if ((engine->state & ENGINE_STATE_TIME_FROZEN) == 0u)
        camera_zoom_step_adjust(s_frame_panel_zoom_item,
                                EngineTimeScale() * dt * 25.0f);
    s_frame_panel_zoom_item = -1;
}

/* UI arbitration owns a physical gesture through its release, even on a
 * miss or when the other button overlaps. World actions are admitted only
 * by selection_pointer_world_dispatch, once per admitted release sample. */
static void frame_panel_pointer_release(EngineContext* engine,
                                         int category, int item)
{
    (void)engine; /* TransitionControl uses the bound current EngineContext. */
    (void)frontend_panels_pointer_activate(category, item);
}

static void frame_pointer_ingress(GameContext* game, EngineContext* engine,
    InputState* input, SimulationPointerCapture captures[2],
    uint32_t* capture_serial, int width, int height)
{
    static AdvisorMessagePointerCapture messages[2];
    static FrontendPanelPointerCapture panel;
    static int logical, panel_owned[2];
    static uint32_t panel_serial;
    int i, b, enabled, panel_enabled;
    frame_tutorial_record_pointer_mark();
    if (*capture_serial != input->pointer_reset_serial ||
        panel_serial != input->pointer_reset_serial || mission_reward_active(game)) {
        frontend_panels_pointer_capture_cancel(&panel);
        logical = 0;
        memset(panel_owned, 0, sizeof(panel_owned));
        memset(messages, 0, sizeof(messages));
        for (b = 0; b < 2; ++b) {
            simulation_pointer_capture_reset(&captures[b]);
            input->pointer_domain[b] = INPUT_POINTER_NONE;
        }
        *capture_serial = input->pointer_reset_serial;
        panel_serial = input->pointer_reset_serial;
        if (mission_reward_active(game)) { input->pointer_count = 0; return; }
    }
    enabled = (engine->state & (0x100u | 0x100000u | 0x08000000u)) == 0u &&
              (engine->debug & 0x1000u) == 0u;
    panel_enabled = (engine->state & ENGINE_STATE_INDEXED_SUBSYS) != 0u;
    if (!enabled || !panel_enabled) {
        frontend_panels_pointer_capture_cancel(&panel);
        logical = 0;
        if (!enabled) memset(panel_owned, 0, sizeof(panel_owned));
    }
    if (!enabled) {
        memset(messages, 0, sizeof(messages));
        input_pointer_rearm(input);
        for (b = 0; b < 2; ++b) simulation_pointer_capture_reset(&captures[b]);
        *capture_serial = input->pointer_reset_serial;
        return;
    }
    for (i = 0; i < input->pointer_count; ++i) {
        InputPointerSample* sample = &input->pointer_samples[i];
        int previous = logical, hit_panel = 0, skip_panel = 0;
        FrontendPanelPointerHit hit;
        if (sample->transition == PLATFORM_MOUSE_CANCEL) {
            frontend_panels_pointer_capture_cancel(&panel);
            logical = 0; memset(panel_owned, 0, sizeof(panel_owned));
            memset(messages, 0, sizeof(messages));
            for (b = 0; b < 2; ++b) {
                input->pointer_domain[b] = INPUT_POINTER_NONE;
                simulation_pointer_capture_reset(&captures[b]);
            }
            continue;
        }
        b = sample->button == PLATFORM_MOUSE_LEFT ? 0 :
            sample->button == PLATFORM_MOUSE_RIGHT ? 1 : -1;

        if (b >= 0 && (sample->transition == PLATFORM_MOUSE_DOWN ||
                       sample->transition == PLATFORM_MOUSE_UP) &&
            ((s_engine_feature_word & 0x4000u) == 0u || b == 0))
            logical = sample->transition == PLATFORM_MOUSE_DOWN;
        if (sample->domain == INPUT_POINTER_BLOCKED ||
            (b >= 0 && input->pointer_domain[b] == INPUT_POINTER_BLOCKED)) {
            frontend_panels_pointer_capture_cancel(&panel);
            logical = 0; skip_panel = 1;
        }
        /* Other UI owns its own release; no panel action through that UI. */
        if (b >= 0 && input->pointer_domain[b] == INPUT_POINTER_UI &&
            !panel_owned[b]) skip_panel = 1;
        if (b >= 0 && sample->transition == PLATFORM_MOUSE_DOWN) {
            int other = 1 - b;
            if (input->pointer_domain[other] == INPUT_POINTER_UI &&
                !panel_owned[other]) skip_panel = 1;
        }
        if (panel_enabled && !skip_panel) {
            hit_panel = frontend_panels_pointer_update(&panel, sample->x,
                sample->y, logical, previous, &hit);
            if (hit_panel) engine->state |= 0x200u;
            if (hit.released) {
                frame_panel_pointer_release(engine, hit.category, hit.item);
                if ((engine->state & (0x100u | 0x100000u | 0x08000000u)) != 0u ||
                    (engine->debug & 0x1000u) != 0u) {
                    frontend_panels_pointer_capture_cancel(&panel);
                    logical = 0;
                    memset(panel_owned, 0, sizeof(panel_owned));
                    memset(messages, 0, sizeof(messages));
                    input_pointer_rearm(input);
                    for (b = 0; b < 2; ++b)
                        simulation_pointer_capture_reset(&captures[b]);
                    *capture_serial = input->pointer_reset_serial;
                    panel_serial = input->pointer_reset_serial;
                    return;
                }
            }
        }
        if (b >= 0 && sample->transition == PLATFORM_MOUSE_DOWN) {
            int consumed = 0;
            simulation_pointer_capture_reset(&captures[b]);
            memset(&messages[b], 0, sizeof(messages[b]));
            panel_owned[b] = 0;
            if (sample->domain == INPUT_POINTER_BLOCKED) {
                input->pointer_domain[b] = INPUT_POINTER_BLOCKED;
            } else {
                if (hit_panel) { consumed = 1; panel_owned[b] = 1; }
                if (!consumed) consumed = simulation_pointer_capture_dispatch(
                    &captures[b], sample->x, sample->y, 1, 0, engine->views[0]);
                if (!consumed) consumed = advisor_message_pointer_capture_dispatch(
                    &messages[b], sample->x, sample->y, 1);
                else if (!panel_owned[b]) messages[b].down = 1;
                input->pointer_domain[b] = consumed ? INPUT_POINTER_UI : INPUT_POINTER_WORLD;
            }
            sample->domain = input->pointer_domain[b];
        } else if (b >= 0 && sample->transition == PLATFORM_MOUSE_UP) {
            int domain = input->pointer_domain[b];
            sample->domain = domain;
            if (domain == INPUT_POINTER_UI && !panel_owned[b]) {
                if (messages[b].owner != 0)
                    (void)advisor_message_pointer_capture_dispatch(&messages[b], sample->x, sample->y, 0);
                else (void)simulation_pointer_capture_dispatch(&captures[b], sample->x, sample->y, 0, 1, engine->views[0]);
            } else if (domain == INPUT_POINTER_WORLD && hit_panel) {
                sample->admitted = 1;
                sample->domain = INPUT_POINTER_UI;
            } else if (domain == INPUT_POINTER_WORLD && !sample->admitted) {
                if (((s_engine_feature_word & 0x4000u) == 0u || b == 0) &&
                    frame_tutorial_world_pointer_admit(game, engine, sample->x, sample->y, width, height))
                    (void)selection_pointer_world_dispatch(game, engine, sample, width, height);
                else sample->admitted = 1;
            }
            if ((s_engine_feature_word & 0x4000u) == 0u) {
                int other = 1 - b;
                if (input->pointer_domain[other] == INPUT_POINTER_WORLD)
                    input->pointer_domain[other] = INPUT_POINTER_NONE;
            }
            panel_owned[b] = 0; memset(&messages[b], 0, sizeof(messages[b]));
            simulation_pointer_capture_reset(&captures[b]);
            input->pointer_domain[b] = INPUT_POINTER_NONE;
        } else {
            for (b = 0; b < 2; ++b) {
                int button = b == 0 ? PLATFORM_MOUSE_LEFT : PLATFORM_MOUSE_RIGHT;
                if ((sample->held & button) && input->pointer_domain[b] == INPUT_POINTER_UI && !panel_owned[b]) {
                    if (messages[b].owner) (void)advisor_message_pointer_capture_dispatch(&messages[b], sample->x, sample->y, 1);
                    else (void)simulation_pointer_capture_dispatch(&captures[b], sample->x, sample->y, 1, 0, engine->views[0]);
                }
            }
        }
    }
    /* Preserve independent non-panel UI hold processing from the real caller. */
    for (b = 0; b < 2; ++b) {
        int button = b == 0 ? PLATFORM_MOUSE_LEFT : PLATFORM_MOUSE_RIGHT;
        if ((input->pointer_buttons & button) && input->pointer_domain[b] == INPUT_POINTER_UI && !panel_owned[b]) {
            if (messages[b].owner) (void)advisor_message_pointer_capture_dispatch(&messages[b], input->mouse_x, input->mouse_y, 1);
            else (void)simulation_pointer_capture_dispatch(&captures[b], input->mouse_x, input->mouse_y, 1, 0, engine->views[0]);
        }
    }
    if (!panel_enabled && input->pointer_buttons == 0) {
        SimulationPointerCapture hover;
        AdvisorMessagePointerCapture message_hover;
        simulation_pointer_capture_reset(&hover);
        if (!simulation_pointer_capture_dispatch(&hover, input->mouse_x, input->mouse_y, 0, 0, engine->views[0])) {
            memset(&message_hover, 0, sizeof(message_hover));
            (void)advisor_message_pointer_capture_dispatch(&message_hover, input->mouse_x, input->mouse_y, 0);
        }
    }
    if (panel_enabled) {
        FrontendPanelPointerHit hit;
        int panel_hit = frontend_panels_pointer_update(&panel, input->mouse_x,
            input->mouse_y, logical, logical, &hit);
        if (panel_hit) engine->state |= 0x200u;
        frame_panel_zoom_hold_capture(logical, &hit);
        if (!panel_hit && !logical && input->pointer_buttons == 0) {
            SimulationPointerCapture hover;
            AdvisorMessagePointerCapture message_hover;
            simulation_pointer_capture_reset(&hover);
            if (!simulation_pointer_capture_dispatch(&hover, input->mouse_x, input->mouse_y, 0, 0, engine->views[0])) {
                memset(&message_hover, 0, sizeof(message_hover));
                (void)advisor_message_pointer_capture_dispatch(&message_hover, input->mouse_x, input->mouse_y, 0);
            }
        }
    }
}

void main_loop_frame_pump_hooks_set(const MainLoopFramePumpHooks* hooks)
{
    if (hooks == NULL) {
        memset(&s_main_loop_frame_pump_hooks, 0,
               sizeof(s_main_loop_frame_pump_hooks));
    }
    else {
        s_main_loop_frame_pump_hooks = *hooks;
    }
}

/* Win32MessagePumpDispatch (FUN_00478040): remove and dispatch every pending
 * message accepted by the fullscreen/windowed filter. */
// FUNCTION: LEGORR 0x00478040
static void window_message_dispatch_pump(void)
{
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
    MSG message;
    while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE)) {
        /* WM_QUIT is a thread message: DispatchMessage cannot latch it. */
        if (message.message == 0x12u) {
            s_window_close_requested = 1u;
            continue;
        }
        if (window_runtime_message_dispatch_filter(&message)) {
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }
    }
#endif /* native backends pump messages in src/platform */
    (void)frameloop_application_quit_ingest(&s_frameloop_application, NULL, 0);
}

/* MainLoopFramePump (FUN_00477f30): drain Win32 messages, poll the active
 * input state, update the cursor, begin/present one renderer frame, submit
 * the optional projection overlay, then clear the once-per-frame feature bit. */
// FUNCTION: LEGORR 0x00477f30
void main_loop_frame_pump(int draw_projection_overlay)
{
    MainLoopFramePumpHooks* const hooks = &s_main_loop_frame_pump_hooks;
    window_message_dispatch_pump();
    input_state_poll_retail();
    input_cursor_position_update();
    renderer_begin_frame_once_retail();
#if defined(LEGORRR_BACKEND_WINAPI)
    render_surface_present();
#else
    if (hooks->renderer_present != NULL)
        hooks->renderer_present(hooks->context);
#endif
    if (draw_projection_overlay != 0 && hooks->projection_overlay != NULL)
        hooks->projection_overlay(hooks->context, 0, 0);
    s_engine_feature_word &= ~1u;
}

void window_runtime_ops_set(const WindowRuntimeOps* ops)
{
    if (ops == NULL) memset(&s_window_runtime_ops, 0,
                            sizeof(s_window_runtime_ops));
    else s_window_runtime_ops = *ops;
}

void window_runtime_ops_install_defaults(const WindowRuntimeOps* ops)
{
    if (ops == NULL) return;
    if (s_window_runtime_ops.device_caps_query == NULL)
        s_window_runtime_ops.device_caps_query = ops->device_caps_query;
    if (s_window_runtime_ops.adjust_window_rect == NULL)
        s_window_runtime_ops.adjust_window_rect = ops->adjust_window_rect;
    if (s_window_runtime_ops.mouse_capture == NULL)
        s_window_runtime_ops.mouse_capture = ops->mouse_capture;
    if (s_window_runtime_ops.mouse_release == NULL)
        s_window_runtime_ops.mouse_release = ops->mouse_release;
}

void window_runtime_default_proc_set(WindowDefaultProc proc, void* context)
{
    s_window_default_proc = proc;
    s_window_default_proc_context = context;
}

void window_runtime_window_handle_set(void* window)
{
    s_window_runtime_state.window = window;
}

void window_runtime_mode_set(int fullscreen, int32_t style)
{
    if (fullscreen != 0) s_window_runtime_state.flags |= 2u;
    else s_window_runtime_state.flags &= ~2u;
    if (fullscreen != 0) s_engine_feature_word |= 2u;
    else s_engine_feature_word &= ~2u;
    s_window_runtime_state.window_style = style;
    s_window_runtime_style = style;
}

void window_runtime_flags_set(uint32_t flags)
{
    s_window_runtime_state.flags = flags;
}

uint32_t window_runtime_flags_get(void)
{
    return s_window_runtime_state.flags;
}

const WindowRuntimeState* window_runtime_state_get(void)
{
    return &s_window_runtime_state;
}

void window_runtime_state_initialize(void* window)
{
    memset(&s_window_runtime_state, 0, sizeof(s_window_runtime_state));
    display_dimensions_set(g_platform_cursor_width, g_platform_cursor_height);
    s_window_application_active = 0u;
    s_window_close_requested = 0u;
    g_renderer_word_c = 0u;
    s_window_mouse_word_40 = 0u;
    s_window_mouse_word_4c = 0u;
    s_window_mouse_word_50 = 0u;
    s_window_mouse_word_54 = 0u;
    s_window_mouse_word_58 = 0u;
    s_window_runtime_state.window = window;
    window_state_initialize_retail(window);
}

// FUNCTION: LEGORR 0x00477f80
void window_runtime_display_caps_query(void)
{
    HDC dc = GetDC((HWND)g_platform_cursor_window_handle);
    (void)GetDeviceCaps(dc, 12);
}

// FUNCTION: LEGORR 0x00477fd0
int window_runtime_callback_triplet_set(const size_t* triplet)
{
    if (triplet != NULL) {
        s_window_runtime_state.callback_triplet[0] = triplet[0];
        s_window_runtime_state.callback_triplet[1] = triplet[1];
        s_window_runtime_state.callback_triplet[2] = triplet[2];
        s_window_runtime_state.callback_triplet_valid = 1;
        return 1;
    }
    return 0;
}

int window_runtime_callback_triplet_get(size_t triplet[3])
{
    if (triplet == NULL ||
        s_window_runtime_state.callback_triplet_valid == 0u) return 0;
    triplet[0] = s_window_runtime_state.callback_triplet[0];
    triplet[1] = s_window_runtime_state.callback_triplet[1];
    triplet[2] = s_window_runtime_state.callback_triplet[2];
    return 1;
}

// FUNCTION: LEGORR 0x00478010
int window_runtime_message_dispatch_filter(const void* message)
{
    const uint32_t* message_words = (const uint32_t*)message;
    if ((s_window_runtime_state.flags & 2u) != 0u &&
        (message_words[1] == 0x1cu || message_words[1] == 0x104u ||
         message_words[1] == 0x105u)) {
        return 0;
    }
    return 1;
}

// FUNCTION: LEGORR 0x00478310
void window_runtime_adjust_rect(WindowPortRect* rect)
{
    if ((s_engine_feature_word & 2u) == 0u) {
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
        (void)AdjustWindowRect((LPRECT)rect, (DWORD)s_window_runtime_style, 0);
#else
        if (s_window_runtime_ops.adjust_window_rect != NULL) {
            (void)s_window_runtime_ops.adjust_window_rect(
                s_window_runtime_ops.context, rect,
                (int32_t)s_window_runtime_style, 0);
        }
#endif
    }
}

static ptrdiff_t window_runtime_default(void* window, uint32_t message,
                                        size_t wparam, ptrdiff_t lparam)
{
    if (s_window_default_proc != NULL) {
        return s_window_default_proc(s_window_default_proc_context, window,
                                     message, wparam, lparam);
    }
    return 0;
}

static void window_runtime_capture(void* window)
{
    if (s_window_runtime_ops.mouse_capture != NULL) {
        s_window_runtime_ops.mouse_capture(s_window_runtime_ops.context,
                                           window);
    }
    s_window_runtime_state.mouse_capture = 1;
}

static void window_runtime_release_capture(void)
{
    if (s_window_runtime_ops.mouse_release != NULL) {
        s_window_runtime_ops.mouse_release(s_window_runtime_ops.context);
    }
    s_window_runtime_state.mouse_capture = 0;
}

/* GameWindowProcedure (FUN_004784c0): fullscreen message filtering and
 * mouse-event globals follow the decompiled WNDPROC path. */
// FUNCTION: LEGORR 0x004784c0
static ptrdiff_t LEGORRR_WINDOW_PROC_CALL window_game_window_procedure_retail(
    void* window, uint32_t message, size_t wparam, ptrdiff_t lparam)
{
    if (message <= 0x10u) {
        if (message != 0x10u) {
            switch (message) {
            default:
                goto default_proc;
            case 2u:
                break;
            case 3u:
            case 5u:
            case 6u:
                return 0;
            }
        }
    }
    else {
        if (message > 0x1cu) {
            if (message < 0x87u) {
                if (message == 0x86u) return 0;
                if (message == 0x46u) {
                    uint32_t* flags = (uint32_t*)((char*)lparam + 24);
                    *flags &= ~0x200u;
                    return 0;
                }
            }
            else if (message < 0x105u) {
                if (message == 0x104u) return 0;
                if (message > 0xffu && message < 0x102u) return 0;
            }
            else {
                switch (message) {
                case 0x200u:
                case 0x211u:
                case 0x212u:
                    return 0;
                case 0x201u:
                case 0x202u:
                case 0x204u:
                case 0x205u:
                    if ((s_engine_feature_word & 0x4000u) == 0u) {
                        switch (message) {
                        case 0x201u:
                        case 0x204u:
                            if (g_renderer_word_c == 0u) {
                                s_window_mouse_word_4c = 1u;
                                s_window_mouse_word_50 = 1u;
                            }
                            s_window_mouse_word_40 = 1u;
                            g_renderer_word_c = 1u;
                            s_window_mouse_word_54 = 1u;
                            return 0;
                        case 0x202u:
                        case 0x205u:
                            s_window_mouse_word_4c = 0u;
                            s_window_mouse_word_50 = 0u;
                            s_window_mouse_word_40 = 0u;
                            g_renderer_word_c = 0u;
                            s_window_mouse_word_54 = 1u;
                            return 0;
                        }
                    }
                    else {
                        switch (message) {
                        case 0x201u:
                            s_window_mouse_word_4c = 1u;
                            g_renderer_word_c = 1u;
                            s_window_mouse_word_54 = 1u;
                            return 0;
                        case 0x202u:
                            s_window_mouse_word_4c = 0u;
                            g_renderer_word_c = 0u;
                            return 0;
                        case 0x204u:
                            s_window_mouse_word_50 = 1u;
                            s_window_mouse_word_40 = 1u;
                            return 0;
                        case 0x205u:
                            s_window_mouse_word_50 = 0u;
                            s_window_mouse_word_40 = 0u;
                            return 0;
                        }
                    }
                    break;
                case 0x203u:
                    s_window_mouse_word_54 = 1u;
                    return 0;
                case 0x206u:
                    s_window_mouse_word_58 = 1u;
                    return 0;
                }
            }
            goto default_proc;
        }
        if (message == 0x1cu) {
            s_window_application_active = (uint32_t)wparam;
            return 0;
        }
        if (message != 0x12u) goto default_proc;
    }
    s_window_close_requested = 1u;
default_proc:
#if defined(LEGORRR_BACKEND_WINAPI)
    return (ptrdiff_t)DefWindowProcA((HWND)window, (UINT)message,
                                     (WPARAM)wparam, (LPARAM)lparam);
#else
    return window_runtime_default(window, message, wparam, lparam);
#endif
}

/* MainWindowActivationAndCloseHandler (FUN_004786c0): windowed message
 * handling adds capture on the primary-button press and release. */
// FUNCTION: LEGORR 0x004786c0
static ptrdiff_t LEGORRR_WINDOW_PROC_CALL window_activation_close_retail(
    void* window, uint32_t message, size_t wparam, ptrdiff_t lparam)
{
    uint32_t button;
    uint32_t key;

    if (message < 7u) {
        key = message;
        if (message == 6u) return 0;
    }
    else {
        if (message > 0x1cu) {
            button = message - 0x201u;
            switch (button) {
            case 0u:
            case 1u:
            case 3u:
            case 4u:
                if ((s_engine_feature_word & 0x4000u) == 0u) {
                    switch (button) {
                    case 0u:
                    case 3u:
#if defined(LEGORRR_BACKEND_WINAPI)
                        SetCapture((HWND)window);
#else
                        window_runtime_capture(window);
#endif
                        if (g_renderer_word_c == 0u) {
                            s_window_mouse_word_4c = 1u;
                            s_window_mouse_word_50 = 1u;
                        }
                        s_window_mouse_word_40 = 1u;
                        g_renderer_word_c = 1u;
                        s_window_mouse_word_54 = 1u;
                        return 0;
                    case 1u:
                    case 4u:
#if defined(LEGORRR_BACKEND_WINAPI)
                        ReleaseCapture();
#else
                        window_runtime_release_capture();
#endif
                        s_window_mouse_word_4c = 0u;
                        s_window_mouse_word_50 = 0u;
                        s_window_mouse_word_40 = 0u;
                        g_renderer_word_c = 0u;
                        break;
                    }
                }
                else {
                    switch (button) {
                    case 0u:
#if defined(LEGORRR_BACKEND_WINAPI)
                        SetCapture((HWND)window);
#else
                        window_runtime_capture(window);
#endif
                        s_window_mouse_word_4c = 1u;
                        g_renderer_word_c = 1u;
                        s_window_mouse_word_54 = 1u;
                        return 0;
                    case 1u:
#if defined(LEGORRR_BACKEND_WINAPI)
                        ReleaseCapture();
#else
                        window_runtime_release_capture();
#endif
                        s_window_mouse_word_54 = 1u;
                        s_window_mouse_word_4c = 0u;
                        g_renderer_word_c = 0u;
                        return 0;
                    case 3u:
                        s_window_mouse_word_50 = 1u;
                        s_window_mouse_word_40 = 1u;
                        s_window_mouse_word_54 = 1u;
                        return 0;
                    case 4u:
                        s_window_mouse_word_54 = 1u;
                        s_window_mouse_word_50 = 0u;
                        s_window_mouse_word_40 = 0u;
                        return 0;
                    }
                }
                /* Retail falls through to the shared click-state cases. */
            case 2u:
                s_window_mouse_word_54 = 1u;
                return 0;
            case 5u:
                s_window_mouse_word_58 = 1u;
                return 0;
            }
            goto default_proc;
        }
        if (message == 0x1cu) {
            s_window_application_active = (uint32_t)wparam;
            return 0;
        }
        key = message - 0x10u;
        if (key == 0u) {
            s_window_close_requested = 1u;
            return 0;
        }
    }
    if (key == 2u) {
        s_window_close_requested = 1u;
        return 0;
    }
default_proc:
#if defined(LEGORRR_BACKEND_WINAPI)
    return (ptrdiff_t)DefWindowProcA((HWND)window, (UINT)message,
                                     (WPARAM)wparam, (LPARAM)lparam);
#else
    return window_runtime_default(window, message, wparam, lparam);
#endif
}

static void window_runtime_state_sync_from_globals(uint32_t message,
                                                   int capture_enabled)
{
    WindowRuntimeState* state = &s_window_runtime_state;
    int dual_mouse = (s_engine_feature_word & 0x4000u) != 0u;
    state->close_requested = s_window_close_requested;
    state->app_active = s_window_application_active;
    state->mouse_word_3c = g_renderer_word_c;
    state->mouse_word_40 = s_window_mouse_word_40;
    state->mouse_word_4c = s_window_mouse_word_4c;
    state->mouse_word_50 = s_window_mouse_word_50;
    state->mouse_word_54 = s_window_mouse_word_54;
    state->mouse_word_58 = s_window_mouse_word_58;
    if (message >= 0x201u && message <= 0x202u) {
        int down = message == 0x201u;
        state->left_button_down = down;
        if (!dual_mouse) state->right_button_down = down;
        state->mouse_activity = 1u;
        if (capture_enabled) state->mouse_capture = down != 0;
    }
    else if (message >= 0x204u && message <= 0x205u) {
        int down = message == 0x204u;
        if (dual_mouse) state->right_button_down = down;
        else {
            state->left_button_down = down;
            state->right_button_down = down;
        }
        state->mouse_activity = 1u;
        if (capture_enabled) state->mouse_capture = down != 0;
    }
    else if (message == 0x203u) {
        state->left_double_click = 1u;
        state->mouse_activity = 1u;
    }
    else if (message == 0x206u) {
        state->right_double_click = 1u;
        state->mouse_activity = 1u;
    }
}

ptrdiff_t window_runtime_game_window_proc(void* window, uint32_t message,
                                          size_t wparam, ptrdiff_t lparam)
{
    ptrdiff_t result = window_game_window_procedure_retail(
        window, message, wparam, lparam);
    window_runtime_state_sync_from_globals(message, 0);
    return result;
}

ptrdiff_t window_runtime_activation_close_proc(void* window, uint32_t message,
                                               size_t wparam,
                                               ptrdiff_t lparam)
{
    ptrdiff_t result = window_activation_close_retail(
        window, message, wparam, lparam);
    window_runtime_state_sync_from_globals(message, 1);
    return result;
}

/* WindowTitleSet (FUN_004783d0): forwards the title to the active platform
 * window; the native and WinAPI adapters both update their owned window. */
// FUNCTION: LEGORR 0x004783d0
void window_title_set(const char* title)
{
#if defined(LEGORRR_BACKEND_WINAPI)
    SetWindowTextA((HWND)g_platform_cursor_window_handle, title);
#else
    platform_set_window_title(title);
#endif
}
static void* frame_camera_target_indexed_scene_resolve(
    void* source, uint32_t target_id, uint32_t* component_present)
{
    EntitySlot* entity = (EntitySlot*)source;
    EntityActivityRunnerSidecar* runner;
    void* scene_object;
    if (component_present != NULL) *component_present = 0u;
    if (entity == NULL) return NULL;
    if (entity->type == 1) {
        return entity_type1_scene_resource_indexed_get(
            entity, target_id, component_present);
    }
    if ((entity->type != 2 && entity->type != 3) ||
        !entity_activity_runner_type_config_bind(entity)) return NULL;
    runner = entity->type == 2 ? &entity->activity_runner_type2
                               : &entity->activity_runner_type3;
    if (component_present != NULL)
        *component_present = runner->component_gate_value == 1u ? 1u : 0u;
    scene_object = entity_activity_runner_indexed_get(entity, entity->type,
                                                       target_id);
    return scene_object;
}

static void frame_camera_target_registry_update(EngineContext* engine,
                                               GameContext* game)
{
    EngineCameraTargetRecord previous[10];
    uint32_t count = 0u;
    uint32_t i;
    if (engine == NULL) return;
    memcpy(previous, engine->camera_targets, sizeof(previous));
    memset(engine->camera_targets, 0, sizeof(engine->camera_targets));
    if (game != NULL && game->level != NULL)
        count = object_pointer_registry_count_get();
    if (count > 10u) count = 10u;
    for (i = 0u; i < count; ++i) {
        EntitySlot* entity = object_pointer_registry_entity_get(i);
        EngineCameraTargetRecord* target = &engine->camera_targets[i];
        float xy[2];
        float angle;
        if (entity == NULL) continue;
        if (entity->type < 1 || entity->type > 3) continue;
        target->type = (uint32_t)entity->type;
        target->source[entity->type - 1] = entity;
        if (entity->type == 2 || entity->type == 3) {
            (void)entity_activity_runner_type_config_bind(entity);
            target->indexed_scene_resolve =
                frame_camera_target_indexed_scene_resolve;
            target->cache_id = previous[i].type == target->type &&
                               previous[i].source[entity->type - 1] == entity
                ? previous[i].cache_id : -1;
            continue;
        }
        if (entity_type1_scene_resource_bind(entity)) {
            target->indexed_scene_resolve =
                frame_camera_target_indexed_scene_resolve;
        }
        target->scene_cache = entity->attached_scene_object;
        target->cache_id = previous[i].type == target->type &&
                           previous[i].source[entity->type - 1] == entity &&
                           previous[i].scene_cache == target->scene_cache
            ? previous[i].cache_id : -1;
        entity_transform_position_get(entity, xy);
        target->position[0] = xy[0];
        target->position[1] = xy[1];
        target->position[2] = entity->z;
        if (target->scene_cache != NULL) {
            float scene_position[3];
            if (ddiscene_object_position_get(target->scene_cache, NULL,
                                             scene_position)) {
                memcpy(target->position, scene_position,
                       sizeof(scene_position));
            }
        }
        angle = entity->rot;
        target->facing[0] = cosf(angle);
        target->facing[1] = sinf(angle);
        target->facing[2] = 0.0f;
        if (target->scene_cache != NULL) {
            float scene_facing[3];
            if (ddiscene_object_facing_get(target->scene_cache, NULL,
                                           scene_facing)) {
                memcpy(target->facing, scene_facing, sizeof(scene_facing));
            }
        }
        target->component_present = 0u;
    }
    engine->camera_target_count = count;
}
#include "game/util.hpp"
#include "frameloop_for_frontend_unit.hpp"
#include "frameloop_from_frontend_unit.hpp"

static const char* env_get(const char* name, const char* fallback)
{
    const char* v = getenv(name);
    return (v != NULL && v[0] != '\0') ? v : fallback;
}

static int frameloop_resource_startup_text_lookup(
    void* context, const char* registry_key, const char* text_key,
    char* text_out, size_t text_capacity)
{
    (void)context;
#if defined(LEGORRR_BACKEND_WINAPI)
    if (text_out == NULL || text_capacity == 0u ||
        text_capacity > 0xffffffffu) return 0;
    return registry_path_accessor(registry_key, text_key, 0, text_out,
                                  (uint32_t)text_capacity);
#else
    (void)registry_key;
    (void)text_key;
    (void)text_out;
    (void)text_capacity;
    return 0;
#endif
}

static int frameloop_resource_startup_message_box(
    void* context, const char* text, const char* caption, unsigned style)
{
    (void)context;
#if defined(LEGORRR_BACKEND_WINAPI)
    return (int)MessageBoxA(NULL, text, caption, (UINT)style);
#else
    (void)style;
    fprintf(stderr, "%s: %s\n", caption != NULL ? caption : "",
            text != NULL ? text : "");
    return 1;
#endif
}

static void frameloop_resource_startup_services_install(void)
{
    ResourceStartupOps ops;
    memset(&ops, 0, sizeof(ops));
    ops.text_lookup = frameloop_resource_startup_text_lookup;
    ops.message_box = frameloop_resource_startup_message_box;
    /* The retail routine exits on cancel/missing data. The portable loop
     * reports its status to the caller instead of terminating the process. */
    resource_startup_ops_set(&ops);
}

int s_icontains(const char* haystack, const char* needle)
{
    if (haystack == NULL || needle == NULL) {
        return 0;
    }
    if (haystack[0] == '\0') {
        return 0;
    }
    if (needle[0] == '\0') {
        return 1;
    }
    return case_insensitive_substring_find(haystack, needle) != NULL;
}

static void frameloop_game_context_dispose(GameContext* game)
{
    if (game == NULL) {
        return;
    }
    selection_active_context_bind(NULL);
    checkpoint_blob_free(game);
    mission_reward_resources_release(game);
    free(game->records);
    tutorial_runtime_context_set(NULL, NULL, NULL);
    engine_tutorial_dispatch_hook_set(NULL, NULL);
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
    engine_world_arrow_capture_hook_set(NULL, NULL);
    frame_world_arrows_level_releasing();
    tutorial_world_arrows_release(&s_frame_world_arrows);
    advisor_tutorial_icon_image_release();
#endif
    (void)tutorial_display_resources_release();
    advisor_message_ui_release();
    frontend_panels_release();
    level_destroy(game->level);
    if (game->cfg) block_close(game->cfg);
}

/* Streamed soundtrack aliases live beside the menu SFX aliases in Samples.
 * Their leading '@' marks a streamed WAVE in Lego.cfg, not part of its path. */
static const char* menu_music_resource(BlockFile* cfg)
{
    const char* music = block_get_strptr(cfg,
                                         "Lego*::Samples::SFX_AmbientMusicLoop");
    if (music != NULL && music[0] == '@') {
        ++music;
    }
    return music;
}

/* Held-key probe for the executor's cheat-keybind pre-pass (finding 06):
 * reads the backend's current keyboard snapshot, not the frame-edge table. */
static int demo_keyheld(int key)
{
    return input_keyboard_key_held(key);
}

/* Message-bus demo hooks (finding 22 §1): incidents dispatch a per-id
 * sound configured from the InfoMessages config section (message.c), else
 * a stand-in wav. */
static AudioPlayer* s_demo_ap = NULL;
static void demo_msg_sound(int id, float vol)
{
    (void)vol;
    if (s_demo_ap == NULL || s_demo_ap->dev == NULL) {
        return;
    }
    const char* snd = message_sound(id);
    if (snd != NULL && snd[0] != '\0') {
        audio_play_named(s_demo_ap, snd);
    } else {
        audio_play_named(s_demo_ap, "Sounds\\Buildings\\elec5.wav");
    }
}

typedef struct FrameloopOverlayMenuContext {
    FrontEnd* fe;
    GameContext* game;
    FeOverlayMenuGateState* gate;
    const FeInput* input;
    const TextureSurface* measure;
    FeEvent event;
    int mode;
} FrameloopOverlayMenuContext;

static void frameloop_overlay_menu_scroll_refresh(void* context)
{
    FrameloopOverlayMenuContext* gate_ctx =
        (FrameloopOverlayMenuContext*)context;
    scroll_slider_refresh(gate_ctx != NULL ? gate_ctx->game : NULL);
}

static int frameloop_overlay_menu_tick(void* context, int active_item, int mode)
{
    FrameloopOverlayMenuContext* gate_ctx =
        (FrameloopOverlayMenuContext*)context;
    (void)active_item;
    (void)mode;
    if (gate_ctx == NULL || gate_ctx->fe == NULL || gate_ctx->input == NULL) {
        return 0;
    }
    gate_ctx->event = frontend_tick(gate_ctx->fe, gate_ctx->input,
                                    gate_ctx->measure);
    if (gate_ctx->event != FE_EVT_NONE && gate_ctx->gate != NULL) {
        FeOverlayMenuSet* menu = &gate_ctx->gate->menus[gate_ctx->mode];
        const int slot = gate_ctx->mode == 0 && gate_ctx->fe->sel == 2 ? 2 : 0;
        menu->entries[slot].action_state = 1;
    }
    return gate_ctx->fe->open ? gate_ctx->fe->sel + 1 : 0;
}
static int demo_msg_cb(int id)
{
    (void)id; /* the bus appends it to the active queue */
    return 0; /* unhandled: dedupe callback does not consume */
}

/* Monotonic wall clock (ms) — the shell's frame pacing clock (the corpus
 * uses timeGetTime; the port is libc-portable). */
int64_t now_ms(void)
{
#if defined(LEGORRR_BACKEND_WINAPI)
    return (int64_t)GetTickCount();
#else
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
#endif
}

/* Play one AVI through the software decoder (src/gods98/movie.*) through
 * the rect-aware pump entry (movie_video_playback_loop, the port
 * replacement for DirectShowVideoPlaybackLoop @ 0x42efc0 — the level-load
 * and objective-tick video paths shape through this same seam): decode ->
 * scale-fill (full window for rect == NULL, into rect otherwise) ->
 * present, feed the movie's audio track to the seam, paced at the movie's
 * fps. Modal like the corpus's MoviePlayLoop @ 0x4157c0 — steps until done
 * or a skip key (ESC/Space/Enter/click), and propagates a window quit.
 * Returns 1 when a movie was played (or skipped after opening), 0 when it
 * could not be opened. */
typedef struct MoviePumpState {
    Movie*             mv;
    MovieInfo          mi;
    const MovieRect*   rect;      /* NULL = full-window branch */
    PlatformWindow*    win;
    DisplaySurface*    surf;
    AudioPlayer*       ap;
    int*               quit;
    int                skipped;
    int64_t            next_t;
    int                frame_ms;
    int                frames;
} MoviePumpState;

static int s_movie_frame_next(MovieOut* out, void* user)
{
    MoviePumpState* st = (MoviePumpState*)user;
    int r = movie_next(st->mv, out);
    return r == 0 ? 0 : 1;   /* EOF or error ends the branch */
}

static int s_movie_frame_present(const MovieOut* out, const MovieInfo* mi,
                                const MovieRect* rect, void* user)
{
    MoviePumpState* st = (MoviePumpState*)user;
    if (st->ap->dev != NULL && out->audio_frames > 0 && out->audio_rate > 0) {
        platform_audio_play_stereo(st->ap->dev, out->audio, out->audio_frames,
                                   out->audio_rate);
    }
    void* pixels = NULL;
    int   pitch = 0;
    if (surface_lock(st->surf, &pixels, &pitch) == 0) {
        if (rect != NULL) {
            movie_scale_rgba_rect(out->rgba, mi->width, mi->height,
                                  out->rgba_pitch, (uint8_t*)pixels,
                                  WINDOW_W, WINDOW_H, pitch, rect);
        } else {
            movie_scale_rgba(out->rgba, mi->width, mi->height, out->rgba_pitch,
                             (uint8_t*)pixels, WINDOW_W, WINDOW_H, pitch);
        }
        platform_screenshot(st->win, pixels, pitch, WINDOW_W, WINDOW_H,
                            (int)st->frames);
        platform_present_pixels(st->win, pixels, pitch, WINDOW_W, WINDOW_H);
        surface_unlock(st->surf, pixels);
    }
    st->next_t += st->frame_ms;
    int64_t now = now_ms();
    if (st->next_t > now) {
        Sleep((DWORD)(st->next_t - now));
    } else if (now - st->next_t > 2 * st->frame_ms) {
        st->next_t = now;    /* fell behind — resync instead of bursting */
    }
    ++st->frames;
    return 0;
}

static int s_movie_frame_quit(void* user)
{
    MoviePumpState* st = (MoviePumpState*)user;
    PlatformEvent evb[16];
    int nev = platform_pump_events(st->win, evb, 16);
    (void)frameloop_application_events_track(&s_frameloop_application, evb, nev);
    /* Quit wins even when a skip precedes it in this removed batch. */
    if (s_frameloop_application.quit) {
        *st->quit = 1;
        return 1;
    }
    for (int i = 0; i < nev; ++i) {
        if (evb[i].kind == PLATFORM_EVENT_QUIT) {
            *st->quit = 1;
            return 1;
        }
        if (evb[i].kind == PLATFORM_EVENT_KEYDOWN
            && (evb[i].code == PLATFORM_KEY_ESC
                || evb[i].code == ' ' || evb[i].code == PLATFORM_KEY_ENTER)) {
            st->skipped = 1;
            return 1;
        }
        if (evb[i].kind == PLATFORM_EVENT_MOUSE && evb[i].button == 1) {
            st->skipped = 1;
            return 1;
        }
    }
    return 0;
}

static int movie_play_avi_rect(const char* os_path, const char* label,
                               PlatformWindow* win, DisplaySurface* surf,
                               AudioPlayer* ap, int* quit,
                               const MovieRect* rect, FILE* log);

static int movie_play_avi(const char* os_path, const char* label,
                          PlatformWindow* win, DisplaySurface* surf,
                          AudioPlayer* ap, int* quit, FILE* log)
{
    return movie_play_avi_rect(os_path, label, win, surf, ap, quit, NULL, log);
}

static int movie_play_avi_rect(const char* os_path, const char* label,
                               PlatformWindow* win, DisplaySurface* surf,
                               AudioPlayer* ap, int* quit,
                               const MovieRect* rect, FILE* log)
{
    Movie* mv = movie_open_path(os_path);
    if (mv == NULL) {
        fprintf(log, "movie: %s: cannot open/decode %s\n", label, os_path);
        return 0;
    }
    MoviePumpState st;
    memset(&st, 0, sizeof(st));
    st.mv = mv;
    if (movie_info(mv, &st.mi) != 0 || st.mi.width <= 0 || st.mi.height <= 0) {
        fprintf(log, "movie: %s: no video stream in %s\n", label, os_path);
        movie_close(mv);
        return 0;
    }
    fprintf(log, "movie: %s %s — %dx%d @ %d fps, %d Hz audio%s\n", label,
            os_path, st.mi.width, st.mi.height,
            st.mi.fps_den > 0 && st.mi.fps_num > 0 ? st.mi.fps_num / st.mi.fps_den
                                             : st.mi.fps_num,
            st.mi.audio_rate, rect != NULL ? " (rect)" : "");

    /* The movie carries its own soundtrack — silence the demo's ambient
     * loop while it plays (the caller re-seeds it afterwards). */
    audio_stop(ap);

    st.rect = rect;
    st.win = win;
    st.surf = surf;
    st.ap = ap;
    st.quit = quit;
    st.frame_ms = st.mi.fps_den > 0 && st.mi.fps_num > 0
                      ? 1000 * st.mi.fps_den / st.mi.fps_num
                      : 40;
    if (st.frame_ms < 1) {
        st.frame_ms = 1;
    }
    st.next_t = now_ms();

    MoviePump pump;
    memset(&pump, 0, sizeof(pump));
    pump.next = s_movie_frame_next;
    pump.next_user = &st;
    pump.info = &st.mi;
    pump.rect = rect;                 /* NULL = full-window branch */
    pump.present = s_movie_frame_present;
    pump.present_user = &st;
    pump.quit = s_movie_frame_quit;
    pump.quit_user = &st;
    /* No restore hook: the port's lock-present-blit path leaves no lasting
     * surface state, so the corpus's setup-time restore seam is inert
     * here. */
    movie_video_playback_loop(&pump);

    movie_close(mv);
    fprintf(log, "movie: %s done — %d frames%s\n", label, st.frames,
            st.skipped ? " (skipped)" : "");
    return 1;
}

/* Level load + mission seeding + level intro movie + engine binding. Both
 * the post-front-end boot path and the pause menu's Restart route here
 * (finding 07/09 + #47's level cinematics). */
static void frameloop_load_level(FILE* log, const char* wad_dir,
                                 int play_movies, PlatformWindow* win,
                                 DisplaySurface* surf, AudioPlayer* ap,
                                 int audio_status, int* quit,
                                 GameContext* game, EngineContext* ctx,
                                 ViewContext* views, const char* level_name)
{
    view_primary_level_release(ctx);
    game->ol_camera_set = 0;
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
    frame_world_arrows_level_releasing();
#endif
    options_apply_from_slot(game);
    game->level = level_load(game->cfg, level_name);
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
    if (game->level != NULL) frame_world_arrows_level_activated(game->level);
#endif
    if (game->level != NULL) {
        fprintf(log, "frameloop: level %s loaded: %dx%d grid, voxel stride "
                     "0x48, objective word 0x%08x\n",
                level_name, level_width(game->level), level_height(game->level),
                game->level->objective);
        /* Level intro cinematic — MoviePlayLevelVideo @ 0x415990 reads the
         * level's `Video` config key and plays it after load (the corpus
         * plays it inside LevelLoad @ 0x429890 through
         * DirectShowVideoPlaybackLoop @ 0x42efc0 — whose full-window branch
         * this seam reproduces: the corpus's rect blit never applied its
         * computed rectangle, finding 206), same gates as boot. */
        if (play_movies) {
            char mp[256], os[512];
            if (movie_level_video_path(game->cfg, level_name, mp, sizeof(mp))
                && movie_path_resolve(wad_dir, mp, os, sizeof(os))) {
                movie_play_avi(os, level_name, win, surf, ap, quit, log);
                if (audio_status == 0 && !*quit) {
                    /* the level movie also silenced the ambient */
                    audio_loop_named(ap, "Sounds\\Iambloop.wav", 0.0f);
                }
            }
        }
        level_no_gather_config_setup(game->cfg, level_name);
        /* Mission plumbing (finding 09/10): seed the 10-task table and
         * reward screen from the level's Reward group. LevelLoad has already
         * parsed the `.ol` object list. */
        mission_task_init(game, game->cfg, level_name);
        mission_reward_init(game, game->cfg, level_name);
        /* Retail StartLevelSelected snapshots the type-4 name list and Pilot
         * count after the level object list has populated the entity tables. */
        start_level_selected(game);
        fprintf(log, "frameloop: mission tasks seeded, %s, %d object(s) "
                     "placed, crystal quota=%d\n",
                game->cfg != NULL ? "reward config ok" : "no cfg",
                game->entity_placed, game->crystal_quota);
    } else {
        fprintf(log, "frameloop: level %s load FAILED (maps/config missing)\n",
                level_name);
    }
    /* Engine <-> level binding: the engine sim reads the loaded LevelState
     * (width/height/voxels) so cameras/tickers see the real terrain. The
     * full pointer cannot ride the 32-bit corpus slot on a 64-bit host, so
     * the resolved voxel pointer is bound here via the game layer — after
     * the front end chose this level. */
    ctx->level_block = game->level;
    ctx->level_voxels = game->level != NULL
                            ? (const uint8_t*)level_load_ptr(game->level,
                                                             LEVEL_OFF_VOXELS)
                            : NULL;
    ctx->level_terrain = game->level != NULL
                             ? level_load_ptr(game->level, LEVEL_OFF_TERRAIN)
                             : NULL;
    if (game->level != NULL) {
        fprintf(log, "frameloop: engine bound to level %dx%d, blocksize %.1f, "
                     "voxels@%sresolved\n",
                engine_level_width(ctx), engine_level_height(ctx),
                engine_level_blocksize(ctx),
                ctx->level_voxels != NULL ? "" : "UN");
        /* Let the engine's camera grid sweep walk the real voxel grid. */
        g_camera_grid_enabled = 1;
    }
    /* Apply the already parsed OList world pose once, after script reset.
     * Subsequent target/zoom requests remain owned by the primary view. */
    if (game->level != NULL && game->ol_camera_set &&
        !view_primary_level_initialize(ctx, game->ol_camera_x,
            game->ol_camera_y, game->ol_camera_heading)) {
        fprintf(log, "frameloop: authored primary camera initialization failed\n");
        *quit = 1;
    }
    (void)views;
}

void frameloop_blit_texture_scaled(const TextureSurface* texture,
                                          void* pixels, int pitch,
                                          int width, int height, int x, int y,
                                          int draw_w, int draw_h)
{
    if (texture == NULL || texture->pixels == NULL || pixels == NULL
        || texture->width <= 0 || texture->height <= 0
        || draw_w <= 0 || draw_h <= 0) {
        return;
    }
    for (int dy = 0; dy < draw_h; ++dy) {
        int py = y + dy;
        if (py < 0 || py >= height) {
            continue;
        }
        int sy = (int)((int64_t)dy * texture->height / draw_h);
        uint8_t* dst = (uint8_t*)pixels + (size_t)py * pitch;
        for (int dx = 0; dx < draw_w; ++dx) {
            int px = x + dx;
            if (px < 0 || px >= width) {
                continue;
            }
            int sx = (int)((int64_t)dx * texture->width / draw_w);
            uint32_t color = texture->pixels[(size_t)sy * texture->width + sx];
            if (texture->source_key_set) {
                uint32_t rgb = color & 0x00ffffffu;
                if (rgb >= (texture->source_key_low & 0x00ffffffu) &&
                    rgb <= (texture->source_key_high & 0x00ffffffu)) continue;
            }
            dst[(size_t)px * 4 + 0] = (uint8_t)(color >> 16);
            dst[(size_t)px * 4 + 1] = (uint8_t)(color >> 8);
            dst[(size_t)px * 4 + 2] = (uint8_t)color;
            dst[(size_t)px * 4 + 3] = (uint8_t)(color >> 24);
        }
    }
}

/* FUN_004574e0 reads the active level's ObjectiveText resource/section and
 * treats every \\a as a page boundary, not as the end of its briefing. */
static int frameloop_brief_load(BlockFile* cfg, const char* level_name,
                                int kind, char* out, size_t out_size)
{
    char path[512];
    const char* group;
    const char* text_path;
    const char* label;
    size_t label_length;
    if (out == NULL || out_size == 0) return 0;
    out[0] = '\0';
    switch (kind) {
    case OBJECTIVE_PRESENTATION_BRIEFING: label = "Objective:"; break;
    case OBJECTIVE_PRESENTATION_ACHIEVED: label = "Completion:"; break;
    case OBJECTIVE_PRESENTATION_FAILED: label = "Failure:"; break;
    case OBJECTIVE_PRESENTATION_CRYSTAL_FAILED: label = "CrystalFailure:"; break;
    default: return 0;
    }
    label_length = strlen(label);
    if (util_level_cfg_path(cfg, level_name, "ObjectiveText", path,
                             sizeof(path)) == NULL) return 0;
    text_path = block_get_strptr(cfg, path);
    if (text_path == NULL || text_path[0] == '\0') return 0;
    group = strstr(level_name, "::");
    group = group != NULL ? group + 2 : level_name;
    Resource* r = resource_open(text_path, RESOURCE_AUTO);
    if (r == NULL || resource_read(r) < 0 || r->data == NULL) {
        resource_free(r);
        return 0;
    }
    const char* data = (const char*)r->data;
    const size_t size = (size_t)r->size;
    int section = 0;
    const char* end = data + size;
    for (const char* p = data; p < end;) {
        const char* line_end = (const char*)memchr(p, '\n', (size_t)(end - p));
        if (line_end == NULL) line_end = end;
        const char* line = p;
        while (line < line_end && (*line == ' ' || *line == '\t' || *line == '\r')) ++line;
        if (line < line_end && *line == '[') {
            const char* close = (const char*)memchr(
                line, ']', (size_t)(line_end - line));
            section = close != NULL &&
                (size_t)(close - line - 1) == strlen(group) &&
                strnicmp_lite(line + 1, group, (int)strlen(group)) == 0;
        } else if (section && (size_t)(line_end - line) >= label_length
                   && strnicmp_lite(line, label, (int)label_length) == 0) {
            line += label_length;
            while (line < line_end && (*line == ' ' || *line == '\t')) ++line;
            size_t n = (size_t)(line_end - line);
            while (n > 0 && (line[n - 1] == '\r' || line[n - 1] == ' ')) --n;
            /* Do not silently discard an authored page on overflow. */
            if (n >= out_size) {
                resource_free(r);
                return 0;
            }
            size_t at = 0;
            for (size_t i = 0; i < n; ++i) {
                char c = line[i];
                if (c == '\\' && i + 1 < n &&
                    (line[i + 1] == 'a' || line[i + 1] == 'n')) {
                    c = line[++i] == 'a' ? '\a' : '\n';
                }
                out[at++] = c;
            }
            out[at] = '\0';
            resource_free(r);
            return n > 0;
        }
        p = line_end < end ? line_end + 1 : end;
    }
    resource_free(r);
    return 0;
}

static void frameloop_wrap_brief(const char* src, char* out, size_t out_size,
                                const TextureSurface* font, int max_width)
{
    size_t at = 0;
    int line_width = 0;
    const int space_width = (int)formatted_text_draw_and_measure(
        font, NULL, 0, 0, 0, 0, 0, " ", NULL);
    for (const char* p = src; p != NULL && *p != '\0' && at + 1 < out_size;) {
        p += strspn(p, " \t");
        if (*p == '\0') break;
        if (*p == '\n') {
            out[at++] = *p++;
            line_width = 0;
            continue;
        }
        const char* word = p;
        size_t len = strcspn(p, " \t\n");
        size_t copy_len = len;
        if (copy_len > out_size - at - 1) copy_len = out_size - at - 1;
        char word_text[1024];
        if (copy_len >= sizeof(word_text)) copy_len = sizeof(word_text) - 1;
        memcpy(word_text, word, copy_len);
        word_text[copy_len] = '\0';
        int width = (int)formatted_text_draw_and_measure(
            font, NULL, 0, 0, 0, 0, 0, word_text, NULL);
        if (line_width > 0 && line_width + space_width + width > max_width) {
            out[at++] = '\n';
            line_width = 0;
        } else if (line_width > 0 && at + 1 < out_size) {
            out[at++] = ' ';
            line_width += space_width;
        }
        if (copy_len > out_size - at - 1) copy_len = out_size - at - 1;
        memcpy(out + at, word, copy_len);
        at += copy_len;
        line_width += width;
        p += len;
    }
    out[at] = '\0';
}

/* ObjectivePoll uses the shared Back bitmap at its objective-specific origin. */
static int frameloop_briefing_page_action(const char* brief, int* page_offset,
                                          int key_continue, int mouse_pressed,
                                          int mouse_x, int mouse_y,
                                          const TextureSurface* back_image,
                                          const TextureSurface* next_image)
{
    const char* page = brief + *page_offset;
    const char* next = strchr(page, '\a');
    const int over_back = *page_offset != 0 && back_image != NULL &&
        mouse_x >= 130 && mouse_y >= 315 &&
        mouse_x < 130 + back_image->width && mouse_y < 315 + back_image->height;
    if (mouse_pressed && over_back) {
        int previous = *page_offset - 1;
        while (previous > 0 && brief[previous - 1] != '\a') --previous;
        *page_offset = previous;
        return 1;
    }
    if (!key_continue && !mouse_pressed) return 0;
    /* ObjectivePoll excludes Back before acknowledging the final page,
     * including when Space supplied the continue edge. */
    if (next == NULL && over_back) return 0;
    if (next != NULL && !key_continue &&
        (next_image == NULL || mouse_x < 470 || mouse_y < 315 ||
         mouse_x >= 470 + next_image->width ||
         mouse_y >= 315 + next_image->height)) return 0;
    if (next != NULL) {
        *page_offset = (int)(next + 1 - brief);
        return 1;
    }
    return 2;
}

static void frameloop_briefing_render(PlatformWindow* win,
                                     DisplaySurface* surf,
                                     const TextureSurface* panel,
                                     const TextureSurface* title_font,
                                     const TextureSurface* body_font,
                                     const char* brief,
                                     const TextureSurface* next_image,
                                     const TextureSurface* back_image,
                                     int has_next, int has_back,
                                     int panel_x, int panel_y, const char* title,
                                     LevelState* level, GameContext* game,
                                     float delta, int frame)
{
    void* pixels = NULL;
    int pitch = 0;
    if (surface_lock(surf, &pixels, &pitch) != 0) return;
    level_render_scene(level, game, WINDOW_W, WINDOW_H, pixels, pitch);
    render_apply_brightness(pixels, pitch, WINDOW_W, WINDOW_H,
                            game->options.brightness);
    frontend_panels_render(*engine_shared_state_word(), pixels, pitch,
                           WINDOW_W, WINDOW_H);
    if (panel != NULL) {
        frameloop_blit_texture_scaled(panel, pixels, pitch, WINDOW_W, WINDOW_H,
                                      panel_x, panel_y, panel->width, panel->height);
    }
    if (title_font != NULL) {
        int tw = (int)formatted_text_printf_width(title_font, title);
        formatted_text_draw(title_font, pixels, pitch, WINDOW_W, WINDOW_H,
                            panel_x + 40 + (422 - tw) / 2, panel_y + 34, title);
    }
    if (body_font != NULL) {
        char wrapped[1200];
        if (brief != NULL && brief[0] != '\0') {
            frameloop_wrap_brief(brief, wrapped, sizeof(wrapped), body_font, 390);
            formatted_text_draw(body_font, pixels, pitch, WINDOW_W, WINDOW_H,
                                panel_x + 60, panel_y + 60, wrapped);
        }
    }
    if (has_next && next_image != NULL)
        frameloop_blit_texture_scaled(next_image, pixels, pitch, WINDOW_W, WINDOW_H,
                                      470, 315, next_image->width, next_image->height);
    if (has_back && back_image != NULL)
        frameloop_blit_texture_scaled(back_image, pixels, pitch, WINDOW_W, WINDOW_H,
                                      130, 315, back_image->width, back_image->height);
    surface_unlock(surf, pixels);
    advisor_text_animation_update(0x16, 0, 0);
    advisor_text_display_worker(delta);
    pixels = NULL;
    if (surface_lock(surf, &pixels, &pitch) != 0) return;
    pointer_cursor_draw_pixels(s_renderer_word_a, s_renderer_word_b,
                               pixels, pitch, WINDOW_W, WINDOW_H);
    platform_screenshot(win, pixels, pitch, WINDOW_W, WINDOW_H, frame);
    platform_present_pixels(win, pixels, pitch, WINDOW_W, WINDOW_H);
    surface_unlock(surf, pixels);
}

static int frameloop_briefing_prepare(FrameloopApplicationState* app)
{
    GameContext* game = app->game_storage;
    const char* level_name = game->level != NULL &&
        game->level->ptr_name != NULL ? (const char*)game->level->ptr_name
                                     : app->level_name;
    app->mission_brief[0] = '\0';
    app->mission_brief_page = 0;
    app->mission_brief_keys_held = input_keyboard_key_held(' ') ? 1u : 0u;
    app->mission_brief_kind = objective_presentation_kind(game->level);
    app->mission_brief_active =
        app->mission_brief_kind != OBJECTIVE_PRESENTATION_NONE;
    texture_release(app->briefing_panel);
    texture_release(app->briefing_next);
    texture_release(app->briefing_back);
    texture_release(app->briefing_back_arrow);
    app->briefing_panel = NULL;
    app->briefing_title_font = NULL;
    app->briefing_body_font = NULL;
    app->briefing_next = NULL;
    app->briefing_back = NULL;
    app->briefing_back_arrow = NULL;
    app->briefing_x = 0;
    app->briefing_y = 0;
    app->briefing_title[0] = '\0';
    if (!app->mission_brief_active) return 1;
    if (!frameloop_brief_load(game->cfg, level_name, app->mission_brief_kind,
                              app->mission_brief, sizeof(app->mission_brief))) {
        fprintf(app->log, "frameloop: active objective briefing unavailable "
                          "for %s\n", level_name);
        return 0;
    }
    {
        char key[512];
        char image_path[512];
        const char* image;
        const char* title;
        if (util_level_cfg_path(game->cfg, level_name, "ObjectiveImage640x480",
                                key, sizeof(key)) != NULL &&
            (image = block_get_strptr(game->cfg, key)) != NULL &&
            sscanf(image, "%511[^,],%d,%d", image_path,
                   &app->briefing_x, &app->briefing_y) == 3) {
            app->briefing_panel = image_surface_load(image_path, 0u, 0u);
            texture_source_color_key_set(app->briefing_panel, 0u, 0u);
        }
        const char* title_key =
            app->mission_brief_kind == OBJECTIVE_PRESENTATION_BRIEFING
                ? "MissionBriefingText" :
            app->mission_brief_kind == OBJECTIVE_PRESENTATION_ACHIEVED
                ? "MissionCompletedText" : "MissionFailedText";
        snprintf(key, sizeof(key), "%s::Main::%s", util_root_name(game->cfg), title_key);
        title = block_get_strptr(game->cfg, key);
        if (title != NULL) {
            size_t i;
            strncpy(app->briefing_title, title, sizeof(app->briefing_title) - 1);
            app->briefing_title[sizeof(app->briefing_title) - 1] = '\0';
            for (i = 0; app->briefing_title[i] != '\0'; ++i)
                if (app->briefing_title[i] == '_') app->briefing_title[i] = ' ';
        }
        snprintf(key, sizeof(key), "%s::Main::NextButton%ix%i", util_root_name(game->cfg), WINDOW_W, WINDOW_H);
        image = block_get_strptr(game->cfg, key);
        if (image != NULL) {
            app->briefing_next = image_surface_load(image, 0u, 0u);
            texture_source_color_key_set(app->briefing_next, 0u, 0u);
        }
        snprintf(key, sizeof(key), "%s::Main::BackButton%ix%i", util_root_name(game->cfg), WINDOW_W, WINDOW_H);
        image = block_get_strptr(game->cfg, key);
        if (image != NULL) {
            app->briefing_back = image_surface_load(image, 0u, 0u);
            texture_source_color_key_set(app->briefing_back, 0u, 0u);
        }
        snprintf(key, sizeof(key), "%s::Main::BackArrow", util_root_name(game->cfg));
        image = block_get_strptr(game->cfg, key);
        if (image != NULL) {
            app->briefing_back_arrow = image_surface_load(image, 0u, 0u);
            texture_source_color_key_set(app->briefing_back_arrow, 0u, 0u);
        }
    }
    app->briefing_title_font = texture_get_or_load(
        NULL, "Interface\\Fonts\\MbriefFont2.bmp");
    app->briefing_body_font = texture_get_or_load(
        NULL, "Interface\\Fonts\\MbriefFont.bmp");
    fprintf(app->log, "frameloop: Mission Brief ready (ObjectiveText loaded)\n");
    return 1;
}

/* ObjectivePoll's terminal acknowledgment and pause Quit share KeepRunning's
 * reward/return lane. Keep the level and blob alive until the reward/save
 * owner releases them. The frame-driven frontend retains the actual picker
 * origin instead of blocking here in MenuEnter's retail menu loop. */
static void frameloop_level_return_begin(FrameloopApplicationState* app)
{
    GameContext* game = app->game_storage;
    FrontEnd* fe = app->frontend_storage;
    const int show_frontend = frontend_enabled(game->cfg);
    app->picker_return_pending = 1;
    app->picker_return_origin = show_frontend ? fe->origin_picker_sub : -1;
    app->picker_return_level = game->level;
    app->reward_tick_level = game->level;
    app->reward_tick_started = 1;
    app->reward_frame.zone_action_consumed = 0;
    if (show_frontend && game->level != NULL) {
        mission_reward_set_active(game, 1);
        input_begin_frame(&app->input);
        (void)reward_screen_tick_bound();
        /* The zone owner already committed Advance. Its continuation is the
         * pending frontend return, not a second checkpoint and direct load. */
        if (app->reward_frame.zone_action_consumed == 2)
            mission_reward_set_active(game, 0);
    } else {
        mission_reward_set_active(game, 0);
    }
    app->reward_frame.zone_action_consumed = 0;
}

/* The portable frontend owns modal menu admission. TransitionControl still
 * owns the reference pause bit, sound stop and eventual resume effects. */
/* Choose the authored group at the real frame caller, not in TransitionControl. */
static int frameloop_overlay_request_mode(GameContext* game,
    const EngineContext* engine, int pause_requested)
{
    const int active = frontend_enabled(game->cfg);
    if ((engine->debug & 0x100u) != 0u && active) return 1;
    if ((pause_requested && (engine->debug & 0x100u) == 0u) ||
        ((engine->state & 0x2000000u) != 0u && active)) return 0;
    return -1;
}

static int frameloop_overlay_open(FrontEnd* fe, int mode)
{
    if (!frontend_open(fe, mode == 1 ? FE_MENU_OPTIONS : FE_MENU_PAUSED))
        return 0;
    if (mode == 0)
        unported_ledger_stubs::ledger_stub_00425c30_TransitionControl(0, 1u);
    return 1;
}

static void frameloop_pause_finish(GameContext* game, EngineContext* engine,
                                   int restart, int leave, int quit)
{
    /* Restart already calls TransitionControl from FrontEndStartLevel.
     * An OS close goes straight to teardown, not a synthetic resume. */
    if (quit || restart) return;
    if (leave) {
        if ((engine->state & 0x08000000u) != 0u) return;
    } else if (global_option_low_bits_enabled(game->level)) return;
    unported_ledger_stubs::ledger_stub_00425c30_TransitionControl(0, 0u);
}

/* Draw one front-end frame: frozen level (in-game pause) or a plain fill
 * (boot menu), then the submenu's MenuImage art + the menu layer. */
static void frameloop_menu_render(PlatformWindow* win, DisplaySurface* surf,
                                  FrontEnd* fe, const TextureSurface* hi_font,
                                  const TextureSurface* lo_font,
                                  GameContext* game, int frame)
{
    void* pixels = NULL;
    int   pitch = 0;
    if (surface_lock(surf, &pixels, &pitch) != 0) {
        return;
    }
    if (game->level != NULL) {
        level_render_scene(game->level, game, WINDOW_W, WINDOW_H,
                           pixels, pitch);
    } else {
        surface_fill(pixels, pitch, WINDOW_W, WINDOW_H, 8, 8, 20);
    }
    /* BrightnessSet updates scene lights; it does not dim 2D menu art.
     * Keep the software world's existing brightness effect before overlays. */
    render_apply_brightness(pixels, pitch, WINDOW_W, WINDOW_H,
                            game->options.brightness);
    if (game->level != NULL && !mission_reward_active(game))
        frontend_panels_render(*engine_shared_state_word(), pixels, pitch,
                               WINDOW_W, WINDOW_H);
    if ((fe->menu == FE_MENU_PAUSED || fe->menu == FE_MENU_OPTIONS) &&
        game->level != NULL && !mission_reward_active(game)) {
        /* EngineTick presents the paused message worker before MenuTick.
         * Its real target must be unlocked for the text/surface owners. */
        surface_unlock(surf, pixels);
        advisor_message_ui_draw(0.0f);
        if (surface_lock(surf, &pixels, &pitch) != 0) return;
    }
    frontend_background_render(fe, pixels, pitch, WINDOW_W, WINDOW_H);
    menu_overlay_surface_and_text_setup(fe, hi_font,
                                        lo_font, pixels, pitch, WINDOW_W,
                                        WINDOW_H, game->level == NULL);
    const FeSubmenu* active = frontend_active_submenu(fe);
    if (active != NULL) {
        for (int i = 0; i < active->item_count; ++i) {
            menu_save_image_config_rect_blit(fe, game, i, pixels, pitch,
                                             WINDOW_W, WINDOW_H);
        }
    }
    if (game->level != NULL)
        pointer_cursor_draw_pixels(s_renderer_word_a, s_renderer_word_b,
                                   pixels, pitch, WINDOW_W, WINDOW_H);
    platform_screenshot(win, pixels, pitch, WINDOW_W, WINDOW_H, frame);
    platform_present_pixels(win, pixels, pitch, WINDOW_W, WINDOW_H);
    surface_unlock(surf, pixels);
}

static int frameloop_game_init_callback(const char* root, void* user)
{
    GameContext* game = (GameContext*)user;
    BlockFile* cfg;
    if (root == NULL || root[0] == '\0' || game == NULL) return 0;
    cfg = game->cfg;
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
    /* Retail GameConfigInit creates the root (0x472710), camera frames and
     * configured lights (0x433770) before LightEffectsConfigurationLoad.
     * The application owns these objects across the menu/level transition. */
    if (!engine_scene_initialize(&s_frameloop_application.engine_context,
                                  cfg)) {
        fprintf(s_frameloop_application.log,
                "frameloop: scene initialization failed\n");
        return 0;
    }
    if (!building_scene_initialize()) {
        fprintf(s_frameloop_application.log, "frameloop: building scene owner initialization failed\n");
        return 0;
    }
    if (!actor_scene_initialize()) {
        fprintf(s_frameloop_application.log, "frameloop: actor scene owner initialization failed\n");
        return 0;
    }
    fprintf(s_frameloop_application.log,
            "frameloop: scene root/cameras/lights initialized root=%p\n",
            scene_root_object_get());
    fflush(s_frameloop_application.log);
#elif defined(LEGORRR_BACKEND_MINIWIN)
    /* The native creature scenes resolve meshes against Main::SharedObjects
     * after their own directory, as engine_scene_initialize sets it up. */
    if (cfg != NULL) {
        char key[256];
        snprintf(key, sizeof(key), "%s::Main::SharedObjects",
                 util_root_name(cfg));
        (void)shared_object_directory_root_set(block_get_strptr(cfg, key));
    }
#endif
    if (cfg != NULL) {
        EngineContext* engine = &s_frameloop_application.engine_context;
        char key[256];
        float speed, drop_off, acceleration, minimum, maximum;
        int indent;
        const char* config_root = util_root_name(cfg);
        snprintf(key, sizeof(key), "%s::Main::CameraSpeed", config_root);
        speed = block_get_float(cfg, key, 0.0f);
        snprintf(key, sizeof(key), "%s::Main::CameraDropOff", config_root);
        drop_off = block_get_float(cfg, key, 0.0f);
        snprintf(key, sizeof(key), "%s::Main::CameraAcceleration", config_root);
        acceleration = block_get_float(cfg, key, 0.0f);
        snprintf(key, sizeof(key), "%s::Main::MouseScrollIndent", config_root);
        indent = (int)block_get_float(cfg, key, 0.0f);
        camera_view_context_set(speed, drop_off, acceleration, indent);
        snprintf(key, sizeof(key), "%s::Main::MinDist", config_root);
        minimum = block_get_float(cfg, key, 0.0f);
        snprintf(key, sizeof(key), "%s::Main::MaxDist", config_root);
        maximum = block_get_float(cfg, key, 0.0f);
        camera_distance_bounds_set(engine->camera_controller, minimum, maximum);
        snprintf(key, sizeof(key), "%s::Main::MinTilt", config_root);
        minimum = block_get_float(cfg, key, 0.0f) * 0.017453292f;
        snprintf(key, sizeof(key), "%s::Main::MaxTilt", config_root);
        maximum = block_get_float(cfg, key, 0.0f) * 0.017453292f;
        camera_tilt_bounds_set(engine->camera_controller, minimum, maximum);
        engine_render_poly_ranges_set(
            block_get_float(cfg, "Lego*::Main::MedPolyRange", 0.0f),
            block_get_float(cfg, "Lego*::Main::HighPolyRange", 0.0f));
        /* GameConfigInit loads HUD config before the later bubble and font
         * resource passes. This is now part of the registered init callback. */
        bubble_config_global_flags_set(engine_feature_word_peek());
        (void)lego::upgrade_types_load(cfg);
        entity_names_config_load(cfg);
        if (block_get_bool_path(cfg, "HelpWindow::HelpWindowOn") == 1) {
            help_window_resources_init(cfg, NULL);
        }
        frontend_hud_runtime_install();
        frontend_render_three_pass_update(cfg, util_root_name(cfg));
        (void)light_effect_waypoint_config_load(cfg, "Lego*");
        light_effect_configuration_load(cfg, "Lego*");
        bubble_config_texture_load(cfg);
        preload_numbered_bitmaps("Interface\\FONTS\\HealthFont", "a000_");
        tooltip_icons_config_load(cfg);
        lego::level_upgrade_name_strings_load(cfg);
    }
    /* The configured incident-message names are resolved against this table,
     * so seed it before loading the InfoMessages records. */
    message_init_defaults();
    if (cfg != NULL) message_load_infomessages(cfg);
    return 1;
}

/* One front-end frame. This is deliberately a step function: the caller owns
 * frame pacing and the next invocation, so this body can be dispatched from
 * the application callback loop instead of requiring a menu-owned while(). */
static FeEvent frameloop_main_menu_frame(FrameloopApplicationState* app,
                                         float dt)
{
    GameContext* game;
    FrontEnd* fe;
    PlatformEvent* events;
    int event_count;
    const FeSubmenu* submenu;
    TextureSurface* hi_font = NULL;
    TextureSurface* lo_font = NULL;
    FeInput input_frame;
    FeEvent event;
    int tick_menu, tick_sub;
    int i;
    if (app == NULL || app->game_storage == NULL
        || app->frontend_storage == NULL || app->quit) {
        return FE_EVT_NONE;
    }
    game = app->game_storage;
    fe = app->frontend_storage;
    events = app->frame_events;
    event_count = app->frame_event_count;
    (void)frameloop_application_events_track(app, events, event_count);
    if (app->quit) return FE_EVT_NONE;
    input_capture_events(&app->input, events, event_count);
    input_cursor_position_update();
    input_cursor_position_sync(&app->input);
    for (i = 0; i < event_count; ++i) {
        if (events[i].kind == PLATFORM_EVENT_QUIT) {
            app->quit = 1;
            app->stop_reason = "closed";
            return FE_EVT_NONE;
        }
    }
    if (app->frame_cap > 0 && app->frames >= app->frame_cap) {
        app->stop_reason = "autostop";
        return FE_EVT_NONE;
    }
    submenu = frontend_active_submenu(fe);
    if (submenu != NULL) {
        if (submenu->hifont[0] != '\0')
            hi_font = texture_get_or_load(NULL, submenu->hifont);
        if (submenu->lofont[0] != '\0')
            lo_font = texture_get_or_load(NULL, submenu->lofont);
    }
    input_frame.up = input_key_edge(&app->input, PLATFORM_KEY_UP);
    input_frame.down = input_key_edge(&app->input, PLATFORM_KEY_DOWN);
    input_frame.left = input_key_edge(&app->input, PLATFORM_KEY_LEFT);
    input_frame.right = input_key_edge(&app->input, PLATFORM_KEY_RIGHT);
    input_frame.enter = input_key_edge(&app->input, ' ')
                        || input_key_edge(&app->input, PLATFORM_KEY_ENTER);
    input_frame.esc = input_key_edge(&app->input, PLATFORM_KEY_ESC);
    input_frame.mouse_x = app->input.mouse_x;
    input_frame.mouse_y = app->input.mouse_y;
    input_frame.click = input_mouse_pressed(&app->input);
    input_frame.mdown = app->input.mouse_down;
    tick_menu = fe->menu;
    tick_sub = fe->sub;
    event = frontend_tick(fe, &input_frame, hi_font);
    if (fe->option_changed) {
        const FeItem* item = frontend_active_item(fe);
        if (item != NULL) {
            const int is_sfx = s_icontains(item->label, "sfx")
                               || s_icontains(item->label, "sound");
            const int is_music = s_icontains(item->label, "music");
            const int live = item->kind == FE_ITEM_SLIDER
                             && (is_sfx || is_music) && item->max > 0;
            if (live && app->audio.dev != NULL) {
                if (is_sfx) audio_menu_sfx_volume_set(item->value);
                else audio_menu_volume_set(item->value);
            }
            fprintf(app->log, "frameloop: option %s -> %d/%d (%s)\n",
                    item->label, item->value, item->max,
                    live ? "audio" : "store — applied live");
            fe->option_changed = 0;
        }
    }
    if (engine_callback_triplet_tick_dispatch(dt) == 0) {
        app->quit = 1;
        app->stop_reason = "tick callback";
        return FE_EVT_NONE;
    }
    if (event == FE_EVT_QUIT) {
        app->quit = 1;
        app->stop_reason = "menu quit";
        return event;
    }
    if (event == FE_EVT_START_LEVEL) {
        snprintf(app->level_name, sizeof(app->level_name), "%s",
                 fe->pending_level);
        game->current_record = fe_record_for_level(fe, app->level_name);
        if (game->current_record < 0) return FE_EVT_NONE;
        if (fe->selected_save_slot >= 0) {
            if (!save_read_slot(game, fe->selected_save_slot, 1)) {
                (void)frontend_open_submenu(fe, FE_MENU_MAIN, 1);
                return FE_EVT_NONE;
            }
            app->pending_load_slot = fe->selected_save_slot;
            app->pending_load_record = game->current_record;
        } else {
            save_slot_set(game, -1);
        }
        fprintf(app->log, "frameloop: front end picked level %s\n",
                app->level_name);
        return event;
    }
    if (event == FE_EVT_LOAD_GAME) {
        if (frontend_open_saved_campaign(fe, game, frontend_slot_raw_index(fe->load_slot))) {
            fprintf(app->log, "frameloop: selected slot %d; campaign picker open\n",
                    fe->selected_save_slot);
            return FE_EVT_NONE;
        }
        /* Empty/invalid slot semantics remain as before. */
        snprintf(app->level_name, sizeof(app->level_name), "%s",
                 env_get("LEGORRR_LEVEL", "Level01"));
        return event;
    }
    if (event == FE_EVT_CREDITS) {
        frameloop_credits(app->window, &app->surface, game->cfg,
                          app->wad_dir, &app->audio, app->audio_status,
                          &app->quit, app->log, &app->frames);
    }
    sound_subsystem_frame_tick(app->menu_sound_clock.dt);
    frameloop_menu_render(app->window, &app->surface, fe, hi_font, lo_font,
                          game, (int)app->frames);
    if (fe->open && fe->menu == tick_menu && fe->sub == tick_sub)
        frontend_scroll_update(fe, &input_frame);
    if (app->audio_status == 0) audio_update(&app->audio);
    ++app->frames;
    menu_sound_tick_clock_advance(&app->menu_sound_clock);
    return event;
}

int frameloop_application_init(void)
{
    FrameloopApplicationState& app = s_frameloop_application;
    if (app.initialized) return 1;
    memset(&app, 0, sizeof(app));
    indexed_surface_resource_hooks_set(NULL,
        frameloop_indexed_texture_load,
        frameloop_indexed_texture_release, NULL, NULL);
    const char* wad_dir_environment = getenv("LEGORRR_WAD_DIR");
    const int explicit_wad_dir = wad_dir_environment != NULL &&
                                 wad_dir_environment[0] != '\0';
    const char*& wad_dir = app.wad_dir;
    wad_dir = explicit_wad_dir ? wad_dir_environment : ".";
    const char* autostop = env_get("LEGORRR_AUTOSTOP_FRAMES", "0");
    long& frame_cap = app.frame_cap;
    frame_cap = strtol(autostop, NULL, 10);
    if (frame_cap < 0) {
        frame_cap = 0;
    }
    /* CI/headless hook: open the pause overlay at this in-game frame (as if
     * Esc was pressed) so the PausedMenu can be screenshotted. */
    long& autopause = app.autopause;
    autopause = strtol(env_get("LEGORRR_AUTOPAUSE_FRAMES", "0"), NULL, 10);

    FILE*& log = app.log;
    log = fopen("legoRR-stub.log", "a");
    if (log == NULL) {
        log = stderr;
    }
    app.log_owned = log != stderr;

    int mounted;
    if (explicit_wad_dir) {
        /* Keep the explicit WAD_DIR path used by make run and data tests:
         * it mounts exactly the requested directory and skips retail startup
         * prompts/CD policy. */
        resource_set_cd_root(wad_dir);
        mounted = resource_mount(wad_dir, resource_get_wad_prefix());
    } else {
        /* With no host override, enter the retail-shaped cwd/Data startup
         * flow. Registry values and the WinAPI modal prompt use the platform
         * services installed below; no process-exit hook is installed. */
        frameloop_resource_startup_services_install();
        resource_mount_startup_target(resource_get_wad_prefix(),
            resource_startup_require_cd_get(),
            "SOFTWARE\\LEGO Media\\Games\\Rock Raiders");
        mounted = resource_mounted_count();
    }
    fprintf(log, "frameloop: mounted %d wad(s) from %s\n", mounted, wad_dir);
    if (mounted == 0) {
        if (log != stderr) {
            fclose(log);
        }
        return 0;
    }

    /* These reconstructed contexts contain large fixed-capacity tables. The
     * reference keeps its corresponding state outside LegorrrRun's stack;
     * placing both port structs here creates a >1 MiB frame and overflows
     * the executable's 1 MiB PE stack before startup reaches this function. */
    GameContext*& game_storage = app.game_storage;
    FrontEnd*& fe_storage = app.frontend_storage;
    game_storage = (GameContext*)calloc(1, sizeof(GameContext));
    fe_storage = (FrontEnd*)calloc(1, sizeof(FrontEnd));
    if (game_storage == NULL || fe_storage == NULL) {
        fprintf(log, "frameloop: could not allocate game/frontend contexts\n");
        free(game_storage);
        free(fe_storage);
        if (log != stderr) {
            fclose(log);
        }
        return 1;
    }
    GameContext& game = *game_storage;
    FrontEnd& fe = *fe_storage;

    /* --- Game layer: config DB + level load (findings 07/12/22) ---------- */
    game_context_init(&game);
    if (!save_bind_resource_directory(&game)) {
        fprintf(log, "frameloop: could not resolve save directory\n");
        frameloop_game_context_dispose(&game);
        free(game_storage);
        free(fe_storage);
        if (log != stderr) fclose(log);
        return 1;
    }
    cmd_init();
    cmd_context_set(&game);
    game.cfg = block_open("Lego.cfg");

    EngineState& state = app.engine_state;
    InputState& input = app.input;
    memset(&state, 0, sizeof(state));
    memset(&input, 0, sizeof(input));
    input_reset(&input);
    input_direct_state_bind(&input);
    /* Constructor 0x435b20: primary kind 1, secondary kind 2, free kind 3.
     * Keep the retail local eye distance separate from world follow X/Y. */
    ViewContext (&views)[3] = app.views;
    memset(views, 0, sizeof(app.views));
    for (int i = 0; i < 3; ++i) {
        views[i].mode = i + 1;
        views[i].follow_span = 200.0f;
        cam_set_pos(&views[i].cam_world, 0.0f, 0.0f, -200.0f);
        views[i].cam_world.rot[2] = 1.0f;
    }
    EngineContext& ctx = app.engine_context;
    memset(&ctx, 0, sizeof(ctx));
    ctx.app_mode = ENGINE_APP_NORMAL;
    ctx.context_mode = 1;
    ctx.state_block = &state;
    ctx.input = &input;
    ctx.views[0] = &views[0];
    ctx.views[1] = &views[1];
    ctx.views[2] = &views[2];
    views[0].controller = ctx.camera_controller;
    camera_distance_set(ctx.camera_controller, 200.0f);
    /* CameraZoomStepAdjust targets the primary camera record (0x5560e4). */
    camera_zoom_step_context_set(ctx.camera_controller);
    ctx.time_scale = 1.0f;
    const float dt = 1.0f / 30.0f;
    app.frame_dt = dt;
    tutorial_runtime_context_set(&game, &ctx, &input);
    engine_tutorial_dispatch_hook_set(frame_tutorial_script_dispatch, NULL);
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
    engine_world_arrow_capture_hook_set(frame_world_arrows_capture, &game);
#endif
    /* Engine <-> level binding happens after the front end below — the level
     * loads only once the player picks one — so ctx.level_block/voxels/
     * terrain are bound there, not here. */
    /* Selection/command wiring (finding 06): the executor's held-key probe
     * reads this frame's input tables; a keybind demo for a global op. */
    game.input_keyheld = demo_keyheld;
    cmd_bind_key('z', 0x40, 0, 0, 0, 0);

    PlatformWindow*& win = app.window;
    win = platform_create_window("lrr-portable (frame loop)",
                                 WINDOW_W, WINDOW_H);
    if (win == NULL) {
        fprintf(log, "frameloop: no platform window (headless?)\n");
        frameloop_game_context_dispose(&game);
        free(game_storage);
        free(fe_storage);
        if (log != stderr) {
            fclose(log);
        }
        return 1;
    }

#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
    /* Retail WinMain opens DirectDrawDeviceInitialize's mode selector after
     * creating the owner window and before entering the game callback loop. */
    surface_winapi_startup_ops_install(platform_window_hwnd(win));
    if (!directdraw_device_initialize(1, 0, 0, 0,
            "No DirectX 3D accelerator could be found.")) {
        int cancelled = directdraw_device_selection_cancelled();
        fprintf(log, cancelled ? "frameloop: DirectDraw mode selection cancelled\n"
                               : "frameloop: DirectDraw mode selection failed\n");
        platform_destroy_window(win);
        frameloop_game_context_dispose(&game);
        free(game_storage);
        free(fe_storage);
        if (log != stderr) fclose(log);
        return cancelled ? 0 : 3;
    }
    fprintf(log, "frameloop: DirectDraw mode selection accepted\n");
    fflush(log);
#endif

    DisplaySurface& surf = app.surface;
    memset(&surf, 0, sizeof(surf));
    int rc = surface_create(&surf, platform_window_hwnd(win),
                            WINDOW_W, WINDOW_H);
    if (rc != 0) {
        fprintf(log, "frameloop: surface_create rc=%d\n", rc);
        platform_destroy_window(win);
        frameloop_game_context_dispose(&game);
        free(game_storage);
        free(fe_storage);
        if (log != stderr) {
            fclose(log);
        }
        return 2;
    }
    fprintf(log, "frameloop: surface_create rc=0 (%dx%d)\n",
            WINDOW_W, WINDOW_H);
    fflush(log);
    surface_active_render_target_set(&surf);
    /* Register the same init/tick/shutdown boundary consumed by retail
     * WinMain. Startup config loading above remains owned by this adapter;
     * the registered init callback validates that the app root is ready. */
    engine_game_config_callback_registry_hooks_set(
        &ctx, NULL, frameloop_game_init_callback, &game);
    engine_game_config_callback_registry_init("LegoRR");
    if (!engine_callback_triplet_init_dispatch()) {
        fprintf(log, "frameloop: application init callback failed\n");
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
        engine_scene_shutdown();
#endif
        surface_active_render_target_set(NULL);
        surface_destroy(&surf);
        platform_destroy_window(win);
        frameloop_game_context_dispose(&game);
        free(game_storage);
        free(fe_storage);
        if (log != stderr) fclose(log);
        return 1;
    }
    int save_loaded = save_read_slot(&game, SAVE_DEFAULT_SLOT, 0);
    fprintf(log, "frameloop: save slot %d %s\n", SAVE_DEFAULT_SLOT,
            save_loaded ? "loaded" : "bootstrap");
    catalog_slot_flag_hooks_set(frame_catalog_slot_eligible,
                                frame_catalog_slot_available,
                                frame_catalog_slot_rank, &game);
    input_action_state_query_set(frame_action_catalog_query, &app);

    /* Real corpus textures through the merged texture loader. */
    TextureSurface*& rock = app.rock_texture;
    TextureSurface*& ui = app.ui_texture;
    rock = texture_get_or_load(NULL,
                               "World\\WorldTextures\\ROCKROOF.BMP");
    ui = texture_get_or_load(NULL,
                             "Interface\\Buttons\\info_path.bmp");
    if (rock == NULL || ui == NULL) {
        fprintf(log, "frameloop: texture load failed (rock=%p ui=%p)\n",
                (void*)rock, (void*)ui);
    }

    /* Audio: open the DirectSound-analog surface + play a real corpus wav. */
    AudioPlayer& ap = app.audio;
    memset(&ap, 0, sizeof(ap));
    audio_system_player_set(&ap);
    if (!s_command_line_sound_disabled) {
#if defined(LEGORRR_BACKEND_WINAPI)
        /* CDAudioDeviceStateUpdate is the retail startup bridge: it records
         * the DirectSound availability gate before DirectSoundSystemInit. */
        (void)media_cd_audio_device_state_update(0);
#else
        audio_system_init(platform_window_hwnd(win));
#endif
    }
    if (game.cfg != NULL) {
        const char* cd_start = block_get_strptr(game.cfg, "Lego*::Main::CDStartTrack");
        const char* cd_count = block_get_strptr(game.cfg, "Lego*::Main::CDTracks");
        int count = cd_count != NULL ? atoi(cd_count) : 0;
        engine_debug_word_bit_set(0x800u,
            count != 0 && block_get_bool_path(game.cfg, "Lego*::Main::MusicOn") == 1);
        media_cd_config_set(cd_start != NULL ? atoi(cd_start) : 0, count);
    }
    if (game.cfg != NULL) {
        const char* tooltip_rgb = block_get_strptr(game.cfg, "Lego*::Main::ToolTipRGB");
        int tooltip_r = 0, tooltip_g = 0, tooltip_b = 0;
        TextureSurface* tooltip_font = bitmap_font_resource_load(
            "Interface\\Fonts\\ToolTipFont.bmp");
        if (tooltip_rgb != NULL)
            (void)sscanf(tooltip_rgb, "%d:%d:%d", &tooltip_r, &tooltip_g, &tooltip_b);
        tooltip_software_initialize(tooltip_font, WINDOW_W, WINDOW_H,
            (float)tooltip_r / 255.0f, (float)tooltip_g / 255.0f,
            (float)tooltip_b / 255.0f);
        /* GameConfigInit supplies the tooltip font to the action catalog
         * initializer at the shipped logical origin (0x41fd1b). */
        interface_menu_action_labels_init(565u, 18u, tooltip_font);
        /* GameConfigInit loads the Samples aliases after enabling the sound
         * bank. This portable startup has the Lego.cfg tree ready earlier,
         * so load the same section once the audio backend opens. */
        sound_samples_config_load(game.cfg, 1);
        advisor_tooltip_and_sound_config_load(game.cfg);
    }
    int& audio_status = app.audio_status;
    audio_status = !s_command_line_sound_disabled && sound_bank_sound_on()
                 ? 0 : -1;
    int sfx_frames = 0;
    if (audio_status == 0) {
        /* Master gain from the store, not a fixed 0.6: the active save
         * context's restored SFX_Volume (or the default) applies immediately
         * (issue #85) — saved settings take effect without a menu nudge. */
        audio_set_volume(&ap, options_audio_gain(&game.options));
        int loop_rc = audio_loop_named(&ap, "Sounds\\Iambloop.wav", 0.0f);
        sfx_frames = audio_play_named(&ap, "Sounds\\Buildings\\elec5.wav");
        fprintf(log,
                "frameloop: audio on, ambient loop Iambloop.wav rc=%d, "
                "startup sfx elec5.wav -> %d frames\n",
                loop_rc, sfx_frames);
    } else {
        fprintf(log, "frameloop: audio unavailable (headless?) rc=%d\n",
                audio_status);
    }
    /* Message bus demo hooks (finding 22): incidents dispatched by the game
     * layer (AI deposits, advisor hints) land here. */
    s_demo_ap = &ap;
    message_set_sound_hook(demo_msg_sound);
    message_set_callback(demo_msg_cb);
    /* Air supply drains in the demo; the air-beat loop + notifications
     * (0x11/0x12) kick in when it runs low. One loop track, so the air beat
     * replaces the ambient. */
    state.air_beat = 3;
    airbeat_set_loop(audio_status == 0 ? &ap : NULL,
                     "Sounds\\Vehicles\\Heliloop1.wav");

    long& frames = app.frames;
    frames = 0;
    int& thumb_slot = app.thumbnail_slot;
    thumb_slot = -1;   /* pending `.dat` thumbnail dump (see below) */
    int& quit = app.quit;
    quit = 0;
    const char*& stop = app.stop_reason;
    stop = "closed";
    RewardFramePumpContext& reward_frame = app.reward_frame;
    RewardScreenOps& reward_ops = app.reward_ops;
    memset(&reward_frame, 0, sizeof(reward_frame));
    memset(&reward_ops, 0, sizeof(reward_ops));
    reward_frame.window = win;
    reward_frame.surface = &surf;
    reward_frame.game = &game;
    reward_frame.input = &input;
    reward_frame.audio = audio_status == 0 ? &ap : NULL;
    reward_frame.frontend = &fe;
    reward_frame.log = log;
    reward_frame.frames = &frames;
    reward_frame.frame_cap = frame_cap;
    reward_frame.quit = &quit;
    reward_frame.stop = &stop;
    reward_ops.context = &reward_frame;
    reward_ops.time_get_ms = reward_frame_time_get;
    reward_ops.save_button_check = reward_save_button_check;
    reward_ops.loop_should_abort = reward_frame_loop_abort;
    reward_ops.reward_zone_check = reward_zone_dispatch;
    reward_ops.task_visual_draw = reward_task_visual_queue;
    reward_ops.task_text_draw = reward_task_text_queue;
    reward_ops.score_text_draw = reward_score_text_queue;
    reward_ops.reward_stats_draw = reward_stats_result_queue;
    reward_ops.frame_pump = reward_frame_pump;
    /* mission_reward_init creates and binds its default state after level
     * load; preinstall the live frame operations so that state inherits them. */
    reward_screen_runtime_bind(NULL, &reward_ops);

    /* --- Boot movies (finding 14 §4 + finding 23 callbacks) ----------------
     * The corpus's boot hub (FUN_0041fb50) plays the LMI logo (`LegoAvi`)
     * then the intro (`RRAvi`) before the front-end, each gated by
     * AvisEnabled @ 0x414ce0 (the `DontPlayAvis` kill switch). Decoding is
     * software (src/gods98/movie.*, FFmpeg dlopen'd at runtime), so movies
     * play where the runtime libs exist; headless/CI runs
     * (LEGORRR_AUTOSTOP_FRAMES) and LEGORRR_NO_MOVIES skip them. */
    int avis_enabled = movie_avis_enabled(game.cfg);
    const char* no_movies = env_get("LEGORRR_NO_MOVIES", "");
    int& play_movies = app.play_movies;
    play_movies = frame_cap == 0
                      && avis_enabled
                      && no_movies[0] == '\0'
                      && movie_available();
    if (play_movies) {
        char mp[256], os[512];
        if (movie_key_path(game.cfg, "LegoAvi", mp, sizeof(mp))
            && movie_path_resolve(wad_dir, mp, os, sizeof(os))) {
            movie_play_avi(os, "LegoAvi", win, &surf, &ap, &quit, log);
        } else {
            fprintf(log, "movieloop: no LegoAvi (%s)\n",
                    mp[0] != '\0' ? mp : "(no config key)");
        }
        if (!quit
            && movie_key_path(game.cfg, "RRAvi", mp, sizeof(mp))
            && movie_path_resolve(wad_dir, mp, os, sizeof(os))) {
            movie_play_avi(os, "RRAvi", win, &surf, &ap, &quit, log);
        }
        if (audio_status == 0 && !quit) {
            /* The movies silenced the ambient; bring it back. */
            audio_loop_named(&ap, "Sounds\\Iambloop.wav", 0.0f);
        }
    } else if (frame_cap == 0) {
        const char* reason = !avis_enabled ? "DontPlayAvis"
                              : no_movies[0] != '\0' ? "LEGORRR_NO_MOVIES"
                                                     : "FFmpeg runtime unavailable";
        fprintf(log, "movieloop: boot movies off (%s)\n", reason);
    }

    /* --- Front-end boot (finding 23): main menu before level load -----------
     * Ordinary launches enter the reconstructed front end when enabled.
     * LEGORRR_LEVEL remains a direct-launch override for development. */
    char (&level_name)[FE_LEVEL_NAME_MAX] = app.level_name;
    int& boot_menu = app.boot_menu;
    int& frontend_built = app.frontend_built;
    boot_menu = 0;
    frontend_built = 0;
    if (game.cfg != NULL) {
        fprintf(log, "frameloop: config Lego.cfg loaded\n");
        int built = frontend_build(&fe, game.cfg);
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
        advisor_tutorial_icon_runtime_initialize(game.cfg);
#endif
        /* GameConfigInit loads AdvisorPositions after panel-name setup, and
         * only when DisplayAdvisor is enabled. frontend_build initializes
         * the panel table used by the loader's optional action token. */
        char advisor_config_path[128];
        snprintf(advisor_config_path, sizeof(advisor_config_path),
                 "%s::DisplayAdvisor", util_root_name(game.cfg));
        if (block_get_bool_path(game.cfg, advisor_config_path) == 1) {
            advisor_position_config_load(game.cfg, util_root_name(game.cfg),
                                         WINDOW_W, WINDOW_H);
        }
        /* SaveMenuBuild builds and retains the campaign/tutorial trees before
         * refreshing all six slots; SavReadSlot uses the campaign tree count. */
        sav_refresh_slots(&game);
        /* Functional options (issue #85): attach the live store so the menu
         * widgets show and set the effective values. frontend_build ran
         * with no store (defaults); sync seeds the loaded slot's values. */
        fe.opts = &game.options;
        frontend_options_sync(&fe);
        frontend_built = built;
        boot_menu = frontend_built && frontend_enabled(game.cfg)
                 && env_get("LEGORRR_LEVEL", "")[0] == '\0'
                 && s_command_line_level_override[0] == '\0';
        fprintf(log, "frameloop: frontend built=%d enabled=%d override=%d\n",
                frontend_built, frontend_enabled(game.cfg),
                env_get("LEGORRR_LEVEL", "")[0] != '\0');
        fflush(log);
    } else {
        fprintf(log, "frameloop: Lego.cfg not found (wads need mounted game data)\n");
        fflush(log);
    }
    if (frontend_built && audio_status == 0) {
        frontend_set_audio(&fe, &ap);
    }
    {
        const char* environment_level = env_get("LEGORRR_LEVEL", "");
        const char* selected_level = environment_level[0] != '\0'
            ? environment_level : s_command_line_level_override;
        if (selected_level[0] == '\0') selected_level = "Level01";
        snprintf(level_name, sizeof(level_name), "%s", selected_level);
    }

    /* Runtime save/load slots (SaveMenuBuild @ 0x415a10 rows): state from
     * the on-disk slots, grafted onto the SaveLoad screens by the front
     * end. Visible labels 1..5 map to raw save slots 0..4. */
    int& pending_load_slot = app.pending_load_slot;
    int& pending_load_record = app.pending_load_record;
    pending_load_slot = -1;
    pending_load_record = -1;
    {
        FeSlot slots[FE_SLOT_COUNT];
        int sn = 0;
        for (int s = 1; s < SAVE_SLOTS; s++) {
            int filled = 0, completed = 0, resume = -1;
            save_slot_state(&game, frontend_slot_raw_index(s), &filled, &completed, &resume);
            memset(&slots[sn], 0, sizeof(slots[sn]));
            snprintf(slots[sn].label, FE_NAME_MAX, "Save Game %d", s);
            if (resume >= 0) {
                char level[FE_LEVEL_NAME_MAX];
                if (fe_level_for_record(&fe, resume, level, sizeof(level))) {
                    snprintf(slots[sn].status, FE_NAME_MAX,
                             "%s (%d complete)", level, completed);
                }
            } else if (filled > 0) {
                snprintf(slots[sn].status, FE_NAME_MAX,
                         "Saved game (%d records)", filled);
            }
            slots[sn].id = s;
            slots[sn].present = filled > 0;
            slots[sn].completed = completed;
            sn++;
        }
        frontend_build_slots(&fe, slots, sn);
    }
    if (boot_menu) {
        if (audio_status == 0) {
            const char* music = menu_music_resource(game.cfg);
            int music_rc = (music != NULL && music[0] != '\0')
                         ? audio_loop_named(&ap, music, 0.0f) : -1;
            if (music_rc == 0) audio_update(&ap);
            fprintf(log, "frameloop: main-menu music %s -> %s (%d)\n",
                    music != NULL ? music : "(missing SFX_AmbientMusicLoop)",
                    music_rc == 0 ? "queued" : "unavailable", music_rc);
        }
        menu_enter(&fe, 0);
        fprintf(log, "frameloop: front end up — main menu (arrows select, "
                     "Enter/Space act, Esc quits)\n");
        menu_sound_tick_clock_init(&app.menu_sound_clock);
    }
    app.phase = boot_menu ? FRAMELOOP_PHASE_MENU
                          : FRAMELOOP_PHASE_GAMEPLAY_START;
    engine_current_context_bind(&ctx);
    app.initialized = 1;
    return 1;
}

static void frameloop_application_prepare_gameplay(void)
{
    FrameloopApplicationState& app = s_frameloop_application;
    const char* wad_dir = app.wad_dir;
    FILE* log = app.log;
    GameContext& game = *app.game_storage;
    ViewContext (&views)[3] = app.views;
    EngineContext& ctx = app.engine_context;
    PlatformWindow* win = app.window;
    DisplaySurface& surf = app.surface;
    AudioPlayer& ap = app.audio;
    const int audio_status = app.audio_status;
    long& frames = app.frames;
    int& quit = app.quit;
    const int play_movies = app.play_movies;
    char (&level_name)[FE_LEVEL_NAME_MAX] = app.level_name;
    const int boot_menu = app.boot_menu;
    int& pending_load_slot = app.pending_load_slot;
    int& pending_load_record = app.pending_load_record;

    /* Restore the game's ambient loop after leaving the main menu. */
    if (boot_menu && audio_status == 0 && !quit) {
        audio_loop_named(&ap, "Sounds\\Iambloop.wav", 0.0f);
    }

    /* --- Deferred level load (the front end chose a level) ------------------ */
    if (!quit && game.cfg != NULL) {
        frameloop_load_level(log, wad_dir, play_movies, win, &surf, &ap,
                             audio_status, &quit, &game, &ctx, views,
                             level_name);
        if (pending_load_slot >= 0 && game.level != NULL) {
            /* Restore the saved checkpoint over the freshly-seeded task
             * table (the corpus's model is a per-level checkpoint, not an
             * entity dump — finding 08). The slot's records are still live
             * in ctx->records from save_slot_state. */
            uint32_t* fl = save_record_flags(&game, pending_load_record);
            if (fl != NULL) {
                checkpoint_blob_apply(&game, (const uint8_t*)(fl + 1));
                fprintf(log, "frameloop: restored checkpoint slot %d record "
                             "%d\n", pending_load_slot, pending_load_record);
            }
            /* Load-side `.osf` sidecar restore (LevelLoad @ 0x429890 →
             * OsfRead @ 0x459470): bring the slot's completed-objective
             * table back the way the shipped game does. The corpus gates
             * the read on a clean in-memory table (OsfDirtyGet()==0) and
             * an active slot (SavGetSlot()!=-1); the port mirrors it here
             * — after the checkpoint apply the slot is active and the
             * table has nothing unsaved, so the sidecar replaces it. */
            if (game.osf_dirty == 0) {
                (void)osf_read_path(&game, pending_load_slot);
                fprintf(log, "frameloop: restored .osf sidecar slot %d "
                             "(%d completed objective(s))\n",
                        pending_load_slot, game.osf_count);
            }
            pending_load_slot = -1;
        }
    }

    if (!frameloop_briefing_prepare(&app)) {
        quit = 1;
        app.stop_reason = "objective briefing load failed";
    }

    LevelState*& pointer_dispatch_level = app.pointer_dispatch_level;
    int& pointer_dispatch_previous_down = app.pointer_dispatch_previous_down;
    LevelState*& reward_tick_level = app.reward_tick_level;
    int& reward_tick_started = app.reward_tick_started;
    pointer_dispatch_level = NULL;
    pointer_dispatch_previous_down = 0;
    reward_tick_level = NULL;
    reward_tick_started = 0;
    /* Game Speed (issue #85): options_scale_frame stamps the store's factor
     * into ctx.time_scale (EngineTick's subsystem tickers) and returns the
     * scaled dt for the game-layer tickers (ai/objective). */
    /* Interactive proof: the arrow keys pan the blitted texture. */
    int& rock_x = app.rock_x;
    int& rock_y = app.rock_y;
    rock_x = 24;
    rock_y = 24;

}

static int frameloop_application_gameplay_frame(float dt)
{
    FrameloopApplicationState& app = s_frameloop_application;
    const char* wad_dir = app.wad_dir;
    const long frame_cap = app.frame_cap;
    const long autopause = app.autopause;
    FILE* log = app.log;
    GameContext* game_storage = app.game_storage;
    FrontEnd* fe_storage = app.frontend_storage;
    GameContext& game = *app.game_storage;
    FrontEnd& fe = *app.frontend_storage;
    EngineState& state = app.engine_state;
    InputState& input = app.input;
    ViewContext (&views)[3] = app.views;
    EngineContext& ctx = app.engine_context;
    PlatformWindow* win = app.window;
    DisplaySurface& surf = app.surface;
    TextureSurface* rock = app.rock_texture;
    TextureSurface* ui = app.ui_texture;
    AudioPlayer& ap = app.audio;
    const int audio_status = app.audio_status;
    long& frames = app.frames;
    int& thumb_slot = app.thumbnail_slot;
    int& quit = app.quit;
    const char*& stop = app.stop_reason;
    RewardFramePumpContext& reward_frame = app.reward_frame;
    RewardScreenOps& reward_ops = app.reward_ops;
    const int play_movies = app.play_movies;
    char (&level_name)[FE_LEVEL_NAME_MAX] = app.level_name;
    const int boot_menu = app.boot_menu;
    const int frontend_built = app.frontend_built;
    int& pending_load_slot = app.pending_load_slot;
    int& pending_load_record = app.pending_load_record;
    char (&mission_brief)[1024] = app.mission_brief;
    int& mission_brief_active = app.mission_brief_active;
    if (!app.picker_return_pending && !mission_brief_active &&
        objective_briefing_active(game.level)) {
        if (!frameloop_briefing_prepare(&app)) {
            quit = 1;
            stop = "objective briefing load failed";
            return 0;
        }
    }
    TextureSurface* briefing_panel = app.briefing_panel;
    TextureSurface* briefing_title_font = app.briefing_title_font;
    TextureSurface* briefing_body_font = app.briefing_body_font;
    LevelState*& pointer_dispatch_level = app.pointer_dispatch_level;
    int& pointer_dispatch_previous_down = app.pointer_dispatch_previous_down;
    LevelState*& reward_tick_level = app.reward_tick_level;
    int& reward_tick_started = app.reward_tick_started;
    int& rock_x = app.rock_x;
    int& rock_y = app.rock_y;

        /* Begin-frame adapter state is per iteration. The retail bit is
         * cleared by FUN_0047c8d0 after RendererBeginFrameOnce; no renderer
         * device is bound in this portable loop yet. */
        renderer_begin_frame_once_reset();
        /* 1. Device capture for this frame (the finding-05 "writer" half):
         *    platform events -> canonical codes -> input tables. */
        PlatformEvent* evbuf = app.frame_events;
        int nev = app.frame_event_count;
        const int pause_escape =
            frameloop_application_events_track(&app, evbuf, nev);
        frameloop_gameplay_input_capture(&app, evbuf, nev);
        input_cursor_position_update();
        input_cursor_position_sync(&input);
        for (int i = 0; i < nev; ++i) {
            if (evbuf[i].kind == PLATFORM_EVENT_QUIT) {
                quit = 1;
                stop = "closed";
                break;
            }
        }
        if (quit) return 0;
        frame_tutorial_message_owner_update(&game, &input);
        if (frame_cap > 0 && frames >= frame_cap) {
            stop = "autostop";
            quit = 1;
            return 0;
        }

        /* FUN_004589c0 advances all authored pages before acknowledging
         * the pending presentation through FUN_004588e0. Simulation and
         * world input remain held for every page and the acceptance frame. */
        if (mission_brief_active) {
            advisor_message_pointer_cancel(input.mouse_down);
            int key_continue = 0;
            for (int k = 0; k < nev; ++k) {
                unsigned int mask = evbuf[k].code == ' ' ? 1u : 0u;
                if (evbuf[k].kind == PLATFORM_EVENT_KEYDOWN && mask != 0u) {
                    if ((app.mission_brief_keys_held & mask) == 0u)
                        key_continue = 1;
                    app.mission_brief_keys_held |= mask;
                } else if (evbuf[k].kind == PLATFORM_EVENT_KEYUP) {
                    app.mission_brief_keys_held &= ~mask;
                }
            }
            objective_tick_target(NULL, game.level, 0.0f, dt);
            const char* page = mission_brief + app.mission_brief_page;
            const char* next = strchr(page, '\a');
            const int page_action = frameloop_briefing_page_action(
                mission_brief, &app.mission_brief_page, key_continue,
                input_mouse_pressed(&input), input.mouse_x, input.mouse_y,
                app.briefing_back, app.briefing_next);
            if (page_action != 0) {
                if (page_action == 1) {
                    fprintf(log, "frameloop: Mission Brief page changed\n");
                } else if (objective_presentation_acknowledge(game.level)) {
                    mission_brief_active = 0;
                    if (app.mission_brief_kind != OBJECTIVE_PRESENTATION_BRIEFING) {
                        frameloop_level_return_begin(&app);
                        input_begin_frame(&input);
                        return !quit;
                    }
                    objective_score_text_origin_set(NULL);
                    objective_task_text_origin_set(&ctx, NULL);
                    unported_ledger_stubs::ledger_stub_00425c30_TransitionControl(0, 0u);
                    input_cursor_client_position_set(WINDOW_W / 2, WINDOW_H / 2);
                    if (global_task_refresh_flag_get() != 0u)
                        state_flag_toggle_when_zero();
                    fprintf(log, "frameloop: Mission Brief acknowledged; "
                                 "controls live\n");
                }
            } else {
                char page_text[1024];
                size_t length = next != NULL ? (size_t)(next - page)
                                             : strlen(page);
                memcpy(page_text, page, length);
                page_text[length] = '\0';
                frameloop_briefing_render(win, &surf, briefing_panel,
                                          briefing_title_font,
                                          briefing_body_font, page_text,
                                          app.briefing_next, app.briefing_back_arrow,
                                          next != NULL, app.mission_brief_page != 0,
                                          app.briefing_x, app.briefing_y,
                                          app.briefing_title, game.level, &game,
                                          dt * 25.0f, (int)frames);
                if (audio_status == 0) audio_update(&ap);
                ++frames;
                input_begin_frame(&input);
                return !quit;
            }
            ++frames;
            input_pointer_rearm(&input);
            input_begin_frame(&input);
            return !quit;
        }

        /* SaveMenuOpenFromReward is frame-driven in this port: once mode 4
         * opens the slot picker, consume frames here until a slot is saved or
         * the user backs out. The level sim remains paused under the menu. */
        if (fe.open && fe.save_action) {
            MenuSoundTickClock save_sound_clock;
            menu_sound_tick_clock_init(&save_sound_clock);
            const FeSubmenu* fs = frontend_active_submenu(&fe);
            TextureSurface* hi_font = fs != NULL && fs->hifont[0] != '\0'
                                          ? texture_get_or_load(NULL, fs->hifont)
                                          : NULL;
            TextureSurface* lo_font = fs != NULL && fs->lofont[0] != '\0'
                                          ? texture_get_or_load(NULL, fs->lofont)
                                          : NULL;
            FeInput fi;
            fi.up = input_key_edge(&input, PLATFORM_KEY_UP);
            fi.down = input_key_edge(&input, PLATFORM_KEY_DOWN);
            fi.left = fi.right = 0;
            fi.enter = input_key_edge(&input, ' ')
                       || input_key_edge(&input, PLATFORM_KEY_ENTER);
            fi.esc = input_key_edge(&input, PLATFORM_KEY_ESC);
            fi.mouse_x = input.mouse_x;
            fi.mouse_y = input.mouse_y;
            fi.click = input_mouse_pressed(&input);
            fi.mdown = input.mouse_down;
            int tick_menu = fe.menu, tick_sub = fe.sub;
            FeEvent save_ev = frontend_tick(&fe, &fi, hi_font);
            if (save_ev == FE_EVT_SAVE_GAME) {
                save_slot_set(&game, frontend_slot_raw_index(fe.load_slot));
                checkpoint_blob_build(&game);
                save_write_record(&game, game.current_record,
                                  game.checkpoint_blob);
                save_mark_record(&game, game.current_record);
                show_level_overlay_for_current_selection(&game);
                int filled = 0, completed = 0, resume = -1;
                if (save_slot_state(&game, frontend_slot_raw_index(fe.load_slot), &filled, &completed,
                                    &resume)) {
                    FeSlot saved_slot;
                    memset(&saved_slot, 0, sizeof(saved_slot));
                    saved_slot.id = fe.load_slot;
                    saved_slot.present = filled > 0;
                    saved_slot.completed = completed;
                    snprintf(saved_slot.label, sizeof(saved_slot.label),
                             "Save Game %d", fe.load_slot);
                    if (resume >= 0) {
                        char level[FE_LEVEL_NAME_MAX];
                        if (fe_level_for_record(&fe, resume, level,
                                                sizeof(level))) {
                            snprintf(saved_slot.status,
                                     sizeof(saved_slot.status),
                                     "%s (%d complete)", level, completed);
                        }
                    } else if (filled > 0) {
                        snprintf(saved_slot.status,
                                 sizeof(saved_slot.status),
                                 "Saved game (%d records)", filled);
                    }
                    frontend_update_slot(&fe, &saved_slot);
                }
                mission_reward_set_active(&game, 0);
                thumb_slot = frontend_slot_raw_index(fe.load_slot);
                fprintf(log, "frameloop: reward checkpoint saved to slot %d\n",
                        fe.load_slot);
            }
            sound_subsystem_frame_tick(save_sound_clock.dt);
            if (fe.open) {
                frameloop_menu_render(win, &surf, &fe, hi_font, lo_font,
                                      &game, (int)frames);
                if (fe.menu == tick_menu && fe.sub == tick_sub)
                    frontend_scroll_update(&fe, &fi);
            }
            if (audio_status == 0) {
                audio_update(&ap);
            }
            ++frames;
            menu_sound_tick_clock_advance(&save_sound_clock);
            input_begin_frame(&input);
            return !quit;
        }

        /* Keep the departing level alive while its reward or save menu owns
         * it. Advance may replace that level; its new owner must not be
         * released by the pending picker return. */
        if (app.picker_return_pending && game.level != app.picker_return_level) {
            app.picker_return_pending = 0;
            app.picker_return_level = NULL;
        }
        if (app.picker_return_pending && !mission_reward_active(&game)) {
            int origin = app.picker_return_origin;
                char pending_level[FE_LEVEL_NAME_MAX];
                const char* next_level = game.level != NULL
                    ? level_next_name(game.level) : NULL;
                snprintf(pending_level, sizeof(pending_level), "%s",
                         next_level != NULL ? next_level : "");
                checkpoint_blob_free(&game);
                app.picker_return_pending = 0;
                app.picker_return_level = NULL;
                view_primary_level_release(&ctx);
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
                frame_world_arrows_level_releasing();
#endif
                mission_reward_resources_release(&game);
                level_destroy(game.level);
                game.level = NULL;
                ctx.level_block = NULL;
                ctx.level_voxels = NULL;
                ctx.level_terrain = NULL;
                g_camera_grid_enabled = 0;
                pointer_dispatch_level = NULL;
                reward_tick_level = NULL;
                reward_tick_started = 0;
                app.gameplay_prepared = 0;
                app.mission_brief_active = 0;
                app.pending_load_slot = -1;
                if (origin != FE_STATE_MISSIONS && origin != FE_STATE_TRAINING) {
                    /* No picker owns this launch. PendingLevelName follows
                     * its retained next name after the old level's teardown. */
                    if (pending_level[0] == '\0') {
                        quit = 1;
                        stop = "level chain ended";
                        return 0;
                    }
                    options_load_or_default(&game, 1, 0);
                    snprintf(level_name, sizeof(level_name), "%s", pending_level);
                    app.phase = FRAMELOOP_PHASE_GAMEPLAY_START;
                    input_begin_frame(&input);
                    return 1;
                }
                if (!menu_enter(&fe, origin)) {
                    quit = 1;
                    stop = "return picker unavailable";
                    return 0;
                }
                if (audio_status == 0) {
                    const char* music = menu_music_resource(game.cfg);
                    if (music != NULL && music[0] != '\0')
                        (void)audio_loop_named(&ap, music, 0.0f);
                }
                menu_sound_tick_clock_init(&app.menu_sound_clock);
                input_begin_frame(&input);
                app.phase = FRAMELOOP_PHASE_MENU;
                return 1;
        }

        if (app.picker_return_pending && mission_reward_active(&game)) {
            int hit_save = 0;
            int hit_advance = 0;
            if (input_mouse_pressed(&input) &&
                mission_reward_zone(&game, input.mouse_x, input.mouse_y,
                                    &hit_save, &hit_advance)) {
                if (hit_save) {
                    (void)save_menu_open_from_reward(&fe);
                } else if (hit_advance) {
                    checkpoint_step(&game);
                    mission_reward_set_active(&game, 0);
                }
            }
            reward_frame_pump(&reward_frame, 1);
            return !quit;
        }

        /* Esc (or the LEGORRR_AUTOPAUSE_FRAMES CI hook) opens the in-game
         * pause overlay — the shipped PausedMenu — instead of quitting. */
        const int overlay_mode = frameloop_overlay_request_mode(&game, &ctx,
            pause_escape || (autopause > 0 && frames == autopause));
        if (overlay_mode >= 0 && frameloop_overlay_open(&fe, overlay_mode)) {
            timed_effect_hover_reset();
            snprintf(fe.resume_level, sizeof(fe.resume_level), "%s",
                     level_name);
            /* A held gameplay gesture cannot become a menu activation. */
            input_pointer_rearm(&input);
            input_begin_frame(&input);
            FeOverlayMenuGateState overlay_gate;
            memset(&overlay_gate, 0, sizeof(overlay_gate));
            overlay_gate.cached_menu = -1;
            overlay_gate.menus[overlay_mode].entries[0].handle = 1;
            overlay_gate.menus[overlay_mode].entries[2].handle = 1;
            fprintf(log, "frameloop: %s overlay\n",
                    overlay_mode == 1 ? "options" : "paused");
            int want_reload = 0;
            int want_picker = 0;
            MenuSoundTickClock pause_sound_clock;
            menu_sound_tick_clock_init(&pause_sound_clock);
            while (fe.open && !quit) {
                PlatformEvent pev[32];
                int pn = platform_pump_events(win, pev, 32);
                input_capture_events(&input, pev, pn);
                const int pause_escape =
                    frameloop_application_events_track(&app, pev, pn);
                input_cursor_position_update();
                input_cursor_position_sync(&input);
                for (int i = 0; i < pn; ++i) {
                    if (pev[i].kind == PLATFORM_EVENT_QUIT) {
                        quit = 1;
                        stop = "closed";
                        break;
                    }
                }
                if (quit) {
                    break;
                }
                if (frame_cap > 0 && frames >= frame_cap) {
                    quit = 1;
                    stop = "autostop";
                    break;
                }
                const FeSubmenu* fs = frontend_active_submenu(&fe);
                TextureSurface* hi_font = NULL, *lo_font = NULL;
                if (fs != NULL) {
                    if (fs->hifont[0] != '\0') {
                        hi_font = texture_get_or_load(NULL, fs->hifont);
                    }
                    if (fs->lofont[0] != '\0') {
                        lo_font = texture_get_or_load(NULL, fs->lofont);
                    }
                }
                FeInput fi;
                fi.up = input_key_edge(&input, PLATFORM_KEY_UP);
                fi.down = input_key_edge(&input, PLATFORM_KEY_DOWN);
                fi.left = input_key_edge(&input, PLATFORM_KEY_LEFT);
                fi.right = input_key_edge(&input, PLATFORM_KEY_RIGHT);
                fi.enter = input_key_edge(&input, ' ')
                           || input_key_edge(&input, PLATFORM_KEY_ENTER);
                fi.esc = pause_escape;
                fi.mouse_x = input.mouse_x;
                fi.mouse_y = input.mouse_y;
                fi.click = input_mouse_pressed(&input);
                fi.mdown = input.mouse_down;
                FrameloopOverlayMenuContext gate_ctx;
                gate_ctx.fe = &fe;
                gate_ctx.game = &game;
                gate_ctx.gate = &overlay_gate;
                gate_ctx.input = &fi;
                gate_ctx.measure = hi_font;
                gate_ctx.event = FE_EVT_NONE;
                gate_ctx.mode = overlay_mode;
                overlay_gate.level_start_flag = level_start_flag();
                overlay_gate.options_flag = menu_options_flag();
                overlay_gate.blocked =
                    global_option_low_bits_enabled(game.level);
                int tick_menu = fe.menu, tick_sub = fe.sub;
                (void)overlay_menu_gate(&overlay_gate, overlay_mode,
                                        frameloop_overlay_menu_scroll_refresh,
                                        frameloop_overlay_menu_tick, &gate_ctx);
                FeEvent ev = gate_ctx.event;
                if (fe.option_changed) {
                    const FeItem* oit = frontend_active_item(&fe);
                    if (oit != NULL) {
                        int is_sfx = s_icontains(oit->label, "sfx")
                                     || s_icontains(oit->label, "sound");
                        int is_music = s_icontains(oit->label, "music");
                        int live = oit->kind == FE_ITEM_SLIDER
                                   && (is_sfx || is_music) && oit->max > 0;
                        if (live && ap.dev != NULL) {
                            if (is_sfx) {
                                audio_menu_sfx_volume_set(oit->value);
                            } else {
                                audio_menu_volume_set(oit->value);
                            }
                        }
                        fprintf(log, "frameloop: option %s -> %d/%d (%s)\n",
                                oit->label, oit->value, oit->max,
                                live ? "audio" : "store — applied live");
                        fe.option_changed = 0;
                    }
                }
                if (ev == FE_EVT_LEAVE_LEVEL) {
                    want_picker = 1;
                    break;
                }
                if (ev == FE_EVT_RESUME) {
                    break;
                }
                if (ev == FE_EVT_START_LEVEL) {
                    want_reload = 1;
                    break;
                }
                sound_subsystem_frame_tick(pause_sound_clock.dt);
                frameloop_menu_render(win, &surf, &fe, hi_font, lo_font,
                                      &game, (int)frames);
                if (fe.open && fe.menu == tick_menu && fe.sub == tick_sub)
                    frontend_scroll_update(&fe, &fi);
                Sleep((DWORD)FRAME_MS);
                ++frames;
                menu_sound_tick_clock_advance(&pause_sound_clock);
                input_begin_frame(&input);
            }
            if (overlay_mode == 1) {
                engine_debug_word_bit_set(0x100u, 0);
                if (!quit && global_option_low_bits_enabled(game.level))
                    ctx.state |= 0x2000000u;
            } else {
                ctx.state &= ~0x2000000u;
            }
            input_pointer_rearm(&input);
            frameloop_pause_finish(&game, &ctx, want_reload, want_picker, quit);
            if (want_picker && !quit) {
                frameloop_level_return_begin(&app);
                input_begin_frame(&input);
                return !quit;
            }
            if (want_reload && !quit) {
                /* Pause -> Restart_Mission: reload the level in place. */
                fprintf(log, "frameloop: restarting level %s\n", level_name);
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
                frame_world_arrows_level_releasing();
#endif
                level_destroy(game.level);
                game.level = NULL;
                pointer_dispatch_level = NULL;
                frameloop_load_level(log, wad_dir, play_movies, win, &surf,
                                     &ap, audio_status, &quit, &game, &ctx,
                                     views, level_name);
                /* The corpus re-enters LevelLoad for this reload through
                 * the in-game callback registered by FUN_0041fa20
                 * (FUN_0041fb50 calls FUN_00429890), so the `.osf` sidecar
                 * read runs again under the same gate: clean table +
                 * active slot -> re-read; dirty -> skipped and the
                 * unsaved live table survives untouched. */
                if (game.level != NULL && game.osf_dirty == 0
                    && save_slot_get(&game) >= 0) {
                    (void)osf_read_path(&game, save_slot_get(&game));
                }
            }
            if (!quit && game.level == NULL) {
                /* not restarting; Esc from the pause root resumes */
                fprintf(log, "frameloop: resumed\n");
            }
            /* The pause consumed this whole frame. Bound the next supplied
             * delta by time since this return, because the outer clock was
             * sampled before entering the modal loop (and may have clamped
             * away the original elapsed interval). */
            if (!quit || game.level != NULL) {
                app.gameplay_mouse_suppressed = app.pause_mouse_down;
                advisor_message_pointer_cancel(app.pause_mouse_down);
                pointer_dispatch_previous_down = 0;
                (void)pointer_action_state_dispatch(0, 0, 0, 0, 0);
                input_pointer_rearm(&input);
                input_begin_frame(&input);
                if (app.gameplay_mouse_suppressed) input.mouse_down = 0;
                app.gameplay_resume_ms = platform_time_milliseconds();
                app.gameplay_clock_rebase = 1;
                return !quit;
            }
        }

        /* 2. Fine-grained pan via key edges (cur vs prev). */
        if (input_key_edge(&input, PLATFORM_KEY_UP))    { rock_y -= 8; }
        if (input_key_edge(&input, PLATFORM_KEY_DOWN))  { rock_y += 8; }
        if (input_key_edge(&input, PLATFORM_KEY_LEFT))  { rock_x -= 8; }
        if (input_key_edge(&input, PLATFORM_KEY_RIGHT)) { rock_x += 8; }
        /* Space re-triggers the SFX (interactive audio proof). */
        if (ap.dev != NULL && input_key_edge(&input, ' ')) {
            audio_play_named(&ap, "Sounds\\Buildings\\elec5.wav");
        }
        /* '[' / ']' adjust the master volume (FUN_00464ef0 analog). */
        if (ap.dev != NULL) {
            if (input_key_edge(&input, '[')) {
                audio_set_volume(&ap, ap.volume - 0.1f);
                fprintf(log, "frameloop: volume %0.1f\n", ap.volume);
            }
            if (input_key_edge(&input, ']')) {
                audio_set_volume(&ap, ap.volume + 0.1f);
                fprintf(log, "frameloop: volume %0.1f\n", ap.volume);
            }
            /* The looping ambient's pan follows the blitted texture — the
             * texture is the sound emitter in this demo. */
            float pan = (2.0f * (float)rock_x / (float)WINDOW_W) - 1.0f;
            audio_set_loop_pan(&ap, pan);
            audio_update(&ap);

            /* demo: air drains every ~2 s so the air-beat loop + the
             * 0x11/0x12 messages fire, and the air loop audibly replaces
             * the ambient. */
            if (frames > 0 && frames % 60 == 0 && state.air_beat > 0) {
                state.air_beat -= 1;
                fprintf(log, "frameloop: air=%u airbeat_loop=%d\n",
                        state.air_beat, airbeat_loop_active());
            }
        }
        /* Game layer proof-of-life:
         *  'S' — build the checkpoint blob + commit it to the active slot
         *  'L' — run CheckpointStep (bootstrap slot 5 + write-through)      */
        if (game.level != NULL && input_key_edge(&input, 's')) {
            checkpoint_blob_build(&game);
            save_write_record(&game, game.current_record,
                              game.checkpoint_blob);
            save_mark_record(&game, game.current_record);
            save_commit(&game);
            fprintf(log, "frameloop: saved blob to slot %d record %d (0x%x bytes)\n",
                    SAVE_DEFAULT_SLOT, game.current_record, SAVE_BLOB_SIZE);
        }
        if (game.level != NULL && input_key_edge(&input, 'l')) {
            game.save_slot = -1; /* force the bootstrap path */
            checkpoint_step(&game);
            fprintf(log, "frameloop: checkpoint_step committed slot %d record %d\n",
                    SAVE_DEFAULT_SLOT, game.current_record);
        }
        /* Mission live-progress keys (finding 09 bridge):
         *  'c' +1 crystal   'o' +1 ore   '+' +10 score
         * These drive the objective checker and task completions in lieu
         * of the (future) entity sim. */
        if (game.level != NULL && input_key_edge(&input, 'c')) {
            mission_set_crystals(&game, game.mission.crystals + 1);
        }
        if (game.level != NULL && input_key_edge(&input, 'o')) {
            mission_set_ore(&game, game.mission.ore + 1);
        }
        if (game.level != NULL && input_key_edge(&input, '+')) {
            mission_add_score(&game, 10);
        }

        /* Selection / pointer input (finding 06): the tracked mouse maps to
         * a grid cell; 1-4 pick the nearest placed entity, 'm' moves the
         * selection there, 'n' clears. Commands are produced into the
         * double-buffered queue and dispatched by cmd_queue_tick below. */
        if (game.level != NULL) {
            FramePointerDispatchContext pointer_context;
            SimulationPointerDispatchOps pointer_ops;
            UiActionDispatchOps ui_action_ops;
            static UiActionDispatchState ui_action_state;
            if (pointer_dispatch_level != game.level) {
                /* ObjectiveSelectionStateReset enables the target's 0x4/0x80
                 * pointer gate. The first inactive dispatch seeds its 0x4a
                 * action sentinel before hover samples begin. */
                objective_selection_state_reset();
                (void)pointer_action_state_dispatch(0, 0, 0, 0, 0);
                memset(&ui_action_state, 0, sizeof(ui_action_state));
                pointer_dispatch_level = game.level;
                pointer_dispatch_previous_down = 0;
                input_pointer_rearm(&input);
                simulation_pointer_capture_reset(&app.pointer_captures[0]);
                simulation_pointer_capture_reset(&app.pointer_captures[1]);
                app.pointer_capture_serial = input.pointer_reset_serial;
            }
            pointer_context.game = &game;
            pointer_context.level = game.level;
            pointer_context.engine = &ctx;
            pointer_context.screen_width = WINDOW_W;
            pointer_context.screen_height = WINDOW_H;
            pointer_context.ui_action_state = &ui_action_state;
            memset(&pointer_ops, 0, sizeof(pointer_ops));
            memset(&ui_action_ops, 0, sizeof(ui_action_ops));
            pointer_ops.context = &pointer_context;
            pointer_ops.feature_word = ctx.debug;
            /* The existing arbiter owns the whole gesture. Resolve the
             * authored catalog after the dependency-grid UI, before world
             * input, using the same sliding layout and images as rendering. */
            frontend_action_catalog_pointer_setup(&pointer_ops);
            pointer_ops.overlay_fallback = frame_action_catalog_hit;
            pointer_ops.action_count = frame_ui_action_count;
            pointer_ops.action_complete = frame_ui_action_complete;
            ui_action_ops.context = &pointer_context;
            ui_action_ops.current_selection_get =
                frame_ui_action_current_selection;
            ui_action_ops.action_already_held =
                frame_ui_action_already_held;
            ui_action_ops.entity_flags_get = frame_ui_action_entity_flags;
            ui_action_ops.entity_byte_flags_get =
                frame_ui_action_entity_byte_flags;
            ui_action_ops.entity_type_get = frame_ui_action_entity_type;
            ui_action_ops.selected_entity_predicate =
                frame_ui_action_selected_entity_predicate;
            ui_action_ops.selected_cell_flags_get =
                frame_ui_action_cell_flags;
            ui_action_ops.terrain_cell_command_02510 =
                frame_ui_action_cell_command_02510;
            ui_action_ops.terrain_cell_command_03030 =
                frame_ui_action_cell_command_03030;
            ui_action_ops.terrain_cell_command_02ae0 =
                frame_ui_action_cell_refresh_02ae0;
            ui_action_ops.terrain_cell_to_world_position =
                frame_ui_action_terrain_cell_to_world;
            ui_action_ops.terrain_cell_command_02d50 =
                frame_ui_action_cell_command_02d50;
            ui_action_ops.terrain_cell_command_02fb0 =
                frame_ui_action_cell_command_02fb0;
            ui_action_ops.ui_mode_request = frame_ui_action_mode_request;
            ui_action_ops.input_request_enqueue =
                frame_ui_action_input_request;
            ui_action_ops.selected_map_action_enqueue =
                frame_ui_action_map_request;
            ui_action_ops.selection_clear_request_and_hud_reset =
                frame_ui_action_selection_clear;
            ui_action_ops.ui_action_helper_00452be0 =
                frame_ui_action_helper_00452be0;
            ui_action_ops.entity_kind_command_issue =
                frame_ui_action_entity_kind_command;
            ui_action_ops.entity_action_kind_create =
                frame_ui_action_entity_action_kind_create;
            ui_action_ops.entity_flag_80000000_set_clear =
                frame_ui_action_entity_flag_80000000;
            ui_action_ops.selected_entity_action_02ec0 =
                frame_ui_action_conditional_task_schedule;
            ui_action_ops.selected_entities_detach_or_pose =
                frame_ui_action_selected_entities_detach_or_pose;
            ui_action_ops.selected_entity_action_21 =
                frame_ui_action_selected_entity_action_21;
            ui_action_ops.entity_action_record_find =
                frame_ui_action_record_find;
            ui_action_ops.entity_action_record_apply =
                frame_ui_action_record_apply;
            ui_action_ops.object_command_task_create =
                frame_ui_action_object_command_task_create;
            ui_action_ops.selected_entity_field_f6_clear =
                frame_ui_action_entity_field_f6_clear;
            ui_action_ops.cursor_action_rejected =
                frame_ui_action_cursor_action_rejected;
            ui_action_ops.tutorial_path_descriptor_create =
                frame_ui_action_tutorial_path_descriptor_create;
            ui_action_ops.selected_cell_reveal_mask_clear =
                frame_ui_action_cell_mask_clear;
            ui_action_ops.terrain_cell_update = frame_ui_action_cell_update;
            ui_action_ops.terrain_descriptor_tiles_reset_and_free =
                frame_ui_action_terrain_descriptor_reset;
            ui_action_ops.selection_group_clear =
                frame_ui_action_selection_group_clear;
            ui_action_ops.global_state_low_byte_clear_and_refresh =
                frame_ui_action_global_flags_clear;
            ui_action_ops.cursor_action_pair_request =
                frame_ui_action_cursor_request;
            ui_action_ops.building_upgrade_commit =
                frame_ui_action_building_upgrade_commit;
            ui_action_state.ops = &ui_action_ops;
            /* This synchronous installation supplies real UI callbacks for
             * every ordered edge. The dispatcher return chooses UI or world
             * ownership; no second raw mouse_press path may bypass it. */
            simulation_pointer_dispatch_ops_set(&pointer_ops);
#if defined(LEGORRR_BACKEND_WINAPI)
            frame_pointer_ingress(&game, &ctx, &input, app.pointer_captures,
                &app.pointer_capture_serial, WINDOW_W, WINDOW_H);
#else
            /* Native still supplies untagged aggregate events. Preserve its
             * existing adapter without inventing physical button identity. */
            int native_ui_consumed = 0;
            {
                /* Native has no physical button identity, so the panel
                 * pass uses the aggregate level as its logical button. It
                 * provides hover/press feedback, owns the gesture against
                 * world clicks, and feeds the held camera-zoom buttons. */
                static FrontendPanelPointerCapture native_panel;
                FrontendPanelPointerHit native_hit;
                const int native_enabled =
                    (ctx.state & (0x100u | 0x100000u | 0x08000000u)) == 0u &&
                    (ctx.debug & 0x1000u) == 0u &&
                    (ctx.state & ENGINE_STATE_INDEXED_SUBSYS) != 0u;
                if (native_enabled) {
                    if (frontend_panels_pointer_update(&native_panel,
                            input.mouse_x, input.mouse_y, input.mouse_down,
                            pointer_dispatch_previous_down, &native_hit)) {
                        ctx.state |= 0x200u;
                        native_ui_consumed = 1;
                    }
                    if (native_panel.active) native_ui_consumed = 1;
                    frame_panel_zoom_hold_capture(input.mouse_down,
                                                  &native_hit);
                } else {
                    frontend_panels_pointer_capture_cancel(&native_panel);
                }
            }
            if (!native_ui_consumed &&
                (ctx.state & 0x100u) == 0u && (ctx.debug & 0x1000u) == 0u)
                native_ui_consumed = simulation_pointer_action_dispatch(
                    input.mouse_x, input.mouse_y, input.mouse_down,
                    pointer_dispatch_previous_down,
                    pointer_dispatch_previous_down && !input.mouse_down,
                    ctx.views[0]);
#endif
            simulation_pointer_dispatch_ops_set(NULL);
            pointer_dispatch_previous_down = input.mouse_down;
            int lw = level_width(game.level);
            int lh = level_height(game.level);
            float cx = 0.0f, cy = 0.0f;
            int mouse_on_terrain = level_scene_mouse_cell(
                &game, game.level, WINDOW_W, WINDOW_H,
                input.mouse_x, input.mouse_y,
                &cx, &cy);
            if (!mouse_on_terrain) {
                cx = lw > 0 ? (float)input.mouse_x / (float)WINDOW_W * (float)lw : 0.0f;
                cy = lh > 0 ? (float)input.mouse_y / (float)WINDOW_H * (float)lh : 0.0f;
            }
            /* MainSimInteractionAndCellUpdate calls this dispatcher after
             * resolving the current cell coordinates, under debug bit 0x10. */
            level_debug_input_cell_mutator_dispatch(
                &ctx, &game, &input, (int)cx, (int)cy);

#if !defined(LEGORRR_BACKEND_WINAPI)
            /* Existing native aggregate world adapter; native ingress is
             * outside this Win32 repair. */
            if (!native_ui_consumed && input_mouse_pressed(&input)) {
                EntitySlot* hit = level_scene_pick_entity(
                    &game, game.level, WINDOW_W, WINDOW_H,
                    input.mouse_x, input.mouse_y, 30.0f);
                if (hit != NULL) {
                    selection_clear(&game);
                    selection_add(&game, hit);
                    fprintf(log, "frameloop: selected Raider %s\n", hit->name);
                } else if (mouse_on_terrain && selection_has_any(&game)) {
                    int n = selection_cmd_move(&game, cx, cy);
                    fprintf(log, "frameloop: click move %d selected -> "
                                 "cell (%.1f,%.1f)\n", n, cx, cy);
                }
            }

#endif

            if (input_key_edge(&input, 'n')) {
                selection_clear(&game);
                fprintf(log, "frameloop: selection cleared\n");
            }
            for (int t = 1; t <= 4; ++t) {
                char k = (char)('0' + t);
                if (input_key_edge(&input, k)) {
                    EntitySlot* hit = selection_pick(&game, cx, cy, 3.0f);
                    if (hit != NULL && (hit->type == t || hit->etype == t)) {
                        selection_add(&game, hit);
                        fprintf(log, "frameloop: picked %s (type %d) at cell "
                                     "(%.1f,%.1f)\n", hit->name, hit->type,
                                hit->x, hit->y);
                    } else if (hit != NULL) {
                        selection_clear(&game);
                        selection_add(&game, hit);
                        fprintf(log, "frameloop: picked nearest %s (type %d)\n",
                                hit->name, hit->type);
                    }
                }
            }
            if (input_key_edge(&input, 'm')) {
                int n = selection_cmd_move(&game, cx, cy);
                fprintf(log, "frameloop: move %d selected -> cell (%.1f,%.1f)\n",
                        n, cx, cy);
            }
            /* 'g' — issue a task (opcode 0x17) for every selected entity;
             * the AI receivers run it and deposit into the mission
             * counters (crystal/ore kinds by entity priority). */
            if (input_key_edge(&input, 'g')) {
                int n = 0;
                for (int i = 0; i < game.selection_count; ++i) {
                    cmd_enqueue(0x17, game.selection[i], 0, 0, 0);
                    ++n;
                }
                fprintf(log, "frameloop: gather task enqueued for %d selected\n",
                        n);
            }
            /* 'y' toggles the reward overlay manually (finding 09). */
            if (input_key_edge(&input, 'y')) {
                mission_reward_set_active(&game,
                                          !mission_reward_active(&game));
            }
        }
        if (rock_x < 0) { rock_x = 0; }
        if (rock_y < 0) { rock_y = 0; }
        if (rock_x > WINDOW_W - 64) { rock_x = WINDOW_W - 64; }
        if (rock_y > WINDOW_H - 64) { rock_y = WINDOW_H - 64; }

        /* Mission plumbing: per-frame progress bridge, AI receivers, and
         * objective state machine (findings 09/10/22). */
        if (game.level != NULL) {
            if (reward_tick_level != game.level) {
                reward_tick_level = game.level;
                reward_tick_started = 0;
            }
            float sdt = options_scale_frame(&game.options, &ctx, dt);
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
            if ((ctx.state & ENGINE_STATE_TIME_FROZEN) == 0u)
                building_scene_tick(&game, sdt);
#endif
            int ai_done = ai_tick(&game, sdt);
            if (ai_done != 0) {
                fprintf(log, "frameloop: ai commands completed: %d\n", ai_done);
            }
            int done = mission_update(&game);
            if (done != 0) {
                fprintf(log, "frameloop: tasks completed this frame (mask 0x%x)\n",
                        done);
            }
            int resolved = objective_tick(game.level, sdt);
            if (resolved == 0) {
                fprintf(log, "frameloop: objective resolved (word 0x%08x)\n",
                        game.level->objective);
                /* Advisor hint runtime (finding 22): the stream text the
                 * announcer built; the sound is gated on the outcome's
                 * suppression bits. */
                const char* adv = objective_stream(game.level);
                if (adv[0] != '\0') {
                    fprintf(log, "frameloop: advisor: %s\n", adv);
                }
                if (objective_advisor_shown(game.level) && ap.dev != NULL) {
                    audio_play_named(&ap, "Sounds\\Buildings\\elec5.wav");
                }
                /* Readiness owns an authored terminal presentation. Rewards
                 * and level teardown follow its final-page acknowledgment. */
                if (!frameloop_briefing_prepare(&app)) {
                    quit = 1;
                    stop = "objective result text load failed";
                }
                input_begin_frame(&input);
                return !quit;
            }
            /* Reward-screen buttons (finding 09): a real mouse press inside
             * the Save/Advance zone commits the checkpoint. 'b' is the
             * keyboard fallback (headless). */
            if (!mission_reward_active(&game)) reward_tick_started = 0;
            if (mission_reward_active(&game)) {
                if (!reward_tick_started) {
                    /* The enclosing gameplay frame supplied this input
                     * snapshot already. Start the modal sequence on a fresh
                     * edge boundary; its pump owns subsequent capture and
                     * input_begin_frame calls until the task reveal ends. */
                    input_begin_frame(&input);
                    reward_tick_started = 1;
                    (void)reward_screen_tick_bound();
                    if (reward_frame.zone_action_consumed) {
                        input_begin_frame(&input);
                        reward_frame.zone_action_consumed = 0;
                    }
                }
                int hit_save = 0, hit_advance = 0;
                int clicked = input_mouse_pressed(&input);
                if (clicked
                    || (input_key_edge(&input, 'b')
                        && mission_reward_zone(&game, input.mouse_x,
                                               input.mouse_y, &hit_save,
                                               &hit_advance))) {
                    if (mission_reward_zone(&game, input.mouse_x,
                                            input.mouse_y, &hit_save,
                                            &hit_advance)) {
                        if (hit_save) {
                            if (save_menu_open_from_reward(&fe)) {
                                fprintf(log, "frameloop: reward Save clicked -> "
                                             "save-slot menu opened\n");
                            }
                        } else {
                            const char* nm = mission_reward_advance(&game);
                            if (nm != NULL && app.picker_return_pending) {
                                app.picker_return_pending = 0;
                                app.picker_return_level = NULL;
                            }
                            fprintf(log, "frameloop: reward Advance clicked -> "
                                         "%s\n",
                                    nm != NULL ? nm : "campaign end");
                        }
                    }
                }
            }
        }

#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
        frame_world_arrows_begin_frame();
#endif
        frame_camera_target_registry_update(&ctx, &game);
        frame_panel_zoom_hold_apply(&ctx, dt);
        if (engine_callback_triplet_tick_dispatch(dt) == 0) {
            quit = 1;
            stop = "tick callback";
        }
        screen_effects_tick(dt);

        /* Retail MainLoop calls RendererBeginFrameOnce after the registered
         * per-frame tick callback returns and before its next frame-time
         * calculation. EngineTick is that callback in this portable loop. */
        (void)renderer_begin_frame_once();
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
        /* Allocation failure keeps the last displayed scene/projection.
         * Logical phases and terrain bookkeeping still advanced once. */
        if (!s_frame_world_arrow_display_ready) {
            terrain_camera_detail_restore(game.level);
            input_begin_frame(&input);
            return !quit;
        }
#endif

        void* pixels = NULL;
        int   pitch = 0;
        if (surface_lock(&surf, &pixels, &pitch) == 0) {
            if (game.level != NULL) {
                level_render_scene(game.level, &game, WINDOW_W, WINDOW_H,
                                   pixels, pitch);
                render_apply_brightness(pixels, pitch, WINDOW_W, WINDOW_H,
                                        game.options.brightness);
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
                frame_world_arrows_draw(&game, pixels, pitch,
                                         WINDOW_W, WINDOW_H);
#endif
            } else {
                surface_fill(pixels, pitch, WINDOW_W, WINDOW_H, 8, 16, 48);
                surface_draw_sweep(pixels, pitch, WINDOW_W, WINDOW_H,
                                   (int)frames);
                render_apply_brightness(pixels, pitch, WINDOW_W, WINDOW_H,
                                        game.options.brightness);
                if (rock != NULL) {
                    surface_blit_texture(pixels, pitch, WINDOW_W, WINDOW_H,
                                         rock, rock_x, rock_y);
                }
                if (ui != NULL) {
                    surface_blit_texture(pixels, pitch, WINDOW_W, WINDOW_H,
                                         ui, rock_x + 64 + 8, rock_y);
                }
            }
            if (game.level != NULL && !mission_reward_active(&game)) {
                frontend_panels_render(ctx.state, pixels, pitch,
                                       WINDOW_W, WINDOW_H);
                if ((engine_debug_word_peek() & 0x1000u) == 0u)
                    frontend_action_catalog_images_render(&ctx, &game,
                        pixels, pitch, WINDOW_W, WINDOW_H);
            }
            /* Help Window (issue #85): the option toggles the shipped help
             * panel over the level; the advisor stream text leads when the
             * announcer has one. frontend_help_overlay holds the On/Off
             * gate — this call is unconditional. */
            if (game.level != NULL) {
                frontend_help_overlay(pixels, pitch, WINDOW_W, WINDOW_H,
                                      objective_stream(game.level),
                                      &game.options);
            }
            /* The atlas renderer locks the active target for each glyph;
             * finish the scene lock before entering the real message worker. */
            surface_unlock(&surf, pixels);
            /* The primary clear queue remains live through actual ground
             * and arrow draw. Restore before any later UI lock can fail. */
            terrain_camera_detail_restore(game.level);
            frame_tutorial_message_owner_update(&game, &input);

            if (game.level != NULL && !mission_reward_active(&game))
                advisor_message_ui_draw(dt * 25.0f);
            if (surface_lock(&surf, &pixels, &pitch) != 0) {
                input_begin_frame(&input);
                return !quit;
            }
            if (game.level != NULL && !mission_reward_active(&game)) {
                FrameTooltipDrawContext tooltip_context;
                TimedEffectSlotRenderHooks tooltip_hooks;
                memset(&tooltip_hooks, 0, sizeof(tooltip_hooks));
                tooltip_context.pixels = pixels;
                tooltip_context.pitch = pitch;
                tooltip_context.width = WINDOW_W;
                tooltip_context.height = WINDOW_H;
                tooltip_hooks.context = &tooltip_context;
                tooltip_hooks.overlay_hooks.context = &tooltip_context;
                tooltip_hooks.overlay_hooks.region_fill = frame_tooltip_fill;
                tooltip_hooks.overlay_hooks.polyline_draw = frame_tooltip_lines;
                tooltip_hooks.render_text = frame_tooltip_text;
                timed_effect_slot_render_hooks_set(&tooltip_hooks);
                timed_effect_runtime_update((uint32_t)input.mouse_x,
                                            (uint32_t)input.mouse_y, dt * 25.0f);
                timed_effect_slot_render_hooks_set(NULL);
            }
            pointer_cursor_draw_pixels((uint32_t)input.mouse_x,
                (uint32_t)input.mouse_y, pixels, pitch, WINDOW_W, WINDOW_H);
            platform_screenshot(win, pixels, pitch, WINDOW_W, WINDOW_H,
                                (int)frames);
            platform_present_pixels(win, pixels, pitch, WINDOW_W, WINDOW_H);
            if (thumb_slot >= 0) {
                /* Save-confirm thumbnail (FUN_0047e440): dump the frame we
                 * just presented to Saves\<slot>.dat as a 24bpp BMP. */
                save_write_thumbnail(&game, thumb_slot, pixels, pitch,
                                     WINDOW_W, WINDOW_H);
                frontend_save_thumbnail_invalidate(&fe, thumb_slot + 1);
                fprintf(log, "frameloop: .dat thumbnail written (slot %d)\n",
                        thumb_slot);
                thumb_slot = -1;
            }
            surface_unlock(&surf, pixels);
        }
        /* Also restore when the first surface lock failed/no draw occurred. */
        terrain_camera_detail_restore(game.level);
        ++frames;

        /* 3. Frame boundary: this frame's keys become prev; cur clears for
         *    the next capture (edge semantics, findings 04/05). */
        input_begin_frame(&input);
        return !quit;
}

int frameloop_application_tick(float dt)
{
    FrameloopApplicationState& app = s_frameloop_application;
    if (!app.initialized) return 0;
    (void)frameloop_application_quit_ingest(&app, app.frame_events,
                                           app.frame_event_count);
    if (app.phase == FRAMELOOP_PHASE_DONE || app.quit)
        return 0;
    app.frame_dt = dt;
    if (app.gameplay_clock_rebase) {
        const uint32_t elapsed = platform_time_milliseconds()
                                 - app.gameplay_resume_ms;
        const float resumed_dt = (float)elapsed * 0.001f;
        /* Preserve a smaller supplied delta, including pause/fixed-step
         * values. This changes only the first tick after the modal return. */
        if (dt > resumed_dt) dt = resumed_dt;
        app.gameplay_clock_rebase = 0;
    }
    if (app.phase == FRAMELOOP_PHASE_MENU) {
        FeEvent event = frameloop_main_menu_frame(&app, dt);
        (void)frameloop_application_quit_ingest(&app, NULL, 0);
        if (app.quit) {
            app.phase = FRAMELOOP_PHASE_DONE;
            return 0;
        }
        if (event == FE_EVT_START_LEVEL || event == FE_EVT_LOAD_GAME) {
            app.phase = FRAMELOOP_PHASE_GAMEPLAY_START;
        } else if (app.frame_cap > 0 && app.frames >= app.frame_cap) {
            app.stop_reason = "autostop";
            app.quit = 1;
            app.phase = FRAMELOOP_PHASE_DONE;
            return 0;
        }
        input_begin_frame(&app.input);
        return 1;
    }
    if (app.phase == FRAMELOOP_PHASE_GAMEPLAY_START) {
        if (!app.gameplay_prepared) {
            frameloop_application_prepare_gameplay();
            app.gameplay_prepared = 1;
        }
        (void)frameloop_application_quit_ingest(&app, NULL, 0);
        if (app.quit) {
            app.phase = FRAMELOOP_PHASE_DONE;
            return 0;
        }
        app.phase = FRAMELOOP_PHASE_GAMEPLAY;
    }
    if (app.phase == FRAMELOOP_PHASE_GAMEPLAY) {
        const int running = frameloop_application_gameplay_frame(dt);
        (void)frameloop_application_quit_ingest(&app, NULL, 0);
        if (running && !app.quit) return 1;
    }
    app.phase = FRAMELOOP_PHASE_DONE;
    return 0;
}

void frameloop_application_events_set(const PlatformEvent* events, int count)
{
    FrameloopApplicationState& app = s_frameloop_application;
    int copy_count = count;
    if (copy_count < 0) copy_count = 0;
    if (copy_count > 32) copy_count = 32;
    if (events == NULL) copy_count = 0;
    if (copy_count > 0) {
        memcpy(app.frame_events, events,
               sizeof(PlatformEvent) * (size_t)copy_count);
    }
    app.frame_event_count = copy_count;
}

PlatformWindow* frameloop_application_window_get(void)
{
    return s_frameloop_application.window;
}

void frameloop_application_shutdown(void)
{
    FrameloopApplicationState& app = s_frameloop_application;
    if (!app.initialized) return;
    GameContext& game = *app.game_storage;
    FrontEnd& fe = *app.frontend_storage;
    EngineContext& ctx = app.engine_context;
    FILE* log = app.log;
    DisplaySurface& surf = app.surface;
    PlatformWindow* win = app.window;
    GameContext* game_storage = app.game_storage;
    FrontEnd* fe_storage = app.frontend_storage;
    app.phase = FRAMELOOP_PHASE_DONE;
    fprintf(log, "frameloop: %ld frames, gauged state=0x%08x, tickers=%d, "
            "exit=%s\n", app.frames, ctx.state, ctx.tickers_ran,
            app.stop_reason);
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
    actor_scene_require_stopped();
    building_scene_require_stopped();
#endif
    engine_callback_triplet_shutdown_dispatch();
    reward_screen_runtime_bind(NULL, NULL);
    sound_subsystem_stop_all();
    audio_active_shutdown();
    audio_system_player_set(NULL);
    tooltip_software_shutdown();
    graphics_overlay_resources_release();
    indexed_surface_resource_hooks_set(NULL, NULL, NULL, NULL, NULL);
    texture_release(app.briefing_panel);
    texture_release(app.briefing_next);
    texture_release(app.briefing_back);
    texture_release(app.briefing_back_arrow);
    texture_cache_clear();
    surface_active_render_target_set(NULL);
    input_direct_state_bind(NULL);
    surface_destroy(&surf);
    platform_destroy_window(win);
    frontend_close(&fe);
    sav_tree_roots_clear();
    catalog_slot_flag_hooks_set(NULL, NULL, NULL, NULL);
    input_action_state_query_set(NULL, NULL);
    frameloop_game_context_dispose(&game);
    free(fe_storage);
    engine_current_context_bind(NULL);
    free(game_storage);
    if (log != stderr) fclose(log);
    memset(&app, 0, sizeof(app));
}

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
int LegorrrRun(void)
{
    FrameloopApplicationState& app = s_frameloop_application;
    PlatformEvent events[32];
    const int init_result = frameloop_application_init();
    if (!app.initialized) return init_result;
    while (1) {
        int event_count = platform_pump_events(app.window, events, 32);
        frameloop_application_events_set(events, event_count);
        if (!frameloop_application_tick(app.frame_dt)) break;
        Sleep((DWORD)FRAME_MS);
    }
    frameloop_application_shutdown();
    return 0;
}
#endif /* !LEGORRR_BACKEND_WINAPI */

namespace unported_ledger_stubs {

// MainLoopFramePump
void ledger_stub_00477f30_MainLoopFramePump(int draw_projection_overlay)
{
    main_loop_frame_pump(draw_projection_overlay);
}

// DisplayDeviceCapsQueryDiscard
void ledger_stub_00477f80_DisplayDeviceCapsQueryDiscard()
{
    window_runtime_display_caps_query();
}

// WindowRectAdjustForWindowedMode
void ledger_stub_00478310_WindowRectAdjustForWindowedMode(WindowPortRect* rect)
{
    window_runtime_adjust_rect(rect);
}

} // namespace unported_ledger_stubs
