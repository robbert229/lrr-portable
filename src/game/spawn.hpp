// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef LEGORRR_GAME_SPAWN_HPP
#define LEGORRR_GAME_SPAWN_HPP
struct EntitySlot;
namespace lego {
typedef void (*SpawnRandomTriggerHook)(void* entity, void* context);
typedef void (*SpawnScatterHook)(void* entity, int count, int mode, void* context);
typedef int (*SpawnScenePositionGetHook)(::EntitySlot* entity,
                                         float position_xy[2], void* context);
typedef void (*PairingActivityPrepareHook)(::EntitySlot* target,
                                            void* context);
void spawn_hooks_set(SpawnRandomTriggerHook trigger, SpawnScatterHook scatter, void* context);
void spawn_scene_position_get_hook_set(SpawnScenePositionGetHook hook,
                                       void* context);
void pairing_activity_prepare_hook_set(PairingActivityPrepareHook hook,
                                      void* context);
void pairing_activity_prepare(::EntitySlot* target);
void scatter_spawn_gate_set(int enabled); /* 0x0044adc0 */
int scatter_spawn_gate_get();
void entity_kind_one_random_spawn_trigger(void* entity); /* 0x0044ade0 */
void random_kind_six_scatter_spawn(void* entity, int count, int mode); /* 0x0044ae50 */
typedef void (*LegacyInterfaceInvoke)(void* object, int argument);
void interface_invoke_one_if_nonnull(void* object); /* 0x0047ed70 */
typedef int (*LegacyResultCheckHook)(void* context, void* value);
void legacy_result_check_hook_set(LegacyResultCheckHook hook, void* context);
int zero_result_is_success(void* value); /* 0x00480780 */
typedef int (*GlobalScatterSpawnFindHook)(::EntitySlot* entity, int mode,
                                          void* context);
void global_scatter_spawn_find_hook_set(GlobalScatterSpawnFindHook hook,
                                        void* context);
void global_scatter_spawn_state_set(int flag_a, int flag_b);
void global_scatter_spawn_enable_and_drain(void); /* FUN_0044c550 */
typedef struct ConfiguredMonsterSpawnHooks {
    void* context;
    void (*attached_spawn_record_process)(::EntitySlot* entity,
                                          void* context);
    void (*configured_spawn_teardown)(::EntitySlot* entity, void* context);
    void (*global_counter_increment)(void* context);
} ConfiguredMonsterSpawnHooks;
void configured_monster_spawn_hooks_set(
    const ConfiguredMonsterSpawnHooks* hooks);
int configured_monster_spawn_behavior_tick(::EntitySlot* entity);
}
#endif
