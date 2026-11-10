// SPDX-License-Identifier: LGPL-3.0-or-later
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "entity.hpp"
#include "game.hpp"
#include "tutorial_hud_ports.hpp"
#include "util.hpp"
#include "ui_list.hpp"
#include "frontend.hpp"
#include "select.hpp"
#include "objective.hpp"
#include "level.hpp"
#include "spawn.hpp"
#include "cmd.hpp"
#include "../crt/crt.hpp"
#include "../gods98/block.hpp"
#include "../gods98/heap.hpp"
#include "../gods98/resource.hpp"
#include "../gods98/input.hpp"
#include "../gods98/engine.hpp"
#include "../gods98/texture.hpp"
#include "../gods98/view.hpp"
#include "../gods98/string.hpp"
#include "terrain_grid.hpp"
#include "../gods98/message.hpp"
#include "../gods98/media.hpp"

/* The old NERP/advisor owner lives in DAT windows in the corpus.  Keep the
 * parser and interpreter state explicit here so callers can install the
 * operations that belong to the game or platform layer. */
typedef struct TutorialTokenEntry {
    char* name;
    uint32_t value;
    int kind;
} TutorialTokenEntry;

#define NERP_ICON_STATE_COUNT 29
#define NERP_TUTORIAL_CELL_CAPACITY 2048

typedef struct NerpTutorialCellState {
    int32_t x;
    int32_t y;
    uint32_t block_clicks;
    uint32_t crystal_count;
    uint32_t ore_count;
    uint32_t flags;
} NerpTutorialCellState;

typedef struct NerpRuntimeState {
    /* Reference history[0] aliases the frontend's last inline sound index.
     * Only entries 1..8 live here; always use tutorial_message_history_get. */
    uint32_t message_history[8];
    uint32_t message_maximum;
    uint32_t message_offset;
    uint32_t message_ack_count;
    uint32_t message_pending_ack;
    uint32_t message_previous_ack;
    uint32_t message_flags;
    uint32_t message_wait;
    uint32_t message_suppress_arrow;
    uint32_t message_sound_active;
    float message_timer;
    uint32_t icon_clicked[NERP_ICON_STATE_COUNT];
    uint32_t icon_flashing[NERP_ICON_STATE_COUNT];
    float icon_position[2];
    uint32_t icon_space;
    uint32_t icon_width;
    uint32_t tutorial_flags;
    uint32_t tutorial_block_clicks;
    uint32_t tutorial_crystals;
    uint32_t tutorial_block_type_flags;
    uint32_t tutorial_record_pointer;
    uint32_t camera_control_flags;
    int32_t camera_target_cells[2][2];
    uint32_t camera_target_valid[2];
    uint32_t hidden_objects_found;
    uint32_t objective_switch;
    uint32_t pointer_release_count;
    int32_t pointer_action;
    int32_t pointer_x;
    int32_t pointer_y;
    NerpTutorialCellState cells[NERP_TUTORIAL_CELL_CAPACITY];
    uint32_t cell_count;
} NerpRuntimeState;

static TutorialScriptOps s_tutorial_script_ops;
static GameContext* s_tutorial_game_context;
static EngineContext* s_tutorial_engine_context;
static void* s_tutorial_script_data;
static uint32_t s_tutorial_script_size;
static TutorialScriptFreeFn s_tutorial_script_release;
static void* s_tutorial_script_release_context;
static uint32_t* s_tutorial_program_words;
static uint32_t s_tutorial_program_count;
static uint32_t s_tutorial_program_capacity;
static uint32_t s_tutorial_nerp_registers[8];
static float s_tutorial_nerp_timers[4];
static NerpRuntimeState s_nerp_runtime;
// GLOBAL: LEGORR 0x00555d40
static uint32_t s_tutorial_555d40_state[45];
// GLOBAL: LEGORR 0x004ff958
static uint32_t s_tutorial_4ff958_state[11];
static TutorialTokenEntry s_tutorial_tokens[512];
static uint32_t s_tutorial_token_count;
static uint32_t s_tutorial_literal_token_count;
static TutorialScriptOps s_advisor_token_ops;
static uint32_t s_tutorial_message_generation = 0xffffffffu;
/* Initialized .data words at 0x4a6748/4c/50, outside the script reset bands. */
static float s_tutorial_message_timer_values[3] = {1000.0f, 500.0f, 4000.0f};
static uint32_t s_tutorial_focus_id;
static uint32_t s_tutorial_focus_valid;
static float s_tutorial_focus_offset_target;
static float s_tutorial_focus_angle_target;
static uint32_t s_tutorial_focus_offset_active;
static uint32_t s_tutorial_focus_angle_active;
static float s_tutorial_focus_offset;
static float s_tutorial_focus_angle;
static float s_tutorial_focus_angle_velocity;

static int tutorial_builtin_command(uint32_t command, const uint32_t* args,
                                    uint32_t argc, uint32_t* result_out);
static uint32_t tutorial_builtin_command_adapter(uint32_t command,
                                                const uint32_t* args,
                                                uint32_t arg_count,
                                                void* context);

static void tutorial_scroll_runtime_state_reset(void)
{
    memset(s_nerp_runtime.message_history, 0,
           sizeof(s_nerp_runtime.message_history));
    s_nerp_runtime.message_maximum = 0u;
    s_nerp_runtime.message_offset = 0u;
    s_nerp_runtime.message_ack_count = 0u;
    s_nerp_runtime.message_pending_ack = 0u;
    s_nerp_runtime.message_previous_ack = 0u;
    s_nerp_runtime.message_flags = 0u;
    s_nerp_runtime.message_timer = 0.0f;
    s_nerp_runtime.message_sound_active = 0u;
    s_tutorial_message_generation = objective_advisor_token_generation_get();
    advisor_message_source_clear();
}

/* LevelLoad also loads tokens directly through objective.cpp. Observe that
 * table's generation before any retained text/history access, not only when
 * the optional tutorial token loader was used. */
void tutorial_message_generation_sync(void)
{
    if (s_tutorial_message_generation != objective_advisor_token_generation_get())
        tutorial_scroll_runtime_state_reset();
}

uint32_t tutorial_message_history_get(uint32_t index)
{
    tutorial_message_generation_sync();
    if (index == 0u) return advisor_inline_last_sound_get();
    return index <= 8u ? s_nerp_runtime.message_history[index - 1u] : 0u;
}

void tutorial_message_timer_values_get(float values[3])
{
    if (values != NULL)
        memcpy(values, s_tutorial_message_timer_values,
               sizeof(s_tutorial_message_timer_values));
}

void tutorial_message_duration_set(float duration)
{
    s_nerp_runtime.message_timer = duration;
}

void tutorial_message_sound_active_set(void)
{
    s_nerp_runtime.message_sound_active = 1u;
}

static void tutorial_camera_focus_state_reset(void)
{
    s_tutorial_focus_id = 0xffffffffu;
    s_tutorial_focus_valid = 0u;
    s_tutorial_focus_offset_target = 0.0f;
    s_tutorial_focus_angle_target = 0.0f;
    s_tutorial_focus_offset_active = 0u;
    s_tutorial_focus_angle_active = 0u;
    s_tutorial_focus_offset = 0.0f;
    s_tutorial_focus_angle = 0.0f;
    s_tutorial_focus_angle_velocity = 0.0f;
}

static void tutorial_camera_runtime_state_reset(void)
{
    tutorial_camera_focus_state_reset();
    if (s_tutorial_engine_context != NULL) {
        /* NERP owns requests, not the primary camera's constructor,
         * configured bounds or scene-frame pointers. */
        if (s_tutorial_engine_context->views[0] != NULL) {
            s_tutorial_engine_context->views[0]->target_pending = 0;
            s_tutorial_engine_context->views[0]->target_tween = 0;
        }
    }
}

/* NERP callback metadata is the third dword of each 12-byte callback-table
 * row at 0x004a5948. The retail table has 293 rows. Metadata determines
 * argument count and whether the result feeds a condition; keep the bytecode
 * cursor aligned even for callbacks whose effects have not been ported yet. */
static const uint8_t s_tutorial_command_kinds[293] = {
    3,0,0,0,0,0,0,0,3,3,3,3,0,0,0,0,
    5,0,0,0,0,0,0,0,0,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
    4,4,0,0,0,0,4,4,4,0,4,4,4,4,0,4,
    4,0,4,0,0,0,0,4,4,4,4,0,0,0,0,0,
    0,0,0,0,4,5,4,4,4,0,4,4,5,0,4,5,
    0,0,0,0,0,4,4,0,0,4,4,0,6,1,5,1,
    5,0,1,4,4,0,4,4,0,0,1,0,4,4,0,5,
    0,0,4,4,0,4,4,0,5,5,4,3,4,4,4,1,
    0,0,0,0,0,4,4,0,4,4,0,4,0,4,4,4,
    0,0,4,0,4,0,4,4,4,0,0,4,4,4,5,4,
    4,0,1,0,4,0,4,0,4,0,4,3,0,0,5,4,
    0,0,0,0,0,1,0,1,3,4,3,3,3,3,4,0,
    0,0,4,4,4,0,4,4,0,4,4,0,4,4,0,4,
    0,0,0,4,4,4,4,4,4,4,4,4,0,0,0,0,
    0,0,0,0,0,4,0,4,0,4,4,4,3,4,4,0,
    4,4,0,4,4,1,4,0,4,4,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0
};

static void tutorial_script_data_release(void* data)
{
    if (data == NULL) return;
    if (s_tutorial_script_release != NULL)
        s_tutorial_script_release(data, s_tutorial_script_release_context);
    else
        engine_heap_free(data);
    s_tutorial_script_release = NULL;
    s_tutorial_script_release_context = NULL;
}

static int tutorial_text_equal(const char* a, const char* b)
{
    if (a == NULL || b == NULL) return a == b;
    while (*a != '\0' && *b != '\0') {
        unsigned char ca = (unsigned char)*a++;
        unsigned char cb = (unsigned char)*b++;
        if (ca >= 'A' && ca <= 'Z') ca = (unsigned char)(ca + ('a' - 'A'));
        if (cb >= 'A' && cb <= 'Z') cb = (unsigned char)(cb + ('a' - 'A'));
        if (ca != cb) return 0;
    }
    return *a == *b;
}

static void tutorial_tokens_clear(void)
{
    uint32_t i;
    for (i = 0; i < s_tutorial_token_count; ++i) {
        if (s_tutorial_tokens[i].kind == 2 &&
            s_tutorial_script_ops.resolved_token_release != NULL)
            s_tutorial_script_ops.resolved_token_release(
                s_tutorial_tokens[i].kind, s_tutorial_tokens[i].value,
                s_tutorial_script_ops.context);
        free(s_tutorial_tokens[i].name);
        s_tutorial_tokens[i].name = NULL;
        s_tutorial_tokens[i].value = 0;
        s_tutorial_tokens[i].kind = 0;
    }
    s_tutorial_token_count = 0;
    s_tutorial_literal_token_count = 0;
}

static int tutorial_token_add(const char* start, uint32_t length, int kind)
{
    char* copy;
    uint32_t i;
    uint32_t value = 0;
    if (s_tutorial_token_count >= 512 || length == 0) return 0;
    copy = (char*)malloc((size_t)length + 1);
    if (copy == NULL) return 0;
    for (i = 0; i < length; ++i) {
        copy[i] = start[i] == '_' ? ' ' : start[i];
    }
    copy[length] = '\0';
    if (kind != 0 && s_tutorial_script_ops.resolve != NULL) {
        value = s_tutorial_script_ops.resolve(kind, copy,
                                              s_tutorial_script_ops.context);
    }
    s_tutorial_tokens[s_tutorial_token_count].name = copy;
    s_tutorial_tokens[s_tutorial_token_count].value = value;
    s_tutorial_tokens[s_tutorial_token_count].kind = kind;
    ++s_tutorial_token_count;
    return 1;
}

void tutorial_script_ops_set(const TutorialScriptOps* ops)
{
    if (ops != NULL) s_tutorial_script_ops = *ops;
    else memset(&s_tutorial_script_ops, 0, sizeof(s_tutorial_script_ops));
    if (s_tutorial_script_ops.command == NULL)
        s_tutorial_script_ops.command = tutorial_builtin_command_adapter;
}

void tutorial_runtime_context_set(GameContext* game, EngineContext* engine,
                                  const InputState* input)
{
    s_tutorial_game_context = game;
    s_tutorial_engine_context = engine;
    (void)input;
    if (s_tutorial_script_ops.command == NULL)
        s_tutorial_script_ops.command = tutorial_builtin_command_adapter;
}

void tutorial_nerp_pointer_release_record(int action, int cell_x, int cell_y)
{
    ++s_nerp_runtime.pointer_release_count;
    s_nerp_runtime.pointer_action = action;
    s_nerp_runtime.pointer_x = cell_x;
    s_nerp_runtime.pointer_y = cell_y;
}

static void tutorial_script_program_clear(void)
{
    if (s_tutorial_program_words != NULL) {
        free(s_tutorial_program_words);
        s_tutorial_program_words = NULL;
    }
    s_tutorial_program_count = 0u;
    s_tutorial_program_capacity = 0u;
}

void tutorial_script_program_install(const uint32_t* words, uint32_t count)
{
    uint32_t* grown;
    if (words == NULL) count = 0u;
    if (count == 0u) {
        s_tutorial_program_count = 0u;
        return;
    }
    if ((size_t)count > ((size_t)-1) / sizeof(*s_tutorial_program_words)) {
        s_tutorial_program_count = 0u;
        return;
    }
    if (count > s_tutorial_program_capacity) {
        grown = (uint32_t*)realloc(s_tutorial_program_words,
                                  (size_t)count * sizeof(*grown));
        if (grown == NULL) {
            s_tutorial_program_count = 0u;
            return;
        }
        s_tutorial_program_words = grown;
        s_tutorial_program_capacity = count;
    }
    memcpy(s_tutorial_program_words, words,
           (size_t)count * sizeof(*s_tutorial_program_words));
    s_tutorial_program_count = count;
}

/* Host-owned sidecars have no retail storage in the target's two cleared
 * global blocks. Retire them in the portable wrapper before entering the
 * mapped body, so the target keeps the retail call order and clear extents. */
static void tutorial_script_host_state_reset(void)
{
    objective_advisor_tokens_release();
    tutorial_tokens_clear();
    tutorial_script_data_release(s_tutorial_script_data);
    s_tutorial_script_data = NULL;
    s_tutorial_script_size = 0;
    tutorial_script_program_clear();
    memset(s_tutorial_nerp_registers, 0, sizeof(s_tutorial_nerp_registers));
    memset(&s_nerp_runtime, 0, sizeof(s_nerp_runtime));
    tutorial_camera_runtime_state_reset();
    tutorial_scroll_runtime_state_reset();
    if (s_tutorial_script_ops.state_reset != NULL)
        s_tutorial_script_ops.state_reset(s_tutorial_script_ops.context);
    if (s_tutorial_script_ops.resource_state_initialize != NULL)
        s_tutorial_script_ops.resource_state_initialize(
            s_tutorial_script_ops.context);
}

/* TutorialNerpScriptFileLoad @ 0x00452df0. The two global clear bands have
 * exact extents of 45 and 11 dwords; retain their unmapped fields as raw
 * sidecars while keeping the observed engine calls in their original order. */
// FUNCTION: LEGORR 0x00452df0
static int tutorial_script_file_load_target(const char* path)
{
    uint32_t size = 0u;
    void* data = NULL;
    int mode = 1;
    uint32_t* state_word = s_tutorial_555d40_state;
    int state_count = 45;
    for (; state_count != 0; --state_count) *state_word++ = 0u;
    ui_message_overlay_state_initialize();
    state_word = s_tutorial_4ff958_state;
    state_count = 11;
    for (; state_count != 0; --state_count) *state_word++ = 0u;
    if (s_tutorial_script_ops.mode_set != NULL)
        s_tutorial_script_ops.mode_set(mode, s_tutorial_script_ops.context);
    else
        (void)engine_mode_set_target(&mode);
    if (path != NULL) {
        if (s_tutorial_script_ops.load != NULL)
            data = s_tutorial_script_ops.load(
                path, &size, s_tutorial_script_ops.context);
        else {
            int32_t byte_size = 0;
            data = read_whole_file(path, &byte_size);
            if (byte_size > 0) size = (uint32_t)byte_size;
        }
    }
    s_tutorial_script_data = data;
    s_tutorial_script_size = size;
    return data != NULL;
}

int tutorial_script_file_load(const char* path)
{
    int loaded;
    tutorial_script_host_state_reset();
    /* The mapped body selects mode 1 once, immediately before loading the
     * script. Its optional adapter replaces that engine call on hosts that
     * provide their own mode owner. */
    loaded = tutorial_script_file_load_target(path);
    if (s_tutorial_script_data != NULL && s_tutorial_script_ops.load != NULL) {
        s_tutorial_script_release = s_tutorial_script_ops.release;
        s_tutorial_script_release_context = s_tutorial_script_ops.context;
    }
    if (s_tutorial_script_data != NULL &&
        s_tutorial_script_size >= sizeof(uint32_t)) {
        uint32_t count = s_tutorial_script_size / sizeof(uint32_t);
        tutorial_script_program_install(
            (const uint32_t*)s_tutorial_script_data, count);
    }
    return loaded;
}

static void tutorial_objective_token_output_snapshot(char** output_tokens)
{
    uint32_t i = 0u;
    s_tutorial_literal_token_count = 0u;
    while (i != 0xffffffffu && objective_advisor_token_text_get(i) != NULL) {
        if (output_tokens != NULL && i < 512u)
            output_tokens[i] = (char*)objective_advisor_token_text_get(i);
        ++i;
    }
    s_tutorial_literal_token_count = i;
}

static uintptr_t tutorial_objective_token_resolve(int kind, const char* value,
                                                  void* user)
{
    TutorialScriptOps* ops = (TutorialScriptOps*)user;
    if (ops == NULL || ops->resolve == NULL) return (uintptr_t)0;
    return (uintptr_t)ops->resolve(kind, value, ops->context);
}

static void tutorial_objective_token_release(int kind, uintptr_t handle,
                                             void* user)
{
    TutorialScriptOps* ops = (TutorialScriptOps*)user;
    if (ops != NULL && ops->resolved_token_release != NULL)
        ops->resolved_token_release(kind, (uint32_t)handle, ops->context);
}

int tutorial_objective_token_load(const char* path)
{
    return tutorial_objective_token_file_load(path, NULL);
}

/* LevelLoad passes NERPMessageFile separately from NERPFile. Keep the
 * message resource's lifetime scoped to token parsing; the parsed strings
 * and resolved handles are retained by the advisor token table. */
int tutorial_objective_token_file_load(const char* path,
                                       char** output_tokens)
{
    int loaded;
    if (output_tokens != NULL) output_tokens[0] = NULL;
    tutorial_tokens_clear();
    s_tutorial_literal_token_count = 0u;
    if (s_tutorial_script_ops.resolve == NULL) {
        loaded = objective_advisor_token_file_load(path);
    } else {
        Resource* resource;
        int32_t source_size;
        objective_advisor_tokens_release();
        if (s_tutorial_script_ops.resource_state_initialize != NULL)
            s_tutorial_script_ops.resource_state_initialize(
                s_tutorial_script_ops.context);
        if (path == NULL || path[0] == '\0') return 0;
        resource = resource_open(path, RESOURCE_AUTO);
        if (resource == NULL) return 0;
        source_size = resource_read(resource);
        s_advisor_token_ops = s_tutorial_script_ops;
        if (source_size >= 0) {
            loaded = objective_advisor_tokens_parse(
                (const char*)resource->data, (uint32_t)source_size,
                tutorial_objective_token_resolve,
                tutorial_objective_token_release, &s_advisor_token_ops);
        } else {
            loaded = 0;
        }
        resource_free(resource);
    }
    if (!loaded) return 0;
    tutorial_objective_token_output_snapshot(output_tokens);
    return 1;
}

const char* tutorial_objective_token_lookup(uint32_t index)
{
    /* LevelLoad currently calls objective_advisor_token_file_load directly,
     * so look through its live table even when this module's optional output
     * snapshot has not been requested. */
    return objective_advisor_token_text_get(index);
}

// FUNCTION: LEGORR 0x00453220
int tutorial_display_resources_release(void)
{
    tutorial_script_data_release(s_tutorial_script_data);
    s_tutorial_script_data = NULL;
    s_tutorial_script_size = 0;
    tutorial_script_program_clear();
    memset(s_tutorial_nerp_registers, 0, sizeof(s_tutorial_nerp_registers));
    memset(&s_nerp_runtime, 0, sizeof(s_nerp_runtime));
    tutorial_camera_focus_state_reset();
    tutorial_scroll_runtime_state_reset();
    if (s_tutorial_engine_context != NULL &&
        s_tutorial_engine_context->views[0] != NULL) {
        s_tutorial_engine_context->views[0]->target_record = NULL;
        s_tutorial_engine_context->views[0]->follow_id = 0;
    }
    objective_advisor_tokens_release();
    tutorial_tokens_clear();
    if (s_tutorial_script_ops.resource_state_release != NULL)
        s_tutorial_script_ops.resource_state_release(
            s_tutorial_script_ops.context);
    /* AdvisorDisplayStateResourcesRelease reinitializes the shared message
     * overlay after clearing the tutorial-owned resource tables. */
    ui_message_overlay_state_initialize();
    return 1;
}

static uint32_t tutorial_command_kind_default(uint32_t command);

// FUNCTION: LEGORR 0x004532e0
void tutorial_indirect_handle_resolve(uint32_t* value)
{
    uint32_t index;
    uint32_t kind;
    uint32_t resolved;
    if (value == NULL || (*value & 0xffff0000u) != 0x00020000u) return;
    index = *value & 0xffffu;
    kind = s_tutorial_script_ops.command_kind != NULL
        ? s_tutorial_script_ops.command_kind(
            index, s_tutorial_script_ops.context)
        : tutorial_command_kind_default(index);
    if (kind != 0u) return;
    if (s_tutorial_script_ops.indirect_resolve != NULL)
        *value = s_tutorial_script_ops.indirect_resolve(
            index, s_tutorial_script_ops.context);
    else if (s_tutorial_script_ops.command != NULL &&
             s_tutorial_script_ops.command !=
                 tutorial_builtin_command_adapter)
        *value = s_tutorial_script_ops.command(
            index, NULL, 0u, s_tutorial_script_ops.context);
    else if (tutorial_builtin_command(index, NULL, 0u, &resolved))
        *value = resolved;
}

static uint32_t tutorial_compare_values(uint32_t op, uint32_t previous,
                                        uint32_t current)
{
    switch (op) {
    case 1: return previous != 0u && current != 0u;
    case 2: return previous != 0u || current != 0u;
    case 3: return current < previous;
    case 4: return previous < current;
    case 5: return previous == current;
    case 6: return current <= previous;
    case 7: return previous <= current;
    case 8: return previous != current;
    default: return current;
    }
}

static uint32_t tutorial_command_kind_default(uint32_t command)
{
    return command < 293u ? s_tutorial_command_kinds[command] : 0u;
}

static int tutorial_command_invoke(uint32_t command, const uint32_t* args,
                                   uint32_t argc, uint32_t* result)
{
    if (result == NULL) return 0;
    if (s_tutorial_script_ops.command != NULL) {
        *result = s_tutorial_script_ops.command(
            command, argc == 0u ? NULL : args, argc,
            s_tutorial_script_ops.context);
        return 1;
    }
    return tutorial_builtin_command(command,
        argc == 0u ? NULL : args, argc, result);
}

// FUNCTION: LEGORR 0x00453bb0
void tutorial_frame_timer_array_advance(float delta)
{
    const float timer_delta = delta * 1000.0f * 0.04f;
    uint32_t i;
    tutorial_message_generation_sync();
    for (i = 0u; i < 4u; ++i)
        s_tutorial_nerp_timers[i] += timer_delta;
    if (s_nerp_runtime.message_timer == 0.0f) return;
    if (s_nerp_runtime.message_timer >= 0.0f) {
        if (deferred_callback_peek() == 0u ||
            s_nerp_runtime.message_flags != 0u) {
            s_nerp_runtime.message_timer -= timer_delta;
            if (s_nerp_runtime.message_timer > 0.0f) goto message_tail;
        } else {
            if (s_nerp_runtime.message_previous_ack != 0u &&
                (((int32_t)s_nerp_runtime.message_offset > 0 &&
                  s_nerp_runtime.message_suppress_arrow != 0u) ||
                 s_nerp_runtime.message_suppress_arrow == 0u)) {
                engine_global_state_flags_bits6_and_9_set_target(
                    s_nerp_runtime.message_pending_ack != 0u, 1);
            }
            s_nerp_runtime.message_previous_ack =
                s_nerp_runtime.message_pending_ack;
            if (s_nerp_runtime.message_pending_ack != 0u) goto message_tail;
        }
    }
    s_nerp_runtime.message_timer = 0.0f;
message_tail:
    if (s_nerp_runtime.message_sound_active != 0u &&
        s_nerp_runtime.message_timer < 1000.0f)
        advisor_clear_active_state();
}

static int tutorial_grid_count_command_type(uint32_t command,
                                            const char** type_name)
{
    static const struct {
        uint16_t command;
        const char* type_name;
    } counts[] = {
        { 52u, "Pilot" }, { 75u, "Barracks" }, { 76u, "OreRefinery" },
        { 77u, "Toolstation" }, { 78u, "ProcessedOre" },
        { 79u, "TeleportPad" }, { 100u, "CrystalRefinery" },
        { 113u, "Path" }, { 126u, "GunStation" },
        { 128u, "TeleportBIG" }, { 135u, "Upgrade" },
        { 170u, "Powerstation" }, { 185u, "Geo-dome" },
        { 188u, "Slug" }, { 193u, "WalkerDigger" },
        { 194u, "SmallDigger" }, { 195u, "Docks" },
        { 196u, "SmallCat" }, { 198u, "SmallHeli" }
    };
    uint32_t i;
    for (i = 0u; i < sizeof(counts) / sizeof(counts[0]); ++i) {
        if (counts[i].command == command) {
            *type_name = counts[i].type_name;
            return 1;
        }
    }
    return 0;
}

static int tutorial_entity_type_count_get(const char* type_name)
{
    int type = 0;
    int row = 0;
    int total = 0;
    int slot;
    if (!entity_type_row_for_name(type_name, &type, &row)) return 0;
    /* These wrappers call FUN_004551f0(name, -1), which walks the type's
     * grid-count slots through FUN_00437460 (second minus first table). */
    for (slot = 0; slot < 16; ++slot) {
        int first = grid_count_table_slot_peek(type, row, slot, 0);
        int second = grid_count_table_slot_peek(type, row, slot, 1);
        if (first >= 0 && second >= 0) total += second - first;
    }
    return total;
}

static int tutorial_entity_type_count_get_at_slot(const char* type_name,
                                                 int slot)
{
    int type = 0;
    int row = 0;
    int first;
    int second;
    if (!entity_type_row_for_name(type_name, &type, &row)) return 0;
    first = grid_count_table_slot_peek(type, row, slot, 0);
    second = grid_count_table_slot_peek(type, row, slot, 1);
    return first < 0 || second < 0 ? 0 : second - first;
}

static int tutorial_building_level_command(uint32_t command,
                                           const char** type_name,
                                           int* count_slot)
{
    static const char* const names[9] = {
        "Barracks", "Docks", "Geo-dome", "Powerstation",
        "Toolstation", "GunStation", "TeleportPad", "TeleportBIG",
        "Upgrade"
    };
    if (command >= 266u && command <= 274u) {
        *type_name = names[command - 266u];
        *count_slot = 1;
        return 1;
    }
    if (command >= 275u && command <= 283u) {
        *type_name = names[command - 275u];
        *count_slot = 2;
        return 1;
    }
    if (command >= 284u && command <= 292u) {
        *type_name = names[command - 284u];
        *count_slot = -2;
        return 1;
    }
    return 0;
}

static int tutorial_powered_entity_count_get(const GameContext* game,
                                             const char* type_name)
{
    int type = 0;
    int row = 0;
    int total = 0;
    int table;
    if (game == NULL || !entity_type_row_for_name(type_name, &type, &row))
        return 0;
    for (table = 0; table < ENTITY_TYPES; ++table) {
        int i;
        int limit = game->entity_count[table];
        if (limit < 0) limit = 0;
        if (limit > ENTITY_CAP) limit = ENTITY_CAP;
        for (i = 0; i < limit; ++i) {
            const EntitySlot* entity = &game->entity[table][i];
            if (entity->type == type && entity->etype == row &&
                (entity->flags_3e8 & 0x20000000u) != 0u)
                ++total;
        }
    }
    return total;
}

static int tutorial_selected_type_command_name(uint32_t command,
                                               const char** type_name)
{
    static const struct {
        uint16_t command;
        const char* type_name;
    } selected[] = {
        { 80u, "Pilot" }, { 81u, "SmallDigger" },
        { 120u, "WalkerDigger" }, { 121u, "LargeDigger" },
        { 145u, "SmallHeli" }, { 146u, "SmallCat" },
        { 224u, "SmallTruck" }, { 236u, "Barracks" },
        { 237u, "Docks" }, { 238u, "Geo-Dome" },
        { 239u, "Powerstation" }, { 240u, "Toolstation" },
        { 241u, "Gunstation" }, { 242u, "TeleportPad" },
        { 243u, "TeleportBIG" }, { 244u, "Upgrade" }
    };
    uint32_t i;
    for (i = 0u; i < sizeof(selected) / sizeof(selected[0]); ++i) {
        if (selected[i].command == command) {
            *type_name = selected[i].type_name;
            return 1;
        }
    }
    return 0;
}

static int tutorial_selected_type_get(const GameContext* game,
                                      const char* type_name)
{
    EntitySlot* entity;
    int type = 0;
    int row = 0;
    if (!entity_type_row_for_name(type_name, &type, &row)) return 0;
    entity = selection_first(game);
    return entity != NULL && entity->type == type && entity->etype == row;
}

static int tutorial_building_level_command_name(uint32_t command,
                                                const char** type_name)
{
    static const struct {
        uint16_t command;
        const char* type_name;
    } levels[] = {
        { 227u, "Barracks" }, { 228u, "Docks" },
        { 229u, "Geo-dome" }, { 230u, "Powerstation" },
        { 231u, "Toolstation" }, { 232u, "Gunstation" },
        { 233u, "TeleportPad" }, { 234u, "TeleportBIG" },
        { 235u, "Upgrade" }
    };
    uint32_t i;
    for (i = 0u; i < sizeof(levels) / sizeof(levels[0]); ++i) {
        if (levels[i].command == command) {
            *type_name = levels[i].type_name;
            return 1;
        }
    }
    return 0;
}

static void tutorial_building_level_set(GameContext* game,
                                        const char* type_name,
                                        uint32_t state)
{
    int type = 0;
    int row = 0;
    int table;
    if (game == NULL || !entity_type_row_for_name(type_name, &type, &row))
        return;
    for (table = 0; table < ENTITY_TYPES; ++table) {
        int i;
        int limit = game->entity_count[table];
        if (limit < 0) limit = 0;
        if (limit > ENTITY_CAP) limit = ENTITY_CAP;
        for (i = 0; i < limit; ++i) {
            EntitySlot* entity = &game->entity[table][i];
            if (entity->type == type && entity->etype == row)
                (void)entity_animation_state_set(entity, state);
        }
    }
}

static int tutorial_vehicle_occupant_type_command_name(
    uint32_t command, const char** type_name)
{
    static const struct {
        uint16_t command;
        const char* type_name;
    } occupants[] = {
        { 82u, "SmallDigger" }, { 147u, "SmallCat" },
        { 207u, "WalkerDigger" }, { 208u, "LargeDigger" },
        { 225u, "SmallTruck" }, { 226u, "SmallHeli" }
    };
    uint32_t i;
    for (i = 0u; i < sizeof(occupants) / sizeof(occupants[0]); ++i) {
        if (occupants[i].command == command) {
            *type_name = occupants[i].type_name;
            return 1;
        }
    }
    return 0;
}

static int tutorial_vehicle_occupant_count_get(const GameContext* game,
                                                const char* type_name)
{
    int type = 0;
    int row = 0;
    int count = 0;
    int table;
    if (game == NULL || !entity_type_row_for_name(type_name, &type, &row))
        return 0;
    for (table = 0; table < ENTITY_TYPES; ++table) {
        int i;
        int limit = game->entity_count[table];
        if (limit < 0) limit = 0;
        if (limit > ENTITY_CAP) limit = ENTITY_CAP;
        for (i = 0; i < limit; ++i) {
            const EntitySlot* entity = &game->entity[table][i];
            if (entity->type == type && entity->etype == row &&
                entity->owner_36c != NULL) {
                ++count;
            }
        }
    }
    return count;
}

static uint32_t tutorial_coordinate_field_sum(LevelState* level,
                                             uint32_t list_index,
                                             uint32_t field_offset)
{
    void* node;
    uint32_t total = 0u;
    uint32_t visited = 0u;
    if (level == NULL || list_index >= 0x37u) return 0u;
    node = level->pool_heads[list_index];
    while (node != NULL && visited < NERP_TUTORIAL_CELL_CAPACITY) {
        int32_t x;
        int32_t y;
        uint8_t* voxel;
        memcpy(&x, node, sizeof(x));
        memcpy(&y, (char*)node + 4, sizeof(y));
        voxel = level_voxel(level, x, y);
        if (voxel != NULL) {
            uint32_t value;
            memcpy(&value, voxel + field_offset, sizeof(value));
            total += value;
        }
        node = *(void**)((char*)node + 8);
        ++visited;
    }
    return total;
}

static uint32_t tutorial_coordinate_flag_sum(LevelState* level,
                                             uint32_t list_index,
                                             uint32_t flag_mask)
{
    void* node;
    uint32_t total = 0u;
    uint32_t visited = 0u;
    if (level == NULL || list_index >= 0x37u) return 0u;
    node = level->pool_heads[list_index];
    while (node != NULL && visited < NERP_TUTORIAL_CELL_CAPACITY) {
        int32_t x;
        int32_t y;
        uint8_t* voxel;
        uint32_t flags = 0u;
        memcpy(&x, node, sizeof(x));
        memcpy(&y, (char*)node + 4, sizeof(y));
        voxel = level_voxel(level, x, y);
        if (voxel != NULL) {
            memcpy(&flags, voxel + 8, sizeof(flags));
            total += flags & flag_mask;
        }
        node = *(void**)((char*)node + 8);
        ++visited;
    }
    return total;
}

static int tutorial_coordinate_field_set(LevelState* level,
                                         uint32_t list_index,
                                         uint32_t field_offset,
                                         uint32_t value)
{
    void* node;
    uint32_t visited = 0u;
    int changed = 0;
    if (level == NULL || list_index >= 0x37u) return 0;
    node = level->pool_heads[list_index];
    while (node != NULL && visited < NERP_TUTORIAL_CELL_CAPACITY) {
        int32_t x;
        int32_t y;
        uint8_t* voxel;
        memcpy(&x, node, sizeof(x));
        memcpy(&y, (char*)node + 4, sizeof(y));
        voxel = level_voxel(level, x, y);
        if (voxel != NULL) {
            memcpy(voxel + field_offset, &value, sizeof(value));
            ++changed;
        }
        node = *(void**)((char*)node + 8);
        ++visited;
    }
    return changed;
}

typedef struct TutorialPointerEntityMark {
    LevelState* level;
    int32_t x;
    int32_t y;
} TutorialPointerEntityMark;

static int tutorial_pointer_entity_cell_get(const EntitySlot* entity,
                                             LevelState* level,
                                             int* x, int* y)
{
    float xy[2];
    if (entity == NULL || level == NULL || level->ptr_terrain == NULL ||
        terrain_blocksize_get(level->ptr_terrain) == 0.0f) return 0;
    entity_transform_position_get(entity, xy);
    /* 0x4424f0 consumes the converted outputs even outside physical bounds.
     * Keep that contract separate from height/picker admission. */
    (void)terrain_world_xy_to_cell(level->ptr_terrain, xy[0], xy[1], x, y);
    return 1;
}

/* 0x4570d0: assignment for each visited cell, never a union of permissions. */
static int tutorial_pointer_entity_mark(EntitySlot* entity, void* payload)
{
    TutorialPointerEntityMark* mark = (TutorialPointerEntityMark*)payload;
    int x = -1;
    int y = -1;
    if (entity != NULL && mark != NULL) {
        entity->tutorial_unit_at_block_3f8 =
            tutorial_pointer_entity_cell_get(entity, mark->level, &x, &y) &&
            x == mark->x && y == mark->y ? 1u : 0u;
    }
    return 0;
}

/* 0x456cb0/context11 -> 0x456f2e -> filtered retail pool visitor. */
static void tutorial_pointer_context11(GameContext* game, uint32_t list_index)
{
    void* node;
    uint32_t visited = 0u;
    if (game == NULL || game->level == NULL || list_index >= 0x37u) return;
    node = game->level->pool_heads[list_index];
    while (node != NULL && visited < NERP_TUTORIAL_CELL_CAPACITY) {
        TutorialPointerEntityMark mark;
        mark.level = game->level;
        memcpy(&mark.x, node, sizeof(mark.x));
        memcpy(&mark.y, (char*)node + 4, sizeof(mark.y));
        (void)entity_pool_find_by_callback_filtered(game,
            tutorial_pointer_entity_mark, &mark);
        node = *(void**)((char*)node + 8);
        ++visited;
    }
}

static void tutorial_coordinate_marker_set(GameContext* game,
                                           uint32_t list_index,
                                           uint32_t enabled)
{
    LevelState* level = game != NULL ? game->level : NULL;
    void* node;
    uint32_t visited = 0u;
    uint32_t slot;
    EntitySlot* actor;
    if (level == NULL) return;
    node = list_index < 0x37u ? level->pool_heads[list_index] : NULL;
    while (node != NULL && visited < NERP_TUTORIAL_CELL_CAPACITY) {
        int32_t x;
        int32_t y;
        uint8_t* voxel;
        memcpy(&x, node, sizeof(x));
        memcpy(&y, (char*)node + 4, sizeof(y));
        voxel = level_voxel(level, x, y);
        if (enabled == 1u) {
            if (voxel != NULL) voxel[4] = (uint8_t)list_index;
            /* 0x4571b4..0x4571cf repeats the full context11 list visit
             * after EACH marker write, in the authored chain order. */
            tutorial_pointer_context11(game, list_index);
        }
        else if (enabled == 0u && voxel != NULL) voxel[4] = 0u;
        node = *(void**)((char*)node + 8);
        ++visited;
    }
    /* 0x4571fd..0x45724a runs even on disable/other arguments or an empty
     * list. Reconcile only the actual current OList actor at its own cell. */
    slot = tutorial_record_object_pointer_get();
    actor = slot != 0u ? object_pointer_registry_entity_get(slot - 1u) : NULL;
    if (actor != NULL) {
        TutorialPointerEntityMark mark;
        int x, y;
        mark.level = level;
        if (tutorial_pointer_entity_cell_get(actor, level, &x, &y)) {
            mark.x = x;
            mark.y = y;
            (void)tutorial_pointer_entity_mark(actor, &mark);
        }
    }
}

static void tutorial_coordinate_variant_flag_propagate(
    LevelState* level, uint32_t list_index)
{
    void* node;
    uint32_t visited = 0u;
    uint32_t visit_limit;
    if (level == NULL || list_index >= 0x37u) return;
    visit_limit = (uint32_t)level_width(level) *
                  (uint32_t)level_height(level);
    node = level->pool_heads[list_index];
    while (node != NULL && visited < visit_limit) {
        int32_t x;
        int32_t y;
        int cell_xy[2];
        memcpy(&x, node, sizeof(x));
        memcpy(&y, (char*)node + 4, sizeof(y));
        cell_xy[0] = x;
        cell_xy[1] = y;
        /* Context 4 clears the low state before the state/path owners run;
         * the wrapper's second NERP argument is not the mutation mode. */
        {
            uint8_t* voxel = level_voxel(level, x, y);
            if (voxel != NULL) {
                uint32_t flags;
                memcpy(&flags, voxel + 8, sizeof(flags));
                flags &= ~3u;
                memcpy(voxel + 8, &flags, sizeof(flags));
            }
        }
        (void)terrain_cell_state_update(level, cell_xy);
        (void)terrain_cell_variant_flag_propagate(level, cell_xy);
        node = *(void**)((char*)node + 8);
        ++visited;
    }
}

static void tutorial_coordinate_rock_monster_spawn(GameContext* game,
                                                    uint32_t list_index)
{
    LevelState* level;
    uint32_t owner_index;
    const EntitySlot* owner;
    void* node;
    uint32_t visited = 0u;
    if (game == NULL || game->level == NULL || list_index >= 0x37u) return;
    level = game->level;
    owner_index = active_engine_field_b0_get(level);
    owner = entity_at(game, 3, (int)owner_index);
    if (owner == NULL) return;
    node = level->pool_heads[list_index];
    while (node != NULL && visited < NERP_TUTORIAL_CELL_CAPACITY) {
        int32_t x;
        int32_t y;
        EntitySlot* spawned;
        memcpy(&x, node, sizeof(x));
        memcpy(&y, (char*)node + 4, sizeof(y));
        spawned = unported_ledger_stubs::creature_spawn_from_emergence(
            (void*)owner, 3, (int)owner_index, x, y);
        if (spawned != NULL)
            entity_stats_float_ate8_set(spawned, 60.0f);
        node = *(void**)((char*)node + 8);
        ++visited;
    }
}

static int tutorial_entity_matches_coordinate_list(const EntitySlot* entity,
                                                  LevelState* level,
                                                  uint32_t list_index)
{
    void* node;
    int cell_x = -1;
    int cell_y = -1;
    uint32_t visited = 0u;
    if (entity == NULL || level == NULL || list_index >= 0x37u) return 0;
    entity_position_to_terrain_cell(entity, level, &cell_x, &cell_y);
    node = level->pool_heads[list_index];
    while (node != NULL && visited < NERP_TUTORIAL_CELL_CAPACITY) {
        int32_t x;
        int32_t y;
        memcpy(&x, node, sizeof(x));
        memcpy(&y, (char*)node + 4, sizeof(y));
        if (x == cell_x && y == cell_y) return 1;
        node = *(void**)((char*)node + 8);
        ++visited;
    }
    return 0;
}

typedef struct TutorialCoordinateEntityCount {
    LevelState* level;
    int32_t x;
    int32_t y;
    uint32_t count;
} TutorialCoordinateEntityCount;

static int tutorial_coordinate_entity_count_callback(EntitySlot* entity,
                                                       void* payload)
{
    TutorialCoordinateEntityCount* state =
        (TutorialCoordinateEntityCount*)payload;
    int cell_x = -1;
    int cell_y = -1;
    if (entity == NULL || state == NULL || state->level == NULL) return 0;
    entity_position_to_terrain_cell(entity, state->level, &cell_x, &cell_y);
    if (cell_x == state->x && cell_y == state->y) ++state->count;
    return 0;
}

static uint32_t tutorial_entity_count_at_coordinate_list(
    GameContext* game, uint32_t list_index)
{
    LevelState* level;
    void* node;
    uint32_t visited = 0u;
    uint32_t total = 0u;
    if (game == NULL || game->level == NULL || list_index >= 0x37u)
        return 0u;
    level = game->level;
    node = level->pool_heads[list_index];
    while (node != NULL && visited < NERP_TUTORIAL_CELL_CAPACITY) {
        TutorialCoordinateEntityCount state;
        memcpy(&state.x, node, sizeof(state.x));
        memcpy(&state.y, (char*)node + 4, sizeof(state.y));
        state.level = level;
        state.count = 0u;
        (void)entity_pool_find_by_callback_filtered(
            game, tutorial_coordinate_entity_count_callback, &state);
        total += state.count;
        node = *(void**)((char*)node + 8);
        ++visited;
    }
    return total;
}

static int tutorial_set_monster_attack_callback(EntitySlot* entity,
                                                 void* payload)
{
    LevelState* level = (LevelState*)payload;
    int cell_x = -1;
    int cell_y = -1;
    if (entity == NULL || level == NULL) return 0;
    entity_position_to_terrain_cell(entity, level, &cell_x, &cell_y);
    if (level_cell_occupied_or_flag(level, cell_x, cell_y, 1) != 0u)
        task_record_kind10_queue(entity);
    return 0;
}

static uint32_t tutorial_record_object_at_list(LevelState* level,
                                               uint32_t list_index)
{
    uint32_t count = object_pointer_registry_count_get();
    uint32_t index;
    if (count > 10u) count = 10u;
    for (index = 0u; index < count; ++index) {
        EntitySlot* entity = object_pointer_registry_entity_get(index);
        if (tutorial_entity_matches_coordinate_list(entity, level,
                                                   list_index))
            return index + 1u;
    }
    return 0u;
}

/* GetSelectedRecordObject @ 0x454220 queries OList slots, not positions in
 * the selection array. FUN_00452610(entity, NULL) tests the selected bit. */
static uint32_t tutorial_selected_record_object_get(void)
{
    uint32_t index;
    for (index = 0u; index < 10u; ++index) {
        EntitySlot* entity = object_pointer_registry_entity_get(index);
        if (entity != NULL &&
            selection_contains_entity_active_target(entity, NULL) != 0u)
            return index + 1u;
    }
    return 0u;
}

uint32_t tutorial_record_object_pointer_get(void)
{
    /* This is a one-based OList slot, not a retained entity pointer. */
    return s_nerp_runtime.tutorial_record_pointer;
}

static uint32_t tutorial_record_object_amount_at_list(LevelState* level,
                                                      uint32_t list_index)
{
    uint32_t count = object_pointer_registry_count_get();
    uint32_t index;
    uint32_t total = 0u;
    if (count > 10u) count = 10u;
    for (index = 0u; index < count; ++index) {
        EntitySlot* entity = object_pointer_registry_entity_get(index);
        if (tutorial_entity_matches_coordinate_list(entity, level,
                                                   list_index))
            ++total;
    }
    return total;
}

static int tutorial_camera_at_coordinate_list(GameContext* game,
                                             EngineContext* engine,
                                             uint32_t list_index)
{
    float position[3];
    int cell_x = -1;
    int cell_y = -1;
    void* node;
    uint32_t visited = 0u;
    if (game == NULL || engine == NULL || game->level == NULL ||
        engine->views[0] == NULL || list_index >= 0x37u) return 0;
    (void)camera_get_position_with_terrain_height(engine->views[0], engine,
                                                  position);
    if (!terrain_world_xy_to_cell(game->level->ptr_terrain,
                                  position[0], position[1],
                                  &cell_x, &cell_y)) return 0;
    node = game->level->pool_heads[list_index];
    while (node != NULL && visited < NERP_TUTORIAL_CELL_CAPACITY) {
        int32_t x;
        int32_t y;
        memcpy(&x, node, sizeof(x));
        memcpy(&y, (char*)node + 4, sizeof(y));
        if (x == cell_x && y == cell_y) return 1;
        node = *(void**)((char*)node + 8);
        ++visited;
    }
    return 0;
}

static void tutorial_camera_goto_coordinate_list(GameContext* game,
                                                 EngineContext* engine,
                                                 uint32_t list_index)
{
    ViewportTargetClampState clamp_state;
    void* node;
    uint32_t visited = 0u;
    if (game == NULL || engine == NULL || game->level == NULL ||
        list_index >= 0x37u) return;
    memset(&clamp_state, 0, sizeof(clamp_state));
    clamp_state.map_width = level_width(game->level) - 1;
    clamp_state.map_height = level_height(game->level) - 1;
    clamp_state.edge_extent = view_grid_edge_get();
    clamp_state.global_flags = engine->state;
    node = game->level->pool_heads[list_index];
    while (node != NULL && visited < NERP_TUTORIAL_CELL_CAPACITY) {
        int32_t x;
        int32_t y;
        int cell_xy[2];
        memcpy(&x, node, sizeof(x));
        memcpy(&y, (char*)node + 4, sizeof(y));
        cell_xy[0] = x;
        cell_xy[1] = y;
        (void)viewport_target_position_clamp(&clamp_state, cell_xy);
        s_nerp_runtime.camera_target_cells[1][0] =
            clamp_state.target[0];
        s_nerp_runtime.camera_target_cells[1][1] =
            clamp_state.target[1];
        s_nerp_runtime.camera_target_valid[1] = 1u;
        node = *(void**)((char*)node + 8);
        ++visited;
    }
    engine->state = clamp_state.global_flags;
    if (s_nerp_runtime.camera_target_valid[1] != 0u &&
        engine->views[0] != NULL) {
        float world[3];
        if (terrain_cell_world_position_get(
                s_nerp_runtime.camera_target_cells[1][0],
                s_nerp_runtime.camera_target_cells[1][1], world) != 0) {
            view_primary_target_request(engine, world[0], world[1], 1);
        }
    }
}

static int tutorial_camera_focus_target_apply(GameContext* game,
                                              EngineContext* engine,
                                              uint32_t target_index)
{
    EntitySlot* entity;
    int cell_x;
    int cell_y;
    int cell_xy[2];
    ViewportTargetClampState clamp_state;
    float world[3];
    if (game == NULL || engine == NULL || game->level == NULL ||
        target_index >= object_pointer_registry_count_get()) return 0;
    entity = object_pointer_registry_entity_get(target_index);
    if (entity == NULL) return 0;
    entity_position_to_terrain_cell(entity, game->level, &cell_x, &cell_y);
    cell_xy[0] = cell_x;
    cell_xy[1] = cell_y;
    memset(&clamp_state, 0, sizeof(clamp_state));
    clamp_state.map_width = level_width(game->level) - 1;
    clamp_state.map_height = level_height(game->level) - 1;
    clamp_state.edge_extent = view_grid_edge_get();
    clamp_state.global_flags = engine->state;
    /* 0x42de40 preserves an object's exact world point unless the map
     * inset actually changes its cell. Do not quantize scripted follow. */
    entity_transform_position_get(entity, world);
    if (viewport_target_position_clamp(&clamp_state, cell_xy) &&
        !terrain_cell_world_center_xy_get(game->level->ptr_terrain,
            clamp_state.target[0], clamp_state.target[1], world)) return 0;
    engine->state = clamp_state.global_flags;
    view_primary_target_request(engine, world[0], world[1], 1);
    return 1;
}

static void tutorial_unit_at_coordinate_list_set(GameContext* game,
                                                 uint32_t list_index)
{
    int table;
    if (game == NULL) return;
    for (table = 0; table < ENTITY_TYPES; ++table) {
        int i;
        int limit = game->entity_count[table];
        if (limit < 0) limit = 0;
        if (limit > ENTITY_CAP) limit = ENTITY_CAP;
        for (i = 0; i < limit; ++i) {
            EntitySlot* entity = &game->entity[table][i];
            if ((entity->flags_3e8 & 0x80000u) != 0u) continue;
            entity->tutorial_unit_at_block_3f8 =
                tutorial_entity_matches_coordinate_list(
                    entity, game->level, list_index) ? 1u : 0u;
        }
    }
}

/* The eight NERP arithmetic registers are fully described by the callback
 * table: GetR0..7 return a register, SetR0..7 replace it, AddR0..7 add one
 * argument, and SubR0..7 subtract one argument. Other callback IDs still
 * need their game-specific effects bound. */
static int tutorial_builtin_command(uint32_t command, const uint32_t* args,
                                    uint32_t argc, uint32_t* result_out)
{
    uint32_t reg;
    const char* count_type;
    const char* selected_type;
    const char* building_type;
    const char* vehicle_type;
    if (command == 1u) {
        *result_out = 1u;
        return 1;
    }
    if (command == 2u || command == 3u) {
        *result_out = 0u;
        return 1;
    }
    if (command == 8u || command == 10u) {
        /* Table IDs 8/10 point to ObjectiveVoiceAliasRaiseCommit. */
        *result_out = (uint32_t)objective_voice_alias_raise_commit(
            s_tutorial_game_context != NULL
                ? s_tutorial_game_context->level : NULL);
        return 1;
    }
    if (command == 12u) {
        *result_out = s_tutorial_game_context != NULL
            ? level_field94_get(s_tutorial_game_context->level) : 0u;
        return 1;
    }
    if (command == 49u && argc == 1u && args != NULL) {
        /* SetMessagePermit is the one-argument EngineModeSet callback. */
        int mode = (int32_t)args[0];
        *result_out = (uint32_t)engine_mode_set_target(&mode);
        return 1;
    }
    if (command == 16u && argc == 2u && args != NULL) {
        uint32_t token_index = args[0] == 0u ? 0u : args[0] - 1u;
        uint32_t slot;
        const char* token_text;
        if (global_flag_004ff978_get() != 0u) {
            *result_out = 0u;
            return 1;
        }
        token_text = tutorial_objective_token_lookup(token_index);
        tutorial_message_generation_sync();
        slot = s_nerp_runtime.message_maximum;
        if (tutorial_message_history_get(slot) != token_index) {
            if (slot == 8u) {
                memmove(s_nerp_runtime.message_history,
                        s_nerp_runtime.message_history + 4,
                        4u * sizeof(uint32_t));
                slot = 4u;
            }
            s_nerp_runtime.message_history[slot] = token_index;
            s_nerp_runtime.message_maximum = slot + 1u;
            engine_global_state_flags_bit7_set_target(1);
            if (s_nerp_runtime.message_offset != 0u)
                ++s_nerp_runtime.message_offset;
        }
        advisor_text_request_setup_text(token_text, args[1]);
        s_nerp_runtime.message_flags = args[1];
        s_nerp_runtime.message_pending_ack = 1u;
        *result_out = args[0];
        return 1;
    }
    if (command == 88u && argc == 1u && args != NULL) {
        s_nerp_runtime.message_wait = args[0];
        *result_out = args[0];
        return 1;
    }
    if (command == 109u && argc == 1u && args != NULL) {
        s_nerp_runtime.tutorial_block_clicks = tutorial_coordinate_field_sum(
            s_tutorial_game_context != NULL
                ? s_tutorial_game_context->level : NULL,
            args[0], 0x28u);
        *result_out = s_nerp_runtime.tutorial_block_clicks;
        return 1;
    }
    if (command == 110u && argc == 2u && args != NULL) {
        (void)tutorial_coordinate_field_set(
            s_tutorial_game_context != NULL
                ? s_tutorial_game_context->level : NULL,
            args[0], 0x28u, args[1]);
        s_nerp_runtime.tutorial_block_clicks = args[1];
        *result_out = 1u;
        return 1;
    }
    if (command == 111u && argc == 1u && args != NULL) {
        s_nerp_runtime.tutorial_crystals = tutorial_coordinate_field_sum(
            s_tutorial_game_context != NULL
                ? s_tutorial_game_context->level : NULL,
            args[0], 0x2cu);
        *result_out = s_nerp_runtime.tutorial_crystals;
        return 1;
    }
    if (command == 112u && argc == 2u && args != NULL) {
        (void)tutorial_coordinate_field_set(
            s_tutorial_game_context != NULL
                ? s_tutorial_game_context->level : NULL,
            args[0], 0x2cu, args[1]);
        s_nerp_runtime.tutorial_crystals = args[1];
        *result_out = 1u;
        return 1;
    }
    if (command == 114u && argc == 1u && args != NULL) {
        s_nerp_runtime.tutorial_block_type_flags =
            tutorial_coordinate_flag_sum(
            s_tutorial_game_context != NULL
                ? s_tutorial_game_context->level : NULL,
            args[0], 0x8u);
        *result_out = s_nerp_runtime.tutorial_block_type_flags;
        return 1;
    }
    if (command == 136u && argc == 2u && args != NULL) {
        LevelState* level = s_tutorial_game_context != NULL
            ? s_tutorial_game_context->level : NULL;
        void* node;
        uint32_t visited = 0u;
        if (level != NULL && args[0] < 0x37u) {
            node = level->pool_heads[args[0]];
            while (node != NULL && visited < NERP_TUTORIAL_CELL_CAPACITY) {
                int32_t x;
                int32_t y;
                memcpy(&x, node, sizeof(x));
                memcpy(&y, (char*)node + 4, sizeof(y));
                (void)terrain_cell_mutate(level, (uint32_t)x,
                                          (uint32_t)y, 0);
                node = *(void**)((char*)node + 8);
                ++visited;
            }
        }
        *result_out = 1u;
        return 1;
    }
    if (command == 122u && argc == 1u && args != NULL) {
        s_nerp_runtime.tutorial_block_type_flags =
            tutorial_coordinate_flag_sum(
            s_tutorial_game_context != NULL
                ? s_tutorial_game_context->level : NULL,
            args[0], 0x20000000u);
        *result_out = s_nerp_runtime.tutorial_block_type_flags;
        return 1;
    }
    if (command == 137u && argc == 2u && args != NULL) {
        tutorial_coordinate_variant_flag_propagate(
            s_tutorial_game_context != NULL
                ? s_tutorial_game_context->level : NULL,
            args[0]);
        /* SetTutorialBlockIsPath initializes and returns a zero result. */
        *result_out = 0u;
        return 1;
    }
    if (command == 138u && argc == 1u && args != NULL) {
        if (args[0] != 0u) {
            s_tutorial_focus_id = args[0] - 1u;
            s_tutorial_focus_valid = 1u;
            /* Reference 0x454050 records a target only. The primary
             * remains kind 1; kind 3 is the separate free camera. */
        }
        *result_out = 0u;
        return 1;
    }
    if (command == 139u) {
        s_tutorial_focus_id = 0xffffffffu;
        s_tutorial_focus_valid = 0u;
        *result_out = 0u;
        return 1;
    }
    if ((command == 140u || command == 141u) && argc == 1u &&
        args != NULL) {
        s_tutorial_focus_offset_target = command == 140u
            ? -(float)(int32_t)args[0] : (float)(int32_t)args[0];
        s_tutorial_focus_offset = 0.0f;
        s_tutorial_focus_offset_active =
            s_tutorial_focus_offset_target != 0.0f;
        *result_out = 0u;
        return 1;
    }
    if (command == 142u && argc == 1u && args != NULL) {
        float target = (float)(int32_t)args[0];
        if (target > 180.0f) target -= 360.0f;
        s_tutorial_focus_angle_target = target;
        s_tutorial_focus_angle = 0.0f;
        s_tutorial_focus_angle_velocity = 0.0f;
        s_tutorial_focus_angle_active = target != 0.0f;
        *result_out = 0u;
        return 1;
    }
    if (command == 159u && argc == 1u && args != NULL) {
        tutorial_coordinate_rock_monster_spawn(
            s_tutorial_game_context, args[0]);
        *result_out = 1u;
        return 1;
    }
    if (command == 190u && argc == 2u && args != NULL) {
        LevelState* level = s_tutorial_game_context != NULL
            ? s_tutorial_game_context->level : NULL;
        void* node;
        uint32_t visited = 0u;
        if (level != NULL && args[0] < 0x37u) {
            node = level->pool_heads[args[0]];
            while (node != NULL && visited < NERP_TUTORIAL_CELL_CAPACITY) {
                int32_t x;
                int32_t y;
                memcpy(&x, node, sizeof(x));
                memcpy(&y, (char*)node + 4, sizeof(y));
                cmd_tutorial_congregation_set(x, y,
                                              (int)args[1]);
                node = *(void**)((char*)node + 8);
                ++visited;
            }
        }
        *result_out = 1u;
        return 1;
    }
    if (command == 178u && argc == 1u && args != NULL) {
        *result_out = tutorial_record_object_at_list(
            s_tutorial_game_context != NULL
                ? s_tutorial_game_context->level : NULL,
            args[0]);
        return 1;
    }
    if (command == 199u && argc == 1u && args != NULL) {
        *result_out = tutorial_record_object_amount_at_list(
            s_tutorial_game_context != NULL
                ? s_tutorial_game_context->level : NULL,
            args[0]);
        return 1;
    }
    if (command == 197u && argc == 1u && args != NULL) {
        tutorial_unit_at_coordinate_list_set(s_tutorial_game_context,
                                             args[0]);
        *result_out = 1u;
        return 1;
    }
    if (command == 143u && argc == 1u && args != NULL) {
        *result_out = (uint32_t)tutorial_camera_at_coordinate_list(
            s_tutorial_game_context, s_tutorial_engine_context, args[0]);
        return 1;
    }
    if (command == 144u && argc == 0u) {
        *result_out = tutorial_selected_record_object_get();
        return 1;
    }
    if (command == 254u && argc == 1u && args != NULL) {
        s_nerp_runtime.tutorial_record_pointer = args[0];
        /* The shipped setter leaves the loaded argument in EAX; kind 4
         * consumes one argument and discards the callback result. */
        *result_out = args[0];
        return 1;
    }
    if (command == 115u && argc == 1u && args != NULL) {
        tutorial_camera_goto_coordinate_list(
            s_tutorial_game_context, s_tutorial_engine_context, args[0]);
        *result_out = 1u;
        return 1;
    }
    if (command == 92u && argc == 2u && args != NULL) {
        tutorial_coordinate_marker_set(s_tutorial_game_context,
                                       args[0], args[1]);
        *result_out = 1u;
        return 1;
    }
    if (command == 85u && argc == 2u && args != NULL) {
        s_nerp_runtime.icon_position[0] = (float)(int32_t)args[0];
        s_nerp_runtime.icon_position[1] = (float)(int32_t)args[1];
        *result_out = 1u;
        return 1;
    }
    if (command == 86u && argc == 1u && args != NULL) {
        s_nerp_runtime.icon_space = args[0];
        *result_out = 1u;
        return 1;
    }
    if (command == 87u && argc == 1u && args != NULL) {
        s_nerp_runtime.icon_width = args[0];
        *result_out = 1u;
        return 1;
    }
    if (command == 108u && argc == 3u && args != NULL) {
        s_tutorial_message_timer_values[0] = (float)args[0];
        s_tutorial_message_timer_values[1] = (float)args[1];
        s_tutorial_message_timer_values[2] = (float)args[2];
        *result_out = 1u;
        return 1;
    }
    if (command == 107u) {
        *result_out = (uint32_t)(int32_t)crt_float_to_long(
            s_nerp_runtime.message_timer);
        return 1;
    }
    if (command == 9u) {
        /* ID 9's callback applies the retail DAT_00556ecc bit-27 gate. */
        *result_out = (uint32_t)objective_voice_alias_raise_gate_bit27(
            s_tutorial_game_context != NULL
                ? s_tutorial_game_context->level : NULL);
        return 1;
    }
    if (command == 4u) {
        *result_out = rng_thunk_draw15() & 0x0fffu;
        return 1;
    }
    if (command == 5u || command == 6u) {
        const int divisor = command == 5u ? 10 : 100;
        *result_out = (uint32_t)((int16_t)rng_thunk_draw15() % divisor);
        return 1;
    }
    if (command == 7u) {
        *result_out = rng_thunk_draw15() & 1u;
        return 1;
    }
    if (command == 50u) {
        *result_out = s_tutorial_game_context != NULL
            ? (uint32_t)global_option_low_bits_enabled(
                s_tutorial_game_context->level) : 0u;
        return 1;
    }
    if (command == 200u) {
        if ((int32_t)s_nerp_runtime.message_offset > 1)
            s_nerp_runtime.message_offset = 1u;
        tutorial_advisor_scroll_step_advance();
        *result_out = 0u;
        return 1;
    }
    if (command == 201u && argc == 1u && args != NULL) {
        s_nerp_runtime.camera_control_flags = args[0] << 12;
        *result_out = 0u;
        return 1;
    }
    if (command == 202u || command == 203u) {
        uint32_t flags = command == 202u ? 0x1ff3u : 0x1ffbu;
        flags -= s_nerp_runtime.camera_control_flags;
        *result_out = system_state_flags_set(&flags);
        return 1;
    }
    if (command == 204u || command == 205u) {
        uint32_t flags = command == 204u ? 0x1ffeu : 0x1fffu;
        flags -= s_nerp_runtime.camera_control_flags;
        *result_out = system_state_flags_set(&flags);
        return 1;
    }
    if (command == 206u && argc == 1u && args != NULL) {
        s_nerp_runtime.message_suppress_arrow = args[0];
        engine_global_state_flags_bits6_and_9_set_target(
            s_nerp_runtime.message_offset != 0u || args[0] == 0u, 1);
        *result_out = 0u;
        return 1;
    }
    if (command == 209u) {
        uint32_t count = 0u;
        while (tutorial_objective_token_lookup(count) != NULL) ++count;
        *result_out = s_nerp_runtime.message_ack_count >= count;
        return 1;
    }
    if (command == 166u && argc == 1u && args != NULL) {
        panel_selection_highlight_toggle(s_tutorial_engine_context,
                                         args[0] != 0u);
        *result_out = 0u;
        return 1;
    }
    if (command == 189u) {
        *result_out = (uint32_t)(
            tutorial_entity_type_count_get_at_slot("LavaMonster", 0) +
            tutorial_entity_type_count_get_at_slot("RockMonster", 0) +
            tutorial_entity_type_count_get_at_slot("IceMonster", 0));
        return 1;
    }
    if (command == 96u) {
        *result_out = s_tutorial_game_context != NULL
            ? level_fielda8_get(s_tutorial_game_context->level) : 0u;
        return 1;
    }
    if (command == 97u) {
        *result_out = s_tutorial_game_context != NULL
            ? upgrade_resource_check(s_tutorial_game_context->level, 0) : 0u;
        return 1;
    }
    if (command == 191u && argc == 1u && args != NULL) {
        s_nerp_runtime.objective_switch = objective_switch_state_set(args[0]);
        *result_out = s_nerp_runtime.objective_switch;
        return 1;
    }
    if (command == 192u) {
        s_nerp_runtime.objective_switch = objective_switch_state_get();
        *result_out = s_nerp_runtime.objective_switch != 0u;
        return 1;
    }
    if (command == 261u && argc == 1u && args != NULL) {
        *result_out = tutorial_entity_count_at_coordinate_list(
            s_tutorial_game_context, args[0]);
        return 1;
    }
    if (command == 262u && argc == 1u && args != NULL) {
        if (args[0] == 1u && s_tutorial_game_context != NULL &&
            s_tutorial_game_context->level != NULL) {
            (void)entity_pool_find_by_callback_filtered(
                s_tutorial_game_context,
                tutorial_set_monster_attack_callback,
                s_tutorial_game_context->level);
        }
        *result_out = 0u;
        return 1;
    }
    if (tutorial_grid_count_command_type(command, &count_type)) {
        *result_out = (uint32_t)tutorial_entity_type_count_get(count_type);
        return 1;
    }
    {
        int count_slot;
        if (tutorial_building_level_command(command, &count_type,
                                            &count_slot)) {
            *result_out = count_slot == -2
                ? (uint32_t)tutorial_powered_entity_count_get(
                    s_tutorial_game_context, count_type)
                : (uint32_t)tutorial_entity_type_count_get_at_slot(
                    count_type, count_slot);
            return 1;
        }
    }
    if (tutorial_selected_type_command_name(command, &selected_type)) {
        *result_out = (uint32_t)tutorial_selected_type_get(
            s_tutorial_game_context, selected_type);
        return 1;
    }
    if (tutorial_building_level_command_name(command, &building_type) &&
        argc == 1u && args != NULL) {
        tutorial_building_level_set(s_tutorial_game_context, building_type,
                                     args[0]);
        *result_out = 0u;
        return 1;
    }
    if (tutorial_vehicle_occupant_type_command_name(command, &vehicle_type)) {
        *result_out = (uint32_t)tutorial_vehicle_occupant_count_get(
            s_tutorial_game_context, vehicle_type);
        return 1;
    }
    if (command == 93u) {
        *result_out = system_state_flags_get();
        return 1;
    }
    if (command == 94u && argc == 1u && args != NULL) {
        *result_out = system_state_flags_set(args);
        return 1;
    }
    if (command >= 67u && command <= 70u) {
        *result_out = (uint32_t)(int32_t)crt_float_to_long(
            s_tutorial_nerp_timers[command - 67u]);
        return 1;
    }
    if (command >= 71u && command <= 74u && argc == 1u && args != NULL) {
        s_tutorial_nerp_timers[command - 71u] = (float)(int32_t)args[0];
        *result_out = 1u;
        return 1;
    }
    if (command == 174u && argc == 2u && args != NULL) {
        *result_out = (uint32_t)camera_focus_scroll_scale_commit(
            s_tutorial_engine_context, args);
        return 1;
    }
    if (command == 154u) {
        *result_out = (uint32_t)input_keyboard_down_key_count();
        return 1;
    }
    if (command == 160u) {
        *result_out = engine_debug_word_peek() & 1u;
        return 1;
    }
    if (command == 179u) {
        *result_out = s_nerp_runtime.hidden_objects_found;
        return 1;
    }
    if (command == 180u && argc == 1u && args != NULL) {
        s_nerp_runtime.hidden_objects_found = args[0];
        *result_out = args[0];
        return 1;
    }
    {
        static const struct NerpIconCallback { uint16_t command; uint8_t slot; uint8_t operation; } icons[] = {
            {51,0,0}, {53,1,0}, {54,1,1}, {55,2,1}, {56,3,1},
            {57,3,0}, {58,3,2}, {59,1,2}, {60,2,2}, {61,4,1},
            {62,4,0}, {63,4,2}, {64,5,1}, {65,5,0}, {66,5,2},
            {89,6,0}, {90,6,1}, {91,6,2}, {101,7,2}, {102,7,1},
            {103,7,0}, {104,8,0}, {105,8,1}, {106,8,2}, {116,9,2},
            {117,9,0}, {118,9,1}, {123,10,0}, {124,10,1}, {125,10,2},
            {129,11,0}, {130,11,1}, {131,11,2}, {132,12,0}, {133,12,1},
            {134,12,2}, {148,13,0}, {149,13,1}, {150,13,2}, {151,14,0},
            {152,14,1}, {153,14,2}, {156,15,0}, {157,15,1}, {158,15,2},
            {162,16,1}, {163,16,0}, {164,16,2}, {167,17,2}, {168,17,1},
            {169,17,0}, {175,18,2}, {176,18,1}, {177,18,0}, {182,19,2},
            {183,19,0}, {184,19,1}, {212,20,1}, {213,20,0}, {214,20,2},
            {215,21,1}, {216,21,0}, {217,21,2}, {218,22,1}, {219,22,0},
            {220,22,2}, {221,23,1}, {222,23,0}, {223,23,2}, {245,24,1},
            {246,24,0}, {247,24,2}, {253,25,2}, {255,26,0},
            {256,26,1}, {257,26,2}, {258,27,0}, {259,27,1}, {260,27,2},
            {263,28,0}, {264,28,1}, {265,28,2}
        };
        uint32_t i;
        for (i = 0u; i < sizeof(icons) / sizeof(icons[0]); ++i) {
            if (icons[i].command != command) continue;
            if (icons[i].operation == 0u) {
                *result_out = s_nerp_runtime.icon_clicked[icons[i].slot];
            } else if (icons[i].operation == 1u && argc == 1u && args != NULL) {
                uint32_t old = s_nerp_runtime.icon_clicked[icons[i].slot];
                s_nerp_runtime.icon_clicked[icons[i].slot] = args[0];
                *result_out = old;
            } else if (icons[i].operation == 2u && argc == 1u && args != NULL) {
                s_nerp_runtime.icon_flashing[icons[i].slot] = args[0] == 1u;
                *result_out = args[0] == 1u;
            } else {
                *result_out = 0u;
            }
            return 1;
        }
    }
    if (command >= 17u && command <= 24u) {
        *result_out = s_tutorial_nerp_registers[command - 17u];
        return 1;
    }
    if (command >= 25u && command <= 48u && argc == 1u && args != NULL) {
        reg = (command - 25u) & 7u;
        if (command < 33u) s_tutorial_nerp_registers[reg] = args[0];
        else if (command < 41u) s_tutorial_nerp_registers[reg] += args[0];
        else s_tutorial_nerp_registers[reg] -= args[0];
        *result_out = s_tutorial_nerp_registers[reg];
        return 1;
    }
    /* ID 11 is named SetGameFail in the PE table, but its callback target
     * has no checked-in body or matching port API; leave it unsupported. */
    return 0;
}

/* Install the ported callback subset through the same call slot as host
 * command providers. The subset recognizes commands by the exact retail ID
 * table; unrepresented IDs return zero without claiming a guessed effect. */
static uint32_t tutorial_builtin_command_adapter(uint32_t command,
                                                const uint32_t* args,
                                                uint32_t arg_count,
                                                void* context)
{
    uint32_t result = 0u;
    (void)context;
    (void)tutorial_builtin_command(command, args, arg_count, &result);
    return result;
}

// FUNCTION: LEGORR 0x00453320
void tutorial_script_command_dispatch(float delta)
{
    uint32_t pc = 0u;
    uint32_t previous = 0;
    uint32_t last_result = 0u;
    uint32_t compare_op = 0;
    int negate_next = 0;
    if (s_tutorial_script_data == NULL && s_tutorial_program_count == 0u)
        return;
    while (pc < s_tutorial_program_count) {
        uint32_t word = s_tutorial_program_words[pc];
        uint32_t command = word & 0xffffu;
        uint32_t kind = 0u;
        uint32_t argc = 0u;
        uint32_t args[3];
        uint32_t result = 0u;
        uint32_t i;
        uint32_t step = 1u;
        if ((word & 0x20000u) == 0u) {
            if ((word & 0x10000u) != 0u) {
                negate_next = 0;
                if (command == 0u) {
                    /* Preserve the accumulated predicate and select AND. */
                    compare_op = 1u;
                } else if (command == 1u) {
                    negate_next = 1;
                } else if (command == 2u) {
                    compare_op = 2u;
                } else if (command >= 5u && command <= 10u) {
                    compare_op = command - 2u;
                }
            } else if ((word & 0x40000u) != 0u) {
                previous = 0u;
                compare_op = 0u;
                negate_next = 0;
            } else if ((word & 0x80000u) != 0u) {
                if (previous != 0u) {
                    pc = command < s_tutorial_program_count
                        ? command : s_tutorial_program_count;
                }
                previous = 0u;
                compare_op = 0u;
                negate_next = 0;
            } else {
                uint32_t current = (uint32_t)(int32_t)(int16_t)command;
                if (negate_next) current = current == 0u;
                last_result = current;
                previous = compare_op != 0u
                    ? tutorial_compare_values(compare_op, previous, current)
                    : current;
                negate_next = 0;
                compare_op = 1u;
            }
            ++pc;
            continue;
        }
        if (command == 0u && (previous != 0u || compare_op == 0u)) break;
        kind = s_tutorial_script_ops.command_kind != NULL
            ? s_tutorial_script_ops.command_kind(command,
                                                  s_tutorial_script_ops.context)
            : tutorial_command_kind_default(command);
        if (kind <= 2u) {
            if (kind == 1u) argc = 1u;
            else if (kind == 2u) argc = 2u;
            for (i = 0u; i < argc; ++i) {
                args[i] = pc + 1u + i < s_tutorial_program_count
                    ? s_tutorial_program_words[pc + 1u + i] : 0u;
                tutorial_indirect_handle_resolve(&args[i]);
            }
            step += argc;
            (void)tutorial_command_invoke(command,
                argc == 0u ? NULL : args, argc, &result);
            if (negate_next) result = result == 0u;
            last_result = result;
            previous = compare_op != 0u
                ? tutorial_compare_values(compare_op, previous, result)
                : result;
            negate_next = 0;
            compare_op = 1u;
        } else if (kind >= 3u && kind <= 6u) {
            argc = kind - 3u;
            for (i = 0u; i < argc; ++i) {
                args[i] = pc + 1u + i < s_tutorial_program_count
                    ? s_tutorial_program_words[pc + 1u + i] : 0u;
                if (previous != 0u)
                    tutorial_indirect_handle_resolve(&args[i]);
            }
            step += argc;
            if (previous != 0u) {
                (void)tutorial_command_invoke(command,
                    argc == 0u ? NULL : args, argc, &result);
            }
            previous = 0u;
            compare_op = 0u;
            negate_next = 0;
        } else {
            ++pc;
            continue;
        }
        pc += step;
    }
    /* This dispatcher receives simulation seconds. Only the camera's
     * reference-shaped tween consumes the original 25-Hz frame units. */
    tutorial_camera_focus_tween_update(view_camera_delta_from_seconds(delta));
}

// FUNCTION: LEGORR 0x00453920
void tutorial_advisor_scroll_step_advance(void)
{
    static const uint32_t scroll_scale_record[2] = { 100u, 0u };
    tutorial_message_generation_sync();
    media_action_feedback_sound_dispatch(0);
    if (s_nerp_runtime.message_offset != 0u) {
        --s_nerp_runtime.message_offset;
        if ((int32_t)s_nerp_runtime.message_offset > 3)
            s_nerp_runtime.message_offset = 3u;
        if ((int32_t)s_nerp_runtime.message_offset >
            (int32_t)s_nerp_runtime.message_maximum)
            s_nerp_runtime.message_offset = s_nerp_runtime.message_maximum;
        {
            uint32_t index = s_nerp_runtime.message_maximum -
                             s_nerp_runtime.message_offset;
            const char* token = tutorial_objective_token_lookup(
                tutorial_message_history_get(index));
            advisor_text_request_setup_text(token, 0u);
        }
        if (s_nerp_runtime.message_offset == 0u) {
            engine_global_state_flags_bits6_and_9_set_target(0, 1);
            if (system_state_flags_get() != 0u)
                (void)camera_focus_scroll_scale_commit(
                    s_tutorial_engine_context, scroll_scale_record);
        }
        engine_global_state_flags_bit7_set_target(1);
    } else {
        if (s_nerp_runtime.message_pending_ack != 0u)
            ++s_nerp_runtime.message_ack_count;
        s_nerp_runtime.message_pending_ack = 0u;
    }
}

void tutorial_camera_focus_state_set(uint32_t focus_id, uint32_t valid,
                                     float offset_target, float angle_target,
                                     uint32_t offset_active,
                                     uint32_t angle_active)
{
    s_tutorial_focus_id = focus_id;
    s_tutorial_focus_valid = valid;
    s_tutorial_focus_offset_target = offset_target;
    s_tutorial_focus_angle_target = angle_target;
    s_tutorial_focus_offset_active = offset_active;
    s_tutorial_focus_angle_active = angle_active;
    s_tutorial_focus_offset = 0.0f;
    s_tutorial_focus_angle = 0.0f;
    s_tutorial_focus_angle_velocity = 0.0f;
}

// FUNCTION: LEGORR 0x00453da0
void tutorial_camera_focus_tween_update(float delta)
{
    float scale = 1.0f;
    float step;
    if (delta <= 0.0f) return;
    if (s_tutorial_script_ops.delta_scale != NULL)
        scale = s_tutorial_script_ops.delta_scale(
            s_tutorial_script_ops.context);
    if (s_tutorial_focus_valid != 0) {
        if (!tutorial_camera_focus_target_apply(s_tutorial_game_context,
                                                s_tutorial_engine_context,
                                                s_tutorial_focus_id)) {
            s_tutorial_focus_valid = 0u;
        }
        if (s_tutorial_script_ops.camera_focus != NULL)
            s_tutorial_script_ops.camera_focus(
                s_tutorial_focus_id, 0.0f, 0.0f,
                s_tutorial_script_ops.context);
    }
    if (s_tutorial_focus_offset_active != 0) {
        step = s_tutorial_focus_offset_target / (delta * 25.0f);
        s_tutorial_focus_offset += step;
        if (s_tutorial_focus_offset_target - 5.0f < s_tutorial_focus_offset &&
            s_tutorial_focus_offset < s_tutorial_focus_offset_target + 5.0f) {
            s_tutorial_focus_offset_active = 0;
            s_tutorial_focus_offset = 0.0f;
        }
        if (s_tutorial_script_ops.camera_offset != NULL)
            s_tutorial_script_ops.camera_offset(
                step, s_tutorial_script_ops.context);
        else if (s_tutorial_engine_context != NULL)
            camera_zoom_update(
                s_tutorial_engine_context->camera_controller, step);
    }
    if (s_tutorial_focus_angle_active != 0) {
        float remaining = s_tutorial_focus_angle <= 0.0f
                        ? s_tutorial_focus_angle + s_tutorial_focus_angle_target
                        : s_tutorial_focus_angle - s_tutorial_focus_angle_target;
        float scaled = remaining * 0.05555556f * 0.33333334f * scale;
        s_tutorial_focus_angle_velocity += scaled;
        if ((s_tutorial_focus_angle_target > 0.0f &&
             s_tutorial_focus_angle_target - 1.0f <
                 s_tutorial_focus_angle_velocity) ||
            (s_tutorial_focus_angle_target < 0.0f &&
             s_tutorial_focus_angle_velocity <
                 s_tutorial_focus_angle_target + 1.0f)) {
            s_tutorial_focus_angle_active = 0;
            s_tutorial_focus_angle_velocity = 0.0f;
        }
        s_tutorial_focus_angle += scaled;
        if (s_tutorial_script_ops.camera_rotation != NULL)
            s_tutorial_script_ops.camera_rotation(
                scaled * 0.017453292f, s_tutorial_script_ops.context);
        else if (s_tutorial_engine_context != NULL) {
            float current = 0.0f;
            float next;
            memcpy(&current,
                   s_tutorial_engine_context->camera_controller + 0x84,
                   sizeof(current));
            next = current + scaled * 0.017453292f;
            camera_rotation_set(
                s_tutorial_engine_context->camera_controller, next);
        }
    }
    if (s_tutorial_script_ops.camera_state != NULL &&
        s_tutorial_script_ops.camera_state(s_tutorial_script_ops.context) != 0 &&
        s_tutorial_script_ops.camera_scroll != NULL)
        s_tutorial_script_ops.camera_scroll(100, 0.0f,
                                            s_tutorial_script_ops.context);
}

static void tutorial_inline_append(char* out, uint32_t* used, const char* text)
{
    uint32_t i;
    if (out == NULL || used == NULL || text == NULL) return;
    for (i = 0; text[i] != '\0' && *used < 511; ++i)
        out[(*used)++] = text[i];
    out[*used] = '\0';
}

void tutorial_inline_token_render(char* source, char* output, int draw)
{
    uint32_t used = 0;
    int text_x = 0;
    uint32_t i = 0;
    uint32_t n;
    int has_named_tokens = 0;
    if (source == NULL) return;
    if (output != NULL) output[0] = '\0';
    for (n = 0; n < s_tutorial_token_count; ++n) {
        if (s_tutorial_tokens[n].kind == 1 ||
            s_tutorial_tokens[n].kind == 2) {
            has_named_tokens = 1;
            break;
        }
    }
    if (!has_named_tokens && output != NULL) {
        /* With no parsed named tokens, the corpus copies the source as-is;
         * markup is only interpreted once one of its token tables exists. */
        tutorial_inline_append(output, &used, source);
        return;
    }
    while (source[i] != '\0') {
        char marker = source[i];
        if ((marker == '<' || marker == '#') && source[i + 1] != '\0') {
            uint32_t start = i + 1;
            uint32_t end = start;
            int kind = marker == '<' ? 1 : 2;
            char name[256];
            uint32_t len;
            uint32_t n;
            while (source[end] != '\0' && source[end] != (marker == '<' ? '>' : '#') &&
                   end - start < 255) ++end;
            if (source[end] == (marker == '<' ? '>' : '#')) {
                len = end - start;
                memcpy(name, source + start, len);
                name[len] = '\0';
                {
                    int matched = 0;
                    for (n = 0; n < s_tutorial_token_count; ++n) {
                        if (s_tutorial_tokens[n].kind == kind &&
                            tutorial_text_equal(s_tutorial_tokens[n].name,
                                                name)) {
                            if (kind == 1) {
                                tutorial_inline_append(
                                    output, &used, s_tutorial_tokens[n].name);
                                if (draw && s_tutorial_script_ops.text_draw != NULL)
                                    s_tutorial_script_ops.text_draw(
                                        s_tutorial_tokens[n].name, text_x, 0,
                                        s_tutorial_script_ops.context);
                                text_x += (int)strlen(
                                    s_tutorial_tokens[n].name);
                            } else if (s_tutorial_script_ops.command != NULL) {
                                s_tutorial_script_ops.command(
                                    s_tutorial_tokens[n].value, NULL, 0,
                                    s_tutorial_script_ops.context);
                            }
                            matched = 1;
                            break;
                        }
                    }
                }
                i = end + 1;
                continue;
            }
        }
        if (output != NULL && used < 511) {
            output[used++] = source[i];
            output[used] = '\0';
        }
        if (draw && s_tutorial_script_ops.text_draw != NULL) {
            char one[2]; one[0] = source[i]; one[1] = '\0';
            s_tutorial_script_ops.text_draw(one, text_x, 0,
                                            s_tutorial_script_ops.context);
        }
        ++text_x;
        ++i;
    }
}

uint32_t tutorial_script_word_count(void) { return s_tutorial_program_count; }
uint32_t tutorial_token_count(void) { return s_tutorial_token_count; }
uint32_t tutorial_literal_token_count(void)
{
    tutorial_objective_token_output_snapshot(NULL);
    return s_tutorial_literal_token_count;
}
const char* tutorial_token_name_get(uint32_t index)
{
    return index < s_tutorial_token_count ? s_tutorial_tokens[index].name : NULL;
}
uint32_t tutorial_token_value_get(uint32_t index)
{
    return index < s_tutorial_token_count ? s_tutorial_tokens[index].value : 0;
}

typedef struct LegacyPanelPortRecord {
    float initial[4];
    float current[4];
    uint32_t flags;
    uint32_t image_flags;
    void* image;
    int image_mode;
    int image_texture_owned;
    uint32_t selection_flags[64];
    /* Mirrors the original per-entry value word (record entry +0x24),
       cleared by the interaction-state reset alongside selection_flags. */
    uint32_t selection_values[64];
    uint32_t selection_count;
} LegacyPanelPortRecord;

typedef struct LegacyPanelImagePortEntry {
    void* surface;
    int x;
    int y;
    int width;
    int height;
    int panel;
    uint32_t flags;
} LegacyPanelImagePortEntry;

static LegacyHudOps s_legacy_hud_ops;
static LegacyHealthBarState s_legacy_health_bar;
static LegacyPanelPortRecord s_legacy_panels[18];
static void* s_legacy_hunger_images[5];
static float s_legacy_hunger_position[2];
static float s_legacy_bubble_position[2];
static float s_legacy_entity_ring_radius;
static float s_legacy_entity_ring_height;

void legacy_hud_ops_set(const LegacyHudOps* ops)
{
    if (ops != NULL) s_legacy_hud_ops = *ops;
    else memset(&s_legacy_hud_ops, 0, sizeof(s_legacy_hud_ops));
}

static int legacy_config_pair(BlockFile* cfg, const char* path, int* x, int* y)
{
    char text[128];
    char* separator;
    if (cfg == NULL || path == NULL || x == NULL || y == NULL) return 0;
    if (s_legacy_hud_ops.config_pair != NULL)
        return s_legacy_hud_ops.config_pair(cfg, path, x, y,
                                            s_legacy_hud_ops.context);
    if (!block_get_str(cfg, path, text, sizeof(text))) return 0;
    /* FUN_00477440 splits these config values on the shared ':' delimiter
     * and HealthBarConfigLoadAndShadeDerive accepts exactly two fields. */
    separator = strchr(text, ':');
    if (separator == NULL || strchr(separator + 1, ':') != NULL) return 0;
    *separator = '\0';
    *x = atoi(text);
    *y = atoi(separator + 1);
    return 1;
}

static int legacy_config_rgb(BlockFile* cfg, const char* path, float rgb[3])
{
    if (cfg == NULL || path == NULL || rgb == NULL) return 0;
    if (s_legacy_hud_ops.config_rgb != NULL)
        return s_legacy_hud_ops.config_rgb(cfg, path, rgb,
                                           s_legacy_hud_ops.context);
    return config_rgb_vector_read(cfg, path, &rgb[0], &rgb[1], &rgb[2]);
}

/* The retail loaders resolve each setting through a key-name file first,
 * then fetch and parse the corresponding value from the value file. Most
 * production callers pass the same Lego.cfg for both, but keep the two
 * inputs distinct for callers which supply a separate key table. */
static const char* legacy_hud_config_path(const char* config_root,
                                          const char* key)
{
    return string_path_join(config_root, "ObjInfo", key, (char*)NULL);
}

static int legacy_config_pair_from_root(BlockFile* values,
                                        const char* config_root,
                                        const char* key, int* x, int* y)
{
    const char* path;
    if (values == NULL || config_root == NULL || key == NULL) return 0;
    path = legacy_hud_config_path(config_root, key);
    return legacy_config_pair(values, path, x, y);
}

static int legacy_config_rgb_from_root(BlockFile* values,
                                       const char* config_root,
                                       const char* key, float rgb[3])
{
    const char* path;
    if (values == NULL || config_root == NULL || key == NULL) return 0;
    path = legacy_hud_config_path(config_root, key);
    return legacy_config_rgb(values, path, rgb);
}

static int legacy_config_int_from_root(BlockFile* values,
                                       const char* config_root,
                                       const char* key, int fallback)
{
    const char* path;
    if (values == NULL || config_root == NULL || key == NULL) return fallback;
    path = legacy_hud_config_path(config_root, key);
    return block_get_int(values, path, fallback);
}

static int legacy_config_bool_from_root(BlockFile* values,
                                        const char* config_root,
                                        const char* key)
{
    const char* path;
    if (values == NULL || config_root == NULL || key == NULL) return 0;
    path = legacy_hud_config_path(config_root, key);
    return block_get_bool_path(values, path);
}

static float legacy_clamp_01(float value)
{
    if (value < 0.0f) return 0.0f;
    if (value > 1.0f) return 1.0f;
    return value;
}

static void legacy_color_pair(float source[3], float high[3], float low[3])
{
    int i;
    for (i = 0; i < 3; ++i) {
        /* The retail loop walks the interleaved globals in three strides,
         * clamping the source, lighter, and darker RGB channels in place. */
        high[i] = legacy_clamp_01(source[i] + source[i] * 0.4f);
        low[i] = legacy_clamp_01(source[i] - source[i] * 0.4f);
        source[i] = legacy_clamp_01(source[i]);
    }
}

// FUNCTION: LEGORR 0x00459560
int legacy_health_bar_config_load(BlockFile* values, const char* config_root)
{
    int x, y;
    int width, height;
    BlockFile* cfg = values;
    int border;
    int vertical;
    float rgb[3];
    if (cfg == NULL) return 0;
    if (!legacy_config_pair_from_root(values, config_root,
            "HealthBarPosition", &x, &y)) return 0;
    s_legacy_health_bar.x = (float)x;
    s_legacy_health_bar.y = (float)y;
    if (!legacy_config_pair_from_root(values, config_root,
            "HealthBarWidthHeight", &width, &height)) return 0;
    s_legacy_health_bar.width = (float)width;
    s_legacy_health_bar.height = (float)height;
    // The original derives this through an integer conversion of the raw
    // value string, so a missing or empty key yields 0 rather than 1.
    border = legacy_config_int_from_root(values, config_root,
            "HealthBarBorderSize", 0);
    s_legacy_health_bar.border = (float)border;
    if (!legacy_config_rgb_from_root(values, config_root,
            "HealthBarBorderRGB", rgb)) return 0;
    memcpy(s_legacy_health_bar.border_rgb, rgb, sizeof(rgb));
    legacy_color_pair(s_legacy_health_bar.border_rgb,
                      s_legacy_health_bar.border_high,
                      s_legacy_health_bar.border_low);
    if (!legacy_config_rgb_from_root(values, config_root,
            "HealthBarBackgroundRGB", rgb)) return 0;
    memcpy(s_legacy_health_bar.background_rgb, rgb, sizeof(rgb));
    if (!legacy_config_rgb_from_root(values, config_root,
            "HealthBarRGB", rgb)) return 0;
    memcpy(s_legacy_health_bar.health_rgb, rgb, sizeof(rgb));
    vertical = legacy_config_bool_from_root(values, config_root,
            "HealthBarVertical");
    /* DAT_004ffe68 is shared by the other HUD loaders. The original only
       ORs these bits: a later horizontal reload does not clear vertical. */
    if (vertical == 1) s_panel_enable_word |= 8u;
    s_panel_enable_word |= 2u;
    s_legacy_health_bar.vertical =
        (s_panel_enable_word & 8u) != 0 ? 1u : 0u;
    s_legacy_health_bar.flags |= 2u;
    if ((s_panel_enable_word & 8u) != 0)
        s_legacy_health_bar.flags |= 8u;
    return 1;
}

static void legacy_hud_resource_name(BlockFile* cfg, const char* config_root,
                                     const char* key,
                                     char* out, uint32_t out_size)
{
    const char* path;
    out[0] = '\0';
    if (cfg == NULL || config_root == NULL || key == NULL || out_size == 0)
        return;
    path = legacy_hud_config_path(config_root, key);
    if (s_legacy_hud_ops.config_resource != NULL) {
        s_legacy_hud_ops.config_resource(cfg, path, out, out_size,
                                        s_legacy_hud_ops.context);
    } else {
        block_get_str(cfg, path, out, (size_t)out_size);
    }
}

// FUNCTION: LEGORR 0x004599c0
void legacy_hunger_images_load(BlockFile* values, const char* config_root)
{
    int i;
    char path[96];
    char resource_name[256];
    BlockFile* cfg = values;
    // The original re-reads the five ObjInfo::HungerImages::HungerImage%i
    // keys and overwrites each slot without releasing the previous surface
    // (the resource layer owns surface lifetime).
    for (i = 0; i < 5; ++i) {
        void* image = NULL;
        sprintf(path, "HungerImages::HungerImage%i", i);
        legacy_hud_resource_name(cfg, config_root, path, resource_name,
                                 (uint32_t)sizeof(resource_name));
        if (resource_name[0] != '\0' &&
            s_legacy_hud_ops.surface_load != NULL) {
            image = s_legacy_hud_ops.surface_load(
                cfg, resource_name, 0, 0, s_legacy_hud_ops.context);
            if (image != NULL && s_legacy_hud_ops.surface_initialize != NULL)
                s_legacy_hud_ops.surface_initialize(
                    image, s_legacy_hud_ops.context);
        }
        s_legacy_hunger_images[i] = image;
        /* These are the five DAT_004ffec4 slots consumed by
         * IndexedPanelResourcePositionApply. Keep the config-loaded images
         * in the same table used by the live panel drawing path. */
        panel_position_slot_set(i, image);
    }
}

// FUNCTION: LEGORR 0x00459900
int legacy_hunger_images_position_config_load(BlockFile* values,
                                               const char* config_root)
{
    int x, y;
    BlockFile* cfg = values;
    legacy_hunger_images_load(values, config_root);
    if (cfg == NULL || !legacy_config_pair_from_root(values, config_root,
            "HungerImagesPosition", &x, &y)) return 0;
    s_legacy_hunger_position[0] = (float)x;
    s_legacy_hunger_position[1] = (float)y;
    panel_position_offsets_set((float)x, (float)y);
    s_panel_enable_word |= 1u;
    return 1;
}

int legacy_bubble_images_position_config_load(BlockFile* values,
                                              BlockFile* names)
{
    int x, y;
    BlockFile* value_cfg = values != NULL ? values : names;
    BlockFile* name_cfg = names != NULL ? names : values;
    const char* path = "ObjInfo::BubbleImagesPosition";
    /* RenderThreePassUpdate receives the value and key-name BlockFiles
     * separately. The retail config path first resolves this key in the
     * names file, then parses its value from the value file. */
    if (value_cfg == NULL || name_cfg == NULL ||
        block_find(name_cfg, path) == NULL ||
        !legacy_config_pair(value_cfg, path, &x, &y)) return 0;
    s_legacy_bubble_position[0] = (float)x;
    s_legacy_bubble_position[1] = (float)y;
    s_panel_enable_word |= 0x10u;
    bubble_offsets_set(s_legacy_bubble_position[0],
                       s_legacy_bubble_position[1]);
    return 1;
}

void legacy_health_bar_config_peek(LegacyHealthBarState* state)
{
    if (state != NULL) *state = s_legacy_health_bar;
}

static uint32_t legacy_rgb_pack(const float rgb[3])
{
    uint32_t r = (uint32_t)(legacy_clamp_01(rgb[0]) * 255.0f);
    uint32_t g = (uint32_t)(legacy_clamp_01(rgb[1]) * 255.0f);
    uint32_t b = (uint32_t)(legacy_clamp_01(rgb[2]) * 255.0f);
    return (r << 16) | (g << 8) | b;
}

// FUNCTION: LEGORR 0x00459b00
void legacy_entity_screen_ring_render(void* entity, int x, int y)
{
    EntitySlot* slot = (EntitySlot*)entity;
    float first_xy[20];
    float second_xy[20];
    float rect[4];
    float health;
    float width, height, border;
    float health_size;
    float x0, y0;
    int i;
    int point_count;
    if (slot == NULL || (s_panel_enable_word & 2u) == 0u) return;
    if (entity_quad_mesh_render_record_allowed(slot) == 0) return;
    health = slot->field_340;
    if (health < 0.0f) health = 0.0f;
    if (health > 100.0f) health = 100.0f;
    border = s_legacy_health_bar.border;
    width = s_legacy_health_bar.width + border * 2.0f;
    height = s_legacy_health_bar.height + border * 2.0f;
    x0 = (float)x + s_legacy_health_bar.x;
    y0 = (float)y + s_legacy_health_bar.y;

    /* Match the two interleaved 2*border vertex streams built by retail.
     * Each pair walks one screen edge while stepping one pixel inward. */
    point_count = (int)border * 2;
    if (point_count > 10) point_count = 10;
    if (point_count < 0) point_count = 0;
    memset(first_xy, 0, sizeof(first_xy));
    memset(second_xy, 0, sizeof(second_xy));
    for (i = 0; i < point_count / 2; ++i) {
        float fi = (float)i;
        int at = i * 4;
        first_xy[at] = x0 + fi;
        first_xy[at + 1] = y0 + fi;
        first_xy[at + 2] = x0 + width - 1.0f - fi;
        first_xy[at + 3] = y0 + fi;
        second_xy[at] = x0 + fi;
        second_xy[at + 1] = y0 + height - 2.0f - fi;
        second_xy[at + 2] = x0 + fi + 1.0f;
        second_xy[at + 3] = y0 + fi;
    }
    {
        float opposite_a[20];
        float opposite_b[20];
        memset(opposite_a, 0, sizeof(opposite_a));
        memset(opposite_b, 0, sizeof(opposite_b));
        for (i = 0; i < point_count / 2; ++i) {
            float fi = (float)i;
            int at = i * 4;
            opposite_a[at] = x0 + fi;
            opposite_a[at + 1] = y0 + height - 1.0f - fi;
            opposite_a[at + 2] = x0 + width - 1.0f - fi;
            opposite_a[at + 3] = y0 + height - 1.0f - fi;
            opposite_b[at] = x0 + width - 2.0f - fi;
            opposite_b[at + 1] = y0 + height - 1.0f - fi;
            opposite_b[at + 2] = x0 + width - 1.0f - fi;
            opposite_b[at + 3] = y0 + fi + 1.0f;
        }
        if (s_legacy_hud_ops.ring_strip_draw != NULL && point_count > 0) {
            s_legacy_hud_ops.ring_strip_draw(first_xy, second_xy,
                (uint32_t)point_count, s_legacy_health_bar.border_high, 0,
                s_legacy_hud_ops.context);
            s_legacy_hud_ops.ring_strip_draw(opposite_a, opposite_b,
                (uint32_t)point_count, s_legacy_health_bar.border_low, 0,
                s_legacy_hud_ops.context);
        }
    }

    if (s_legacy_hud_ops.rect_draw != NULL) {
        if (s_legacy_health_bar.vertical == 0u) {
            health_size = health * width * 0.01f;
            rect[0] = x0;
            rect[1] = y0;
            rect[2] = health_size;
            rect[3] = height;
            s_legacy_hud_ops.rect_draw(rect,
                legacy_rgb_pack(s_legacy_health_bar.health_rgb),
                s_legacy_hud_ops.context);
            rect[0] = x0 + health_size;
            rect[2] = width - health_size;
            s_legacy_hud_ops.rect_draw(rect,
                legacy_rgb_pack(s_legacy_health_bar.background_rgb),
                s_legacy_hud_ops.context);
        }
        else {
            health_size = (100.0f - health) * height * 0.01f;
            rect[0] = x0;
            rect[1] = y0;
            rect[2] = width;
            rect[3] = health_size;
            s_legacy_hud_ops.rect_draw(rect,
                legacy_rgb_pack(s_legacy_health_bar.background_rgb),
                s_legacy_hud_ops.context);
            rect[1] = y0 + health_size;
            rect[3] = height - health_size;
            s_legacy_hud_ops.rect_draw(rect,
                legacy_rgb_pack(s_legacy_health_bar.health_rgb),
                s_legacy_hud_ops.context);
        }
    }
}

void legacy_entity_screen_ring_render_with_ops(
    void* entity, int x, int y, const LegacyHudOps* ops)
{
    LegacyHudOps saved_ops = s_legacy_hud_ops;
    if (ops != NULL) s_legacy_hud_ops = *ops;
    else memset(&s_legacy_hud_ops, 0, sizeof(s_legacy_hud_ops));
    legacy_entity_screen_ring_render(entity, x, y);
    s_legacy_hud_ops = saved_ops;
}

// FUNCTION: LEGORR 0x0045a240
void legacy_scroll_panel_controls_initialize(void)
{
    /* The original initialises the scroll info panel, creates the
       configured overlay element, then calls FUN_0045abb0(6, 1, 1) and
       FUN_0045abb0(6, 2, 1) — the panel-item request-state setter. */
    ui_scroll_info_panel_initialize(s_legacy_hud_ops.overlay_init_sink);
    ui_overlay_create_configured_element(s_legacy_hud_ops.overlay_init_sink);
    panel_item_flag32_set(6, 1, 1);
    panel_item_flag32_set(6, 2, 1);
}

// FUNCTION: LEGORR 0x0045a270
void legacy_panel_category_interaction_state_reset(void)
{
    int i;
    /* The original walks the twelve records between DAT_00500110 and
       0x500350 (stride 0x30), restores the two view-offset dwords from
       their initial pair, forces the record flags to (f & ~0xa) | 4, and
       clears each selection entry's value word and its 0x48 flag bits.
       It does not touch the HUD state-flag word. */
    for (i = 0; i < 12; ++i) {
        uint32_t j;
        LegacyPanelPortRecord* record = &s_legacy_panels[i];
        record->current[0] = record->initial[0];
        record->current[1] = record->initial[1];
        record->flags = (record->flags & ~0xau) | 4u;
        for (j = 0; j < record->selection_count && j < 64; ++j) {
            record->selection_values[j] = 0;
            record->selection_flags[j] &= ~0x48u;
        }
    }
    overlay_anim_float_install(0.0f);
}

// FUNCTION: LEGORR 0x0045a2e0
void legacy_overlay_image_record_load(const char* path, int panel,
                                      uint32_t state)
{
    LegacyPanelPortRecord* record;
    if (panel < 0 || panel >= 18) return;
    record = &s_legacy_panels[panel];
    if (record->image != NULL) {
        if (record->image_texture_owned) {
            texture_release((TextureSurface*)record->image);
        }
        else if (s_legacy_hud_ops.surface_release != NULL) {
            s_legacy_hud_ops.surface_release(record->image,
                                             s_legacy_hud_ops.context);
        }
    }
    record->selection_count = 0;
    record->flags = state;
    record->image_mode = 0;
    record->image_flags = 0;
    record->image = NULL;
    record->image_texture_owned = 0;
    if (path != NULL) {
        if (s_legacy_hud_ops.surface_load != NULL) {
            record->image = s_legacy_hud_ops.surface_load(
                NULL, path, 0, 0, s_legacy_hud_ops.context);
        }
        else {
            record->image = texture_get_or_load(NULL, path);
            record->image_texture_owned = record->image != NULL;
        }
    }
    if (record->image != NULL) {
        if (s_legacy_hud_ops.surface_initialize != NULL)
            s_legacy_hud_ops.surface_initialize(record->image,
                                                s_legacy_hud_ops.context);
        record->image_mode = 0;
        record->image_flags |= 1u;
        return;
    }
    if (path != NULL && s_legacy_hud_ops.surface_alternate_load != NULL) {
        record->image = s_legacy_hud_ops.surface_alternate_load(
            NULL, path, panel, 3, s_legacy_hud_ops.context);
        if (record->image != NULL) {
            record->image_texture_owned = 0;
            record->image_mode = 1;
            record->image_flags |= 1u;
        }
    }
}

// FUNCTION: LEGORR 0x0045a3b0
int legacy_panel_slot_coordinate_hit_test(int panel, int x, int y)
{
    LegacyPanelPortRecord* record;
    int hit;
    /* Preserve the port record-array bound; the corpus directly indexes its
       panel record and has no corresponding range branch. */
    if (panel < 0 || panel >= 18) return 1;
    record = &s_legacy_panels[panel];
    if (record->image_mode != 0 || record->image == NULL ||
        s_legacy_hud_ops.surface_hit == NULL) return 1;
    hit = s_legacy_hud_ops.surface_hit(record->image,
        x - (int)record->current[0], y - (int)record->current[1],
        s_legacy_hud_ops.context);
    return hit ? panel == 0 : 1;
}

/* Portable panel-image entry. This is separate from PanelImageSlotEntryAllocate
 * (0x0045a500), whose 16-byte wrapper owns a TextBuffer record. */
void* legacy_panel_image_slot_entry_allocate(int panel, const char* path,
                                             const int rect[4],
                                             uint32_t flags)
{
    LegacyPanelImagePortEntry* entry;
    BlockFile* cfg = NULL;
    if (panel < 0 || panel >= 18 || rect == NULL) return NULL;
    entry = (LegacyPanelImagePortEntry*)engine_heap_alloc(
        (long)sizeof(LegacyPanelImagePortEntry));
    if (entry == NULL) return NULL;
    memset(entry, 0, sizeof(*entry));
    entry->x = rect[0];
    entry->y = rect[1];
    entry->width = rect[2];
    entry->height = rect[3];
    entry->panel = panel;
    entry->flags = flags;
    if (path != NULL && s_legacy_hud_ops.surface_load != NULL)
        entry->surface = s_legacy_hud_ops.surface_load(
            cfg, path, rect[2], rect[3], s_legacy_hud_ops.context);
    return entry;
}

void legacy_scrolling_advisor_text_append(void* record, const char* format, ...)
{
    va_list args;
    if (record == NULL || format == NULL) return;
    va_start(args, format);
    legacy_scrolling_advisor_text_append_v(record, format, args);
    va_end(args);
}

void legacy_scrolling_advisor_text_append_v(void* record, const char* format,
                                            va_list args)
{
    if (record == NULL || format == NULL) return;
    text_buffer_append_vformat(record, 0, format, args);
}

void legacy_ui_text_output_position_and_draw(void* record, const char* text,
                                             const char* fallback)
{
    LegacyPanelImagePortEntry* entry = (LegacyPanelImagePortEntry*)record;
    const char* draw_text = text != NULL ? text : fallback;
    int x, y;
    if (entry == NULL || draw_text == NULL) return;
    x = entry->x;
    y = entry->y;
    if (entry->panel >= 0 && entry->panel < 18) {
        x += (int)s_legacy_panels[entry->panel].current[0];
        y += (int)s_legacy_panels[entry->panel].current[1];
    }
    if (s_legacy_hud_ops.text_measure != NULL)
        s_legacy_hud_ops.text_measure(draw_text, s_legacy_hud_ops.context);
    if (s_legacy_hud_ops.text_draw != NULL)
        s_legacy_hud_ops.text_draw(draw_text, x, y,
                                   s_legacy_hud_ops.context);
}

uint32_t legacy_panel_category_flags_get(int panel)
{
    return panel >= 0 && panel < 18 ? s_legacy_panels[panel].flags : 0;
}

void legacy_panel_category_flags_set(int panel, uint32_t flags)
{
    if (panel >= 0 && panel < 18) s_legacy_panels[panel].flags = flags;
}

void legacy_panel_category_record_install(int panel, const float rect[4],
                                         uint32_t flags,
                                         const uint32_t* selections,
                                         uint32_t selection_count)
{
    LegacyPanelPortRecord* record;
    uint32_t i;
    if (panel < 0 || panel >= 18 || rect == NULL) return;
    record = &s_legacy_panels[panel];
    memcpy(record->initial, rect, sizeof(record->initial));
    memcpy(record->current, rect, sizeof(record->current));
    record->flags = flags;
    if (selection_count > 64) selection_count = 64;
    record->selection_count = selection_count;
    for (i = 0; i < selection_count; ++i) {
        record->selection_flags[i] = selections != NULL ? selections[i] : 0;
        record->selection_values[i] = 0;
    }
}

void* legacy_panel_image_get(int panel)
{
    return panel >= 0 && panel < 18 ? s_legacy_panels[panel].image : NULL;
}

void legacy_panel_image_slot_entry_release(void* opaque)
{
    LegacyPanelImagePortEntry* entry = (LegacyPanelImagePortEntry*)opaque;
    if (entry == NULL) return;
    if (entry->surface != NULL && s_legacy_hud_ops.surface_release != NULL)
        s_legacy_hud_ops.surface_release(entry->surface,
                                         s_legacy_hud_ops.context);
    engine_heap_free(entry);
}

#if defined(LEGORRR_BACKEND_WINAPI) && defined(_M_IX86)
#include "../../tools/diagnostics/wine-nerp-owner-probe.inc" /* DEBUG-next-owner */
#endif
