// SPDX-License-Identifier: LGPL-3.0-or-later
#include <string.h>
#include <stdlib.h>

#include "message.hpp"

#define MESSAGE_CD_RESET 25.0f /* 0x41c80000 */

static MessageConfig s_cfg[MESSAGE_MAX_ID];
static int           s_queue_ids[MESSAGE_MAX_ID];
static float         s_queue_ts[MESSAGE_MAX_ID];
static int           s_queue_count = 0;
static uint32_t      s_flags = 0u;          /* DAT_004dcd48 */
static uint32_t      s_system_state = 0u;   /* DAT_004ff980 */
static float         s_now = 0.0f;          /* _DAT_0049e264 analog */
static MessageCallback  s_callback = NULL;
static MessageSoundHook s_sound = NULL;

/* The InfoMessages catalog — finding 22's 40 names, in table order. */
static const char* const s_catalog[MESSAGE_MAX_ID] = {
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
        s_cfg[i].name = s_catalog[i];
    }
    s_flags = 0x4u; /* system on */
}

void message_clear(void)
{
    memset(s_cfg, 0, sizeof(s_cfg));
    memset(s_queue_ids, 0, sizeof(s_queue_ids));
    memset(s_queue_ts, 0, sizeof(s_queue_ts));
    s_queue_count = 0;
    s_now = 0.0f;
    s_flags = 0;
    s_system_state = 0;
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
void message_set_system_state(uint32_t v)     { s_system_state = v; }
void message_set_callback(MessageCallback cb) { s_callback = cb; }
void message_set_sound_hook(MessageSoundHook h) { s_sound = h; }

/* Dedupe-with-live-callback (corpus MessagePreCheck FUN_004199f0). */
static int message_pre_check(int id)
{
    if (id == 10 || id == 11 || id == 12 || id == 38) {
        return 0; /* exception list skips dedupe */
    }
    for (int i = 0; i < s_queue_count; ++i) {
        if (s_queue_ids[i] == id) {
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

static int message_queue_find(int id)
{
    for (int i = 0; i < s_queue_count; ++i) {
        if (s_queue_ids[i] == id) {
            return i;
        }
    }
    return -1;
}

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
        int idx = message_queue_find(id);
        if (idx < 0) {
            idx = s_queue_count;
            s_queue_ts[idx] = s_now;
            s_queue_count += 1;
        }
        s_queue_ids[idx] = id;
        if (s_callback != NULL) {
            s_callback(id);
        }
    }
}

void message_update(float dt)
{
    s_now += dt;
}

/* corpus FUN_0041a2c0: the ticker walks the cooldown array (stride 5 in the
 * corpus block; per-id here) and decrements toward zero. */
void message_cooldown_tick(float dt)
{
    for (int i = 0; i < MESSAGE_MAX_ID; ++i) {
        if (s_cfg[i].cooldown > 0.0f) {
            s_cfg[i].cooldown -= dt;
        }
    }
}

int message_active_count(void)
{
    return s_queue_count;
}

int message_active_has(int id)
{
    return message_queue_find(id) >= 0;
}

float message_cooldown(int id)
{
    if (id < 0 || id >= MESSAGE_MAX_ID) {
        return 0.0f;
    }
    return s_cfg[id].cooldown;
}