// SPDX-License-Identifier: LGPL-3.0-or-later
#include <string.h>
#include <stdlib.h>

#include "game/savegame.hpp"
#include "message.hpp"

#define MESSAGE_CD_RESET 25.0f /* 0x41c80000 */

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

/* The InfoMessages catalog — finding 22's 40 names, in table order.
 * (Corpus fills its stride-4 table at DAT_004dc658 at runtime through
 * CatalogInit, so the object stays unannotated: image bytes are zero
 * there, ours are the name pointers.) */
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
    memset(s_queue_records, 0, sizeof(s_queue_records));
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

static int msg_streql(const char* a, const char* b)
{
    for (;;) {
        int ca = (unsigned char)*a++;
        int cb = (unsigned char)*b++;
        if (ca >= 'a' && ca <= 'z') ca -= 0x20;
        if (cb >= 'a' && cb <= 'z') cb -= 0x20;
        if (ca != cb) return 0;
        if (ca == 0) return 1;
    }
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
            || !msg_streql(block_name(rec), "InfoMessages")) {
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
                if (msg_streql(nm, s_catalog[k])) {
                    id = k;
                    break;
                }
            }
            if (id < 0) {
                continue;
            }
            uint32_t flags = 0;
            const char* sound = NULL;
            if (val != NULL && !msg_streql(val, "AlwaysPlaySFX")
                && strlen(val) >= 4) {
                sound = val; /* corpus: short/empty flag strings mute */
            }
            if (val == NULL || msg_streql(val, "AlwaysPlaySFX")
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
static uint32_t s_msg_catalog[MSG_DAT_WINDOW]; /* DAT_004dc6fc stride-0x14 block */
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
    return system_state_flags_get() != 0u ? 1 : 0;
}

/* --- the indexed-list window (see message.hpp) ---------------------------- */

#define IDX_LIST_SLOTS 16 /* deviation: the corpus strides the list index */
static void* s_idx_head[IDX_LIST_SLOTS];  /* DAT_004dca18 role */
static int   s_idx_count[IDX_LIST_SLOTS]; /* DAT_004dca1c role */

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
        __builtin_memcpy(&next, (const char*)entry + 0x10, sizeof next);
        entry = next;
        --index;
    }
    return entry;
}
