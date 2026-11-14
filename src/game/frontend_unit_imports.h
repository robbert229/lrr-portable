// SPDX-License-Identifier: LGPL-3.0-or-later
/* FrontEnd.c (GAME-017) imports: C declarations of the callees and globals
 * the unit uses from other modules (ADR 0005 step 6). Included only by
 * frontend_unit.c and test_c_boundaries.cpp, so the other C units' symbol
 * numbering does not change (finding 568, "Unit-private imports header").
 * Each declaration names the definition the matching build links, with
 * C linkage; the unit casts its original-layout records to the port's
 * parameter types. */
#ifndef LEGORRR_GAME_FRONTEND_UNIT_IMPORTS_H
#define LEGORRR_GAME_FRONTEND_UNIT_IMPORTS_H

#include <stdarg.h>
#include <stdint.h>

#include "gods98/animclone.h"
#include "gods98/config.h"
#include "gods98/flic.h"
#include "gods98/flic_api.h"
#include "gods98/frameloop.h"
#include "gods98/images.h"
#include "gods98/viewports.h"
#include "reward.h"
#include "savegame.h"

#ifdef __cplusplus
extern "C" {
#endif

struct TextureSurface;
struct Flic;

/* gods98 Main.c and Input.c state (crt.cpp, input). */
extern int s_display_w;                     /* 00505924 appWidth */
extern uint32_t s_engine_feature_word;      /* 00505f84 mainGlobs.flags */
extern int32_t s_application_mode;          /* 00505f88 programmerLevel */
extern uint32_t s_renderer_word_a;          /* 0076ab24 msx */
extern uint32_t s_renderer_word_b;          /* 0076ab28 msy */
extern uint32_t g_renderer_word_c;          /* 0076ab3c mslb */
extern uint32_t s_window_mouse_word_40;     /* 0076ab40 msrb */
extern uint8_t s_global_current_keys[256];  /* 0076ab80 Input_KeyMap */
extern uint8_t s_global_previous_keys[256]; /* 0076aa00 Input_PrevKeyMap */

extern const char* s_engine_config_root;    /* 005560c4 gameName */
extern void* DAT_0055613c;                  /* 0055613c the credits font */
extern void* DAT_00556f50;                  /* 00556f50 CreditsTextFile */
extern void* DAT_00556f54;                  /* 00556f54 CreditsBackAVI */

/* Config's "" (004b8a14) and ":" (004a1cd4), which config.c defines
 * (finding 581). */
extern const char s_config_angle_empty[4];
extern const char s_config_rgb_separator[];

/* GAME-001 Advisor.c (advisor.c): Font.h's inline Font_GetStringWidth. */
uint32_t formatted_text_printf_width(const struct TextureSurface* font,
                                     const char* text, ...);      /* 00401c50 */
/* GAME-009 Credits.c (screen_effect_quads.c); natively the port's
 * main_menu_text_item (frontend.hpp, frontend_api.h). */
void main_menu_text_item(void);
void credits_play(const char* textFile, struct TextureSurface* font,
                  const char* backAvi);                           /* 0040a0c0 */
/* GAME-016 (steering.c): Mem_Free of an item's data. */
void optional_resource_destroy(void* item);                       /* 0040f7b0 */
/* GAME-021 Lego.c (lego_unit.c): the overwrite question. */
int lego_menu_option_panel_texture_update(const char* title, const char* msg,
                                          const char* ok,
                                          const char* cancel);    /* 004355c0 */
/* GAME-028 NERPs (tutorial_script.c): NERPFunc__GetTutorialFlags. */
int nerpfunc_get_tutorial_flags(unsigned int* stack);             /* 00456240 */
/* GAME-033 Pointer.c (pointer.c). */
void pointer_set_current_if_timer_finished(int pointerType);      /* 0045cc20 */
int pointer_get_current_type(void);                               /* 0045cc60 */
void pointer_draw_pointer(unsigned int mouseX, unsigned int mouseY); /* 0045cc70 */
/* GAME-037 Reward.c (reward.h): Reward_GetLevel. */
/* GAME-041 SFX.c (sfx.c). */
int sfx_get_type(const char* sfxName, int* sfxID);                /* 00464c70 */
void sfx_add_to_queue(int sfxID, int mode);                       /* 00464f60 */
int sfx_random_play_sound_normal(int sfxID, int loop);            /* 00464fa0 */
void sfx_update(float elapsed);                                   /* 004651a0 */
/* GAME-046 ToolTip.c (tooltip.c). */
int tooltip_get_type(const char* toolTipName, int* toolTipType);  /* 0046b4d0 */
void tooltip_activate(int toolTipType);                           /* 0046b770 */
void tooltip_update(unsigned int mouseX, unsigned int mouseY,
                    float elapsed);                               /* 0046b7c0 */

/* gods98 Container.c (container_unit.cpp, view.h, record.h, container.h). */
/* Container_SetLightAttenuation-style setters (00473460, 00473480): the
 * port declares the float argument as its bits (view.hpp); the unit
 * passes the float (frontend_unit.c). */
void scene_render_lock_toggle(void* scene_object, int enabled);   /* 00473b40 */
void render_object_set_packed_rgba_from_float_channels(
    void* scene_object, float red, float green, float blue,
    float alpha);                                                 /* 004750b0 */
float ddiscene_frame_update_and_callbacks(void* scene_object, float time); /* 00475140 */
unsigned int ddiscene_object_frame_position_get(void* scene_object); /* 004753f0 */
int ddiscene_object_position_set(void* object, void* alternate_frame,
                                 float x, float y, float z);      /* 00475430 */
int ddiscene_object_pose_method_dispatch(void* object,
                                         void* alternate_frame,
                                         float dir_x, float dir_y,
                                         float dir_z, float up_x,
                                         float up_y, float up_z); /* 00475470 */
int ddiscene_object_position_get(void* object, void* alternate_frame,
                                 float position[3]);              /* 004754c0 */
int ddiscene_object_facing_pair_get(void* object, void* alternate_frame,
                                    float first[3], float second[3]); /* 00475500 */
void ddiscene_scene_object_attach_or_unlink(void* pair_record,
                                            void* parent);        /* 00475710 */
/* gods98 Viewports.c: viewports.h. */
/* gods98 Utils.c (utils.c). */
int sound_variant_split(char* text, char** out, const char* separators); /* 00477440 */
const char* case_insensitive_substring_find(const char* haystack,
                                            const char* needle);  /* 00477610 */
/* gods98 Font.c (font.c), by the names its definitions take. */
void* font_atlas_record_create(const char* name);                 /* 00479ee0 */
void font_print_f(const struct TextureSurface* font, int x, int y,
                  const char* msg, ...);                          /* 0047a180 */
uint32_t object_field_be4_dword_get(const void* record);          /* 0047a540 */
/* gods98 Sound3D.c (sound3d.c). */
void audio_sound_volume_index_set(int index);                     /* 0047b530 */
int sound_stream_stop_and_release(int stream_index);              /* 0047ba20 */
#if defined(_MSC_VER) && _MSC_VER < 1200 && !defined(__cplusplus)
/* Images.c is images_unit.c in the matching build (finding 632): the
 * C callers' calls go to the unit (images_link_names.h). */
#define image_surface_release Image_Remove /* 0047d470 */
#define resource_record_add_ref Image_SetPenZeroTrans /* 0047dbc0 */
#define image_surface_blit_transformed Image_SetupTrans /* 0047dbf0 */
#define front_image_save_bmp_adapter Image_SaveBMP /* 0047e440 */
#endif
/* gods98 Images.c (images_unit.cpp, images.h). */
void image_surface_release(void* record);                         /* 0047d470 */
void resource_record_add_ref(void* record);                       /* 0047dbc0 */
void image_surface_blit_transformed(struct TextureSurface* surface,
                                    float low_red, float low_green,
                                    float low_blue, float high_red,
                                    float high_green, float high_blue); /* 0047dbf0 */
/* Image_SaveBMP (0047e440) has no port entry: an unannotated adapter in
 * frontend_port.cpp. */
int front_image_save_bmp_adapter(void* image, const char* fname); /* 0047e440 */
/* gods98 Flic.c: flic.h's adapters (Flic_Setup, Flic_Animate) and
 * flic_api.h's Flic_Close. The original reads the flic's width through
 * 004897d0 (animclone.h's anim_clone_is_lws) and its height through
 * 004850e0. */
uint32_t object_field10_get(const void* object);                  /* 004850e0 */
/* gods98 TextWindow.c (textwindow.c), by the names its definitions take. */
void text_buffer_append_format(void* record, const char* format, ...); /* 00485230 */
int text_buffer_draw_wrapped(void* record, uint32_t start_line,
                             float line_scale, int* height_out);  /* 00485390 */
void text_buffer_reset_target(void* record);                      /* 004859b0 */
/* FrontEnd.c part 2 (frontend_port.cpp): Front_Save_GetSaveData
 * (00417e70) and Front_Save_ReadSaveFile (00417a90) are in savegame.h;
 * 004150b0 is declared in frontend_unit.c. */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_FRONTEND_UNIT_IMPORTS_H */
