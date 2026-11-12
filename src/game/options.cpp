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
#include "options_for_frontend_unit.hpp"

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
    if (a == NULL || b == NULL) return 0;
    for (;;) {
        unsigned char ca, cb;
        while (*a == ' ' || *a == '_') ++a;
        while (*b == ' ' || *b == '_') ++b;
        ca = (unsigned char)*a++;
        cb = (unsigned char)*b++;
        if (ca >= 'A' && ca <= 'Z') ca = (unsigned char)(ca + ('a' - 'A'));
        if (cb >= 'A' && cb <= 'Z') cb = (unsigned char)(cb + ('a' - 'A'));
        if (ca != cb) return 0;
        if (ca == 0) return 1;
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
void options_load_corpus_defaults(GameOptions* o)
{
    if (o == NULL) {
        return;
    }
    o->game_speed = OPT_DFLT_GAME_SPEED;
    o->sfx_volume = OPT_DFLT_SFX_VOLUME;
    o->brightness = OPT_RETAIL_BRIGHTNESS_FALLBACK;
}

int s_options_frontend_selection = -1;

void options_frontend_selection_set(int selected_slot)
{
    s_options_frontend_selection = selected_slot;
}

/* Typed adapter for the target's two-argument ABI. */
void options_load_or_default(struct GameContext* ctx, int load_options,
                             int reset_frontend)
{
    if (ctx != NULL) save_context_bind(ctx);
    options_load_or_default_target(load_options, reset_frontend);
}

void options_defaults(GameOptions* o)
{
    if (o == NULL) {
        return;
    }
    options_load_corpus_defaults(o);
    o->music_volume = OPT_DFLT_MUSIC_VOLUME;
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
    /* Publish an actual option adjustment once, through the same owner as
     * SetGameSpeed. The slider index zero still means half speed. */
    if (id == OPT_GAME_SPEED) {
        scroll_scale_set_and_clamp(options_time_scale(o));
    }
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
    /* This is a reader, not another speed producer. Reapplying the positive
     * slider every frame bypasses authored SetGameSpeed 0 race holds. */
    (void)o;
    float scale = EngineTimeScale();
    if (ctx != NULL) {
        /* Keep the portable context mirror useful to local observers. */
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

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
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
#endif /* !LEGORRR_BACKEND_WINAPI */

int options_load_header(GameOptions* o, const uint8_t* blk)
{
    if (o == NULL || blk == NULL || s_get_u32(blk) != OPT_HDR_MAGIC ||
        s_get_u32(blk + 4) != OPT_HDR_VERSION) {
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

uint32_t s_options_slot_window[OPTIONS_SLOT_WINDOW_DWORDS];
void* s_options_slot_resource;

void options_write_to_slot(GameContext* ctx)
{
    if (ctx == NULL) return;
    save_context_bind(ctx);
    options_write_to_slot_target();
}

void options_apply_from_slot(GameContext* ctx)
{
    if (ctx == NULL || ctx->save_slot < 0 ||
        ctx->active_slot != ctx->save_slot) return;
    audio_menu_sfx_volume_set(ctx->saved_sfx_volume);
    ctx->options.brightness = ctx->saved_brightness;
    audio_menu_volume_set(ctx->saved_music_volume);
}

/* Typed OptionsContextSixFieldSet adapter. */
void options_context_six_field_set(struct GameContext* ctx,
                                   const uint32_t* src)
{
    if (ctx == NULL || src == NULL) return;
    save_context_bind(ctx);
    options_context_six_field_set_target(src);
}

/* --- the slot-context options window (see options.hpp) ---------------------
 * The corpus's 0xb8-byte DAT_00557080 block (stride 0xb8, active slot only)
 * has no ported owner, so the 46-dword window and the owned resource word
 * land as zero-init own storage (the unowned-DAT convention; the corpus's
 * 32-bit pointer word is widened to void* for the port's LP64 addresses,
 * the fixed-record-slot precedent). */
/* Typed OptionsContextCopyAndReplaceResource adapter. */
void options_context_copy_and_replace_resource(GameContext* ctx,
                                               const uint32_t* source)
{
    if (ctx == NULL || source == NULL) return;
    save_context_bind(ctx);
    options_context_copy_and_replace_resource_target(source);
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


/* Original-ABI adapter (ADR 0005 step 6) for GAME-021's lego_unit.c: the
 * original FUN_00415150 takes no argument; the port's entry above takes the
 * game context. */
extern "C" GameContext* entity_pool_callback_context_get(void); /* entity.hpp */
// PORT-ONLY: original-ABI adapter for GAME-021 lego_unit.c (options_apply_from_slot, 0x00415150)
extern "C" void options_apply_from_slot_original(void)
{
    options_apply_from_slot(entity_pool_callback_context_get());
}
