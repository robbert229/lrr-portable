// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stdlib.h>
#include <string.h>

#include "entity.hpp"
#include "building_scene.hpp"
#include "actor_scene.hpp"
#include "crt/crt.hpp"
#include "frontend.hpp"
#include "cmd.hpp"
#include "type_config_ports.hpp"
#include "gods98/entity.hpp"
#include "gods98/block.hpp"
#include "gods98/heap.hpp"
#include "gods98/sound.hpp"
#include "gods98/string.hpp"
#include "util.hpp"

static TypeConfigTable s_minifigure_type_config;
static TypeConfigTable s_building_type_config;
static TypeConfigTable s_vehicle_type_config;
static TypeConfigTable s_rockmonster_type_config;
static BlockFile* s_entity_type_config_file;

/* These loaders receive the complete game config. Retail builds each key
 * beneath its actual root; BlockFile retains that root in its hierarchy. */
static BlockRec* entity_config_section_find(BlockFile* config,
                                             const char* section_name)
{
    char path[512];
    const char* root;
    size_t root_length;
    size_t section_length;
    if (config == NULL || section_name == NULL) return NULL;
    root = util_root_name(config);
    root_length = strlen(root);
    section_length = strlen(section_name);
    if (root_length + section_length + 2u > sizeof(path)) return NULL;
    memcpy(path, root, root_length);
    path[root_length] = ' ';
    memcpy(path + root_length + 1u, section_name, section_length + 1u);
    return block_subgroup(config, path);
}

const TypeConfigTable* entity_type_config_table_get(uint32_t row_kind)
{
    if (row_kind == 1u) return &s_minifigure_type_config;
    if (row_kind == 2u) return &s_building_type_config;
    if (row_kind == 3u) return &s_rockmonster_type_config;
    if (row_kind == 4u) return &s_vehicle_type_config;
    return NULL;
}

const TypeConfigTable* entity_type_config_table_for_entity_type(int type)
{
    if (type == 1) return &s_vehicle_type_config;
    if (type == 2) return &s_minifigure_type_config;
    if (type == 3) return &s_rockmonster_type_config;
    if (type == 4) return &s_building_type_config;
    return NULL;
}

static uint32_t entity_type_config_entry_count(void* context, void* section)
{
    BlockRec* child;
    uint32_t count = 0u;
    (void)context;
    for (child = block_first_child((BlockRec*)section); child != NULL;
         child = block_next_equal_key(child)) ++count;
    return count;
}

static int entity_type_config_entry_get(void* context, void* section,
                                        uint32_t index, const char** name,
                                        void** entry)
{
    BlockRec* child = block_first_child((BlockRec*)section);
    (void)context;
    while (child != NULL && index != 0u) {
        child = block_next_equal_key(child);
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
    EntityActivityRunnerResourceOps runner_ops;
    EntitySceneSlotRuntimeOps scene_slot_ops;
    if (ops != NULL) s_entity_type_runtime_ops = *ops;
    else memset(&s_entity_type_runtime_ops, 0, sizeof(s_entity_type_runtime_ops));
    memset(&runner_ops, 0, sizeof(runner_ops));
    if (ops != NULL) {
        runner_ops.context = ops->context;
        runner_ops.scene_clone = ops->activity_scene_clone;
        runner_ops.scene_release = ops->activity_scene_release;
        runner_ops.scene_owner_bind = ops->activity_scene_owner_bind;
        runner_ops.resource_chain_clone =
            ops->activity_resource_chain_clone;
        runner_ops.resource_chain_release =
            ops->activity_resource_chain_release;
        runner_ops.indexed_resource_resolve =
            ops->activity_indexed_resource_resolve;
        runner_ops.scene_time_update = ops->activity_scene_time_update;
        runner_ops.scene_component_gate_read =
            ops->activity_scene_component_gate_read;
    }
    entity_activity_runner_resource_ops_set(&runner_ops);
    memset(&scene_slot_ops, 0, sizeof(scene_slot_ops));
    if (ops != NULL) {
        scene_slot_ops.context = ops->context;
        scene_slot_ops.renderer_position_prepare =
            ops->scene_slot_renderer_position_prepare;
        scene_slot_ops.renderer_state_apply =
            ops->scene_slot_renderer_state_apply;
        scene_slot_ops.node_animation_select =
            ops->scene_slot_node_animation_select;
        scene_slot_ops.resource_slot_register =
            ops->scene_slot_resource_register;
    }
    entity_scene_slot_runtime_ops_set(&scene_slot_ops);
}

/* SceneEntitySlotTableRebuild's scene-node owner is supplied by the renderer
 * that loaded the type's visual resource. Resolve only the scene token from
 * the retained type row here; the renderer callback creates native-width
 * nodes and installs the owner keys without storing pointers in config
 * dwords. */
int entity_type_config_scene_slot_nodes_load(EntitySlot* entity)
{
    const TypeConfigTable* table;
    const uint8_t* row_data;
    uint32_t scene_token;
    int config_type;
    int config_row;
    if (entity == NULL || entity->type < 1 || entity->type > 4 ||
        entity->type_name[0] == '\0' ||
        s_entity_type_runtime_ops.scene_slot_nodes_load == NULL ||
        !entity_type_row_for_name(entity->type_name, &config_type,
                                  &config_row) ||
        config_type != entity->type || config_row < 0) return 0;
    table = entity_type_config_table_for_entity_type(entity->type);
    if (table == NULL || table->rows == NULL || table->row_stride < 8u ||
        (uint32_t)config_row >= table->count) return 0;
    row_data = (const uint8_t*)table->rows +
               (size_t)(uint32_t)config_row * table->row_stride;
    memcpy(&scene_token, row_data + 4u, sizeof(scene_token));
    if (scene_token == 0u) return 0;
    s_entity_type_runtime_ops.scene_slot_nodes_load(
        s_entity_type_runtime_ops.context, entity, scene_token);
    return 1;
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
                                          TypeConfigTable* table,
                                          uint32_t row_index)
{
    char* copy;
    char* token;
    char* cursor;
    TypeConfigShapeOwner* owner;
    uint32_t count = 1u;
    size_t length;

    if (table == NULL || table->row_owned == NULL ||
        row_index >= table->capacity || row_index == 0xffffffffu) return 0;
    owner = (TypeConfigShapeOwner*)calloc(1u, sizeof(*owner));
    if (owner == NULL) return 0;
    table->row_owned[row_index] = owner;
    entity_type_config_word_set(row, 0x84u, row_index + 1u);
    entity_type_config_word_set(row, 0x88u, 1u);
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
            owner->strings[owner->string_count] = full;
            ++owner->string_count;
            owner->entries[count * 2u] = owner->string_count;
            ++count;
            entity_type_config_word_set(row, 0x88u, count);
            if (comma == NULL) {
                engine_heap_free(copy);
                return 0;
            }
            owner->strings[owner->string_count] = string_duplicate(comma + 1);
            if (owner->strings[owner->string_count] == NULL) {
                engine_heap_free(copy);
                return 0;
            }
            ++owner->string_count;
            owner->entries[count * 2u - 1u] = owner->string_count;
        }

        if (!separated) break;
        token = cursor + 1;
        ++cursor;
    }
    engine_heap_free(copy);
    return 1;
}

/* UpgradeLevelTableParse (FUN_0046c160) writes a four-word header at the
 * building row's +0x8c and five-word records in a 0x14-byte array. The
 * portable owner keeps the name/value pointers out of the retail dword
 * image while preserving its indices and counts. */
static int entity_type_config_building_upgrades_parse(
    void* row, TypeConfigTable* table, uint32_t row_index)
{
    TypeConfigShapeOwner* owner;
    uint32_t level;
    if (row == NULL || table == NULL || table->row_owned == NULL ||
        row_index >= table->count) return 0;
    owner = (TypeConfigShapeOwner*)table->row_owned[row_index];
    if (owner == NULL) return 0;
    owner->upgrade_records = (uint32_t*)engine_heap_alloc(4000L);
    owner->upgrade_values = (char**)calloc(200u, sizeof(char*));
    owner->upgrade_names = (const void**)calloc(200u, sizeof(void*));
    if (owner->upgrade_records == NULL || owner->upgrade_values == NULL ||
        owner->upgrade_names == NULL) {
        engine_heap_free(owner->upgrade_records);
        free(owner->upgrade_values);
        free(owner->upgrade_names);
        owner->upgrade_records = NULL;
        owner->upgrade_values = NULL;
        owner->upgrade_names = NULL;
        return 0;
    }
    memset(owner->upgrade_records, 0, 4000u);
    owner->upgrade_count = 0u;
    owner->upgrade_level_mask = 0u;
    for (level = 0u; level < 16u; ++level) {
        char path[48];
        BlockRec* section;
        BlockRec* entry;
        path[0] = 'U'; path[1] = 'p'; path[2] = 'g'; path[3] = 'r';
        path[4] = 'a'; path[5] = 'd'; path[6] = 'e'; path[7] = 's';
        path[8] = ' ';
        path[9] = 'L'; path[10] = 'e'; path[11] = 'v'; path[12] = 'e';
        path[13] = 'l';
        path[14] = (char)('0' + ((level >> 3) & 1u));
        path[15] = (char)('0' + ((level >> 2) & 1u));
        path[16] = (char)('0' + ((level >> 1) & 1u));
        path[17] = (char)('0' + (level & 1u));
        path[18] = 0;
        section = s_entity_type_config_file != NULL
            ? entity_config_section_find(s_entity_type_config_file, path) : NULL;
        if (section == NULL) continue;
        owner->upgrade_level_mask |= level;
        for (entry = block_first_child(section); entry != NULL;
             entry = block_next_equal_key(entry)) {
            const char* source = block_value(entry);
            const char* first_comma;
            const char* second_comma;
            size_t value_length;
            size_t third_length;
            char* value_copy;
            char* third_copy;
            char* second_copy;
            uint32_t* record;
            uint32_t ordinal;
            if (source == NULL || owner->upgrade_count >= 200u) continue;
            first_comma = strchr(source, ',');
            second_comma = first_comma != NULL
                ? strchr(first_comma + 1, ',') : NULL;
            if (first_comma == NULL || second_comma == NULL ||
                strchr(second_comma + 1, ',') != NULL) continue;
            value_length = (size_t)(second_comma - source);
            third_length = strlen(second_comma + 1);
            if (value_length >= 256u || third_length >= 256u) continue;
            value_copy = (char*)engine_heap_alloc((long)(value_length + 1u));
            third_copy = (char*)engine_heap_alloc((long)(third_length + 1u));
            second_copy = (char*)engine_heap_alloc(
                (long)((size_t)(first_comma - source) + 1u));
            if (value_copy == NULL || third_copy == NULL ||
                second_copy == NULL) {
                engine_heap_free(value_copy);
                engine_heap_free(third_copy);
                engine_heap_free(second_copy);
                continue;
            }
            memcpy(second_copy, source, (size_t)(first_comma - source));
            second_copy[first_comma - source] = '\0';
            memcpy(value_copy, first_comma + 1, value_length -
                   (size_t)(first_comma - source) - 1u);
            value_copy[value_length -
                (size_t)(first_comma - source) - 1u] = '\0';
            memcpy(third_copy, second_comma + 1, third_length + 1u);
            record = owner->upgrade_records + owner->upgrade_count * 5u;
            record[0] = level;
            record[1] = (uint32_t)(atoi(value_copy) - 1);
            record[2] = (uint32_t)atoi(second_copy);
            ordinal = owner->upgrade_count;
            owner->upgrade_values[ordinal] = third_copy;
            owner->upgrade_names[ordinal] =
                upgrade_name_entry_find(block_name(entry));
            owner->upgrade_count = ordinal + 1u;
            record[3] = ordinal + 1u;
            record[4] = owner->upgrade_names[ordinal] != NULL
                ? ordinal + 1u : 0u;
            engine_heap_free(second_copy);
        }
    }
    entity_type_config_word_set(row, 0x90u, owner->upgrade_count);
    entity_type_config_word_set(row, 0x98u, owner->upgrade_level_mask);
    if (owner->upgrade_count == 0u) {
        engine_heap_free(owner->upgrade_records);
        free(owner->upgrade_values);
        free(owner->upgrade_names);
        owner->upgrade_records = NULL;
        owner->upgrade_values = NULL;
        owner->upgrade_names = NULL;
    }
    else {
        entity_type_config_word_set(row, 0x8cu, row_index + 1u);
    }
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
static int entity_type_config_row_parse_path(void* context,
                                             const char* asset_path,
                                             void* destination,
                                             uint32_t row_kind)
{
    char ae_path[520];
    const char* base;
    BlockFile* ae;
    uint32_t scene = 0u;
    if (asset_path == NULL || destination == NULL ||
        (row_kind != 1u && row_kind != 2u && row_kind != 3u &&
         row_kind != 4u)) return 0;
    if (*asset_path == '\0') return 0;

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
    /* FUN_00406990 and FUN_00407d60 look up the "Lego*" record before
     * accepting the visual.  An existing .ae file with no such record is
     * still a failed type row in the reference loader. */
    if (block_subgroup(ae, "Lego*") == NULL) {
        block_close(ae);
        return 0;
    }
    /* The host renderer may not expose the retail ACT scene constructor yet.
     * Preserve all deterministic config fields in that case, using zero for
     * opaque scene handles; when a loader is installed, its failure remains
     * the same row rejection as in the retail path. */
    if (s_entity_type_runtime_ops.visual_load != NULL &&
        (!s_entity_type_runtime_ops.visual_load(
             s_entity_type_runtime_ops.context, asset_path, &scene) ||
         scene == 0u)) {
        block_close(ae);
        return 0;
    }

    if (row_kind == 1u || row_kind == 3u || row_kind == 4u) {
        uint32_t i;
        uint32_t camera_frames;
        uint32_t camera_null;
        entity_type_config_word_set(destination, 0u, 0u);
        entity_type_config_word_set(destination, 4u, scene);
        camera_null = entity_type_config_named_get(scene,
            entity_type_config_ae_value(ae, "CameraNullName"), 0);
        entity_type_config_word_set(destination, 32u, camera_null);
        camera_frames = camera_null != 0u
            ? entity_type_config_frames(ae, "CameraNullFrames") : 0u;
        entity_type_config_word_set(destination, 36u, camera_frames);
        if (camera_null != 0u) {
            entity_type_config_word_set(destination, 40u,
                entity_type_config_named_get(scene,
                    entity_type_config_ae_value(ae, "CameraFlipDir"), 0));
        }
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
        TypeConfigTable* table = (TypeConfigTable*)context;
        uint32_t row_index = 0xffffffffu;
        if (table != NULL && table->row_owned != NULL &&
            table->rows != NULL && table->row_stride != 0u) {
            size_t row_offset = (size_t)((char*)destination -
                                          (char*)table->rows);
            if (row_offset % table->row_stride == 0u &&
                row_offset / table->row_stride < table->capacity)
                row_index = (uint32_t)(row_offset / table->row_stride);
        }
        if (!entity_type_config_shape_parse(destination,
                entity_type_config_ae_value(ae, "Shape"), table, row_index) ||
            !entity_type_config_building_upgrades_parse(
                destination, table, row_index)) {
            block_close(ae);
            return 0;
        }
    }
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
    if (row_kind == 1u || row_kind == 2u || row_kind == 3u) {
        TypeConfigTable* table = (TypeConfigTable*)context;
        size_t offset;
        if (table == NULL || table->rows == NULL || table->row_stride == 0u) {
            block_close(ae); return 0;
        }
        offset = (size_t)((char*)destination - (char*)table->rows);
        if (offset % table->row_stride != 0u ||
            offset / table->row_stride >= table->capacity ||
            !(row_kind == 2u
                ? building_scene_config_path_set((uint32_t)(offset / table->row_stride),
                    asset_path, engine_feature_word_peek())
                : actor_scene_config_path_set(row_kind == 1u ? 2 : 3,
                    (uint32_t)(offset / table->row_stride), asset_path,
                    engine_feature_word_peek()))) {
            block_close(ae); return 0;
        }
    }
#endif
    block_close(ae);
    return 1;
}

int entity_type_config_row_parse(void* context, void* entry,
                                 void* destination, uint32_t row_kind)
{
    BlockRec* config_entry = (BlockRec*)entry;
    if (config_entry == NULL) return 0;
    return entity_type_config_row_parse_path(
        context, block_value(config_entry), destination, row_kind);
}

// FUNCTION: LEGORR 0x00406990
int entity_type_config_resource_row_load(void* destination,
                                        uint32_t row_index,
                                        void* scene_context,
                                        const char* asset_path,
                                        void* engine_config,
                                        uint32_t row_kind)
{
    TypeConfigTable* table = NULL;
    uint32_t row_slot = 0xffffffffu;
    uintptr_t row_address;
    uintptr_t table_address;
    (void)scene_context;
    (void)engine_config;
    if (destination == NULL) return 0;
    if (row_kind == 1u) table = &s_minifigure_type_config;
    else if (row_kind == 2u) table = &s_building_type_config;
    else if (row_kind == 3u) table = &s_rockmonster_type_config;
    else if (row_kind == 4u) table = &s_vehicle_type_config;
    if (row_kind == 2u) memset(destination, 0, 0x14cu);
    row_address = (uintptr_t)destination;
    table_address = table != NULL ? (uintptr_t)table->rows : 0u;
    if (table != NULL && table->rows != NULL && table->row_owned != NULL &&
        table->row_stride != 0u && row_address >= table_address &&
        row_address - table_address <
            (uintptr_t)table->capacity * table->row_stride) {
        size_t offset = (size_t)(row_address - table_address);
        if (offset % table->row_stride == 0u &&
            offset / table->row_stride < table->capacity)
            row_slot = (uint32_t)(offset / table->row_stride);
    }
    if (row_kind == 2u && row_slot == 0xffffffffu) return 0;
    if (!entity_type_config_row_parse_path(table, asset_path, destination,
                                           row_kind)) return 0;
    entity_type_config_word_set(destination, 0u, row_index);
    if (row_kind == 2u) {
        entity_type_config_word_set(destination, 0x148u, 0u);
        if (s_entity_type_runtime_ops.scene_apply_mode != NULL) {
            uint32_t scene = 0u;
            memcpy(&scene, (char*)destination + 4u, sizeof(scene));
            if (scene != 0u)
                s_entity_type_runtime_ops.scene_apply_mode(
                    s_entity_type_runtime_ops.context, scene, 1);
        }
    }
    return 1;
}

// FUNCTION: LEGORR 0x00407d60
int entity_building_type_config_record_parse(void* destination,
                                             uint32_t row_index,
                                             void* scene_context,
                                             const char* asset_path,
                                             void* engine_config)
{
    if (destination == NULL) return 0;
    memset(destination, 0, 0x14cu);
    return entity_type_config_resource_row_load(
        destination, row_index, scene_context, asset_path, engine_config, 2u);
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

static void entity_type_config_family_release(TypeConfigTable* table, uint32_t kind)
{
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
    if (kind == 2u && !building_scene_types_clear())
        building_scene_require_stopped();
    if ((kind == 1u || kind == 3u) &&
        !actor_scene_types_clear(kind == 1u ? 2 : 3))
        actor_scene_require_stopped();
#else
    (void)kind;
#endif
    type_config_table_release(table);
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

/* MiniFigureTypeTableLoad (FUN_0042cf50) reads the active engine config,
 * builds 0x74-byte rows, finalizes each parsed row, and stores copied names. */
// FUNCTION: LEGORR 0x0042CF50
int entity_minifigure_type_table_load(void)
{
    ENTITY_TYPE_CONFIG_TABLE_LOAD(&s_minifigure_type_config,
                                  "MiniFigureTypes", 1u, 0x74u);
}

/* RockMonsterTypesLoad (FUN_0042d100) uses the same 0x74-byte visual row
 * reader as the vehicle and minifigure tables (FUN_00406990). */
// FUNCTION: LEGORR 0x0042D100
static int entity_rockmonster_type_table_load(void)
{
    ENTITY_TYPE_CONFIG_TABLE_LOAD(&s_rockmonster_type_config,
                                  "RockMonsterTypes", 3u, 0x74u);
}

/* VehicleTypesLoad (FUN_0042cda0) likewise loads 0x74-byte visual rows. */
// FUNCTION: LEGORR 0x0042CDA0
static int entity_vehicle_type_table_load(void)
{
    ENTITY_TYPE_CONFIG_TABLE_LOAD(&s_vehicle_type_config,
                                  "VehicleTypes", 4u, 0x74u);
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
        child = block_next_equal_key(child);
    }
    return NULL;
}

typedef struct EntityNameResolveContext {
    char named_values[9][256];
    char named_the_values[9][256];
    uint8_t named_values_found[9];
    uint8_t named_the_values_found[9];
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
            state->named_the_values_found[index] = 1u;
            return;
        }
        strncpy(state->named_the_values[index], (const char*)value,
                sizeof(state->named_the_values[index]) - 1u);
        state->named_the_values[index][
            sizeof(state->named_the_values[index]) - 1u] = '\0';
        state->named_the_values_found[index] = 1u;
    }
    else {
        if (value == NULL) {
            state->named_values[index][0] = '\0';
            state->named_values_found[index] = 1u;
            return;
        }
        strncpy(state->named_values[index], (const char*)value,
                sizeof(state->named_values[index]) - 1u);
        state->named_values[index][
            sizeof(state->named_values[index]) - 1u] = '\0';
        state->named_values_found[index] = 1u;
    }
}

static const char** entity_config_name_table_build(BlockFile* cfg,
                                                    const char* section_name,
                                                    uint32_t* count)
{
    BlockRec* section = entity_config_section_find(cfg, section_name);
    BlockRec* child;
    const char** names;
    uint32_t i;
    *count = 0u;
    for (child = block_first_child(section); child != NULL;
         child = block_next_equal_key(child)) {
        if (*count == 0xffffffffu) {
            *count = 0u;
            return NULL;
        }
        ++*count;
    }
    if (*count == 0u || (size_t)*count > ((size_t)-1) / sizeof(*names))
        return NULL;
    names = (const char**)malloc((size_t)*count * sizeof(*names));
    if (names == NULL) {
        *count = 0u;
        return NULL;
    }
    child = block_first_child(section);
    for (i = 0u; i < *count; ++i) {
        if (child == NULL || block_name(child) == NULL) {
            free((void*)names);
            *count = 0u;
            return NULL;
        }
        names[i] = block_name(child);
        child = block_next_equal_key(child);
    }
    return names;
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
    const char** names[5] = { NULL, NULL, NULL, NULL, NULL };
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
    const TypeConfigTable* type_tables[4];
    static EntityNameResolveContext resolve_state;
    const char* special_names[16][2];
    const char* special_the_names[16][2];
    uint32_t i;

    if (cfg == NULL) return;
    /* The prior table borrows BuildingTypes-owned strings. Clear those
     * borrowed pointers before the type loaders release and replace them. */
    string_table_install(NULL, 0u);
    s_entity_type_config_file = cfg;
    /* GameConfigInit loads ToolNames from the same Lego.cfg tree before its
     * ObjectNames/ObjectTheNames work. Keep the indexed tool-name handles
     * ready at the boot-time config boundary, using the established root. */
    entity_tool_name_strings_load(cfg, (void*)util_root_name(cfg));
    (void)entity_vehicle_type_table_load();
    (void)entity_minifigure_type_table_load();
    (void)entity_rockmonster_type_table_load();
    (void)entity_building_types_load();
    type_tables[0] = entity_type_config_table_get(4u);
    type_tables[1] = entity_type_config_table_get(1u);
    type_tables[2] = entity_type_config_table_get(3u);
    type_tables[3] = entity_type_config_table_get(2u);
    memset(&tables, 0, sizeof(tables));
    memset(&resolve_state, 0, sizeof(resolve_state));
    memset(special_names, 0, sizeof(special_names));
    memset(special_the_names, 0, sizeof(special_the_names));
    for (i = 0u; i < 5u; ++i) {
        if (i < 4u) {
            names[i] = type_tables[i] != NULL
                ? (const char**)type_tables[i]->names : NULL;
            counts[i] = type_tables[i] != NULL ? type_tables[i]->count : 0u;
        }
        else {
            names[i] = entity_config_name_table_build(
                cfg, sections[i], &counts[i]);
        }
        tables.type_names[i] = names[i];
        tables.counts[i] = counts[i];
        tables.output_values[i] = &object_values[i];
        catalog_count_install((int)i, counts[i]);
        if (i < 4u) {
            catalog_name_table_install((int)i,
                type_tables[i] != NULL ? type_tables[i]->names : NULL);
        }
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
     * each occupy the single configured row for their special kind. The
     * retail loaders update named globals only when their config key exists,
     * so keep the previous localized value for omitted keys on reload. */
    {
        static const int kinds[9] = { 7, 6, 7, 8, 9, 11, 12, 13, 15 };
        static const int rows[9] = { 0, 0, 1, 0, 0, 0, 0, 0, 0 };
        for (i = 0u; i < 9u; ++i) {
            const char* old_name = localized_object_name_get(
                (uint32_t)kinds[i], rows[i]);
            const char* old_the_name = localized_object_the_name_get(
                (uint32_t)kinds[i], rows[i]);
            if (resolve_state.named_values_found[i] != 0u)
                special_names[kinds[i]][rows[i]] =
                    resolve_state.named_values[i];
            else
                special_names[kinds[i]][rows[i]] = old_name;
            if (resolve_state.named_the_values_found[i] != 0u)
                special_the_names[kinds[i]][rows[i]] =
                    resolve_state.named_the_values[i];
            else
                special_the_names[kinds[i]][rows[i]] = old_the_name;
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
    /* StringTableIndexLookup's only observed caller is Stats.BuildingBase.
     * The retail pointer table's writer is not present in the decompile
     * xrefs, so bind its portable equivalent to the live BuildingTypes name
     * table, whose entries own the same configured names and remain alive
     * for the Stats load and subsequent lookups. */
    string_table_install(names[3], counts[3]);
    /* Stats names resolve through the type-name tables installed above, and
     * their records back the per-type/per-row getters used by entity logic. */
    (void)entity_stats_config_load_from_block(cfg);
    /* GameConfigInit allocates the vehicle/building maps after Stats has
     * been loaded. Counts were installed from the type tables above. */
    entity_table_maps_allocate();
    for (i = 0u; i < 5u; ++i) {
        entity_config_values_release(object_values[i], counts[i]);
        entity_config_values_release(the_values[i], counts[i]);
    }
    free((void*)names[4]);
    for (i = 0u; i < 9u; ++i) engine_heap_free(named_values[i]);
    for (i = 0u; i < 9u; ++i) engine_heap_free(named_the_values[i]);
}

typedef struct EntityStatsBlockLoadContext {
    BlockFile* config;
    BlockRec* section;
    BlockRec* current_entry;
} EntityStatsBlockLoadContext;

static void* s_entity_stats_record_allocations[STATS_TYPE_TABLE_TYPES]
                                                [STATS_TYPE_TABLE_ROWS];
typedef struct EntityStatsRetiredAllocation {
    void* records;
    struct EntityStatsRetiredAllocation* next;
} EntityStatsRetiredAllocation;
static EntityStatsRetiredAllocation* s_entity_stats_retired_allocations;

static void entity_stats_allocation_retire(void* records)
{
    EntityStatsRetiredAllocation* retired;
    if (records == NULL) return;
    retired = (EntityStatsRetiredAllocation*)malloc(sizeof(*retired));
    if (retired == NULL) return; /* retain the rows rather than dangle users */
    retired->records = records;
    retired->next = s_entity_stats_retired_allocations;
    s_entity_stats_retired_allocations = retired;
}

static void entity_stats_retired_allocations_release(void)
{
    EntityStatsRetiredAllocation* retired = s_entity_stats_retired_allocations;
    while (retired != NULL) {
        EntityStatsRetiredAllocation* next = retired->next;
        engine_heap_free(retired->records);
        free(retired);
        retired = next;
    }
    s_entity_stats_retired_allocations = NULL;
}

static BlockRec* entity_stats_field_find(BlockRec* entry, const char* name)
{
    BlockRec* field;
    for (field = block_first_child(entry); field != NULL;
         field = block_next_equal_key(field)) {
        if (entity_config_name_equal(block_name(field), name)) return field;
    }
    return NULL;
}

static unsigned int entity_stats_block_entry_count(void* context,
                                                  void* config,
                                                  void* section)
{
    BlockRec* entry;
    unsigned int count = 0u;
    (void)context;
    (void)config;
    for (entry = block_first_child((BlockRec*)section); entry != NULL;
         entry = block_next_equal_key(entry)) ++count;
    return count;
}

static const char* entity_stats_block_entry_name(void* context,
                                                  void* config,
                                                  void* section,
                                                  unsigned int index)
{
    EntityStatsBlockLoadContext* state =
        (EntityStatsBlockLoadContext*)context;
    BlockRec* entry = block_first_child((BlockRec*)section);
    (void)config;
    while (entry != NULL && index != 0u) {
        entry = block_next_equal_key(entry);
        --index;
    }
    if (state != NULL) state->current_entry = entry;
    return entry != NULL ? block_name(entry) : NULL;
}

static int entity_stats_block_type_row_resolve(void* context,
                                               const char* name,
                                               int* type, int* row)
{
    (void)context;
    return entity_type_row_for_name(name, type, row);
}

static unsigned int entity_stats_block_type_capacity(void* context,
                                                      int type)
{
    EntityStatsBlockLoadContext* state =
        (EntityStatsBlockLoadContext*)context;
    BlockRec* levels;
    const char* value;
    float level_limit;
    (void)type;
    if (state == NULL || state->current_entry == NULL) return 0u;
    levels = entity_stats_field_find(state->current_entry, "Levels");
    value = levels != NULL ? block_value(levels) : NULL;
    level_limit = value != NULL ? (float)atof(value) : 0.0f;
    if (!(level_limit >= 0.0f)) return 0u;
    if (level_limit > 16.0f) level_limit = 16.0f;
    return (unsigned int)level_limit;
}

static int entity_stats_block_float_value_get(void* context,
                                              const char* type_name,
                                              const char* field_name,
                                              unsigned int level,
                                              float* output)
{
    EntityStatsBlockLoadContext* state =
        (EntityStatsBlockLoadContext*)context;
    BlockRec* field;
    const char* value;
    const char* token;
    const char* end;
    char number[64];
    size_t length;
    (void)type_name;
    if (output == NULL) return 0;
    *output = 0.0f;
    if (state == NULL || state->current_entry == NULL) return 0;
    field = entity_stats_field_find(state->current_entry, field_name);
    value = field != NULL ? block_value(field) : NULL;
    if (value == NULL) {
        if (entity_config_name_equal(field_name, "BuildingBase")) {
            const uint32_t missing_name = 0xffc00000u;
            memcpy(output, &missing_name, sizeof(missing_name));
            return 1;
        }
        return 0;
    }
    if (entity_config_name_equal(field_name, "DrillSound") ||
        entity_config_name_equal(field_name, "DrillFadeSound") ||
        entity_config_name_equal(field_name, "EngineSound")) {
        level = 0u;
    }
    token = value;
    while (level != 0u && *token != '\0') {
        if (*token == ',') --level;
        ++token;
    }
    if (*token == '\0' && level != 0u) return 0;
    end = token;
    while (*end != '\0' && *end != ',') ++end;
    length = (size_t)(end - token);
    if (length >= sizeof(number)) length = sizeof(number) - 1u;
    memcpy(number, token, length);
    number[length] = '\0';
    if (entity_config_name_equal(field_name, "DrillSound") ||
        entity_config_name_equal(field_name, "DrillFadeSound") ||
        entity_config_name_equal(field_name, "EngineSound")) {
        int alias;
        const int fallback = entity_config_name_equal(field_name, "DrillSound")
            ? 2 : (entity_config_name_equal(field_name, "DrillFadeSound")
                ? 3 : 0);
        *output = sound_alias_lookup(number, &alias)
            ? (float)alias : (float)fallback;
        return 1;
    }
    {
        int bool_value = block_get_bool(number);
        if (bool_value != 2) *output = bool_value != 0 ? 1.0f : 0.0f;
        else if (entity_config_name_equal(field_name, "BuildingBase"))
            *output = (float)string_table_index_lookup(number);
        else *output = (float)atof(number);
    }
    return 1;
}

typedef struct EntityStatsFieldBinding {
    const char* name;
    uint32_t offset;
    float scale;
    int integer;
} EntityStatsFieldBinding;

static const EntityStatsFieldBinding s_entity_stats_field_bindings[] = {
    { "RouteSpeed", 0x20u, 1.0f, 0 },
    { "SoilDrillTime", 0x24u, 25.0f, 0 },
    { "LooseDrillTime", 0x28u, 25.0f, 0 },
    { "MedDrillTime", 0x2cu, 25.0f, 0 },
    { "HardDrillTime", 0x30u, 25.0f, 0 },
    { "SeamDrillTime", 0x34u, 25.0f, 0 },
    { "CollHeight", 0x74u, 1.0f, 0 },
    { "CollRadius", 0x6cu, 1.0f, 0 },
    { "BuildingBase", 0xa8u, 1.0f, 0 },
    { "PickSphere", 0x78u, 1.0f, 0 },
    { "TrackDist", 0x7cu, 1.0f, 0 },
    { "HealthDecayRate", 0x80u, 1.0f, 0 },
    { "EnergyDecayRate", 0x84u, 1.0f, 0 },
    { "DrillSound", 0xc0u, 1.0f, 1 },
    { "DrillFadeSound", 0xc4u, 1.0f, 1 },
    { "EngineSound", 0xc8u, 1.0f, 1 },
    { "RepairValue", 0x88u, 1.0f, 0 },
    { "RestPercent", 0x8cu, 1.0f, 0 },
    { "CarryMinHealth", 0x90u, 1.0f, 0 },
    { "StampRadius", 0x94u, 1.0f, 0 },
    { "AttackRadius", 0x98u, 1.0f, 0 },
    { "AlertRadius", 0x70u, 1.0f, 0 },
    { "SurveyRadius", 0xacu, 1.0f, 1 },
    { "CostOre", 0xb0u, 1.0f, 1 },
    { "CostCrystal", 0xb4u, 1.0f, 1 },
    { "CostRefinedOre", 0xb8u, 1.0f, 1 },
    { "CrystalDrain", 0xbcu, 1.0f, 1 },
    { "UpgradeCostOre", 0xfcu, 1.0f, 1 },
    { "UpgradeCostStuds", 0x100u, 1.0f, 1 },
    { "WaterEntrances", 0xd0u, 1.0f, 1 },
    { "RubbleCoef", 0xd4u, 1.0f, 0 },
    { "PathCoef", 0xd8u, 1.0f, 0 },
    { "OxygenCoef", 0xdcu, 1.0f, 0 },
    { "Capacity", 0xe0u, 1.0f, 1 },
    { "AwarenessRange", 0xe4u, 1.0f, 0 },
    { "PainThreshold", 0xe8u, 1.0f, 0 },
    { "MaxCarry", 0xecu, 1.0f, 1 },
    { "CarryStart", 0xf0u, 1.0f, 1 },
    { "UpgradeTime", 0xf4u, 25.0f, 0 },
    { "FunctionCoef", 0xf8u, 1.0f, 0 },
    { "WakeRadius", 0x9cu, 1.0f, 0 },
    { "FreezerTime", 0x10cu, 1.0f, 0 },
    { "FreezerDamage", 0x110u, 1.0f, 0 },
    { "PusherDist", 0x114u, 1.0f, 0 },
    { "PusherDamage", 0x118u, 1.0f, 0 },
    { "LaserDamage", 0x11cu, 1.0f, 0 },
    { "Flocks_Turn", 0x120u, 1.0f, 0 },
    { "Flocks_Speed", 0x124u, 1.0f, 0 },
    { "Flocks_Tightness", 0x128u, 1.0f, 0 },
    { "Flocks_GoalUpdate", 0x12cu, 1.0f, 0 },
    { "Flocks_Height", 0x130u, 1.0f, 0 },
    { "Flocks_Randomness", 0x134u, 1.0f, 0 },
    { "Flocks_AttackTime", 0x138u, 1.0f, 0 },
    { "Flocks_Size", 0x13cu, 1.0f, 0 },
    { "RandomMoveTime", 0x140u, 1.0f, 0 },
    { "NumOfToolsCanCarry", 0xccu, 1.0f, 1 }
};

static void entity_stats_block_record_commit(
    void* context, const EntityStatsConfigRecord* config_record)
{
    const EntityStatsFieldBinding* binding;
    uint8_t* records;
    unsigned int level;
    unsigned int field;
    uint32_t flags;
    uint32_t flags_14c;
    if (config_record == NULL || config_record->type < 0 ||
        config_record->type >= STATS_TYPE_TABLE_TYPES ||
        config_record->row < 0 ||
        config_record->row >= STATS_TYPE_TABLE_ROWS) return;
    engine_heap_free(s_entity_stats_record_allocations[config_record->type]
                                                   [config_record->row]);
    s_entity_stats_record_allocations[config_record->type]
                                     [config_record->row] = NULL;
    records = NULL;
    if (config_record->level_count != 0u) {
        records = (uint8_t*)engine_heap_alloc(
            (long)(config_record->level_count * 0x150u));
        if (records == NULL) {
            stats_type_table_install(config_record->type, config_record->row,
                                     NULL, 0);
            return;
        }
        memset(records, 0, config_record->level_count * 0x150u);
    }
    for (level = 0u; level < config_record->level_count; ++level) {
        uint8_t* record = records + level * 0x150u;
        float collision_radius = 0.0f;
        float collision_box_x = 0.0f;
        float collision_box_y = 0.0f;
        int have_collision_box = 0;
        if (context != NULL) {
            EntityStatsBlockLoadContext* state =
                (EntityStatsBlockLoadContext*)context;
            entity_stats_block_float_value_get(context, config_record->name,
                "CollRadius", level, &collision_radius);
            entity_stats_block_float_value_get(context, config_record->name,
                "CollBox", 0u, &collision_box_x);
            entity_stats_block_float_value_get(context, config_record->name,
                "CollBox", 1u, &collision_box_y);
            have_collision_box = entity_stats_field_find(
                state->current_entry, "CollBox") != NULL;
        }
        for (field = 0u; field < sizeof(s_entity_stats_field_bindings) /
                                      sizeof(s_entity_stats_field_bindings[0]);
             ++field) {
            binding = &s_entity_stats_field_bindings[field];
            {
                unsigned int value_index;
                for (value_index = 0u;
                     value_index < ENTITY_STATS_CONFIG_FIELD_COUNT;
                     ++value_index) {
                    if (entity_config_name_equal(
                            config_record->field_names[value_index],
                            binding->name)) {
                        float value = config_record->level_fields[level]
                                                             [value_index] *
                                      binding->scale;
                        if (value == 0.0f) {
                            if (entity_config_name_equal(binding->name,
                                                         "RubbleCoef") ||
                                entity_config_name_equal(binding->name,
                                                         "PathCoef"))
                                value = 1.0f;
                            else if (entity_config_name_equal(binding->name,
                                                            "Flocks_Turn"))
                                value = 0.06f;
                            else if (entity_config_name_equal(binding->name,
                                                            "Flocks_Speed") ||
                                     entity_config_name_equal(binding->name,
                                                            "Flocks_Tightness") ||
                                     entity_config_name_equal(binding->name,
                                                            "Flocks_GoalUpdate") ||
                                     entity_config_name_equal(binding->name,
                                                            "Flocks_Randomness"))
                                value = 2.0f;
                            else if (entity_config_name_equal(binding->name,
                                                            "Flocks_Height"))
                                value = 30.0f;
                            else if (entity_config_name_equal(binding->name,
                                                            "Flocks_AttackTime"))
                                value = 300.0f;
                            else if (entity_config_name_equal(binding->name,
                                                            "WakeRadius"))
                                value = 20.0f;
                            else if (entity_config_name_equal(binding->name,
                                                            "RandomMoveTime"))
                                value = 200.0f;
                        }
                        if (entity_config_name_equal(binding->name,
                                                     "Flocks_Size")) {
                            if (value == 0.0f)
                                entity_type_config_word_set(record,
                                    binding->offset, 1u);
                            else
                                entity_type_config_float_set(record,
                                    binding->offset, (float)(uint32_t)value);
                        } else if (binding->integer)
                            entity_type_config_word_set(record,
                                binding->offset, (uint32_t)value);
                        else
                            entity_type_config_float_set(record,
                                binding->offset, value);
                        break;
                    }
                }
            }
        }
        flags = 0u;
        if (collision_radius != 0.0f) flags |= 0x80u;
        else if (have_collision_box) flags |= 0x100u;
        for (field = 0u; field < ENTITY_STATS_CONFIG_FIELD_COUNT; ++field) {
            const char* name = config_record->field_names[field];
            float value = config_record->level_fields[level][field];
            if (value == 0.0f) continue;
            if (entity_config_name_equal(name, "SingleWidthDig")) flags |= 1u;
            else if (entity_config_name_equal(name, "Flocks_AnimateByPitch"))
                flags |= 0x10000000u;
            else if (entity_config_name_equal(name, "Flocks"))
                flags |= 0x200000u;
            else if (entity_config_name_equal(name, "Flocks_Debug"))
                flags |= 0x400000u;
            else if (entity_config_name_equal(name, "Flocks_Smooth"))
                flags |= 0x800000u;
            else if (entity_config_name_equal(name, "Flocks_OnGround"))
                flags |= 0x4000000u;
            else if (entity_config_name_equal(name, "Flocks_QuickDestroy"))
                flags |= 0x8000000u;
            else if (entity_config_name_equal(name, "CrossLand"))
                flags |= 0x2000000u;
            else if (entity_config_name_equal(name, "CrossWater"))
                flags |= 0x1000000u;
            else if (entity_config_name_equal(name, "BumpDamage"))
                flags |= 0x40000000u;
            else if (entity_config_name_equal(name, "RouteAvoidance"))
                flags |= 0x20000000u;
            else if (entity_config_name_equal(name, "CanSteal"))
                flags |= 0x80000u;
            else if (entity_config_name_equal(name, "CanClearRubble"))
                flags |= 0x10000u;
            else if (entity_config_name_equal(name, "CanBeDriven"))
                flags |= 0x20000u;
            else if (entity_config_name_equal(name, "GrabMinifigure"))
                flags |= 0x8000u;
            else if (entity_config_name_equal(name, "ScaredByPlayer"))
                flags |= 0x2000u;
            else if (entity_config_name_equal(name, "RandomEnterWall"))
                flags |= 0x1000u;
            else if (entity_config_name_equal(name, "CanScareScorpion"))
                flags |= 0x40000u;
            else if (entity_config_name_equal(name, "CanScare"))
                flags |= 0x800u;
            else if (entity_config_name_equal(name, "RandomMove"))
                flags |= 0x400u;
            else if (entity_config_name_equal(name, "ProcessOre")) flags |= 2u;
            else if (entity_config_name_equal(name, "ProcessCrystal")) flags |= 4u;
            else if (entity_config_name_equal(name, "StoreObjects")) flags |= 8u;
            else if (entity_config_name_equal(name, "SmallTeleporter")) flags |= 0x10u;
            else if (entity_config_name_equal(name, "BigTeleporter")) flags |= 0x20u;
            else if (entity_config_name_equal(name, "WaterTeleporter")) flags |= 0x40u;
            else if (entity_config_name_equal(name, "CauseSlip")) flags |= 0x200u;
            else if (entity_config_name_equal(name, "SnaxULike")) flags |= 0x4000u;
            else if (entity_config_name_equal(name, "ToolStore")) flags |= 0x100000u;
            else if (entity_config_name_equal(name, "ManTeleporter"))
                flags |= 0x80000000u;
        }
        if (collision_radius == 0.0f && have_collision_box) {
            float maximum = collision_box_x > collision_box_y
                ? collision_box_x : collision_box_y;
            collision_radius = maximum * 0.5f;
            entity_type_config_float_set(record, 0xa0u, collision_box_x);
            entity_type_config_float_set(record, 0xa4u, collision_box_y);
            entity_type_config_float_set(record, 0x6cu, collision_radius);
        }
        entity_type_config_word_set(record, 0x144u, flags);
        flags = 0u;
        for (field = 0u; field < ENTITY_STATS_CONFIG_FIELD_COUNT; ++field) {
            const char* name = config_record->field_names[field];
            float value = config_record->level_fields[level][field];
            if (value == 0.0f) continue;
            if (entity_config_name_equal(name, "ScaredByBigBangs")) flags |= 1u;
            else if (entity_config_name_equal(name, "UpgradeBuilding"))
                flags |= 2u;
            else if (entity_config_name_equal(name, "CanBeShotAt"))
                flags |= 0x40000u;
            else if (entity_config_name_equal(name, "UseBigTeleporter"))
                flags |= 0x800u;
            else if (entity_config_name_equal(name, "UseSmallTeleporter"))
                flags |= 0x1000u;
            else if (entity_config_name_equal(name, "UseWaterTeleporter"))
                flags |= 0x2000u;
            else if (entity_config_name_equal(name, "UseLegoManTeleporter"))
                flags |= 0x400000u;
            else if (entity_config_name_equal(name, "TrainPilot"))
                flags |= 0x4u;
            else if (entity_config_name_equal(name, "TrainSailor"))
                flags |= 0x8u;
            else if (entity_config_name_equal(name, "TrainDriver"))
                flags |= 0x10u;
            else if (entity_config_name_equal(name, "TrainDynamite"))
                flags |= 0x20u;
            else if (entity_config_name_equal(name, "TrainRepair"))
                flags |= 0x40u;
            else if (entity_config_name_equal(name, "TrainScanner"))
                flags |= 0x80u;
            else if (entity_config_name_equal(name, "Tracker"))
                flags |= 0x100u;
            else if (entity_config_name_equal(name, "PowerBuilding"))
                flags |= 0x600u;
            else if (entity_config_name_equal(name, "SelfPowered"))
                flags |= 0x400u;
            else if (entity_config_name_equal(name, "AttackPaths"))
                flags |= 0x4000u;
            else if (entity_config_name_equal(name, "SplitOnZeroHealth"))
                flags |= 0x8000u;
            else if (entity_config_name_equal(name, "CanBeHitByFence"))
                flags |= 0x10000u;
            else if (entity_config_name_equal(name, "CanDoubleSelect"))
                flags |= 0x20000u;
            else if (entity_config_name_equal(name, "DrainPower"))
                flags |= 0x80000u;
            else if (entity_config_name_equal(name, "UseHoles"))
                flags |= 0x100000u;
            else if (entity_config_name_equal(name, "CrossLava"))
                flags |= 0x200000u;
            else if (entity_config_name_equal(name, "DamageCausesCallToArms"))
                flags |= 0x800000u;
            else if (entity_config_name_equal(name, "CanFreeze"))
                flags |= 0x1000000u;
            else if (entity_config_name_equal(name, "CanLaser"))
                flags |= 0x2000000u;
            else if (entity_config_name_equal(name, "CanPush"))
                flags |= 0x4000000u;
            else if (entity_config_name_equal(name, "DontShowDamage"))
                flags |= 0x8000000u;
            else if (entity_config_name_equal(name, "RemoveReinforcement"))
                flags |= 0x10000000u;
            else if (entity_config_name_equal(name, "DontShowOnRadar"))
                flags |= 0x20000000u;
            else if (entity_config_name_equal(name, "InvisibleDriver"))
                flags |= 0x40000000u;
            else if (entity_config_name_equal(name, "Unselectable"))
                flags |= 0x80000000u;
        }
        entity_type_config_word_set(record, 0x148u, flags);
        flags_14c = 0u;
        for (field = 0u; field < ENTITY_STATS_CONFIG_FIELD_COUNT; ++field) {
            const char* name = config_record->field_names[field];
            const float value = config_record->level_fields[level][field];
            if (value == 0.0f) continue;
            if (entity_config_name_equal(name, "ClassAsLarge"))
                flags_14c |= 0x8u;
            else if (entity_config_name_equal(name, "NeedsPilot"))
                flags_14c |= 0x100u;
            else if (entity_config_name_equal(name, "CarryVehicles"))
                flags_14c |= 1u;
            else if (entity_config_name_equal(name, "VehicleCanBeCarried"))
                flags_14c |= 2u;
            else if (entity_config_name_equal(name, "CanStrafe"))
                flags_14c |= 4u;
            else if (entity_config_name_equal(name, "GetOutAtLand"))
                flags_14c |= 0x10u;
            else if (entity_config_name_equal(name, "GetInAtLand"))
                flags_14c |= 0x20u;
            else if (entity_config_name_equal(name, "TakeCarryingDrivers"))
                flags_14c |= 0x40u;
            else if (entity_config_name_equal(name, "ShowHealthBar"))
                flags_14c |= 0x80u;
            else if (entity_config_name_equal(name, "EnterToolStore"))
                flags_14c |= 0x200u;
        }
        entity_type_config_word_set(record, 0x14cu, flags_14c);
    }
    stats_type_table_install(config_record->type, config_record->row,
                             records, (int)config_record->level_count);
    stats_type_table_level_limit_install(config_record->type,
        config_record->row, config_record->level_limit);
    s_entity_stats_record_allocations[config_record->type]
                                     [config_record->row] = records;
}

static int entity_stats_rebind_live_entity(EntitySlot* entity, void* unused)
{
    (void)unused;
    if (entity != NULL && entity->task_state >= 0)
        (void)entity_task_state_capacity_bind(entity);
    else if (entity != NULL && entity->type >= 1 && entity->type <= 4)
        (void)entity_stats_record_bind(entity);
    return 0;
}

unsigned int entity_stats_config_load_from_block(BlockFile* config)
{
    EntityStatsBlockLoadContext state;
    EntityStatsConfigHooks hooks;
    int type;
    int row;
    unsigned int loaded;
    GameContext* game;
    if (config == NULL) return 0u;
    state.config = config;
    state.section = entity_config_section_find(config, "Stats");
    state.current_entry = NULL;
    if (state.section == NULL) return 0u;
    for (type = 0; type < STATS_TYPE_TABLE_TYPES; ++type) {
        for (row = 0; row < STATS_TYPE_TABLE_ROWS; ++row) {
            entity_stats_allocation_retire(
                s_entity_stats_record_allocations[type][row]);
            s_entity_stats_record_allocations[type][row] = NULL;
            stats_type_table_install(type, row, NULL, 0);
        }
    }
    memset(&hooks, 0, sizeof(hooks));
    hooks.context = &state;
    hooks.entry_count = entity_stats_block_entry_count;
    hooks.entry_name = entity_stats_block_entry_name;
    hooks.type_row_resolve = entity_stats_block_type_row_resolve;
    hooks.type_capacity = entity_stats_block_type_capacity;
    hooks.float_value_get = entity_stats_block_float_value_get;
    hooks.record_commit = entity_stats_block_record_commit;
    loaded = entity_stats_config_load(config, state.section, &hooks);
    /* Reloading Stats replaces its owned 0x150-byte rows. Repoint active
     * entities after all rows are installed so no EntitySlot retains a
     * pointer into the released table. */
    game = entity_pool_callback_context_get();
    if (game != NULL) {
        entity_pool_find_by_callback(game, entity_stats_rebind_live_entity,
                                     NULL, 0);
        entity_stats_retired_allocations_release();
    }
    return loaded;
}

#include "port_loop_ports.hpp"

/* VehicleTypeDefinitionParse (FUN_0046c6f0): the source parser zeroes its
 * 0x7b-word vehicle record before reading config. Keep that initialization
 * and provide an indexed field reader for the config library boundary. */
int vehicle_type_definition_parse(VehicleTypeDefinition* definition,
                                  void* context,
                                  VehicleTypeFieldReadHook read_field)
{
    uint32_t index;
    uint32_t present = 0;
    if (definition == NULL) return 0;
    memset(definition, 0, sizeof(*definition));
    if (read_field == NULL) return 0;
    for (index = 0; index < 0x7bu; ++index) {
        uint32_t value = 0;
        if (read_field(context, index, &value)) {
            definition->words[index] = value;
            ++present;
        }
    }
    return present != 0u;
}

namespace unported_ledger_stubs {

// TypeTableResourceRowLoad
int ledger_stub_00406990_TypeTableResourceRowLoad(
    void* row, uint32_t row_index, void* scene_context,
    const char* asset_path, void* engine_config)
{
    return entity_type_config_resource_row_load(
        row, row_index, scene_context, asset_path, engine_config, 1u);
}


// BuildingTypeConfigRecordParse
int ledger_stub_00407d60_BuildingTypeConfigRecordParse(
    void* row, uint32_t row_index, void* scene_context,
    const char* asset_path, void* engine_config)
{
    return entity_building_type_config_record_parse(
        row, row_index, scene_context, asset_path, engine_config);
}


} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {

// Port-facing adapter retained for callers that provide a table and hooks.
int ledger_stub_0042cf50_MiniFigureTypeTableLoad(
    TypeConfigTable* table, void* section, const TypeConfigHooks* hooks)
{
    /* FUN_0042cf50 uses 0x74-byte rows and its shared type-row parser. */
    return type_config_record_table_load(table, section, 1u, 0x74u, hooks);
}

// Port-facing adapter retained for callers that provide a table and hooks.
int ledger_stub_0042d2b0_BuildingTypesLoad(
    TypeConfigTable* table, void* section, const TypeConfigHooks* hooks)
{
    /* FUN_0042d2b0 uses 0x14c-byte building records. */
    return type_config_record_table_load(table, section, 2u, 0x14cu, hooks);
}

// Port-facing adapter retained for callers that provide table and hooks.
void ledger_stub_0042d600_ObjectNamesLoad(
    ObjectNameTableSet* tables, void* section,
    const ObjectNameLoadHooks* hooks)
{
    /* The five input name tables and counts are the vehicle, mini-figure,
     * monster, building, and upgrade tables consumed by the decompile. */
    object_names_load(tables, section, hooks);
}

// Port-facing adapter retained for callers that provide table and hooks.
void ledger_stub_0042da20_ObjectTheNamesLoad(
    ObjectNameTableSet* tables, BlockFile* config,
    const ObjectNameLoadHooks* hooks)
{
    BlockRec* section;
    if (tables == NULL || config == NULL || hooks == NULL) return;
    /* Unlike ObjectNamesLoad, this loader first opens the named config
     * subgroup and uses its own parallel table and handle globals. */
    section = entity_config_section_find(config, "ObjectTheNames");
    if (section == NULL) return;
    object_names_load(tables, section, hooks);
}


} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {

unsigned int ledger_stub_004667e0_EntityStatsConfigLoad(
    void* config, void* section, const EntityStatsConfigHooks* hooks)
{
    return entity_stats_config_load(config, section, hooks);
}


} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {

// VehicleTypeDefinitionParse
int ledger_stub_0046c6f0_VehicleTypeDefinitionParse(
    VehicleTypeDefinition* definition, void* context,
    VehicleTypeFieldReadHook read_field)
{
    return vehicle_type_definition_parse(definition, context, read_field);
}


} // namespace unported_ledger_stubs
