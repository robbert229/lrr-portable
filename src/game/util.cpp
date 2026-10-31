// SPDX-License-Identifier: LGPL-3.0-or-later
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#include "gods98/resource.hpp"
#include "util.hpp"

static ParsedEntityHealthValue s_parsed_entity_health_value;

/* FUNCTION: LEGORR 0x004774B0 */
int whitespace_tokenize(char* text, char** fields)
{
    unsigned char* cursor;
    int count;
    char** output;
    if (text == NULL || fields == NULL || text[0] == '\0') {
        return 0;
    }

    cursor = (unsigned char*)text;
    fields[0] = text;
    output = fields + 1;
    count = 1;
    while (*cursor != 0) {
        if (isspace((int)*cursor) == 0) {
            ++cursor;
            continue;
        }

        *cursor = 0;
        ++cursor;
        while (isspace((int)*cursor) != 0) {
            ++cursor;
        }
        *output = (char*)cursor;
        ++output;
        ++count;
    }
    return count;
}

/* FUNCTION: LEGORR 0x00477670 */
int game_string_hash_compute(const unsigned char* text,
                             int skip_whitespace,
                             int uppercase)
{
    uint32_t hash;
    uint32_t factor;
    uint32_t position;
    const unsigned char* cursor;
    if (text == NULL) {
        return 0;
    }
    hash = 0;
    factor = 0x00600937u;
    position = 1;
    cursor = text;
    while (*cursor != 0) {
        unsigned char value;
        uint32_t term;
        if (skip_whitespace != 0) {
            while (isspace((int)*cursor) != 0) {
                ++cursor;
            }
        }
        value = *cursor;
        ++cursor;
        if (uppercase != 0) {
            value = (unsigned char)toupper((int)value);
        }
        term = position * factor;
        hash += term * (uint32_t)value;
        factor *= 0x00600937u;
        ++position;
    }
    return (int)hash;
}

/* FUNCTION: LEGORR 0x00481AC0 */
int* string_trailing_digits_split(const char* text, char* prefix,
                                  int* value, int* digit_count)
{
    size_t length;
    size_t split;
    size_t digits;
    uint32_t number;
    uint32_t place;
    if (text == NULL || prefix == NULL || value == NULL
        || digit_count == NULL) {
        return NULL;
    }
    length = strlen(text);
    if (length < 2u || text[length - 1u] < '0' || text[length - 1u] > '9') {
        return NULL;
    }
    split = length - 1u;
    number = 0u;
    place = 1u;
    digits = 0u;
    while (split != 0u) {
        unsigned char ch = (unsigned char)text[split];
        uint32_t digit;
        if (ch < (unsigned char)'0' || ch > (unsigned char)'9') {
            break;
        }
        digit = (uint32_t)(ch - (unsigned char)'0');
        number += digit * place;
        place *= 10u;
        ++digits;
        --split;
    }
    if (digits == 0u || split == 0u) {
        return NULL;
    }
    strcpy(prefix, text);
    prefix[split + 1u] = '\0';
    *value = (int)number;
    *digit_count = (int)digits;
    return digit_count;
}

/* FUNCTION: LEGORR 0x00481B80 */
void counter_format_string_build(const char* prefix, char* out,
                                 int* counter, int width)
{
    char digits[16];
    int padding;
    size_t prefix_length;
    size_t digit_length;
    if (prefix == NULL || out == NULL || counter == NULL) {
        return;
    }
    strcpy(out, prefix);
    prefix_length = strlen(out);
    sprintf(digits, "%d", *counter);
    digit_length = strlen(digits);
    padding = width - (int)digit_length;
    while (padding > 0) {
        out[prefix_length] = '0';
        ++prefix_length;
        --padding;
    }
    out[prefix_length] = '\0';
    ++*counter;
    strcat(out, digits);
}

// FUNCTION: LEGORR 0x004907f0
const ParsedEntityHealthValue* parse_entity_health_value(
    const char* text, int length, int flags, int mode)
{
    char buffer[260];
    char* end;
    size_t count;
    size_t i;
    int has_real_syntax = 0;
    (void)flags;
    (void)mode;
    memset(&s_parsed_entity_health_value, 0,
           sizeof(s_parsed_entity_health_value));
    if (text == NULL) {
        s_parsed_entity_health_value.flags = 0x200u;
        return &s_parsed_entity_health_value;
    }
    if (length < 0) length = 0;
    count = (length == 0) ? strlen(text) : (size_t)length;
    if (count >= sizeof(buffer)) count = sizeof(buffer) - 1u;
    memcpy(buffer, text, count);
    buffer[count] = '\0';
    s_parsed_entity_health_value.value = strtod(buffer, &end);
    if (end == buffer) {
        s_parsed_entity_health_value.flags = 0x200u;
        return &s_parsed_entity_health_value;
    }
    s_parsed_entity_health_value.consumed = (uint32_t)(end - buffer);
    for (i = 0; i < s_parsed_entity_health_value.consumed; ++i) {
        if (buffer[i] == '.' || buffer[i] == 'e' || buffer[i] == 'E') {
            has_real_syntax = 1;
            break;
        }
    }
    s_parsed_entity_health_value.flags = has_real_syntax ? 0x100u : 0x80u;
    return &s_parsed_entity_health_value;
}

static MotionUtilityState* s_motion_utility_state;
static MotionRandomRangeFn s_motion_random_range;
static void* s_motion_utility_user;
static MotionDriftState* s_motion_drift_state;
static MotionRandomRangeFn s_motion_drift_random_range;
static void* s_motion_drift_user;

void motion_utility_state_install(MotionUtilityState* state,
                                  MotionRandomRangeFn random_range,
                                  void* user)
{
    s_motion_utility_state = state;
    s_motion_random_range = random_range;
    s_motion_utility_user = user;
}

void motion_drift_state_install(MotionDriftState* state,
                                MotionRandomRangeFn random_range,
                                void* user)
{
    s_motion_drift_state = state;
    s_motion_drift_random_range = random_range;
    s_motion_drift_user = user;
}

/* DriftOffsetStepApply (FUN_0044d0d0): periodically choose a three-axis
 * offset, reflect it at the horizontal bound, and accumulate it into the
 * module-owned drift position. */
// FUNCTION: LEGORR 0x0044D0D0
void motion_drift_offset_step(float delta)
{
    MotionDriftState* state = s_motion_drift_state;
    float random_value;
    float step[3];
    float shifted_x;
    int i;

    if (state == NULL || (state->flags & 1u) == 0u ||
        s_motion_drift_random_range == NULL) return;
    state->timer -= delta;
    if (state->timer > 0.0f) return;

    state->timer = s_motion_drift_random_range(state->timer_min,
                                                state->timer_max,
                                                s_motion_drift_user);
    random_value = s_motion_drift_random_range(0.0f, 1.0f,
                                               s_motion_drift_user);
    for (i = 0; i != 3; ++i)
        step[i] = ((random_value * 2.0f) - 1.0f) * state->amplitude[i];
    shifted_x = step[0] + state->center_x;
    if (state->max_abs_x <= (shifted_x < 0.0f ? -shifted_x : shifted_x)) {
        for (i = 0; i != 3; ++i)
            step[i] = step[i] < 0.0f ? step[i] : -step[i];
        shifted_x = step[0] + state->center_x;
        if (state->max_abs_x <=
            (shifted_x < 0.0f ? -shifted_x : shifted_x)) return;
    }
    state->offset[0] += step[0];
    state->offset[1] += step[1];
    state->offset[2] += step[2];
    state->center_x = shifted_x;
}

/* FUNCTION: LEGORR 0x0044da50 */
void motion_three_channel_tween_step(float delta)
{
    MotionUtilityState* state = s_motion_utility_state;
    if (state == NULL) return;
    if ((state->flags & 0x200u) == 0u) {
        if ((state->flags & 0x400u) == 0u) {
            const float step = delta * 0.1f;
            state->channel[0] += step;
            state->channel[1] += step;
            state->channel[2] += step;
            if (state->lower_limit <= state->channel[0]) {
                state->flags |= 0x400u;
                state->channel[0] = state->upper_limit;
            }
        }
    } else {
        if ((state->flags & 0x1000u) != 0u) {
            state->channel[0] = 0.0f;
            state->channel[1] = 0.0f;
            state->channel[2] = 0.0f;
            return;
        }
        const float step = delta * 0.1f;
        state->channel[0] -= step;
        state->channel[1] -= step;
        state->channel[2] -= step;
        if (state->channel[0] <= 0.0f) {
            state->channel[0] = 0.0f;
            state->flags |= 0x1000u;
        }
    }
}

/* FUNCTION: LEGORR 0x0044d710 */
void motion_waypoint_vector_random_scale_apply(void)
{
    MotionUtilityState* state = s_motion_utility_state;
    if (state == NULL || s_motion_random_range == NULL) return;
    const float scale = s_motion_random_range(state->random_min,
                                               state->random_max,
                                               s_motion_utility_user);
    state->scaled_vector[0] = state->source_vector[0] / scale;
    state->scaled_vector[1] = state->source_vector[1] / scale;
    state->scaled_vector[2] = state->source_vector[2] / scale;
}

#define STRING_TABLE_MAX_ENTRIES 256
static const char* s_string_table_entries[STRING_TABLE_MAX_ENTRIES];
// GLOBAL: LEGORR 0x004b8a50
static unsigned int s_string_table_count;
// GLOBAL: LEGORR 0x00557900
static const char* s_bubble_action_strings[BUBBLE_ACTION_STRING_COUNT];

#define PENDING_RECORD_CAPACITY 256
static uint32_t s_pending_record_rows[PENDING_RECORD_CAPACITY][5];
static unsigned int s_pending_record_count;
static uint32_t s_pending_record_flags;
static void (*s_pending_record_release_hook)(uint32_t* row);

#define LEVEL_REMAP_CAPACITY 128
static uint32_t s_level_remap_pairs[LEVEL_REMAP_CAPACITY][2];
static unsigned int s_level_remap_count;

static EntityLinkList** s_entity_link_lists;
static void (*s_entity_link_node_release_hook)(EntityLinkNode*);
static void (*s_entity_link_list_release_hook)(EntityLinkList*);
static void (*s_resource_entry_release_hook)(void*);
static void (*s_resource_array_release_hook)(void*);

#define STRING_QUEUE_CAPACITY 15
#define STRING_QUEUE_TEXT_CAPACITY 1024
struct StringQueueEntry {
    uint32_t first;
    uint32_t second;
    uint32_t third;
    char text[STRING_QUEUE_TEXT_CAPACITY];
};
static StringQueueEntry s_string_queue[STRING_QUEUE_CAPACITY];
static unsigned int s_string_queue_count;
static StringQueueFlushHook s_string_queue_flush_hook;

void string_table_install(const char* const* entries, unsigned int count)
{
    unsigned int i;
    if (count > STRING_TABLE_MAX_ENTRIES) count = STRING_TABLE_MAX_ENTRIES;
    s_string_table_count = count;
    for (i = 0; i < count; ++i) {
        s_string_table_entries[i] = entries != NULL ? entries[i] : NULL;
    }
    for (; i < STRING_TABLE_MAX_ENTRIES; ++i) {
        s_string_table_entries[i] = NULL;
    }
}

// FUNCTION: LEGORR 0x00408c80
unsigned int string_table_index_lookup(const char* value)
{
    unsigned int i;
    if (s_string_table_count == 0 || value == NULL) return 0;
    for (i = 0; i < s_string_table_count; ++i) {
        if (s_string_table_entries[i] != NULL &&
            strcmp(s_string_table_entries[i], value) == 0) {
            return i;
        }
    }
    return s_string_table_count;
}

// FUNCTION: LEGORR 0x004070b0
void bubble_action_strings_initialize(void)
{
    s_bubble_action_strings[0] = "Bubble_CantDo";
    s_bubble_action_strings[1] = "Bubble_Idle";
    s_bubble_action_strings[2] = "Bubble_CollectCrystal";
    s_bubble_action_strings[3] = "Bubble_CollectOre";
    s_bubble_action_strings[4] = "Bubble_CollectStud";
    s_bubble_action_strings[5] = "Bubble_CollectDynamite";
    s_bubble_action_strings[7] = "Bubble_CollectElecFence";
    s_bubble_action_strings[6] = "Bubble_CollectBarrier";
    s_bubble_action_strings[8] = "Bubble_CollectDrill";
    s_bubble_action_strings[9] = "Bubble_CollectSpade";
    s_bubble_action_strings[10] = "Bubble_CollectHammer";
    s_bubble_action_strings[11] = "Bubble_CollectSpanner";
    s_bubble_action_strings[12] = "Bubble_CollectLaser";
    s_bubble_action_strings[13] = "Bubble_CollectPusher";
    s_bubble_action_strings[14] = "Bubble_CollectFreezer";
    s_bubble_action_strings[15] = "Bubble_CollectBirdScarer";
    s_bubble_action_strings[16] = "Bubble_CarryCrystal";
    s_bubble_action_strings[17] = "Bubble_CarryOre";
    s_bubble_action_strings[18] = "Bubble_CarryStud";
    s_bubble_action_strings[19] = "Bubble_CarryDynamite";
    s_bubble_action_strings[20] = "Bubble_CarryBarrier";
    s_bubble_action_strings[21] = "Bubble_CarryElecFence";
    s_bubble_action_strings[22] = "Bubble_Goto";
    s_bubble_action_strings[23] = "Bubble_Dynamite";
    s_bubble_action_strings[24] = "Bubble_Reinforce";
    s_bubble_action_strings[25] = "Bubble_Drill";
    s_bubble_action_strings[26] = "Bubble_Repair";
    s_bubble_action_strings[27] = "Bubble_Dig";
    s_bubble_action_strings[28] = "Bubble_Flee";
    s_bubble_action_strings[29] = "Bubble_PowerOff";
    s_bubble_action_strings[30] = "Bubble_CallToArms";
    s_bubble_action_strings[31] = "Bubble_ElecFence";
    s_bubble_action_strings[32] = "Bubble_Eat";
    s_bubble_action_strings[33] = "Bubble_Drive";
    s_bubble_action_strings[34] = "Bubble_Upgrade";
    s_bubble_action_strings[35] = "Bubble_BuildPath";
    s_bubble_action_strings[36] = "Bubble_Train";
    s_bubble_action_strings[37] = "Bubble_Recharge";
    s_bubble_action_strings[38] = "Bubble_Request";
}

const char* bubble_action_string_get(unsigned int index)
{
    if (index >= BUBBLE_ACTION_STRING_COUNT) return NULL;
    return s_bubble_action_strings[index];
}

/* FUNCTION: LEGORR 0x00451fd0
 * The reference scans 65 resource records and compares each name at +8
 * case-insensitively. This seam accepts the table explicitly; a miss
 * returns the final comparison result just as the reference does. */
int case_insensitive_name_table_lookup_at_offset8(const void* const* entries,
                                                   unsigned int count,
                                                   const char* name)
{
    unsigned int i;
    int comparison = -1;
    if (entries == NULL || name == NULL) return comparison;
    for (i = 0; i < count; ++i) {
        const unsigned char* left;
        const unsigned char* right;
        const char* entry = (const char*)entries[i];
        if (entry == NULL) {
            comparison = 1;
        } else {
            left = (const unsigned char*)(entry + 8);
            right = (const unsigned char*)name;
            do {
                const int a = tolower(*left++);
                const int b = tolower(*right++);
                comparison = a - b;
            } while (comparison == 0 && left[-1] != 0);
        }
        if (comparison == 0) return (int)i;
    }
    return comparison;
}

void pending_record_table_install(const uint32_t* rows, unsigned int count)
{
    unsigned int i;
    unsigned int j;
    if (count > PENDING_RECORD_CAPACITY) count = PENDING_RECORD_CAPACITY;
    s_pending_record_count = count;
    for (i = 0; i < count; ++i) {
        for (j = 0; j < 5; ++j)
            s_pending_record_rows[i][j] = rows != NULL ? rows[i * 5 + j] : 0;
    }
    for (; i < PENDING_RECORD_CAPACITY; ++i) {
        for (j = 0; j < 5; ++j) s_pending_record_rows[i][j] = 0;
    }
}

void pending_record_release_hook_set(void (*hook)(uint32_t* row))
{
    s_pending_record_release_hook = hook;
}

unsigned int pending_record_count_get(void)
{
    return s_pending_record_count;
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

/* PendingRecordReleaseIfPresent (FUN_00419e60): release the head row only
 * when the table is nonempty. */
// FUNCTION: LEGORR 0x00419E60
void pending_record_release_if_present(void)
{
    if (s_pending_record_count != 0u) {
        pending_record_list_entry_remove_and_release(0u);
    }
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

void level_identifier_remap_table_install(const uint32_t* pairs,
                                          unsigned int count)
{
    unsigned int i;
    if (count > LEVEL_REMAP_CAPACITY) count = LEVEL_REMAP_CAPACITY;
    s_level_remap_count = count;
    for (i = 0; i < count; ++i) {
        s_level_remap_pairs[i][0] = pairs != NULL ? pairs[i * 2] : 0;
        s_level_remap_pairs[i][1] = pairs != NULL ? pairs[i * 2 + 1] : 0;
    }
}

// FUNCTION: LEGORR 0x0045d870
void command_entity_reference_remap(uint32_t* entity_reference)
{
    unsigned int i;
    if (entity_reference == NULL) return;
    for (i = 0; i < s_level_remap_count; ++i) {
        if (*entity_reference == s_level_remap_pairs[i][0]) {
            *entity_reference = s_level_remap_pairs[i][1];
            return;
        }
    }
}

void entity_link_lists_install(EntityLinkList** head)
{
    s_entity_link_lists = head;
}

void entity_link_release_hooks_set(void (*node_hook)(EntityLinkNode*),
                                  void (*list_hook)(EntityLinkList*))
{
    s_entity_link_node_release_hook = node_hook;
    s_entity_link_list_release_hook = list_hook;
}

static void entity_linked_lists_remove_by_key(uint32_t key, int terrain_only)
{
    EntityLinkList** list_link;
    if (s_entity_link_lists == NULL) return;
    list_link = s_entity_link_lists;
    while (*list_link != NULL) {
        EntityLinkList* list = *list_link;
        if (terrain_only && list->kind != 6 && list->kind != 7) {
            list_link = &list->next;
            continue;
        }
        EntityLinkNode** node_link = &list->head;
        while (*node_link != NULL) {
            EntityLinkNode* node = *node_link;
            if (node->key == key) {
                *node_link = node->next;
                if (list->count != 0) --list->count;
                if (s_entity_link_node_release_hook != NULL)
                    s_entity_link_node_release_hook(node);
            }
            else {
                node_link = &node->next;
            }
        }
        if (list->head == NULL || list->count == 0) {
            *list_link = list->next;
            if (s_entity_link_list_release_hook != NULL)
                s_entity_link_list_release_hook(list);
        }
        else {
            list_link = &list->next;
        }
    }
}

// FUNCTION: LEGORR 0x00419830
void entity_linked_list_entries_remove_by_key(uint32_t key)
{
    entity_linked_lists_remove_by_key(key, 0);
}

// FUNCTION: LEGORR 0x004198f0
void terrain_cell_linked_entries_remove(uint32_t key)
{
    entity_linked_lists_remove_by_key(key, 1);
}

void rectangular_resource_table_release_hooks_set(void (*resource_hook)(void*),
                                                   void (*array_hook)(void*))
{
    s_resource_entry_release_hook = resource_hook;
    s_resource_array_release_hook = array_hook;
}

// FUNCTION: LEGORR 0x0042b480
void rectangular_resource_table_destroy(RectangularResourceTable* table)
{
    unsigned long i;
    unsigned long total;
    if (table == NULL || table->resources == NULL) return;
    total = (unsigned long)table->width * (unsigned long)table->height;
    for (i = 0; i < total; ++i) {
        if (s_resource_entry_release_hook != NULL)
            s_resource_entry_release_hook(table->resources[i]);
    }
    if (s_resource_array_release_hook != NULL)
        s_resource_array_release_hook(table->resources);
    table->resources = NULL;
}

void string_queue_flush_hook_set(StringQueueFlushHook hook)
{
    s_string_queue_flush_hook = hook;
}

unsigned int string_queue_count_get(void)
{
    return s_string_queue_count;
}

// FUNCTION: LEGORR 0x00410320
void string_queue_record_or_flush(uint32_t first, uint32_t second,
                                  uint32_t third, const char* text,
                                  int record)
{
    unsigned int i;
    if (!record) {
        if (s_string_queue_flush_hook != NULL) {
            for (i = 0; i < s_string_queue_count; ++i) {
                s_string_queue_flush_hook(s_string_queue[i].first,
                                          s_string_queue[i].second,
                                          s_string_queue[i].third,
                                          s_string_queue[i].text);
            }
        }
        s_string_queue_count = 0;
        return;
    }
    if (s_string_queue_count >= STRING_QUEUE_CAPACITY || text == NULL) return;
    s_string_queue[s_string_queue_count].first = first;
    s_string_queue[s_string_queue_count].second = second;
    s_string_queue[s_string_queue_count].third = third;
    strncpy(s_string_queue[s_string_queue_count].text, text,
            STRING_QUEUE_TEXT_CAPACITY - 1);
    s_string_queue[s_string_queue_count].text[STRING_QUEUE_TEXT_CAPACITY - 1] = '\0';
    ++s_string_queue_count;
}

// FUNCTION: LEGORR 0x00410280
int callback_operation_run_and_return_status(void* stack_owner,
                                             CallbackOperationNode* first,
                                             CallbackOperationVisit visit)
{
    int status = 1;
    CallbackOperationNode* node;
    if (visit == NULL) return status;
    for (node = first; node != NULL; node = node->next)
        visit(stack_owner, node, &status);
    return status;
}

// FUNCTION: LEGORR 0x0040a5e0
void record_count_digits_fill(const RecordCountRenderHooks* hooks, uint32_t value)
{
    unsigned int slot;
    unsigned int digits = 0;
    uint32_t probe = value;
    float red;
    float green;
    if (hooks == NULL) return;
    if (value < 5u) { red = 0.0f; green = 1.0f; }
    else if (value < 10u) { red = 1.0f; green = 1.0f; }
    else { red = 1.0f; green = 0.25f; }
    if (hooks->style_set != NULL) {
        for (slot = 0; slot < 4; ++slot) {
            hooks->style_set(hooks->context, slot, red, green, 0);
            hooks->style_set(hooks->context, slot, red, green, 3);
        }
    }
    if (probe != 0) {
        uint32_t limit = 1;
        while (limit <= probe && limit <= 100000000u) {
            ++digits;
            limit *= 10u;
        }
    }
    if (digits > 4) digits = 4;
    for (slot = 0; slot < digits; ++slot) {
        if (hooks->digit_set != NULL)
            hooks->digit_set(hooks->context, slot, value % 10u);
        if (hooks->slot_hide != NULL)
            hooks->slot_hide(hooks->context, slot, 0);
        value /= 10u;
    }
    if (hooks->blank_set != NULL) hooks->blank_set(hooks->context, digits);
    for (slot = digits + 1; slot < 4; ++slot) {
        if (hooks->slot_hide != NULL)
            hooks->slot_hide(hooks->context, slot, 1);
    }
}


const char* util_root_name(BlockFile* bf)
{
    static char root_buf[64];
    if (bf == NULL || block_record_count(bf) == 0) {
        return "Lego*";
    }
    BlockRec* r0 = block_record_at(bf, 0);
    const char* name = r0 ? block_name(r0) : NULL;
    snprintf(root_buf, sizeof(root_buf), "%s", name != NULL ? name : "Lego*");
    return root_buf;
}

const char* util_level_cfg_path(BlockFile* cfg, const char* level,
                                const char* suffix, char* out, size_t out_sz)
{
    if (cfg == NULL || level == NULL) {
        return NULL;
    }
    const char* root = util_root_name(cfg);
    snprintf(out, out_sz, "%s Levels %s %s", root, level,
             suffix != NULL ? suffix : "");
    if (block_get_strptr(cfg, out) != NULL) {
        return out;
    }
    snprintf(out, out_sz, "%s %s %s", root, level,
             suffix != NULL ? suffix : "");
    return block_get_strptr(cfg, out) != NULL ? out : NULL;
}

/* Split a NUL-terminated token stream produced by `src` into fields.
 * `|` is a field separator too (the config's pipe-delimited
 * `file.bmp|x|y` triples, finding 09 §"Config format"). Returns the number
 * of fields written (0..6). */
static int util_tokenize_fields(const char* src,
                                char out[UTIL_QUERY_MAX_FIELDS][256], size_t out_sz)
{
    static const char delimiters[] = " \t\r\n|";
    int n = 0;
    const char* p = src;
    while (*p != '\0' && n < UTIL_QUERY_MAX_FIELDS) {
        p += strspn(p, delimiters);
        if (*p == '\0') {
            break;
        }
        size_t token_length = strcspn(p, delimiters);
        size_t copy_length = token_length;
        if (copy_length >= out_sz) copy_length = out_sz - 1;
        memcpy(out[n], p, copy_length);
        out[n][copy_length] = '\0';
        p += copy_length;
        ++n;
    }
    return n;
}

static int util_status_from_count(int n)
{
    if (n <= 0) {
        return 0;
    }
    if (n == 3) {
        return 3;
    }
    if (n >= 6) {
        return 6;
    }
    return 2;
}

int util_query_fields(const char* path_or_value,
                      char out[UTIL_QUERY_MAX_FIELDS][256], size_t out_sz)
{
    for (int i = 0; i < UTIL_QUERY_MAX_FIELDS; ++i) {
        out[i][0] = '\0';
    }
    if (path_or_value == NULL || path_or_value[0] == '\0') {
        return 0;
    }

    /* Case (b): the value names a resource. Parse its content when it
     * opens; otherwise fall through and parse the value inline (case a). */
    Resource* r = resource_open(path_or_value, RESOURCE_AUTO);
    if (r != NULL) {
        int32_t sz = resource_read(r);
        if (sz > 0) {
            char* buf = (char*)malloc((size_t)sz + 1);
            if (buf != NULL) {
                memcpy(buf, r->data, (size_t)sz);
                buf[sz] = '\0';
                int n = util_tokenize_fields(buf, out, out_sz);
                free(buf);
                resource_free(r);
                return util_status_from_count(n);
            }
        }
        resource_free(r);
    }

    /* Case (a): inline token list (also the path-with-pipes shape some
     * config keys use). */
    int n = util_tokenize_fields(path_or_value, out, out_sz);
    return util_status_from_count(n);
}
