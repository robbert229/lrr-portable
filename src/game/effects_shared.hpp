// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private to src/game/effects.cpp and the units relocated out of it (ADR 0006
 * decision 4; tools/relocate_unit.py). Statics both sides use have
 * external linkage through the declarations below. Include it only from
 * those files: it is not a shared header. */
#ifndef LRR_GAME_EFFECTS_SHARED_HPP
#define LRR_GAME_EFFECTS_SHARED_HPP

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

#endif /* LRR_GAME_EFFECTS_SHARED_HPP */
