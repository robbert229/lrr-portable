// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-019 Info.c: the unit's functions in original address order and the
 * state they own, relocated verbatim by tools/relocate_unit.py (ADR 0006).
 * Body hashes: docs/coverage/relocations/GAME-019.csv. */
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
#include <stdarg.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include "frontend.hpp"
#include "frontend.h"
#include "objective.hpp"
#include "level.hpp"
#include "entity.hpp"
#include "terrain_feedback.hpp"
#include "port_loop_ports.hpp"
#include "effects.hpp"
#include "gods98/audio.hpp"
#include "gods98/simtick.hpp"
#include "gods98/flic.hpp"
#include "gods98/media.hpp"
#include "gods98/surface.hpp"
#include "gods98/block.hpp"
#include "util.hpp"
#include "select.hpp"
#include "cmd.hpp"
#include "priority_config_ports.hpp"
#include "tutorial_hud_ports.hpp"
#include "ui_action_dispatch.hpp"
#include "simulation_ui_ports.hpp"
#include "camera_drag.hpp"
#include "screen_pick.hpp"
#include "pointer_cursor.hpp"
#include "ui_list.hpp"
#include "gods98/view.hpp"
#include "gods98/resource.hpp"
#include "frontend_for_info.hpp"
#include <errno.h>
#include <stdint.h>
#include <locale.h>
#include "util_for_info.hpp"
#include "gods98/message_for_info.hpp"
#include "frontend_for_info.hpp"
#include "util_for_info.hpp"

#define MESSAGE_CD_RESET 25.0f /* 0x41c80000 */

typedef struct MessagePreCheckContext {
    uint32_t value;
    const uint32_t* pair;
} MessagePreCheckContext;
/* The corpus's decay gate compares each countdown against this .rdata zero
 * dword (objdump-verified fcomp operand of InfoMessageTimerDecay); a named
 * const static keeps the fcomp operand resolvable by Reccmp. */
// GLOBAL: LEGORR 0x0049e264
static const float s_cooldown_zero = 0.0f;   /* DAT_0049e264, verified */

// GLOBAL: LEGORR 0x004dc658
const char* s_catalog[MESSAGE_MAX_ID];

// GLOBAL: LEGORR 0x004dc6fc
IncidentMessageCatalogRecord s_msg_catalog[MSG_DAT_WINDOW];
/* The corpus's 40 stride-0x14 incident-message records (MessageConfig);
 * the countdown at +0x00 is what the corpus walk reads and resets. */
// GLOBAL: LEGORR 0x004dc704
MessageConfig s_cfg[MESSAGE_MAX_ID];

// GLOBAL: LEGORR 0x004dca20
MessageQueueRecord s_queue_records[MESSAGE_MAX_ID];
// GLOBAL: LEGORR 0x004dcd38
int           s_queue_count = 0;

// GLOBAL: LEGORR 0x004dcd40
TextureSurface* s_interface_message_tab_info_text;

/* ScreenRightEdgeCacheRefresh (FUN_0041a290) — see frontend.hpp. */
// GLOBAL: LEGORR 0x004dcd44
uint32_t s_screen_right_edge;

// GLOBAL: LEGORR 0x004dcd48
uint32_t      s_flags = 0u;          /* DAT_004dcd48 */

static uint32_t s_pending_record_flags;

namespace unported_ledger_stubs {
void ui_text_output_position_and_draw(void* record, uint32_t start_line,
                                      float line_scale);
} // namespace unported_ledger_stubs

extern void scroll_offset_from_cursor_update(void);
extern void* panel_ui_record_peek(void);
extern uint32_t screen_right_edge_cache_peek(void);

/* Defined below (forward declarations added by tools/relocate_unit.py). */
static void message_node_untrack(void* node);
static int message_pre_check_node_match(void* entry, void* context);
static void message_node_track(void* node);
static void reset_family_cooldown(void);
static int s_in_family(int id);
static int incident_message_text_append_default( void* output, const char* format, const uint8_t arguments[5], unsigned int argument_count);

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

/* --- the incident/message DAT-global band (see message.hpp) -----------------
 * The globals at DAT_004dc6fc and DAT_004dc700 are the first two dwords of
 * 0x14-byte records. Keep that stride in the portable storage. */
// FUNCTION: LEGORR 0x00419420
uint32_t incident_message_text_catalog_get(int id)
{
    return s_msg_catalog[id].resource;
}

/* InterfaceMessageTabInfoTextLoad (FUN_00419430): load the scrolling
 * message-tab panel surface and apply the shared resource-record setup. */
// FUNCTION: LEGORR 0x00419430
void interface_message_tab_info_text_load(const char* path)
{
    s_interface_message_tab_info_text = image_surface_load(path, 0u, 0u);
    resource_record_add_ref(s_interface_message_tab_info_text);
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

// FUNCTION: LEGORR 0x00419680
void info_message_secondary_sound_set(int id, uint32_t value)
{
    if (id >= 0 && id < MSG_DAT_WINDOW) {
        s_msg_catalog[id].secondary_sound = value;
    }
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

// FUNCTION: LEGORR 0x00419830
void entity_linked_list_entries_remove_by_key(uintptr_t key)
{
    EntityLinkList** list_link;
    if (s_entity_link_lists == NULL) return;
    list_link = s_entity_link_lists;
    while (*list_link != NULL) {
        EntityLinkList* list = *list_link;
        EntityLinkNode** node_link = &list->head;
        while (*node_link != NULL) {
            EntityLinkNode* node = *node_link;
            if (node->key == key) {
                /* FUN_004197b0 returns the removed entry, but 0x419830
                 * discards that pointer; this path does not release it. */
                *node_link = node->next;
                --list->count;
            }
            else {
                node_link = &node->next;
            }
        }
        if (list->count == 0) {
            /* FUN_00419e80 compacts the empty indexed registry record; it
             * does not free the host mirror record itself. */
            *list_link = list->next;
        }
        else {
            list_link = &list->next;
        }
    }
}

// FUNCTION: LEGORR 0x004198f0
void terrain_cell_linked_entries_remove(uint32_t key)
{
    EntityLinkList** list_link;
    if (s_entity_link_lists == NULL) return;
    list_link = s_entity_link_lists;
    while (*list_link != NULL) {
        EntityLinkList* list = *list_link;
        if (list->kind != 6 && list->kind != 7) {
            list_link = &list->next;
            continue;
        }
        EntityLinkNode** node_link = &list->head;
        while (*node_link != NULL) {
            EntityLinkNode* node = *node_link;
            if (node->key == key) {
                *node_link = node->next;
                --list->count;
                if (s_entity_link_node_release_hook != NULL)
                    s_entity_link_node_release_hook(node);
            }
            else {
                node_link = &node->next;
            }
        }
        if (list->count == 0) {
            *list_link = list->next;
            if (s_entity_link_list_release_hook != NULL)
                s_entity_link_list_release_hook(list);
        }
        else {
            list_link = &list->next;
        }
    }
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

// FUNCTION: LEGORR 0x00419de0
void pending_record_list_entry_move_to_front(unsigned int index)
{
    uint32_t row[5];
    unsigned int i;
    unsigned int j;
    if (index < s_pending_record_count) {
        if (index != 0) {
            for (j = 0; j < 5; ++j) row[j] = s_pending_record_rows[index][j];
            for (i = index; i > 0; --i) {
                for (j = 0; j < 5; ++j)
                    s_pending_record_rows[i][j] = s_pending_record_rows[i - 1][j];
            }
            for (j = 0; j < 5; ++j) s_pending_record_rows[0][j] = row[j];
        }
        s_pending_record_flags |= 3u;
    }
}

/* PendingRecordReleaseIfPresent (FUN_00419e60): release the head row only
 * when the table is nonempty. */
// FUNCTION: LEGORR 0x00419E60
void pending_record_release_if_present(void)
{
    if (s_pending_record_count != 0u) {
        pending_record_list_entry_remove_and_release(0u);
    }
}
// FUNCTION: LEGORR 0x00419e80
void pending_record_list_entry_remove_and_release(unsigned int index)
{
    unsigned int i;
    unsigned int j;
    if (index >= s_pending_record_count) return;
    if (s_pending_record_release_hook != NULL)
        s_pending_record_release_hook(s_pending_record_rows[index]);
    if (s_pending_record_rows[index][1] == 0) {
        for (i = index; i + 1 < s_pending_record_count; ++i) {
            for (j = 0; j < 5; ++j)
                s_pending_record_rows[i][j] = s_pending_record_rows[i + 1][j];
        }
        --s_pending_record_count;
        for (j = 0; j < 5; ++j)
            s_pending_record_rows[s_pending_record_count][j] = 0;
    }
    if (index == 0) s_pending_record_flags &= ~1u;
}

/* PendingRecordReleaseAll (FUN_00419f10): repeatedly release the head until
 * the table empties. The guard prevents a malformed retained row from
 * trapping the portable loop when its release hook does not clear +4. */
// FUNCTION: LEGORR 0x00419F10
void pending_record_release_all(void)
{
    unsigned int attempts = 0u;
    while (s_pending_record_count != 0u &&
           attempts < PENDING_RECORD_CAPACITY) {
        const unsigned int before = s_pending_record_count;
        pending_record_list_entry_remove_and_release(0u);
        ++attempts;
        if (s_pending_record_count == before) {
            break;
        }
    }
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

namespace unported_ledger_stubs {
// FUNCTION: LEGORR 0x0041a1a0
int ui_entry_hit_test_and_dispatch(
    int x, int y, int click)
{
    UiEntryHitTestState* state = s_ui_entry_hit_test_state;
    uint32_t index, entry_count;
    if (state == NULL || state->entries == NULL || state->count == 0u ||
        state->object_bounds_get == NULL) return 0;
    entry_count = state->count;
    if (entry_count > UI_HIT_ENTRY_CAPACITY)
        entry_count = UI_HIT_ENTRY_CAPACITY;

    for (index = 0u; index < entry_count; ++index) {
        UiEntryHitRecord* entry = &state->entries[index];
        int width = 0;
        int height = 0;
        int top = crt_float_to_long(entry->vertical_offset);
        if (!state->object_bounds_get(state->context, entry->fields[2],
                                      &width, &height)) continue;
        if (x < 0 || x > width || y < top || y > top + height) continue;
        if (click != 0) {
            if ((state->flags & 1u) != 0u && index == 0u) {
                panel_state_flags_toggle(7);
                state->flags &= ~1u;
                return 1;
            }
            if (index != 0u) {
                UiEntryHitRecord selected = *entry;
                uint32_t move;
                for (move = index; move != 0u; --move)
                    state->entries[move] = state->entries[move - 1u];
                state->entries[0] = selected;
            }
            /* RecordListEntryMoveToFront sets both state bits even when the
             * clicked row is already at index zero. */
            state->flags |= 3u;
        }
        return 1;
    }
    return 0;
}
} // namespace unported_ledger_stubs

// FUNCTION: LEGORR 0x0041a250
void scroll_offset_from_cursor_update(void)
{
    uint32_t lo = 0, hi = 0;
    /* the corpus pushes the DAT_005007d8 record value itself, whose +0
     * field the forward reads; the unowned-global deviation keeps the
     * slot NULL-defaulted and the callee's guards answer zeros */
    ui_record_callback_forward((const void* const*)s_panel_ui_record,
                               &lo, &hi);
    int diff = (int)(lo - hi);
    if (diff < 0) {
        diff = 0;
    }
    indexed_text_metrics_pair_set(0, (uint32_t)diff, 0u);
    screen_right_edge_cache_refresh();
}
// FUNCTION: LEGORR 0x41a290
void screen_right_edge_cache_refresh(void)
{
    uint32_t x = 0, y = 0;
    indexed_text_metrics_pair_get(0, &x, &y);
    s_screen_right_edge = x - y;
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

// FUNCTION: LEGORR 0x0041a2f0
uint32_t incident_message_state_get(void)
{
    return s_incident_state;
}
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
