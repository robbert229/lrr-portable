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
 *   - dedupe (MessagePreCheck): ids 10/11/12/38 skip; otherwise a queued id
 *     has its stored callback run immediately (LAB_004199a0) and suppresses
 *     a re-dispatch.
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

#define MESSAGE_MAX_ID 40     /* DAT_004dcd38 cap (0x28) */
#define MESSAGE_FAMILY {10, 11, 12, 38}

/* Reconstruction flag: entry marked AlwaysPlaySFX — plays regardless of the
 * game state (finding 22 §1's "play regardless" rule; the corpus bit is
 * read from the 0x20000 cooldown wiring, this is our own marker). */
#define MESSAGE_FLAG_PLAY_ALWAYS 0x8000u

/* per-id config (corpus stride 0x14) */
typedef struct MessageConfig {
    const char* name;       /* Info_* catalog name (finding 22) */
    const char* sound;      /* sound resource name (0x04 analog) */
    float       cooldown;   /* cooldown timer (0x08 analog) */
    uint32_t    flags;      /* 0x10000 repeatable/slow-mo, 0x20000 auto-cd */
} MessageConfig;

/* hooks (reconstruction-owned; corpus uses function pointers/vtables) */
typedef int  (*MessageCallback)(int id);             /* LAB_004199a0 role */
typedef void (*MessageSoundHook)(int id, float vol); /* FUN_00464ef0 role */

void message_init_defaults(void);   /* seed the 40-id catalog (finding 22) */
void message_clear(void);           /* reset queue + config for tests */

void message_config_set(int id, const char* sound, float cooldown,
                        uint32_t flags);
void message_set_flags(uint32_t flags);       /* DAT_004dcd48 */
void message_set_system_state(uint32_t v);    /* DAT_004ff980 */
void message_set_callback(MessageCallback cb);
void message_set_sound_hook(MessageSoundHook hook);

/* Per-id read accessors (tests/diagnostics + the sound hook). */
const char* message_sound(int id);

/* Populate per-id sounds/cooldowns from the `InfoMessages` config section
 * (corpus FUN_00434c10, finding 22 §1): each entry keyed by the Info name
 * carries its sound; an entry whose value is empty/short or `AlwaysPlaySFX`
 * sets the play-always flag instead. */
void message_load_infomessages(BlockFile* cfg);

/* fire a message (corpus MessageDispatch). p2/p3/p4 are payload carried to
 * the callback; unused flags are kept for signature fidelity. */
void message_dispatch(int id, uint32_t p2, uint32_t p3, const void* p4);

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
void message_cooldown_tick(float dt);

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
uint32_t system_state_flags_get(void);
uint32_t system_state_flags_set(const uint32_t* value);

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
void     deferred_callback_set(uint32_t value);
uint32_t deferred_callback_peek(void);

/* IncidentMessageFlagSet (FUN_00419ae0, findings 06/440): raises or drops
 * bit 4 of the message flag word through the module's own s_flags. */
void incident_message_flag_set(int value);

#endif /* LEGORRR_MESSAGE_HPP */