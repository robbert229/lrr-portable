// SPDX-License-Identifier: LGPL-3.0-or-later
/* Game layer — functional option store (issue #85). See options.hpp. */
#include <stddef.h>
#include <string.h>

#include "options.hpp"
#include "savegame.hpp"
#include "frontend.hpp"

#include "gods98/engine.hpp"
#include "gods98/heap.hpp"
#include "gods98/audio.hpp"
#include "gods98/string.hpp"

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

/* Case- and separator-insensitive exact match (the banner may arrive as
 * "Game Speed", "Game_Speed" or "game speed"). */
static int s_optname_eq(const char* a, const char* b)
{
    if (a == NULL || b == NULL) {
        return 0;
    }
    for (;;) {
        a += strspn(a, " _");
        b += strspn(b, " _");
        if (*a == '\0' || *b == '\0') return *a == *b;

        const size_t a_length = strcspn(a, " _");
        const size_t b_length = strcspn(b, " _");
        if (a_length != b_length
            || strnicmp_lite(a, b, (int)a_length) != 0) {
            return 0;
        }
        a += a_length;
        b += b_length;
    }
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

/* The corpus's fallback assigns only this effective option triple. */
static void options_load_corpus_defaults(GameOptions* o)
{
    if (o == NULL) {
        return;
    }
    o->game_speed = OPT_DFLT_GAME_SPEED;
    o->sfx_volume = OPT_DFLT_SFX_VOLUME;
    o->music_volume = OPT_DFLT_MUSIC_VOLUME;
}

/* OptionsLoadOrDefault (FUN_00416940): reset the front-end selection words
 * when requested, then preserve the active context's loaded option values or
 * seed the corpus's {speed, SFX, music} defaults. The port keeps the selected
 * context's persisted values in GameContext::options instead of DAT fields. */
// FUNCTION: LEGORR 0x00416940
void options_load_or_default(struct GameContext* ctx, int load_options,
                             int reset_frontend)
{
    if (reset_frontend != 0) {
        frontend_flow_word_set(0, 0);
        frontend_flow_word_set(1, -1);
        frontend_flow_word_set(2, -1);
        frontend_flow_word_set(3, 0);
    }
    if (load_options != 0 && ctx != NULL) {
        char* block = (char*)sav_build_ctx(ctx);
        if (block == NULL) {
            options_load_corpus_defaults(&ctx->options);
        } else {
            uint32_t sfx;
            uint32_t speed;
            uint32_t music;
            __builtin_memcpy(&sfx, block + 8, sizeof(sfx));
            __builtin_memcpy(&speed, block + 12, sizeof(speed));
            __builtin_memcpy(&music, block + 16, sizeof(music));
            ctx->options.sfx_volume = (int)sfx;
            ctx->options.game_speed = (int)speed;
            ctx->options.music_volume = (int)music;
        }
    }
}

void options_defaults(GameOptions* o)
{
    if (o == NULL) {
        return;
    }
    options_load_corpus_defaults(o);
    for (int i = 3; i < GAME_OPT_COUNT; i++) {
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
    return audio_sfx_volume_gain(v);
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
// FUNCTION: LEGORR 0x416910
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
// FUNCTION: LEGORR 0x417f20
void options_context_six_field_set(struct GameContext* ctx,
                                   const uint32_t* src)
{
    char* block = (char*)sav_build_ctx(ctx);
    if (block == NULL || src == NULL) {
        return;
    }
    uint8_t* dst = (uint8_t*)block + 0x1c;
    const uint32_t* p = src;
    for (int i = 6; i != 0; --i) {
        __builtin_memcpy(dst, p, sizeof(*p));
        dst += sizeof(*p);
        ++p;
    }
}

/* --- the slot-context options window (see options.hpp) ---------------------
 * The corpus's 0xb8-byte DAT_00557080 block (stride 0xb8, active slot only)
 * has no ported owner, so the 46-dword window and the owned resource word
 * land as zero-init own storage (the unowned-DAT convention; the corpus's
 * 32-bit pointer word is widened to void* for the port's LP64 addresses,
 * the fixed-record-slot precedent). */
#define OPTIONS_SLOT_WINDOW_DWORDS 46
static uint32_t s_options_slot_window[OPTIONS_SLOT_WINDOW_DWORDS];
static void* s_options_slot_resource;

/* OptionsContextCopyAndReplaceResource (FUN_004180c0) — see options.hpp. */
// FUNCTION: LEGORR 0x4180c0
void options_context_copy_and_replace_resource(GameContext* ctx,
                                               const uint32_t* source)
{
    if (sav_build_ctx(ctx) == NULL || source == NULL) {
        return; /* deviation: the corpus strides its context blindly */
    }
    if (s_options_slot_resource != NULL) {
        engine_heap_free(s_options_slot_resource);
    }
    __builtin_memcpy(s_options_slot_window, source,
                     sizeof s_options_slot_window);
    s_options_slot_resource = (void*)(uintptr_t)source[13];
    s_options_slot_window[13] = source[13];
}

void options_slot_window_install(const uint32_t* block, void* resource)
{
    if (block != NULL) {
        __builtin_memcpy(s_options_slot_window, block,
                         sizeof s_options_slot_window);
    } else {
        memset(s_options_slot_window, 0, sizeof s_options_slot_window);
    }
    s_options_slot_resource = resource;
}

uint32_t options_slot_window_dword_peek(int index)
{
    if (index < 0 || index >= OPTIONS_SLOT_WINDOW_DWORDS) {
        return 0u; /* deviation: the corpus strides the index blindly */
    }
    return s_options_slot_window[index];
}

void* options_slot_resource_peek(void)
{
    return s_options_slot_resource;
}

/* --- the scroll speed index band (see options.hpp) ----------------------- */
// FUNCTION: LEGORR 0x00415030
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

/* ScrollToSliderIndexStore (FUN_00414f90) stores the same speed band in the
 * context-owned option word; the corpus writes its module global. */
// FUNCTION: LEGORR 0x414f90
void options_scroll_index_store(struct GameContext* ctx)
{
    if (ctx == NULL) {
        return; /* deviation: the corpus stores the global blindly */
    }
    const double v = (double)scroll_scale_peek();
    if (v < 0.6666667) {
        ctx->options.game_speed = 0;
        return;
    }
    if (v < 1.0) {
        ctx->options.game_speed = 1;
        return;
    }
    if (v < 1.3333334) {
        ctx->options.game_speed = 2;
        return;
    }
    if (v < 1.6666666) {
        ctx->options.game_speed = 3;
        return;
    }
    ctx->options.game_speed = 4;
    if (2.0 <= v) {
        ctx->options.game_speed = 5;
    }
}

// FUNCTION: LEGORR 0x00415360
void scroll_slider_refresh(struct GameContext* ctx)
{
    if (ctx != NULL) {
        ctx->options.game_speed = scroll_to_slider_index();
    }
}
