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
#include "game/ui_action_dispatch.hpp"
#include "game/upgrades.hpp"
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

#define WINDOW_W 640
#define WINDOW_H 480
#define FRAME_MS 33 /* ~30 fps */

typedef struct MenuSoundTickClock {
    uint32_t last_ms;
    float dt;
} MenuSoundTickClock;

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
static WindowRuntimeState s_window_runtime_state;
static WindowDefaultProc s_window_default_proc;
static void* s_window_default_proc_context;

typedef struct RewardFramePumpContext {
    PlatformWindow* window;
    DisplaySurface* surface;
    GameContext* game;
    InputState* input;
    AudioPlayer* audio;
    FrontEnd* frontend;
    FILE* log;
    long* frames;
    long frame_cap;
    int* quit;
    const char** stop;
    uint32_t task_draw_masks[GAME_TASK_COUNT];
    Flic* task_flics[GAME_TASK_COUNT];
    int score_text_requested;
    int stats_result_requested;
    int zone_action_consumed;
} RewardFramePumpContext;

static uint32_t reward_frame_time_get(void* context)
{
    (void)context;
    return platform_time_milliseconds();
}

static int reward_frame_loop_abort(void* opaque)
{
    RewardFramePumpContext* context = (RewardFramePumpContext*)opaque;
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
        sound_request_queue_submit(0x29, 1);
        context->zone_action_consumed = 1;
        return 1;
    }
    if (hit_advance) {
        checkpoint_step(state->game);
        state->transition_stage = 3;
        context->zone_action_consumed = 1;
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
            frontend_help_overlay(pixels, pitch, WINDOW_W, WINDOW_H,
                                  objective_stream(context->game->level),
                                  &context->game->options);
        } else {
            surface_fill(pixels, pitch, WINDOW_W, WINDOW_H, 8, 16, 48);
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
        render_apply_brightness(pixels, pitch, WINDOW_W, WINDOW_H,
                                context->game->options.brightness);
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

static void frame_ui_action_complete(void* context, int action, int cell_x,
                                    int cell_y)
{
    FramePointerDispatchContext* frame =
        (FramePointerDispatchContext*)context;
    UiActionDispatchState* state;
    int result;
    if (frame == NULL || frame->ui_action_state == NULL) return;
    state = frame->ui_action_state;
    state->current_mode = (int)objective_selection_word_peek();
    state->selected_cell_x = cell_x;
    state->selected_cell_y = cell_y;
    state->cursor_action_x = (int)cursor_tile_pair_peek(0);
    state->cursor_action_y = (int)cursor_tile_pair_peek(1);
    state->action_flags = global_state_low_word_get();
    state->cursor_flags = terrain_choice_flags_peek();
    tutorial_nerp_pointer_release_record(action, cell_x, cell_y);
    ui_action_dispatch_state_install(state);
    result = ui_action_dispatch(action);
    global_state_low_word_set(state->action_flags);
    terrain_choice_flags_set(state->cursor_flags);
    ui_action_dispatch_state_install(NULL);
    media_action_feedback_sound_dispatch(result == 0 ? 1 : 0);
}

static void frame_tutorial_script_dispatch(float delta, void* context)
{
    (void)context;
    tutorial_frame_timer_array_advance(delta);
    tutorial_script_command_dispatch(delta);
}

static int frame_pointer_overlay_hit(void* context, float x, float y,
                                     float offset_x, float offset_y,
                                     uint32_t* action_out,
                                     uint32_t* cell_x_out,
                                     uint32_t* cell_y_out)
{
    FramePointerDispatchContext* state =
        (FramePointerDispatchContext*)context;
    GameContext* game;
    LevelState* level;
    EntitySlot* entity;
    float cell_x;
    float cell_y;
    int mouse_x;
    int mouse_y;
    int on_terrain;
    (void)offset_x;
    (void)offset_y;
    if (state == NULL || state->game == NULL || state->level == NULL ||
        action_out == NULL || cell_x_out == NULL || cell_y_out == NULL)
        return 0;
    game = state->game;
    level = state->level;
    mouse_x = (int)x;
    mouse_y = (int)y;
    cell_x = 0.0f;
    cell_y = 0.0f;
    entity = level_scene_pick_entity(game, level, state->screen_width,
                                     state->screen_height, mouse_x, mouse_y,
                                     30.0f);
    on_terrain = level_scene_mouse_cell(
        game, level, state->screen_width, state->screen_height,
        mouse_x, mouse_y, &cell_x, &cell_y);
    if (entity == NULL && !on_terrain) return 0;
    *action_out = 0x49u;
    if (entity != NULL) {
        *cell_x_out = (uint32_t)entity->x;
        *cell_y_out = (uint32_t)entity->y;
    }
    else {
        *cell_x_out = (uint32_t)cell_x;
        *cell_y_out = (uint32_t)cell_y;
    }
    return 1;
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

/* MainLoopFramePump (FUN_00477f30): drain messages, poll input, sample the
 * cursor, begin/present one renderer frame, optionally submit the projection
 * overlay, then clear the once-per-frame feature bit. The outer portable
 * frame loop owns its own event/tick/draw sequence and does not call this
 * helper recursively. */
// FUNCTION: LEGORR 0x00477f30
void main_loop_frame_pump(int draw_projection_overlay)
{
    MainLoopFramePumpHooks* const hooks = &s_main_loop_frame_pump_hooks;
    if (hooks->window_messages != NULL)
        hooks->window_messages(hooks->context);
    if (hooks->input_poll != NULL)
        hooks->input_poll(hooks->context);
    input_cursor_position_update();
    if (hooks->renderer_begin != NULL)
        hooks->renderer_begin(hooks->context);
    else
        (void)renderer_begin_frame_once();
    if (hooks->renderer_present != NULL)
        hooks->renderer_present(hooks->context);
    if (draw_projection_overlay != 0 && hooks->projection_overlay != NULL)
        hooks->projection_overlay(hooks->context, 0, 0);
    renderer_begin_frame_once_reset();
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
    s_window_runtime_state.window_style = style;
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

// FUNCTION: LEGORR 0x0047c170
void window_runtime_state_initialize(void* window)
{
    memset(&s_window_runtime_state, 0, sizeof(s_window_runtime_state));
    renderer_word_c_set(0u);
    s_window_runtime_state.window = window;
}

// FUNCTION: LEGORR 0x00477f80
void window_runtime_display_caps_query(void)
{
    if (s_window_runtime_ops.device_caps_query != NULL) {
        (void)s_window_runtime_ops.device_caps_query(
            s_window_runtime_ops.context, s_window_runtime_state.window, 12);
    }
}

// FUNCTION: LEGORR 0x00477fd0
int window_runtime_callback_triplet_set(const size_t* triplet)
{
    if (triplet == NULL) return 0;
    s_window_runtime_state.callback_triplet[0] = triplet[0];
    s_window_runtime_state.callback_triplet[1] = triplet[1];
    s_window_runtime_state.callback_triplet[2] = triplet[2];
    s_window_runtime_state.callback_triplet_valid = 1;
    return 1;
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
    uint32_t message_id;
    if (message == NULL) return 1;
    memcpy(&message_id, (const char*)message + 4, sizeof(message_id));
    if ((s_window_runtime_state.flags & 2u) != 0u &&
        (message_id == 0x1cu || message_id == 0x104u ||
         message_id == 0x105u)) {
        return 0;
    }
    return 1;
}

// FUNCTION: LEGORR 0x00478310
void window_runtime_adjust_rect(WindowPortRect* rect)
{
    if (rect == NULL) return;
    /* Retail adjusts a windowed rectangle using the saved style and a false
     * menu flag. Fullscreen leaves all four coordinates untouched. */
    if ((s_window_runtime_state.flags & 2u) == 0u &&
        s_window_runtime_ops.adjust_window_rect != NULL) {
        (void)s_window_runtime_ops.adjust_window_rect(
            s_window_runtime_ops.context, rect,
            s_window_runtime_state.window_style, 0);
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

// FUNCTION: LEGORR 0x004784c0
ptrdiff_t window_runtime_game_window_proc(void* window, uint32_t message,
                                          size_t wparam, ptrdiff_t lparam)
{
    WindowRuntimeState* state = &s_window_runtime_state;
    if (message == 2u || message == 0x10u) {
        state->close_requested = 1;
    }
    if (message == 0x1cu) {
        state->app_active = (uint32_t)wparam;
        return 0;
    }
    if (message == 0x46u) {
        if (lparam != 0) {
            uint32_t* flags = (uint32_t*)((char*)lparam + 24);
            *flags &= ~0x200u;
        }
        return 0;
    }
    if (message == 3u || message == 5u || message == 6u ||
        message == 0x86u || message == 0x100u || message == 0x101u ||
        message == 0x104u || message == 0x200u || message == 0x211u ||
        message == 0x212u) {
        return 0;
    }
    if (message == 0x201u || message == 0x202u || message == 0x204u ||
        message == 0x205u) {
        int down = message == 0x201u || message == 0x204u;
        int right = message == 0x204u || message == 0x205u;
        if ((state->flags & 0x4000u) == 0u) {
            if (down) {
                if (state->mouse_word_3c == 0u) {
                    state->mouse_word_4c = 1u;
                    state->mouse_word_50 = 1u;
                }
                state->mouse_word_40 = 1u;
                state->mouse_word_3c = 1u;
                state->mouse_word_54 = 1u;
                state->left_button_down = 1u;
                state->right_button_down = 1u;
                state->mouse_activity = 1u;
                return 0;
            }
            state->mouse_word_4c = 0u;
            state->mouse_word_50 = 0u;
            state->mouse_word_40 = 0u;
            state->mouse_word_3c = 0u;
            state->mouse_word_54 = 1u;
            state->left_button_down = 0;
            state->right_button_down = 0;
            state->mouse_activity = 1u;
            return 0;
        }
        else {
            if (right) {
                state->mouse_word_50 = down != 0 ? 1u : 0u;
                state->mouse_word_40 = down != 0 ? 1u : 0u;
                state->right_button_down = down != 0 ? 1u : 0u;
            }
            else {
                state->mouse_word_4c = down != 0 ? 1u : 0u;
                state->mouse_word_3c = down != 0 ? 1u : 0u;
                state->left_button_down = down != 0 ? 1u : 0u;
            }
            return 0;
        }
    }
    if (message == 0x203u) {
        state->left_double_click = 1;
        state->mouse_activity = 1u;
        state->mouse_word_54 = 1u;
        return 0;
    }
    if (message == 0x206u) {
        state->right_double_click = 1;
        state->mouse_activity = 1u;
        state->mouse_word_58 = 1u;
        return 0;
    }
    return window_runtime_default(window, message, wparam, lparam);
}

// FUNCTION: LEGORR 0x004786c0
ptrdiff_t window_runtime_activation_close_proc(void* window, uint32_t message,
                                               size_t wparam,
                                               ptrdiff_t lparam)
{
    WindowRuntimeState* state = &s_window_runtime_state;
    int button;
    if (message == 6u) return 0;
    if (message == 0x1cu) {
        state->app_active = (uint32_t)wparam;
        return 0;
    }
    if (message == 0x10u || message == 2u) {
        state->close_requested = 1;
        return 0;
    }
    if (message == 0x203u) {
        state->mouse_activity = 1;
        state->left_double_click = 1;
        state->mouse_word_54 = 1u;
        return 0;
    }
    if (message == 0x206u) {
        state->mouse_activity = 1;
        state->right_double_click = 1;
        state->mouse_word_58 = 1u;
        return 0;
    }
    if (message >= 0x201u && message <= 0x205u) {
        button = (int)(message - 0x201u);
        if (button == 0 || button == 1 || button == 3 || button == 4) {
            int down = button == 0 || button == 3;
            int right = button == 3 || button == 4;
            if ((state->flags & 0x4000u) == 0u) {
                if (down) {
                    window_runtime_capture(window);
                    if (state->mouse_word_3c == 0u) {
                        state->mouse_word_4c = 1u;
                        state->mouse_word_50 = 1u;
                    }
                    state->mouse_word_40 = 1u;
                    state->mouse_word_3c = 1u;
                    state->mouse_word_54 = 1u;
                    state->left_button_down = 1;
                    state->right_button_down = 1;
                    state->mouse_activity = 1;
                    return 0;
                }
                window_runtime_release_capture();
                state->mouse_word_4c = 0u;
                state->mouse_word_50 = 0u;
                state->mouse_word_40 = 0u;
                state->mouse_word_3c = 0u;
                state->mouse_word_54 = 1u;
                state->left_button_down = 0;
                state->right_button_down = 0;
                state->mouse_activity = 1u;
                return 0;
            }
            else {
                if (button == 0) {
                    window_runtime_capture(window);
                    state->mouse_word_4c = 1u;
                    state->mouse_word_3c = 1u;
                    state->mouse_word_54 = 1u;
                    state->left_button_down = 1;
                    state->mouse_activity = 1;
                    return 0;
                }
                if (button == 1) {
                    window_runtime_release_capture();
                    state->mouse_word_54 = 1u;
                    state->mouse_word_4c = 0u;
                    state->mouse_word_3c = 0u;
                    state->mouse_activity = 1;
                    state->left_button_down = 0;
                    return 0;
                }
                if (right) {
                    state->mouse_word_50 = down != 0 ? 1u : 0u;
                    state->mouse_word_40 = down != 0 ? 1u : 0u;
                    state->right_button_down = down != 0 ? 1u : 0u;
                }
                state->mouse_word_54 = 1u;
                state->mouse_activity = 1;
                return 0;
            }
        }
    }
    return window_runtime_default(window, message, wparam, lparam);
}

/* WindowTitleSet (FUN_004783d0): forwards the title to the active platform
 * window; the native and WinAPI adapters both update their owned window. */
// FUNCTION: LEGORR 0x004783d0
void window_title_set(const char* title)
{
    platform_set_window_title(title);
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

static int s_icontains(const char* haystack, const char* needle)
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
    checkpoint_blob_free(game);
    mission_reward_resources_release(game);
    free(game->records);
    tutorial_runtime_context_set(NULL, NULL, NULL);
    engine_tutorial_dispatch_hook_set(NULL, NULL);
    (void)tutorial_display_resources_release();
    level_destroy(game->level);
    block_close(game->cfg);
}

/* Streamed soundtrack aliases live beside the menu SFX aliases in Samples.
 * Their leading '@' marks a streamed WAVE in Lego.cfg, not part of its path. */
static const char* menu_music_resource(BlockFile* cfg)
{
    const char* music = block_get_strptr(cfg,
                                         "Lego* Samples SFX_AmbientMusicLoop");
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
static int64_t now_ms(void)
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

/* CreditsTextFile + CreditsBackAVI: scroll centered text over the configured
 * AVI until the player presses a key or clicks. */
// FUNCTION: LEGORR 0x0040a0c0
static void frameloop_credits(PlatformWindow* win, DisplaySurface* surf,
                              BlockFile* cfg, const char* wad_dir,
                              AudioPlayer* ap, int audio_status, int* quit,
                              FILE* log, long* frame_number)
{
    uint32_t scroll_clock = (uint32_t)now_ms();
    const char* text_name = block_get_strptr(cfg,
                                             "Lego* Main CreditsTextFile");
    char** lines = NULL;
    int line_count = 0;
    if (text_name == NULL
        || !frontend_credits_lines_load(text_name, &lines, &line_count)) {
        fprintf(log, "credits: cannot load %s\n",
                text_name != NULL ? text_name : "Lego* Main CreditsTextFile");
        return;
    }

    TextureSurface* font = texture_get_or_load(
        NULL, "Interface\\Fonts\\RSFont.bmp");
    Movie* movie = NULL;
    MovieInfo mi;
    MovieOut out;
    memset(&mi, 0, sizeof(mi));
    memset(&out, 0, sizeof(out));
    int video_frame_ms = FRAME_MS;
    char movie_rel[256], movie_os[512];
    if (movie_key_path(cfg, "CreditsBackAVI", movie_rel,
                       sizeof(movie_rel))
        && movie_path_resolve(wad_dir, movie_rel, movie_os,
                              sizeof(movie_os))) {
        movie = movie_open_path(movie_os);
        if (movie == NULL || movie_info(movie, &mi) != 0
            || mi.width <= 0 || mi.height <= 0) {
            fprintf(log, "credits: cannot open backdrop %s\n", movie_os);
            movie_close(movie);
            movie = NULL;
        } else if (mi.fps_num > 0 && mi.fps_den > 0) {
            video_frame_ms = 1000 * mi.fps_den / mi.fps_num;
            if (video_frame_ms < 1) {
                video_frame_ms = 1;
            }
        }
    }
    InputState input;
    input_reset(&input);
    int scroll_px = 0;
    int64_t next_video_ms = now_ms();
    int have_video_frame = 0;
    int leave = 0;
    int mouse_was_down = 0;
    while (!*quit && !leave) {
        PlatformEvent evbuf[32];
        int nev = platform_pump_events(win, evbuf, 32);
        input_capture_events(&input, evbuf, nev);
        renderer_word_c_set(input.mouse_down != 0 ? 1u : 0u);
        input_cursor_position_update();
        input_cursor_position_sync(&input);
        for (int i = 0; i < nev; ++i) {
            if (evbuf[i].kind == PLATFORM_EVENT_QUIT) {
                *quit = 1;
                break;
            }
            if (evbuf[i].kind == PLATFORM_EVENT_KEYDOWN) {
                leave = 1;
            }
            if (evbuf[i].kind == PLATFORM_EVENT_MOUSE) {
                if (!evbuf[i].held && mouse_was_down) {
                    leave = 1;
                }
                mouse_was_down = evbuf[i].held;
            }
        }
        if (*quit || leave) {
            break;
        }

        if (movie != NULL && now_ms() >= next_video_ms) {
            int mr = movie_next(movie, &out);
            if (mr == 1) {
                movie_close(movie);
                movie = movie_open_path(movie_os);
                if (movie != NULL && movie_info(movie, &mi) == 0) {
                    mr = movie_next(movie, &out);
                } else {
                    movie_close(movie);
                    movie = NULL;
                    mr = -1;
                }
            }
            if (mr == 0) {
                have_video_frame = 1;
            } else if (mr < 0) {
                movie_close(movie);
                movie = NULL;
                have_video_frame = 0;
            }
            next_video_ms += video_frame_ms;
            if (next_video_ms < now_ms() - video_frame_ms) {
                next_video_ms = now_ms();
            }
        }

        frontend_credits_scroll_step((uint32_t)now_ms(), &scroll_clock,
                                     &scroll_px);

        void* pixels = NULL;
        int pitch = 0;
        if (surface_lock(surf, &pixels, &pitch) == 0) {
            if (have_video_frame) {
                movie_scale_rgba(out.rgba, mi.width, mi.height, out.rgba_pitch,
                                 (uint8_t*)pixels, WINDOW_W, WINDOW_H, pitch);
            } else {
                surface_fill(pixels, pitch, WINDOW_W, WINDOW_H, 0, 0, 0);
            }
            frontend_render_credits(font,
                                    (const char* const*)lines, line_count,
                                    (int)scroll_px, WINDOW_W, WINDOW_H,
                                    pixels, pitch);
            platform_screenshot(win, pixels, pitch, WINDOW_W, WINDOW_H,
                                (int)*frame_number);
            platform_present_pixels(win, pixels, pitch, WINDOW_W, WINDOW_H);
            surface_unlock(surf, pixels);
        }
        /* ScrollingTextDisplay calls SoundSubsystemFrameTick after its
         * text/video draw and before the next frame boundary. */
        sound_subsystem_frame_tick(1.0f);
        if (audio_status == 0) {
            audio_update(ap);
        }
        if (movie == NULL) {
            Sleep(10);
        }
        ++*frame_number;
        input_begin_frame(&input);
    }

    movie_close(movie);
    texture_release(font);
    frontend_credits_lines_free(lines, line_count);
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
    game->level = level_load(game->cfg, level_name);
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
    /* Seed the follow camera at the grid centre so mode-1 starts over the
     * loaded level instead of the origin. */
    if (game->level != NULL) {
        cam_set_pos(&views[0].cam_follow,
                    (float)engine_level_width(ctx) * 0.5f,
                    (float)engine_level_height(ctx) * 0.5f, 0.0f);
    }
}

static void menu_save_image_config_rect_blit(FrontEnd* fe,
                                             GameContext* game,
                                             int item_index, void* pixels,
                                             int pitch, int width, int height);

static void frameloop_blit_texture_scaled(const TextureSurface* texture,
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
            dst[(size_t)px * 4 + 0] = (uint8_t)(color >> 16);
            dst[(size_t)px * 4 + 1] = (uint8_t)(color >> 8);
            dst[(size_t)px * 4 + 2] = (uint8_t)color;
            dst[(size_t)px * 4 + 3] = (uint8_t)(color >> 24);
        }
    }
}

/* The first tutorial's Mission Brief is shipped as the Objective entry in
 * ObjectiveText.txt. Keep this read at the resource boundary so the briefing
 * follows the installed language data instead of duplicating its prose. */
static int frameloop_tutorial_brief_load(char* out, size_t out_size)
{
    if (out == NULL || out_size == 0) return 0;
    out[0] = '\0';
    Resource* r = resource_open("Languages\\ObjectiveText.txt", RESOURCE_AUTO);
    if (r == NULL || resource_read(r) < 0 || r->data == NULL) {
        resource_free(r);
        return 0;
    }
    const char* data = (const char*)r->data;
    const size_t size = (size_t)r->size;
    const char* section = NULL;
    const char* end = data + size;
    for (const char* p = data; p < end;) {
        const char* line_end = (const char*)memchr(p, '\n', (size_t)(end - p));
        if (line_end == NULL) line_end = end;
        const char* line = p;
        while (line < line_end && (*line == ' ' || *line == '\t' || *line == '\r')) ++line;
        if (line < line_end && *line == '[') {
            section = ((size_t)(line_end - line) >= 13
                       && memcmp(line, "[MoveTuto_01]", 13) == 0)
                          ? line_end : NULL;
        } else if (section != NULL && line_end - line >= 10
                   && memcmp(line, "Objective:", 10) == 0) {
            line += 10;
            while (line < line_end && (*line == ' ' || *line == '\t')) ++line;
            size_t n = (size_t)(line_end - line);
            while (n > 0 && (line[n - 1] == '\r' || line[n - 1] == ' ')) --n;
            /* The \a marker separates the mission briefing from the later
             * objective update; the opening panel shows the first paragraph. */
            for (size_t i = 0; i + 1 < n; ++i) {
                if (line[i] == '\\' && line[i + 1] == 'a') { n = i; break; }
            }
            if (n >= out_size) n = out_size - 1;
            memcpy(out, line, n);
            out[n] = '\0';
            resource_free(r);
            return n > 0;
        }
        p = line_end < end ? line_end + 1 : end;
    }
    resource_free(r);
    return 0;
}

static void frameloop_wrap_brief(const char* src, char* out, size_t out_size,
                                int max_chars)
{
    size_t at = 0;
    int col = 0;
    for (const char* p = src; p != NULL && *p != '\0' && at + 1 < out_size;) {
        p += strspn(p, " \t");
        if (*p == '\0') break;
        const char* word = p;
        int len = (int)strcspn(p, " \t");
        if (col > 0 && col + 1 + len > max_chars) {
            out[at++] = '\n';
            col = 0;
        } else if (col > 0 && at + 1 < out_size) {
            out[at++] = ' ';
            ++col;
        }
        int copy_len = len;
        const size_t remaining = out_size - at - 1;
        if ((size_t)copy_len > remaining) {
            copy_len = (int)remaining;
        }
        strncpy(out + at, word, (size_t)copy_len);
        at += (size_t)copy_len;
        col += copy_len;
        p += len;
    }
    out[at] = '\0';
}

static void frameloop_briefing_render(PlatformWindow* win,
                                     DisplaySurface* surf,
                                     const TextureSurface* panel,
                                     const TextureSurface* title_font,
                                     const TextureSurface* body_font,
                                     const char* brief,
                                     LevelState* level, GameContext* game,
                                     int frame)
{
    void* pixels = NULL;
    int pitch = 0;
    if (surface_lock(surf, &pixels, &pitch) != 0) return;
    level_render_scene(level, game, WINDOW_W, WINDOW_H, pixels, pitch);
    if (panel != NULL) {
        frameloop_blit_texture_scaled(panel, pixels, pitch, WINDOW_W, WINDOW_H,
                                      76, 100, 488, 281);
    }
    if (title_font != NULL) {
        const char* title = "Mission Brief";
        int tw = (int)formatted_text_printf_width(title_font, title);
        formatted_text_draw(title_font, pixels, pitch, WINDOW_W, WINDOW_H,
                            (WINDOW_W - tw) / 2, 130, title);
    }
    if (body_font != NULL) {
        char wrapped[1200];
        if (brief != NULL && brief[0] != '\0') {
            frameloop_wrap_brief(brief, wrapped, sizeof(wrapped), 43);
            formatted_text_draw(body_font, pixels, pitch, WINDOW_W, WINDOW_H,
                                119, 160, wrapped);
        }
        uint8_t* frame = (uint8_t*)pixels;
        for (int y = 438; y < WINDOW_H; ++y) {
            uint8_t* row = frame + (size_t)y * pitch;
            for (int x = 0; x < WINDOW_W; ++x) {
                row[(size_t)x * 4 + 0] = 5;
                row[(size_t)x * 4 + 1] = 47;
                row[(size_t)x * 4 + 2] = 9;
                row[(size_t)x * 4 + 3] = 255;
            }
        }
        const TextureSurface* prompt_font = title_font != NULL
            ? title_font : body_font;
        formatted_text_draw(prompt_font, pixels, pitch, WINDOW_W, WINDOW_H,
                            122, 452, "Press SPACE to continue.");
    }
    render_apply_brightness(pixels, pitch, WINDOW_W, WINDOW_H,
                            game->options.brightness);
    platform_screenshot(win, pixels, pitch, WINDOW_W, WINDOW_H, frame);
    platform_present_pixels(win, pixels, pitch, WINDOW_W, WINDOW_H);
    surface_unlock(surf, pixels);
}

/* MenuOverlaySurfaceAndTextSetup (FUN_004129d0): the corpus resets the
 * save/load panel surface and its text resources when entering these menu
 * records. The frame-driven adapter composes the typed panel layout and the
 * current menu text on the software surface. */
// FUNCTION: LEGORR 0x004129d0
static void menu_overlay_surface_and_text_setup(
    FrontEnd* fe, const TextureSurface* hi_font, const TextureSurface* lo_font,
    void* pixels, int pitch, int width, int height)
{
    if (fe == NULL || pixels == NULL) {
        return;
    }
    const char* image = frontend_active_image(fe);
    const FeSubmenu* active = frontend_active_submenu(fe);
    if (active != NULL
        && (fe->menu == FE_MENU_SAVE
            || (fe->menu == FE_MENU_MAIN && image != NULL
                && s_icontains(image, "SaveLoad")))) {
        const FeSaveMenuLayout* layout = fe->save_action
                                             ? &fe->save_text_layout
                                             : &fe->level_text_layout;
        if (layout->has_panel && layout->panel_path[0] != '\0') {
            TextureSurface* panel = texture_get_or_load(NULL,
                                                        layout->panel_path);
            if (panel != NULL) {
                frameloop_blit_texture_scaled(panel, pixels, pitch, width,
                                              height, layout->panel_x,
                                              layout->panel_y, layout->panel_w,
                                              layout->panel_h);
                texture_release(panel);
            }
        }
    }
    frontend_render(fe, hi_font, lo_font, width, height, pixels, pitch);
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
    const char* img = frontend_active_image(fe);
    if (img != NULL) {
        TextureSurface* bg = texture_get_or_load(NULL, img);
        if (bg != NULL) {
            surface_blit_texture(pixels, pitch, WINDOW_W, WINDOW_H, bg, 0, 0);
        }
        texture_release(bg);
    }
    menu_overlay_surface_and_text_setup(fe, hi_font,
                                        lo_font, pixels, pitch, WINDOW_W,
                                        WINDOW_H);
    const FeSubmenu* active = frontend_active_submenu(fe);
    if (active != NULL) {
        for (int i = 0; i < active->item_count; ++i) {
            menu_save_image_config_rect_blit(fe, game, i, pixels, pitch,
                                             WINDOW_W, WINDOW_H);
        }
    }
    /* Brightness (issue #85) is a post-pass over the finished frame. */
    render_apply_brightness(pixels, pitch, WINDOW_W, WINDOW_H,
                            game->options.brightness);
    platform_screenshot(win, pixels, pitch, WINDOW_W, WINDOW_H, frame);
    platform_present_pixels(win, pixels, pitch, WINDOW_W, WINDOW_H);
    surface_unlock(surf, pixels);
}

/* MenuSaveImageConfigRectBlit (FUN_004124f0): load the selected slot's
 * framebuffer thumbnail and draw it in the configured SaveImage rectangle.
 * The port's TextureSurface replaces the corpus's cached DirectDraw handle. */
// FUNCTION: LEGORR 0x004124f0
static void menu_save_image_config_rect_blit(FrontEnd* fe,
                                             GameContext* game,
                                             int item_index, void* pixels,
                                             int pitch, int width, int height)
{
    if (fe == NULL || game == NULL || pixels == NULL || fe->cfg == NULL
        || item_index < 0) {
        return;
    }
    const FeSubmenu* sub = frontend_active_submenu(fe);
    const char* image = frontend_active_image(fe);
    if (sub == NULL || image == NULL || !s_icontains(image, "SaveLoad")
        || item_index >= sub->item_count) {
        return;
    }
    const FeItem* item = &sub->items[item_index];
    if (item->load_slot <= 0) {
        return;
    }

    char key[160];
    snprintf(key, sizeof(key), "%s Menu::SaveImage pos%d",
             util_root_name(fe->cfg), item->load_slot);
    const char* position = block_get_strptr(fe->cfg, key);
    if (position == NULL) {
        return;
    }
    const char* separator = strchr(position, ':');
    if (separator == NULL || separator == position || separator[1] == '\0') {
        return;
    }
    int x = atoi(position);
    int y = atoi(separator + 1);
    snprintf(key, sizeof(key), "%s Menu::SaveImage Width",
             util_root_name(fe->cfg));
    const char* width_value = block_get_strptr(fe->cfg, key);
    int draw_w = atoi(width_value != NULL ? width_value : "");
    snprintf(key, sizeof(key), "%s Menu::SaveImage Height",
             util_root_name(fe->cfg));
    const char* height_value = block_get_strptr(fe->cfg, key);
    int draw_h = atoi(height_value != NULL ? height_value : "");
    if (draw_w <= 0 || draw_h <= 0 || draw_w > width * 2
        || draw_h > height * 2) {
        return;
    }

    char thumb_path[320];
    save_slot_path(game, thumb_path, sizeof(thumb_path), item->load_slot,
                   "dat");
    const int cache_index = item->load_slot - 1;
    if (cache_index < 0 || cache_index >= FE_SAVE_THUMBNAIL_COUNT) {
        return;
    }
    if (fe->save_thumbnails[cache_index] == NULL) {
        fe->save_thumbnails[cache_index] =
            texture_get_or_load(NULL, thumb_path);
    }
    TextureSurface* thumb = fe->save_thumbnails[cache_index];
    if (thumb == NULL || thumb->pixels == NULL || thumb->width <= 0
        || thumb->height <= 0) {
        return;
    }
    frameloop_blit_texture_scaled(thumb, pixels, pitch, width, height,
                                  (int)x, (int)y, draw_w, draw_h);
}

int LegorrrRun(void)
{
    const char* wad_dir_environment = getenv("LEGORRR_WAD_DIR");
    const int explicit_wad_dir = wad_dir_environment != NULL &&
                                 wad_dir_environment[0] != '\0';
    const char* wad_dir = explicit_wad_dir ? wad_dir_environment : ".";
    const char* autostop = env_get("LEGORRR_AUTOSTOP_FRAMES", "0");
    long frame_cap = strtol(autostop, NULL, 10);
    if (frame_cap < 0) {
        frame_cap = 0;
    }
    /* CI/headless hook: open the pause overlay at this in-game frame (as if
     * Esc was pressed) so the PausedMenu can be screenshotted. */
    long autopause = strtol(env_get("LEGORRR_AUTOPAUSE_FRAMES", "0"), NULL, 10);

    FILE* log = fopen("legoRR-stub.log", "a");
    if (log == NULL) {
        log = stderr;
    }

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
    GameContext* game_storage = (GameContext*)calloc(1, sizeof(GameContext));
    FrontEnd* fe_storage = (FrontEnd*)calloc(1, sizeof(FrontEnd));
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
    cmd_init();
    cmd_context_set(&game);
    game.cfg = block_open("Lego.cfg");
    if (game.cfg != NULL) {
        engine_render_poly_ranges_set(
            block_get_float(game.cfg, "Lego* Main MedPolyRange", 0.0f),
            block_get_float(game.cfg, "Lego* Main HighPolyRange", 0.0f));
        /* GameConfigInit loads HUD config before the later bubble and font
         * resource passes. */
        bubble_config_global_flags_set(engine_feature_word_peek());
        (void)lego::upgrade_types_load(game.cfg);
        entity_names_config_load(game.cfg);
        if (block_get_bool(game.cfg, "HelpWindow::HelpWindowOn") == 1) {
            help_window_resources_init(game.cfg, NULL);
        }
        frontend_hud_runtime_install();
        frontend_render_three_pass_update(game.cfg, game.cfg);
        (void)light_effect_waypoint_config_load(game.cfg, "Lego*");
        light_effect_configuration_load(game.cfg, "Lego*");
        bubble_config_texture_load(game.cfg);
        preload_numbered_bitmaps("Interface\\FONTS\\HealthFont", "a000_");
        tooltip_icons_config_load(game.cfg);
        lego::level_upgrade_name_strings_load(game.cfg);
    }
    int save_loaded = save_read_slot(&game, SAVE_DEFAULT_SLOT, 0);
    fprintf(log, "frameloop: save slot %d %s\n", SAVE_DEFAULT_SLOT,
            save_loaded ? "loaded" : "bootstrap");

    EngineState state = { 0 };
    InputState  input = { 0 };
    input_reset(&input);
    input_direct_state_bind(&input);
    /* The three view contexts (DAT_005560e4/e8/ec): free/cruise cameras with
     * a glide step; the demo has no world yet, so they tick quietly. */
    ViewContext views[3] = { 0 };
    for (int i = 0; i < 3; ++i) {
        views[i].mode = 3;
        views[i].step = 8.0f;
    }
    EngineContext ctx = {
        ENGINE_APP_NORMAL, 0, 0, 0, 1,
        &state, &input,
        { &views[0], &views[1], &views[2] },
        NULL, /* level_block — bound to game.level below */
        NULL, /* level_voxels — resolved at bind (64-bit safe) */
        NULL, /* level_terrain */
        0.0f, 1.0f, 0,
    };
    tutorial_runtime_context_set(&game, &ctx, &input);
    engine_tutorial_dispatch_hook_set(frame_tutorial_script_dispatch, NULL);
    /* Engine <-> level binding happens after the front end below — the level
     * loads only once the player picks one — so ctx.level_block/voxels/
     * terrain are bound there, not here. */
    /* The finding-22 InfoMessages catalog + bus enabled. */
    message_init_defaults();
    /* Load config entries only after seeding the message-name catalog used by
     * message_load_infomessages to resolve each configured Info_* record. */
    if (game.cfg != NULL) {
        message_load_infomessages(game.cfg);
    }

    /* Selection/command wiring (finding 06): the executor's held-key probe
     * reads this frame's input tables; a keybind demo for a global op. */
    game.input_keyheld = demo_keyheld;
    cmd_bind_key('z', 0x40, 0, 0, 0, 0);

    PlatformWindow* win = platform_create_window("lrr-portable (frame loop)",
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
        fprintf(log, "frameloop: DirectDraw mode selection failed\n");
        platform_destroy_window(win);
        frameloop_game_context_dispose(&game);
        free(game_storage);
        free(fe_storage);
        if (log != stderr) fclose(log);
        return 3;
    }
    fprintf(log, "frameloop: DirectDraw mode selection accepted\n");
    fflush(log);
#endif

    DisplaySurface surf = { 0, 0, 0, 0, 0 };
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
    catalog_slot_flag_hooks_set(frame_catalog_slot_eligible,
                                frame_catalog_slot_available,
                                frame_catalog_slot_rank, &game);

    /* Real corpus textures through the merged texture loader. */
    TextureSurface* rock = texture_get_or_load(NULL,
                                               "World\\WorldTextures\\ROCKROOF.BMP");
    TextureSurface* ui = texture_get_or_load(NULL,
                                             "Interface\\Buttons\\info_path.bmp");
    if (rock == NULL || ui == NULL) {
        fprintf(log, "frameloop: texture load failed (rock=%p ui=%p)\n",
                (void*)rock, (void*)ui);
    }

    /* Audio: open the DirectSound-analog surface + play a real corpus wav. */
    AudioPlayer ap = { 0 };
    audio_system_player_set(&ap);
#if defined(LEGORRR_BACKEND_WINAPI)
    /* CDAudioDeviceStateUpdate is the retail startup bridge: it records the
     * DirectSound availability gate before invoking DirectSoundSystemInit.
     * That gate enables the later MCI CD-audio calls on WinAPI. */
    (void)media_cd_audio_device_state_update(0);
#else
    audio_system_init(platform_window_hwnd(win));
#endif
    if (game.cfg != NULL) {
        /* GameConfigInit loads the Samples aliases after enabling the sound
         * bank. This portable startup has the Lego.cfg tree ready earlier,
         * so load the same section once the audio backend opens. */
        sound_samples_config_load(game.cfg, 1);
        advisor_tooltip_and_sound_config_load(game.cfg);
    }
    int audio_status = sound_bank_sound_on() ? 0 : -1;
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

    long frames = 0;
    int thumb_slot = -1;   /* pending `.dat` thumbnail dump (see below) */
    int quit = 0;
    const char* stop = "closed";
    RewardFramePumpContext reward_frame;
    RewardScreenOps reward_ops;
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
    int play_movies = frame_cap == 0
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
    char level_name[FE_LEVEL_NAME_MAX];
    int boot_menu = 0, frontend_built = 0;
    if (game.cfg != NULL) {
        fprintf(log, "frameloop: config Lego.cfg loaded\n");
        int built = frontend_build(&fe, game.cfg);
        /* GameConfigInit loads AdvisorPositions after panel-name setup, and
         * only when DisplayAdvisor is enabled. frontend_build initializes
         * the panel table used by the loader's optional action token. */
        char advisor_config_path[128];
        snprintf(advisor_config_path, sizeof(advisor_config_path),
                 "%s DisplayAdvisor", util_root_name(game.cfg));
        if (block_get_bool(game.cfg, advisor_config_path) == 1) {
            advisor_position_config_load(game.cfg, WINDOW_W, WINDOW_H);
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
                 && env_get("LEGORRR_LEVEL", "")[0] == '\0';
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
    snprintf(level_name, sizeof(level_name), "%s",
             env_get("LEGORRR_LEVEL", "Level01"));

    /* Runtime save/load slots (SaveMenuBuild @ 0x415a10 rows): state from
     * the on-disk slots, grafted onto the SaveLoad screens by the front
     * end. Slot slots run 1..SAVE_SLOTS-1 (5 is the default/autosave). */
    int pending_load_slot = -1, pending_load_record = -1;
    {
        FeSlot slots[FE_SLOT_COUNT];
        int sn = 0;
        for (int s = 1; s < SAVE_SLOTS; s++) {
            int filled = 0, completed = 0, resume = -1;
            save_slot_state(&game, s, &filled, &completed, &resume);
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
            if (music_rc == 0) {
                /* Unlike one-shot FLIC sounds, the continuous track only
                 * reaches the platform queue from audio_update(). */
                audio_update(&ap);
            }
            fprintf(log, "frameloop: main-menu music %s -> %s (%d)\n",
                    music != NULL ? music : "(missing SFX_AmbientMusicLoop)",
                    music_rc == 0 ? "queued" : "unavailable", music_rc);
        }
        menu_enter(&fe, 0);
        fprintf(log, "frameloop: front end up — main menu (arrows select, "
                     "Enter/Space act, Esc quits)\n");
        MenuSoundTickClock menu_sound_clock;
        menu_sound_tick_clock_init(&menu_sound_clock);
        while (!quit) {
            PlatformEvent evbuf[32];
            int nev = platform_pump_events(win, evbuf, 32);
            input_capture_events(&input, evbuf, nev);
            input_cursor_position_update();
            input_cursor_position_sync(&input);
            for (int i = 0; i < nev; ++i) {
                if (evbuf[i].kind == PLATFORM_EVENT_QUIT) {
                    quit = 1;
                    stop = "closed";
                    break;
                }
            }
            if (quit || (frame_cap > 0 && frames >= frame_cap)) {
                if (!quit) {
                    stop = "autostop";
                }
                break;
            }
            /* Bitmap fonts (finding-23 grid): HiFont = selected items,
             * LoFont = unselected; NULL falls back to the 3x5 face. The
             * Hi font also measures pointer hit-testing. */
            TextureSurface* hi_font = NULL, *lo_font = NULL;
            const FeSubmenu* fs = frontend_active_submenu(&fe);
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
            fi.enter = input_key_edge(&input, ' ')
                       || input_key_edge(&input, PLATFORM_KEY_ENTER);
            fi.esc = input_key_edge(&input, PLATFORM_KEY_ESC);
            fi.mouse_x = input.mouse_x;
            fi.mouse_y = input.mouse_y;
            fi.click = input_mouse_pressed(&input);
            fi.mdown = input.mouse_down;
            FeEvent ev = frontend_tick(&fe, &fi, hi_font);
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
            if (ev == FE_EVT_QUIT) {
                quit = 1;
                stop = "menu quit";
                break;
            }
            if (ev == FE_EVT_START_LEVEL) {
                snprintf(level_name, sizeof(level_name), "%s", fe.pending_level);
                /* record<->level mapping (port of CheckpointStep's
                 * list-index assignment): checkpoints during this level
                 * land in the level's own record, so mid-campaign saves
                 * neither collide nor resume short. */
                game.current_record = fe_record_for_level(&fe, level_name);
                if (game.current_record < 0) {
                    game.current_record = 0;
                }
                fprintf(log, "frameloop: front end picked level %s\n", level_name);
                break;
            }
            if (ev == FE_EVT_LOAD_GAME) {
                int filled = 0, completed = 0, resume = -1;
                if (save_slot_state(&game, fe.load_slot, &filled, &completed,
                                    &resume)) {
                    /* record index -> level (the inverse mapping — the
                     * port's level-tree stand-in for SavTreeLoad): resume
                     * the furthest completed record's level, else start
                     * the campaign fresh. */
                    pending_load_slot = fe.load_slot;
                    pending_load_record = resume >= 0 ? resume : 0;
                    if (resume >= 0
                        && fe_level_for_record(&fe, resume, level_name,
                                               sizeof(level_name))) {
                        /* mapped */
                    } else if (fe.campaign_n > 0) {
                        snprintf(level_name, sizeof(level_name), "%s",
                                 fe.campaign[0]);
                    } else {
                        snprintf(level_name, sizeof(level_name), "%s",
                                 "Level01");
                    }
                    game.save_slot = fe.load_slot;
                    game.current_record = pending_load_record;
                    fprintf(log, "frameloop: loading slot %d record %d -> "
                                 "level %s (%d/%d completed)\n",
                            pending_load_slot, pending_load_record,
                            level_name, completed, filled);
                } else {
                    snprintf(level_name, sizeof(level_name), "%s",
                             env_get("LEGORRR_LEVEL", "Level01"));
                    fprintf(log, "frameloop: slot %d absent — starting fresh "
                                 "at %s\n", fe.load_slot, level_name);
                }
                break;
            }
            if (ev == FE_EVT_CREDITS) {
                frameloop_credits(win, &surf, game.cfg, wad_dir, &ap,
                                  audio_status, &quit, log, &frames);
            }

            /* MenuRunLoop drains/rotates deferred aliases after menu logic
             * and before its frame pump. This preserves the one-frame queue
             * boundary for FLIC and menu sound requests. */
            sound_subsystem_frame_tick(menu_sound_clock.dt);
            frameloop_menu_render(win, &surf, &fe, hi_font, lo_font, &game,
                                  (int)frames);
            if (audio_status == 0) {
                audio_update(&ap);
            }
            Sleep((DWORD)FRAME_MS);
            ++frames;
            menu_sound_tick_clock_advance(&menu_sound_clock);
            input_begin_frame(&input);
        }
    }

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

    char mission_brief[1024] = { 0 };
    int mission_brief_active = game.level != NULL
        && pending_load_slot < 0
        && env_get("LEGORRR_SKIP_BRIEFING", "")[0] == '\0'
        && (s_icontains(level_name, "MoveTuto_01")
            || s_icontains(level_name, "Tutorial01"));
    TextureSurface* briefing_panel = NULL;
    TextureSurface* briefing_title_font = NULL;
    TextureSurface* briefing_body_font = NULL;
    if (mission_brief_active) {
        (void)frameloop_tutorial_brief_load(mission_brief,
                                            sizeof(mission_brief));
        briefing_panel = texture_get_or_load(
            NULL, "Interface\\BriefingPanel\\BriefingPanel.bmp");
        briefing_title_font = texture_get_or_load(
            NULL, "Interface\\Fonts\\MbriefFont2.bmp");
        briefing_body_font = texture_get_or_load(
            NULL, "Interface\\Fonts\\MbriefFont.bmp");
        int voice_frames = audio_status == 0
            ? audio_play_named(&ap,
                "Sounds\\Streamed\\Training\\Moving1\\mov1br.wav")
            : -1;
        fprintf(log, "frameloop: Mission Brief ready (%s, voice %s, %d frames)\n",
                mission_brief[0] != '\0' ? "ObjectiveText loaded"
                                          : "ObjectiveText unavailable",
                voice_frames >= 0 ? "played" : "unavailable", voice_frames);
    }

    const float dt = 1.0f / 30.0f;
    LevelState* pointer_dispatch_level = NULL;
    int pointer_dispatch_previous_down = 0;
    LevelState* reward_tick_level = NULL;
    int reward_tick_started = 0;
    /* Game Speed (issue #85): options_scale_frame stamps the store's factor
     * into ctx.time_scale (EngineTick's subsystem tickers) and returns the
     * scaled dt for the game-layer tickers (ai/objective). */
    /* Interactive proof: the arrow keys pan the blitted texture. */
    int rock_x = 24, rock_y = 24;

    while (!quit) {
        /* Begin-frame adapter state is per iteration. The retail bit is
         * cleared by FUN_0047c8d0 after RendererBeginFrameOnce; no renderer
         * device is bound in this portable loop yet. */
        renderer_begin_frame_once_reset();
        /* 1. Device capture for this frame (the finding-05 "writer" half):
         *    platform events -> canonical codes -> input tables. */
        PlatformEvent evbuf[32];
        int nev = platform_pump_events(win, evbuf, 32);
        input_capture_events(&input, evbuf, nev);
        input_cursor_position_update();
        input_cursor_position_sync(&input);
        for (int i = 0; i < nev; ++i) {
            if (evbuf[i].kind == PLATFORM_EVENT_QUIT) {
                quit = 1;
                stop = "closed";
                break;
            }
        }
        if (quit) {
            break;
        }
        if (frame_cap > 0 && frames >= frame_cap) {
            stop = "autostop";
            break;
        }

        /* The mission introduction holds simulation and accepts the same
         * simple continue input as the reference. On dismissal the next
         * frame enters normal gameplay without consuming that input twice. */
        if (mission_brief_active) {
            const int dismissed = input_key_edge(&input, ' ')
                || input_key_edge(&input, PLATFORM_KEY_ENTER)
                || input_mouse_pressed(&input);
            if (dismissed) {
                mission_brief_active = 0;
                fprintf(log, "frameloop: Mission Brief dismissed; controls live\n");
            } else {
                frameloop_briefing_render(win, &surf, briefing_panel,
                                          briefing_title_font,
                                          briefing_body_font, mission_brief,
                                          game.level, &game, (int)frames);
                if (audio_status == 0) audio_update(&ap);
                Sleep((DWORD)FRAME_MS);
                ++frames;
                input_begin_frame(&input);
                continue;
            }
            Sleep((DWORD)FRAME_MS);
            ++frames;
            input_begin_frame(&input);
            continue;
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
            FeEvent save_ev = frontend_tick(&fe, &fi, hi_font);
            if (save_ev == FE_EVT_SAVE_GAME) {
                save_slot_set(&game, fe.load_slot);
                checkpoint_blob_build(&game);
                save_write_record(&game, game.current_record,
                                  game.checkpoint_blob);
                save_mark_record(&game, game.current_record);
                show_level_overlay_for_current_selection(&game);
                int filled = 0, completed = 0, resume = -1;
                if (save_slot_state(&game, fe.load_slot, &filled, &completed,
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
                thumb_slot = fe.load_slot;
                fprintf(log, "frameloop: reward checkpoint saved to slot %d\n",
                        fe.load_slot);
            }
            sound_subsystem_frame_tick(save_sound_clock.dt);
            if (fe.open) {
                frameloop_menu_render(win, &surf, &fe, hi_font, lo_font,
                                      &game, (int)frames);
            }
            if (audio_status == 0) {
                audio_update(&ap);
            }
            Sleep((DWORD)FRAME_MS);
            ++frames;
            menu_sound_tick_clock_advance(&save_sound_clock);
            input_begin_frame(&input);
            continue;
        }

        /* Esc (or the LEGORRR_AUTOPAUSE_FRAMES CI hook) opens the in-game
         * pause overlay — the shipped PausedMenu — instead of quitting. */
        if (input_key_edge(&input, PLATFORM_KEY_ESC)
            || (autopause > 0 && frames == autopause)) {
            snprintf(fe.resume_level, sizeof(fe.resume_level), "%s",
                     level_name);
            menu_enter(&fe, 2);
            FeOverlayMenuGateState overlay_gate;
            memset(&overlay_gate, 0, sizeof(overlay_gate));
            overlay_gate.cached_menu = -1;
            overlay_gate.menus[0].entries[0].handle = 1;
            overlay_gate.menus[0].entries[2].handle = 1;
            fprintf(log, "frameloop: paused (Continue / Options / Restart / "
                         "Quit)\n");
            int want_reload = 0;
            MenuSoundTickClock pause_sound_clock;
            menu_sound_tick_clock_init(&pause_sound_clock);
            while (fe.open && !quit) {
                PlatformEvent pev[32];
                int pn = platform_pump_events(win, pev, 32);
                input_capture_events(&input, pev, pn);
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
                fi.enter = input_key_edge(&input, ' ')
                           || input_key_edge(&input, PLATFORM_KEY_ENTER);
                fi.esc = input_key_edge(&input, PLATFORM_KEY_ESC);
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
                gate_ctx.mode = 0;
                overlay_gate.level_start_flag = level_start_flag();
                overlay_gate.options_flag = menu_options_flag();
                overlay_gate.blocked =
                    global_option_low_bits_enabled(game.level);
                (void)overlay_menu_gate(&overlay_gate, 0,
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
                if (ev == FE_EVT_QUIT) {
                    quit = 1;
                    stop = "menu quit";
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
                Sleep((DWORD)FRAME_MS);
                ++frames;
                menu_sound_tick_clock_advance(&pause_sound_clock);
                input_begin_frame(&input);
            }
            if (want_reload && !quit) {
                /* Pause -> Restart_Mission: reload the level in place. */
                fprintf(log, "frameloop: restarting level %s\n", level_name);
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
            /* The pause consumed this whole frame (its own loop rendered).
             * Continue to the next in-game frame without running the game
             * sim or the level renderer over the menu. */
            if (!quit || game.level != NULL) {
                Sleep((DWORD)FRAME_MS);
                input_begin_frame(&input);
                continue;
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
            pointer_ops.overlay_fallback = frame_pointer_overlay_hit;
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
            /* MainSimInteractionAndCellUpdate calls 0x41baa0 only after its
             * 0x100 engine-state and 0x1000 debug-state gates. Its fourth
             * argument is the previous held state; the fifth is the release
             * edge used for action counting. Keep this adapter alive only
             * across the synchronous call because its fallback retains the
             * stack context. */
            if ((ctx.state & 0x100u) == 0u &&
                (ctx.debug & 0x1000u) == 0u) {
                simulation_pointer_dispatch_ops_set(&pointer_ops);
                (void)simulation_pointer_action_dispatch(
                    input.mouse_x, input.mouse_y, input.mouse_down,
                    pointer_dispatch_previous_down,
                    pointer_dispatch_previous_down != 0 &&
                        input.mouse_down == 0,
                    ctx.views[0]);
                simulation_pointer_dispatch_ops_set(NULL);
            }
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

            /* Left-click a Raider to select it; click open ground to send
             * the selected group there. Both operations use the rendered
             * camera projection, so the pointer agrees with what is shown. */
            if (input_mouse_pressed(&input)) {
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
                if (!mission_reward_active(&game)) {
                    mission_reward_set_active(&game, 1);
                    fprintf(log, "frameloop: reward screen shown (click its "
                                 "Save/Advance button)\n");
                }
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
                            fprintf(log, "frameloop: reward Advance clicked -> "
                                         "%s\n",
                                    nm != NULL ? nm : "campaign end");
                        }
                    }
                }
            }
        }

        frame_camera_target_registry_update(&ctx, &game);
        EngineTick(&ctx, dt);
        screen_effects_tick(dt);

        /* Retail MainLoop calls RendererBeginFrameOnce after the registered
         * per-frame tick callback returns and before its next frame-time
         * calculation. EngineTick is that callback in this portable loop. */
        (void)renderer_begin_frame_once();

        void* pixels = NULL;
        int   pitch = 0;
        if (surface_lock(&surf, &pixels, &pitch) == 0) {
            if (game.level != NULL) {
                level_render_scene(game.level, &game, WINDOW_W, WINDOW_H,
                                   pixels, pitch);
            } else {
                surface_fill(pixels, pitch, WINDOW_W, WINDOW_H, 8, 16, 48);
                surface_draw_sweep(pixels, pitch, WINDOW_W, WINDOW_H,
                                   (int)frames);
                if (rock != NULL) {
                    surface_blit_texture(pixels, pitch, WINDOW_W, WINDOW_H,
                                         rock, rock_x, rock_y);
                }
                if (ui != NULL) {
                    surface_blit_texture(pixels, pitch, WINDOW_W, WINDOW_H,
                                         ui, rock_x + 64 + 8, rock_y);
                }
            }
            /* Help Window (issue #85): the option toggles the shipped help
             * panel over the level; the advisor stream text leads when the
             * announcer has one. frontend_help_overlay holds the On/Off
             * gate — this call is unconditional. */
            if (game.level != NULL) {
                frontend_help_overlay(pixels, pitch, WINDOW_W, WINDOW_H,
                                      objective_stream(game.level),
                                      &game.options);
                if ((s_icontains(level_name, "MoveTuto_01")
                     || s_icontains(level_name, "Tutorial01"))
                    && briefing_body_font != NULL) {
                    uint8_t* frame = (uint8_t*)pixels;
                    for (int y = WINDOW_H - 22; y < WINDOW_H - 2; ++y) {
                        uint8_t* row = frame + (size_t)y * pitch;
                        for (int x = 0; x < WINDOW_W; ++x) {
                            row[(size_t)x * 4 + 0] = 8;
                            row[(size_t)x * 4 + 1] = 18;
                            row[(size_t)x * 4 + 2] = 10;
                            row[(size_t)x * 4 + 3] = 255;
                        }
                    }
                    formatted_text_draw(briefing_body_font, pixels, pitch,
                        WINDOW_W, WINDOW_H, 12, WINDOW_H - 19,
                        "Click Raider then cave to move. Keys 1-4 select, M move. Esc pauses.");
                }
            }
            /* Brightness (issue #85): BrightnessSet's scalar on the frame. */
            render_apply_brightness(pixels, pitch, WINDOW_W, WINDOW_H,
                                    game.options.brightness);
            platform_screenshot(win, pixels, pitch, WINDOW_W, WINDOW_H,
                                (int)frames);
            platform_present_pixels(win, pixels, pitch, WINDOW_W, WINDOW_H);
            if (thumb_slot >= 0) {
                /* Save-confirm thumbnail (FUN_0047e440): dump the frame we
                 * just presented to Saves\<slot>.dat as a 24bpp BMP. */
                save_write_thumbnail(&game, thumb_slot, pixels, pitch,
                                     WINDOW_W, WINDOW_H);
                frontend_save_thumbnail_invalidate(&fe, thumb_slot);
                fprintf(log, "frameloop: .dat thumbnail written (slot %d)\n",
                        thumb_slot);
                thumb_slot = -1;
            }
            surface_unlock(&surf, pixels);
        }
        Sleep((DWORD)FRAME_MS);
        ++frames;

        /* 3. Frame boundary: this frame's keys become prev; cur clears for
         *    the next capture (edge semantics, findings 04/05). */
        input_begin_frame(&input);
    }

    fprintf(log, "frameloop: %ld frames, gauged state=0x%08x, tickers=%d, "
            "exit=%s\n", frames, ctx.state, ctx.tickers_ran, stop);

    EngineShutdown(&ctx);
    reward_screen_runtime_bind(NULL, NULL);
    sound_subsystem_stop_all();
    audio_active_shutdown();
    audio_system_player_set(NULL);
    texture_cache_clear();
    surface_active_render_target_set(NULL);
    input_direct_state_bind(NULL);
    surface_destroy(&surf);
    platform_destroy_window(win);

    frontend_close(&fe);
    sav_tree_roots_clear();
    catalog_slot_flag_hooks_set(NULL, NULL, NULL, NULL);
    frameloop_game_context_dispose(&game);
    free(fe_storage);
    free(game_storage);

    if (log != stderr) {
        fclose(log);
    }
    return 0;
}

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
