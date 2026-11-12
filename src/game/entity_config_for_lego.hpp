// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what lego uses from entity_config.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GAME_ENTITY_CONFIG_FOR_LEGO_HPP
#define LRR_GAME_ENTITY_CONFIG_FOR_LEGO_HPP

#define ENTITY_TYPE_CONFIG_TABLE_LOAD(table_, section_name_, kind_, stride_) \
    TypeConfigTable* table = (table_); \
    BlockRec* section; \
    BlockRec* child; \
    uint32_t count; \
    uint32_t i; \
    char* rows; \
    char** names; \
    void** row_owned; \
    if (table == NULL || s_entity_type_config_file == NULL) return 0; \
    section = entity_config_section_find(s_entity_type_config_file, (section_name_)); \
    entity_type_config_family_release(table, (kind_)); \
    count = entity_type_config_entry_count(NULL, section); \
    table->row_stride = (stride_); \
    if (count == 0u || count > 0xffffffffu / (stride_) || \
        count > 0xffffffffu / (uint32_t)sizeof(char*)) return 0; \
    rows = (char*)engine_heap_alloc((long)((size_t)count * (stride_))); \
    if (rows == NULL) return 0; \
    names = (char**)engine_heap_alloc((long)((size_t)count * sizeof(char*))); \
    if (names == NULL) { engine_heap_free(rows); return 0; } \
    memset(names, 0, (size_t)count * sizeof(char*)); \
    row_owned = (void**)calloc((size_t)count, sizeof(void*)); \
    if (row_owned == NULL) { \
        engine_heap_free(names); engine_heap_free(rows); return 0; \
    } \
    table->rows = rows; table->names = names; \
    table->row_owned = row_owned; table->capacity = count; \
    child = block_first_child(section); \
    for (i = 0u; i < count; ++i) { \
        const char* source_name; \
        size_t name_length; \
        char* row = rows + (size_t)i * (stride_); \
        memset(row, 0, (stride_)); \
        table->count = i + 1u; \
        if (child == NULL || \
            !entity_type_config_row_parse(table, child, row, (kind_))) { \
            entity_type_config_family_release(table, (kind_)); return 0; \
        } \
        memcpy(row, &i, sizeof(i)); \
        entity_type_config_row_finalize(table, row, (kind_)); \
        source_name = block_name(child); \
        if (source_name == NULL) { \
            entity_type_config_family_release(table, (kind_)); return 0; \
        } \
        name_length = strlen(source_name); \
        names[i] = (char*)engine_heap_alloc((long)(name_length + 1u)); \
        if (names[i] == NULL) { \
            entity_type_config_family_release(table, (kind_)); return 0; \
        } \
        memcpy(names[i], source_name, name_length + 1u); \
        child = block_next_equal_key(child); \
    } \
    return 1

typedef struct EntityObjectNameLoadState {
    ObjectNameTableSet* tables;
    ObjectNameLoadHooks* hooks;
} EntityObjectNameLoadState;

#define ENTITY_OBJECT_NAMES_LOAD_BODY(section_name_) \
    static const char* const named_keys[9] = { \
        "Ore", "PowerCrystal", "ProcessedOre", "Dynamite", "Barrier", \
        "ElectricFence", "SpiderWeb", "OohScary", "Path" \
    }; \
    BlockRec* section; \
    EntityObjectNameLoadState* state = s_entity_object_name_load_state; \
    ObjectNameTableSet* tables; \
    ObjectNameLoadHooks* hooks; \
    uint32_t i; \
    if (state == NULL || state->tables == NULL || state->hooks == NULL || \
        config == NULL) return; \
    section = entity_config_section_find(config, (section_name_)); \
    tables = state->tables; \
    hooks = state->hooks; \
    for (i = 0u; i < 5u; ++i) { \
        uint32_t count = tables->counts[i]; \
        void** values; \
        uint32_t j; \
        if (tables->output_values[i] == NULL) continue; \
        if ((size_t)count > ((size_t)-1) / sizeof(void*)) { \
            *tables->output_values[i] = NULL; \
            continue; \
        } \
        values = (void**)malloc((size_t)count * sizeof(void*)); \
        *tables->output_values[i] = values; \
        if (count != 0u && values == NULL) continue; \
        for (j = 0u; j < count; ++j) { \
            const char* key = tables->type_names[i] != NULL \
                ? tables->type_names[i][j] : NULL; \
            const char* value = hooks->value_find != NULL && key != NULL \
                ? hooks->value_find(hooks->context, section, key) : NULL; \
            void* resolved = NULL; \
            if (i != 4u || value != NULL) { \
                resolved = hooks->value_resolve != NULL \
                    ? hooks->value_resolve(hooks->context, value) : NULL; \
            } \
            /* ObjectNamesLoad/ObjectTheNamesLoad resolve each upgrade's
             * configured label, then immediately clear the just-written
             * slot in the retail loop. Keep the observed table contents
             * (all upgrade labels NULL) without retaining the discarded
             * temporary string. */ \
            if (i == 4u) { \
                engine_heap_free(resolved); \
                resolved = NULL; \
            } \
            values[j] = resolved; \
        } \
    } \
    for (i = 0u; i < 9u; ++i) { \
        const char* value = hooks->value_find != NULL \
            ? hooks->value_find(hooks->context, section, named_keys[i]) : NULL; \
        if (value != NULL) { \
            void* resolved = hooks->value_resolve != NULL \
                ? hooks->value_resolve(hooks->context, value) : NULL; \
            if (tables->named_output_values[i] != NULL) \
                *tables->named_output_values[i] = resolved; \
            if (hooks->named_value_set != NULL) \
                hooks->named_value_set(hooks->context, named_keys[i], resolved); \
        } \
    }

uint32_t entity_type_config_entry_count(void* context, void* section);
extern EntityObjectNameLoadState* s_entity_object_name_load_state;
BlockRec* entity_config_section_find(BlockFile* config, const char* section_name);
extern BlockFile* s_entity_type_config_file;
void entity_type_config_family_release(TypeConfigTable* table, uint32_t kind);

#endif /* LRR_GAME_ENTITY_CONFIG_FOR_LEGO_HPP */
