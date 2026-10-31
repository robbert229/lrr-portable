// SPDX-License-Identifier: LGPL-3.0-or-later
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "game/savegame.hpp"
#include "heap.hpp"
#include "message.hpp"
#include "sound.hpp"
#include "string.hpp"
#include "texture.hpp"

#define MESSAGE_CD_RESET 25.0f /* 0x41c80000 */
#define IDX_LIST_SLOTS 16 /* deviation: the corpus strides the list index */

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
static unsigned int s_ui_message_current;
static uint32_t s_ui_message_flags;
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
}

const char* ui_message_text_slot_get(unsigned int index)
{
    return index < 26u ? s_ui_message_text_slots[index] : NULL;
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
    s_ui_message_current = 0u;
    s_ui_message_line_delay = (line_delay == 0.0f) ? 100.0f : line_delay;
}

// FUNCTION: LEGORR 0x0046aa90
void ui_message_overlay_state_initialize(void)
{
    s_ui_message_count = 26u;
    s_ui_message_current = 0u;
    s_ui_message_flags = 0u;
    if (s_ui_message_line_delay == 0.0f) s_ui_message_line_delay = 100.0f;
}

const char* ui_message_label_get(unsigned int index)
{
    return index < 26u ? s_ui_message_labels[index] : NULL;
}

// FUNCTION: LEGORR 0x0046aad0
int ui_message_label_index_find(const char* label, int* out_index)
{
    int i;
    if (label == NULL || out_index == NULL) return 0;
    for (i = 0; i < 26; ++i) {
        if (strcasecmp_lite(s_ui_message_labels[i], label) == 0) {
            *out_index = i;
            return 1;
        }
    }
    return 0;
}

/* The corpus's decay gate compares each countdown against this .rdata zero
 * dword (objdump-verified fcomp operand of InfoMessageTimerDecay); a named
 * const static keeps the fcomp operand resolvable by Reccmp. */
// GLOBAL: LEGORR 0x0049e264
static const float s_cooldown_zero = 0.0f;   /* DAT_0049e264, verified */

/* The corpus's 40 stride-0x14 incident-message records (MessageConfig);
 * the countdown at +0x00 is what the corpus walk reads and resets. */
// GLOBAL: LEGORR 0x004dc704
static MessageConfig s_cfg[MESSAGE_MAX_ID];
/* The corpus keeps each queued message as one 0x14-byte record at
 * DAT_004dca20 with the id at +0 (the timestamp rides inside the record);
 * the parallel-array model emitted add ecx, 4 where the corpus strides
 * 0x14. Own storage, zero-init like the corpus's zero-filled tail. */
struct MessageQueueRecord {
    uint32_t id;
    float timestamp;
    unsigned char pad[12];
};
// GLOBAL: LEGORR 0x004dca20
static MessageQueueRecord s_queue_records[MESSAGE_MAX_ID];
// GLOBAL: LEGORR 0x004dcd38
static int           s_queue_count = 0;
// GLOBAL: LEGORR 0x004dcd48
static uint32_t      s_flags = 0u;          /* DAT_004dcd48 */
// GLOBAL: LEGORR 0x004ff980
static uint32_t      s_system_state = 0u;   /* DAT_004ff980 */
static float         s_now = 0.0f;          /* _DAT_0049e264 analog */
static MessageCallback  s_callback = NULL;
static MessageSoundHook s_sound = NULL;
static char* s_compiled_template[MESSAGE_MAX_ID];
static Sound* s_info_message_sounds[MESSAGE_MAX_ID];
static uint32_t s_catalog_owner;
struct IncidentNodeTextSlot {
    void* node;
    char* text;
};
#define INCIDENT_NODE_TEXT_SLOTS 256
static IncidentNodeTextSlot s_node_text[INCIDENT_NODE_TEXT_SLOTS];
static void* s_idx_head[IDX_LIST_SLOTS];  /* DAT_004dca18 role */
static int   s_idx_count[IDX_LIST_SLOTS]; /* DAT_004dca1c role */
static void* s_idx_aux[IDX_LIST_SLOTS];
static MessagePanelRefreshHook s_panel_refresh_hook;
static void* s_panel_refresh_context;
#define CATALOG_SLOT_CAPACITY 256
static uint32_t s_catalog_slot_flags[2][CATALOG_SLOT_CAPACITY];
static CatalogSlotPredicateHook s_catalog_slot_eligible;
static CatalogSlotPredicateHook s_catalog_slot_available;
static CatalogSlotRankHook s_catalog_slot_rank;
static CatalogSlotLimitHook s_catalog_slot_limit;
static void* s_catalog_slot_context;
static uint32_t s_catalog_slot_global_flags;

void catalog_slot_flag_hooks_set(CatalogSlotPredicateHook eligible,
                                 CatalogSlotPredicateHook available,
                                 CatalogSlotRankHook rank,
                                 CatalogSlotLimitHook limit, void* context)
{
    s_catalog_slot_eligible = eligible;
    s_catalog_slot_available = available;
    s_catalog_slot_rank = rank;
    s_catalog_slot_limit = limit;
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
    int table = catalog_slot_table_index(category);
    if (table < 0 || slot < 0 || slot >= CATALOG_SLOT_CAPACITY) return;
    s_catalog_slot_flags[table][slot] = flags;
}

uint32_t catalog_slot_flag_word_get(int category, int slot)
{
    int table = catalog_slot_table_index(category);
    if (table < 0 || slot < 0 || slot >= CATALOG_SLOT_CAPACITY) return 0;
    return s_catalog_slot_flags[table][slot];
}

void catalog_slot_flag_global_state_set(uint32_t flags)
{
    s_catalog_slot_global_flags = flags;
}

/* FUNCTION: LEGORR 0x0041c800 */
void catalog_slot_flag_update(int category, int slot)
{
    int table = catalog_slot_table_index(category);
    uint32_t flags;
    int should_clear = 0;
    if (table < 0 || slot < 0 || slot >= CATALOG_SLOT_CAPACITY) return;
    flags = s_catalog_slot_flags[table][slot];
    if ((flags & 2u) == 0 && (s_catalog_slot_global_flags & 0x80u) != 0) {
        if (s_catalog_slot_eligible != NULL
            && s_catalog_slot_available != NULL
            && s_catalog_slot_eligible(category, slot,
                                        s_catalog_slot_context) != 0
            && s_catalog_slot_available(category, slot,
                                        s_catalog_slot_context) != 0) {
            if (category == 4) {
                should_clear = 1;
            }
            else if (s_catalog_slot_rank != NULL
                     && s_catalog_slot_limit != NULL
                     && s_catalog_slot_rank(category, slot,
                                            s_catalog_slot_context)
                        <= s_catalog_slot_limit(category,
                                                s_catalog_slot_context)) {
                should_clear = 1;
            }
        }
        if (should_clear) flags &= ~1u;
        else flags |= 1u;
    }
    else {
        flags |= 1u;
    }
    s_catalog_slot_flags[table][slot] = flags;
}

/* The InfoMessages catalog — finding 22's 40 names, in table order. The
 * corpus copies these pointers into the zero-filled runtime table at
 * DAT_004dc658 during CatalogInit. */
static const char* const s_catalog_defaults[MESSAGE_MAX_ID] = {
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
// GLOBAL: LEGORR 0x004dc658
static const char* s_catalog[MESSAGE_MAX_ID];

static int s_in_family(int id)
{
    static const int family[] = MESSAGE_FAMILY;
    for (size_t i = 0; i < sizeof(family) / sizeof(family[0]); ++i) {
        if (family[i] == id) {
            return 1;
        }
    }
    return 0;
}

static void reset_family_cooldown(void)
{
    static const int family[] = MESSAGE_FAMILY;
    for (size_t i = 0; i < sizeof(family) / sizeof(family[0]); ++i) {
        s_cfg[family[i]].cooldown = s_now + MESSAGE_CD_RESET;
    }
    if (s_sound != NULL) {
        for (size_t i = 0; i < sizeof(family) / sizeof(family[0]); ++i) {
            s_sound(family[i], 0.0f);
        }
    }
}

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
    for (i = 0; i < MESSAGE_MAX_ID; ++i) {
        if (s_compiled_template[i] != NULL) {
            engine_heap_free(s_compiled_template[i]);
            s_compiled_template[i] = NULL;
        }
        if (s_info_message_sounds[i] != NULL) {
            sound_free(s_info_message_sounds[i]);
            s_info_message_sounds[i] = NULL;
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
    s_catalog_owner = 0u;
    memset(s_cfg, 0, sizeof(s_cfg));
    memset(s_queue_records, 0, sizeof(s_queue_records));
    s_queue_count = 0;
    s_now = 0.0f;
    s_flags = 0;
    s_system_state = 0;
}

/* IncidentMessageCatalogInit (FUN_00419230): the corpus copies the static
 * Info name table into its global pointer band, stores one opaque owner word
 * and raises flags 0x18. The native port already owns the same names in
 * s_catalog, so only the stateful effects need recording. */
// FUNCTION: LEGORR 0x00419230
void incident_message_catalog_init(uint32_t owner)
{
    for (int i = 0; i < MESSAGE_MAX_ID; ++i) {
        s_catalog[i] = s_catalog_defaults[i];
    }
    s_catalog_owner = owner;
    s_flags |= 0x18u;
}

/* InfoMessageSoundLoad (FUN_00419650): the native port loads the configured
 * message sound through the sound subsystem and retains the resulting handle.
 * The DirectDraw resource-enable call that followed the original DDiResource
 * constructor has no direct native equivalent. */
// FUNCTION: LEGORR 0x00419650
void info_message_sound_load(int id, const char* path)
{
    if (id < 0 || id >= MESSAGE_MAX_ID) {
        return; /* deviation: corpus indexes the sound table blindly */
    }
    if (s_info_message_sounds[id] != NULL) {
        sound_free(s_info_message_sounds[id]);
        s_info_message_sounds[id] = NULL;
    }
    if (path != NULL && path[0] != '\0') {
        s_info_message_sounds[id] = sound_load(path);
    }
}

struct Sound* info_message_sound_peek(int id)
{
    if (id < 0 || id >= MESSAGE_MAX_ID) {
        return NULL;
    }
    return s_info_message_sounds[id];
}

/* RemainingTimerDurationGet (FUN_00419b50): the corpus starts at 333 and
 * subtracts each queued resource's +8 duration. The native sound owner uses
 * PCM byte-rate metadata to derive that duration; missing resource handles
 * contribute zero (documented partial-resource deviation). */
// FUNCTION: LEGORR 0x00419B50
float remaining_timer_duration_get(void)
{
    float remaining = 333.0f;
    for (int i = 0; i < s_queue_count; ++i) {
        int id = (int)s_queue_records[i].id;
        if (id >= 0 && id < MESSAGE_MAX_ID
            && s_info_message_sounds[id] != NULL
            && s_info_message_sounds[id]->format.byte_rate != 0u) {
            remaining -= (float)s_info_message_sounds[id]->data_len
                / (float)s_info_message_sounds[id]->format.byte_rate;
        }
    }
    return remaining;
}

uint32_t incident_message_catalog_owner_get(void)
{
    return s_catalog_owner;
}

/* IncidentMessageTemplateCompile (FUN_00419530). Name tokens found in the
 * Info catalog become `%b` substitutions whose catalog value is stored in
 * the five-byte trailer. The cursor-name resolver has no port owner yet, so
 * unresolved cursor tokens are omitted (documented deviation). */
// FUNCTION: LEGORR 0x00419530
void incident_message_template_compile(const char* text, char** out)
{
    size_t input_length;
    size_t source;
    size_t dest;
    char* compiled;
    unsigned char substitution[5];
    unsigned int substitution_count = 0;
    if (out == NULL) {
        return;
    }
    *out = NULL;
    if (text == NULL) {
        return;
    }
    input_length = strlen(text);
    if (input_length > 0x7ffffff0u) {
        return;
    }
    compiled = (char*)engine_heap_alloc((long)(input_length + 11u));
    if (compiled == NULL) {
        return;
    }
    source = 0;
    dest = 0;
    while (source < input_length) {
        char ch = text[source];
        if (ch == '_') {
            compiled[dest++] = ' ';
            ++source;
        } else if (ch == '\\' && source + 1u < input_length
                   && text[source + 1u] == 'n') {
            compiled[dest++] = '\n';
            source += 2u;
        } else if (ch == '$') {
            size_t end = source + 1u;
            while (end < input_length && text[end] != '$') {
                ++end;
            }
            if (end == input_length) {
                compiled[dest++] = ch;
                ++source;
            } else {
                size_t token_length = end - source - 1u;
                char token[32];
                int id = -1;
                if (token_length < sizeof(token)) {
                    memcpy(token, text + source + 1u, token_length);
                    token[token_length] = '\0';
                    if (incident_message_config_lookup(token, &id) != 0
                        && substitution_count < 5u) {
                        uint32_t code = incident_message_text_catalog_get(id);
                        if (code != 0u) {
                            compiled[dest++] = '%';
                            compiled[dest++] = 'b';
                            substitution[substitution_count++] = (unsigned char)code;
                        }
                    }
                }
                source = end + 1u;
            }
        } else {
            compiled[dest++] = ch;
            ++source;
        }
    }
    compiled[dest++] = '\0';
    compiled[dest++] = (char)substitution_count;
    for (unsigned int i = 0; i < 5u; ++i) {
        compiled[dest++] = i < substitution_count
            ? (char)substitution[i] : '\0';
    }
    *out = compiled;
}

/* IncidentMessageTemplateReplace (FUN_004194b0): release the previous
 * compiled string, then compile and store the replacement for this id. */
// FUNCTION: LEGORR 0x004194B0
void incident_message_template_replace(int id, const char* text)
{
    if (id < 0 || id >= MESSAGE_MAX_ID) {
        return; /* deviation: corpus indexes this band without a guard */
    }
    if (s_compiled_template[id] != NULL) {
        engine_heap_free(s_compiled_template[id]);
        s_compiled_template[id] = NULL;
    }
    incident_message_template_compile(text, &s_compiled_template[id]);
}

/* IncidentMessageNodeCreate (FUN_00419a80): the original allocation is
 * five dwords {pair[0], pair[1], value, text, next}. On 64-bit hosts, the
 * native next pointer remains at +0x10 and the compiled text pointer is
 * held in a sidecar slot whose 32-bit index occupies the original +0x0c
 * word. This preserves the linked-list field offset without truncating a
 * host pointer. */
// FUNCTION: LEGORR 0x00419A80
void* incident_message_node_create(const uint32_t* first_pair,
                                   uint32_t value, const char* text)
{
    struct NativeIncidentMessageNode {
        uint32_t first;
        uint32_t second;
        uint32_t value;
        uint32_t text_slot;
        void* next;
    };
    NativeIncidentMessageNode* node;
    char* compiled_text = NULL;
    int slot = -1;
    node = (NativeIncidentMessageNode*)engine_heap_alloc(
        (long)sizeof(NativeIncidentMessageNode));
    if (node == NULL) {
        return NULL;
    }
    memset(node, 0, sizeof(NativeIncidentMessageNode));
    if (first_pair == NULL) {
        node->first = 0xffffffffu;
        node->second = 0xffffffffu;
    } else {
        node->first = first_pair[0];
        node->second = first_pair[1];
    }
    node->value = value;
    if (text != NULL) {
        incident_message_template_compile(text, &compiled_text);
        if (compiled_text == NULL) {
            engine_heap_free(node);
            return NULL;
        }
        for (int i = 0; i < INCIDENT_NODE_TEXT_SLOTS; ++i) {
            if (s_node_text[i].node == NULL) {
                slot = i;
                break;
            }
        }
        if (slot < 0) {
            engine_heap_free(compiled_text);
            engine_heap_free(node);
            return NULL; /* deviation: bounded sidecar table */
        }
        s_node_text[slot].node = node;
        s_node_text[slot].text = compiled_text;
        node->text_slot = (uint32_t)slot + 1u;
    }
    return node;
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
uint32_t system_state_flags_set(const uint32_t* value)
{
    if (value == NULL) {
        return 0u; /* deviation: the corpus dereferences blindly */
    }
    s_system_state = *value;
    return *value;
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

// FUNCTION: LEGORR 0x00434c10
void message_load_infomessages(BlockFile* cfg)
{
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
            uint32_t flags = 0;
            const char* sound = NULL;
            if (val != NULL && strcasecmp_lite(val, "AlwaysPlaySFX") != 0
                && strlen(val) >= 4) {
                sound = val; /* corpus: short/empty flag strings mute */
            }
            if (val == NULL || strcasecmp_lite(val, "AlwaysPlaySFX") == 0
                || strlen(val) < 4) {
                flags |= MESSAGE_FLAG_PLAY_ALWAYS;
            }
            message_config_set(id, sound, 0.0f, flags);
        }
    }
}

/* Dedupe-with-live-callback (corpus MessagePreCheck FUN_004199f0). */
// FUNCTION: LEGORR 0x004199f0
static int message_pre_check(int id)
{
    if (id == 10 || id == 11 || id == 12 || id == 38) {
        return 0; /* exception list skips dedupe */
    }
    for (int i = 0; i < s_queue_count; ++i) {
        if (s_queue_records[i].id == id) {
            /* the queued entry's stored callback runs immediately; when a
             * callback hook exists and handles it, the re-dispatch is
             * suppressed. */
            if (s_callback == NULL || s_callback(id) != 0) {
                return 1;
            }
        }
    }
    return 0;
}

/* IncidentMessageConfigLookup (FUN_004193e0) — see message.hpp. The corpus
 * calls the shared case-insensitive compare strcasecmp_lite (0x48ff10) on
 * each table entry blindly, with no null check before the call; the walk
 * strides the port's stride-4 name table (s_catalog, DAT_004dc658) exactly
 * as the corpus strides its name array. */
// FUNCTION: LEGORR 0x004193e0
int incident_message_config_lookup(const char* name, int* out_id)
{
    if (name == NULL || out_id == NULL) {
        return 0; /* pinned deviation: the corpus strides both blindly */
    }
    /* stride-4 name-table walk, as the corpus: unsigned end compare (the
     * loop-back is jb) with the index counter beside the pointer (edi) */
    const char* const* entry = s_catalog;
    const char* const* end = s_catalog + MESSAGE_MAX_ID;
    int i = 0;
    for (; entry < end; ++entry, ++i) {
        if (strcasecmp_lite(*entry, name) == 0) {
            *out_id = i;
            return 1;
        }
    }
    return 0;
}

/* InfoMessageQueueFindId (FUN_00419b20): the queue's index-by-id walk.
 * The corpus strides the 5-dword queue records against the count word;
 * the port's record array mirrors that layout. */
// FUNCTION: LEGORR 0x00419b20
int message_queue_find_id(int id)
{
    for (int i = 0; i < s_queue_count; ++i) {
        if (s_queue_records[i].id == id) {
            return i;
        }
    }
    return -1;
}

// FUNCTION: LEGORR 0x00419b80
void message_dispatch(int id, uint32_t p2, uint32_t p3, const void* p4)
{
    (void)p2; (void)p3; (void)p4;
    if (id < 0 || id >= MESSAGE_MAX_ID) {
        return;
    }
    if ((s_flags & 0x4u) == 0) {
        return;
    }
    if ((s_system_state & 0x100u) != 0) {
        return; /* game not live */
    }
    MessageConfig* c = &s_cfg[id];

    /* auto-cooldown rule (finding 04): ids flagged 0x20000 with a spent
     * cooldown reset the linked family + mute. */
    if (((s_flags & 0x8u) != 0) && ((c->flags & 0x20000u) != 0)
        && (c->cooldown <= s_now)) {
        reset_family_cooldown();
        return;
    }

    if (message_pre_check(id)) {
        return; /* duplicate handled live */
    }

    /* configured? (corpus FUN_00419b00: a bound entry points at config) */
    if (c->name == NULL && c->sound == NULL) {
        return;
    }

    /* second cooldown rule for non-flagged ids. */
    if (((s_flags & 0x8u) != 0) && ((c->flags & 0x20000u) == 0)
        && (c->cooldown <= s_now)) {
        if (s_in_family(id)) {
            reset_family_cooldown();
        } else {
            c->cooldown = s_now + MESSAGE_CD_RESET;
        }
        if (s_sound != NULL) {
            s_sound(id, 0.0f);
        }
        return;
    }

    if (s_queue_count < MESSAGE_MAX_ID) {
        int idx = message_queue_find_id(id);
        if (idx < 0) {
            idx = s_queue_count;
            s_queue_records[idx].timestamp = s_now;
            s_queue_count += 1;
        }
        s_queue_records[idx].id = id;
        if (s_callback != NULL) {
            s_callback(id);
        }
    }
}

void message_update(float dt)
{
    s_now += dt;
}

/* corpus FUN_0041a2c0 (InfoMessageTimerDecay): walks the 40 stride-0x14
 * records at DAT_004dc704 (add ecx, 0x14; end at 0x4dca24) and subtracts
 * the frame delta from each strictly-positive countdown — the corpus gates
 * with fcomp against the .rdata 0.0 dword (0x49e264), test ah, 0x41. */
// FUNCTION: LEGORR 0x0041a2c0
void message_cooldown_tick(float dt)
{
    MessageConfig* rec;

    /* pointer walk, as the corpus: the end compare lands on &s_cfg[40] and
     * stays unsigned (the corpus loop-back is jb, not jl) */
    rec = s_cfg;
    for (; rec < &s_cfg[MESSAGE_MAX_ID]; ++rec) {
        if (rec->cooldown > s_cooldown_zero) {
            rec->cooldown -= dt;
        }
    }
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
/* --- corpus flag leaves (see message.hpp) -------------------------------- */
// FUNCTION: LEGORR 0x00419450
void selection_flag10_set_clear(int set)
{
    if (set != 0) {
        s_flags = s_flags | 0x10u;
    } else {
        s_flags = s_flags & ~0x10u;
    }
}

// FUNCTION: LEGORR 0x00419470
void info_message_always_play_flag_set(int id, int set)
{
    if (id < 0 || id >= MESSAGE_MAX_ID) {
        return; /* deviation: the corpus indexes blindly */
    }
    MessageConfig* c = &s_cfg[id];
    if (set != 0) {
        c->flags = c->flags | 0x10000u;
    } else {
        c->flags = c->flags & ~0x10000u;
    }
}

// FUNCTION: LEGORR 0x004194f0
void indexed_selection_flag_20000_set_clear(int id, int set)
{
    if (id < 0 || id >= MESSAGE_MAX_ID) {
        return; /* deviation: the corpus indexes blindly */
    }
    MessageConfig* c = &s_cfg[id];
    if (set != 0) {
        c->flags = c->flags | 0x20000u;
    } else {
        c->flags = c->flags & ~0x20000u;
    }
}

/* --- the incident/message DAT-global band (see message.hpp) -----------------
 * Zero-init storage per the unowned-global convention (finding 05's all-zero
 * byte-read is the faithful initial state); the corpus's index math and the
 * stride-0x14 record window are reproduced. */
#define MSG_DAT_WINDOW 0x28 /* the corpus's 40-entry block per finding 05 */
// GLOBAL: LEGORR 0x004dc6fc
static uint32_t s_msg_catalog[MSG_DAT_WINDOW]; /* DAT_004dc6fc stride-0x14 block */
// GLOBAL: LEGORR 0x004dc700
static uint32_t s_msg_secondary[MSG_DAT_WINDOW]; /* DAT_004dc700 stride-0x14 block */
// GLOBAL: LEGORR 0x004dcd58
static uint32_t s_incident_state;              /* DAT_004dcd58 */

// FUNCTION: LEGORR 0x00419420
uint32_t incident_message_text_catalog_get(int id)
{
    if (id < 0 || id >= MSG_DAT_WINDOW) {
        return 0u; /* deviation: the corpus strides blindly past the block */
    }
    return s_msg_catalog[id];
}

// FUNCTION: LEGORR 0x00419680
void info_message_secondary_sound_set(int id, uint32_t value)
{
    if (id >= 0 && id < MSG_DAT_WINDOW) {
        s_msg_secondary[id] = value;
    }
}

// FUNCTION: LEGORR 0x0041a2f0
uint32_t incident_message_state_get(void)
{
    return s_incident_state;
}

/* DeferredCallbackSet (FUN_00453900) — zero-init module static (the
 * corpus's DAT_004a673c presence word). */
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

/* IncidentMessageFlagSet (FUN_00419ae0) — s_flags bit 4. */
// FUNCTION: LEGORR 0x00419ae0
void incident_message_flag_set(int value)
{
    if (value != 0) {
        s_flags |= 4u;
    } else {
        s_flags &= ~4u;
    }
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

// FUNCTION: LEGORR 0x00419780
void* indexed_linked_list_entry_get(int list, int index)
{
    if (list < 0 || list >= IDX_LIST_SLOTS) {
        return NULL; /* deviation: blind stride */
    }
    void* entry = s_idx_head[list];
    if (index < 0 || s_idx_count[list] <= index) {
        return NULL;
    }
    while (index > 0 && entry != NULL) {
        void* next = NULL; /* deviation: the corpus strides blindly */
        memcpy(&next, (const char*)entry + 0x10, sizeof next);
        entry = next;
        --index;
    }
    return entry;
}

/* IncidentMessageQueueAppend (FUN_004196f0): append to the selected
 * indexed list and discard its oldest node when the walk reaches the
 * corpus's ninth-node threshold. The list/node storage is module-owned
 * because the original DAT-backed table has no port-side owner. */
// FUNCTION: LEGORR 0x004196F0
void incident_message_queue_append(int list, void* entry)
{
    void* tail;
    void* next;
    int walked;
    if (list < 0 || list >= IDX_LIST_SLOTS || entry == NULL) {
        return; /* deviation: guard the corpus's blind table/node access */
    }
    tail = s_idx_head[list];
    if (tail == NULL) {
        s_idx_head[list] = entry;
        next = NULL;
        memcpy((char*)entry + 0x10, &next, sizeof next);
        ++s_idx_count[list];
        return;
    }
    walked = 1;
    memcpy(&next, (char*)tail + 0x10, sizeof next);
    while (next != NULL) {
        ++walked;
        if (walked == 9) {
            (void)incident_message_queue_remove_at(list, 0);
        }
        tail = next;
        memcpy(&next, (char*)tail + 0x10, sizeof next);
    }
    memcpy((char*)tail + 0x10, &entry, sizeof entry);
    next = NULL;
    memcpy((char*)entry + 0x10, &next, sizeof next);
    ++s_idx_count[list];
}

/* MessageSubscriberDispatch (FUN_004196a0): visit subscribers in link order
 * and stop at the first callback that reports a match. */
// FUNCTION: LEGORR 0x004196a0
int message_subscriber_dispatch(int list, MessageSubscriberCallback callback,
                                void* context)
{
    void* entry;
    if (list < 0 || list >= IDX_LIST_SLOTS || callback == NULL) {
        return 0;
    }
    entry = s_idx_head[list];
    while (entry != NULL) {
        void* next = NULL;
        if (callback(entry, context) != 0) {
            return 1;
        }
        __builtin_memcpy(&next, (const char*)entry + 0x10, sizeof next);
        entry = next;
    }
    return 0;
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

/* IncidentMessagePanelRefreshIfActive (FUN_00419da0): the guard is exact;
 * the final viewport/layout operation is represented by a module hook because
 * its 0x42de40 owner has no port implementation yet. */
// FUNCTION: LEGORR 0x00419DA0
void incident_message_panel_refresh_if_active(void)
{
    uint32_t first = 0u;
    uint32_t second = 0u;
    void* queue = s_idx_head[0];
    if (MESSAGE_MAX_ID == 0 || s_idx_count[0] == 0) {
        return;
    }
    if (queue != NULL) {
        memcpy(&first, queue, sizeof(first));
        memcpy(&second, (const char*)queue + 4, sizeof(second));
    }
    if (s_idx_aux[0] != NULL || (first > 0u && second > 0u)) {
        if (s_panel_refresh_hook != NULL) {
            s_panel_refresh_hook(s_idx_aux[0], queue, 0,
                                 s_panel_refresh_context);
        }
    }
}

/* IncidentMessageQueueRemoveAt (FUN_004197b0): unlink the indexed node and
 * return it to the caller; node links occupy the record's +0x10 slot. */
// FUNCTION: LEGORR 0x004197b0
void* incident_message_queue_remove_at(int list, int index)
{
    void* entry;
    void* previous;
    void* next = NULL;
    if (list < 0 || list >= IDX_LIST_SLOTS) {
        return NULL;
    }
    entry = indexed_linked_list_entry_get(list, index);
    if (entry == NULL) {
        return NULL;
    }
    previous = indexed_linked_list_entry_get(list, index - 1);
    memcpy(&next, (const char*)entry + 0x10, sizeof next);
    if (index == 0) {
        s_idx_head[list] = next;
    }
    else if (previous != NULL) {
        memcpy((char*)previous + 0x10, &next, sizeof next);
    }
    --s_idx_count[list];
    return entry;
}
