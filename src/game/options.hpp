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

/* OptionsLoadOrDefault (FUN_00416940): when `load_options` is nonzero,
 * retain the selected slot's loaded speed/SFX/music triple or apply the
 * corpus defaults if no slot is selected. `reset_frontend` clears the
 * modeled front-end selection words. */
struct GameContext;
void options_load_or_default(struct GameContext* ctx, int load_options,
                             int reset_frontend);
void options_load_or_default_target(int load_options,
                                    int reset_frontend); /* FUN_00416940 */

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

/* Master audio gain for the store — the SFX_Volume index maps through the
 * corpus's nonlinear DirectSound attenuation table (FUN_0047b530), then to
 * a linear amplitude. Applied at audio start so restored settings take
 * effect immediately (issue #85). */
float options_audio_gain(const GameOptions* o);

/* Legacy portable-only GOPS block helpers. Retail `.sav` files instead keep
 * the three tracked option words in the raw slot-context header at +0x08,
 * +0x0c, and +0x10. The loader retains these helpers only to read saves made
 * by earlier portable builds. */
void options_save_header(const GameOptions* o, uint8_t* blk);
int  options_load_header(GameOptions* o, const uint8_t* blk);

#ifdef __cplusplus
}
#endif

/* Typed owner adapter plus the corpus's no-argument target entry. */
void options_write_to_slot(struct GameContext* ctx);
void options_write_to_slot_target(void); /* FUN_00416910 */

/* OptionsContextSixFieldSet (FUN_00417f20, findings 438/372): the target
 * entry takes only `src` and probes the active save context. The typed
 * adapter binds a GameContext before calling that exact-ABI entry. */
void options_context_six_field_set(struct GameContext* ctx,
                                   const uint32_t* src);
void options_context_six_field_set_target(const uint32_t* src);

/* OptionsContextCopyAndReplaceResource (FUN_004180c0): with a save slot
 * open, frees the slot context's owned resource word (the corpus's
 * DAT_00557080+slot*0xb8 dword 0xd through EngineHeapFree), copies the
 * full 46-dword source block over the window, and re-lands the source's
 * dword 0xd. The target takes only the source pointer; the typed adapter
 * binds GameContext first. The corpus's per-slot block has no ported owner,
 * so the window and widened resource ride module-owned sidecar storage. */
void options_context_copy_and_replace_resource(struct GameContext* ctx,
                                               const uint32_t* source);
void options_context_copy_and_replace_resource_target(const uint32_t* source);
void     options_slot_window_install(const uint32_t* block, void* resource);
uint32_t options_slot_window_dword_peek(int index);
void*    options_slot_resource_peek(void); /* FUN_00417f20 */

/* --- scroll speed index band (2026-10-25) --------------------------------
 * ScrollToSliderIndex (FUN_00415030): maps the engine's scroll-scale float
 * (the corpus's _DAT alias, modeled by the engine's peek/set accessors) to
 * a speed-index band — 0 below 0.6666667, 1 below 1.0, 2 below 1.3333334,
 * 3 below 1.6666666, else 4 (5 at or above 2.0). Corpus shape pinned: the
 * comparisons promote the stored float, so a NaN scale fails every test
 * and answers 4. ScrollSliderRefresh (FUN_00415360): stores the band value
 * into the options speed word (the corpus writes the same dword that the
 * port models as GameOptions.game_speed). Deviation: the port threads a
 * GameContext argument; a NULL context stores nothing. */
int  scroll_to_slider_index(void);                    /* FUN_00415030 */
void options_scroll_index_store(struct GameContext* ctx); /* FUN_00414f90 */
void scroll_slider_refresh(struct GameContext* ctx);  /* FUN_00415360 */


#endif /* LEGORRR_GAME_OPTIONS_HPP */
