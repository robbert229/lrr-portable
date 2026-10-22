// SPDX-License-Identifier: LGPL-3.0-or-later
/* Game layer — functional option store (issue #85).
 *
 * The shipped Options/Advanced `Slider`/`Cycle` controls are display state on
 * the menu model; the *effective* value lives here, on the GameContext. The
 * menu seeds its widgets from this store when it builds or re-enters (so
 * re-opening Options never resets a changed control), and writes back every
 * adjustment it takes — the menu-to-runtime seam both ways.
 *
 * Defaults and ranges follow the corpus:
 *   OptionsLoadOrDefault @ 0x416940 — sets exactly the triple at
 *     DAT_00557030/34/3c: speed 1, SFX 8, music 5; it assigns no brightness
 *     value (the same attribution previously stood for brightness too —
 *     corrected here). Brightness 8 and Help_Window Off are port-chosen
 *     neutral defaults (cycles default to option 0);
 *   ScrollToSliderIndexStore @ 0x414f90 — the speed *index* 0..5 the slider
 *     sets, derived from the continuous time scale (bands 0.667/1.0/1.333/
 *     1.667/2.0); options_time_scale() is that inverse: index -> dt factor.
 *
 * Per-slot persistence mirrors the corpus's slot option words (SavReadSlot/
 * OptionsApplyFromSlot @ 0x417a90/0x415150): the active save context's
 * header carries them, so save/reload restores them; a fresh context uses
 * the defaults above.
 *
 * Discipline (ADR 0003/0004): plain C-shaped C++, no OS headers, no STL.
 */
#ifndef LEGORRR_GAME_OPTIONS_HPP
#define LEGORRR_GAME_OPTIONS_HPP

#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

#define GAME_OPT_COUNT 9

/* Control ids — the shipped Options/Advanced banners' order. */
#define OPT_GAME_SPEED     0   /* Slider Game_Speed           0..5  */
#define OPT_SFX_VOLUME     1   /* Slider SFX_Volume           0..10 */
#define OPT_MUSIC_VOLUME   2   /* Slider Music_Volume         0..10 */
#define OPT_BRIGHTNESS     3   /* Slider Brightness           0..10 */
#define OPT_HELP_WINDOW    4   /* Cycle Help_Window           0..1  */
#define OPT_WALL_DETAIL    5   /* Cycle Wall_Detail           0..1  */
#define OPT_MUSIC_ONOFF    6   /* Cycle Music (On:Off)        0..1  */
#define OPT_SFX_ONOFF      7   /* Cycle Sound_Effects (On:Off) 0..1 */
#define OPT_AUTO_SPEED     8   /* Cycle Automatic_Game_Speed  0..1  */

/* Corpus triple OptionsLoadOrDefault @ 0x416940: speed 1, SFX 8, music 5.
 * Brightness 8 / Help_Window Off: port-chosen neutrals (no corpus default
 * exists — the decompile assigns only the DAT triple). Cycles open at
 * option 0 (the shipped lists are High/On first). */
#define OPT_DFLT_GAME_SPEED    1
#define OPT_DFLT_SFX_VOLUME    8
#define OPT_DFLT_MUSIC_VOLUME  5
#define OPT_DFLT_BRIGHTNESS    8   /* port neutral, not OptionsLoadOrDefault */
#define OPT_DFLT_CYCLE         0

typedef struct GameOptions {
    int game_speed;      /* slider index 0..5 (1 = normal speed)   */
    int sfx_volume;      /* slider 0..10 (OptionsLoadOrDefault 8)  */
    int music_volume;    /* slider 0..10 (corpus default 5)        */
    int brightness;      /* slider 0..10 (port default 8)        */
    int help_window;     /* 0 Off / 1 On                           */
    int wall_detail;     /* 0 High / 1 Low                         */
    int music_onoff;     /* 0 On / 1 Off (the shipped cycle order) */
    int sfx_onoff;       /* 0 On / 1 Off                           */
    int auto_game_speed; /* 0 On / 1 Off                           */
} GameOptions;

/* Map a shipped control banner ("Game_Speed", "SFX_Volume", "Brightness",
 * "Help_Window", …, case- and underscore-insensitive) to its id, or -1 for
 * unknown labels (menu rows that are not options controls). */
int options_id_for_label(const char* label);

/* id -> [lo,hi], the shipped config range. 1 when the id is known. */
int options_range(int id, int* lo, int* hi);

/* id -> the documented default (OPT_DFLT_*). */
int options_default(int id);

/* Load `o` with the documented defaults. */
void options_defaults(GameOptions* o);

/* Get/set by id. `options_set` clamps to the shipped range and silently
 * ignores unknown ids; `options_get` yields 0 for unknown ids. */
int  options_get(const GameOptions* o, int id);
void options_set(GameOptions* o, int id, int v);

/* Label-driven wrappers — the menu seam: write a slider/cycle's value into
 * the store (1 when the label is a known option control), read the effective
 * value back out (1 when known). */
int options_set_label(GameOptions* o, const char* label, int v);
int options_get_label(const GameOptions* o, const char* label, int* out);

/* dt multiplier for the speed slider index (ScrollToSliderIndexStore's
 * continuous counterpart): {0.5, 1.0, 1.25, 1.5, 1.75, 2.0}; index 1 is the
 * default, 1.0 = normal pacing. */
float options_time_scale(const GameOptions* o);

struct EngineContext;

/* The frame loop's per-frame speed connection: stamps the store's factor
 * into the engine context's time scale (EngineTick's subsystem tickers
 * read it back through EngineTimeScale) and returns the dt the frame loop
 * hands to the game-layer tickers (ai_tick / objective_tick). `ctx` may be
 * NULL when no engine context exists (menu-only frames). */
float options_scale_frame(const GameOptions* o, struct EngineContext* ctx,
                          float dt);

/* Master audio gain 0..1 for the store — SFX_Volume / 10, the same mapping
 * the live menu adjustment feeds to audio_set_volume. Applied at the audio
 * start so settings restored from the active save context take effect
 * immediately (issue #85), not only after another menu nudge. */
float options_audio_gain(const GameOptions* o);

/* --- save-context serialisation -----------------------------------------
 * 48-byte option block stamped into the `.sav` slot header: magic 'GOPS', u32
 * version (1), u32 count (GAME_OPT_COUNT), then the values by id.
 * options_save_header writes the block; options_load_header restores it and
 * returns 1 when the magic matches (absent/garbage: 0, the caller keeps the
 * defaults). */
void options_save_header(const GameOptions* o, uint8_t* blk);
int  options_load_header(GameOptions* o, const uint8_t* blk);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_OPTIONS_HPP */
