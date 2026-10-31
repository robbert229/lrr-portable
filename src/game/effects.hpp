// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef LEGORRR_GAME_EFFECTS_HPP
#define LEGORRR_GAME_EFFECTS_HPP

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* EffectModeSurfaceQueryTypeResolve (FUN_0046fef0). Unknown modes leave
 * both output words untouched, matching the reference switch. */
void effect_mode_surface_query_type_resolve(uint32_t mode,
                                            uint32_t* surface_type,
                                            uint32_t* flags);

/* The two fixed corpus pools each expose ten records. Their first word is
 * the occupancy sentinel; callers can mark a slot when constructing or
 * releasing the owning record. */
int active_effect_record_slot_find(void); /* FUN_0046ff70 */
int active_segment_visual_record_slot_find(void); /* FUN_0046ff90 */
void active_effect_record_slot_set(int slot, int occupied);
void active_segment_visual_record_slot_set(int slot, int occupied);
void active_effect_record_slots_clear(void);
void active_segment_visual_record_slots_clear(void);
typedef void (*EntityImpulseApplyHook)(void* entity, float magnitude,
                                       int mode, float scale);
typedef void (*EntityDirectionApplyHook)(void* entity, uint32_t mode,
                                         uint32_t direction);
void entity_effect_motion_hooks_set(EntityImpulseApplyHook impulse_hook,
                                    EntityDirectionApplyHook direction_hook);
void entity_apply_impulse_and_direction(void* entity, float magnitude,
                                        uint32_t direction); /* FUN_0046f380 */
typedef void* (*TimedSegmentVisualCreateHook)(void* scene, uint32_t flags);
typedef void (*TimedSegmentVisualBuildHook)(void* visual, float length,
                                           const float start[3],
                                           const float end[3],
                                           float red, float green, float blue,
                                           float alpha);
typedef struct TimedSegmentVisualHooks {
    void* (*scene_get)(void* entity);
    TimedSegmentVisualCreateHook visual_create;
    TimedSegmentVisualBuildHook segment_build;
} TimedSegmentVisualHooks;
void timed_segment_visual_hooks_set(const TimedSegmentVisualHooks* hooks);
void timed_entity_segment_visual_create(void* entity, const float start[3],
                                       const float end[3]); /* FUN_00470690 */
typedef void (*MeshSegmentQuadEmitHook)(void* mesh, const float vertices[12],
                                        float red, float green, float blue,
                                        float alpha);
void mesh_segment_quad_emit_hook_set(MeshSegmentQuadEmitHook hook);
void mesh_build_eight_sided_segment(void* mesh, float radius,
                                    const float start[3], const float end[3],
                                    float red, float green, float blue,
                                    float alpha); /* FUN_00470760 */
typedef struct KindEffectDispatchHooks {
    void* context;
    int (*entity_position_get)(void* context, void* entity, float out_xyz[3]);
    void (*facing_vector_get)(void* context, void* entity, float out_xyz[3]);
    float (*terrain_height_get)(void* context, float x, float y);
    float (*entity_half_height_get)(void* context, void* entity);
    float (*resource_duration_get)(void* context, const char* resource_name);
    void (*effect_spawn)(void* context, void* entity, int effect_kind,
                         const float position[3], const float direction[3],
                         float duration);
    void (*cooldown_set)(void* context, void* entity, float duration);
} KindEffectDispatchHooks;
void kind_effect_dispatch_hooks_set(const KindEffectDispatchHooks* hooks);
void kind_effect_slot_init_dispatch(void* entity, int kind); /* FUN_004420d0 */
typedef struct ActiveEffectRecord {
    uint32_t initialized;
    uint32_t auxiliary;
    uint32_t mode;
    void* owner;
    void* spawned_entity;
    float position[3];
    float previous_position[3];
    float direction[3];
    float speed;
    float duration;
    float curve[4];
    uint32_t sample_count;
} ActiveEffectRecord;
typedef struct ActiveEffectBuildHooks {
    void* context;
    int (*surface_type_exists)(void* context, uint32_t type, uint32_t flags);
    void (*pre_spawn_prepare)(void* context, void* owner, uint32_t auxiliary);
    void* (*entity_spawn)(void* context, uint32_t surface_type,
                         uint32_t flags, const float position[3]);
    float (*flight_curve_build)(void* context, ActiveEffectRecord* record,
                                const float end[3]);
    void (*record_activate)(void* context, ActiveEffectRecord* record,
                            int slot);
} ActiveEffectBuildHooks;
void active_effect_build_hooks_set(const ActiveEffectBuildHooks* hooks);
const ActiveEffectRecord* active_effect_record_peek(int slot);
int entity_spawn_record_slot_init(void* owner, const float position[3],
                                  const float direction[3],
                                  uint32_t auxiliary, uint32_t mode); /* FUN_0046faf0 */
int object_flight_curve_record_init(void* owner, const float start[3],
                                    const float end[3], uint32_t auxiliary,
                                    uint32_t mode); /* FUN_0046fc70 */
typedef struct EffectApplyHooks {
    void* context;
    uint32_t (*entity_flags_get)(void* context, void* entity);
    int (*entity_motion_check)(void* context, void* entity);
    float (*entity_value_get)(void* context, void* entity, uint32_t selector);
    void (*command_effect_apply)(void* context, int mode, int command);
    void (*impulse_apply)(void* context, void* entity, float amount,
                          int direction);
    void (*effect_apply)(void* context, void* entity, int mode,
                         const float direction_xy[2], float amount,
                         float duration);
    void (*generic_motion_apply)(void* context, void* entity,
                                 uint32_t type, const float direction_xy[2]);
} EffectApplyHooks;
void effect_apply_hooks_set(const EffectApplyHooks* hooks);
void effect_apply_by_kind_branch(void* entity, const float direction_xy[2],
                                 int command, uint32_t type, int mode); /* FUN_0046f770 */
typedef void (*ScreenSpaceEffectDispatchHook)(void* context, void* effect,
                                               void* argument, int mode,
                                               float scale, int flags);
void screen_space_effect_dispatch_hook_set(ScreenSpaceEffectDispatchHook hook,
                                           void* context);
void screen_space_effect_dispatch_if_enabled(uint8_t effect_flags,
                                             uint32_t render_flags,
                                             void* effect, void* argument);
                                                /* FUN_0040e8d0 */
typedef struct ScreenEffectTickHooks {
    void* context;
    int (*simulation_paused)(void* context);
    int (*entity_eligible)(void* entity, int pool, void* context);
    void (*entity_tick)(void* entity, int pool, float delta, void* context);
    void (*pulse_tick)(void* entity, float delta, void* context);
} ScreenEffectTickHooks;
void screen_effect_tick_hooks_set(const ScreenEffectTickHooks* hooks);
void screen_effect_slot_set(int pool, int slot, void* entity, float remaining);
void* screen_effect_slot_peek(int pool, int slot, float* remaining);
void screen_effects_tick(float delta); /* FUN_004075a0 */
typedef void (*SceneTransformApplyHook)(void* context, void* target,
                                        float x, float y, float z,
                                        float scale);
typedef struct SceneTransformState {
    void* target;
    void* owner;
    float base[3];
    float current[3];
    uint32_t flags;
} SceneTransformState;
void scene_transform_apply_hook_set(SceneTransformApplyHook hook,
                                    void* context);
void scene_transform_state_initialize(void* target, void* owner,
                                      float x, float y, float z);
void scene_transform_enable_set(int enabled);
void scene_transform_current_set(float x, float y, float z);
void scene_transform_rgb_clamp_apply(void); /* FUN_0044cff0 */
const SceneTransformState* scene_transform_state_peek(void);
typedef void (*EntityNodeScaleApplyHook)(void* context, void* entity,
                                         float first_scale,
                                         float second_scale);
void entity_node_scale_apply_hook_set(EntityNodeScaleApplyHook hook,
                                      void* context);
void entity_node_scale_update(void* entity, int doubled); /* FUN_0044bb30 */
typedef struct LightEffectParameterState {
    uint32_t flags;
    uint32_t blink[6];
    uint32_t fade[12];
    uint32_t move[9];
} LightEffectParameterState;
void light_effect_blink_parameters_store(uint32_t a, uint32_t b,
                                         uint32_t c, uint32_t d,
                                         uint32_t e, uint32_t f);
void light_effect_fade_parameters_store(uint32_t a, uint32_t b, uint32_t c,
                                        uint32_t d, uint32_t e, uint32_t f,
                                        uint32_t g, uint32_t h, uint32_t i,
                                        uint32_t j, uint32_t k, uint32_t l);
void light_effect_move_parameters_store(uint32_t a, uint32_t b, uint32_t c,
                                        uint32_t d, uint32_t e, uint32_t f,
                                        uint32_t g, uint32_t h, uint32_t i);
const LightEffectParameterState* light_effect_parameter_state_peek(void);
typedef uint32_t (*LightEffectConfigLoadHook)(void* context, int kind,
                                              void* config, void* section);
void light_effect_config_load_hook_set(LightEffectConfigLoadHook hook,
                                       void* context);
uint32_t light_effect_blink_config_load(void* config, void* section);
uint32_t light_effect_fade_config_load(void* config, void* section);
uint32_t light_effect_move_config_load(void* config, void* section);
void light_effect_configuration_load(void* config, void* section);
typedef struct TransientEffectHooks {
    void* context;
    int (*textures_preload)(void* context);
    void* (*emitter_create)(void* context, int kind,
                            const float position[3]);
    void (*instance_release)(void* context, void* instance);
} TransientEffectHooks;
void transient_effect_hooks_set(const TransientEffectHooks* hooks);
int transient_effect_textures_preload(void);
void* transient_effect_emitter_create(int kind, const float position[3]);
void transient_effect_instance_release(void* instance);
void transient_effect_registry_release_all(void);
void transient_effect_registry_enable_set(int enabled);
int transient_effect_registry_enabled_get(void);

typedef struct EventTimerState {
    uint32_t flags;
    uint32_t external_flags;
    int current_event;
    int next_event;
    float timer;
    float rate;
    float lower;
    float upper;
    float next_lower;
    float next_upper;
} EventTimerState;
typedef void (*EventTimerActionHook)(void* context, int action, int value);
void event_timer_state_set(EventTimerState* state);
void event_timer_action_hook_set(EventTimerActionHook hook, void* context);
void event_timer_countdown_advance(float delta); /* FUN_0041eac0 */
typedef struct TimedEffectCueSlot {
    uint32_t flags;
    float timer;
    uint32_t cue_id;
    uint32_t render_id;
    float x;
    float y;
} TimedEffectCueSlot;
typedef struct TimedEffectCueState {
    TimedEffectCueSlot slots[45];
    float threshold;
    uint32_t cue_active;
    uint32_t cue_suppressed;
} TimedEffectCueState;
typedef struct TimedEffectCueHooks {
    void* context;
    int (*cue_done)(void* context, uint32_t cue_id);
    void (*slot_render)(void* context, const TimedEffectCueSlot* slot,
                        float x, float y);
} TimedEffectCueHooks;
void timed_effect_cue_update(TimedEffectCueState* state, float x, float y,
                             float delta,
                             const TimedEffectCueHooks* hooks); /* 0x0046b7c0 */
typedef struct RectangularGridOverlayState {
    float x;
    float y;
    float width;
    float height;
    unsigned int bands;
    uint32_t fill_color[3];
    uint32_t first_line_color[3];
    uint32_t second_line_color[3];
    int overlay_mode;
} RectangularGridOverlayState;
typedef struct RectangularGridOverlayHooks {
    void* context;
    void (*region_fill)(void* context, const RectangularGridOverlayState* state,
                        int mode);
    void (*polyline_draw)(void* context, const float points[10],
                          unsigned int count, const uint32_t color[3],
                          int mode);
} RectangularGridOverlayHooks;
void rectangular_grid_overlay_geometry_draw(
    const RectangularGridOverlayState* state,
    const RectangularGridOverlayHooks* hooks); /* 0x0046b8b0 */
typedef struct TimedEffectRenderItem {
    void* handle;
    float width;
    float height;
    int enabled;
} TimedEffectRenderItem;
typedef struct TimedEffectSlotRenderState {
    uint32_t flags;
    float top_a;
    float top_b;
    float left;
    float alternate_top;
    float alternate_left;
    float column_width;
    float column;
    float wanted_width;
    float wanted_height;
    float view_width;
    float view_height;
    float origin_y;
    unsigned int bands;
    unsigned int padding;
    TimedEffectRenderItem* items;
    unsigned int item_count;
    TimedEffectRenderItem special_item;
    uint32_t colors[4][3];
} TimedEffectSlotRenderState;
typedef struct TimedEffectSlotRenderHooks {
    void* context;
    RectangularGridOverlayHooks overlay_hooks;
    void (*overlay_background)(void* context, float x, float y,
                               float width, float height,
                               const uint32_t colors[4][3], int mode);
    void (*render_item)(void* context, void* item, float x, float y,
                        float width, float height);
} TimedEffectSlotRenderHooks;
void timed_effect_slot_render(const TimedEffectSlotRenderState* state,
                              const TimedEffectSlotRenderHooks* hooks); /* 0x0046bc30 */

#ifdef __cplusplus
}
#endif

#endif
