// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_airbeat — validation for the EngineAirBeat sound hook (finding 03 §7).
 *
 * No audio device: airbeat_set_loop(NULL, ...) keeps messages dispatching
 * while the loop-engagement state is tracked logically. Verifies the
 * 0x11 (air low) / 0x12 (air out) messages and the loop on/off transitions.
 */
#include <stdio.h>
#include <string.h>

#include "engine.hpp"
#include "message.hpp"
#include "tickers.hpp"

static int s_failures = 0;

static void CHECK(int cond, const char* what)
{
    if (!cond) {
        printf("FAIL: %s\n", what);
        ++s_failures;
    }
}

int main(void)
{
    message_init_defaults();
    message_set_flags(0x4u);
    airbeat_set_loop(NULL, NULL); /* logic-only mode for the test */
    CHECK(!airbeat_loop_active(), "init: loop off");

    EngineState st = {};
    EngineContext ctx = {};
    ctx.state_block = &st;

    /* Started with fresh air: no low/out, no loop. */
    st.air_beat = 3;
    EngineAirBeat(&ctx, 1.0f);
    CHECK(!airbeat_loop_active(), "fresh: no loop");
    CHECK(!message_active_has(0x11) && !message_active_has(0x12),
          "fresh: no air messages");

    /* Drain to low (<= 1): 0x11 + loop engaged. */
    st.air_beat = 1;
    EngineAirBeat(&ctx, 1.0f);
    CHECK(message_active_has(0x11), "low: 0x11 dispatched");
    CHECK(airbeat_loop_active(), "low: loop engaged");

    /* Still low: no re-fire of 0x11 (dedupe), loop stays. */
    message_clear();
    message_init_defaults();
    message_set_flags(0x4u);
    st.air_beat = 1;
    EngineAirBeat(&ctx, 1.0f);
    CHECK(airbeat_loop_active(), "still-low: loop stays");

    /* Out (0): 0x12 dispatched, loop remains engaged (alarm until restored). */
    message_clear();
    message_init_defaults();
    message_set_flags(0x4u);
    st.air_beat = 0;
    EngineAirBeat(&ctx, 1.0f);
    CHECK(message_active_has(0x12), "out: 0x12 dispatched");
    CHECK(!message_active_has(0x11), "out: no 0x11 after dedupe");
    CHECK(airbeat_loop_active(), "out: loop remains");

    if (s_failures == 0) {
        printf("airbeat_test: PASS\n");
        return 0;
    }
    printf("airbeat_test: %d failure(s)\n", s_failures);
    return 1;
}