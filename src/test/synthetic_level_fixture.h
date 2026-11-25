// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef LRR_SYNTHETIC_LEVEL_FIXTURE_H
#define LRR_SYNTHETIC_LEVEL_FIXTURE_H
namespace synthetic_fixture {
constexpr unsigned W = 8, H = 8;
bool write_text(const char* path, const char* text);
bool write_map(const char* path, unsigned width, unsigned height,
               unsigned (*value)(unsigned x, unsigned y));
bool write_bmp(const char* path, unsigned width, unsigned height);
bool write_triangle(const char* path);
// One animated null, two keyframes, no mesh or sound dependencies.
bool write_null_scene(const char* path);
bool make_fixture(const char* directory);
// Two abstract maps: GridA (existing 8x8 grid), GridB (10x9 grid).
// Writes Data/transition.cfg; pass "transition.cfg" to boot_game.
constexpr unsigned TransitionW = 10, TransitionH = 9;
bool make_transition_fixture(const char* directory);
bool write_transition_surface(); // Restore GridB's surface file after a negative test.

bool initialise_headless(const char* programName);
bool boot_game();
bool boot_game(const char* configPath);
}
#endif
