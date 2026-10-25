// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_message — validation for src/gods98/message.cpp (MessageDispatch
 * FUN_00419b80 / MessagePreCheck FUN_004199f0; findings 04/22).
 *
 * Covers the gates, queue/dedupe (incl. the exception ids 10/11/12/38), the
 * auto-cooldown rule + linked family reset, and the 40-entry cap.
 */
#include <stdio.h>
#include <string.h>

#include "block.hpp"
#include "message.hpp"

static int s_failures = 0;
static int s_fires = 0;   /* callback hook fires */

static void CHECK(int cond, const char* what)
{
    if (!cond) {
        printf("FAIL: %s\n", what);
        ++s_failures;
    }
}

static int fire(int /*id*/)
{
    ++s_fires;
    return 1;
}

int main(void)
{
    message_init_defaults();
    CHECK((message_active_count() == 0), "init: empty queue");
    message_set_callback(fire);
    message_update(0.1f);

    /* gates: system off suppresses. */
    message_set_flags(0x0u);
    message_dispatch(0, 0, 0, NULL);
    CHECK(s_fires == 0, "gate: system off");
    message_set_flags(0x4u);
    message_set_system_state(0x100u);
    message_dispatch(0, 0, 0, NULL);
    CHECK(s_fires == 0, "gate: not live");
    message_set_system_state(0);

    /* basic dispatch + config */
    message_config_set(3, "Sounds\\x.wav", 0.0f, 0);
    message_dispatch(3, 0, 0, NULL);
    CHECK(message_active_has(3) && message_active_count() == 1,
          "dispatch: id queued");
    CHECK(s_fires == 1, "dispatch: fired once");

    /* dedupe suppresses a second fire of an active id. */
    message_dispatch(3, 0, 0, NULL);
    CHECK(message_active_count() == 1, "dedupe: no re-queue");
    CHECK(s_fires == 2, "dedupe: duplicate ran live callback");

    /* exception ids {10,11,12,38} skip dedupe. */
    message_dispatch(11, 0, 0, NULL);
    message_dispatch(11, 0, 0, NULL);
    CHECK(message_active_has(11), "exception: id queued");
    CHECK(s_fires == 4, "exception: both dispatches fired");

    /* auto-cooldown rule: flags(+0x8 flag, cfg 0x20000) with spent cooldown
     * resets the linked family and mutes; nothing is queued. */
    message_clear();
    message_init_defaults();
    message_set_callback(fire);
    message_set_flags(0x4u | 0x8u);
    message_config_set(10, "Sounds\\a.wav", 0.0f, 0x20000u);
    message_dispatch(10, 0, 0, NULL);
    CHECK(message_active_count() == 0, "cooldown: no queue");
    message_update(0.1f);
    message_dispatch(10, 0, 0, NULL); /* cooldown still future -> fires */
    CHECK(s_fires > 0, "cooldown: later dispatch fires");

    /* 40-entry cap. */
    message_clear();
    message_init_defaults();
    message_set_callback(fire);
    for (int i = 0; i < 44; ++i) {
        message_dispatch(i % 40, 0, 0, NULL); /* walk the 0..39 id space */
    }
    CHECK(message_active_count() == 40, "cap: 40 slots max");

    /* InfoMessages config -> per-id sounds (finding 22 §1, FUN_00434c10). */
    {
        char txt[] =
            "InfoMessages {\n"
            "  UnderAttack Sounds\\UnderAttack.wav\n"
            "  Info_CrystalFound Sounds\\Crystal.wav\n"
            "  LegoManDeath AlwaysPlaySFX\n"
            "  NotAnEntry Sounds\\Ignored.wav\n"
            "}\n";
        BlockFile* bf = block_parse(txt, strlen(txt));
        CHECK(bf != NULL, "infomessages: parse");
        message_clear();
        message_init_defaults();
        message_load_infomessages(bf);
        CHECK(message_sound(5) != NULL
              && strcmp(message_sound(5), "Sounds\\UnderAttack.wav") == 0,
              "infomessages: UnderAttack sound configured");
        CHECK(message_sound(0) != NULL
              && strcmp(message_sound(0), "Sounds\\Crystal.wav") == 0,
              "infomessages: Info_-prefixed CrystalFound sound configured");
        CHECK(message_sound(10) == NULL, "infomessages: AlwaysPlaySFX has no sound");
        CHECK(message_sound(11) == NULL, "infomessages: unknown entries ignored");
        block_close(bf);
    }

    /* The system-state accessors (2026-10-24): get/set over the module's
     * own DAT_004ff980 static — shared with the diagnostics setter. */
    {
        const uint32_t before = system_state_flags_get();
        const uint32_t probe = 0x1234u;
        CHECK(system_state_flags_set(&probe) == probe,
              "the setter returns the installed value");
        CHECK(system_state_flags_get() == 0x1234u,
              "the getter reads it back");
        message_set_system_state(before); /* leave the module as found */
        CHECK(system_state_flags_get() == before,
              "the diagnostics setter shares the same static");
        CHECK(system_state_flags_set(NULL) == 0u
                  && system_state_flags_get() == before,
              "NULL answers zero and stores nothing (deviation)");
    }

    /* The corpus flag leaves (2026-10-24): global bit 0x10 and the two
     * per-id config bits over the module's own storage. */
    {
        message_set_flags(0);
        selection_flag10_set_clear(1);
        CHECK(message_flags_peek() == 0x10u, "the raise sets bit 0x10");
        selection_flag10_set_clear(1);
        CHECK(message_flags_peek() == 0x10u, "raising twice is idempotent");
        selection_flag10_set_clear(0);
        CHECK(message_flags_peek() == 0u, "the clear drops it");
        message_set_flags(0x130u);
        selection_flag10_set_clear(0);
        CHECK(message_flags_peek() == (0x130u & ~0x10u),
              "the clear spares the neighbours");
    }
    {
        message_config_set(5, NULL, 0.0f, 0u);
        indexed_selection_flag_20000_set_clear(5, 1);
        CHECK(message_config_flags(5) == 0x20000u, "the 0x20000 leaf raises");
        info_message_always_play_flag_set(5, 1);
        CHECK(message_config_flags(5) == 0x30000u,
              "the 0x10000 leaf raises next to it");
        info_message_always_play_flag_set(5, 0);
        indexed_selection_flag_20000_set_clear(5, 0);
        CHECK(message_config_flags(5) == 0u, "both clears land");

        indexed_selection_flag_20000_set_clear(-1, 1);
        info_message_always_play_flag_set(MESSAGE_MAX_ID, 1);
        CHECK(1, "the guarded ids are no-ops");

        /* neighbours stay untouched (per-id isolation) */
        message_config_set(6, NULL, 0.0f, 0x20000u);
        indexed_selection_flag_20000_set_clear(7, 1);
        CHECK(message_config_flags(6) == 0x20000u
                  && message_config_flags(7) == 0x20000u,
              "the raise stays on its id");
        indexed_selection_flag_20000_set_clear(7, 0);
        CHECK(message_config_flags(6) == 0x20000u, "the clear spares the id");
    }

    /* The incident/message DAT-global band (2026-10-24). */
    {
        CHECK(incident_message_text_catalog_get(0) == 0u
                  && incident_message_text_catalog_get(MESSAGE_MAX_ID) == 0u,
              "message: the catalog window answers zero, guarded");
        info_message_secondary_sound_set(7, 0x1234u);
        info_message_secondary_sound_set(-1, 9u);   /* guarded */
        info_message_secondary_sound_set(MESSAGE_MAX_ID, 9u); /* guarded */
        CHECK(incident_message_state_get() == 0u,
              "message: the incident state word starts clear");
        info_message_secondary_sound_set(7, 0u); /* restore for later tests */
    }

    /* DeferredCallbackSet (2026-10-24). */
    {
        CHECK(deferred_callback_peek() == 0u, "presence starts absent");
        deferred_callback_set(1u);
        CHECK(deferred_callback_peek() == 1u, "the store lands");
        deferred_callback_set(0u);
    }

    /* IncidentMessageFlagSet (2026-10-24): s_flags bit 4. */
    {
        const uint32_t saved = message_flags_peek();
        message_set_flags(0u);
        incident_message_flag_set(1);
        CHECK((message_flags_peek() & 4u) != 0u, "the flag rises");
        incident_message_flag_set(0);
        CHECK((message_flags_peek() & 4u) == 0u, "the flag drops");
        incident_message_flag_set(1);
        message_set_flags(saved); /* restore the boot state */
        CHECK(message_flags_peek() == saved, "the test restores the word");
    }

    /* HasActiveResource (2026-10-24): the +0x38 handle and the flags. */
    {
        char rec[0x40];
        memset(rec, 0, sizeof rec);
        const uint32_t before = system_state_flags_get();
        uint32_t on = 0x1u;
        system_state_flags_set(&on);
        /* Clear handle: answers no even with flags set. */
        CHECK(has_active_resource(rec) == 0, "a clear handle answers no");
        const uint32_t handle = 0x5u;
        __builtin_memcpy(rec + 0x38, &handle, sizeof handle);
        CHECK(has_active_resource(rec) == 1, "the handle rides the flags");
        uint32_t off = 0u;
        system_state_flags_set(&off);
        CHECK(has_active_resource(rec) == 0, "clear flags answer no");
        CHECK(has_active_resource(NULL) == 0, "the NULL record is guarded");
        system_state_flags_set(&before); /* restore */
    }

    if (s_failures == 0) {
        printf("message_test: PASS\n");
        return 0;
    }
    printf("message_test: %d failure(s)\n", s_failures);
    return 1;
}