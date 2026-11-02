// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef LEGO_GAME_TYPE_CONFIG_PORTS_HPP
#define LEGO_GAME_TYPE_CONFIG_PORTS_HPP

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "../gods98/heap.hpp"

struct EntitySceneSlotNode;

/* These adapters keep the loader's concrete work here while leaving only
 * config traversal and the type-specific parsers at the engine seam. */
typedef struct TypeConfigTable {
    void* rows;
    char** names;
    /* Native ownership records for data referenced through fixed-width
     * dword tokens in the original rows (currently BuildingType Shape). */
    void** row_owned;
    uint32_t count;
    uint32_t capacity;
    uint32_t row_stride;
} TypeConfigTable;

/* The original BuildingType Shape field points at a 0x50-byte table of ten
 * pairs of 32-bit pointers. Keep that exact layout in entries, and retain the
 * host pointers separately so 64-bit builds neither truncate pointers nor
 * overwrite adjacent entries. Each nonzero entry word is a 1-based index
 * into strings; the row's +0x84 word is a 1-based row index in TypeConfigTable. */
typedef struct TypeConfigShapeOwner {
    uint32_t entries[20];
    char* strings[18];
    uint32_t string_count;
    uint32_t* upgrade_records;
    char** upgrade_values;
    const void** upgrade_names;
    uint32_t upgrade_count;
    uint32_t upgrade_level_mask;
} TypeConfigShapeOwner;

static inline const uint32_t* type_config_shape_table_resolve(
    const TypeConfigTable* table, uint32_t table_token)
{
    const TypeConfigShapeOwner* owner;
    if (table == NULL || table_token == 0u ||
        table_token > table->count || table->row_owned == NULL) return NULL;
    owner = (const TypeConfigShapeOwner*)table->row_owned[table_token - 1u];
    return owner != NULL ? owner->entries : NULL;
}

static inline const char* type_config_shape_string_resolve(
    const TypeConfigTable* table, uint32_t table_token, uint32_t string_token)
{
    const TypeConfigShapeOwner* owner;
    if (table == NULL || table_token == 0u ||
        table_token > table->count || table->row_owned == NULL ||
        string_token == 0u) return NULL;
    owner = (const TypeConfigShapeOwner*)table->row_owned[table_token - 1u];
    if (owner == NULL || string_token > owner->string_count) return NULL;
    return owner->strings[string_token - 1u];
}

static inline const uint32_t* type_config_building_upgrades_resolve(
    const TypeConfigTable* table, uint32_t row_token, uint32_t* count_out,
    uint32_t* level_mask_out)
{
    const TypeConfigShapeOwner* owner;
    if (count_out != NULL) *count_out = 0u;
    if (level_mask_out != NULL) *level_mask_out = 0u;
    if (table == NULL || row_token == 0u || row_token > table->count ||
        table->row_owned == NULL) return NULL;
    owner = (const TypeConfigShapeOwner*)table->row_owned[row_token - 1u];
    if (owner == NULL) return NULL;
    if (count_out != NULL) *count_out = owner->upgrade_count;
    if (level_mask_out != NULL) *level_mask_out = owner->upgrade_level_mask;
    return owner->upgrade_records;
}

static inline const char* type_config_building_upgrade_value_resolve(
    const TypeConfigTable* table, uint32_t row_token, uint32_t value_token)
{
    const TypeConfigShapeOwner* owner;
    if (table == NULL || row_token == 0u || row_token > table->count ||
        table->row_owned == NULL || value_token == 0u) return NULL;
    owner = (const TypeConfigShapeOwner*)table->row_owned[row_token - 1u];
    if (owner == NULL || owner->upgrade_values == NULL ||
        value_token > owner->upgrade_count) return NULL;
    return owner->upgrade_values[value_token - 1u];
}

static inline const void* type_config_building_upgrade_name_resolve(
    const TypeConfigTable* table, uint32_t row_token, uint32_t name_token)
{
    const TypeConfigShapeOwner* owner;
    if (table == NULL || row_token == 0u || row_token > table->count ||
        table->row_owned == NULL || name_token == 0u) return NULL;
    owner = (const TypeConfigShapeOwner*)table->row_owned[row_token - 1u];
    if (owner == NULL || owner->upgrade_names == NULL ||
        name_token > owner->upgrade_count) return NULL;
    return owner->upgrade_names[name_token - 1u];
}

typedef struct TypeConfigHooks {
    uint32_t (*entry_count)(void* context, void* section);
    int (*entry_get)(void* context, void* section, uint32_t index,
                     const char** name, void** entry);
    int (*row_parse)(void* context, void* entry, void* destination,
                     uint32_t row_kind);
    void (*row_finalize)(void* context, void* row, uint32_t row_kind);
    void* context;
} TypeConfigHooks;

/* The parser's scene/model operations cross into the renderer runtime. Values
 * stored in the reference's dword record slots are represented by backend
 * tokens here; the adapter owns their interpretation and lifetime. */
typedef struct EntityTypeConfigRuntimeOps {
    void* context;
    int (*visual_load)(void* context, const char* asset_path,
                       uint32_t* scene_token_out);
    uint32_t (*named_resource_get)(void* context, uint32_t scene_token,
                                   const char* name, int frame_count);
    uint32_t (*configured_value_token)(void* context, const char* value);
    uint32_t (*poly_variant_build)(void* context, uint32_t scene_token,
                                   const char* asset_path,
                                   const char* section_key,
                                   uint32_t variant, uint32_t frame_count);
    void (*scene_apply_mode)(void* context, uint32_t scene_token, int mode);
    void* (*activity_scene_clone)(void* context, uint32_t scene_token);
    void (*activity_scene_release)(void* context, void* scene);
    void (*activity_scene_owner_bind)(void* context, void* scene,
                                     struct EntitySlot* entity);
    void* (*activity_resource_chain_clone)(void* context,
                                           uint32_t chain_token);
    void (*activity_resource_chain_release)(void* context, void* chain);
    void* (*activity_indexed_resource_resolve)(void* context, void* scene,
                                              uint32_t source_token,
                                              uint32_t index);
    void (*activity_scene_time_update)(void* context, void* scene,
                                      void* value);
    int (*activity_scene_component_gate_read)(void* context, void* scene,
                                              uint32_t* value_out);
    /* Build the host mirror of the loaded scene's +0x9c linked renderer
     * node list. The callback appends nodes and installs the three owner
     * resource-name keys through the entity scene-slot APIs. */
    void (*scene_slot_nodes_load)(void* context, struct EntitySlot* entity,
                                  uint32_t scene_token);
    void (*scene_slot_renderer_position_prepare)(void* context,
                                                 struct EntitySlot* entity,
                                                 struct EntitySceneSlotNode* node);
    void (*scene_slot_renderer_state_apply)(void* context,
                                            struct EntitySceneSlotNode* node);
    void (*scene_slot_node_animation_select)(void* context,
                                             struct EntitySceneSlotNode* node,
                                             int mode);
    void* (*scene_slot_resource_register)(void* context,
                                          struct EntitySceneSlotNode* node,
                                          uint32_t owner_key,
                                          uint32_t index);
} EntityTypeConfigRuntimeOps;

void entity_type_config_runtime_ops_install(
    const EntityTypeConfigRuntimeOps* ops);
int entity_type_config_row_parse(void* context, void* entry,
                                 void* destination, uint32_t row_kind);
int entity_type_config_resource_row_load(void* destination,
                                        uint32_t row_index,
                                        void* scene_context,
                                        const char* asset_path,
                                        void* engine_config,
                                        uint32_t row_kind);
int entity_building_type_config_record_parse(void* destination,
                                             uint32_t row_index,
                                             void* scene_context,
                                             const char* asset_path,
                                             void* engine_config);
int entity_type_config_scene_slot_nodes_load(struct EntitySlot* entity);
void entity_type_config_row_finalize(void* context, void* row,
                                     uint32_t row_kind);

static inline void type_config_table_release(TypeConfigTable* table)
{
    uint32_t i;
    if (table == NULL) return;
    if (table->row_owned != NULL) {
        for (i = 0; i < table->count; ++i) {
            TypeConfigShapeOwner* shape =
                (TypeConfigShapeOwner*)table->row_owned[i];
            uint32_t j;
            if (shape == NULL) continue;
            if (shape->string_count > 18u) shape->string_count = 18u;
            for (j = 0u; j < shape->string_count; ++j)
                engine_heap_free(shape->strings[j]);
            if (shape->upgrade_values != NULL) {
                for (j = 0u; j < shape->upgrade_count; ++j)
                    engine_heap_free(shape->upgrade_values[j]);
            }
            free(shape->upgrade_values);
            free(shape->upgrade_names);
            engine_heap_free(shape->upgrade_records);
            free(shape);
        }
        free(table->row_owned);
    }
    if (table->names != NULL) {
        for (i = 0; i < table->count; ++i)
            engine_heap_free(table->names[i]);
        engine_heap_free(table->names);
    }
    engine_heap_free(table->rows);
    table->rows = NULL;
    table->names = NULL;
    table->row_owned = NULL;
    table->count = 0;
    table->capacity = 0;
}

/* MiniFigureTypeTableLoad (0x0042cf50) and BuildingTypesLoad
 * (0x0042d2b0): count the section, allocate the fixed-stride row block and
 * parallel owned-name array, then parse/finalize/copy each entry in order.
 * A missing section, empty section, allocation failure, or row parse failure
 * answers zero. A partial load releases both allocations, as the decompile
 * does. */
static inline int type_config_record_table_load(TypeConfigTable* table,
                                                void* section,
                                                uint32_t row_kind,
                                                uint32_t row_stride,
                                                const TypeConfigHooks* hooks)
{
    uint32_t count;
    uint32_t i;
    char* rows;
    char** names;
    void** row_owned;
    if (table == NULL || hooks == NULL || hooks->entry_count == NULL ||
        hooks->entry_get == NULL || hooks->row_parse == NULL ||
        row_stride == 0u) return 0;

    table->rows = NULL;
    table->names = NULL;
    table->row_owned = NULL;
    table->count = 0u;
    table->capacity = 0u;
    table->row_stride = row_stride;
    count = hooks->entry_count(hooks->context, section);
    if (count == 0u || count > 0xffffffffu / row_stride ||
        count > 0xffffffffu / (uint32_t)sizeof(char*)) return 0;

    rows = (char*)engine_heap_alloc((long)((size_t)count * row_stride));
    if (rows == NULL) return 0;
    names = (char**)engine_heap_alloc((long)((size_t)count * sizeof(char*)));
    if (names == NULL) {
        engine_heap_free(rows);
        return 0;
    }
    memset(names, 0, (size_t)count * sizeof(char*));
    row_owned = (void**)calloc((size_t)count, sizeof(void*));
    if (row_owned == NULL) {
        engine_heap_free(names);
        engine_heap_free(rows);
        return 0;
    }
    table->rows = rows;
    table->names = names;
    table->row_owned = row_owned;
    table->capacity = count;

    for (i = 0u; i < count; ++i) {
        const char* source_name = NULL;
        void* entry = NULL;
        size_t length;
        char* row = rows + (size_t)i * row_stride;
        memset(row, 0, row_stride);
        table->count = i + 1u;
        if (!hooks->entry_get(hooks->context, section, i,
                              &source_name, &entry) || source_name == NULL ||
            !hooks->row_parse(hooks->context, entry, row, row_kind)) {
            type_config_table_release(table);
            return 0;
        }
        memcpy(row, &i, sizeof(i));
        if (hooks->row_finalize != NULL)
            hooks->row_finalize(hooks->context, row, row_kind);
        length = strlen(source_name);
        names[i] = (char*)engine_heap_alloc((long)(length + 1u));
        if (names[i] == NULL) {
            type_config_table_release(table);
            return 0;
        }
        memcpy(names[i], source_name, length + 1u);
    }
    return 1;
}

/* Keep the older preallocated-table adapter used by the separate row-parser
 * wrappers. Its caller owns both buffers and its result is the loaded count. */
static inline int type_config_table_load(TypeConfigTable* table,
                                         void* section,
                                         uint32_t row_kind,
                                         const TypeConfigHooks* hooks)
{
    uint32_t count;
    uint32_t i;
    char* rows;
    if (table == NULL || hooks == NULL || hooks->entry_count == NULL ||
        hooks->entry_get == NULL || hooks->row_parse == NULL ||
        table->rows == NULL || table->names == NULL ||
        table->row_stride == 0u) return 0;
    count = hooks->entry_count(hooks->context, section);
    if (count == 0u || count > table->capacity) {
        table->count = 0u;
        return 0;
    }
    table->count = 0u;
    rows = (char*)table->rows;
    for (i = 0u; i < count; ++i) {
        const char* name = NULL;
        void* entry = NULL;
        void* row = rows + (size_t)i * table->row_stride;
        memset(row, 0, table->row_stride);
        if (!hooks->entry_get(hooks->context, section, i, &name, &entry) ||
            !hooks->row_parse(hooks->context, entry, row, row_kind)) {
            table->count = 0u;
            return 0;
        }
        table->names[i] = (char*)name;
        if (hooks->row_finalize != NULL)
            hooks->row_finalize(hooks->context, row, row_kind);
        table->count = i + 1u;
    }
    return (int)table->count;
}

typedef struct ObjectNameTableSet {
    const char** type_names[5];
    uint32_t counts[5];
    void*** output_values[5];
    void** named_output_values[9];
} ObjectNameTableSet;

typedef struct ObjectNameLoadHooks {
    const char* (*value_find)(void* context, void* section,
                              const char* type_name);
    void* (*value_resolve)(void* context, const char* value);
    void (*named_value_set)(void* context, const char* name, void* value);
    void* context;
} ObjectNameLoadHooks;

/* Shared by ObjectNamesLoad and ObjectTheNamesLoad. The five parallel table
 * groups are vehicle, mini-figure, monster, building, and upgrade. The
 * reference intentionally zeroes each upgrade slot after attempting to
 * resolve it. Named handles are updated only when their key exists. */
static inline void object_names_load(ObjectNameTableSet* tables,
                                     void* section,
                                     const ObjectNameLoadHooks* hooks)
{
    static const char* named_keys[9] = {
        "Ore", "PowerCrystal", "ProcessedOre", "Dynamite", "Barrier",
        "ElectricFence", "SpiderWeb", "OohScary", "Path"
    };
    uint32_t i;
    uint32_t j;
    if (tables == NULL || hooks == NULL) return;
    for (i = 0u; i < 5u; ++i) {
        uint32_t count = tables->counts[i];
        void** values;
        if (tables->output_values[i] == NULL) continue;
        values = (void**)malloc((size_t)count * sizeof(void*));
        *tables->output_values[i] = values;
        if (count != 0u && values == NULL) continue;
        for (j = 0u; j < count; ++j) {
            const char* key = tables->type_names[i] != NULL
                ? tables->type_names[i][j] : NULL;
            const char* value = hooks->value_find != NULL && key != NULL
                ? hooks->value_find(hooks->context, section, key) : NULL;
            void* resolved = NULL;
            if (i != 4u || value != NULL) {
                resolved = hooks->value_resolve != NULL
                    ? hooks->value_resolve(hooks->context, value) : NULL;
            }
            values[j] = resolved;
        }
    }
    for (i = 0u; i < 9u; ++i) {
        const char* value = hooks->value_find != NULL
            ? hooks->value_find(hooks->context, section, named_keys[i]) : NULL;
        if (value != NULL) {
            void* resolved = hooks->value_resolve != NULL
                ? hooks->value_resolve(hooks->context, value) : NULL;
            if (tables->named_output_values[i] != NULL)
                *tables->named_output_values[i] = resolved;
            if (hooks->named_value_set != NULL) {
                hooks->named_value_set(hooks->context, named_keys[i], resolved);
            }
        }
    }
}

#endif
