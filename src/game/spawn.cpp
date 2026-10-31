// SPDX-License-Identifier: LGPL-3.0-or-later
#include "spawn.hpp"
#include "entity.hpp"
#include <string.h>
namespace lego {
namespace {
unsigned int s_spawn_flags = 0;
SpawnRandomTriggerHook s_trigger_hook = 0;
SpawnScatterHook s_scatter_hook = 0;
void* s_hook_context = 0;
LegacyResultCheckHook s_result_check_hook = 0;
void* s_result_check_context = 0;
GlobalScatterSpawnFindHook s_global_scatter_find_hook = 0;
void* s_global_scatter_find_context = 0;
int s_global_scatter_flag_a = 0;
int s_global_scatter_flag_b = 0;
ConfiguredMonsterSpawnHooks s_configured_monster_spawn_hooks;

static int global_scatter_find_adapter(EntitySlot* entity, void* payload)
{
    int mode = payload != NULL ? *(int*)payload : 0;
    if (s_global_scatter_find_hook == NULL) return 0;
    return s_global_scatter_find_hook(entity, mode,
                                     s_global_scatter_find_context);
}

static int global_scatter_find(int mode)
{
    return entity_pool_callback_search(global_scatter_find_adapter, &mode);
}
}
void spawn_hooks_set(SpawnRandomTriggerHook trigger, SpawnScatterHook scatter, void* context)
{ s_trigger_hook = trigger; s_scatter_hook = scatter; s_hook_context = context; }
// FUNCTION: LEGORR 0x0044adc0
void scatter_spawn_gate_set(int enabled)
{ if (enabled != 0) s_spawn_flags |= 0x20U; else s_spawn_flags &= ~0x20U; }
int scatter_spawn_gate_get() { return (s_spawn_flags & 0x20U) != 0; }
// FUNCTION: LEGORR 0x0044ade0
void entity_kind_one_random_spawn_trigger(void* entity)
{ if (entity != 0 && !scatter_spawn_gate_get() && s_trigger_hook != 0) s_trigger_hook(entity, s_hook_context); }
// FUNCTION: LEGORR 0x0044ae50
void random_kind_six_scatter_spawn(void* entity, int count, int mode)
{ if (entity != 0 && count > 0 && s_scatter_hook != 0) s_scatter_hook(entity, count, mode, s_hook_context); }
// FUNCTION: LEGORR 0x0047ed70
void interface_invoke_one_if_nonnull(void* object)
{
    if (object != 0) {
        void** vtable = *static_cast<void***>(object);
        LegacyInterfaceInvoke invoke = reinterpret_cast<LegacyInterfaceInvoke>(vtable[0]);
        invoke(object, 1);
    }
}
void legacy_result_check_hook_set(LegacyResultCheckHook hook, void* context)
{ s_result_check_hook = hook; s_result_check_context = context; }
// FUNCTION: LEGORR 0x00480780
int zero_result_is_success(void* value)
{ return s_result_check_hook != 0 && s_result_check_hook(s_result_check_context, value) == 0; }

void global_scatter_spawn_find_hook_set(GlobalScatterSpawnFindHook hook,
                                        void* context)
{
    s_global_scatter_find_hook = hook;
    s_global_scatter_find_context = context;
}

void global_scatter_spawn_state_set(int flag_a, int flag_b)
{
    s_global_scatter_flag_a = flag_a != 0;
    s_global_scatter_flag_b = flag_b != 0;
}

// FUNCTION: LEGORR 0x0044c550
void global_scatter_spawn_enable_and_drain(void)
{
    s_spawn_flags |= 0x40u;
    if (global_scatter_find(0) != 0) return;
    if (s_global_scatter_flag_b == 0) {
        if (global_scatter_find(1) == 0 && s_global_scatter_flag_a != 0) {
            s_global_scatter_flag_a = 0;
            s_global_scatter_flag_b = 0;
            global_scatter_find(1);
            return;
        }
    }
    else {
        s_global_scatter_flag_a = 0;
        s_global_scatter_flag_b = 0;
        global_scatter_find(0);
    }
}

void configured_monster_spawn_hooks_set(
    const ConfiguredMonsterSpawnHooks* hooks)
{
    if (hooks == NULL) {
        memset(&s_configured_monster_spawn_hooks, 0,
               sizeof(s_configured_monster_spawn_hooks));
    }
    else {
        s_configured_monster_spawn_hooks = *hooks;
    }
}

// FUNCTION: LEGORR 0x0044bfd0
int configured_monster_spawn_behavior_tick(EntitySlot* entity)
{
    if (entity == NULL || entity->activity_timer <= 0.0f) return 0;
    entity->flags_3e0 &= ~0x00100000u;
    if (s_configured_monster_spawn_hooks.configured_spawn_teardown != NULL) {
        s_configured_monster_spawn_hooks.configured_spawn_teardown(
            entity, s_configured_monster_spawn_hooks.context);
    }
    if (s_configured_monster_spawn_hooks.attached_spawn_record_process != NULL) {
        s_configured_monster_spawn_hooks.attached_spawn_record_process(
            entity, s_configured_monster_spawn_hooks.context);
    }
    entity->flags_3e8 |= 0x00800000u;
    if (s_configured_monster_spawn_hooks.global_counter_increment != NULL) {
        s_configured_monster_spawn_hooks.global_counter_increment(
            s_configured_monster_spawn_hooks.context);
    }
    return 1;
}
}
