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

/* UI-owned operations used by IncidentMessageTextConsumer. The text output
 * record layout and implementations live in game/frontend.cpp; forward
 * declarations avoid pulling its broad header into the engine message module. */
extern uint32_t interaction_flags_peek(void);
extern void interaction_flags_set(uint32_t flags);

// GLOBAL: LEGORR 0x00503190
static const char* s_ui_message_labels[26] = {
    "Text_Walk", "Text_Drill", "Text_NoDrill", "Text_CantDrill",
    "Text_Reinforce", "Text_NoReinforce", "Text_CantReinforce",
    "Text_PickupCrystal", "Text_Encyclopedia", "Text_RadarHelp",
    "Text_BuildHelp", "Text_SelectionHelp", "Text_Drive",
    "Text_CantDrive", "Text_MakeTeleporter", "Text_CannotPlaceBuilding",
    "Text_CrystalOreDisabled", "Text_CrystalFound", "Text_CavernDiscovered",
    "Text_OreFound", "Text_AirSupplyLow", "Text_AirSupplyRunningOut",
    "Text_SpaceToContinue", "Text_GameCompleted", "Text_ManTrained",
    "Text_UnitUpgraded"
};
static void* s_ui_message_primary_font;
static void* s_ui_message_secondary_font;
static void* s_ui_message_resource;
static void* s_ui_message_subtitle_font;
static float s_ui_message_line_delay = 100.0f;
static unsigned int s_ui_message_count = 26u;
static unsigned int s_ui_message_slot_count = 26u;
static unsigned int s_ui_message_current;
static uint32_t s_ui_message_flags;
// GLOBAL: LEGORR 0x005031f8
static char* s_ui_message_text_slots[26];
static TextureSurface* s_ui_message_image_slots[26];
static char s_ui_message_image_names[26][32];

// FUNCTION: LEGORR 0x0046ab10
void ui_message_text_slot_assign(int index, const char* text)
{
    size_t length;
    const char* source;
    char* target;
    if (index < 0 || index >= 26 || text == NULL) return;
    engine_heap_free_wrapper(s_ui_message_text_slots[index]);
    s_ui_message_text_slots[index] = NULL;
    length = strlen(text);
    s_ui_message_text_slots[index] = (char*)engine_heap_alloc((long)length + 1);
    if (s_ui_message_text_slots[index] != NULL) {
        source = text;
        target = s_ui_message_text_slots[index];
        while (*source != '\0') {
            if (*source == '_') {
                *target = ' ';
            } else if (*source == '\\' && source[1] == 'n') {
                *target = '\n';
                ++source;
            } else {
                *target = *source;
            }
            ++source;
            ++target;
        }
        *target = '\0';
    }
    texture_release(s_ui_message_image_slots[index]);
    s_ui_message_image_slots[index] = NULL;
}

// FUNCTION: LEGORR 0x0046abb0
void ui_message_text_slot_assign_with_image(int index, const char* text,
                                            const char* image_path,
                                            const char* display_path)
{
    if (index < 0 || index >= 26) return;
    ui_message_text_slot_assign(index, text);
    snprintf(s_ui_message_image_names[index],
             sizeof(s_ui_message_image_names[index]), "%s",
             display_path != NULL ? display_path : "");
    s_ui_message_image_slots[index] = image_path != NULL
        ? image_surface_load(image_path, 0u, 0u) : NULL;
    if (s_ui_message_image_slots[index] != NULL) {
        texture_source_color_key_set(s_ui_message_image_slots[index], 0u, 0u);
    }
}

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

// FUNCTION: LEGORR 0x0046a7f0
void ui_message_strings_initialize(void* primary_font, void* secondary_font,
                                   void* message_resource, void* subtitle_font,
                                   float line_delay)
{
    s_ui_message_primary_font = primary_font;
    s_ui_message_secondary_font = secondary_font;
    s_ui_message_resource = message_resource;
    s_ui_message_subtitle_font = subtitle_font;
    s_ui_message_count = 26u;
    s_ui_message_slot_count = 26u;
    s_ui_message_line_delay = (line_delay == 0.0f) ? 100.0f : line_delay;
}

void* ui_message_text_output_get(int alternate)
{
    return alternate != 0 ? s_ui_message_secondary_font :
                            s_ui_message_primary_font;
}

// FUNCTION: LEGORR 0x0046aa90
void ui_message_overlay_state_initialize(void)
{
    s_ui_message_count = 26u;
    s_ui_message_current = 0u;
    s_ui_message_flags = 0u;
    text_buffer_reset(s_ui_message_primary_font);
    text_buffer_reset(s_ui_message_secondary_font);
}

const char* ui_message_label_get(unsigned int index)
{
    return index < 26u ? s_ui_message_labels[index] : NULL;
}

// FUNCTION: LEGORR 0x0046aad0
int ui_message_label_index_find(const char* label, int* out_index)
{
    int i;
    if (label == NULL) return 0;
    for (i = 0; i < 26; ++i) {
        if (strcasecmp_lite(s_ui_message_labels[i], label) == 0) {
            if (out_index == NULL) return 0;
            *out_index = i;
            return 1;
        }
    }
    return 0;
}

static char* s_info_message_sound_names[MESSAGE_MAX_ID];
// GLOBAL: LEGORR 0x004ff980
uint32_t      s_system_state = 0u;   /* DAT_004ff980 */
// GLOBAL: LEGORR 0x004ff9bc
uint32_t             s_objective_switch = 0u; /* DAT_004ff9bc */
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
#define CATALOG_SLOT_CAPACITY 256
static CatalogSlotPredicateHook s_catalog_slot_eligible;
static CatalogSlotPredicateHook s_catalog_slot_available;
static CatalogSlotRankHook s_catalog_slot_rank;
static void* s_catalog_slot_context;
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

static int catalog_slot_table_index(int category)
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

// FUNCTION: LEGORR 0x0041c800
void catalog_slot_flag_update(int category, int slot)
{
    uint32_t flags;
    int should_clear = 0;
    if (catalog_slot_table_index(category) < 0 || slot < 0 ||
        slot >= CATALOG_SLOT_CAPACITY) return;
    flags = input_binding_state_word_get(category, slot);
    if ((flags & 2u) == 0 &&
        (interaction_flags_peek() & 0x80u) != 0u) {
        if (s_catalog_slot_eligible != NULL
            && s_catalog_slot_available != NULL
            && s_catalog_slot_eligible(category, slot,
                                        s_catalog_slot_context) != 0
            && s_catalog_slot_available(category, slot,
                                        s_catalog_slot_context) != 0) {
            if (category == 4) {
                should_clear = 1;
            }
            else {
                /* The retail path calls FUN_00469af0, then the empty
                 * FUN_0041f8e0. Its EAX is therefore still the rank, so the
                 * caller's `rank <= result` test is rank <= rank. Preserve
                 * the rank query's side effects when a live owner supplies
                 * one, but don't replace that no-op with an invented limit. */
                if (s_catalog_slot_rank != NULL)
                    (void)s_catalog_slot_rank(category, slot,
                                              s_catalog_slot_context);
                should_clear = 1;
            }
        }
        if (should_clear) flags &= ~1u;
        else flags |= 1u;
    }
    else {
        flags |= 1u;
    }
    input_binding_state_word_set(category, slot, flags);
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
// GLOBAL: LEGORR 0x004dcd58
uint32_t s_incident_state;

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

// FUNCTION: LEGORR 0x00456240
uint32_t system_state_flags_get(void)
{
    return s_system_state;
}

// FUNCTION: LEGORR 0x00456550
uint32_t system_state_flags_set_target(const uint32_t* value)
{
    s_system_state = *value;
    return *value;
}

uint32_t system_state_flags_set(const uint32_t* value)
{
    return value != NULL ? system_state_flags_set_target(value) : 0u;
}

uint32_t objective_switch_state_set(uint32_t value)
{
    s_objective_switch = value;
    return value;
}

// FUNCTION: LEGORR 0x004567d0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) uint32_t objective_switch_state_get(void)
{
    __asm {
        mov eax, dword ptr [s_objective_switch]
        test eax, eax
        je objective_switch_state_get_false
        mov dword ptr [s_objective_switch], 0
        mov eax, 1
        ret
    objective_switch_state_get_false:
        xor eax, eax
        ret
    }
}
#else
uint32_t objective_switch_state_get(void)
{
    /* FUN_004567d0 consumes the pending state and returns a boolean. */
    if (s_objective_switch == 0u) return 0u;
    s_objective_switch = 0u;
    return 1u;
}
#endif

void message_set_callback(MessageCallback cb) { s_callback = cb; }
void message_set_sound_hook(MessageSoundHook h) { s_sound = h; }

const char* message_sound(int id)
{
    if (id < 0 || id >= MESSAGE_MAX_ID) {
        return NULL;
    }
    return s_cfg[id].sound;
}

// FUNCTION: LEGORR 0x00434c10
void message_load_infomessages(BlockFile* cfg)
{
    static const char separators[] = ":";
    static const char slow_mode[] = "SLOW";
    static const char always_play[] = "ALWAYSPLAYSFX";
    if (cfg == NULL) {
        return;
    }
    /* Find any `InfoMessages` record (nested under Lego*; the parser keeps
     * document order) and configure each child by its Info name. */
    size_t n = block_record_count(cfg);
    for (size_t i = 0; i < n; ++i) {
        BlockRec* rec = block_record_at(cfg, i);
        if (rec == NULL || block_name(rec) == NULL
            || strcasecmp_lite(block_name(rec), "InfoMessages") != 0) {
            continue;
        }
        for (BlockRec* kid = block_first_child(rec); kid != NULL;
             kid = block_next_sibling(kid)) {
            const char* nm = block_name(kid);
            const char* val = block_value(kid);
            if (nm == NULL) {
                continue;
            }
            /* Entries may carry the Info_ prefix (finding 22's stored
             * strings are `Info_*`); the catalog is bare. */
            if (strncmp(nm, "Info_", 5) == 0) {
                nm += 5;
            }
            int id = -1;
            for (int k = 0; k < MESSAGE_MAX_ID; ++k) {
                if (strcasecmp_lite(nm, s_catalog[k]) == 0) {
                    id = k;
                    break;
                }
            }
            if (id < 0) {
                continue;
            }
            char value[1024];
            char* fields[1024];
            int field_count;
            size_t value_length;
            int sound_alias;
            char* sound_name = NULL;
            if (val == NULL) continue;
            value_length = strlen(val);
            if (value_length >= sizeof(value))
                value_length = sizeof(value) - 1u;
            memcpy(value, val, value_length);
            value[value_length] = '\0';
            field_count = sound_variant_split(value, fields, separators);
            if (field_count < 2) continue;

            if (field_count > 2 && fields[2][0] != '\0') {
                size_t sound_length = strlen(fields[2]);
                sound_name = (char*)engine_heap_alloc(
                    (long)(sound_length + 1u));
                if (sound_name != NULL) {
                    memcpy(sound_name, fields[2], sound_length + 1u);
                }
            }
            if (s_info_message_sound_names[id] != NULL)
                engine_heap_free(s_info_message_sound_names[id]);
            s_info_message_sound_names[id] = sound_name;

            /* InfoMessages stores display text, image resource, optional
             * sound alias, and two independent mode markers as colon fields. */
            message_config_set(id, sound_name, 0.0f, 0u);
            incident_message_template_replace(id, fields[0]);
            info_message_surface_load(id, fields[1]);

            if (field_count > 2 &&
                sound_alias_lookup(fields[2], &sound_alias) != 0) {
                info_message_secondary_sound_set(id,
                                                   (uint32_t)sound_alias);
            }
            if (field_count < 4) {
                info_message_always_play_flag_set(id, 1);
                indexed_selection_flag_20000_set_clear(id, 0);
            }
            else {
                info_message_always_play_flag_set(
                    id, strcasecmp_lite(fields[3], slow_mode) == 0);
                indexed_selection_flag_20000_set_clear(id,
                    strcasecmp_lite(fields[3], always_play) == 0 ||
                    (field_count >= 5 &&
                     strcasecmp_lite(fields[4], always_play) == 0));
            }
        }
    }
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

/* DeferredCallbackSet (FUN_00453900) — zero-init module static (the
 * corpus's DAT_004a673c presence word). */
// GLOBAL: LEGORR 0x004a673c
static uint32_t s_deferred_callback_presence;

// FUNCTION: LEGORR 0x00453900
void deferred_callback_set(uint32_t value)
{
    s_deferred_callback_presence = value;
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

