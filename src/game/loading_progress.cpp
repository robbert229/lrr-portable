// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-025 GAME-025: the unit's functions in original address order and the
 * state they own, relocated verbatim by tools/relocate_unit.py (ADR 0006).
 * Body hashes: docs/coverage/relocations/GAME-025.csv. */
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
#include "entity_for_loading_progress.hpp"
#include "util.hpp"
#include "gods98/media.hpp"

namespace unported_ledger_stubs {
// KeyValueEqualsFileLoad
// FUNCTION: LEGORR 0x0044db50
int ledger_stub_0044db50_KeyValueEqualsFileLoad(
    const MotionKeyValueLoadRequest* request, MotionKeyValueTable* table,
    const MotionKeyValueLoadHooks* hooks)
{
    char line[256];
    void* profile = NULL;
    uint32_t i;
    if (request == NULL || table == NULL || hooks == NULL) return 0;
    memset(table, 0, sizeof(*table));
    if (hooks->string_resolve != NULL) {
        table->name_a = hooks->string_resolve(hooks->context,
                                              request->name_a_id);
        table->name_b = hooks->string_resolve(hooks->context,
                                              request->name_b_id);
        if (request->optional_name_id != 0)
            table->optional_name = hooks->string_resolve(
                hooks->context, request->optional_name_id);
        if (request->text_resource_id != 0)
            table->text_resource = hooks->string_resolve(
                hooks->context, request->text_resource_id);
    }
    for (i = 0; i < 4u; ++i) table->values[i] = request->values[i];
    if (hooks->profile_open != NULL)
        profile = hooks->profile_open(hooks->context, request->config,
                                      request->profile_id);
    for (i = 0; i < MOTION_KEY_VALUE_ENTRY_CAPACITY; ++i) {
        char* key = NULL;
        char* value = NULL;
        if (profile == NULL || hooks->profile_line_get == NULL ||
            !hooks->profile_line_get(hooks->context, profile, i, line))
            continue;
        if (hooks->line_split != NULL) {
            if (hooks->line_split(hooks->context, line, &key, &value) != 2)
                continue;
        }
        else {
            char* separator = strchr(line, '=');
            if (separator == NULL) continue;
            *separator = '\0';
            key = line;
            value = separator + 1;
        }
        if (key == NULL || value == NULL) continue;
        table->entries[i].key = hooks->string_duplicate != NULL
            ? hooks->string_duplicate(hooks->context, key)
            : string_duplicate(key);
        if (table->entries[i].key == NULL) continue;
        table->entries[i].direct_value = 0;
        table->entries[i].fallback_value = hooks->integer_parse == NULL
            ? atoi(value) : hooks->integer_parse(hooks->context, value);
    }
    if (profile != NULL && hooks->profile_close != NULL)
        hooks->profile_close(hooks->context, profile);
    if (hooks->deferred_callback_register != NULL)
        hooks->deferred_callback_register(hooks->context, 0x0044dec0u);
    return 1;
}

// EngineLoaderProfileEntriesRelease
// FUNCTION: LEGORR 0x0044dd40
uint32_t ledger_stub_0044dd40_EngineLoaderProfileEntriesRelease(
    void* config, MotionConfigProfileEntry* entries, uint32_t count,
    const MotionConfigProfileReleaseHooks* hooks)
{
    return unported_ledger_stubs::motion_config_profile_entries_release(
        config, entries, count, hooks);
}
} // namespace unported_ledger_stubs
