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
    input_text_pump(&t, buf, (int)sizeof(buf), &len);
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

    if (s_failures == 0) {
        printf("input_test: PASS\n");
        return 0;
    }
    printf("input_test: %d failure(s)\n", s_failures);
    return 1;
}