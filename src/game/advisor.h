// SPDX-License-Identifier: LGPL-3.0-or-later
/* Advisor.c (original translation unit GAME-001, finding 505): the in-game
 * advisor, a talking figure that plays one of eleven LWS animations at one
 * of 21 configured screen positions. The unit also holds the out-of-line
 * copies of gods98 Maths.h's vector helpers and of four Main.h/Font.h
 * header inlines that other units call (finding 550).
 *
 * C-compatible header (ADR 0005). advisor.c is compiled as C in the matching
 * build and as C++ in the native port. Shared C++ headers do not include this
 * file: every include and declaration a header adds shifts MSVC's symbol
 * numbering (finding 506). src/game/test_c_boundaries.cpp checks it against
 * the C++ declarations of the same functions. */
#ifndef LEGORRR_GAME_ADVISOR_H
#define LEGORRR_GAME_ADVISOR_H

#include <stdarg.h>
#include <stdint.h>

#include "gods98/config.h"

#ifdef __cplusplus
extern "C" {
#endif

struct TextureSurface;

/* FUN_00401000 Advisor_Initialise. `camera` is the game camera
 * (LegoCamera); the advisor reads its container at +0x24. */
void advisor_scene_controller_initialize(const char* gameName,
                                         void* camera,
                                         void* viewMain);
/* FUN_004011c0 Advisor_End. */
void advisor_animation_end_cleanup(void);
/* FUN_00401210 Advisor_InitViewport (the argument is unused). */
void advisor_scene_visual_state_initialize(float distance);
/* FUN_00401240 Maths_Vector2DDistance. */
float vec2_dist(const float* a, const float* b);
/* FUN_00401270 Advisor_LoadAnims. */
void advisor_config_slots_load_target(BlockFile* config,
                                      const char* gameName);
/* FUN_004013a0 Maths_Vector3DModulus. */
float vec_len(const float* a);
/* FUN_004013e0 Advisor_GetAdvisorType. */
int advisor_position_name_to_index(const char* name, int* index);
/* FUN_00401420 Maths_Vector3DDistance. */
float vec_dist(const float* a, const float* b);
/* FUN_00401470 Maths_Vector3DCrossProduct. */
float* vec_cross(float* dst, const float* b, const float* c);
/* FUN_004014c0 Advisor_GetAnimType. */
int advisor_animation_name_to_index(const char* name, int* index);
/* FUN_00401500 Maths_Vector3DAdd. */
float* vec_add(float* dst, const float* a, const float* b);
/* FUN_00401530 Advisor_LoadPositions. */
void advisor_position_config_load(BlockFile* config, const char* gameName,
                                  int screenWidth, int screenHeight);
/* FUN_004016c0 Maths_Vector3DSubtract. */
float* vec_sub(float* dst, const float* a, const float* b);
/* FUN_004016f0 Maths_Vector3DDotProduct. */
float vec_dot(const float* a, const float* b);
/* FUN_00401720 Maths_Vector3DScale. */
float* vec_scale(float* dst, const float* src, float s);
/* FUN_00401750 Maths_Vector3DNormalise (zero length unguarded). */
void vec_normalize(float* v);
/* FUN_004017b0 Advisor_InitPosition. */
void advisor_position_record_initialize(int advisorType, int animType,
                                        int textType, int sfxID,
                                        int panelType, float x, float y);
/* FUN_00401840 Advisor_SetParameters. */
void advisor_action_position_and_mode_set(int advisorType, int panelType,
                                          float x, float y);
/* FUN_00401890 Advisor_GetOrigPos. */
void advisor_action_position_get(int advisorType, float* x, float* y);
/* FUN_004018c0 Advisor_Start. */
int advisor_request_admit(int advisorType, int loop);
/* FUN_00401930 Advisor_PlayAnim. */
void advisor_request_reset_and_submit_sound(int advisorType);
/* FUN_00401990 Advisor_Update. */
void advisor_transition_worker(float elapsed);
/* FUN_00401a70 Advisor_MoveAnimation. */
int advisor_scene_advance_completion_check(int animType, float elapsed);
/* FUN_00401b20 Advisor_EndLoop. */
void advisor_clear_active_state(void);
/* FUN_00401b30 Advisor_PositionUpdate. */
void advisor_apply_slot_transform(int advisorType);
/* FUN_00401bf0 Main_ProgrammerMode (Main.h inline). */
int engine_app_mode_get(void);
/* FUN_00401c20 Advisor_IsRunning (Advisor.h inline). */
uint32_t global_flag_bit0_is_set(void);
/* FUN_00401c30 Main_GetWindowWidth (Main.h inline). */
int display_width_get(void);
/* FUN_00401c40 Main_GetWindowHeight (Main.h inline). */
int display_height_get(void);
/* FUN_00401c50 Font_GetStringWidth (Font.h inline). */
uint32_t formatted_text_printf_width(const struct TextureSurface* font,
                                     const char* text, ...);
/* FUN_00401c80 Font_GetStringInfo (Font.h inline). */
void formatted_text_printf(const struct TextureSurface* font,
                           uint32_t* width, uint32_t* lineCount,
                           const char* msg, ...);

/* Port-only test accessors (not in the matching build). */
uint32_t advisor_state_word_peek(void);
void advisor_state_word_set(uint32_t v);
uint32_t advisor_action_flags_peek(int row);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_ADVISOR_H */
