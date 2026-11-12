// SPDX-License-Identifier: LGPL-3.0-or-later
/* Rough owner for the recovered weapon definition record footer and name
 * table. The original has a 0x4b68-byte record per weapon; this API keeps the
 * recovered scalar fields and wall-destruction table in a compact form. */
#ifndef LEGORRR_GAME_WEAPON_HPP
#define LEGORRR_GAME_WEAPON_HPP

#include <stddef.h>
#include <stdint.h>

namespace lego {

enum { WEAPON_TYPE_CAPACITY = 64, WEAPON_WALL_TYPE_COUNT = 18,
       WEAPON_NAME_CAPACITY = 64 };

struct WeaponDefinition {
    char name[WEAPON_NAME_CAPACITY];
    float recharge_time;
    float default_damage;
    float discharge_rate;
    float range;
    float wall_destroy_time[WEAPON_WALL_TYPE_COUNT];
};

enum {
    WEAPON_INDEXED_GRID_FLOAT_COUNT = 0x12c0,
    WEAPON_TYPE_RECORD_FLOAT_COUNT = 0x12da,
    WEAPON_DEFAULT_DAMAGE_FLOAT_INDEX = 0x12c4
};

/* Exact 0x4b68-byte record used by the entity-indexed weapon lookup. The
 * first 0x12c0 dwords hold the (entity kind, selected index, level) grid;
 * the named footer occupies the last 26 dwords of that record. */
struct WeaponIndexedTypeRecord {
    float indexed_values[WEAPON_INDEXED_GRID_FLOAT_COUNT]; /* +0x0000 */
    int32_t dispatch_mode;                                 /* +0x4b00 */
    float dispatch_scale;                                  /* +0x4b04 */
    int32_t dispatch_index;                                /* +0x4b08 */
    float recharge_time;                                   /* +0x4b0c */
    float default_damage;                                  /* +0x4b10 */
    float discharge_rate;                                  /* +0x4b14 */
    float unknown_scalar;                                  /* +0x4b18 */
    float range;                                           /* +0x4b1c */
    float wall_destroy_time[WEAPON_WALL_TYPE_COUNT];        /* +0x4b20 */
};
typedef char WeaponIndexedTypeRecordSizeCheck[
    (sizeof(WeaponIndexedTypeRecord) == 0x4b68) ? 1 : -1];
typedef char WeaponIndexedTypeRecordFooterOffsetCheck[
    (offsetof(WeaponIndexedTypeRecord, dispatch_mode) == 0x4b00 &&
     offsetof(WeaponIndexedTypeRecord, default_damage) == 0x4b10 &&
     offsetof(WeaponIndexedTypeRecord, wall_destroy_time) == 0x4b20) ? 1 : -1];
typedef char WeaponIndexedTypeRecordFloatCountCheck[
    (sizeof(WeaponIndexedTypeRecord) / sizeof(float) ==
     WEAPON_TYPE_RECORD_FLOAT_COUNT) ? 1 : -1];

/* Typed, non-owning view over the fixed-stride weapon table. */
struct WeaponIndexedFloatTable {
    const WeaponIndexedTypeRecord* records;
    uint32_t record_count;
};

/* Entity fields consumed by 0x0046f1d0 after reading +0/+4/+0x330. */
struct WeaponIndexedEntityState {
    int32_t entity_type;
    int32_t selected_index;
    int32_t level_index;
};

typedef int (*WeaponDefinitionsParseHook)(void* config, void* context,
                                          WeaponDefinition* definitions,
                                          uint32_t capacity,
                                          uint32_t* count_out);

void weapon_definitions_parse_hook_set(WeaponDefinitionsParseHook hook);
extern "C" int weapon_type_config_load(void* config, void* context); /* FUN_0046eb80 */
uint32_t object_type_name_index_find(const char* name); /* FUN_0046f0d0 */
float weapon_type_recharge_time_get(int type); /* FUN_0046f110 */
float weapon_type_discharge_rate_get(int type); /* FUN_0046f140 */
float weapon_type_range_get(int type); /* FUN_0046f170 */
float weapon_type_wall_destroy_time_get(int type, int wall_type); /* FUN_0046f1a0 */
uint32_t weapon_type_count_get(void);
void weapon_indexed_float_table_install(
    const WeaponIndexedTypeRecord* records, uint32_t record_count);
float weapon_indexed_float_lookup(
    int weapon_type, const WeaponIndexedEntityState* entity_state);
float entity_indexed_float_lookup_with_minus_one_fallback(
    int weapon_type, const void* entity_record); /* 0x0046f1d0 */

} // namespace lego

#endif
