// SPDX-License-Identifier: LGPL-3.0-or-later
/* Synthetic emergence, scene queries, status bookkeeping and effect lifetimes.
 * All model data is a generated animated null or an empty scene frame. */
#include <cmath>
#include <string>

#include "gods98/config.h"
#include "gods98/container_link_names.h"
#include "gods98/container_unit.h"
#include "gods98/container_record.h"
#include "gods98/lws.h"
#include "game/animation_object_record.h"
#include "game/entity_stats.h"
#include "game/lego_unit.h"
#include "game/legoobject_unit.h"
#include "game/panel_unit.h"
#include "game/rock_fall_styles_link_names.h"
#include "game/terrain_map.h"
#include "game/weapons_record.h"
#include "platform/native/test_fixture.h"
#include "test/lrr_test.h"
#include "test/synthetic_level_fixture.h"
#include "game/lego_globs.h"

// Existing unit functions not exposed by the small matching-build headers.
struct Effect_Misc;
struct Effect_Vector3F { float x, y, z; };
extern "C" {
void legoobject_type_behavior_update(Lego_Object*, float);
Effect_Misc* effect_get_misc_effect_data(unsigned int);
int effect_load_misc_effect(Effect_Misc*, void*, const char*);
int effect_spawn_particle(unsigned int, Effect_Vector3F*, Effect_Vector3F*);
int effect_load_rock_fall_style(void*, const char*, unsigned int, unsigned int, const char*);
int effect_spawn_rock_fall(unsigned int, unsigned int, unsigned int, float, float, float, float, float);
unsigned int effect_update_all(float, unsigned int**, unsigned int**);
void effect_get_block_pos_rock_fall(unsigned int, unsigned int, unsigned int*, unsigned int*);
void* effect_find_rock_fall_part(unsigned int, unsigned int);
void effect_remove_all_rock_falls(void);

int weapons_initialise(BlockRec*, const char*);
void weapons_generic_damage_object(Lego_Object*, int, int, float, const float*);
void weapons_update(float);
int weapons_object_collision_check(Lego_Object*, float*, float*, int);
int weapons_object_quad_intersect(Lego_Object*, float*, float*, float*, int);
int weapons_ray_trace_nearest(Weapons_Vector3F*, Weapons_Vector3F*, Lego_Object**, float*, Lego_Object*);
int weapons_object_can_be_hit(Lego_Object*);
int weapons_object_center_get(Lego_Object*, Weapons_Vector3F*);
}

namespace {
using namespace synthetic_fixture;
constexpr unsigned Floor = 8, Wall = 16, Reserved = 0x8000000;
constexpr unsigned Appearing = 0x100, Ready = 0x10000, Removing = 0x800000;
constexpr unsigned Suspended = 0x80000000, StatusPaused = 0x8000000;

Lego_Level* level() { return legoGlobs.currLevel; }
TerrainMap* map() { return level()->map; }
Container* root() { return static_cast<Container*>(legoGlobs.rootCont); }
Lego_Block& cell(int x, int y) { return level()->blocks[y * level()->width + x]; }

void check_cell(Lego_Object* obj, int x, int y)
{
    int bx = -1, by = -1;
    CHECK_EQ(legoobject_position_to_terrain_cell(obj, &bx, &by), 1);
    CHECK_EQ(bx, x);
    CHECK_EQ(by, y);
}

void place(Lego_Object* obj, int x, int y)
{
    float wx, wy;
    terrain_map_block_to_world(map(), x, y, &wx, &wy);
    legoobject_world_position_set(obj, wx, wy, 0, 1);
}

struct Census {
    int type;
    const Lego_Object* exclude;
    unsigned count = 0;
    Lego_Object* last = nullptr;
};

int collect_kind(Lego_Object* obj, void* data)
{
    auto* census = static_cast<Census*>(data);
    if (obj->type == census->type && obj != census->exclude) {
        ++census->count;
        census->last = obj;
    }
    return 0;
}

Census census(int type, const Lego_Object* exclude = nullptr)
{
    Census result{type, exclude};
    legoobject_run_through_lists_skip_upgrade_parts(collect_kind, &result);
    return result;
}

bool make_null_catalog()
{
    // The same generated null scene is used by each named activity.
    return CHECK(fixture_mkdir("Data/Primitive") &&
           write_null_scene("Data/Primitive/motion.lws") &&
           write_text("Data/Primitive/Primitive.ae",
               "Lego { Activities { Activity_Stand Idle Activity_Emerge Step "
               "Activity_WakeUp Step Activity_Gather Step Activity_HitHard Step "
               "Expand Step Long Idle Teleport Step FlapInWind Idle "
               "Disintegrate Step Normal Idle Melt Step } "
               "Idle { FILE motion LWSFILE TRUE LOOPING TRUE } "
               "Step { FILE motion LWSFILE TRUE LOOPING FALSE } }"),
               "generate generic animated null");
}

unsigned emergence_cell(unsigned x, unsigned y)
{
    if (x == 1 && y == 4) return 1; // Trigger.
    if (x == 1 && y == 3) return 2; // Rejected, already open.
    if (x == 3 && y == 3) return 3; // Valid emergence point.
    return 0;
}

bool load_creature()
{
    constexpr char text[] =
        "Lego { RockMonsterTypes { Creature Primitive } "
        "Stats { Creature { Levels 1 CollRadius 2 CollHeight 4 CrossLand TRUE RouteSpeed 5 } } }";
    auto* config = block_parse(text, sizeof(text) - 1);
    if (!CHECK(config != nullptr, "synthetic creature config")) return false;
    auto* saved = legoGlobs.config;
    legoGlobs.config = config;
    bool ok = CHECK(lego_entity_rockmonster_type_table_load(), "load creature null") &&
              CHECK(stats_initialise(config, "Lego"), "load creature stats");
    legoGlobs.config = saved;
    block_close(config);
    return ok;
}

void check_emergence()
{
    if (!make_null_catalog() || !load_creature()) return;
    auto* model = static_cast<AnimationObject*>(legoGlobs.rockMonsterData);
    // Valid origin and several possible neighbours, explicitly set up on
    // the small grid. North and east are unavailable; south wins over west.
    cell(3, 3).flags1 = Wall;
    cell(3, 3).f_02 = 4;
    cell(3, 2).flags1 = Floor | 0x100000;
    cell(4, 3).flags1 = Wall;
    cell(3, 4).flags1 = Floor;
    cell(3, 4).flags2 = 0;
    cell(3, 4).f_02 = 0;
    cell(2, 3).flags1 = Floor;
    cell(2, 3).flags2 = 0;
    cell(2, 3).f_02 = 0;
    CHECK_EQ(legoobject_creature_spawn_from_emergence(model, 2, 0, 3, 3), nullptr);
    CHECK_EQ(legoobject_creature_spawn_from_emergence(model, 3, 0, 1, 3), nullptr);
    cell(3, 3).flags1 |= Reserved;
    CHECK_EQ(legoobject_creature_spawn_from_emergence(model, 3, 0, 3, 3), nullptr);
    cell(3, 3).flags1 &= ~Reserved;
    cell(3, 4).flags1 = Wall;
    cell(2, 3).flags1 = Wall;
    CHECK_EQ(legoobject_creature_spawn_from_emergence(model, 3, 0, 3, 3), nullptr);
    CHECK_EQ(census(3).count, 0u);
    cell(3, 4).flags1 = cell(2, 3).flags1 = Floor;

    if (!CHECK(write_map("Data/L/emerge.map", W, H, emergence_cell), "generate emergence layer") ||
        !CHECK_EQ(lego_load_layer_l(level(), "L\\emerge.map"), 1)) return;
    CHECK_EQ(level()->f_00c, 1u);
    CHECK(cell(1, 4).flags2 & 0x80, "trigger marked in level");
    CHECK(cell(3, 3).flags2 & 0x40, "emergence point marked in level");
    level()->f_0b0 = 0;
    level()->emergeTimeOut = 30;
    LegoPoint2I2A trigger{1, 4}, wrong{1, 5}, output{-1, -1};
    CHECK_EQ(lego_emerge_map_trigger_spawn(level(), &wrong, &output), 0);
    CHECK_EQ(output.x, -1);
    CHECK_EQ(lego_emerge_map_trigger_spawn(level(), &trigger, &output), 1);
    CHECK_EQ(output.x, 3);
    CHECK_EQ(output.y, 3);
    auto spawned = census(3);
    if (!CHECK_EQ(spawned.count, 1u)) return;
    auto* creature = spawned.last;
    CHECK_EQ(creature->targetBlockPos[0], 3.0f);
    CHECK_EQ(creature->targetBlockPos[1], 3.0f);
    CHECK(creature->flags1 & Appearing, "emergence activity pending");
    CHECK(cell(3, 3).flags1 & Reserved, "origin reserved during emergence");
    check_cell(creature, 3, 4);
    auto* scene = static_cast<Container*>(creature->rockMonster->container);
    float dir[3], up[3];
    Container_GetOrientation(scene, root(), dir, up);
    CHECK_NEAR(dir[0], 0.0f, 1e-4);
    CHECK_NEAR(dir[1], -1.0f, 1e-4);

    auto* record = static_cast<LegoEmergeTrigger2A*>(level()->emergeMap);
    CHECK_EQ(record->timer, 30.0f);
    CHECK_EQ(lego_emerge_map_trigger_spawn(level(), &trigger, nullptr), 0);
    float elapsed = 1;
    legoobject_update_callback(creature, &elapsed);
    CHECK(creature->flags1 & Appearing, "partial animation does not finish");
    CHECK(cell(3, 3).flags1 & Reserved, "reservation survives partial animation");
    elapsed = 20;
    for (unsigned i = 0; i < 5 && (creature->flags1 & Appearing); ++i)
        legoobject_update_callback(creature, &elapsed);
    CHECK_EQ(creature->flags1 & Appearing, 0u);
    CHECK_EQ(cell(3, 3).flags1 & Reserved, 0u);
    CHECK_EQ(creature->flags3 & Suspended, 0u);
    CHECK_EQ(creature->health, 100.0f);

    lego_level_record_timer_countdown(level(), 29);
    CHECK_EQ(record->timer, 1.0f);
    CHECK_EQ(lego_emerge_map_trigger_spawn(level(), &trigger, nullptr), 0);
    lego_level_record_timer_countdown(level(), 1);
    CHECK_EQ(record->timer, 0.0f);
    CHECK_EQ(lego_emerge_map_trigger_spawn(level(), &trigger, nullptr), 1);
    CHECK_EQ(census(3).count, 2u);
    CHECK_EQ(record->timer, 30.0f);
    // Finish the second appearance before removing the two live objects.
    auto others = census(3, creature);
    if (!CHECK_EQ(others.count, 1u)) return;
    auto* second = others.last;
    for (unsigned i = 0; i < 5 && (second->flags1 & Appearing); ++i)
        legoobject_update_callback(second, &elapsed);
    CHECK_EQ(second->flags1 & Appearing, 0u);
    CHECK_EQ(cell(3, 3).flags1 & Reserved, 0u);

    // Health and energy use independent configured rates; suspension
    // prevents decay and energy is capped by the current health.
    creature->stats->HealthDecayRate = 4;
    creature->stats->EnergyDecayRate = 8;
    creature->energy = 80;
    legoobject_energy_low_target_route_command_tick(creature, 25);
    CHECK_NEAR(creature->health, 96.0f, 1e-5);
    CHECK_NEAR(creature->energy, 72.0f, 1e-5);
    creature->flags3 |= Suspended;
    legoobject_energy_low_target_route_command_tick(creature, 25);
    CHECK_NEAR(creature->health, 96.0f, 1e-5);
    CHECK_NEAR(creature->energy, 72.0f, 1e-5);
    creature->flags3 &= ~Suspended;
    creature->energy = 100;
    legoobject_energy_low_target_route_command_tick(creature, 0);
    CHECK_EQ(creature->energy, creature->health);
    // The same synthetic terrain hazard changes health only while the
    // object's crossing capability is absent.
    creature->stats->HealthDecayRate = creature->stats->EnergyDecayRate = 0;
    cell(3, 4).f_02 = 6;
    legoobject_energy_low_target_route_command_tick(creature, 2);
    CHECK_EQ(creature->health, 94.0f);
    CHECK_EQ(creature->energy, 94.0f);
    CHECK_EQ(creature->field_3d0, 2.0f);
    creature->stats->flags2 |= 0x200000;
    legoobject_energy_low_target_route_command_tick(creature, 2);
    CHECK_EQ(creature->health, 94.0f);
    CHECK_EQ(creature->energy, 94.0f);
    creature->stats->flags2 &= ~0x200000u;
    cell(3, 4).f_02 = 0;
    legoobject_remove(second);
    legoobject_remove(creature);
    CHECK_EQ(census(3).count, 0u);
}

void check_effects()
{
    if (!make_null_catalog()) return;
    Container* model = Container_Load(root(), "Primitive", "ACT", 1);
    if (!CHECK(model != nullptr, "generic activity container loaded")) return;
    auto create = [&](int type) {
        return legoobject_create(model, type, 0);
    };
    // A placed scene marker progresses through appearance, stable state,
    // removal animation, and finally releases its object-pool entry.
    auto* marker = create(9);
    if (!CHECK(marker != nullptr, "scene marker")) return;
    auto* scene = static_cast<Container*>(marker->other);
    if (!CHECK(scene->cloneData != nullptr, "animated clone has ownership state")) return;
    CHECK_EQ(scene->cloneData->used, 1);
    marker->flags1 |= 0x80000;
    legoobject_type_behavior_update(marker, 1);
    CHECK(marker->flags1 & 0x80000, "placement waits for animation");
    legoobject_type_behavior_update(marker, 20);
    CHECK_EQ(marker->flags1 & 0x80000, 0u);
    CHECK(marker->flags1 & Appearing, "marker appearance started");
    CHECK(Container_IsCurrentActivity(scene, "Expand"), "appearance animation");
    legoobject_type_behavior_update(marker, 20);
    CHECK_EQ(marker->flags1 & Appearing, 0u);
    CHECK(marker->flags3 & Ready, "marker stable");
    CHECK(Container_IsCurrentActivity(scene, "Long"), "stable animation");
    marker->flags3 |= Removing;
    marker->health = -1;
    legoobject_type_behavior_update(marker, 20);
    CHECK_EQ(marker->flags3 & Removing, 0u);
    CHECK(marker->flags1 & 0x200000, "removal animation in progress");
    CHECK_EQ(marker->health, 100.0f);
    CHECK(Container_IsCurrentActivity(scene, "Teleport"), "removal animation");
    legoobject_type_behavior_update(marker, 20);
    CHECK_EQ(marker->flags1 & 0x200000, 0u);
    CHECK_EQ(marker->health, -1.0f);
    CHECK(marker->flags3 & Removing, "marker ready for release");
    legoobject_objective_sidecar_append_gate(marker);
    CHECK_EQ(census(9).count, 0u);
    CHECK_EQ(scene->cloneData->used, 0);
    CHECK_EQ(Container_GetParent(scene), nullptr);

    // A linked status visual releases its affected object's pause flag
    // only when its final animation completes.
    Lego_Object affected = {};
    affected.flags2 = StatusPaused;
    auto* status = create(18);
    if (!CHECK(status != nullptr, "status visual")) return;
    auto* statusScene = static_cast<Container*>(status->other);
    if (!CHECK(statusScene->cloneData != nullptr, "animated status clone")) return;
    CHECK_EQ(statusScene, scene); // Inactive clone becomes available again.
    CHECK_EQ(statusScene->cloneData->used, 1);
    status->field_364 = &affected;
    status->flags1 |= Appearing;
    legoobject_type_behavior_update(status, 20);
    CHECK_EQ(status->flags1 & Appearing, 0u);
    CHECK(status->flags3 & Ready, "status visual stable");
    CHECK(Container_IsCurrentActivity(statusScene, "Normal"), "stable visual activity");
    status->flags3 |= Removing;
    legoobject_type_behavior_update(status, 20);
    CHECK(status->flags1 & 0x200000, "status release animation");
    CHECK_EQ(affected.flags2 & StatusPaused, StatusPaused);
    CHECK(Container_IsCurrentActivity(statusScene, "Melt"), "release activity");
    legoobject_type_behavior_update(status, 1);
    CHECK_EQ(affected.flags2 & StatusPaused, StatusPaused);
    legoobject_type_behavior_update(status, 20);
    CHECK_EQ(affected.flags2 & StatusPaused, 0u);
    CHECK_EQ(status->health, -1.0f);
    CHECK(status->flags3 & Removing, "status visual ready for release");
    legoobject_objective_sidecar_append_gate(status);
    CHECK_EQ(census(18).count, 0u);
    CHECK_EQ(statusScene->cloneData->used, 0);
    CHECK_EQ(Container_GetParent(statusScene), nullptr);

    // The web visual uses real animation time: a paused visual must not
    // advance, but can resume and play its removal activity.
    auto* web = create(12);
    if (!CHECK(web != nullptr, "web visual")) return;
    auto* webScene = static_cast<Container*>(web->other);
    if (!CHECK(webScene->cloneData != nullptr, "animated web clone")) return;
    CHECK_EQ(webScene, statusScene);
    CHECK_EQ(webScene->cloneData->used, 1);
    web->flags1 |= Appearing;
    legoobject_type_behavior_update(web, 20);
    CHECK_EQ(web->flags1 & Appearing, 0u);
    CHECK(Container_IsCurrentActivity(webScene, "FlapInWind"), "web stable activity");
    web->flags1 |= 0x2000000;
    float before = Container_GetAnimationTime(webScene);
    legoobject_type_behavior_update(web, 8);
    CHECK_EQ(Container_GetAnimationTime(webScene), before);
    web->flags1 &= ~0x2000000u;
    legoobject_type_behavior_update(web, 8);
    CHECK(Container_GetAnimationTime(webScene) > before, "visual resumes");
    web->flags3 |= Removing;
    legoobject_type_behavior_update(web, 20);
    CHECK(Container_IsCurrentActivity(webScene, "Disintegrate"), "web removal activity");
    legoobject_type_behavior_update(web, 20);
    CHECK_EQ(web->health, -1.0f);
    legoobject_objective_sidecar_append_gate(web);
    CHECK_EQ(census(12).count, 0u);
    CHECK_EQ(webScene->cloneData->used, 0);
    CHECK_EQ(Container_GetParent(webScene), nullptr);

    // Miscellaneous particles have ten live slots. Staggered animation
    // completion returns only the expired slot, then all slots become reusable.
    Effect_Misc* misc = effect_get_misc_effect_data(3);
    if (!CHECK(misc != nullptr, "neutral effect slot") ||
        !CHECK_EQ(effect_load_misc_effect(misc, root(), "Primitive\\motion"), 1)) return;
    Effect_Vector3F p{1, 2, 3}, pose{0, 1, 0};
    CHECK_EQ(effect_spawn_particle(4, &p, nullptr), 0); // Valid but unloaded slot.
    CHECK_EQ(effect_spawn_particle(3, &p, nullptr), 1);
    unsigned *types = nullptr, *indices = nullptr;
    CHECK_EQ(effect_update_all(8, &types, &indices), 0u);
    for (unsigned i = 0; i < 9; ++i) CHECK_EQ(effect_spawn_particle(3, &p, &pose), 1);
    CHECK_EQ(effect_spawn_particle(3, &p, nullptr), 0);
    CHECK_EQ(effect_update_all(8, &types, &indices), 0u);
    CHECK_EQ(effect_spawn_particle(3, &p, nullptr), 1);
    CHECK_EQ(effect_spawn_particle(3, &p, nullptr), 0);
    CHECK_EQ(effect_update_all(25, &types, &indices), 0u);
    for (unsigned i = 0; i < 10; ++i) CHECK_EQ(effect_spawn_particle(3, &p, nullptr), 1);
    CHECK_EQ(effect_spawn_particle(3, &p, nullptr), 0);
    CHECK_EQ(effect_update_all(25, &types, &indices), 0u);

    // Terrain decoration uses a fixed four-instance pool and reports the
    // original grid positions for completed effects.
    if (!CHECK_EQ(effect_load_rock_fall_style(root(), "Primitive\\motion", 0, 0, "Anchor"), 1)) return;
    for (unsigned i = 0; i < 4; ++i)
        CHECK_EQ(effect_spawn_rock_fall(0, i + 1, 2, p.x, p.y, p.z, 0, 1), 1);
    CHECK_EQ(effect_spawn_rock_fall(0, 1, 3, p.x, p.y, p.z, 0, 1), 0);
    CHECK_EQ(effect_find_rock_fall_part(5, 5), nullptr);
    CHECK_EQ(effect_update_all(8, &types, &indices), 0u);
    CHECK_EQ(effect_spawn_rock_fall(0, 1, 3, p.x, p.y, p.z, 0, 1), 0);
    unsigned count = effect_update_all(20, &types, &indices);
    if (CHECK_EQ(count, 4u)) {
        for (unsigned i = 0; i < count; ++i) {
            CHECK_EQ(types[i], 0u);
            CHECK_EQ(indices[i], i);
            unsigned x = 0, y = 0;
            effect_get_block_pos_rock_fall(types[i], indices[i], &x, &y);
            CHECK_EQ(x, i + 1);
            CHECK_EQ(y, 2u);
        }
    }
    CHECK_EQ(effect_find_rock_fall_part(1, 2), nullptr);
    CHECK_EQ(effect_spawn_rock_fall(0, 2, 4, p.x, p.y, p.z, 0, 1), 1);
    CHECK_EQ(effect_update_all(20, &types, &indices), 1u);
    effect_remove_all_rock_falls();
    Container_Remove(model);
}

// The scene-query and status functions are tested as abstract engine
// routines on tiny neutral geometry, without any projectile or firing logic.
void check_geometry_status()
{
    Container* prototype = Container_Create(root());
    if (!CHECK(prototype != nullptr, "generic scene frame")) return;
    StatsData row = {};
    row.CollRadius = 3;
    row.CollHeight = 4;
    row.CollBox = {4, 6};
    row.flags1 = 0x2000000;
    row.flags2 = 0x100;
    row.RouteSpeed = 5;
    StatsData* byId[STATS_OBJECT_IDS] = {};
    byId[0] = &row;
    Stats_Globs* stats = entity_stats_port_globs();
    auto* oldStats = stats->objectStats[3];
    const unsigned oldLevels = stats->objectLevels[3][0];
    stats->objectStats[3] = byId;
    stats->objectLevels[3][0] = 1;
    AnimationObject model = {};
    model.container = prototype;
    auto make_object = [&](int x, int y) {
        float wx, wy;
        terrain_map_block_to_world(map(), x, y, &wx, &wy);
        return legoobject_create_in_world(&model, 3, 0, 0, wx, wy, 0);
    };
    auto* firstObject = make_object(1, 3);
    auto* secondObject = make_object(1, 4);
    if (!CHECK(firstObject && secondObject, "generic scene objects")) return;

    Weapons_Vector3F center;
    CHECK_EQ(weapons_object_center_get(firstObject, &center), 1);
    CHECK_NEAR(center.z, terrain_map_world_z_get(map(), center.x, center.y) - 2, 1e-4);
    float from[] = {center.x - 5, center.y, center.z};
    float segment[] = {10, 0, 0};
    CHECK_EQ(weapons_object_collision_check(firstObject, from, segment, 1), 1);
    from[1] += 4;
    CHECK_EQ(weapons_object_collision_check(firstObject, from, segment, 1), 0);
    from[1] = center.y;
    row.flags1 |= 0x100; // Box rather than circular bounds.
    float intersection[2] = {};
    CHECK_EQ(weapons_object_quad_intersect(firstObject, from, segment, intersection, 0), 1);
    CHECK_NEAR(std::fabs(intersection[0] - center.x), 2.0f, 1e-4);
    CHECK_NEAR(intersection[1], center.y, 1e-4);
    from[1] += 4;
    CHECK_EQ(weapons_object_collision_check(firstObject, from, segment, 0), 0);
    row.flags1 &= ~0x100u;

    // Pick the first neutral object in a line of scene frames. Relocating
    // it leaves the second visible; moving both aside yields no object.
    Weapons_Vector3F origin, delta;
    terrain_map_block_to_world(map(), 1, 2, &origin.x, &origin.y);
    origin.z = terrain_map_world_z_get(map(), origin.x, origin.y) - 2;
    delta = {0, -3 * map()->blockSize, 0};
    float distance = 1;
    Lego_Object* picked = nullptr;
    CHECK_EQ(weapons_ray_trace_nearest(&origin, &delta, &picked, &distance, nullptr), 1);
    CHECK_EQ(picked, firstObject);
    CHECK_NEAR(distance, 1.0f / 3.0f, 1e-4);
    place(firstObject, 2, 3);
    distance = 1;
    CHECK_EQ(weapons_ray_trace_nearest(&origin, &delta, &picked, &distance, nullptr), 1);
    CHECK_EQ(picked, secondObject);
    CHECK_NEAR(distance, 2.0f / 3.0f, 1e-4);
    place(secondObject, 2, 4);
    distance = 1;
    CHECK_EQ(weapons_ray_trace_nearest(&origin, &delta, &picked, &distance, nullptr), 0);
    CHECK_EQ(picked, nullptr);

    // A line crossing the actual terrain mesh reports a terrain
    // intersection, not a scene-object selection.
    origin.z -= 8;
    delta = {0, -map()->blockSize, 20};
    distance = 1;
    CHECK_EQ(weapons_ray_trace_nearest(&origin, &delta, &picked, &distance, nullptr), 1);
    CHECK_EQ(picked, nullptr);
    CHECK(distance > 0 && distance < 1, "terrain intersection inside query extent");

    // Abstract instantaneous and timed status changes are applied by
    // the existing game bookkeeping, not a simulated physical tool.
    constexpr char config[] =
        "Lego { WeaponTypes { StatusA { DefaultDamage 4 } StatusB { DefaultDamage 2 SlowDeath 0.5:3 } } }";
    BlockFile* effects = block_parse(config, sizeof(config) - 1);
    if (!CHECK(effects != nullptr, "synthetic status configuration") ||
        !CHECK_EQ(weapons_initialise(effects, "Lego"), 1)) return;
    firstObject->flags1 |= Appearing;
    CHECK_EQ(weapons_object_can_be_hit(firstObject), 0);
    weapons_generic_damage_object(firstObject, 0, 0, 1, nullptr);
    CHECK_EQ(firstObject->health, 100.0f);
    firstObject->flags1 &= ~Appearing;
    CHECK_EQ(weapons_object_can_be_hit(firstObject), 1);
    weapons_generic_damage_object(firstObject, 0, 0, 1, nullptr);
    CHECK_EQ(firstObject->health, 96.0f);
    CHECK_EQ(firstObject->field_3d0, 4.0f);
    // A timed status starts with a partial change and expires after
    // three simulation updates. A separate object remains unchanged.
    weapons_generic_damage_object(firstObject, 1, 1, 1, nullptr);
    CHECK_EQ(firstObject->health, 95.0f);
    CHECK_EQ(firstObject->slowDeathTimer, 3.0f);
    CHECK_EQ(firstObject->slowDeathWeapon, 1);
    CHECK_EQ(firstObject->field_3d0, 5.0f);
    for (unsigned tick = 1; tick <= 3; ++tick) {
        weapons_update(1);
        CHECK_EQ(firstObject->slowDeathTimer, 3.0f - tick);
        CHECK_EQ(firstObject->health, 95.0f - 2.0f * tick);
        CHECK_EQ(firstObject->field_3d0, 5.0f + 2.0f * tick);
        CHECK_EQ(secondObject->health, 100.0f);
    }
    weapons_update(1);
    CHECK_EQ(firstObject->health, 89.0f);
    CHECK_EQ(firstObject->slowDeathTimer, 0.0f);
    legoobject_remove(secondObject);
    legoobject_remove(firstObject);
    stats->objectStats[3] = oldStats;
    stats->objectLevels[3][0] = oldLevels;
    Container_Remove(prototype);
    block_close(effects);
}
} // namespace

int main(int argc, char** argv)
{
    const std::string scenario = argc == 2 ? argv[1] : "emergence";
    if (!CHECK(scenario == "emergence" || scenario == "effects" || scenario == "geometry",
               "known scenario")) return lrr_test_result();
    const std::string directory = "creature_effect_" + scenario;
    if (!CHECK(make_fixture(directory.c_str()), "generate level") ||
        !initialise_headless("CreatureEffectLifecycle") || !boot_game() ||
        !CHECK(lego_level_load("Level01"), "load generated level")) return lrr_test_result();
    lws_initialise(nullptr, nullptr, nullptr, nullptr);
    if (scenario == "emergence") check_emergence();
    else if (scenario == "effects") check_effects();
    else check_geometry_status();

    if (lrr_test::failures != 0) return lrr_test_result();
    const unsigned types[] = {0, 1, 2};
    const float areas[12] = {};
    const int tips[3] = {};
    panel_texture_slots_allocate(PANEL_TOP, 3, types, areas, nullptr, nullptr, nullptr, tips);
    lego_level_free();
    CHECK(legoGlobs.currLevel == nullptr, "level unloaded");
    return lrr_test_result();
}
