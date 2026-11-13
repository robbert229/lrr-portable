// SPDX-License-Identifier: LGPL-3.0-or-later
/* LightEffects.c, original translation unit GAME-024 (finding 505), rebuilt
 * in original shape (ADR 0005; finding 594): text 0044c710-0044db50,
 * .rdata 0049e4b0-0049e4cc, literals 004a5184-004a528c, .bss
 * 004eadd8-004eaecc.
 *
 * The level's ambient light effects. Lego.c creates one coloured light
 * under a root container and hands both to LightEffects_Initialise with
 * the light's initial colour. Lego.cfg's `LightEffects` block configures
 * three effects, each enabled when its block loads:
 *   - blink: a random colour step every so often, bounded by how far the
 *     red channel may drift in total (MaxChangeAllowed);
 *   - fade: wait, fade up to a random colour between FadeRGBMin and
 *     FadeRGBMax, hold, and fade back down;
 *   - move: wait, then step the light (relative to its root) in a random
 *     direction, staying within MoveLimit of where it was placed.
 * A dimmer fades the light out while Lego.c (00426520) finds no block
 * under the cursor, and back in when it does. Every update clamps the
 * current colour to 0-1 and sets it on the light.
 *
 * The port's implementation stays in the port tail lighteffects_port.cpp,
 * unannotated, with its state spread over effects.cpp and util.cpp. In the
 * matching build aitask_port_aliases.h points lego_unit.c's callers at this
 * unit; natively nothing calls it. */
#include <math.h>
#include <stdlib.h>

#include "crt/vc5names.h"
#include "lighteffects.h"
#include "lighteffects_imports.h"

/* The unit's functions have C linkage in the native C++ compile too. */
#ifdef __cplusplus
extern "C" {
#endif

#define LIGHTFX_FLAG_BLINK      0x1
#define LIGHTFX_FLAG_FADE       0x4
#define LIGHTFX_FLAG_FADING     0x8    /* past the wait */
#define LIGHTFX_FLAG_FADE_UP    0x10   /* fading towards fadeDestRGB */
#define LIGHTFX_FLAG_FADE_DOWN  0x20   /* fading back after the hold */
#define LIGHTFX_FLAG_MOVE       0x40
#define LIGHTFX_FLAG_MOVING     0x80
#define LIGHTFX_FLAG_DISABLED   0x100
#define LIGHTFX_FLAG_DIMOUT     0x200  /* dimmer on: fade to black */
#define LIGHTFX_FLAG_DIMIN_DONE 0x400  /* dimmer off: back at full */
#define LIGHTFX_FLAG_DIMOUT_DONE 0x1000

/* Config's "" (004b8a14), which config.c defines (finding 581). */
extern const char s_config_angle_empty[4];
/* Config's ":" (004a1cd4), which config.c defines (finding 581). */
extern const char s_config_rgb_separator[];

typedef struct LightEffects_RGB {
    float r, g, b;
} LightEffects_RGB;

typedef struct LightEffects_Vector3F {
    float x, y, z;
} LightEffects_Vector3F;

typedef struct LightEffects_Range {
    float min, max;
} LightEffects_Range;

/* The original's LightEffects_Globs, 0xf4 bytes at 004eadd8. */
typedef struct LightEffects_Globs {
    void* contSpotlight;                  /* +0x00 the light */
    void* contRoot;                       /* +0x04 its frame */
    LightEffects_RGB initialRGB;          /* +0x08 */
    LightEffects_RGB currentRGB;          /* +0x14 */
    LightEffects_RGB blinkRGBMax;         /* +0x20 */
    float blinkTime;                      /* +0x2c */
    LightEffects_Range blinkWaitRange;    /* +0x30 */
    float blinkChange;                    /* +0x38 red drift so far */
    float blinkChangeMax;                 /* +0x3c */
    LightEffects_RGB fadeDestRGB;         /* +0x40 */
    LightEffects_RGB fadeRGBMin;          /* +0x4c */
    LightEffects_RGB fadeRGBMax;          /* +0x58 */
    float fadeWaitTime;                   /* +0x64 */
    LightEffects_Range fadeWaitRange;     /* +0x68 */
    LightEffects_RGB fadeSpeedRGB;        /* +0x70 */
    LightEffects_Range fadeTimeRange;     /* +0x7c */
    float fadeHoldTime;                   /* +0x84 */
    LightEffects_Range fadeHoldRange;     /* +0x88 */
    LightEffects_RGB fadePosRGB;          /* +0x90 */
    LightEffects_Vector3F moveStartPos;   /* +0x9c */
    LightEffects_Vector3F moveLimit;      /* +0xa8 */
    float moveWaitTime;                   /* +0xb4 */
    LightEffects_Range moveWaitRange;     /* +0xb8 */
    float moveSpeed;                      /* +0xc0 */
    LightEffects_Range moveSpeedRange;    /* +0xc4 */
    LightEffects_Vector3F moveVector;     /* +0xcc */
    float moveDist;                       /* +0xd8 */
    LightEffects_Range moveDistRange;     /* +0xdc */
    unsigned char unused_e4[0x0c];        /* +0xe4 */
    unsigned int flags;                   /* +0xf0 */
} LightEffects_Globs;

// GLOBAL: LEGORR 0x004eadd8
static LightEffects_Globs lightEffectsGlobs;

// FUNCTION: LEGORR 0x0044c710
void light_effects_initialise(void* contSpotlight, void* contRoot,
                              float initialRed, float initialGreen,
                              float initialBlue)
{
    lightEffectsGlobs.initialRGB.r = initialRed;
    lightEffectsGlobs.initialRGB.g = initialGreen;
    lightEffectsGlobs.initialRGB.b = initialBlue;
    lightEffectsGlobs.currentRGB.r = initialRed;
    lightEffectsGlobs.currentRGB.g = initialGreen;
    lightEffectsGlobs.currentRGB.b = initialBlue;
    lightEffectsGlobs.contSpotlight = contSpotlight;
    lightEffectsGlobs.contRoot = contRoot;
    light_effects_reset_colour();
}

// FUNCTION: LEGORR 0x0044c760
void light_effects_reset_colour(void)
{
    render_object_set_packed_rgba_from_float_channels(
        lightEffectsGlobs.contSpotlight, lightEffectsGlobs.initialRGB.r,
        lightEffectsGlobs.initialRGB.g, lightEffectsGlobs.initialRGB.b,
        1.0f);
}

// FUNCTION: LEGORR 0x0044c790
void light_effects_set_disabled(int disabled)
{
    if (disabled) {
        lightEffectsGlobs.flags |= LIGHTFX_FLAG_DISABLED;
        light_effects_reset_colour();
    } else {
        lightEffectsGlobs.flags &= ~LIGHTFX_FLAG_DISABLED;
    }
}

// FUNCTION: LEGORR 0x0044c7c0
void light_effects_load(BlockFile* config, const char* gameName)
{
    light_effects_load_blink(config, gameName);
    light_effects_load_fade(config, gameName);
    light_effects_load_move(config, gameName);
}

// FUNCTION: LEGORR 0x0044c7f0
int light_effects_load_blink(BlockFile* config, const char* gameName)
{
    float r, g, b;
    float maxChange;
    char* str;
    char* argv[10];
    float minTime, maxTime;

    if (config_rgb_vector_read(config, string_path_join(gameName,
            // STRING: LEGORR 0x004a51b8
            "LightEffects",
            // STRING: LEGORR 0x004a51c8
            "BlinkRGBMax", 0), &r, &g, &b)) {
        if (0.0f != (maxChange = (float)atof(block_get_strptr(config, string_path_join(gameName, "LightEffects",
                // STRING: LEGORR 0x004a51a4
                "MaxChangeAllowed", 0))
                ? block_get_strptr(config, string_path_join(gameName, "LightEffects", "MaxChangeAllowed", 0))
                : s_config_angle_empty))) {
            maxChange *= (1.0f / 255.0f);
        }

        if ((str = block_get_str_copy(config, string_path_join(gameName, "LightEffects",
                // STRING: LEGORR 0x004a5184
                "RandomRangeForTimeBetweenBlinks", 0)))) {
            if (sound_variant_split(str, argv, s_config_rgb_separator) == 2) {
                minTime = (float)atof(argv[0]) * 25.0f;
                maxTime = (float)atof(argv[1]) * 25.0f;
                free(str);
                light_effects_set_blink(r, g, b, maxChange, minTime, maxTime);
                return 1;
            }
            free(str);
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x0044c970
void light_effects_set_blink(float maxRed, float maxGreen, float maxBlue,
                             float maxChange, float minTime, float maxTime)
{
    lightEffectsGlobs.blinkRGBMax.r = maxRed;
    lightEffectsGlobs.blinkRGBMax.g = maxGreen;
    lightEffectsGlobs.blinkRGBMax.b = maxBlue;
    lightEffectsGlobs.blinkChangeMax = maxChange;
    lightEffectsGlobs.blinkWaitRange.min = minTime;
    lightEffectsGlobs.blinkWaitRange.max = maxTime;
    lightEffectsGlobs.flags |= LIGHTFX_FLAG_BLINK;
}

// FUNCTION: LEGORR 0x0044c9c0
int light_effects_load_fade(BlockFile* config, const char* gameName)
{
    float minR, minG, minB;
    float maxR, maxG, maxB;
    char* str;
    char* argv[10];
    float minWait, maxWait, minFade, maxFade, minHold, maxHold;

    if (config_rgb_vector_read(config, string_path_join(gameName, "LightEffects",
            // STRING: LEGORR 0x004a523c
            "FadeRGBMin", 0), &minR, &minG, &minB)) {
        if (config_rgb_vector_read(config, string_path_join(gameName, "LightEffects",
                // STRING: LEGORR 0x004a5230
                "FadeRGBMax", 0), &maxR, &maxG, &maxB)) {
            if ((str = block_get_str_copy(config, string_path_join(gameName, "LightEffects",
                    // STRING: LEGORR 0x004a5210
                    "RandomRangeForTimeBetweenFades", 0)))) {
                if (sound_variant_split(str, argv, s_config_rgb_separator) == 2) {
                    minWait = (float)atof(argv[0]) * 25.0f;
                    maxWait = (float)atof(argv[1]) * 25.0f;
                    free(str);
                    if ((str = block_get_str_copy(config, string_path_join(gameName, "LightEffects",
                            // STRING: LEGORR 0x004a51f4
                            "RandomRangeForFadeTimeFade", 0))) == NULL) return 0;
                    if (sound_variant_split(str, argv, s_config_rgb_separator) == 2) {
                        minFade = (float)atof(argv[0]) * 25.0f;
                        maxFade = (float)atof(argv[1]) * 25.0f;
                        free(str);
                        if ((str = block_get_str_copy(config, string_path_join(gameName, "LightEffects",
                                // STRING: LEGORR 0x004a51d4
                                "RandomRangeForHoldTimeOfFade", 0))) == NULL) return 0;
                        if (sound_variant_split(str, argv, s_config_rgb_separator) == 2) {
                            minHold = (float)atof(argv[0]) * 25.0f;
                            maxHold = (float)atof(argv[1]) * 25.0f;
                            free(str);
                            light_effects_set_fade(minR, minG, minB, maxR, maxG, maxB,
                                                   minWait, maxWait, minFade, maxFade,
                                                   minHold, maxHold);
                            return 1;
                        }
                    }
                }
                free(str);
            }
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x0044cc10
void light_effects_set_fade(float minRed, float minGreen, float minBlue,
                            float maxRed, float maxGreen, float maxBlue,
                            float minWait, float maxWait,
                            float minFade, float maxFade,
                            float minHold, float maxHold)
{
    lightEffectsGlobs.fadeRGBMin.r = minRed;
    lightEffectsGlobs.fadeRGBMin.g = minGreen;
    lightEffectsGlobs.fadeRGBMin.b = minBlue;
    lightEffectsGlobs.fadeRGBMax.r = maxRed;
    lightEffectsGlobs.fadeRGBMax.g = maxGreen;
    lightEffectsGlobs.fadeRGBMax.b = maxBlue;
    lightEffectsGlobs.fadeWaitRange.min = minWait;
    lightEffectsGlobs.fadeWaitRange.max = maxWait;
    lightEffectsGlobs.fadeTimeRange.min = minFade;
    lightEffectsGlobs.fadeTimeRange.max = maxFade;
    lightEffectsGlobs.fadeHoldRange.min = minHold;
    lightEffectsGlobs.fadeHoldRange.max = maxHold;
    lightEffectsGlobs.flags |= LIGHTFX_FLAG_FADE;
}

// FUNCTION: LEGORR 0x0044cca0
int light_effects_load_move(BlockFile* config, const char* gameName)
{
    char* str;
    char* argv[10];
    float minWait, maxWait, minSpeed, maxSpeed, minDist, maxDist;
    float limitX, limitY, limitZ;

    if ((str = block_get_str_copy(config, string_path_join(gameName, "LightEffects",
            // STRING: LEGORR 0x004a528c
            "RandomRangeForTimeBetweenMoves", 0)))) {
        if (sound_variant_split(str, argv, s_config_rgb_separator) == 2) {
            minWait = (float)atof(argv[0]) * 25.0f;
            maxWait = (float)atof(argv[1]) * 25.0f;
            free(str);
            if ((str = block_get_str_copy(config, string_path_join(gameName, "LightEffects",
                    // STRING: LEGORR 0x004a5270
                    "RandomRangeForSpeedOfMove", 0))) == NULL) return 0;
            if (sound_variant_split(str, argv, s_config_rgb_separator) == 2) {
                minSpeed = (float)atof(argv[0]) * 25.0f;
                maxSpeed = (float)atof(argv[1]) * 25.0f;
                free(str);
                if ((str = block_get_str_copy(config, string_path_join(gameName, "LightEffects",
                        // STRING: LEGORR 0x004a5254
                        "RandomRangeForDistOfMove", 0))) == NULL) return 0;
                if (sound_variant_split(str, argv, s_config_rgb_separator) == 2) {
                    minDist = (float)atof(argv[0]);
                    maxDist = (float)atof(argv[1]);
                    free(str);
                    if ((str = block_get_str_copy(config, string_path_join(gameName, "LightEffects",
                            // STRING: LEGORR 0x004a5248
                            "MoveLimit", 0))) == NULL) return 0;
                    if (sound_variant_split(str, argv, s_config_rgb_separator) == 3) {
                        limitX = (float)atof(argv[0]);
                        limitY = (float)atof(argv[1]);
                        limitZ = (float)atof(argv[2]);
                        free(str);
                        light_effects_set_move(minWait, maxWait, minSpeed, maxSpeed,
                                               minDist, maxDist, limitX, limitY, limitZ);
                        return 1;
                    }
                }
            }
        }
        free(str);
    }
    return 0;
}

// FUNCTION: LEGORR 0x0044cef0
void light_effects_set_move(float minWait, float maxWait,
                            float minSpeed, float maxSpeed,
                            float minDist, float maxDist,
                            float limitX, float limitY, float limitZ)
{
    lightEffectsGlobs.moveWaitRange.min = minWait;
    lightEffectsGlobs.moveWaitRange.max = maxWait;
    lightEffectsGlobs.moveSpeedRange.min = minSpeed;
    lightEffectsGlobs.moveSpeedRange.max = maxSpeed;
    lightEffectsGlobs.moveDistRange.min = minDist;
    lightEffectsGlobs.moveDistRange.max = maxDist;
    ddiscene_object_position_get(lightEffectsGlobs.contSpotlight,
                                 lightEffectsGlobs.contRoot,
                                 &lightEffectsGlobs.moveStartPos.x);
    lightEffectsGlobs.moveLimit.x = limitX;
    lightEffectsGlobs.moveLimit.y = limitY;
    lightEffectsGlobs.moveLimit.z = limitZ;
    lightEffectsGlobs.flags |= LIGHTFX_FLAG_MOVE;
}

// FUNCTION: LEGORR 0x0044cf70
void light_effects_invalidate_position(void)
{
    ddiscene_object_position_get(lightEffectsGlobs.contSpotlight,
                                 lightEffectsGlobs.contRoot,
                                 &lightEffectsGlobs.moveStartPos.x);
    lightEffectsGlobs.flags &= ~LIGHTFX_FLAG_MOVING;
}

// FUNCTION: LEGORR 0x0044cfa0
void light_effects_update(float elapsedGame)
{
    if (!(lightEffectsGlobs.flags & LIGHTFX_FLAG_DISABLED) &&
        lightEffectsGlobs.contSpotlight != NULL) {
        light_effects_update_blink(elapsedGame);
        light_effects_update_fade(elapsedGame);
        light_effects_update_move(elapsedGame);
        light_effects_update_dimmer(elapsedGame);
        light_effects_update_colour();
    }
}

// FUNCTION: LEGORR 0x0044cff0
void light_effects_update_colour(void)
{
    float r, g, b;

    /* Deviation (scoring only): the channels are copied as raw dwords. The
     * original then compares each local from memory; a float assignment
     * lets MSVC compare the global instead (84.48%, struct copy 45.61%). */
    *(unsigned int*)&r = *(unsigned int*)&lightEffectsGlobs.currentRGB.r;
    *(unsigned int*)&g = *(unsigned int*)&lightEffectsGlobs.currentRGB.g;
    *(unsigned int*)&b = *(unsigned int*)&lightEffectsGlobs.currentRGB.b;
    if (r < 0.0f) r = 0.0f;
    if (r > 1.0f) r = 1.0f;
    if (g < 0.0f) g = 0.0f;
    if (g > 1.0f) g = 1.0f;
    if (b < 0.0f) b = 0.0f;
    if (b > 1.0f) b = 1.0f;
    render_object_set_packed_rgba_from_float_channels(
        lightEffectsGlobs.contSpotlight, r, g, b, 1.0f);
}

// FUNCTION: LEGORR 0x0044d0d0
void light_effects_update_blink(float elapsedGame)
{
    int ok = 0;
    float rnd;
    float r, g, b;
    float change;

    if (lightEffectsGlobs.flags & LIGHTFX_FLAG_BLINK) {
        lightEffectsGlobs.blinkTime -= elapsedGame;
        if (lightEffectsGlobs.blinkTime <= 0.0f) {
            lightEffectsGlobs.blinkTime = random_float_range(
                lightEffectsGlobs.blinkWaitRange.min,
                lightEffectsGlobs.blinkWaitRange.max);
            rnd = (float)(rng_thunk_draw15() % 1000) * 0.001f;
            r = (lightEffectsGlobs.blinkRGBMax.r + lightEffectsGlobs.blinkRGBMax.r) * rnd - lightEffectsGlobs.blinkRGBMax.r;
            g = (lightEffectsGlobs.blinkRGBMax.g + lightEffectsGlobs.blinkRGBMax.g) * rnd - lightEffectsGlobs.blinkRGBMax.g;
            b = (lightEffectsGlobs.blinkRGBMax.b + lightEffectsGlobs.blinkRGBMax.b) * rnd - lightEffectsGlobs.blinkRGBMax.b;
            change = r + lightEffectsGlobs.blinkChange;
            if ((float)fabs(change) < lightEffectsGlobs.blinkChangeMax) {
                ok = 1;
            } else {
                light_effects_flip_sign(&r);
                light_effects_flip_sign(&g);
                light_effects_flip_sign(&b);
                change = r + lightEffectsGlobs.blinkChange;
                if ((float)fabs(change) < lightEffectsGlobs.blinkChangeMax) ok = 1;
            }
            if (ok) {
                lightEffectsGlobs.currentRGB.r += r;
                lightEffectsGlobs.blinkChange = change;
                lightEffectsGlobs.currentRGB.g += g;
                lightEffectsGlobs.currentRGB.b += b;
            }
        }
    }
}

// FUNCTION: LEGORR 0x0044d250
void light_effects_flip_sign(float* value)
{
    if (*value < 0.0f) *value = *value * -1.0f;
    else *value = -*value;
}

// FUNCTION: LEGORR 0x0044d280
void light_effects_update_fade(float elapsedGame)
{
    float rnd;
    float r, g, b;

    if (lightEffectsGlobs.flags & LIGHTFX_FLAG_FADE) {
        if (lightEffectsGlobs.flags & LIGHTFX_FLAG_FADING) {
            if (lightEffectsGlobs.flags & LIGHTFX_FLAG_FADE_UP) {
                r = elapsedGame * lightEffectsGlobs.fadeSpeedRGB.r;
                g = elapsedGame * lightEffectsGlobs.fadeSpeedRGB.g;
                b = elapsedGame * lightEffectsGlobs.fadeSpeedRGB.b;
                lightEffectsGlobs.currentRGB.r += r;
                lightEffectsGlobs.currentRGB.g += g;
                lightEffectsGlobs.currentRGB.b += b;
                lightEffectsGlobs.fadePosRGB.r += r;
                lightEffectsGlobs.fadePosRGB.g += g;
                lightEffectsGlobs.fadePosRGB.b += b;
                if (r >= 0.0f) {
                    if (lightEffectsGlobs.fadePosRGB.r >= lightEffectsGlobs.fadeDestRGB.r) {
                        lightEffectsGlobs.currentRGB.r -= lightEffectsGlobs.fadePosRGB.r - lightEffectsGlobs.fadeDestRGB.r;
                        lightEffectsGlobs.currentRGB.g -= lightEffectsGlobs.fadePosRGB.g - lightEffectsGlobs.fadeDestRGB.g;
                        lightEffectsGlobs.currentRGB.b -= lightEffectsGlobs.fadePosRGB.b - lightEffectsGlobs.fadeDestRGB.b;
                        lightEffectsGlobs.flags &= ~LIGHTFX_FLAG_FADE_UP;
                    }
                } else if (lightEffectsGlobs.fadePosRGB.r <= lightEffectsGlobs.fadeDestRGB.r) {
                    lightEffectsGlobs.currentRGB.r += lightEffectsGlobs.fadeDestRGB.r - lightEffectsGlobs.fadePosRGB.r;
                    lightEffectsGlobs.currentRGB.g += lightEffectsGlobs.fadeDestRGB.g - lightEffectsGlobs.fadePosRGB.g;
                    lightEffectsGlobs.currentRGB.b += lightEffectsGlobs.fadeDestRGB.b - lightEffectsGlobs.fadePosRGB.b;
                    lightEffectsGlobs.flags &= ~LIGHTFX_FLAG_FADE_UP;
                }
            } else if (lightEffectsGlobs.flags & LIGHTFX_FLAG_FADE_DOWN) {
                r = elapsedGame * lightEffectsGlobs.fadeSpeedRGB.r;
                g = elapsedGame * lightEffectsGlobs.fadeSpeedRGB.g;
                b = elapsedGame * lightEffectsGlobs.fadeSpeedRGB.b;
                lightEffectsGlobs.currentRGB.r -= r;
                lightEffectsGlobs.currentRGB.g -= g;
                lightEffectsGlobs.currentRGB.b -= b;
                lightEffectsGlobs.fadePosRGB.r -= r;
                lightEffectsGlobs.fadePosRGB.g -= g;
                lightEffectsGlobs.fadePosRGB.b -= b;
                if (r >= 0.0f) {
                    if (lightEffectsGlobs.fadePosRGB.r <= 0.0f) {
                        lightEffectsGlobs.currentRGB.r -= lightEffectsGlobs.fadePosRGB.r;
                        lightEffectsGlobs.currentRGB.g -= lightEffectsGlobs.fadePosRGB.g;
                        lightEffectsGlobs.currentRGB.b -= lightEffectsGlobs.fadePosRGB.b;
                        lightEffectsGlobs.flags &= ~(LIGHTFX_FLAG_FADING | LIGHTFX_FLAG_FADE_DOWN);
                    }
                } else if (lightEffectsGlobs.fadePosRGB.r >= 0.0f) {
                    lightEffectsGlobs.currentRGB.r -= lightEffectsGlobs.fadePosRGB.r;
                    lightEffectsGlobs.currentRGB.g -= lightEffectsGlobs.fadePosRGB.g;
                    lightEffectsGlobs.currentRGB.b -= lightEffectsGlobs.fadePosRGB.b;
                    lightEffectsGlobs.flags &= ~(LIGHTFX_FLAG_FADING | LIGHTFX_FLAG_FADE_DOWN);
                }
            } else {
                lightEffectsGlobs.fadeHoldTime -= elapsedGame;
                if (lightEffectsGlobs.fadeHoldTime <= 0.0f) {
                    light_effects_set_fade_speed();
                    lightEffectsGlobs.fadePosRGB.r = lightEffectsGlobs.fadeDestRGB.r;
                    lightEffectsGlobs.fadePosRGB.g = lightEffectsGlobs.fadeDestRGB.g;
                    lightEffectsGlobs.fadePosRGB.b = lightEffectsGlobs.fadeDestRGB.b;
                    lightEffectsGlobs.flags |= LIGHTFX_FLAG_FADE_DOWN;
                }
            }
        } else {
            lightEffectsGlobs.fadeWaitTime -= elapsedGame;
            if (lightEffectsGlobs.fadeWaitTime <= 0.0f) {
                lightEffectsGlobs.fadeWaitTime = random_float_range(
                    lightEffectsGlobs.fadeWaitRange.min,
                    lightEffectsGlobs.fadeWaitRange.max);
                lightEffectsGlobs.fadeHoldTime = random_float_range(
                    lightEffectsGlobs.fadeHoldRange.min,
                    lightEffectsGlobs.fadeHoldRange.max);
                rnd = (float)(rng_thunk_draw15() % 1000) * 0.001f;
                lightEffectsGlobs.fadePosRGB.r = 0.0f;
                lightEffectsGlobs.fadePosRGB.g = 0.0f;
                lightEffectsGlobs.fadePosRGB.b = 0.0f;
                lightEffectsGlobs.fadeDestRGB.r = (float)fabs(lightEffectsGlobs.fadeRGBMax.r - lightEffectsGlobs.fadeRGBMin.r) * rnd + lightEffectsGlobs.fadeRGBMin.r;
                lightEffectsGlobs.fadeDestRGB.g = (float)fabs(lightEffectsGlobs.fadeRGBMax.g - lightEffectsGlobs.fadeRGBMin.g) * rnd + lightEffectsGlobs.fadeRGBMin.g;
                lightEffectsGlobs.fadeDestRGB.b = (float)fabs(lightEffectsGlobs.fadeRGBMax.b - lightEffectsGlobs.fadeRGBMin.b) * rnd + lightEffectsGlobs.fadeRGBMin.b;
                light_effects_set_fade_speed();
                lightEffectsGlobs.flags |= (LIGHTFX_FLAG_FADING | LIGHTFX_FLAG_FADE_UP);
            }
        }
    }
}

// FUNCTION: LEGORR 0x0044d710
void light_effects_set_fade_speed(void)
{
    float time = random_float_range(lightEffectsGlobs.fadeTimeRange.min,
                                    lightEffectsGlobs.fadeTimeRange.max);
    lightEffectsGlobs.fadeSpeedRGB.r = lightEffectsGlobs.fadeDestRGB.r / time;
    lightEffectsGlobs.fadeSpeedRGB.g = lightEffectsGlobs.fadeDestRGB.g / time;
    lightEffectsGlobs.fadeSpeedRGB.b = lightEffectsGlobs.fadeDestRGB.b / time;
}

// FUNCTION: LEGORR 0x0044d760
void light_effects_update_move(float elapsedGame)
{
    int ok = 0;
    LightEffects_Vector3F newPos;
    LightEffects_Vector3F pos;
    float scale;

    if (lightEffectsGlobs.flags & LIGHTFX_FLAG_MOVE) {
        if (lightEffectsGlobs.flags & LIGHTFX_FLAG_MOVING) {
            ddiscene_object_position_get(lightEffectsGlobs.contSpotlight,
                                         lightEffectsGlobs.contRoot, &pos.x);
            newPos.x = pos.x + lightEffectsGlobs.moveVector.x;
            newPos.y = pos.y + lightEffectsGlobs.moveVector.y;
            newPos.z = pos.z + lightEffectsGlobs.moveVector.z;
            if (light_effects_check_move_limit(&newPos.x)) {
                ok = 1;
            } else {
                newPos.x = pos.x - lightEffectsGlobs.moveVector.x;
                newPos.y = pos.y - lightEffectsGlobs.moveVector.y;
                newPos.z = pos.z - lightEffectsGlobs.moveVector.z;
                if (light_effects_check_move_limit(&newPos.x)) ok = 1;
            }
            if (ok) {
                ddiscene_object_position_set(lightEffectsGlobs.contSpotlight,
                                             lightEffectsGlobs.contRoot,
                                             newPos.x, newPos.y, newPos.z);
                lightEffectsGlobs.moveDist -= lightEffectsGlobs.moveSpeed;
                if (lightEffectsGlobs.moveDist <= 0.0f) {
                    lightEffectsGlobs.flags &= ~LIGHTFX_FLAG_MOVING;
                }
            } else {
                lightEffectsGlobs.flags &= ~LIGHTFX_FLAG_MOVING;
            }
        } else {
            lightEffectsGlobs.moveWaitTime -= elapsedGame;
            if (lightEffectsGlobs.moveWaitTime <= 0.0f) {
                lightEffectsGlobs.moveWaitTime = random_float_range(
                    lightEffectsGlobs.moveWaitRange.min,
                    lightEffectsGlobs.moveWaitRange.max);
                lightEffectsGlobs.moveSpeed = random_float_range(
                    lightEffectsGlobs.moveSpeedRange.min,
                    lightEffectsGlobs.moveSpeedRange.max);
                lightEffectsGlobs.moveDist = random_float_range(
                    lightEffectsGlobs.moveDistRange.min,
                    lightEffectsGlobs.moveDistRange.max);
                d3drm_vector_random(&lightEffectsGlobs.moveVector.x);
                scale = 1.0f / (float)sqrt(
                    lightEffectsGlobs.moveVector.x * lightEffectsGlobs.moveVector.x +
                    lightEffectsGlobs.moveVector.y * lightEffectsGlobs.moveVector.y +
                    lightEffectsGlobs.moveVector.z * lightEffectsGlobs.moveVector.z);
                lightEffectsGlobs.moveVector.x = scale * lightEffectsGlobs.moveVector.x * lightEffectsGlobs.moveSpeed;
                lightEffectsGlobs.moveVector.y = scale * lightEffectsGlobs.moveVector.y * lightEffectsGlobs.moveSpeed;
                lightEffectsGlobs.moveVector.z = scale * lightEffectsGlobs.moveVector.z * lightEffectsGlobs.moveSpeed;
                lightEffectsGlobs.flags |= LIGHTFX_FLAG_MOVING;
            }
        }
    }
}

// FUNCTION: LEGORR 0x0044d9a0
int light_effects_check_move_limit(const float* position)
{
    LightEffects_Vector3F diff;

    diff.x = position[0] - lightEffectsGlobs.moveStartPos.x;
    diff.y = position[1] - lightEffectsGlobs.moveStartPos.y;
    diff.z = position[2] - lightEffectsGlobs.moveStartPos.z;
    diff.x = (float)fabs(diff.x);
    diff.y = (float)fabs(diff.y);
    if (diff.x > lightEffectsGlobs.moveLimit.x) return 0;
    if (diff.y > lightEffectsGlobs.moveLimit.y) return 0;
    if ((float)fabs(diff.z) > lightEffectsGlobs.moveLimit.z) return 0;
    return 1;
}

// FUNCTION: LEGORR 0x0044da20
void light_effects_set_dimmer_mode(int on)
{
    if (on) {
        lightEffectsGlobs.flags &= ~LIGHTFX_FLAG_DIMIN_DONE;
        lightEffectsGlobs.flags |= LIGHTFX_FLAG_DIMOUT;
    } else {
        lightEffectsGlobs.flags &= ~(LIGHTFX_FLAG_DIMOUT | LIGHTFX_FLAG_DIMOUT_DONE);
    }
}

// FUNCTION: LEGORR 0x0044da50
void light_effects_update_dimmer(float elapsedGame)
{
    if (lightEffectsGlobs.flags & LIGHTFX_FLAG_DIMOUT) {
        if (lightEffectsGlobs.flags & LIGHTFX_FLAG_DIMOUT_DONE) {
            lightEffectsGlobs.currentRGB.r = 0.0f;
            lightEffectsGlobs.currentRGB.g = 0.0f;
            lightEffectsGlobs.currentRGB.b = 0.0f;
        } else {
            elapsedGame *= 0.1f;
            lightEffectsGlobs.currentRGB.r -= elapsedGame;
            lightEffectsGlobs.currentRGB.g -= elapsedGame;
            lightEffectsGlobs.currentRGB.b -= elapsedGame;
            if (lightEffectsGlobs.currentRGB.r <= 0.0f) {
                lightEffectsGlobs.currentRGB.r = 0.0f;
                lightEffectsGlobs.flags |= LIGHTFX_FLAG_DIMOUT_DONE;
            }
        }
    } else if (!(lightEffectsGlobs.flags & LIGHTFX_FLAG_DIMIN_DONE)) {
        elapsedGame *= 0.1f;
        lightEffectsGlobs.currentRGB.r += elapsedGame;
        lightEffectsGlobs.currentRGB.g += elapsedGame;
        lightEffectsGlobs.currentRGB.b += elapsedGame;
        if (lightEffectsGlobs.currentRGB.r >= lightEffectsGlobs.initialRGB.r) {
            lightEffectsGlobs.currentRGB.r = lightEffectsGlobs.initialRGB.b;
            lightEffectsGlobs.flags |= LIGHTFX_FLAG_DIMIN_DONE;
        }
    }
}

#ifdef __cplusplus
}
#endif
