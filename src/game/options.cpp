// SPDX-License-Identifier: LGPL-3.0-or-later
/* Game layer — functional option store (issue #85). See options.hpp. */
#include <stddef.h>
#include <string.h>

#include "options.hpp"
#include "savegame.hpp"

#include "gods98/engine.hpp"
#include "gods98/heap.hpp"

/* One descriptor per control: banner name (normalized for matching), the
 * shipped config range, the documented default, and the store field. The
 * per-id accessors all read this table — no per-control switches. */
static const struct {
    const char* name;
    int         lo;
    int         hi;
    int         dflt;
    size_t      off;
} s_opts[GAME_OPT_COUNT] = {
    { "gamespeed",          0, 5,  OPT_DFLT_GAME_SPEED,
      offsetof(GameOptions, game_speed) },
    { "sfxvolume",          0, 10, OPT_DFLT_SFX_VOLUME,
      offsetof(GameOptions, sfx_volume) },
    { "musicvolume",        0, 10, OPT_DFLT_MUSIC_VOLUME,
      offsetof(GameOptions, music_volume) },
    { "brightness",         0, 10, OPT_DFLT_BRIGHTNESS,
      offsetof(GameOptions, brightness) },
    { "helpwindow",         0, 1,  OPT_DFLT_CYCLE,
      offsetof(GameOptions, help_window) },
    { "walldetail",         0, 1,  OPT_DFLT_CYCLE,
      offsetof(GameOptions, wall_detail) },
    { "music",              0, 1,  OPT_DFLT_CYCLE,
      offsetof(GameOptions, music_onoff) },
    { "soundeffects",       0, 1,  OPT_DFLT_CYCLE,
      offsetof(GameOptions, sfx_onoff) },
    { "automaticgamespeed", 0, 1,  OPT_DFLT_CYCLE,
      offsetof(GameOptions, auto_game_speed) },
};

static int s_lower(int c) { return (c >= 'A' && c <= 'Z') ? c + 0x20 : c; }

/* Case- and separator-insensitive exact match (the banner may arrive as
 * "Game Speed", "Game_Speed" or "game speed"). */
static int s_optname_eq(const char* a, const char* b)
{
    if (a == NULL || b == NULL) {
        return 0;
    }
    while (*a != '\0' && *b != '\0') {
        char ca = s_lower(*a);
        char cb = s_lower(*b);
        if (ca == '_' || ca == ' ') {
            ++a;
            continue;
        }
        if (cb == '_' || cb == ' ') {
            ++b;
            continue;
        }
        if (ca != cb) {
            return 0;
        }
        ++a; ++b;
    }
    while (*a == '_' || *a == ' ') {
        ++a;
    }
    while (*b == '_' || *b == ' ') {
        ++b;
    }
    return *a == '\0' && *b == '\0';
}

int options_id_for_label(const char* label)
{
    if (label == NULL) {
        return -1;
    }
    for (int i = 0; i < GAME_OPT_COUNT; i++) {
        if (s_optname_eq(label, s_opts[i].name)) {
            return i;
        }
    }
    return -1;
}

int options_range(int id, int* lo, int* hi)
{
    if (id < 0 || id >= GAME_OPT_COUNT) {
        return 0;
    }
    /* Sliders carry the shipped Lego.cfg limits; cycles are two-option. */
    if (lo != NULL) {
        *lo = s_opts[id].lo;
    }
    if (hi != NULL) {
        *hi = s_opts[id].hi;
    }
    return 1;
}

int options_default(int id)
{
    if (id < 0 || id >= GAME_OPT_COUNT) {
        return OPT_DFLT_CYCLE;
    }
    return s_opts[id].dflt;
}

static int* option_field(GameOptions* o, int id)
{
    if (o == NULL || id < 0 || id >= GAME_OPT_COUNT) {
        return NULL;
    }
    return (int*)((char*)o + s_opts[id].off);
}

void options_defaults(GameOptions* o)
{
    if (o == NULL) {
        return;
    }
    for (int i = 0; i < GAME_OPT_COUNT; i++) {
        int* f = option_field(o, i);
        if (f != NULL) {
            *f = options_default(i);
        }
    }
}

int options_get(const GameOptions* o, int id)
{
    const GameOptions* co = (const GameOptions*)o;
    int* f = option_field((GameOptions*)co, id);
    return f != NULL ? *f : 0;
}

void options_set(GameOptions* o, int id, int v)
{
    int lo = 0, hi = 0;
    int* f = option_field(o, id);
    if (f == NULL || !options_range(id, &lo, &hi)) {
        return;
    }
    if (v < lo) {
        v = lo;
    }
    if (v > hi) {
        v = hi;
    }
    *f = v;
}

int options_set_label(GameOptions* o, const char* label, int v)
{
    int id = options_id_for_label(label);
    if (id < 0) {
        return 0;
    }
    options_set(o, id, v);
    return 1;
}

int options_get_label(const GameOptions* o, const char* label, int* out)
{
    int id = options_id_for_label(label);
    if (id < 0 || out == NULL) {
        return 0;
    }
    *out = options_get(o, id);
    return 1;
}

float options_time_scale(const GameOptions* o)
{
    static const float k_scales[6] = { 0.5f, 1.0f, 1.25f, 1.5f, 1.75f, 2.0f };
    int v = options_get(o, OPT_GAME_SPEED);
    if (v < 0) {
        v = 0;
    }
    if (v > 5) {
        v = 5;
    }
    return k_scales[v];
}

float options_scale_frame(const GameOptions* o, EngineContext* ctx, float dt)
{
    float scale = options_time_scale(o);
    if (ctx != NULL) {
        /* EngineTick's subsystem tickers read this back via EngineTimeScale. */
        ctx->time_scale = scale;
    }
    return dt * scale;
}

float options_audio_gain(const GameOptions* o)
{
    int v = options_get(o, OPT_SFX_VOLUME); /* clamps to 0..10 */
    if (v < 0) {
        v = 0;
    }
    if (v > 10) {
        v = 10;
    }
    return (float)v / 10.0f;
}

/* --- save-context serialisation ----------------------------------------- */

static void s_put_u32(uint8_t* p, uint32_t v)
{
    p[0] = (uint8_t)(v);
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

static uint32_t s_get_u32(const uint8_t* p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

#define OPT_HDR_MAGIC 0x53504F47u /* 'G''O''P''S' little-endian */
#define OPT_HDR_VERSION 1u
#define OPT_HDR_BYTES (12 + 4 * GAME_OPT_COUNT)

void options_save_header(const GameOptions* o, uint8_t* blk)
{
    if (o == NULL || blk == NULL) {
        return;
    }
    memset(blk, 0, OPT_HDR_BYTES);
    s_put_u32(blk + 0, OPT_HDR_MAGIC);
    s_put_u32(blk + 4, OPT_HDR_VERSION);
    s_put_u32(blk + 8, GAME_OPT_COUNT);
    for (int i = 0; i < GAME_OPT_COUNT; i++) {
        s_put_u32(blk + 12 + 4 * i, (uint32_t)options_get(o, i));
    }
}

int options_load_header(GameOptions* o, const uint8_t* blk)
{
    if (o == NULL || blk == NULL || s_get_u32(blk) != OPT_HDR_MAGIC) {
        return 0;
    }
    uint32_t n = s_get_u32(blk + 8);
    if (n > GAME_OPT_COUNT) {
        n = GAME_OPT_COUNT;
    }
    for (uint32_t i = 0; i < n; i++) {
        options_set(o, (int)i, (int)s_get_u32(blk + 12 + 4 * i));
    }
    return 1;
}

/* OptionsWriteToSlot (FUN_00416910) — see options.hpp. */
void options_write_to_slot(GameContext* ctx)
{
    char* block = (char*)sav_build_ctx(ctx);
    if (block == NULL) {
        return;
    }
    /* corpus: DAT_00557034 -> +8, DAT_00557030 -> +12, DAT_0055703c -> +16 */
    const uint32_t sfx = (uint32_t)ctx->options.sfx_volume;
    const uint32_t speed = (uint32_t)ctx->options.game_speed;
    const uint32_t music = (uint32_t)ctx->options.music_volume;
    __builtin_memcpy(block + 8, &sfx, sizeof sfx);
    __builtin_memcpy(block + 12, &speed, sizeof speed);
    __builtin_memcpy(block + 16, &music, sizeof music);
}

/* OptionsContextSixFieldSet (FUN_00417f20) — see options.hpp. */
void options_context_six_field_set(struct GameContext* ctx,
                                   const uint32_t* src)
{
    char* block = (char*)sav_build_ctx(ctx);
    if (block == NULL || src == NULL) {
        return;
    }
    for (int i = 0; i < 6; ++i) {
        const uint32_t v = src[i];
        __builtin_memcpy(block + 0x1c + (size_t)i * sizeof(v), &v, sizeof v);
    }
}

/* --- the scroll speed index band (see options.hpp) ----------------------- */
int scroll_to_slider_index(void)
{
    const double v = (double)scroll_scale_peek(); /* corpus reads the _DAT
                                                   * alias of its scale word */
    if (v < 0.6666667) {
        return 0;
    }
    if (v < 1.0) {
        return 1;
    }
    if (v < 1.3333334) {
        return 2;
    }
    if (v < 1.6666666) {
        return 3;
    }
    return (2.0 <= v) ? 5 : 4; /* corpus shape: NaN skips both tests, 4 */
}

void scroll_slider_refresh(struct GameContext* ctx)
{
    if (ctx == NULL) {
        return; /* deviation: the corpus stores the global blindly */
    }
    ctx->options.game_speed = scroll_to_slider_index();
}
