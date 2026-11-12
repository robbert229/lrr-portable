// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-050 GAME-050: the unit's functions in original address order and the
 * state they own, relocated verbatim by tools/relocate_unit.py (ADR 0006).
 * Body hashes: docs/coverage/relocations/GAME-050.csv. */
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include "ai.hpp"
#include "entity.hpp"
#include "building_scene.hpp"
#include "actor_scene.hpp"
#include "type_config_ports.hpp"
#include "entity_pose_delta.hpp"
#include "entity_motion_ports.hpp"
#include "activity_timeout.hpp"
#include "entity_motion_probe.hpp"
#include "port_loop_ports.hpp"
#include "simulation_ui_ports.hpp"
#include "type_motion_scale.hpp"
#include "entity_quad_collision.hpp"
#include "spawn.hpp"
#include "weapon.hpp"
#include "effects.hpp"
#include "level.hpp"
#include "terrain_grid.hpp"
#include "cmd.hpp"
#include "select.hpp"
#include "gods98/model.hpp"
#include "gods98/view.hpp"
#include "gods98/engine.hpp"
#include "gods98/resource.hpp"
#include "gods98/sound.hpp"
#include "gods98/record.hpp"
#include "gods98/heap.hpp"
#include "gods98/message.hpp"
#include "gods98/string.hpp"
#include "gods98/block.hpp"
#include "upgrades.hpp"
#include "crt/crt.hpp"
#include "frontend.hpp"
#include "entity_shared.hpp"
#include "util.hpp"
#include "gods98/media.hpp"
#include "combat_ray_ports.hpp"
#include "simtick.hpp"
#include "gods98/scene_pool.hpp"
#include "gods98/scene_resources.hpp"
#include "gods98/surface.hpp"
#include "gods98/texture.hpp"
#include "bezier.h"
#include <stddef.h>
#include <limits.h>
#include "record_shared.hpp"
#include "game/render.hpp"
#include "entity_for_weapons.hpp"
#include "cmd_for_weapons.hpp"
#include "effects_for_weapons.hpp"
#include "weapon_for_weapons.hpp"
#include "gods98/simtick_for_weapons.hpp"

namespace lego {
// GLOBAL: LEGORR 0x00503878
WeaponIndexedTypeRecord* s_weapon_indexed_records;
} // namespace lego

namespace lego {
namespace {
void* s_weapon_config = 0;
} // namespace
} // namespace lego

namespace unported_ledger_stubs {
int ledger_stub_004702b0_EntitySurfaceQuadSegmentIntersect(
    EntitySlot* entity, const float start_xy[2], const float end_xy[2],
    float out_xy[2], int check_side);
} // namespace unported_ledger_stubs

/* Defined below (forward declarations added by tools/relocate_unit.py). */
namespace lego { namespace { void weapon_definition_reset(WeaponDefinition* definition); } }
namespace lego { namespace { int ascii_case_equal(const char* a, const char* b); } }
namespace unported_ledger_stubs { static float combat_ray_configured_range_get(void*, int kind); }
namespace unported_ledger_stubs { static void combat_ray_entity_release(void*, void* entity); }
namespace unported_ledger_stubs { static int combat_ray_scene_trace(void*, const float origin[3], const float direction[3], float* hit_fraction, float hit_point[3], void** hit_object_out); }
static int sfx_entity_scan_callback(EntitySlot* entity, void* payload);
static void active_effect_direction_normalize(float direction[3]);
static int active_effect_type_resolve(uint32_t mode, uint32_t* type, uint32_t* flags);
static void active_effect_curve_samples_build(ActiveEffectRecord* record, const float endpoint[3]);
static int entity_centers_within_radius(const EntitySlot* first, const EntitySlot* second, float radius);

namespace lego {
/* The original walks the block-text `WeaponTypes` section and fills a much
 * larger type/level grid. This portable owner preserves the directly used
 * footer fields and lets the existing config layer provide parsed entries. */
// FUNCTION: LEGORR 0x0046EB80
int weapon_type_config_load(void* config, void* context)
{
    uint32_t i;
    uint32_t count = 0;
    WeaponIndexedTypeRecord* records;
    s_weapon_config = config;
    s_weapon_type_count = 0;
    for (i = 0; i < WEAPON_TYPE_CAPACITY; ++i)
        weapon_definition_reset(&s_weapon_definitions[i]);
    if (s_weapon_parse_hook == 0) return 0;
    if (!s_weapon_parse_hook(config, context, s_weapon_definitions,
                             WEAPON_TYPE_CAPACITY, &count)) return 0;
    if (count > WEAPON_TYPE_CAPACITY) return 0;

    /* Construct the reference's indexed table from the fields available to
     * this parser. Its omitted per-cell config values start at the reference
     * sentinel (-1.0); parsed default damage and scalar footer values remain
     * live and are therefore available to 0x46f1d0 and the scalar getters. */
    records = count != 0 ? (WeaponIndexedTypeRecord*)engine_heap_alloc(
        (long)((size_t)count * sizeof(WeaponIndexedTypeRecord))) : 0;
    if (count != 0 && records == 0) return 0;
    for (i = 0; i < count; ++i) {
        uint32_t cell;
        WeaponIndexedTypeRecord* record = &records[i];
        for (cell = 0; cell < WEAPON_INDEXED_GRID_FLOAT_COUNT; ++cell)
            record->indexed_values[cell] = -1.0f;
        record->dispatch_mode = 0;
        record->dispatch_scale = 0.0f;
        record->dispatch_index = 0;
        record->recharge_time = s_weapon_definitions[i].recharge_time;
        record->default_damage = s_weapon_definitions[i].default_damage;
        record->discharge_rate = s_weapon_definitions[i].discharge_rate;
        record->unknown_scalar = 0.0f;
        record->range = s_weapon_definitions[i].range;
        for (cell = 0; cell < WEAPON_WALL_TYPE_COUNT; ++cell)
            record->wall_destroy_time[cell] =
                s_weapon_definitions[i].wall_destroy_time[cell];
    }
    if (s_weapon_owned_records != 0)
        engine_heap_free(s_weapon_owned_records);
    s_weapon_owned_records = records;
    s_weapon_indexed_records = records;
    s_weapon_indexed_record_count = count;
    s_weapon_type_count = count;
    return 1;
}

// FUNCTION: LEGORR 0x0046F0D0
uint32_t object_type_name_index_find(const char* name)
{
    uint32_t i;
    if (s_weapon_type_count == 0) return 1;
    for (i = 0; i < s_weapon_type_count; ++i) {
        if (ascii_case_equal(s_weapon_definitions[i].name, name)) return i;
    }
    return s_weapon_type_count + 1;
}

// FUNCTION: LEGORR 0x0046F110
extern "C" float weapon_type_recharge_time_get_target(int type)
{
    return s_weapon_indexed_records[type].recharge_time;
}

// FUNCTION: LEGORR 0x0046F140
float weapon_type_discharge_rate_get_target(int type)
{
    return s_weapon_indexed_records[type].discharge_rate;
}

// FUNCTION: LEGORR 0x0046F170
float weapon_type_range_get_target(int type)
{
    return s_weapon_indexed_records[type].range;
}

// FUNCTION: LEGORR 0x0046F1A0
float weapon_type_wall_destroy_time_get_target(int type, int wall_type)
{
    return s_weapon_indexed_records[type].wall_destroy_time[wall_type];
}

// FUNCTION: LEGORR 0x0046f1d0
float entity_indexed_float_lookup_with_minus_one_fallback(
    int weapon_type, const void* entity_record)
{
    const int32_t* fields = (const int32_t*)entity_record;
    const WeaponIndexedTypeRecord* record;
    int32_t entity_type;
    int32_t selected_index;
    int32_t level_index;
    int32_t cell_index;
    float value;
    if (fields == 0 || weapon_type < 0 ||
        (uint32_t)weapon_type >= s_weapon_indexed_record_count ||
        s_weapon_indexed_records == 0)
        return weapon_type >= 0 &&
               (uint32_t)weapon_type < s_weapon_type_count
            ? s_weapon_definitions[weapon_type].default_damage : -1.0f;
    entity_type = fields[0];
    selected_index = fields[1];
    level_index = fields[0xcc];
    if (entity_type < 0 || selected_index < 0 || level_index < 0)
        return -1.0f;
    cell_index = level_index +
        (entity_type * 3 + selected_index + entity_type * 0xc) * 0x10;
    if ((uint32_t)cell_index >= WEAPON_INDEXED_GRID_FLOAT_COUNT)
        return -1.0f;
    record = &s_weapon_indexed_records[weapon_type];
    value = record->indexed_values[cell_index];
    if (value == -1.0f) return record->default_damage;
    return value;
}
} // namespace lego

namespace unported_ledger_stubs {
// TypeMotionScaleApplyDispatch
// FUNCTION: LEGORR 0x0046f270
extern "C" void ledger_stub_0046f270_TypeMotionScaleApplyDispatch(
    EntitySlot* entity, int weapon_type, int apply_index, float scale,
    void* finish_argument)
{
    const TypeMotionScaleHooks* hooks = NULL;
    float base_value;
    float entry_scale = 0.0f;
    int entry_value = 0;
    int entry_present = 0;
    if (entity == NULL) return;
    base_value = lego::entity_indexed_float_lookup_with_minus_one_fallback(
        weapon_type, entity);
    if ((entity_state_flags_get(entity) & 0x00040000u) == 0u ||
        !entity_shared_eligibility_check(entity, 1)) return;
    if (hooks != NULL && hooks->indexed_entry_get != NULL) {
        entry_present = hooks->indexed_entry_get(hooks->context, weapon_type,
                                                 &entry_scale, &entry_value);
    }
    if (entry_present) {
        if (apply_index) {
            if (hooks != NULL && hooks->motion_apply != NULL) {
                hooks->motion_apply(hooks->context, entity,
                                    entry_scale * base_value, 1, scale);
            }
            else {
                entity_damage_apply(entity, entry_scale * base_value, 1);
            }
            if (hooks != NULL && hooks->entry_state_store != NULL)
                hooks->entry_state_store(hooks->context, entity, weapon_type,
                                         entry_value);
            if (hooks != NULL && hooks->finish_apply != NULL)
                hooks->finish_apply(hooks->context, entity, finish_argument, 1);
            else
                entity_directional_activity_start(entity,
                    (const float*)finish_argument, 1);
        }
        else {
            if (hooks != NULL && hooks->motion_apply != NULL) {
                hooks->motion_apply(hooks->context, entity,
                                    base_value * scale, 1, scale);
            }
            else {
                entity_damage_apply(entity, base_value * scale, 1);
            }
        }
        return;
    }
    if (entity->type == ENTITY_TYPE_OBJ) {
        if (hooks != NULL && hooks->type3_entry_miss != NULL) {
            hooks->type3_entry_miss(hooks->context, entity);
        }
        else {
            /* FUN_0046f270 calls FUN_00462890 on a type-3 table miss. */
            global_counter_at_0055532c_increment();
        }
    }
    if (hooks != NULL && hooks->motion_apply != NULL)
        hooks->motion_apply(hooks->context, entity, base_value, 1, scale);
    else
        entity_damage_apply(entity, base_value, 1);
    if (apply_index) {
        if (hooks != NULL && hooks->finish_apply != NULL)
            hooks->finish_apply(hooks->context, entity, finish_argument, 1);
        else
            entity_directional_activity_start(entity,
                (const float*)finish_argument, 1);
    }
    return;
}
} // namespace unported_ledger_stubs

// FUNCTION: LEGORR 0x0046f380
void entity_apply_impulse_and_direction(void* entity, float magnitude,
                                        uint32_t direction)
{
    if (s_entity_impulse_hook != NULL) {
        s_entity_impulse_hook(entity, magnitude, 1, 1.0f);
    }
    else {
        entity_damage_apply((EntitySlot*)entity, magnitude, 1);
    }
    if (s_entity_direction_hook != NULL) {
        s_entity_direction_hook(entity, 0, direction);
    }
    else {
        entity_directional_activity_start((EntitySlot*)entity, NULL,
                                          (int)direction);
    }
}

namespace unported_ledger_stubs {
// RayHitScanAndEffectApply
// FUNCTION: LEGORR 0x0046f3b0
void ledger_stub_0046f3b0_RayHitScanAndEffectApply(
    CombatRayState* state, const CombatRayHooks* hooks)
{
    CombatRayHooks live_hooks;
    if (hooks != NULL || state == NULL) {
        combat_ray_hit_scan_and_effect_apply(state, hooks);
        return;
    }
    live_hooks.configured_range_get = combat_ray_configured_range_get;
    live_hooks.raycast = combat_ray_scene_trace;
    live_hooks.entity_release = combat_ray_entity_release;
    /* The retail call passes the effect direction stored at record +0x290.
     * CombatRayState has no corresponding field, so leave this callback
     * unset instead of substituting the ray's origin-to-target delta. */
    live_hooks.impact_apply = NULL;
    live_hooks.context = state;
    combat_ray_hit_scan_and_effect_apply(state, &live_hooks);
}
} // namespace unported_ledger_stubs

// FUNCTION: LEGORR 0x0046f550
void simtick_sfx_bank(EngineContext* c, float dt)
{
    int i;
    s_sfx_rels = 0;

    /* Ten projectile records: update only records with a spawned entity at
     * +0x2a4. The update owner performs the scene-resource movement from
     * FUN_0046f920 using its retained full record. */
    if (s_sfx_bank_hooks.projectile_record_get != NULL) {
        for (i = 0; i < SIMTICK_SFX_BANK_SLOTS; ++i) {
            SimtickSfxProjectileRecord* record =
                s_sfx_bank_hooks.projectile_record_get(
                    i, s_sfx_bank_hooks.context);
            if (record != NULL && record->tracked_entity != NULL &&
                s_sfx_bank_hooks.projectile_update != NULL) {
                s_sfx_bank_hooks.projectile_update(record->owner_record, dt,
                                                   s_sfx_bank_hooks.context);
            }
        }
    }

    /* LAB_0046f610 is called through the unfiltered entity-pool walk between
     * projectile movement and per-record hit scans. The checked-in full
     * decompile contains no body for that label, so its narrow callback stays
     * optional while the live pool walk and surrounding order are retained. */
    (void)c;
    entity_pool_find_by_callback_current(sfx_entity_scan_callback, &dt, 0);

    /* The hit scanner runs for all ten records, including empty records; the
     * retail helper itself gates on the record's +0x2a4 entity pointer. */
    if (s_sfx_bank_hooks.projectile_record_get != NULL) {
        for (i = 0; i < SIMTICK_SFX_BANK_SLOTS; ++i) {
            SimtickSfxProjectileRecord* record =
                s_sfx_bank_hooks.projectile_record_get(
                    i, s_sfx_bank_hooks.context);
            if (record != NULL && s_sfx_bank_hooks.ray_hit_scan != NULL)
                s_sfx_bank_hooks.ray_hit_scan(record->owner_record,
                                              s_sfx_bank_hooks.context);
        }
    }

    /* Ten 0x10 timed-visual records: negative timers release both handles,
     * clear the first handle as the occupancy sentinel, then every active
     * timer is decremented (including one just released). */
    if (s_sfx_bank_hooks.timed_visual_record_get != NULL) {
        for (i = 0; i < SIMTICK_SFX_BANK_SLOTS; ++i) {
            SimtickSfxTimedVisualRecord* record =
                s_sfx_bank_hooks.timed_visual_record_get(
                    i, s_sfx_bank_hooks.context);
            if (record == NULL || record->first_handle == NULL) continue;
            if (record->remaining < 0.0f) {
                if (s_sfx_bank_hooks.resource_release != NULL) {
                    s_sfx_bank_hooks.resource_release(record->first_handle,
                        record->resource_owner, s_sfx_bank_hooks.context);
                    s_sfx_bank_hooks.resource_release(record->second_handle,
                        record->resource_owner, s_sfx_bank_hooks.context);
                }
                record->first_handle = NULL;
                ++s_sfx_rels;
            }
            record->remaining -= dt;
        }
    } else {
        /* Keep the small raw timer fixture available to existing simtick
         * tests; production records use the typed bank hooks above. */
        for (i = 0; i < s_sfx_n; ++i) {
            float* remaining = (float*)((uint8_t*)s_sfx_entries[i] + 12);
            if (*remaining < 0.0f) {
                ++s_sfx_rels;
                *(uint32_t*)((uint8_t*)s_sfx_entries[i]) = 0;
            }
            *remaining -= dt;
        }
    }
}

// FUNCTION: LEGORR 0x0046f770
void effect_apply_by_kind_branch(void* entity, const float direction_xy[2],
                                 const float effect_position[3],
                                 uint32_t type, int mode)
{
    EntitySlot* target = (EntitySlot*)entity;
    uint32_t flags;
    float direction[2];
    if (entity == NULL || direction_xy == NULL) return;
    direction[0] = direction_xy[0];
    direction[1] = direction_xy[1];
    if (entity_type3_flag04000000_test(target) != 0 &&
        effect_position != NULL) {
        scene_light_create_at_position(effect_position);
    }
    flags = entity_state_flags_get(target);
    if (mode == 1) {
        if ((flags & 0x02000000u) != 0) {
            entity_apply_impulse_and_direction(
                entity, entity_stats_float_at11c(target), 1u);
        }
        if (effect_position != NULL)
            (void)lava_effect_select_and_place(0, effect_position, NULL);
    }
    else if (mode == 2) {
        if ((flags & 0x04000000u) != 0) {
            float amount = entity_stats_float_at118(target);
            float scalar = entity_stats_float_at114(target);
            int32_t pair_words[2];
            int32_t scalar_word;
            direction[0] *= 1.5f;
            direction[1] *= 1.5f;
            entity_apply_impulse_and_direction(entity, amount, 1u);
            memcpy(pair_words, direction, sizeof(pair_words));
            memcpy(&scalar_word, &scalar, sizeof(scalar_word));
            (void)entity_pair_scalar_fields_set(target, pair_words,
                                                scalar_word);
        }
        if (effect_position != NULL)
            (void)lava_effect_select_and_place(1, effect_position, NULL);
    }
    else if (mode == 3) {
        if ((flags & 0x01000000u) != 0) {
            float duration = entity_stats_float_at10c(target);
            float amount = entity_stats_float_at110(target);
            if (duration <= 0.0f) duration = 10.0f;
            entity_apply_impulse_and_direction(entity, amount, 0u);
            (void)entity_one_shot_attached_effect_start(target, duration);
        }
        if (effect_position != NULL)
            (void)lava_effect_select_and_place(2, effect_position, NULL);
    }
    else if (s_effect_apply_hooks.generic_motion_apply != NULL) {
        s_effect_apply_hooks.generic_motion_apply(s_effect_apply_hooks.context,
                                                 entity, type, direction);
    }
}
// FUNCTION: LEGORR 0x0046faf0
int entity_spawn_record_slot_init(void* owner, const float position[3],
                                  const float direction[3],
                                  uint32_t auxiliary, uint32_t mode)
{
    uint32_t type, flags;
    int slot;
    unsigned int i;
    ActiveEffectRecord* record;
    /* Retail performs this owner/mode preparation before resolving the
     * surface pair or testing its table pointer. Use the injected callback
     * when present; otherwise run the live queued-grid animation path that
     * implements FUN_00437300(owner, auxiliary, 1.0). */
    if (mode == 1) {
        if (s_active_effect_build_hooks.pre_spawn_prepare != NULL) {
            s_active_effect_build_hooks.pre_spawn_prepare(
                s_active_effect_build_hooks.context, owner, auxiliary);
        }
        else {
            process_queued_grid_animations(owner, (int)auxiliary, 1.0f);
        }
    }
    if (position == NULL || direction == NULL ||
        !active_effect_type_resolve(mode, &type, &flags)) return 0;
    if (s_active_effect_build_hooks.surface_type_exists != NULL &&
        !s_active_effect_build_hooks.surface_type_exists(
            s_active_effect_build_hooks.context, type, flags)) return 0;
    slot = active_effect_record_slot_find();
    if (slot < 0) return 0;
    record = &s_active_effect_records[slot];
    memset(record, 0, sizeof(*record));
    record->auxiliary = auxiliary;
    record->mode = mode;
    record->owner = owner;
    record->speed = 5.0f;
    for (i = 0; i < 3; ++i) {
        record->position[i] = position[i];
        record->previous_position[i] = position[i];
        record->direction[i] = direction[i];
    }
    active_effect_direction_normalize(record->direction);
    if (s_active_effect_build_hooks.entity_spawn != NULL)
        record->spawned_entity = s_active_effect_build_hooks.entity_spawn(
            s_active_effect_build_hooks.context, type, flags, position);
    record->initialized = 0;
    active_effect_record_slot_set(slot,
                                  record->spawned_entity != NULL);
    if (s_active_effect_build_hooks.record_activate != NULL)
        s_active_effect_build_hooks.record_activate(
            s_active_effect_build_hooks.context, record, slot);
    return 1;
}

// FUNCTION: LEGORR 0x0046fc70
int object_flight_curve_record_init(void* owner, const float start[3],
                                    const float end[3], uint32_t auxiliary,
                                    uint32_t mode)
{
    uint32_t type, flags;
    int slot;
    unsigned int i;
    float distance;
    ActiveEffectRecord* record;
    if (start == NULL || end == NULL ||
        !active_effect_type_resolve(mode, &type, &flags)) return 0;
    if (s_active_effect_build_hooks.surface_type_exists != NULL &&
        !s_active_effect_build_hooks.surface_type_exists(
            s_active_effect_build_hooks.context, type, flags)) return 0;
    slot = active_effect_record_slot_find();
    if (slot < 0) return 0;
    record = &s_active_effect_records[slot];
    memset(record, 0, sizeof(*record));
    record->mode = mode;
    record->auxiliary = auxiliary;
    record->owner = owner;
    record->initialized = 1;
    record->speed = 4.0f;
    record->curve[0] = 0.3f;
    record->curve[1] = 1.0f;
    record->curve[2] = 1.0f;
    record->curve[3] = -1.3f;
    for (i = 0; i < 3; ++i) {
        record->position[i] = start[i];
        record->previous_position[i] = start[i];
        record->curve_origin[i] = start[i];
        record->direction[i] = end[i] - start[i];
    }
    active_effect_direction_normalize(record->direction);
    distance = (float)sqrt((end[0] - start[0]) * (end[0] - start[0]) +
                           (end[1] - start[1]) * (end[1] - start[1]) +
                           (end[2] - start[2]) * (end[2] - start[2]));
    record->duration = distance;
    active_effect_curve_samples_build(record, end);
    if (s_active_effect_build_hooks.flight_curve_build != NULL) {
        record->duration = s_active_effect_build_hooks.flight_curve_build(
            s_active_effect_build_hooks.context, record, end);
    }
    else {
        record->duration = record->curve_total_length;
    }
    if (s_active_effect_build_hooks.entity_spawn != NULL)
        record->spawned_entity = s_active_effect_build_hooks.entity_spawn(
            s_active_effect_build_hooks.context, type, flags, start);
    active_effect_record_slot_set(slot,
                                  record->spawned_entity != NULL);
    if (s_active_effect_build_hooks.record_activate != NULL)
        s_active_effect_build_hooks.record_activate(
            s_active_effect_build_hooks.context, record, slot);
    return 1;
}
// FUNCTION: LEGORR 0x0046fef0
void effect_mode_surface_query_type_resolve(uint32_t mode,
                                            uint32_t* surface_type,
                                            uint32_t* flags)
{
    switch (mode) {
    case 1: *surface_type = 0x13; *flags = 0; break;
    case 2: *surface_type = 0x10; *flags = 0; break;
    case 3: *surface_type = 0x11; *flags = 0; break;
    case 4: *surface_type = 5;    *flags = 0; break;
    default: break;
    }
}

// FUNCTION: LEGORR 0x0046ff70
int active_effect_record_slot_find(void)
{
    int i;
    for (i = 0; i < 10; ++i) {
        if (s_active_effect_occupied[i] == 0 ||
            s_active_effect_records[i].spawned_entity == NULL) return i;
    }
    return -1;
}

// FUNCTION: LEGORR 0x0046ff90
int active_segment_visual_record_slot_find(void)
{
    int i;
    for (i = 0; i < 10; ++i) {
        if (s_active_segment_visual_occupied[i] == 0 ||
            s_timed_segment_visuals[i][0] == NULL) return i;
    }
    return -1;
}

namespace unported_ledger_stubs {
// LaserSpatialSoundAndImpact
// FUNCTION: LEGORR 0x0046ffb0
void* ledger_stub_0046ffb0_LaserSpatialSoundAndImpact(
    void* request, int kind, float scale, const float origin[3],
    const float direction[3], const LaserImpactHooks* hooks)
{
    return laser_spatial_sound_and_impact(request, kind, scale, origin,
                                           direction, hooks);
}

// EntitySurfaceSegmentCollisionCheck
// FUNCTION: LEGORR 0x00470260
void ledger_stub_00470260_EntitySurfaceSegmentCollisionCheck(
    EntitySlot* entity, const float start_xy[2], const float end_xy[2],
    int check_side)
{
    if ((entity_state_flags_get(entity) & 0x100u) != 0u) {
        (void)ledger_stub_004702b0_EntitySurfaceQuadSegmentIntersect(
            entity, start_xy, end_xy, NULL, 0);
        return;
    }
    (void)entity_segment_center_proximity_check(entity, start_xy, end_xy,
                                               check_side);
}

// EntitySurfaceQuadSegmentIntersect
// FUNCTION: LEGORR 0x004702b0
int ledger_stub_004702b0_EntitySurfaceQuadSegmentIntersect(
    EntitySlot* entity, const float start_xy[2], const float end_xy[2],
    float out_xy[2], int check_side)
{
    return entity_surface_quad_segment_intersect(entity, start_xy, end_xy,
        out_xy, check_side, &s_entity_quad_collision_hooks);
}
} // namespace unported_ledger_stubs

// FUNCTION: LEGORR 0x00470540
int entity_segment_center_proximity_check(const EntitySlot* entity,
                                          const float start_xy[2],
                                          const float end_xy[2],
                                          int check_side)
{
    float line_x;
    float line_y;
    float inv_length;
    float projection;
    float projected_x;
    float projected_y;
    float entity_position[2];
    float dx;
    float dy;
    float radius;
    if (entity == NULL || start_xy == NULL || end_xy == NULL) return 0;
    if (check_side && entity_directed_line_side_check(entity, start_xy, end_xy))
        return 0;
    entity_transform_position_get(entity, entity_position);
    line_x = (end_xy[0] + start_xy[0]) - start_xy[0];
    line_y = (end_xy[1] + start_xy[1]) - start_xy[1];
    inv_length = 1.0f / (float)sqrt(line_x * line_x + line_y * line_y);
    line_x *= inv_length;
    line_y *= inv_length;
    projection = (entity_position[0] - start_xy[0]) * line_x +
                 (entity_position[1] - start_xy[1]) * line_y;
    projected_x = start_xy[0] + projection * line_x;
    projected_y = start_xy[1] + projection * line_y;
    dx = projected_x - entity_position[0];
    dy = projected_y - entity_position[1];
    radius = s_entity_segment_radius_hook != NULL
        ? s_entity_segment_radius_hook(entity)
        : entity_stats_float_at6c(entity);
    if (radius <= (float)sqrt(dx * dx + dy * dy)) return 0;
    return 1;
}
// FUNCTION: LEGORR 0x00470630
int entity_directed_line_side_check(const EntitySlot* entity,
                                    const float point_xy[2],
                                    const float direction_xy[2])
{
    float entity_position[2];
    if (entity == NULL || point_xy == NULL || direction_xy == NULL) return 0;
    entity_transform_position_get(entity, entity_position);
    return ((entity_position[0] - point_xy[0]) * direction_xy[0] +
            (entity_position[1] - point_xy[1]) * direction_xy[1]) < 0.0f;
}

// FUNCTION: LEGORR 0x00470690
void timed_entity_segment_visual_create(void* entity, const float start[3],
                                       const float end[3])
{
    int slot = active_segment_visual_record_slot_find();
    void* scene;
    if (slot < 0 || s_timed_segment_visual_hooks.scene_get == NULL ||
        s_timed_segment_visual_hooks.visual_create == NULL ||
        s_timed_segment_visual_hooks.segment_build == NULL) return;
    scene = s_timed_segment_visual_hooks.scene_get(entity);
    s_timed_segment_visuals[slot][0] =
        s_timed_segment_visual_hooks.visual_create(scene, 0x4400u);
    s_timed_segment_visuals[slot][1] =
        s_timed_segment_visual_hooks.visual_create(scene, 0x4400u);
    s_timed_segment_visual_hooks.segment_build(s_timed_segment_visuals[slot][0],
                                               0.2f, start, end,
                                               0.6f, 0.6f, 0.6f, 1.0f);
    s_timed_segment_visual_hooks.segment_build(s_timed_segment_visuals[slot][1],
                                               1.0f, start, end,
                                               0.1f, 0.2f, 0.5f, 1.0f);
    s_timed_segment_visual_owners[slot] = entity;
    s_timed_segment_tick_records[slot].first_handle =
        s_timed_segment_visuals[slot][0];
    s_timed_segment_tick_records[slot].second_handle =
        s_timed_segment_visuals[slot][1];
    s_timed_segment_tick_records[slot].resource_owner = scene;
    s_timed_segment_tick_records[slot].remaining = 5.0f;
    active_segment_visual_record_slot_set(slot, 1);
}

// FUNCTION: LEGORR 0x00470760
void mesh_build_eight_sided_segment(void* mesh, float radius,
                                    const float start[3], const float end[3],
                                    float red, float green, float blue,
                                    float alpha)
{
    static const uint16_t triangle_indices[48] = {
        0, 1, 9, 9, 8, 0, 1, 2, 10, 10, 9, 1,
        2, 3, 11, 11, 10, 2, 3, 4, 12, 12, 11, 3,
        4, 5, 13, 13, 12, 4, 5, 6, 14, 14, 13, 5,
        6, 7, 15, 15, 14, 6, 7, 0, 8, 8, 15, 7
    };
    static const uint32_t texture_pairs[32] = {
        0u, 0x3f800000u, 0u, 0u, 0x3f800000u, 0u, 0x3f800000u, 0x3f800000u,
        0u, 0x3f800000u, 0u, 0u, 0x3f800000u, 0u, 0x3f800000u, 0x3f800000u,
        0u, 0x3f800000u, 0u, 0u, 0x3f800000u, 0u, 0x3f800000u, 0x3f800000u,
        0u, 0x3f800000u, 0u, 0u, 0x3f800000u, 0u, 0x3f800000u, 0x3f800000u
    };
    uint16_t source_indices[96];
    float positions[48];
    float normals[24];
    float dx;
    float dy;
    float dz;
    float inverse_length;
    float axis_y;
    float axis_z;
    float frame_x;
    float frame_y;
    float off_x[8];
    float off_y[8];
    float off_z[8];
    float f6;
    float f9;
    float f5;
    float f8;
    float fparam;
    float diagonal_x;
    float diagonal_y;
    float diagonal_z;
    float diagonal_r;
    float diagonal_f9;
    float normal_length;
    int record_index;
    int i;
    if (mesh == NULL || start == NULL || end == NULL) return;

    dx = end[0] - start[0];
    dy = end[1] - start[1];
    dz = end[2] - start[2];
    /* Keep the decompile's component evaluation order (z, y, x). */
    inverse_length = 1.0f / (float)sqrt(dz * dz + dy * dy + dx * dx);
    axis_y = inverse_length * dy;
    axis_z = inverse_length * dz;
    if (axis_y == 0.0f && axis_z == 0.0f) {
        frame_x = 1.0f;
        frame_y = 0.0f;
    }
    else if (axis_y == 0.0f) {
        frame_y = 1.0f;
        frame_x = 0.0f;
    }
    else {
        frame_x = (float)sqrt(1.0f / ((axis_z * axis_z) /
                                      (axis_y * axis_y) + 1.0f));
        frame_y = -(axis_z / axis_y) * frame_x;
    }

    f6 = frame_y * radius;
    f9 = frame_x * radius;
    f5 = (frame_x * axis_y - frame_y * axis_z) * radius;
    f8 = -(frame_x * inverse_length * dx) * radius;
    fparam = frame_y * inverse_length * dx * radius;
    diagonal_x = f6 * 0.41421354f;
    diagonal_y = f5 * 0.41421354f;
    diagonal_z = f8 * 0.41421354f;
    diagonal_r = fparam * 0.41421354f;
    diagonal_f9 = f9 * 0.41421354f;

    off_x[0] = diagonal_y; off_y[0] = f6 + diagonal_z; off_z[0] = f9 + diagonal_r;
    off_x[1] = f5; off_y[1] = f8 + diagonal_x; off_z[1] = fparam + diagonal_f9;
    off_x[2] = f5; off_y[2] = f8 - diagonal_x; off_z[2] = fparam - diagonal_f9;
    off_x[3] = diagonal_y; off_y[3] = diagonal_z - f6; off_z[3] = diagonal_r - f9;
    off_x[4] = -diagonal_y; off_y[4] = -f6 - diagonal_z; off_z[4] = -f9 - diagonal_r;
    off_x[5] = -f5; off_y[5] = -diagonal_x - f8; off_z[5] = -diagonal_f9 - fparam;
    off_x[6] = -f5; off_y[6] = diagonal_x - f8; off_z[6] = diagonal_f9 - fparam;
    off_x[7] = -diagonal_y; off_y[7] = f6 - diagonal_z; off_z[7] = f9 - diagonal_r;

    for (i = 0; i < 8; ++i) {
        float nx;
        float ny;
        float nz;
        int vertex = i * 3;
        positions[vertex] = start[0] + off_x[i];
        positions[vertex + 1] = start[1] + off_y[i];
        positions[vertex + 2] = start[2] + off_z[i];
        positions[24 + vertex] = end[0] + off_x[i];
        positions[24 + vertex + 1] = end[1] + off_y[i];
        positions[24 + vertex + 2] = end[2] + off_z[i];
        nx = off_x[i];
        ny = off_y[i];
        nz = off_z[i];
        normal_length = (float)sqrt(nx * nx + nz * nz + ny * ny);
        normals[vertex] = nx / normal_length;
        normals[vertex + 1] = ny / normal_length;
        normals[vertex + 2] = nz / normal_length;
    }
    for (i = 0; i < 48; ++i) {
        source_indices[i * 2] = triangle_indices[i];
        source_indices[i * 2 + 1] = 0;
    }

    record_index = mesh_record_allocate_or_append(mesh, 16u, 16u, 3u,
                                                   source_indices);
    if (record_index < 0) return;
    (void)render_object_color_vector_set(mesh, record_index, red, green, blue, 3);
    (void)screen_effect_item_opacity_set(mesh, record_index, alpha, 4);
    mesh_segment_vertex_fill(mesh, record_index, 0, 16, positions,
                             (const uint32_t*)normals, texture_pairs);
}

/* EntityVelocityStateRecordGet (FUN_004712c0): the x86 body mirrors the
 * unchecked type decrement/subtract and embedded-base returns. The portable
 * path retains the NULL record/link guards pinned by test_record. */
// FUNCTION: LEGORR 0x004712c0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked)
void* entity_velocity_state_record_get(const void* record)
{
    __asm {
        mov ecx, dword ptr [esp + 4]
        mov eax, dword ptr [ecx]
        dec eax
        je velocity_type_one
        sub eax, 3
        je velocity_type_four
        xor eax, eax
        ret
    velocity_type_four:
        mov eax, dword ptr [ecx + 18h]
        add eax, 0a0h
        ret
    velocity_type_one:
        mov eax, dword ptr [ecx + 0ch]
        add eax, 138h
        ret
    }
}
#else
void* entity_velocity_state_record_get(const void* record)
{
    if (record == NULL) {
        return NULL; /* deviation: the corpus strides the record blindly */
    }
    uint32_t type = *(const uint32_t*)record;
    type -= 1u;
    if (type == 0u) {
        void* sub = *(void* const*)((const char*)record + 0x0c);
        return (sub != NULL) ? (void*)((char*)sub + 0x138)
                             : NULL; /* deviation: corpus strides blindly */
    }
    type -= 3u;
    if (type == 0u) {
        void* sub = *(void* const*)((const char*)record + 0x18);
        return (sub != NULL) ? (void*)((char*)sub + 0xa0)
                             : NULL; /* deviation: corpus strides blindly */
    }
    return NULL; /* deviation: the corpus walks a null base */
}
#endif

/* EntityVelocitySlotValueGet (FUN_004712f0): the x86 body reads the float
 * at +0x98 + slot*4 without a null or slot guard. The portable path keeps
 * its guarded fallback. */
// FUNCTION: LEGORR 0x004712f0
float entity_velocity_slot_value_get(const void* record, int slot)
{
    void* base = entity_velocity_state_record_get(record);
#if defined(_MSC_VER) && defined(_M_IX86)
    return *(const float*)((const char*)base + 0x98u + slot * 4);
#else
    if (base == NULL || slot < 0) {
        return 0.0f; /* deviation */
    }
    return rec_float(base, 0x98u + (size_t)slot * 4u);
#endif
}

/* EntityVelocitySlotValueSet (FUN_00471310): the x86 body stores the raw
 * value word at +0x98 + slot*4 without guards. */
// FUNCTION: LEGORR 0x00471310
void entity_velocity_slot_value_set(void* record, float value, int slot)
{
    void* base = entity_velocity_state_record_get(record);
#if defined(_MSC_VER) && defined(_M_IX86)
    *(float*)((char*)base + 0x98u + slot * 4) = value;
#else
    if (base == NULL || slot < 0) {
        return; /* deviation */
    }
    __builtin_memcpy((char*)base + 0x98u + (size_t)slot * 4u,
                     &value, sizeof value);
#endif
}

/* InterpolatedZWithinBounds (FUN_00471330): the corpus loads the +8 floats
 * directly in b, a order, then interpolates before `v <= target` and
 * `target - t <= v`. Keep rec_float's NULL-to-zero behavior without helper
 * calls so the native-width loads follow that operand schedule. */
// FUNCTION: LEGORR 0x00471330
int interpolated_z_within_bounds(const void* record_a, const void* record_b,
                                 float t, float target, float scale)
{
    const float v = rec_float(record_b, 8) * scale + rec_float(record_a, 8);
    if (v <= target && target - t <= v) {
        return 1;
    }
    return 0;
}

/* LAB_00471370: the scene callback receives each live entity and the ray
 * record. It skips the ray source, projects the entity center onto the
 * horizontal ray, tests the Stats radius at +0x6c, then checks the ray's Z
 * against the terrain-to-Stats-height interval (Stats +0x74). The callback
 * always returns zero so the pool walker examines every candidate. The ray
 * uses the game's logical [X,Y,height] basis; terrain geometry stores the
 * same horizontal axes at vertex components 0/2 and height at component 1. */
// FUNCTION: LEGORR 0x00471370
int cmd_place_ray_candidate_check(EntitySlot* entity, void* payload)
{
    CmdRayCandidateWalk* walk = (CmdRayCandidateWalk*)payload;
    EntityRayHitSweepState* state;
    float candidate_xy[2];
    float dx, dy, dz;
    float horizontal_length;
    float along;
    float projected_x, projected_y;
    float perp_x, perp_y;
    float radius;
    float height;
    float ground;
    float entry_distance;
    float entry_fraction;
    if (walk == NULL || (state = walk->state) == NULL || entity == NULL ||
        entity == state->ignored_object || walk->game == NULL ||
        walk->game->level == NULL ||
        walk->game->level->ptr_terrain == NULL || state->start == NULL ||
        state->direction == NULL) return 0;

    dx = state->direction[0];
    dy = state->direction[1];
    dz = state->direction[2];
    horizontal_length = (float)sqrt(dx * dx + dy * dy);
    if (horizontal_length <= 0.00001f) return 0;

    entity_transform_position_get(entity, candidate_xy);
    along = (candidate_xy[0] - state->start[0]) * dx / horizontal_length +
            (candidate_xy[1] - state->start[1]) * dy / horizontal_length;
    projected_x = state->start[0] + along * dx / horizontal_length;
    projected_y = state->start[1] + along * dy / horizontal_length;
    perp_x = candidate_xy[0] - projected_x;
    perp_y = candidate_xy[1] - projected_y;
    radius = entity_stats_float_at6c(entity);
    if (radius <= 0.0f || perp_x * perp_x + perp_y * perp_y >=
        radius * radius) return 0;

    entry_distance = along - (float)sqrt(
        radius * radius - perp_x * perp_x - perp_y * perp_y);
    /* A segment that starts inside the candidate's radius reports an
     * immediate hit, after the same vertical interval check at t = 0. */
    entry_fraction = entry_distance > 0.0f
        ? entry_distance / horizontal_length : 0.0f;
    if (entry_fraction >= state->max_distance)
        return 0;

    height = entity_stats_float_at74(entity);
    ground = terrain_surface_height_interpolate(
        walk->game->level->ptr_terrain, candidate_xy[0], candidate_xy[1]);
    /* 0x44f940 returns the terrain quad's vertical component directly.
     * Scene rays store that same vertical value in component +2; components
     * 0/1 are the horizontal pair corresponding to terrain X/Z. */
    if (height <= 0.0f ||
        state->start[2] + dz * entry_fraction > ground + height ||
        state->start[2] + dz * entry_fraction < ground) return 0;

    state->hit_object = entity;
    state->max_distance = entry_fraction;
    state->hit = 1;
    return 0;
}

/* RayHitCellDistanceMinimizeSweep (FUN_00471630): enumerate the grid cells
 * touched by the start/end segment and retain the nearest reported hit. */
// FUNCTION: LEGORR 0x00471630
void ray_hit_cell_distance_minimize_sweep(EntityRayHitSweepState* state)
{
    uint32_t width = 0u;
    uint32_t height = 0u;
    uint32_t start_x = 0u;
    uint32_t start_y = 0u;
    uint32_t end_x = 0u;
    uint32_t end_y = 0u;
    uint32_t min_x;
    uint32_t min_y;
    uint32_t max_x;
    uint32_t max_y;
    uint32_t x;
    uint32_t y;
    float endpoint[3];
    float direction_length;
    if (state == NULL || state->start == NULL || state->direction == NULL ||
        s_entity_ray_hit_sweep_ops.grid_dimensions_get == NULL ||
        s_entity_ray_hit_sweep_ops.world_to_cell == NULL ||
        s_entity_ray_hit_sweep_ops.cell_intersection == NULL ||
        s_entity_ray_hit_sweep_ops.grid_dimensions_get(
            s_entity_ray_hit_sweep_ops.context, &width, &height) == 0 ||
        width == 0u || height == 0u) return;
    for (x = 0u; x < 3u; ++x) {
        endpoint[x] = state->start[x] + state->direction[x];
    }
    if (!s_entity_ray_hit_sweep_ops.world_to_cell(
            s_entity_ray_hit_sweep_ops.context, state->start,
            &start_x, &start_y)) {
        if (start_x >= width) start_x = width - 1u;
        if (start_y >= height) start_y = height - 1u;
    }
    if (!s_entity_ray_hit_sweep_ops.world_to_cell(
            s_entity_ray_hit_sweep_ops.context, endpoint, &end_x, &end_y)) {
        if (end_x >= width) end_x = width - 1u;
        if (end_y >= height) end_y = height - 1u;
    }
    if (start_x >= width) start_x = width - 1u;
    if (start_y >= height) start_y = height - 1u;
    if (end_x >= width) end_x = width - 1u;
    if (end_y >= height) end_y = height - 1u;
    min_x = (start_x < end_x) ? start_x : end_x;
    max_x = (start_x < end_x) ? end_x : start_x;
    min_y = (start_y < end_y) ? start_y : end_y;
    max_y = (start_y < end_y) ? end_y : start_y;
    direction_length = (float)sqrt(
        state->direction[0] * state->direction[0] +
        state->direction[1] * state->direction[1] +
        state->direction[2] * state->direction[2]);
    if (direction_length <= 0.0f) return;
    for (x = min_x; x <= max_x; ++x) {
        for (y = min_y; y <= max_y; ++y) {
            float hit_point[3];
            float dx;
            float dy;
            float dz;
            float distance;
            if (!s_entity_ray_hit_sweep_ops.cell_intersection(
                    s_entity_ray_hit_sweep_ops.context, x, y,
                    state->start, state->direction, hit_point)) continue;
            dx = hit_point[0] - state->start[0];
            dy = hit_point[1] - state->start[1];
            dz = hit_point[2] - state->start[2];
            distance = (float)sqrt(dx * dx + dy * dy + dz * dz) /
                       direction_length;
            if (distance < state->max_distance) {
                state->max_distance = distance;
                state->hit = 1;
                state->hit_object = NULL;
            }
        }
    }
}

/* SceneSegmentTraceNearestHit (FUN_00471860): initialize the trace record,
 * let the scene walk populate its candidate context, then minimize against
 * grid-cell ray hits and publish the nearest result. */
// FUNCTION: LEGORR 0x00471860
int scene_segment_trace_nearest_hit(const float start[3],
                                   const float direction[3],
                                   void** hit_object_out,
                                   float* max_distance_io,
                                   void* ignored_object)
{
    EntityRayHitSweepState state;
    if (hit_object_out != NULL) *hit_object_out = NULL;
    if (start == NULL || direction == NULL || max_distance_io == NULL) return 0;
    state.hit_object = NULL;
    state.max_distance = *max_distance_io;
    state.start = start;
    state.direction = direction;
    state.hit = 0;
    state.ignored_object = ignored_object;
    if (s_entity_ray_hit_sweep_ops.scene_trace_prepare != NULL) {
        s_entity_ray_hit_sweep_ops.scene_trace_prepare(
            s_entity_ray_hit_sweep_ops.context, &state);
    }
    ray_hit_cell_distance_minimize_sweep(&state);
    *max_distance_io = state.max_distance;
    if (hit_object_out != NULL) *hit_object_out = state.hit_object;
    return state.hit;
}

/* ObjectPairRayHitCheck (FUN_004718d0): trace the normalized segment from
 * the first object to the second and ignore a hit reported on the target. */
// FUNCTION: LEGORR 0x004718d0
int object_pair_ray_hit_check(void* first, void* second)
{
    float start[3];
    float target[3];
    float direction[3];
    float max_distance = 1.0f;
    void* hit_object = NULL;
    int index;
    int hit;
    if (first == NULL || second == NULL ||
        s_entity_ray_hit_sweep_ops.object_position_get == NULL ||
        !s_entity_ray_hit_sweep_ops.object_position_get(
            s_entity_ray_hit_sweep_ops.context, first, start) ||
        !s_entity_ray_hit_sweep_ops.object_position_get(
            s_entity_ray_hit_sweep_ops.context, second, target)) return 0;
    for (index = 0; index < 3; ++index) {
        direction[index] = target[index] - start[index];
    }
    hit = scene_segment_trace_nearest_hit(
        start, direction, &hit_object, &max_distance, first);
    return (hit != 0 && hit_object != second) ? 1 : 0;
}

/* SeeThroughWallsRayCheck (FUN_00471960): short-circuit when the setting is
 * enabled; otherwise sweep the object-to-object segment through grid cells. */
// FUNCTION: LEGORR 0x00471960
int see_through_walls_ray_check(void* first, void* second)
{
    float start[3];
    float target[3];
    float direction[3];
    EntityRayHitSweepState state;
    int index;
    if (s_entity_ray_hit_sweep_ops.see_through_walls_enabled != NULL &&
        s_entity_ray_hit_sweep_ops.see_through_walls_enabled(
            s_entity_ray_hit_sweep_ops.context) != 0) return 0;
    if (first == NULL || second == NULL ||
        s_entity_ray_hit_sweep_ops.object_position_get == NULL ||
        !s_entity_ray_hit_sweep_ops.object_position_get(
            s_entity_ray_hit_sweep_ops.context, first, start) ||
        !s_entity_ray_hit_sweep_ops.object_position_get(
            s_entity_ray_hit_sweep_ops.context, second, target)) return 0;
    for (index = 0; index < 3; ++index) {
        direction[index] = target[index] - start[index];
    }
    state.hit_object = NULL;
    state.max_distance = 1.0f;
    state.start = start;
    state.direction = direction;
    state.hit = 0;
    state.ignored_object = NULL;
    ray_hit_cell_distance_minimize_sweep(&state);
    return state.hit;
}

/* ReadIndexedPairSlotDword (FUN_00471a20): base + i*8 + j*4, exactly as
 * the corpus computes it. */
// FUNCTION: LEGORR 0x00471a20
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) uint32_t read_indexed_pair_slot_dword(
    const void* base, uint32_t i, uint32_t j)
{
    __asm {
        mov eax, dword ptr [esp + 8]
        mov ecx, dword ptr [esp + 4]
        lea edx, dword ptr [ecx + eax*8]
        mov eax, dword ptr [esp + 0ch]
        mov eax, dword ptr [edx + eax*4]
        ret
    }
}
#else
uint32_t read_indexed_pair_slot_dword(const void* base, uint32_t i,
                                      uint32_t j)
{
    uint32_t value;
    const size_t offset = (size_t)i * 8u + (size_t)j * 4u;
    __builtin_memcpy(&value, (const char*)base + offset, sizeof(value));
    return value;
}
#endif

/* VelocitySlotPrimaryObjectGet / SecondaryObjectGet (FUN_00471a40 /
 * FUN_00471a50): indexed slot at +0x18 / +0x24. */
// FUNCTION: LEGORR 0x00471a40
uint32_t velocity_slot_primary_object_get(const void* record,
                                          uint32_t index)
{
    uint32_t value;
    __builtin_memcpy(&value, (const char*)record + 0x18u +
                     (size_t)index * 4u, sizeof(value));
    return value;
}

// FUNCTION: LEGORR 0x00471a50
uint32_t velocity_slot_secondary_object_get(const void* record,
                                            uint32_t index)
{
    uint32_t value;
    __builtin_memcpy(&value, (const char*)record + 0x24u +
                     (size_t)index * 4u, sizeof(value));
    return value;
}

/* EntityVelocitySteerBlendUpdate (FUN_00471a60): move the target when asked,
 * blend the existing velocity toward it at the time-scaled rate, normalize,
 * cap upward speed, then apply to the secondary and primary objects. */
// FUNCTION: LEGORR 0x00471a60
void entity_velocity_steer_blend_update(EntityVelocitySteerState* state,
                                        const float target[3], float elapsed,
                                        int update_target)
{
    float position[3];
    float desired[3];
    float length;
    float blend;
    int axis;
    if (state == NULL) return;
    if (update_target != 0 && target != NULL) {
        for (axis = 0; axis < 3; ++axis) state->target[axis] = target[axis];
    }
    if (state->primary == NULL || elapsed <= 0.00001f ||
        s_entity_ray_hit_sweep_ops.object_position_get == NULL ||
        !s_entity_ray_hit_sweep_ops.object_position_get(
            s_entity_ray_hit_sweep_ops.context, state->primary, position)) {
        return;
    }
    for (axis = 0; axis < 3; ++axis) {
        desired[axis] = state->target[axis] - position[axis];
    }
    length = (float)sqrt(desired[0] * desired[0] +
                         desired[1] * desired[1] +
                         desired[2] * desired[2]);
    if (length <= 0.0f) return;
    blend = 5.0f / elapsed;
    for (axis = 0; axis < 3; ++axis) {
        state->velocity[axis] = state->velocity[axis] * blend +
                                desired[axis] / length;
    }
    length = (float)sqrt(state->velocity[0] * state->velocity[0] +
                         state->velocity[1] * state->velocity[1] +
                         state->velocity[2] * state->velocity[2]);
    if (length > 0.0f) {
        for (axis = 0; axis < 3; ++axis) state->velocity[axis] /= length;
    }
    if (state->max_vertical_velocity < state->velocity[2]) {
        state->velocity[2] = state->max_vertical_velocity;
    }
    if (s_entity_steer_velocity_apply != NULL) {
        if (state->secondary != NULL) {
            float secondary_velocity[3];
            secondary_velocity[0] = state->velocity[0];
            secondary_velocity[1] = state->velocity[1];
            secondary_velocity[2] = 0.0f;
            s_entity_steer_velocity_apply(
                s_entity_steer_velocity_context, state->secondary,
                secondary_velocity, state->heading, 1);
        }
        s_entity_steer_velocity_apply(
            s_entity_steer_velocity_context, state->primary,
            state->velocity, state->heading, 0);
    }
}

/* VelocitySlotVectorRead (FUN_00471c70): copy the three slot dwords
 * directly, preserving their bit patterns. */
// FUNCTION: LEGORR 0x00471c70
void velocity_slot_vector_read(const void* record, float out[3],
                               uint32_t index)
{
    const size_t off = 0x44u + (size_t)index * 12u;
    __builtin_memcpy(out, (const char*)record + off, 3u * sizeof(uint32_t));
}

// FUNCTION: LEGORR 0x00471CA0
int entity_world_center_position_read(const EntitySlot* entity,
                                      float out_xyz[3])
{
    float position[3];
    if (entity == NULL || out_xyz == NULL) return 0;
    if (entity->attached_scene_object != NULL &&
        ddiscene_object_position_get(entity->attached_scene_object, NULL,
                                     position)) {
        out_xyz[0] = position[0];
        out_xyz[1] = position[1];
        out_xyz[2] = position[2];
    }
    else {
        out_xyz[0] = entity->x;
        out_xyz[1] = entity->y;
        out_xyz[2] = entity->z;
    }
    out_xyz[2] -= entity_stats_float_at74(entity) * 0.5f;
    return 1;
}

/* EntityFlag100AndSharedEligibilityCheck (FUN_00471ce0) — see entity.hpp. */
// FUNCTION: LEGORR 0x00471ce0
int entity_flag100_and_shared_eligibility_check(const EntitySlot* e)
{
    /* Corpus shape: both gates jump to the shared zero tail, only the
     * double pass answers 1. */
    if ((entity_state_flags_get(e) & 0x100u) != 0u) {
        if (entity_shared_eligibility_check(e, e->type != 4) != 0) {
            return 1;
        }
    }
    return 0;
}

namespace unported_ledger_stubs {
// VelocitySlotSteerDispatchAndSound
// FUNCTION: LEGORR 0x00471d20
int ledger_stub_00471d20_VelocitySlotSteerDispatchAndSound(
    void* entity, void* argument, float delta)
{
    return entity_velocity_steer_dispatch_and_sound(entity, argument, delta);
}
} // namespace unported_ledger_stubs

/* PoseOffsetVelocitySlotBlend (FUN_00471f00): convert the pose's local
 * offset to world scale, add the pose position, and feed that target through
 * the shared velocity steering blend. */
// FUNCTION: LEGORR 0x00471f00
int pose_offset_velocity_slot_blend_update(
    EntityVelocitySteerState* state, const float offset[3],
    const float translation[3], float elapsed)
{
    float target[3];
    int axis;
    if (state == NULL || offset == NULL || translation == NULL) return 1;
    for (axis = 0; axis < 3; ++axis) {
        target[axis] = offset[axis] * 10000.0f + translation[axis];
    }
    entity_velocity_steer_blend_update(state, target, elapsed, 1);
    return 1;
}

/* EntityVelocitySlotDispatch (FUN_00471fc0): walk every velocity slot and
 * choose the pose-offset path or the steer/sound path from entity bit 0. */
// FUNCTION: LEGORR 0x00471fc0
int entity_velocity_slot_dispatch(void* entity, void* argument)
{
    uint32_t flags = 0u;
    uint32_t count = 0u;
    uint32_t slot;
    if (entity == NULL || s_entity_velocity_slot_dispatch_ops.state_get == NULL ||
        !s_entity_velocity_slot_dispatch_ops.state_get(
            s_entity_velocity_slot_dispatch_ops.context, entity,
            &flags, &count)) return 1;
    for (slot = 0u; slot < count; ++slot) {
        if ((flags & 1u) != 0u) {
            if (s_entity_velocity_slot_dispatch_ops.steer_sound_dispatch != NULL) {
                s_entity_velocity_slot_dispatch_ops.steer_sound_dispatch(
                    s_entity_velocity_slot_dispatch_ops.context, entity,
                    argument, slot);
            }
        }
        else if (s_entity_velocity_slot_dispatch_ops.pose_slot_get != NULL) {
            EntityVelocitySteerState* state = NULL;
            float offset[3] = { 0.0f, 0.0f, 0.0f };
            float translation[3] = { 0.0f, 0.0f, 0.0f };
            float elapsed = 0.0f;
            if (s_entity_velocity_slot_dispatch_ops.pose_slot_get(
                    s_entity_velocity_slot_dispatch_ops.context, entity,
                    argument, slot, &state, offset, translation,
                    &elapsed)) {
                pose_offset_velocity_slot_blend_update(
                    state, offset, translation, elapsed);
            }
        }
    }
    return 1;
}

// FUNCTION: LEGORR 0x00472080
int entity_centers_within_configured_radius(const EntitySlot* first,
                                            const EntitySlot* second)
{
    int activity_class;
    if (first == NULL || second == NULL) return 0;
    activity_class = ai_activity_probe(first);
    if (activity_class == 0) return 0;
    return entity_centers_within_radius(first, second,
        lego::weapon_type_range_get(activity_class));
}

// FUNCTION: LEGORR 0x00472130
int entity_centers_within_object_radius(const EntitySlot* first,
                                        const EntitySlot* second)
{
    if (first == NULL || second == NULL) return 0;
    return entity_centers_within_radius(first, second,
                                        entity_collision_radius_get(first));
}
static int entity_centers_within_radius(const EntitySlot* first,
                                       const EntitySlot* second,
                                       float radius)
{
    float a[3];
    float b[3];
    float dx, dy, dz;
    if (first == NULL || second == NULL || radius <= 0.0f
        || !entity_world_center_position_read(first, a)
        || !entity_world_center_position_read(second, b)) return 0;
    dx = a[0] - b[0];
    dy = a[1] - b[1];
    dz = a[2] - b[2];
    return sqrtf(dx * dx + dy * dy + dz * dz) < radius;
}

static int active_effect_type_resolve(uint32_t mode, uint32_t* type,
                                      uint32_t* flags)
{
    *type = 0;
    *flags = 0;
    switch (mode) {
    case 1: *type = 0x13; break;
    case 2: *type = 0x10; break;
    case 3: *type = 0x11; break;
    case 4: *type = 5; break;
    default: return 0;
    }
    return 1;
}

static void active_effect_direction_normalize(float direction[3])
{
    float length = (float)sqrt(direction[0] * direction[0] +
                               direction[1] * direction[1] +
                               direction[2] * direction[2]);
    float inverse = 1.0f / length;
    direction[0] *= inverse;
    direction[1] *= inverse;
    direction[2] *= inverse;
}

/* FUN_0046fc70's path: a cubic in the vertical plane from the start to the
 * end, (horizontal distance, height). It leaves at (1, -1.3) scaled to a
 * fifth of the distance and arrives along (0.3, 1) scaled to a third. The
 * total length becomes the flight duration. record->duration holds the
 * straight-line distance on entry. */
static void active_effect_curve_samples_build(ActiveEffectRecord* record,
                                              const float endpoint[3])
{
    BezierCurve* path = active_effect_path(record);
    float distance = record->duration;
    Point2F p0, p1, p2, p3;
    p0.x = 0.0f;
    p0.y = 0.0f;
    p1.x = 1.0f;
    p1.y = -1.3f;
    p2.x = distance;
    p2.y = endpoint[2] - record->curve_origin[2];
    p3.x = 0.3f;
    p3.y = 1.0f;
    vector2_set_length(&p1, distance * 0.2f);
    vector2_set_length(&p3, distance * 0.33333334f);
    bezier_curve_samples_build(path, &p0, &p1, &p2, &p3, BEZIER_MAXPOINTS);
    record->curve_total_length = polyline_segment_lengths_total(path);
}

namespace unported_ledger_stubs {
static float combat_ray_configured_range_get(void*, int kind)
{
    return lego::weapon_type_range_get(kind);
}

static int combat_ray_scene_trace(void*, const float origin[3],
                                  const float direction[3],
                                  float* hit_fraction, float hit_point[3],
                                  void** hit_object_out)
{
    float distance;
    float nearest_distance;
    void* hit_object = NULL;
    int hit;
    if (hit_fraction == NULL || hit_point == NULL || origin == NULL ||
        direction == NULL) return 0;
    distance = (float)sqrt((double)(direction[0] * direction[0] +
                                    direction[1] * direction[1] +
                                    direction[2] * direction[2]));
    if (distance <= 0.0f) return 0;
    /* SceneSegmentTraceNearestHit bounds and returns distance in units of
     * the supplied direction vector (0 at origin, 1 at its endpoint). */
    nearest_distance = 1.0f;
    hit = scene_segment_trace_nearest_hit(origin, direction, &hit_object,
                                          &nearest_distance, NULL);
    if (hit == 0) return 0;
    *hit_fraction = nearest_distance;
    hit_point[0] = origin[0] + direction[0] * *hit_fraction;
    hit_point[1] = origin[1] + direction[1] * *hit_fraction;
    hit_point[2] = origin[2] + direction[2] * *hit_fraction;
    if (hit_object_out != NULL) *hit_object_out = hit_object;
    return 1;
}

static void combat_ray_entity_release(void*, void* entity)
{
    (void)entity_teardown((EntitySlot*)entity);
}
} // namespace unported_ledger_stubs

namespace lego {
namespace {
void weapon_definition_reset(WeaponDefinition* definition)
{
    unsigned int i;
    memset(definition, 0, sizeof(*definition));
    definition->range = 150.0f;
    for (i = 0; i < WEAPON_WALL_TYPE_COUNT; ++i)
        definition->wall_destroy_time[i] = 5.0f;
}

int ascii_lower(int ch)
{
    if (ch >= 'A' && ch <= 'Z') return ch + ('a' - 'A');
    return ch;
}

int ascii_case_equal(const char* a, const char* b)
{
    if (a == 0 || b == 0) return 0;
    while (*a != '\0' && *b != '\0') {
        if (ascii_lower((unsigned char)*a) != ascii_lower((unsigned char)*b))
            return 0;
        ++a;
        ++b;
    }
    return *a == *b;
}
} // namespace
} // namespace lego

static int sfx_entity_scan_callback(EntitySlot* entity, void* payload)
{
    const float dt = *(const float*)payload;
    if (s_sfx_bank_hooks.entity_segment_scan != NULL)
        s_sfx_bank_hooks.entity_segment_scan(
            entity, dt, s_sfx_bank_hooks.context);
    return 0;
}
