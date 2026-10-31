// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef LEGO_GAME_TYPE_CONFIG_PORTS_HPP
#define LEGO_GAME_TYPE_CONFIG_PORTS_HPP

#include <stdint.h>
#include <string.h>

typedef struct TypeConfigTable {
    void* rows;
    const char** names;
    uint32_t count;
    uint32_t capacity;
    uint32_t row_stride;
} TypeConfigTable;

typedef struct TypeConfigHooks {
    uint32_t (*entry_count)(void* context, void* section);
    int (*entry_get)(void* context, void* section, uint32_t index,
                     const char** name, void** entry);
    int (*row_parse)(void* context, void* entry, void* destination,
                     uint32_t row_kind);
    void (*row_finalize)(void* context, void* row, uint32_t row_kind);
    void (*table_clear)(void* context, TypeConfigTable* table);
    void* context;
} TypeConfigHooks;

static inline int type_config_table_load(TypeConfigTable* table,
                                         void* section,
                                         uint32_t row_kind,
                                         const TypeConfigHooks* hooks)
{
    uint32_t count;
    uint32_t i;
    char* rows;
    if (table == 0 || hooks == 0 || hooks->entry_count == 0 ||
        hooks->entry_get == 0 || hooks->row_parse == 0 ||
        table->rows == 0 || table->names == 0 || table->row_stride == 0)
        return 0;
    count = hooks->entry_count(hooks->context, section);
    if (count == 0 || count > table->capacity) {
        table->count = 0;
        if (hooks->table_clear != 0)
            hooks->table_clear(hooks->context, table);
        return 0;
    }
    table->count = 0;
    rows = (char*)table->rows;
    for (i = 0; i < count; ++i) {
        const char* name = 0;
        void* entry = 0;
        void* row = rows + i * table->row_stride;
        memset(row, 0, table->row_stride);
        if (!hooks->entry_get(hooks->context, section, i, &name, &entry) ||
            !hooks->row_parse(hooks->context, entry, row, row_kind)) {
            table->count = 0;
            if (hooks->table_clear != 0)
                hooks->table_clear(hooks->context, table);
            return 0;
        }
        table->names[i] = name;
        if (hooks->row_finalize != 0)
            hooks->row_finalize(hooks->context, row, row_kind);
        table->count = i + 1;
    }
    return (int)table->count;
}

typedef struct ObjectNameTableSet {
    const char** type_names[5];
    uint32_t counts[5];
    void** output_values[5];
} ObjectNameTableSet;

typedef struct ObjectNameLoadHooks {
    const char* (*value_find)(void* context, void* section,
                              const char* type_name);
    void* (*value_resolve)(void* context, const char* value);
    void (*named_value_set)(void* context, const char* name, void* value);
    void* context;
} ObjectNameLoadHooks;

static inline void object_names_load(ObjectNameTableSet* tables,
                                     void* section,
                                     const ObjectNameLoadHooks* hooks)
{
    static const char* named_keys[5] = {
        "PowerCrystal", "ProcessedOre", "Dynamite", "Barrier",
        "ElectricFence"
    };
    uint32_t i;
    uint32_t j;
    if (tables == 0 || hooks == 0) return;
    for (i = 0; i < 5; ++i) {
        uint32_t count = tables->counts[i];
        if (count > 4096) count = 4096;
        if (tables->output_values[i] == 0) continue;
        for (j = 0; j < count; ++j) {
            const char* value = hooks->value_find != 0 &&
                tables->type_names[i] != 0
                ? hooks->value_find(hooks->context, section,
                                    tables->type_names[i][j]) : 0;
            tables->output_values[i][j] = hooks->value_resolve != 0
                ? hooks->value_resolve(hooks->context, value) : 0;
        }
    }
    if (hooks->named_value_set != 0) {
        for (i = 0; i < 5; ++i) {
            const char* value = hooks->value_find != 0
                ? hooks->value_find(hooks->context, section, named_keys[i])
                : 0;
            hooks->named_value_set(hooks->context, named_keys[i],
                hooks->value_resolve != 0
                    ? hooks->value_resolve(hooks->context, value) : 0);
        }
    }
}

#endif
