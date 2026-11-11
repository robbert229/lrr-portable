// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; #496): what text.cpp sees from
 * message.cpp. A verbatim snapshot of message_shared.hpp as it was when the
 * #496 migration froze it, so the unit's declarations no longer change
 * with later relocations. Never edited by tools/relocate_unit.py. */
#ifndef LRR_GODS98_MESSAGE_FOR_TEXT_HPP
#define LRR_GODS98_MESSAGE_FOR_TEXT_HPP

#define IDX_LIST_SLOTS MESSAGE_MAX_ID

/* The corpus keeps each queued message as one 0x14-byte record at
 * DAT_004dca20: ID at +0, two timer floats at +4/+8, and two opaque dwords.
 * Own storage, zero-init like the corpus's zero-filled tail. */
struct MessageQueueRecord {
    int32_t id;
    float transition_value; /* retail +4; initialized from -int(resource+4) */
    float remaining_value;  /* retail +8; initialized by 0x419b50 */
    unsigned char pad[8];
};

struct IncidentNodeTextSlot {
    void* node;
    char* text;
};

#define INCIDENT_NODE_TEXT_SLOTS 256

#define MESSAGE_NODE_LIMIT (MESSAGE_MAX_ID * 9)

#define MSG_DAT_WINDOW 0x28

typedef struct IncidentMessageCatalogRecord {
    uint32_t resource;
    uint32_t secondary_sound;
    uint32_t reserved[3];
} IncidentMessageCatalogRecord;

extern MessageConfig s_cfg[MESSAGE_MAX_ID];
extern MessageQueueRecord s_queue_records[MESSAGE_MAX_ID];
extern int s_queue_count;
extern uint32_t s_flags;
extern uint32_t s_system_state;
extern float s_now;
extern MessageCallback s_callback;
extern MessageSoundHook s_sound;
extern char* s_compiled_template[MESSAGE_MAX_ID];
extern TextureSurface* s_info_message_surfaces[MESSAGE_MAX_ID];
extern uint32_t s_catalog_owner;
extern IncidentNodeTextSlot s_node_text[INCIDENT_NODE_TEXT_SLOTS];
extern void* s_idx_head[IDX_LIST_SLOTS];
extern int s_idx_count[IDX_LIST_SLOTS];
extern void* s_idx_aux[IDX_LIST_SLOTS];
extern void* s_message_nodes[MESSAGE_NODE_LIMIT];
extern int s_message_node_count;
extern MessagePanelRefreshHook s_panel_refresh_hook;
extern void* s_panel_refresh_context;
extern InfoMessageQueueTickOps s_info_message_queue_tick_ops;
extern IncidentMessageTextConsumerOps s_incident_message_text_ops;
extern const char* const s_catalog_defaults[MESSAGE_MAX_ID];
extern const char* s_catalog[MESSAGE_MAX_ID];
extern IncidentMessageCatalogRecord s_msg_catalog[MSG_DAT_WINDOW];
extern uint32_t s_incident_state;

extern uint32_t s_objective_switch;
extern uint32_t s_deferred_callback_presence;

extern const char* s_ui_message_labels[26];
extern void* s_ui_message_primary_font;
extern void* s_ui_message_secondary_font;
extern void* s_ui_message_resource;
extern void* s_ui_message_subtitle_font;
extern char* s_ui_message_text_slots[26];
extern TextureSurface* s_ui_message_image_slots[26];

#endif /* LRR_GODS98_MESSAGE_FOR_TEXT_HPP */
