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

/* UI-owned operations used by IncidentMessageTextConsumer. The text output
 * record layout and implementations live in game/frontend.cpp; forward
 * declarations avoid pulling its broad header into the engine message module. */
namespace unported_ledger_stubs {
void ui_text_output_position_and_draw(void* record, uint32_t start_line,
                                      float line_scale);
}
extern void scroll_offset_from_cursor_update(void);
extern void* panel_ui_record_peek(void);
extern uint32_t screen_right_edge_cache_peek(void);

#define MESSAGE_CD_RESET 25.0f /* 0x41c80000 */
#define IDX_LIST_SLOTS MESSAGE_MAX_ID

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

/* The corpus's decay gate compares each countdown against this .rdata zero
 * dword (objdump-verified fcomp operand of InfoMessageTimerDecay); a named
 * const static keeps the fcomp operand resolvable by Reccmp. */
// GLOBAL: LEGORR 0x0049e264
static const float s_cooldown_zero = 0.0f;   /* DAT_0049e264, verified */

/* The corpus's 40 stride-0x14 incident-message records (MessageConfig);
 * the countdown at +0x00 is what the corpus walk reads and resets. */
// GLOBAL: LEGORR 0x004dc704
static MessageConfig s_cfg[MESSAGE_MAX_ID];
static char* s_info_message_sound_names[MESSAGE_MAX_ID];
/* The corpus keeps each queued message as one 0x14-byte record at
 * DAT_004dca20: ID at +0, two timer floats at +4/+8, and two opaque dwords.
 * Own storage, zero-init like the corpus's zero-filled tail. */
struct MessageQueueRecord {
    int32_t id;
    float transition_value; /* retail +4; initialized from -int(resource+4) */
    float remaining_value;  /* retail +8; initialized by 0x419b50 */
    unsigned char pad[8];
};
// GLOBAL: LEGORR 0x004dca20
static MessageQueueRecord s_queue_records[MESSAGE_MAX_ID];
// GLOBAL: LEGORR 0x004dcd38
static int           s_queue_count = 0;
// GLOBAL: LEGORR 0x004dcd48
static uint32_t      s_flags = 0u;          /* DAT_004dcd48 */
// GLOBAL: LEGORR 0x004ff980
static uint32_t      s_system_state = 0u;   /* DAT_004ff980 */
// GLOBAL: LEGORR 0x004ff9bc
uint32_t             s_objective_switch = 0u; /* DAT_004ff9bc */
static float         s_now = 0.0f;          /* _DAT_0049e264 analog */
static MessageCallback  s_callback = NULL;
static MessageSoundHook s_sound = NULL;
static char* s_compiled_template[MESSAGE_MAX_ID];
static TextureSurface* s_info_message_surfaces[MESSAGE_MAX_ID];
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
#define MESSAGE_NODE_LIMIT (MESSAGE_MAX_ID * 9)
static void* s_message_nodes[MESSAGE_NODE_LIMIT];
static int s_message_node_count;
static MessagePanelRefreshHook s_panel_refresh_hook;
static void* s_panel_refresh_context;
#define CATALOG_SLOT_CAPACITY 256
static CatalogSlotPredicateHook s_catalog_slot_eligible;
static CatalogSlotPredicateHook s_catalog_slot_available;
static CatalogSlotRankHook s_catalog_slot_rank;
static void* s_catalog_slot_context;
static InfoMessageQueueTickOps s_info_message_queue_tick_ops;
static IncidentMessageTextConsumerOps s_incident_message_text_ops;

static void message_node_track(void* node)
{
    if (node != NULL && s_message_node_count < MESSAGE_NODE_LIMIT) {
        s_message_nodes[s_message_node_count++] = node;
    }
}

static void message_node_untrack(void* node)
{
    int i;
    for (i = 0; i < s_message_node_count; ++i) {
        if (s_message_nodes[i] == node) {
            --s_message_node_count;
            s_message_nodes[i] = s_message_nodes[s_message_node_count];
            s_message_nodes[s_message_node_count] = NULL;
            return;
        }
    }
}

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
    global_state_low_word_set(flags);
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
        (global_state_low_word_get() & 0x80u) != 0u) {
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
#define MSG_DAT_WINDOW 0x28
typedef struct IncidentMessageCatalogRecord {
    uint32_t resource;
    uint32_t secondary_sound;
    uint32_t reserved[3];
} IncidentMessageCatalogRecord;
// GLOBAL: LEGORR 0x004dc6fc
static IncidentMessageCatalogRecord s_msg_catalog[MSG_DAT_WINDOW];
// GLOBAL: LEGORR 0x004dcd58
static uint32_t s_incident_state;

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

/* InfoMessageSoundLoad (FUN_00419650) is named by the corpus, but its body
 * calls BitmapSurfaceLoad (0x47d9d0) and ResourceRecordAddRef (0x47dbc0).
 * The returned image resource is also what RemainingTimerDurationGet reads. */
// FUNCTION: LEGORR 0x00419650
void info_message_surface_load(int id, const char* path)
{
    if (id < 0 || id >= MESSAGE_MAX_ID) {
        return; /* deviation: corpus indexes the resource table blindly */
    }
    if (s_info_message_surfaces[id] != NULL) {
        texture_release(s_info_message_surfaces[id]);
        s_info_message_surfaces[id] = NULL;
    }
    s_msg_catalog[id].resource = 0u;
    if (path != NULL && path[0] != '\0') {
        s_info_message_surfaces[id] = image_surface_load(path, 0u, 0u);
        if (s_info_message_surfaces[id] != NULL) {
            resource_record_add_ref(s_info_message_surfaces[id]);
            s_msg_catalog[id].resource = (uint32_t)(uintptr_t)
                s_info_message_surfaces[id];
        }
    }
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

/* RemainingTimerDurationGet (FUN_00419b50): the retail code reads the
 * signed dword at +8 of each configured BitmapSurface resource record and
 * converts that integer directly to float. BitmapSurfaceLoad's +8 field is
 * its DirectDraw surface pointer, so this deliberately preserves the odd
 * pointer-to-number operation; it is not WAV duration or elapsed time. The
 * portable texture owner has no DirectDraw record, so its TextureSurface
 * address is the corresponding opaque resource word. */
// FUNCTION: LEGORR 0x00419B50
float remaining_timer_duration_get(void)
{
    float remaining = 333.0f;
    for (int i = 0; i < s_queue_count; ++i) {
        int id = (int)s_queue_records[i].id;
        if (id >= 0 && id < MESSAGE_MAX_ID
            && s_info_message_surfaces[id] != NULL) {
            uint32_t resource_word =
                (uint32_t)(uintptr_t)s_info_message_surfaces[id];
            int32_t resource_value;
            memcpy(&resource_value, &resource_word, sizeof resource_value);
            remaining -= (float)resource_value;
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

typedef struct MessagePreCheckContext {
    uint32_t value;
    const uint32_t* pair;
} MessagePreCheckContext;

/* LAB_004199a0 compares the queued node against the dispatch payload. */
static int message_pre_check_node_match(void* entry, void* context)
{
    const uint32_t* node = (const uint32_t*)entry;
    const MessagePreCheckContext* request =
        (const MessagePreCheckContext*)context;
    if (request->value == 0u && request->pair == NULL) return 1;
    if (request->value != 0u && node[2] == request->value) return 1;
    if (request->pair != NULL && node[0] == request->pair[0] &&
        node[1] == request->pair[1]) return 1;
    return 0;
}

/* Dedupe-with-subscriber-scan (corpus MessagePreCheck FUN_004199f0). */
// FUNCTION: LEGORR 0x004199f0
static int message_pre_check(int id, uint32_t value, const void* pair)
{
    MessagePreCheckContext request;
    uint32_t index;
    if (id == 10 || id == 11 || id == 12 || id == 38) {
        return 0; /* exception list skips dedupe */
    }
    request.pair = (const uint32_t*)pair;
    request.value = value;
    for (index = 0; index < s_queue_count; ++index) {
        if (id == s_queue_records[index].id &&
            message_subscriber_dispatch(index, message_pre_check_node_match,
                                        &request) != 0) {
            return 1;
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
    /* Load the table base before the port-side null guards, matching the
     * corpus entry sequence while keeping the tested guarded API. */
    const char* const* entry = s_catalog;
    if (name == NULL || out_id == NULL) {
        return 0; /* pinned deviation: the corpus strides both blindly */
    }
    /* stride-4 name-table walk, as the corpus: unsigned end compare (the
     * loop-back is jb) with the index counter beside the pointer (edi) */
    int i = 0;
    const char* const* end = s_catalog + MESSAGE_MAX_ID;
    for (; entry < end; ++entry, ++i) {
        if (strcasecmp_lite(*entry, name) == 0) {
            *out_id = i;
            return 1;
        }
    }
    return 0;
}

/* InfoMessageQueueFindId (FUN_00419b20): compare the ID dword at +0 of each
 * active queue record, in insertion order, advancing by the 0x14-byte
 * record stride. Return the first matching index, or -1. */
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
void message_dispatch(int id, const char* p2, uint32_t p3, const void* p4)
{
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

    if (message_pre_check(id, p3, p4)) {
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
            /* Retail stores -(float)*(int*)(resource + 4) at queue +4.
             * BitmapSurfaceLoad is called with a zero second argument on
             * this path, so this host resource's corresponding word is 0. */
            s_queue_records[idx].transition_value = 0.0f;
            s_queue_count += 1;
            s_queue_records[idx].remaining_value =
                remaining_timer_duration_get();
        }
        s_queue_records[idx].id = id;
        {
            void* node = incident_message_node_create(
                (const uint32_t*)p4, p3, p2);
            if (node != NULL) {
                message_node_track(node);
                incident_message_queue_append(idx, node);
            }
        }
        if (s_callback != NULL) {
            s_callback(id);
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

/* InfoMessageQueueTick (FUN_00419f30): each per-ID record advances its
 * display offset toward zero at 3 units/second and its remaining value
 * toward the 333-unit ceiling at 8 units/second. Resource positioning and
 * the state-owned text draw are renderer operations, so they remain narrow
 * callbacks while the queue-owned timing runs in the normal message ticker. */
// FUNCTION: LEGORR 0x00419f30
void info_message_queue_tick(float dt)
{
    float ceiling = 333.0f;
    int index;
    float position[2];
    void* global_resource;
    if (s_queue_count == 0) return;
    for (index = 0; index < s_queue_count; ++index) {
        int id = (int)s_queue_records[index].id;
        MessageQueueRecord* record = &s_queue_records[index];
        TextureSurface* resource = NULL;
        if (record->remaining_value < ceiling)
            record->remaining_value += dt * 8.0f;
        if (ceiling < record->remaining_value)
            record->remaining_value = ceiling;
        if (record->transition_value < 0.0f)
            record->transition_value += dt * 3.0f;
        if (record->transition_value > 0.0f)
            record->transition_value = 0.0f;

        position[0] = record->transition_value;
        position[1] = record->remaining_value;
        if (id >= 0 && id < MESSAGE_MAX_ID)
            resource = s_info_message_surfaces[id];
        if (s_info_message_queue_tick_ops.resource_update != NULL) {
            s_info_message_queue_tick_ops.resource_update(
                s_info_message_queue_tick_ops.context, resource, position);
        }
        if (record->remaining_value < 0.0f) {
            global_resource =
                s_info_message_queue_tick_ops.global_resource_get != NULL
                ? s_info_message_queue_tick_ops.global_resource_get(
                    s_info_message_queue_tick_ops.context)
                : NULL;
            position[1] = 0.0f;
            if (s_info_message_queue_tick_ops.resource_update != NULL) {
                s_info_message_queue_tick_ops.resource_update(
                    s_info_message_queue_tick_ops.context, global_resource,
                    position);
            }
            return;
        }
        if (s_info_message_queue_tick_ops.state_draw != NULL) {
            s_info_message_queue_tick_ops.state_draw(
                s_info_message_queue_tick_ops.context, s_incident_state);
        }
        if (s_info_message_queue_tick_ops.resource_limit_get != NULL) {
            ceiling = record->remaining_value - (float)
                s_info_message_queue_tick_ops.resource_limit_get(
                    s_info_message_queue_tick_ops.context, resource);
        }
        else ceiling = record->remaining_value;
    }
}

void incident_message_text_consumer_ops_set(
    const IncidentMessageTextConsumerOps* ops)
{
    if (ops != NULL) s_incident_message_text_ops = *ops;
    else memset(&s_incident_message_text_ops, 0,
                sizeof(s_incident_message_text_ops));
}

static int incident_message_text_append_default(
    void* output, const char* format, const uint8_t arguments[5],
    unsigned int argument_count)
{
#if defined(_M_IX86) || defined(__i386__)
    void* text_buffer;
    if (output == NULL || format == NULL) return 0;
    text_buffer = *(void**)output;
    if (text_buffer == NULL) return 0;
    if (argument_count == 0u) {
        text_buffer_append_format(text_buffer, format);
    }
    else if (argument_count == 5u) {
        text_buffer_append_format(text_buffer, format,
            (int)arguments[0], (int)arguments[1], (int)arguments[2],
            (int)arguments[3], (int)arguments[4]);
    }
    else return 0;
    return 1;
#else
    (void)output;
    (void)format;
    (void)arguments;
    (void)argument_count;
    return 0; /* the host-width output record resolves its buffer by sidecar */
#endif
}

/* IncidentMessageTextConsumer (FUN_0041a080): check the panel transition
 * gates, replace the text from the leading message's first node (falling
 * back to its configured template), update scrolling, clear the pending-text
 * bit, then draw once more on every path. The UI record and its buffer live
 * in the panel owner; their operations stay behind the narrow adapter above. */
// FUNCTION: LEGORR 0x0041a080
void incident_message_text_consumer(float frame_delta)
{
    uint32_t ui_flags = request_record_flags_peek(7);
    uint32_t scroll_offset = 0u;
    void* output = NULL;
    if (s_incident_message_text_ops.ui_state_flags_get != NULL) {
        ui_flags = s_incident_message_text_ops.ui_state_flags_get(
            s_incident_message_text_ops.context);
    }
    if (s_incident_message_text_ops.text_output_get != NULL) {
        output = s_incident_message_text_ops.text_output_get(
            s_incident_message_text_ops.context);
    }
    else {
        output = panel_ui_record_peek();
    }

    if ((s_flags & 1u) == 0u || s_queue_count == 0) {
        if ((ui_flags & 4u) == 0u &&
            s_incident_message_text_ops.panel_transition_request != NULL) {
            s_incident_message_text_ops.panel_transition_request(
                s_incident_message_text_ops.context, 7);
        }
        else if ((ui_flags & 4u) == 0u) {
            panel_state_flags_toggle(7);
        }
    }
    else {
        if ((ui_flags & 2u) == 0u &&
            s_incident_message_text_ops.panel_transition_request != NULL) {
            s_incident_message_text_ops.panel_transition_request(
                s_incident_message_text_ops.context, 7);
        }
        else if ((ui_flags & 2u) == 0u) {
            panel_state_flags_toggle(7);
        }
        if ((s_flags & 2u) != 0u && output != NULL) {
            int id = (int)s_queue_records[0].id;
            const char* text = NULL;
            uint8_t arguments[5];
            unsigned int argument_count = 0u;
            size_t length;
            int appended = 0;
            void* node = s_idx_head[0];
            if (node != NULL) text = incident_message_node_text_get(node);
            if (text == NULL && id >= 0 && id < MESSAGE_MAX_ID)
                text = s_compiled_template[id];

            if (s_incident_message_text_ops.text_output_reset != NULL)
                s_incident_message_text_ops.text_output_reset(
                    s_incident_message_text_ops.context, output);
            else
                ui_text_output_object_release((void* const*)output);
            if (text != NULL) {
                memset(arguments, 0, sizeof(arguments));
                length = strlen(text);
                if (text[length + 1u] != '\0') {
                    unsigned int index;
                    argument_count = 5u;
                    for (index = 0u; index < argument_count; ++index)
                        arguments[index] = (uint8_t)text[length + 2u + index];
                }
                if (s_incident_message_text_ops.text_output_append != NULL) {
                    s_incident_message_text_ops.text_output_append(
                        s_incident_message_text_ops.context, output, text,
                        arguments, argument_count);
                    appended = 1;
                }
                else if (incident_message_text_append_default(
                             output, text, arguments, argument_count)) {
                    appended = 1;
                }
            }
            if (s_incident_message_text_ops.scroll_offset_get != NULL) {
                scroll_offset =
                    s_incident_message_text_ops.scroll_offset_get(
                        s_incident_message_text_ops.context);
            }
            else {
                scroll_offset = screen_right_edge_cache_peek();
            }
            unported_ledger_stubs::ui_text_output_position_and_draw(
                output, scroll_offset, frame_delta);
            if (appended) {
                scroll_offset_from_cursor_update();
                s_flags &= ~2u;
            }
        }
    }

    if (s_incident_message_text_ops.scroll_offset_get != NULL) {
        scroll_offset = s_incident_message_text_ops.scroll_offset_get(
            s_incident_message_text_ops.context);
    }
    else {
        scroll_offset = screen_right_edge_cache_peek();
    }
    unported_ledger_stubs::ui_text_output_position_and_draw(
        output, scroll_offset, frame_delta);
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
 * The globals at DAT_004dc6fc and DAT_004dc700 are the first two dwords of
 * 0x14-byte records. Keep that stride in the portable storage. */
// FUNCTION: LEGORR 0x00419420
uint32_t incident_message_text_catalog_get(int id)
{
    return s_msg_catalog[id].resource;
}

// FUNCTION: LEGORR 0x00419680
void info_message_secondary_sound_set(int id, uint32_t value)
{
    if (id >= 0 && id < MSG_DAT_WINDOW) {
        s_msg_catalog[id].secondary_sound = value;
    }
}

// FUNCTION: LEGORR 0x0041a2f0
uint32_t incident_message_state_get(void)
{
    return s_incident_state;
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
    message_node_untrack(entry);
    return entry;
}
