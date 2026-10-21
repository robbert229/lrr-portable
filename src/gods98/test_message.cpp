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

    if (s_failures == 0) {
        printf("message_test: PASS\n");
        return 0;
    }
    printf("message_test: %d failure(s)\n", s_failures);
    return 1;
}