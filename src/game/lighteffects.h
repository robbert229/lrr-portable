// SPDX-License-Identifier: LGPL-3.0-or-later
/* LightEffects.c (original translation unit GAME-024, finding 505; rebuilt
 * in original shape, finding 594).
 *
 * C-compatible header (ADR 0005). lighteffects.c is compiled as C in the
 * matching build and as C++ in the native port. Shared C++ headers do not
 * include this file: every include and declaration a header adds shifts
 * MSVC's symbol numbering (finding 506). src/game/test_c_boundaries.cpp
 * includes it with the C++ declarations of the port's entries.
 *
 * Containers are `void*` and the config is gods98's BlockFile. lego_unit.c
 * reaches these functions in the matching build through
 * aitask_port_aliases.h; natively the port tail lighteffects_port.cpp keeps
 * serving the port's names. */
#ifndef LEGORRR_GAME_LIGHTEFFECTS_H
#define LEGORRR_GAME_LIGHTEFFECTS_H

#include "gods98/config.h"

#ifdef __cplusplus
extern "C" {
#endif

void light_effects_initialise(void* contSpotlight, void* contRoot,
                              float initialRed, float initialGreen,
                              float initialBlue);              /* 0044c710 */
void light_effects_reset_colour(void);                        /* 0044c760 */
void light_effects_set_disabled(int disabled);                /* 0044c790 */
void light_effects_load(BlockFile* config, const char* gameName); /* 0044c7c0 */
int light_effects_load_blink(BlockFile* config, const char* gameName); /* 0044c7f0 */
void light_effects_set_blink(float maxRed, float maxGreen, float maxBlue,
                             float maxChange, float minTime,
                             float maxTime);                   /* 0044c970 */
int light_effects_load_fade(BlockFile* config, const char* gameName); /* 0044c9c0 */
void light_effects_set_fade(float minRed, float minGreen, float minBlue,
                            float maxRed, float maxGreen, float maxBlue,
                            float minWait, float maxWait,
                            float minFade, float maxFade,
                            float minHold, float maxHold);     /* 0044cc10 */
int light_effects_load_move(BlockFile* config, const char* gameName); /* 0044cca0 */
void light_effects_set_move(float minWait, float maxWait,
                            float minSpeed, float maxSpeed,
                            float minDist, float maxDist,
                            float limitX, float limitY,
                            float limitZ);                     /* 0044cef0 */
void light_effects_invalidate_position(void);                 /* 0044cf70 */
void light_effects_update(float elapsedGame);                 /* 0044cfa0 */
void light_effects_update_colour(void);                       /* 0044cff0 */
void light_effects_update_blink(float elapsedGame);           /* 0044d0d0 */
void light_effects_flip_sign(float* value);                   /* 0044d250 */
void light_effects_update_fade(float elapsedGame);            /* 0044d280 */
void light_effects_set_fade_speed(void);                      /* 0044d710 */
void light_effects_update_move(float elapsedGame);            /* 0044d760 */
int light_effects_check_move_limit(const float* position);    /* 0044d9a0 */
void light_effects_set_dimmer_mode(int on);                   /* 0044da20 */
void light_effects_update_dimmer(float elapsedGame);          /* 0044da50 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_LIGHTEFFECTS_H */
