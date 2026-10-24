// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_input — validation for src/gods98/input.cpp.
 *
 * Pure state-machine tests (findings 04/05): key-table roll, edge/release
 * semantics, KeyToAscii (no-HKL path), the context filter, the EnginePreTick
 * text pump, and the corner-dwell quit machine. No game data required.
 *
 * Wire-up: meson test 'input' (suite 'input').
 */
#include <stdio.h>
#include <string.h>

#include "input.hpp"

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
    InputState in = {};
    input_reset(&in);

    /* begin_frame rolls cur -> prev and clears cur. */
    in.cur['a'] = 1;
    in.cur[0x30] = 1;
    input_begin_frame(&in);
    CHECK(in.prev['a'] == 1 && in.cur['a'] == 0, "begin_frame roll");
    CHECK(in.prev[0x30] == 1 && in.cur[0x30] == 0, "begin_frame roll 2");

    /* edges / down / release semantics (corpus formula cur && !prev). */
    in.prev['a'] = 0;
    in.cur['a'] = 1;
    CHECK(input_key_down(&in, 'a') && input_key_edge(&in, 'a'), "edge down");
    CHECK(!input_key_released(&in, 'a'), "not released");
    in.prev['a'] = 1; /* held last frame */
    in.cur['a'] = 0;
    CHECK(input_key_released(&in, 'a'), "release edge");
    CHECK(!input_key_edge(&in, 'a'), "no edge after release");
    CHECK(!input_key_edge(&in, 'b'), "no edge untouched");
    CHECK(input_key_edge(&in, 999) == 0 && input_key_down(&in, -1) == 0,
          "bounds guard");

    /* KeyToAscii: no-HKL is exactly a-z -> A-Z, else passthrough. */
    CHECK(input_key_to_ascii(&in, 'a') == 'A', "ascii a");
    CHECK(input_key_to_ascii(&in, 'z') == 'Z', "ascii z");
    CHECK(input_key_to_ascii(&in, '0') == '0', "ascii digit passthrough");
    CHECK(input_key_to_ascii(&in, 0x7b) == 0x7b, "ascii out-of-range passthrough");

    /* text pump: edges become uppercased chars; context filter blocks. */
    InputState t = {};
    input_reset(&t);
    t.cur['h'] = 1;
    t.cur['i'] = 1;
    t.cur['q'] = 1;
    t.context_filter['q'] = 1; /* blocked */
    char buf[16] = { 0 };
    int  len = 0;
    input_text_pump(&t, buf, (int)sizeof(buf), &len, 1);
    CHECK(len == 2 && memcmp(buf, "HI", 2) == 0, "text pump chars");
    CHECK(t.cur['h'] == 0 && t.cur['i'] == 0, "text pump clears cur");
    CHECK(t.mirror['h'] == 1 && t.mirror['i'] == 1, "text pump mirror");

    /* text pump with no active target still mirrors + clears. */
    InputState m = {};
    input_reset(&m);
    m.cur['x'] = 1;
    input_mirror_and_clear(&m);
    CHECK(m.mirror['x'] == 1 && m.cur['x'] == 0, "mirror_and_clear");

    /* corner-dwell quit machine (findings 04/05 QuitPoll; slot per 22). */
    InputState q = {};
    input_reset(&q);
    int rect[4] = { 0, 0, 3, 3 };       /* corner zone */
    int qw = 6, qh = 6;
    /* dwell inside the corner (slot 1), then leave past the edge. */
    CHECK(input_quit_dwell(&q, 1, 1, 1, rect[0], rect[1], rect[2], rect[3],
                           qw, qh, 1) == 0, "dwell enter only, no quit");
    CHECK(q.dwell_enter[1] == 1, "dwell enter counter");
    CHECK(input_quit_dwell(&q, 1, 10, 10, rect[0], rect[1], rect[2], rect[3],
                           qw, qh, 0) == 1, "dwell -> quit when !keep_running");
    CHECK(q.dwell_leave[1] == 1, "dwell leave counter");
    CHECK(input_quit_dwell(&q, 1, 10, 10, rect[0], rect[1], rect[2], rect[3],
                           qw, qh, 1) == 0, "no quit when keep_running");
    /* never dwelled -> never quits. */
    InputState q2 = {};
    input_reset(&q2);
    CHECK(input_quit_dwell(&q2, 0, 10, 10, 0, 0, 3, 3, qw, qh, 0) == 0,
          "no dwell, no quit");

    /* device capture (frame-loop writer + canonical codes from platform.h). */
    InputState c = {};
    input_reset(&c);
    PlatformEvent evs[4] = {
        { PLATFORM_EVENT_KEYDOWN, 'a', 0, 0, 0 },
        { PLATFORM_EVENT_KEYDOWN, PLATFORM_KEY_RIGHT, 0, 0, 0 },
        { PLATFORM_EVENT_MOUSE, 0, 300, 200, 1 },
        { PLATFORM_EVENT_KEYUP, PLATFORM_KEY_RIGHT, 0, 0, 0 },
    };
    input_capture_events(&c, evs, 4);
    CHECK(c.cur['a'] == 1, "capture: keydown ascii");
    CHECK(c.cur[PLATFORM_KEY_RIGHT] == 0, "capture: keyup clears");
    CHECK(c.mouse_x == 300 && c.mouse_y == 200, "capture: mouse");
    CHECK(c.mouse_press == 1, "capture: mouse press edge latched");
    CHECK(input_key_edge(&c, 'a'), "capture: edge on fresh prev");

    /* Held level (slider drag): motion carrying buttons down sets it, and a
     * release clears it — the press *edge* latch stays independent. */
    PlatformEvent held1 = { PLATFORM_EVENT_MOUSE, 0, 50, 60, 0, 1 };
    input_capture_events(&c, &held1, 1);
    CHECK(c.mouse_x == 50 && c.mouse_down == 1, "capture: held motion sets level");
    PlatformEvent rel = { PLATFORM_EVENT_MOUSE, 0, 50, 60, 0, 0 };
    input_capture_events(&c, &rel, 1);
    CHECK(c.mouse_down == 0, "capture: release clears level");

    /* text pump with clear=0 keeps cur for MainSimStep (frame-loop model). */
    InputState p = {};
    input_reset(&p);
    p.cur['z'] = 1;
    char pb[8] = { 0 };
    int  plen = 0;
    input_text_pump(&p, pb, (int)sizeof(pb), &plen, 0);
    CHECK(plen == 1 && pb[0] == 'Z', "pump noclear: char uppercased");
    CHECK(p.cur['z'] == 1, "pump noclear: cur preserved");
    CHECK(p.mirror['z'] == 1, "pump noclear: mirror set");

    /* two-frame edge flow (capture -> consume -> frame boundary). */
    InputState f = {};
    input_reset(&f);
    PlatformEvent f1 = { PLATFORM_EVENT_KEYDOWN, PLATFORM_KEY_UP, 0, 0, 0 };
    input_capture_events(&f, &f1, 1);
    CHECK(input_key_edge(&f, PLATFORM_KEY_UP), "flow: frame1 edge");
    input_begin_frame(&f); /* frame boundary: cur -> prev, cur cleared */
    CHECK(!input_key_edge(&f, PLATFORM_KEY_UP), "flow: no edge after boundary (released)");
    PlatformEvent f2 = { PLATFORM_EVENT_KEYDOWN, PLATFORM_KEY_UP, 0, 0, 0 };
    input_capture_events(&f, &f2, 1);
    CHECK(!input_key_edge(&f, PLATFORM_KEY_UP), "flow: held key does not re-edge");
    PlatformEvent f3 = { PLATFORM_EVENT_KEYUP, PLATFORM_KEY_UP, 0, 0, 0 };
    input_capture_events(&f, &f3, 1);
    input_begin_frame(&f); /* boundary after release: prev := cur(all clear) */
    PlatformEvent f4 = { PLATFORM_EVENT_KEYDOWN, PLATFORM_KEY_UP, 0, 0, 0 };
    input_capture_events(&f, &f4, 1);
    CHECK(input_key_edge(&f, PLATFORM_KEY_UP), "flow: press-after-release edges");

    /* The flag-8 leaves (2026-10-24): one bit over the three DAT tables,
     * settable and readable through the corpus's own leaves. */
    {
        CHECK(input_entry_flag8_get(3) == 0u, "the entry flags start zero");
        input_entry_flag8_set_clear(3, 1);
        CHECK(input_entry_flag8_get(3) == 8u, "the raise lands");
        CHECK(input_entry_flag8_get(4) == 0u, "the neighbour entry holds");
        input_entry_flag8_set_clear(3, 0);
        CHECK(input_entry_flag8_get(3) == 0u, "the clear lands");

        CHECK(player_input_flag8_get(1, 7) == 0u
                  && player_input_flag8_get(4, 7) == 0u,
              "both player tables start zero");
        player_input_flag8_set_clear(1, 7, 1);
        CHECK(player_input_flag8_get(1, 7) == 8u
                  && player_input_flag8_get(4, 7) == 0u,
              "kind 1 raises its table only");
        player_input_flag8_set_clear(4, 7, 1);
        CHECK(player_input_flag8_get(1, 7) == 8u
                  && player_input_flag8_get(4, 7) == 8u,
              "the kinds stay independent");
        player_input_flag8_set_clear(1, 7, 0);
        CHECK(player_input_flag8_get(1, 7) == 0u
                  && player_input_flag8_get(4, 7) == 8u,
              "the clear spares the other kind");

        /* Unknown kinds and far indices answer zero / skip (deviation). */
        player_input_flag8_set_clear(2, 5, 1);
        CHECK(player_input_flag8_get(2, 5) == 0u, "kind 2 never fires");
        CHECK(input_entry_flag8_get(1 << 20) == 0u, "the far index reads zero");
        player_input_flag8_set_clear(1, 1 << 20, 1);
        CHECK(player_input_flag8_get(1, 1 << 20) == 0u,
              "the far player index is guarded");
    }

    if (s_failures == 0) {
        printf("input_test: PASS\n");
        return 0;
    }
    printf("input_test: %d failure(s)\n", s_failures);
    return 1;
}