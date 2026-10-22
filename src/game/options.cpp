// SPDX-License-Identifier: LGPL-3.0-or-later
/* Game layer — functional option store (issue #85). See options.hpp. */
#include <stddef.h>
#include <string.h>

#include "options.hpp"

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
    static const char* const names[GAME_OPT_COUNT] = {
        "gamespeed", "sfxvolume", "musicvolume", "brightness", "helpwindow",
        "walldetail", "music", "soundeffects", "automaticgamespeed",
    };
    if (label == NULL) {
        return -1;
    }
    for (int i = 0; i < GAME_OPT_COUNT; i++) {
        if (s_optname_eq(label, names[i])) {
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
    switch (id) {
    case OPT_GAME_SPEED:   *lo = 0; *hi = 5;  break;
    case OPT_SFX_VOLUME:   *lo = 0; *hi = 10; break;
    case OPT_MUSIC_VOLUME: *lo = 0; *hi = 10; break;
    case OPT_BRIGHTNESS:   *lo = 0; *hi = 10; break;
    default:               *lo = 0; *hi = 1;  break; /* cycles */
    }
    return 1;
}

int options_default(int id)
{
    switch (id) {
    case OPT_GAME_SPEED:   return OPT_DFLT_GAME_SPEED;
    case OPT_SFX_VOLUME:   return OPT_DFLT_SFX_VOLUME;
    case OPT_MUSIC_VOLUME: return OPT_DFLT_MUSIC_VOLUME;
    case OPT_BRIGHTNESS:   return OPT_DFLT_BRIGHTNESS;
    default:               return OPT_DFLT_CYCLE;
    }
}

static int* option_field(GameOptions* o, int id)
{
    if (o == NULL || id < 0 || id >= GAME_OPT_COUNT) {
        return NULL;
    }
    switch (id) {
    case OPT_GAME_SPEED:     return &o->game_speed;
    case OPT_SFX_VOLUME:     return &o->sfx_volume;
    case OPT_MUSIC_VOLUME:   return &o->music_volume;
    case OPT_BRIGHTNESS:     return &o->brightness;
    case OPT_HELP_WINDOW:    return &o->help_window;
    case OPT_WALL_DETAIL:    return &o->wall_detail;
    case OPT_MUSIC_ONOFF:    return &o->music_onoff;
    case OPT_SFX_ONOFF:      return &o->sfx_onoff;
    case OPT_AUTO_SPEED:     return &o->auto_game_speed;
    default:                 return NULL;
    }
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
