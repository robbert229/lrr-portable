// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; #496): what lighteffects.cpp sees from
 * effects.cpp. A verbatim snapshot of effects_shared.hpp as it was when the
 * #496 migration froze it, so the unit's declarations no longer change
 * with later relocations. Never edited by tools/relocate_unit.py. */
#ifndef LRR_GAME_EFFECTS_FOR_LIGHTEFFECTS_HPP
#define LRR_GAME_EFFECTS_FOR_LIGHTEFFECTS_HPP

struct ScreenEffectTickRecord {
    void* entity;
    float remaining;
};

extern struct ScreenEffectTickRecord s_screen_effect_tick_records[3][50];
extern ScreenEffectTickHooks s_screen_effect_tick_hooks;

extern SceneTransformState s_scene_transform_state;
extern LightEffectParameterState s_light_effect_parameters;
extern LightEffectConfigLoadHook s_light_effect_config_load_hook;
extern void* s_light_effect_config_load_context;
int light_effect_config_range_pair_read(BlockFile* config, const char* path, float* minimum, float* maximum);
uint32_t light_effect_float_word(float value);

typedef struct TransientEffectInstanceLinks {
    void* instance;
    void* scene_object;
    void* render_record;
    struct TransientEffectInstanceLinks* next;
    struct TransientEffectInstanceLinks* previous;
    struct TransientEffectInstanceLinks* all_next;
} TransientEffectInstanceLinks;

extern TransientEffectHooks s_transient_effect_hooks;
extern TransientEffectQuadBasisGetHook s_transient_effect_quad_basis_hook;
extern void* s_transient_effect_quad_basis_context;
extern TextureSurface* s_transient_effect_textures[12];
extern uint32_t s_transient_effect_texture_count;
extern void* s_transient_effect_head;
extern void* s_transient_effect_tail;
extern TransientEffectInstanceLinks* s_transient_effect_links;
extern int s_transient_effect_enabled;
TransientEffectInstanceLinks* transient_effect_links_find( const void* instance);
TransientEffectInstanceLinks* transient_effect_links_add(void* instance);
void transient_effect_link_pointer_set(void* instance, size_t offset, void* value);

extern TimedEffectRuntimeHooks s_timed_effect_runtime_hooks;
extern TimedEffectSlotRenderHooks s_timed_effect_slot_render_hooks;

#endif /* LRR_GAME_EFFECTS_FOR_LIGHTEFFECTS_HPP */
