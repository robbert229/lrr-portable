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

#define MESSAGE_MAX_ID 40     /* DAT_004dcd38 cap (0x28) */
#define MESSAGE_FAMILY {10, 11, 12, 38}

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

/* fire a message (corpus MessageDispatch). p2/p3/p4 are payload carried to
 * the callback; unused flags are kept for signature fidelity. */
void message_dispatch(int id, uint32_t p2, uint32_t p3, const void* p4);

/* advance the bus clock; drives cooldown comparisons. */
void message_update(float dt);

/* corpus FUN_0041a2c0: decrement every configured cooldown toward 0. */
void message_cooldown_tick(float dt);

/* active queue census (tests/diagnostics). */
int  message_active_count(void);
int  message_active_has(int id);

/* per-id cooldown value (tests/diagnostics). */
float message_cooldown(int id);

#endif /* LEGORRR_MESSAGE_HPP */