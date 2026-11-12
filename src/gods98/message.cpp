// SPDX-License-Identifier: LGPL-3.0-or-later
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "game/savegame.hpp"
#include "engine.hpp"
#include "heap.hpp"
#include "input.hpp"
#include "message.hpp"
#include "sound.hpp"
#include "string.hpp"
#include "texture.hpp"
#include "record.hpp"
#include "crt/crt.hpp"
#include "message_shared.hpp"
#include "message_for_lego.hpp"
#include "message_for_interface.hpp"

extern void interaction_flags_set(uint32_t flags);

void* s_ui_message_primary_font;
void* s_ui_message_secondary_font;
void* s_ui_message_resource;
void* s_ui_message_subtitle_font;
TextureSurface* s_ui_message_image_slots[26];

const char* ui_message_text_slot_get(unsigned int index)
{
    return index < 26u ? s_ui_message_text_slots[index] : NULL;
}

TextureSurface* ui_message_image_slot_get(unsigned int index)
{
    return index < 26u ? s_ui_message_image_slots[index] : NULL;
}

void ui_message_image_position_get(float* x, float* y)
{
    if (x != NULL) *x = (float)(uintptr_t)s_ui_message_resource;
    if (y != NULL) *y = (float)(uintptr_t)s_ui_message_subtitle_font;
}

void* ui_message_text_output_get(int alternate)
{
    return alternate != 0 ? s_ui_message_secondary_font :
                            s_ui_message_primary_font;
}

const char* ui_message_label_get(unsigned int index)
{
    return index < 26u ? s_ui_message_labels[index] : NULL;
}

char* s_info_message_sound_names[MESSAGE_MAX_ID];
float         s_now = 0.0f;          /* _DAT_0049e264 analog */
MessageCallback  s_callback = NULL;
MessageSoundHook s_sound = NULL;
char* s_compiled_template[MESSAGE_MAX_ID];
TextureSurface* s_info_message_surfaces[MESSAGE_MAX_ID];
static Sound* s_info_message_sounds[MESSAGE_MAX_ID];
uint32_t s_catalog_owner;
IncidentNodeTextSlot s_node_text[INCIDENT_NODE_TEXT_SLOTS];
void* s_idx_head[IDX_LIST_SLOTS];  /* DAT_004dca18 role */
int   s_idx_count[IDX_LIST_SLOTS]; /* DAT_004dca1c role */
void* s_idx_aux[IDX_LIST_SLOTS];
void* s_message_nodes[MESSAGE_NODE_LIMIT];
int s_message_node_count;
MessagePanelRefreshHook s_panel_refresh_hook;
void* s_panel_refresh_context;
CatalogSlotPredicateHook s_catalog_slot_eligible;
CatalogSlotPredicateHook s_catalog_slot_available;
CatalogSlotRankHook s_catalog_slot_rank;
void* s_catalog_slot_context;
InfoMessageQueueTickOps s_info_message_queue_tick_ops;
IncidentMessageTextConsumerOps s_incident_message_text_ops;

void catalog_slot_flag_hooks_set(CatalogSlotPredicateHook eligible,
                                 CatalogSlotPredicateHook available,
                                 CatalogSlotRankHook rank, void* context)
{
    s_catalog_slot_eligible = eligible;
    s_catalog_slot_available = available;
    s_catalog_slot_rank = rank;
    s_catalog_slot_context = context;
}

int catalog_slot_table_index(int category)
{
    if (category == 1) return 0;
    if (category == 4) return 1;
    return -1;
}

void catalog_slot_flag_word_set(int category, int slot, uint32_t flags)
{
    if (catalog_slot_table_index(category) < 0 || slot < 0 ||
        slot >= CATALOG_SLOT_CAPACITY) return;
    input_binding_state_word_set(category, slot, flags);
}

uint32_t catalog_slot_flag_word_get(int category, int slot)
{
    if (catalog_slot_table_index(category) < 0 || slot < 0 ||
        slot >= CATALOG_SLOT_CAPACITY) return 0;
    return input_binding_state_word_get(category, slot);
}

void catalog_slot_flag_global_state_set(uint32_t flags)
{
    interaction_flags_set(flags);
}

/* The InfoMessages catalog — finding 22's 40 names, in table order. The
 * corpus copies these pointers into the zero-filled runtime table at
 * DAT_004dc658 during CatalogInit. */
const char* const s_catalog_defaults[MESSAGE_MAX_ID] = {
    "CrystalFound", "OreSeamFound", "RockMonster", "LavaRockMonster",
    "IceRockMonster", "UnderAttack", "Landslide", "CaveIn", "Constructed",
    "CavernLocated", "LegoManDeath", "VehicleDeath", "BuildingDeath",
    "DynamitePlaced", "NoPower", "PowerDrain", "AirDepleting", "AirLow",
    "AirOut", "AirRestored", "TrainDriver", "TrainDynamite", "TrainRepair",
    "TrainPilot", "TrainSailor", "TrainScanner", "OreCollected", "WallDug",
    "WallReinforced", "CrystalPower", "LavaErode", "SlugEmerge",
    "PathCompleted", "FoundMinifigure", "CanUpgradeMinifigure",
    "CanTrainMinifigure", "CrystalSeamFound", "GenericSeamFound",
    "GenericDeath", "GenericMonster",
};

void message_init_defaults(void)
{
    message_clear();
    for (int i = 0; i < MESSAGE_MAX_ID; ++i) {
        s_catalog[i] = s_catalog_defaults[i];
        s_cfg[i].name = s_catalog[i];
    }
    s_flags = 0x4u; /* system on */
}

void message_clear(void)
{
    int i;
    for (i = 0; i < s_message_node_count; ++i) {
        if (s_message_nodes[i] != NULL) {
            incident_message_node_release(s_message_nodes[i]);
            s_message_nodes[i] = NULL;
        }
    }
    s_message_node_count = 0;
    for (i = 0; i < MESSAGE_MAX_ID; ++i) {
        if (s_info_message_sound_names[i] != NULL) {
            engine_heap_free(s_info_message_sound_names[i]);
            s_info_message_sound_names[i] = NULL;
        }
        if (s_compiled_template[i] != NULL) {
            engine_heap_free(s_compiled_template[i]);
            s_compiled_template[i] = NULL;
        }
        if (s_info_message_sounds[i] != NULL) {
            sound_free(s_info_message_sounds[i]);
            s_info_message_sounds[i] = NULL;
        }
        if (s_info_message_surfaces[i] != NULL) {
            texture_release(s_info_message_surfaces[i]);
            s_info_message_surfaces[i] = NULL;
        }
    }
    for (i = 0; i < INCIDENT_NODE_TEXT_SLOTS; ++i) {
        if (s_node_text[i].text != NULL) {
            engine_heap_free(s_node_text[i].text);
        }
        if (s_node_text[i].node != NULL) {
            engine_heap_free(s_node_text[i].node);
        }
        s_node_text[i].node = NULL;
        s_node_text[i].text = NULL;
    }
    memset(s_idx_head, 0, sizeof(s_idx_head));
    memset(s_idx_count, 0, sizeof(s_idx_count));
    memset(s_idx_aux, 0, sizeof(s_idx_aux));
    s_panel_refresh_hook = NULL;
    s_panel_refresh_context = NULL;
    memset(&s_info_message_queue_tick_ops, 0,
           sizeof(s_info_message_queue_tick_ops));
    memset(&s_incident_message_text_ops, 0,
           sizeof(s_incident_message_text_ops));
    s_catalog_owner = 0u;
    memset(s_cfg, 0, sizeof(s_cfg));
    memset(s_queue_records, 0, sizeof(s_queue_records));
    memset(s_msg_catalog, 0, sizeof(s_msg_catalog));
    s_queue_count = 0;
    s_now = 0.0f;
    s_flags = 0;
    s_system_state = 0;
    s_objective_switch = 0u;
}

TextureSurface* info_message_surface_peek(int id)
{
    if (id < 0 || id >= MESSAGE_MAX_ID) return NULL;
    return s_info_message_surfaces[id];
}

/* Optional port audio adapter; it is not the resource table used by the
 * decompiled 0x419650/0x419b50 path. */
void info_message_sound_load(int id, const char* path)
{
    if (id < 0 || id >= MESSAGE_MAX_ID) return;
    if (s_info_message_sounds[id] != NULL) {
        sound_free(s_info_message_sounds[id]);
        s_info_message_sounds[id] = NULL;
    }
    if (path != NULL && path[0] != '\0')
        s_info_message_sounds[id] = sound_load(path);
}

struct Sound* info_message_sound_peek(int id)
{
    if (id < 0 || id >= MESSAGE_MAX_ID) {
        return NULL;
    }
    return s_info_message_sounds[id];
}

uint32_t incident_message_catalog_owner_get(void)
{
    return s_catalog_owner;
}

const char* incident_message_node_text_get(const void* node)
{
    uint32_t slot = 0;
    if (node == NULL) {
        return NULL;
    }
    memcpy(&slot, (const char*)node + 0x0c, sizeof(slot));
    if (slot == 0u || slot > INCIDENT_NODE_TEXT_SLOTS) {
        return NULL;
    }
    if (s_node_text[slot - 1u].node != node) {
        return NULL;
    }
    return s_node_text[slot - 1u].text;
}

void incident_message_node_release(void* node)
{
    if (node == NULL) {
        return;
    }
    for (int i = 0; i < INCIDENT_NODE_TEXT_SLOTS; ++i) {
        if (s_node_text[i].node == node) {
            if (s_node_text[i].text != NULL) {
                engine_heap_free(s_node_text[i].text);
            }
            s_node_text[i].node = NULL;
            s_node_text[i].text = NULL;
            break;
        }
    }
    engine_heap_free(node);
}

void message_config_set(int id, const char* sound, float cooldown,
                        uint32_t flags)
{
    if (id < 0 || id >= MESSAGE_MAX_ID) {
        return;
    }
    MessageConfig* c = &s_cfg[id];
    c->name = c->name != NULL ? c->name : s_catalog[id];
    c->sound = sound;
    c->cooldown = cooldown;
    c->flags = flags;
}

void message_set_flags(uint32_t flags)        { s_flags = flags; }
uint32_t message_flags_peek(void)             { return s_flags; }
uint32_t message_config_flags(int id)
{
    return (id >= 0 && id < MESSAGE_MAX_ID) ? s_cfg[id].flags : 0u;
}
void message_set_system_state(uint32_t v)     { s_system_state = v; }

uint32_t system_state_flags_set(const uint32_t* value)
{
    return value != NULL ? system_state_flags_set_target(value) : 0u;
}

uint32_t objective_switch_state_set(uint32_t value)
{
    s_objective_switch = value;
    return value;
}

void message_set_callback(MessageCallback cb) { s_callback = cb; }
void message_set_sound_hook(MessageSoundHook h) { s_sound = h; }

const char* message_sound(int id)
{
    if (id < 0 || id >= MESSAGE_MAX_ID) {
        return NULL;
    }
    return s_cfg[id].sound;
}

void message_update(float dt)
{
    s_now += dt;
}

void info_message_queue_tick_ops_set(const InfoMessageQueueTickOps* ops)
{
    if (ops != NULL) s_info_message_queue_tick_ops = *ops;
    else memset(&s_info_message_queue_tick_ops, 0,
                sizeof(s_info_message_queue_tick_ops));
}

void incident_message_text_consumer_ops_set(
    const IncidentMessageTextConsumerOps* ops)
{
    if (ops != NULL) s_incident_message_text_ops = *ops;
    else memset(&s_incident_message_text_ops, 0,
                sizeof(s_incident_message_text_ops));
}

int message_active_count(void)
{
    return s_queue_count;
}

int message_active_has(int id)
{
    return message_queue_find_id(id) >= 0;
}

float message_cooldown(int id)
{
    if (id < 0 || id >= MESSAGE_MAX_ID) {
        return 0.0f;
    }
    return s_cfg[id].cooldown;
}

uint32_t deferred_callback_peek(void)
{
    return s_deferred_callback_presence;
}

/* HasActiveResource (FUN_004119d0) — see message.hpp. */
// FUNCTION: LEGORR 0x004119d0
int has_active_resource(const void* record)
{
    uint32_t handle = 0u;
    if (record != NULL) {
        __builtin_memcpy(&handle, (const char*)record + 0x38, sizeof handle);
    }
    if (record == NULL || handle == 0u) {
        return 0;
    }
    if (system_state_flags_get() == 0u) {
        return 0;
    }
    return 1;
}

/* --- the indexed-list window (see message.hpp) ---------------------------- */

void indexed_list_install(int list, void* head, int count)
{
    if (list >= 0 && list < IDX_LIST_SLOTS) {
        s_idx_head[list] = head;
        s_idx_count[list] = count;
    }
}

void incident_message_panel_refresh_hook_install(
    MessagePanelRefreshHook hook, void* context)
{
    s_panel_refresh_hook = hook;
    s_panel_refresh_context = context;
}

void incident_message_panel_aux_install(void* auxiliary)
{
    s_idx_aux[0] = auxiliary;
}

