// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — message/notification bus (corpus MessageDispatch @
 * 0x419b80, MessagePreCheck @ 0x4199f0; findings 04/22).
 *
 * The corpus bus: a 40-id config table (stride 0x14: sound handle, cooldown,
 * flags) + an active queue (stride 5: id, negative age, float timestamp).
 * Semantics preserved here:
 *   - system gate: flags bit 4 (DAT_004dcd48) + game-not-live word bit
 *     (DAT_004ff980 & 0x100, finding 22).
 *   - auto-cooldown rule (flags bit 8): ids flagged 0x20000 reset their
 *     25.0 cooldown and mute their sound; the linked family
 *     {10,11,12,0x26=38} resets together.
 *   - dedupe (MessagePreCheck): ids 10/11/12/38 skip; otherwise subscriber
 *     nodes under matching queued IDs are compared with the dispatch payload
 *     by LAB_004199a0, and a matching node suppresses re-dispatch.
 *   - slow-mo path (flags bit 0x10 + cfg bit 0x10000) is a stub until the
 *     time-scale transition layer lands.
 * The InfoMessages catalog (finding 22) is seeded by message_init_defaults
 * with the corpus's 40 Info_* names; real per-entry config arrives from the
 * level-config layer later.
 */
#ifndef LEGORRR_MESSAGE_HPP
#define LEGORRR_MESSAGE_HPP

#include <cstdint>
#include <cstddef>

#include "block.hpp"

struct Sound;

#define MESSAGE_MAX_ID 40     /* DAT_004dcd38 cap (0x28) */
#define MESSAGE_FAMILY {10, 11, 12, 38}

/* Reconstruction flag: entry marked AlwaysPlaySFX — plays regardless of the
 * game state (finding 22 §1's "play regardless" rule; the corpus bit is
 * read from the 0x20000 cooldown wiring, this is our own marker). */
#define MESSAGE_FLAG_PLAY_ALWAYS 0x8000u

/* per-id config, shaped as the corpus's 40 stride-0x14 incident-message
 * records at DAT_004dc704: the countdown rides at record +0x00 (both
 * InfoMessageTimerDecay's walk and MessageDispatch's reset touch [rec],
 * finding 441 §the countdown band), so the field order is load-bearing. */
typedef struct MessageConfig {
    float           cooldown;   /* corpus record +0x00: the walked countdown */
    const char*     name;       /* Info_* catalog name (finding 22) */
    const char*     sound;      /* sound resource name */
    uint32_t        flags;      /* 0x10000 repeatable/slow-mo, 0x20000 auto-cd */
    unsigned char   pad_[4];    /* corpus stride 0x14 */
} MessageConfig;

/* hooks (reconstruction-owned; corpus uses function pointers/vtables) */
typedef int  (*MessageCallback)(int id); /* port notification after append */
typedef void (*MessageSoundHook)(int id, float vol); /* FUN_00464ef0 role */

void message_init_defaults(void);   /* seed the 40-id catalog (finding 22) */
extern "C" void ui_message_strings_initialize(void* primary_font, void* secondary_font,
                                   void* message_resource, void* subtitle_font,
                                   float line_delay); /* FUN_0046a7f0 */
void* ui_message_text_output_get(int alternate); /* DAT_0050361c/620 */
extern "C" void ui_message_overlay_state_initialize(void); /* FUN_0046aa90 */
const char* ui_message_label_get(unsigned int index);
extern "C" int ui_message_label_index_find(const char* label, int* out_index); /* FUN_0046aad0 */
extern "C" void ui_message_text_slot_assign(int index, const char* text); /* FUN_0046ab10 */
extern "C" void ui_message_text_slot_assign_with_image(int index, const char* text,
                                            const char* image_path,
                                            const char* display_path); /* FUN_0046abb0 */
const char* ui_message_text_slot_get(unsigned int index);
struct TextureSurface;
TextureSurface* ui_message_image_slot_get(unsigned int index);
void ui_message_image_position_get(float* x, float* y);
void message_clear(void);           /* reset queue + config for tests */

/* IncidentMessageCatalogInit / IncidentMessageTemplateReplace /
 * IncidentMessageTemplateCompile (FUN_00419230 / 0x004194b0 / 0x00419530).
 * The catalog names are statically seeded; the initializer retains its
 * opaque caller word and raises the corpus's 0x18 flags. Template compile
 * emits the format string and trailing substitution metadata. */
extern "C" void incident_message_catalog_init(uint32_t owner);
uint32_t incident_message_catalog_owner_get(void); /* port state peek */
struct TextureSurface;
void info_message_surface_load(int id, const char* path); /* FUN_00419650 */
struct TextureSurface* info_message_surface_peek(int id); /* port state peek */
/* Legacy audio adapter retained for callers outside the retail message-image
 * path; 0x419b50 reads the BitmapSurfaceLoad resource, not WAV duration. */
void info_message_sound_load(int id, const char* path); /* FUN_00419650 */
struct Sound* info_message_sound_peek(int id); /* port state peek */
float remaining_timer_duration_get(void); /* FUN_00419b50 */
void incident_message_template_replace(int id, const char* text);
void incident_message_template_compile(const char* text, char** out);
void* incident_message_node_create(const uint32_t* first_pair,
                                   uint32_t value, const char* text);
const char* incident_message_node_text_get(const void* node);
void incident_message_node_release(void* node);

void message_config_set(int id, const char* sound, float cooldown,
                        uint32_t flags);
void message_set_flags(uint32_t flags);       /* DAT_004dcd48 */
void message_set_system_state(uint32_t v);    /* DAT_004ff980 */
void message_set_callback(MessageCallback cb);
void message_set_sound_hook(MessageSoundHook hook);

/* Per-id read accessors (tests/diagnostics + the sound hook). */
const char* message_sound(int id);

/* Load `InfoMessages` (FUN_00434c10): each named row's colon-separated value
 * contains display text, image resource, optional sound alias, and the SLOW /
 * ALWAYSPLAYSFX mode fields. */
extern "C" void message_load_infomessages(BlockFile* cfg);

/* fire a message (corpus MessageDispatch). p2 is an optional text override,
 * p3 is the node payload dword, and p4 points to the optional two-dword
 * event data copied into the incident node. */
extern "C" void message_dispatch(int id, const char* p2, uint32_t p3, const void* p4);

/* Index of the queued message id, or -1 (corpus InfoMessageQueueFindId). */
int message_queue_find_id(int id);                /* FUN_00419b20 */

/* IncidentMessageConfigLookup (FUN_004193e0): C-locale ASCII-fold lookup
 * of the config name table; on a hit stores the id and answers 1.
 * Deviation: the corpus strides the name-pointer table blindly (its
 * install rides the unported catalog-init band); the port guards the name
 * and the answer word and rides the config name model. */
int incident_message_config_lookup(const char* name, int* out_id);

/* advance the bus clock; drives cooldown comparisons. */
void message_update(float dt);

/* corpus FUN_0041a2c0: decrement every configured cooldown toward 0. */
extern "C" void message_cooldown_tick(float dt);

/* InfoMessageQueueTick (FUN_00419f30): advance each active message's
 * display-offset and remaining-time words. The original then updates a
 * renderer-owned bitmap handle and draws through a state-owned text object;
 * those two operations are supplied by the active UI owner. */
typedef void (*InfoMessageQueueResourceUpdateHook)(
    void* context, void* resource, const float position[2]);
typedef void (*InfoMessageQueueStateDrawHook)(void* context,
                                             uint32_t state_handle);
typedef void* (*InfoMessageQueueGlobalResourceHook)(void* context);
/* Reads the signed dword at the renderer resource record's +8. The portable
 * TextureSurface does not expose the retail bitmap-record/DirectDraw layout. */
typedef int32_t (*InfoMessageQueueResourceLimitHook)(
    void* context, void* resource);
typedef struct InfoMessageQueueTickOps {
    void* context;
    InfoMessageQueueResourceUpdateHook resource_update;
    InfoMessageQueueStateDrawHook state_draw;
    InfoMessageQueueGlobalResourceHook global_resource_get;
    InfoMessageQueueResourceLimitHook resource_limit_get;
} InfoMessageQueueTickOps;
void info_message_queue_tick_ops_set(const InfoMessageQueueTickOps* ops);
extern "C" void info_message_queue_tick(float dt); /* FUN_00419f30 */

/* IncidentMessageTextConsumer (FUN_0041a080): the queue/catalog and update
 * bit are message-owned. The scrolling panel record, UI state word, transition
 * request, formatted text append, scroll metrics, and draw entry belong to the
 * UI owner and are exposed here as narrow operations. */
typedef struct IncidentMessageTextConsumerOps {
    void* context;
    uint32_t (*ui_state_flags_get)(void* context); /* DAT_0050028c */
    void* (*text_output_get)(void* context);       /* DAT_005007d8 */
    uint32_t (*scroll_offset_get)(void* context);  /* DAT_004dcd44 */
    void (*panel_transition_request)(void* context, int mode); /* 7 */
    void (*text_output_reset)(void* context, void* output);
    void (*text_output_append)(void* context, void* output,
                               const char* format,
                               const uint8_t arguments[5],
                               unsigned int argument_count);
} IncidentMessageTextConsumerOps;
void incident_message_text_consumer_ops_set(
    const IncidentMessageTextConsumerOps* ops);
extern "C" void incident_message_text_consumer(float frame_delta);

/* CatalogSlotFlagUpdate (FUN_0041c800): category 1 and 4 rows each retain a
 * low-bit state word. The catalog owner supplies the two eligibility checks
 * and the category-1 rank query; the retail no-op comparison makes rank
 * always pass once those checks succeed. */
typedef int (*CatalogSlotPredicateHook)(int category, int slot, void* context);
typedef int (*CatalogSlotRankHook)(int category, int slot, void* context);
void catalog_slot_flag_hooks_set(CatalogSlotPredicateHook eligible,
                                 CatalogSlotPredicateHook available,
                                 CatalogSlotRankHook rank, void* context);
void catalog_slot_flag_word_set(int category, int slot, uint32_t flags);
uint32_t catalog_slot_flag_word_get(int category, int slot);
void catalog_slot_flag_global_state_set(uint32_t flags);
void catalog_slot_flag_update(int category, int slot);

/* active queue census (tests/diagnostics). */
int  message_active_count(void);
int  message_active_has(int id);

/* per-id cooldown value (tests/diagnostics). */
float message_cooldown(int id);

/* flags census (tests/diagnostics): the global flags word and the
 * per-id config flags (the message_active_count precedent). */
uint32_t message_flags_peek(void);
uint32_t message_config_flags(int id);

/* SystemStateFlags / SetSystemStateFlags (FUN_00456240 / FUN_00456550,
 * findings 04/05/440): the corpus's get / set accessors over the
 * game-state bitmask at DAT_004ff980 — the module's own s_system_state,
 * read directly and installed through a pointed-to value. The setter
 * returns the installed value; NULL answers 0 and stores nothing
 * (documented deviation; corpus callers pass live pointers). */
extern "C" uint32_t system_state_flags_get(void);
uint32_t system_state_flags_set(const uint32_t* value);
extern "C" uint32_t system_state_flags_set_target(const uint32_t* value);
uint32_t objective_switch_state_set(uint32_t value);
uint32_t objective_switch_state_get(void);

/* --- corpus flag leaves (2026-10-24) --------------------------------------
 * SelectionFlag10SetClear (FUN_00419450, findings 376/441): raise or clear
 * bit 0x10 in the selection-flags global — our s_flags (DAT_004dcd48).
 * InfoMessageAlwaysPlayFlagSet / IndexedSelectionFlag20000SetClear
 * (FUN_00419470 / FUN_004194f0, finding 441): raise or clear the per-id
 * config record's 0x10000 / 0x20000 flag bits — our per-id config flag
 * word (the corpus table's flag dword). The corpus indexes its config
 * records blindly; the port bounds-checks (documented deviation). */
void selection_flag10_set_clear(int set);
void info_message_always_play_flag_set(int id, int set);
void indexed_selection_flag_20000_set_clear(int id, int set);

/* --- the incident/message DAT-global band (2026-10-24; findings 05/250/441) -
 * The corpus's message config block (DAT_004dc6fc…, stride 0x14 — all-zero
 * byte-read per finding 05) and the incident-state word FUN_0041a2f0 (DAT_004dcd58) have
 * no ported feeder — the shapes land as guarded zero-init storage per the
 * unowned-global convention. The catalog read (FUN_00419420) is the same
 * stride-0x14 dword the corpus's InfoMessageSoundLoad populates into the
 * record's +4 (finding 441 — despite its name it does not read a template
 * field); the secondary-sound store (FUN_00419680) writes the adjacent
 * dword of the same record block. */
uint32_t incident_message_text_catalog_get(int id);
void     info_message_secondary_sound_set(int id, uint32_t value);
uint32_t incident_message_state_get(void);

/* DeferredCallbackSet (FUN_00453900, findings 256/440): the corpus stores
 * the callback-presence word (DAT_004a673c) that GameConfigInit seeds with
 * whether the corresponding message resource was found; the corpus reader
 * (DeferredCallbackInvokeIfSet) is unported, so the word lands as
 * zero-init storage with a peek. */
extern "C" void     deferred_callback_set(uint32_t value);
uint32_t deferred_callback_peek(void);

/* IncidentMessageFlagSet (FUN_00419ae0, findings 06/440): raises or drops
 * bit 4 of the message flag word through the module's own s_flags. */
extern "C" void incident_message_flag_set(int value);

/* HasActiveResource (FUN_004119d0, finding 440): a record with a nonzero
 * +0x38 word answers through the system-state bitmask. NULL record guarded
 * (corpus strides blindly). */
int has_active_resource(const void* record);       /* FUN_004119d0 */

/* IndexedLinkedListEntryGet (FUN_00419780): walk the corpus's indexed
 * list table (five-dword entries: head +0, count +4, chained through
 * the node's +0x10 link) to the entry at index param_2; out-of-range
 * answers 0. IncidentMessageQueueAppend (FUN_004196f0) is the ported
 * append writer; other corpus table writers remain unported. */
void indexed_list_install(int list, void* head, int count); /* test hook */
void* indexed_linked_list_entry_get(int list, int index);   /* FUN_00419780 */
void incident_message_queue_append(int list, void* entry); /* FUN_004196f0 */
typedef int (*MessageSubscriberCallback)(void* entry, void* context);
int message_subscriber_dispatch(int list, MessageSubscriberCallback callback,
                                void* context); /* FUN_004196a0 */
typedef void (*MessagePanelRefreshHook)(void* auxiliary, void* queue,
                                        int reserved, void* context);
void incident_message_panel_refresh_hook_install(
    MessagePanelRefreshHook hook, void* context);
void incident_message_panel_aux_install(void* auxiliary); /* port hook */
extern "C" void incident_message_panel_refresh_if_active(void); /* FUN_00419da0 */
void* incident_message_queue_remove_at(int list, int index); /* FUN_004197b0 */

#endif /* LEGORRR_MESSAGE_HPP */
