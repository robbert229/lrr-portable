// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stdlib.h>
#include <string.h>

#include "entity.hpp"
#include "frontend.hpp"
#include "type_config_ports.hpp"
#include "gods98/entity.hpp"
#include "gods98/block.hpp"
#include "gods98/heap.hpp"
#include "gods98/string.hpp"

static TypeConfigTable s_minifigure_type_config;
static TypeConfigTable s_building_type_config;
static BlockFile* s_entity_type_config_file;

const TypeConfigTable* entity_type_config_table_get(uint32_t row_kind)
{
    if (row_kind == 1u) return &s_minifigure_type_config;
    if (row_kind == 2u) return &s_building_type_config;
    return NULL;
}

static uint32_t entity_type_config_entry_count(void* context, void* section)
{
    BlockRec* child;
    uint32_t count = 0u;
    (void)context;
    for (child = block_first_child((BlockRec*)section); child != NULL;
         child = block_next_sibling(child)) ++count;
    return count;
}

static int entity_type_config_entry_get(void* context, void* section,
                                        uint32_t index, const char** name,
                                        void** entry)
{
    BlockRec* child = block_first_child((BlockRec*)section);
    (void)context;
    while (child != NULL && index != 0u) {
        child = block_next_sibling(child);
        --index;
    }
    if (child == NULL) return 0;
    if (name != NULL) *name = block_name(child);
    if (entry != NULL) *entry = child;
    return 1;
}

static EntityTypeConfigRuntimeOps s_entity_type_runtime_ops;

void entity_type_config_runtime_ops_install(
    const EntityTypeConfigRuntimeOps* ops)
{
    if (ops != NULL) s_entity_type_runtime_ops = *ops;
    else memset(&s_entity_type_runtime_ops, 0, sizeof(s_entity_type_runtime_ops));
}

static void entity_type_config_word_set(void* row, uint32_t offset,
                                        uint32_t value)
{
    memcpy((char*)row + offset, &value, sizeof(value));
}

static void entity_type_config_float_set(void* row, uint32_t offset,
                                         float value)
{
    memcpy((char*)row + offset, &value, sizeof(value));
}

/* BlockTextTokenSplit (FUN_00477440) keeps empty tokens and does not split
 * when the delimiter is the final byte. Shape has room for the empty pair
 * plus nine source tokens in its reference 0x50-byte table. */
static int entity_type_config_shape_parse(void* row, const char* shape_text,
                                          void** owned_table_out)
{
    char* copy;
    char* token;
    char* cursor;
    char* shape_table;
    uint32_t count = 1u;
    size_t length;

    shape_table = (char*)engine_heap_alloc(0x50L);
    if (owned_table_out != NULL) *owned_table_out = shape_table;
    entity_type_config_word_set(row, 0x84u, (uint32_t)(size_t)shape_table);
    entity_type_config_word_set(row, 0x88u, 1u);
    if (shape_table == NULL) return 0;
    memset(shape_table, 0, 0x50u);
    if (shape_text == NULL || *shape_text == '\0') return 1;

    length = strlen(shape_text);
    copy = (char*)engine_heap_alloc((long)(length + 1u));
    if (copy == NULL) return 0;
    memcpy(copy, shape_text, length + 1u);

    token = copy;
    cursor = copy;
    for (;;) {
        int separated = 0;
        while (*cursor != '\0') {
            if (cursor[0] == ':' && cursor[1] != '\0') {
                *cursor = '\0';
                separated = 1;
                break;
            }
            ++cursor;
        }

        if (count >= 10u) {
            engine_heap_free(copy);
            return 0;
        }
        {
            char* comma = (char*)string_find_substring(token, ",");
            char* full = string_duplicate(token);
            if (full == NULL) {
                engine_heap_free(copy);
                return 0;
            }
            ((char**)shape_table)[count * 2u] = full;
            ++count;
            entity_type_config_word_set(row, 0x88u, count);
            if (comma == NULL) {
                engine_heap_free(copy);
                return 0;
            }
            ((char**)shape_table)[count * 2u - 1u] =
                string_duplicate(comma + 1);
            if (((char**)shape_table)[count * 2u - 1u] == NULL) {
                engine_heap_free(copy);
                return 0;
            }
        }

        if (!separated) break;
        token = cursor + 1;
        ++cursor;
    }
    engine_heap_free(copy);
    return 1;
}

static void entity_type_config_string_append(char* destination,
                                              size_t capacity,
                                              const char* value)
{
    size_t used = strlen(destination);
    size_t i = 0u;
    if (used >= capacity) return;
    while (value[i] != '\0' && used + 1u < capacity) {
        destination[used++] = value[i++];
    }
    destination[used] = '\0';
}

static void entity_type_config_key_path(char path[256], const char* key)
{
    strcpy(path, "Lego* ");
    entity_type_config_string_append(path, 256u, key != NULL ? key : "");
}

static uint32_t entity_type_config_named_get(uint32_t scene,
                                             const char* name, int frames)
{
    if (s_entity_type_runtime_ops.named_resource_get == NULL) return 0u;
    return s_entity_type_runtime_ops.named_resource_get(
        s_entity_type_runtime_ops.context, scene, name, frames);
}

static uint32_t entity_type_config_frames(BlockFile* ae, const char* key)
{
    char path[256];
    const char* value;
    if (key == NULL || *key == '\0') return 0u;
    entity_type_config_key_path(path, key);
    value = block_get_strptr(ae, path);
    if (value == NULL) return (uint32_t)block_get_int(ae, path, 0);
    return (uint32_t)atoi(value);
}

static const char* entity_type_config_ae_value(BlockFile* ae,
                                                const char* key)
{
    char path[256];
    if (key == NULL) return NULL;
    entity_type_config_key_path(path, key);
    return block_get_strptr(ae, path);
}

/* Concrete row parser for the two table families in Finding 409. The
 * renderer-facing handles are opaque 32-bit backend tokens supplied by the
 * installed runtime adapter; config text and numeric fields are parsed here. */
int entity_type_config_row_parse(void* context, void* entry,
                                 void* destination, uint32_t row_kind)
{
    BlockRec* config_entry = (BlockRec*)entry;
    const char* asset_path;
    char ae_path[520];
    const char* base;
    BlockFile* ae;
    uint32_t scene = 0u;
    if (config_entry == NULL || destination == NULL ||
        (row_kind != 1u && row_kind != 2u)) return 0;
    asset_path = block_value(config_entry);
    if (asset_path == NULL || *asset_path == '\0') return 0;

    base = strrchr(asset_path, '\\');
    if (base == NULL) base = strrchr(asset_path, '/');
    base = base != NULL ? base + 1 : asset_path;
    ae_path[0] = '\0';
    entity_type_config_string_append(ae_path, sizeof(ae_path), asset_path);
    entity_type_config_string_append(ae_path, sizeof(ae_path), "\\");
    entity_type_config_string_append(ae_path, sizeof(ae_path), base);
    entity_type_config_string_append(ae_path, sizeof(ae_path), ".ae");
    ae = block_open(ae_path);
    if (ae == NULL) return 0;
    /* These records are only valid once the renderer has built the required
     * scene. The portable backend currently has no .ae scene constructor;
     * accepting a row with a zero scene token would make later named-handle
     * and render-mode operations silently target nothing. */
    if (s_entity_type_runtime_ops.visual_load == NULL ||
        !s_entity_type_runtime_ops.visual_load(
            s_entity_type_runtime_ops.context, asset_path, &scene) ||
        scene == 0u) {
        block_close(ae);
        return 0;
    }

    if (row_kind == 1u) {
        uint32_t i;
        uint32_t camera_frames;
        entity_type_config_word_set(destination, 0u, 0u);
        entity_type_config_word_set(destination, 4u, scene);
        camera_frames = entity_type_config_frames(ae, "CameraNullFrames");
        entity_type_config_word_set(destination, 36u, camera_frames);
        entity_type_config_word_set(destination, 32u,
            entity_type_config_named_get(scene,
                entity_type_config_ae_value(ae, "CameraNullName"), 0));
        entity_type_config_word_set(destination, 40u,
            entity_type_config_named_get(scene,
                entity_type_config_ae_value(ae, "CameraFlipDir"), 0));
        entity_type_config_word_set(destination, 44u,
            entity_type_config_named_get(scene,
                entity_type_config_ae_value(ae, "DrillNullName"), 0));
        entity_type_config_word_set(destination, 48u,
            entity_type_config_named_get(scene,
                entity_type_config_ae_value(ae, "FootStepNullName"), 0));
        entity_type_config_word_set(destination, 52u,
            entity_type_config_named_get(scene,
                entity_type_config_ae_value(ae, "CarryNullName"), 0));
        entity_type_config_word_set(destination, 56u,
            entity_type_config_named_get(scene,
                entity_type_config_ae_value(ae, "ThrowNullName"), 0));
        entity_type_config_word_set(destination, 60u,
            entity_type_config_named_get(scene,
                entity_type_config_ae_value(ae, "DepositNullName"), 0));
        if (s_entity_type_runtime_ops.poly_variant_build != NULL) {
            static const char* const poly_keys[3] = {
                "MediumPoly", "HighPoly", "FPPoly::Camera%i"
            };
            for (i = 0u; i < 3u; ++i) {
                uint32_t token = s_entity_type_runtime_ops.poly_variant_build(
                    s_entity_type_runtime_ops.context, scene, asset_path,
                    poly_keys[i], i + 1u,
                    i == 2u ? camera_frames : 1u);
                entity_type_config_word_set(destination, 100u + i * 4u,
                                            token);
            }
        }
        entity_type_config_word_set(destination, 112u, 1u);
    }
    else {
        const char* keys[8] = {
            "CarryNullName", "CameraNullName", "ToolNullName",
            "DepositNullName", "EntranceNullName", "FireNullName",
            "yPivot", "xPivot"
        };
        uint32_t offsets[8] = { 8u, 12u, 24u, 16u, 20u, 28u, 32u, 36u };
        uint32_t i;
        entity_type_config_word_set(destination, 4u, scene);
        for (i = 0u; i < 6u; ++i) {
            const char* key_value = entity_type_config_ae_value(ae, keys[i]);
            entity_type_config_word_set(destination, offsets[i],
                entity_type_config_named_get(scene, key_value, 0));
        }
        if (s_entity_type_runtime_ops.configured_value_token != NULL) {
            entity_type_config_word_set(destination, 32u,
                s_entity_type_runtime_ops.configured_value_token(
                    s_entity_type_runtime_ops.context,
                    entity_type_config_ae_value(ae, "yPivot")));
            entity_type_config_word_set(destination, 36u,
                s_entity_type_runtime_ops.configured_value_token(
                    s_entity_type_runtime_ops.context,
                    entity_type_config_ae_value(ae, "xPivot")));
        }
        entity_type_config_word_set(destination, 112u,
            entity_type_config_frames(ae, "CarryNullFrames"));
        entity_type_config_word_set(destination, 116u,
            entity_type_config_frames(ae, "CameraNullFrames"));
        entity_type_config_word_set(destination, 120u,
            entity_type_config_frames(ae, "ToolNullFrames"));
        entity_type_config_float_set(destination, 208u,
            block_get_float(ae, "Lego* PivotMaxZ", 1.0f));
        entity_type_config_float_set(destination, 212u,
            block_get_float(ae, "Lego* PivotMinZ", -1.0f));
        {
            const char* power_path = entity_type_config_ae_value(
                ae, "PowerLevelScene");
            uint32_t power_scene = 0u;
            if (power_path != NULL && *power_path != '\0' &&
                s_entity_type_runtime_ops.visual_load != NULL &&
                s_entity_type_runtime_ops.visual_load(
                    s_entity_type_runtime_ops.context, power_path,
                    &power_scene)) {
                entity_type_config_word_set(destination, 124u, power_scene);
            }
        }
        void** owned_shape = NULL;
        TypeConfigTable* table = (TypeConfigTable*)context;
        if (table != NULL && table->row_owned != NULL &&
            table->rows != NULL && table->row_stride != 0u) {
            size_t row_offset = (size_t)((char*)destination -
                                          (char*)table->rows);
            size_t row_index = row_offset / table->row_stride;
            if (row_offset % table->row_stride == 0u &&
                row_index < table->capacity)
                owned_shape = &table->row_owned[row_index];
        }
        if (!entity_type_config_shape_parse(destination,
                entity_type_config_ae_value(ae, "Shape"), owned_shape)) {
            block_close(ae);
            return 0;
        }
    }
    block_close(ae);
    return 1;
}

void entity_type_config_row_finalize(void* context, void* row,
                                     uint32_t row_kind)
{
    uint32_t scene = 0u;
    (void)context;
    (void)row_kind;
    if (row == NULL || s_entity_type_runtime_ops.scene_apply_mode == NULL)
        return;
    memcpy(&scene, (char*)row + 4u, sizeof(scene));
    if (scene != 0u)
        s_entity_type_runtime_ops.scene_apply_mode(
            s_entity_type_runtime_ops.context, scene, 1);
    if (row_kind == 2u) {
        memcpy(&scene, (char*)row + 124u, sizeof(scene));
        if (scene != 0u)
            s_entity_type_runtime_ops.scene_apply_mode(
                s_entity_type_runtime_ops.context, scene, 1);
    }
}

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
    section = block_subgroup(s_entity_type_config_file, (section_name_)); \
    type_config_table_release(table); \
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
            type_config_table_release(table); return 0; \
        } \
        memcpy(row, &i, sizeof(i)); \
        entity_type_config_row_finalize(table, row, (kind_)); \
        source_name = block_name(child); \
        if (source_name == NULL) { \
            type_config_table_release(table); return 0; \
        } \
        name_length = strlen(source_name); \
        names[i] = (char*)engine_heap_alloc((long)(name_length + 1u)); \
        if (names[i] == NULL) { \
            type_config_table_release(table); return 0; \
        } \
        memcpy(names[i], source_name, name_length + 1u); \
        child = block_next_sibling(child); \
    } \
    return 1

/* MiniFigureTypeTableLoad (FUN_0042cf50) reads the active engine config,
 * builds 0x74-byte rows, finalizes each parsed row, and stores copied names. */
// FUNCTION: LEGORR 0x0042CF50
int entity_minifigure_type_table_load(void)
{
    ENTITY_TYPE_CONFIG_TABLE_LOAD(&s_minifigure_type_config,
                                  "MiniFigureTypes", 1u, 0x74u);
}

/* BuildingTypesLoad (FUN_0042d2b0) uses the same config walk with 0x14c-byte
 * rows and the building-specific `.ae` parser/finalizer. */
// FUNCTION: LEGORR 0x0042D2B0
int entity_building_types_load(void)
{
    ENTITY_TYPE_CONFIG_TABLE_LOAD(&s_building_type_config,
                                  "BuildingTypes", 2u, 0x14cu);
}

#undef ENTITY_TYPE_CONFIG_TABLE_LOAD

static int entity_config_name_equal(const char* left, const char* right)
{
    if (left == NULL || right == NULL) return 0;
    while (*left != '\0' && *right != '\0') {
        int a = (unsigned char)*left++;
        int b = (unsigned char)*right++;
        if (a >= 'A' && a <= 'Z') a += 'a' - 'A';
        if (b >= 'A' && b <= 'Z') b += 'a' - 'A';
        if (a != b) return 0;
    }
    return *left == '\0' && *right == '\0';
}

static const char* entity_config_value_find(void* context, void* section,
                                            const char* type_name)
{
    BlockRec* child = block_first_child((BlockRec*)section);
    (void)context;
    while (child != NULL) {
        if (entity_config_name_equal(block_name(child), type_name))
            return block_value(child);
        child = block_next_sibling(child);
    }
    return NULL;
}

typedef struct EntityNameResolveContext {
    char named_values[9][256];
    char named_the_values[9][256];
    int the_names;
} EntityNameResolveContext;

static void* entity_config_value_resolve(void* context, const char* value)
{
    (void)context;
    return value != NULL
        ? formatted_text_escape_decode_and_emit(value) : NULL;
}

static int entity_config_named_index(const char* name)
{
    static const char* const names[9] = {
        "Ore", "PowerCrystal", "ProcessedOre", "Dynamite", "Barrier",
        "ElectricFence", "SpiderWeb", "OohScary", "Path"
    };
    int i;
    for (i = 0; i < 9; ++i) {
        if (entity_config_name_equal(name, names[i])) return i;
    }
    return -1;
}

static void entity_config_named_value_set(void* context, const char* name,
                                          void* value)
{
    EntityNameResolveContext* state = (EntityNameResolveContext*)context;
    int index = entity_config_named_index(name);
    if (state == NULL || index < 0) return;
    if (state->the_names) {
        if (value == NULL) {
            state->named_the_values[index][0] = '\0';
            return;
        }
        strncpy(state->named_the_values[index], (const char*)value,
                sizeof(state->named_the_values[index]) - 1u);
        state->named_the_values[index][
            sizeof(state->named_the_values[index]) - 1u] = '\0';
    }
    else {
        if (value == NULL) {
            state->named_values[index][0] = '\0';
            return;
        }
        strncpy(state->named_values[index], (const char*)value,
                sizeof(state->named_values[index]) - 1u);
        state->named_values[index][
            sizeof(state->named_values[index]) - 1u] = '\0';
    }
}

static void entity_config_name_table_build(BlockFile* cfg,
                                           const char* section_name,
                                           const char** names,
                                           uint32_t* count)
{
    BlockRec* section = block_subgroup(cfg, section_name);
    BlockRec* child;
    *count = 0u;
    for (child = block_first_child(section);
         child != NULL && *count < 32u;
         child = block_next_sibling(child)) {
        names[*count] = block_name(child);
        ++*count;
    }
}

static void entity_config_values_release(void** values, uint32_t count)
{
    uint32_t i;
    if (values == NULL) return;
    for (i = 0u; i < count; ++i) engine_heap_free(values[i]);
    free(values);
}

typedef struct EntityObjectNameLoadState {
    ObjectNameTableSet* tables;
    ObjectNameLoadHooks* hooks;
} EntityObjectNameLoadState;

static EntityObjectNameLoadState* s_entity_object_name_load_state;

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
    section = block_subgroup(config, (section_name_)); \
    tables = state->tables; \
    hooks = state->hooks; \
    for (i = 0u; i < 5u; ++i) { \
        uint32_t count = tables->counts[i]; \
        void** values; \
        uint32_t j; \
        if (tables->output_values[i] == NULL) continue; \
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
            values[j] = i == 4u ? NULL : resolved; \
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

/* ObjectNamesLoad (FUN_0042d600) receives the parent config handle. The
 * decompile builds ObjectNames-qualified keys and queries them against that
 * root; block_subgroup provides the equivalent scope for the portable tree. */
// FUNCTION: LEGORR 0x0042D600
void entity_object_names_load(BlockFile* config)
{
    ENTITY_OBJECT_NAMES_LOAD_BODY("ObjectNames");
}

/* ObjectTheNamesLoad (FUN_0042da20) has the same parent-config ABI and uses
 * the separate ObjectTheNames-qualified lookup namespace. */
// FUNCTION: LEGORR 0x0042DA20
void entity_object_the_names_load(BlockFile* config)
{
    ENTITY_OBJECT_NAMES_LOAD_BODY("ObjectTheNames");
}

#undef ENTITY_OBJECT_NAMES_LOAD_BODY

/* GameConfigInit's late ObjectNamesLoad/ObjectTheNamesLoad pair. The block
 * tree is the live Lego.cfg representation; copied display strings are
 * installed into entity's existing localized-name tables before temporary
 * loader allocations are released. */
void entity_names_config_load(BlockFile* cfg)
{
    static const char* const sections[5] = {
        "VehicleTypes", "MiniFigureTypes", "RockMonsterTypes",
        "BuildingTypes", "UpgradeTypes"
    };
    const char* names[5][32];
    uint32_t counts[5];
    void** object_values[5] = { NULL, NULL, NULL, NULL, NULL };
    void** the_values[5] = { NULL, NULL, NULL, NULL, NULL };
    void* named_values[9] = { NULL, NULL, NULL, NULL, NULL,
                              NULL, NULL, NULL, NULL };
    void* named_the_values[9] = { NULL, NULL, NULL, NULL, NULL,
                                  NULL, NULL, NULL, NULL };
    ObjectNameTableSet tables;
    ObjectNameLoadHooks hooks;
    EntityObjectNameLoadState object_load_state;
    static EntityNameResolveContext resolve_state;
    const char* special_names[16][2];
    const char* special_the_names[16][2];
    uint32_t i;

    if (cfg == NULL) return;
    s_entity_type_config_file = cfg;
    (void)entity_minifigure_type_table_load();
    (void)entity_building_types_load();
    memset(&tables, 0, sizeof(tables));
    memset(special_names, 0, sizeof(special_names));
    memset(special_the_names, 0, sizeof(special_the_names));
    for (i = 0u; i < 5u; ++i) {
        entity_config_name_table_build(cfg, sections[i], names[i], &counts[i]);
        tables.type_names[i] = names[i];
        tables.counts[i] = counts[i];
        tables.output_values[i] = &object_values[i];
    }
    for (i = 0u; i < 9u; ++i)
        tables.named_output_values[i] = &named_values[i];
    hooks.value_find = entity_config_value_find;
    hooks.value_resolve = entity_config_value_resolve;
    hooks.named_value_set = entity_config_named_value_set;
    hooks.context = &resolve_state;
    resolve_state.the_names = 0;
    object_load_state.tables = &tables;
    object_load_state.hooks = &hooks;
    s_entity_object_name_load_state = &object_load_state;

    entity_object_names_load(cfg);
    for (i = 0u; i < 9u; ++i)
        tables.named_output_values[i] = &named_the_values[i];
    for (i = 0u; i < 5u; ++i) tables.output_values[i] = &the_values[i];
    resolve_state.the_names = 1;
    entity_object_the_names_load(cfg);
    s_entity_object_name_load_state = NULL;

    for (i = 0u; i < 5u; ++i) {
        int kind = i < 4u ? (int)i + 1 : 10;
        entity_kind_name_tables_install(kind, names[i], counts[i]);
        entity_localized_name_tables_install(kind,
            (const char* const*)object_values[i],
            (const char* const*)the_values[i], counts[i]);
    }
    /* Named labels share kind 7's two configured rows; the remaining labels
     * each occupy the single configured row for their special kind. */
    {
        static const int kinds[9] = { 7, 6, 7, 8, 9, 11, 12, 13, 15 };
        static const int rows[9] = { 0, 0, 1, 0, 0, 0, 0, 0, 0 };
        for (i = 0u; i < 9u; ++i) {
            if (resolve_state.named_values[i][0] != '\0')
                special_names[kinds[i]][rows[i]] =
                    resolve_state.named_values[i];
            if (resolve_state.named_the_values[i][0] != '\0')
                special_the_names[kinds[i]][rows[i]] =
                    resolve_state.named_the_values[i];
        }
        entity_localized_name_tables_install(6, special_names[6],
                                               special_the_names[6], 1u);
        entity_localized_name_tables_install(7, special_names[7],
                                               special_the_names[7], 2u);
        entity_localized_name_tables_install(8, special_names[8],
                                               special_the_names[8], 1u);
        entity_localized_name_tables_install(9, special_names[9],
                                               special_the_names[9], 1u);
        entity_localized_name_tables_install(11, special_names[11],
                                                special_the_names[11], 1u);
        entity_localized_name_tables_install(12, special_names[12],
                                                special_the_names[12], 1u);
        entity_localized_name_tables_install(13, special_names[13],
                                                special_the_names[13], 1u);
        entity_localized_name_tables_install(15, special_names[15],
                                                special_the_names[15], 1u);
    }
    for (i = 0u; i < 5u; ++i) {
        entity_config_values_release(object_values[i], counts[i]);
        entity_config_values_release(the_values[i], counts[i]);
    }
    for (i = 0u; i < 9u; ++i) engine_heap_free(named_values[i]);
    for (i = 0u; i < 9u; ++i) engine_heap_free(named_the_values[i]);
}
