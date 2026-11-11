// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef LEGORRR_GAME_TUTORIAL_HUD_PORTS_HPP
#define LEGORRR_GAME_TUTORIAL_HUD_PORTS_HPP

#include <stdint.h>
#include <stdarg.h>

typedef struct BlockRec BlockFile;
struct GameContext;
struct EngineContext;
struct InputState;

typedef void* (*TutorialScriptLoadFn)(const char* path, uint32_t* size_out,
                                      void* context);
typedef void (*TutorialScriptFreeFn)(void* data, void* context);
typedef uint32_t (*TutorialTokenResolveFn)(int kind, const char* text,
                                           void* context);
typedef uint32_t (*TutorialCommandFn)(uint32_t command,
                                      const uint32_t* args,
                                      uint32_t arg_count, void* context);
typedef uint32_t (*TutorialCommandKindFn)(uint32_t command, void* context);
typedef uint32_t (*TutorialIndirectResolveFn)(uint32_t index, void* context);
typedef struct TutorialScriptOps {
    void* context;
    TutorialScriptLoadFn load;
    TutorialScriptFreeFn release;
    TutorialTokenResolveFn resolve;
    TutorialCommandFn command;
    TutorialCommandKindFn command_kind;
    TutorialIndirectResolveFn indirect_resolve;
    void (*mode_set)(int mode, void* context);
    void (*resource_state_initialize)(void* context);
    void (*state_reset)(void* context);
    void (*resource_state_release)(void* context);
    void (*resolved_token_release)(int kind, uint32_t value,
                                   void* context);
    void (*scroll_commit)(uint32_t index, void* context);
    void (*camera_focus)(uint32_t index, float x, float y, void* context);
    void (*camera_offset)(float value, void* context);
    void (*camera_rotation)(float radians, void* context);
    float (*delta_scale)(void* context);
    uint32_t (*camera_state)(void* context);
    void (*camera_scroll)(uint32_t amount, float scale, void* context);
    void (*text_draw)(const char* text, int x, int y, void* context);
} TutorialScriptOps;

void tutorial_script_ops_set(const TutorialScriptOps* ops);
void tutorial_runtime_context_set(struct GameContext* game,
                                  struct EngineContext* engine,
                                  const struct InputState* input);
void tutorial_nerp_pointer_release_record(int action, int cell_x, int cell_y);
/* SetRecordObjectPointer retains a one-based OList slot (zero disables it).
 * Resolve the slot through the live registry at use time. Indexed actor-arrow
 * rendering is not yet bound to this owner. */
uint32_t tutorial_record_object_pointer_get(void);
int tutorial_script_file_load(const char* path);
void tutorial_script_program_install(const uint32_t* words, uint32_t word_count);
/* FUN_00452e70 receives the configured NERPMessageFile path. Parsed rows and
 * resolved handles are retained in the objective token tables. */
int tutorial_objective_token_load(const char* path);
int tutorial_objective_token_file_load(const char* path,
                                       char** output_tokens);
const char* tutorial_objective_token_lookup(uint32_t index);
int tutorial_display_resources_release(void);
void tutorial_indirect_handle_resolve(uint32_t* value);
void tutorial_frame_timer_array_advance(float delta);
void tutorial_script_command_dispatch(float delta);
void tutorial_advisor_scroll_step_advance(void);
/* Single production message owner; history index zero deliberately reads
 * the frontend inline sound slot. Delta remains the reference float ABI. */
void tutorial_message_generation_sync(void);
uint32_t tutorial_message_history_get(uint32_t index);
void tutorial_message_timer_values_get(float values[3]);
void tutorial_message_duration_set(float duration);
void tutorial_message_sound_active_set(void);
void tutorial_camera_focus_state_set(uint32_t focus_id, uint32_t valid,
                                     float offset_target, float angle_target,
                                     uint32_t offset_active,
                                     uint32_t angle_active);
/* Camera-only delta is in reference 25-Hz ticks, not AI seconds.
 * A nonpositive delta leaves requests/tween accumulators unchanged. */
void tutorial_camera_focus_tween_update(float delta);
void tutorial_inline_token_render(char* source, char* output, int draw);
uint32_t tutorial_script_word_count(void);
uint32_t tutorial_token_count(void);
uint32_t tutorial_literal_token_count(void);
const char* tutorial_token_name_get(uint32_t index);
uint32_t tutorial_token_value_get(uint32_t index);

typedef void* (*LegacyHudSurfaceLoadFn)(BlockFile* cfg, const char* path,
                                        int x, int y, void* context);
typedef void (*LegacyHudSurfaceReleaseFn)(void* surface, void* context);
typedef int (*LegacyHudSurfaceHitFn)(void* surface, int x, int y,
                                     void* context);
typedef void (*LegacyHudSurfaceDrawFn)(void* surface, float x, float y,
                                       void* context);
typedef void (*LegacyHudRectDrawFn)(const float rect[4], uint32_t color,
                                    void* context);
typedef void (*LegacyHudPolylineDrawFn)(const float* xy, uint32_t count,
                                        uint32_t color, void* context);
typedef void (*LegacyHudRingStripDrawFn)(const float first_xy[20],
                                         const float second_xy[20],
                                         uint32_t point_count,
                                         const float rgb[3], int mode,
                                         void* context);
typedef int (*LegacyHudTextMeasureFn)(const char* text, void* context);
typedef void (*LegacyHudTextDrawFn)(const char* text, int x, int y,
                                    void* context);
struct UiOverlayInitSink;
struct TextureSurface;
typedef struct LegacyHudOps {
    void* context;
    int (*entity_renderable)(void* entity, void* context);
    LegacyHudSurfaceLoadFn surface_load;
    LegacyHudSurfaceLoadFn surface_alternate_load;
    LegacyHudSurfaceReleaseFn surface_release;
    LegacyHudSurfaceHitFn surface_hit;
    LegacyHudSurfaceDrawFn surface_draw;
    LegacyHudRectDrawFn rect_draw;
    LegacyHudPolylineDrawFn polyline_draw;
    LegacyHudRingStripDrawFn ring_strip_draw;
    LegacyHudTextMeasureFn text_measure;
    LegacyHudTextDrawFn text_draw;
    int (*config_pair)(BlockFile* cfg, const char* path, int* x, int* y,
                       void* context);
    int (*config_rgb)(BlockFile* cfg, const char* path, float rgb[3],
                      void* context);
    uint32_t (*config_resource)(BlockFile* cfg, const char* path,
                                char* out, uint32_t out_size,
                                void* context);
    void (*surface_initialize)(void* surface, void* context);
    void (*scroll_table_initialize)(void* context, int table);
    void (*scroll_control_initialize)(void* context, int category,
                                      int slot, int enabled);
    const struct UiOverlayInitSink* overlay_init_sink;
    TextureSurface* (*panel_texture_load)(const char* path, void* context);
} LegacyHudOps;

typedef struct LegacyHealthBarState {
    float x, y, width, height, border;
    float border_rgb[3];
    float border_high[3];
    float border_low[3];
    float background_rgb[3];
    float health_rgb[3];
    uint32_t vertical;
    uint32_t flags;
} LegacyHealthBarState;

void legacy_hud_ops_set(const LegacyHudOps* ops);
int legacy_health_bar_config_load(BlockFile* values, const char* config_root);
int legacy_hunger_images_position_config_load(BlockFile* values,
                                              const char* config_root);
void legacy_hunger_images_load(BlockFile* values, const char* config_root);
int legacy_bubble_images_position_config_load(BlockFile* values,
                                               BlockFile* names);
void legacy_health_bar_config_peek(LegacyHealthBarState* state);
void legacy_entity_screen_ring_render(void* entity, int x, int y);
void legacy_entity_screen_ring_render_with_ops(
    void* entity, int x, int y, const LegacyHudOps* ops);
void legacy_scroll_panel_controls_initialize(void);
void legacy_panel_category_interaction_state_reset(void);
void legacy_overlay_image_record_load(const char* path, int panel,
                                      uint32_t state);
int legacy_panel_slot_coordinate_hit_test(int panel, int x, int y);
void* legacy_panel_image_slot_entry_allocate(int panel, const char* path,
                                             const int rect[4],
                                             uint32_t flags);
void legacy_scrolling_advisor_text_append(void* record, const char* format, ...);
void legacy_scrolling_advisor_text_append_v(void* record, const char* format,
                                            va_list args);
void legacy_ui_text_output_position_and_draw(void* record, const char* text,
                                            const char* fallback);
uint32_t legacy_panel_category_flags_get(int panel);
void legacy_panel_category_flags_set(int panel, uint32_t flags);
void legacy_panel_category_record_install(int panel, const float rect[4],
                                         uint32_t flags,
                                         const uint32_t* selections,
                                         uint32_t selection_count);
struct TextureSurface;
TextureSurface* legacy_panel_texture_get(int panel);
void* legacy_panel_image_get(int panel);
void legacy_panel_image_slot_entry_release(void* entry);

#endif
