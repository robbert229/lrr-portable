// SPDX-License-Identifier: LGPL-3.0-or-later
/* Objective.c (GAME-029) imports: C declarations of the callees and globals
 * the unit uses from other modules (ADR 0005 step 6). Included only by
 * objective_unit.c and test_c_boundaries.cpp, so the other C units' symbol
 * numbering does not change (finding 568, "Unit-private imports header").
 * Each declaration names the definition the matching build links, with C
 * linkage; the callees' link-name headers (objective_unit_link_names.h)
 * map the original-shape names to it there. */
#ifndef LEGORRR_GAME_OBJECTIVE_UNIT_IMPORTS_H
#define LEGORRR_GAME_OBJECTIVE_UNIT_IMPORTS_H

#include <stdint.h>

#include "gods98/config.h"
#include "gods98/files.h"
#include "gods98/images.h"
#include "gods98/utils.h"
#include "lego_unit.h"
#include "reward.h"

#ifdef __cplusplus
extern "C" {
#endif

struct TextureSurface;
struct LevelState;

/* gods98 Main.c and Input.c state (crt.cpp, engine.cpp). */
extern int s_display_w;                     /* 00505924 appWidth */
extern int s_display_h;                     /* 00505928 appHeight */
extern int32_t s_application_mode;          /* 00505f88 programmerLevel */
extern uint32_t s_renderer_word_a;          /* 0076ab24 msx */
extern uint32_t s_renderer_word_b;          /* 0076ab28 msy */
extern uint32_t g_renderer_word_c;          /* 0076ab3c mslb */
extern uint32_t s_window_mouse_word_40;     /* 0076ab40 msrb */
extern uint32_t s_window_mouse_word_4c;     /* 0076ab4c */

/* GAME-021 Lego.c's legoGlobs fields (the port's globals; lego_unit.h has
 * the unit's stand-ins). */
extern const char* s_engine_config_root;    /* 005560c4 gameName */
extern uint32_t s_panel_highlight_flags;    /* 00556ed0 flags2 */

/* GAME-017 FrontEnd.c's frontGlobs stand-ins (frontend_unit.c). */
extern struct Front_Globs frontGlobs;
#define frontGlobs_saveImageBigWidth \
    (*(unsigned int*)((char*)&frontGlobs + 0x54c))  /* 0055750c */
#define frontGlobs_saveImageBigHeight \
    (*(unsigned int*)((char*)&frontGlobs + 0x550))  /* 00557510 */

/* Config's "" (004b8a14), which config.c defines (finding 581). */
extern const char s_config_angle_empty[4];

/* GAME-001 Advisor.c (advisor.c). */
int advisor_request_admit(int advisorType, int loop);             /* 004018c0 */
void advisor_clear_active_state(void);                            /* 00401b20 */
uint32_t formatted_text_printf_width(const struct TextureSurface* font,
                                     const char* text, ...);      /* 00401c50 */
/* GAME-005 Bubbles.c (bubbles.c). */
void bubble_toggle_object_uis_always_visible(void);               /* 00407340 */
int bubble_get_object_uis_always_visible(void);                   /* 00407360 */
/* GAME-008 (entity_cell_effects.c). */
void level_state_stored_value_set(uint32_t v);                    /* 00409d40 */
/* GAME-021 Lego.c (lego_unit.c). */
struct LevelState* level_current_owner_get(void);                 /* 00431560 */
int lego_keep_running(void);                                      /* 00435940 */
/* GAME-023 LegoObject.c (legoobject_unit.c). */
int legoobject_run_through_lists_skip_upgrade_parts(
    int (*callback)(struct Lego_Object* obj, void* data),
    void* data);                                                  /* 004377b0 */
int legoobject_position_to_terrain_cell(struct Lego_Object* obj, int* outX,
                                        int* outY);               /* 004424f0 */
void legoobject_scatter_spawn_gate_set(int on);                   /* 0044adc0 */
/* GAME-028 NERPs (tutorial_script.c). */
int nerpfunc_get_tutorial_flags(unsigned int* stack);             /* 00456240 */
int nerpfunc_set_objective_switch(unsigned int* stack);           /* 004567c0 */
/* GAME-030 (object_state_file.cpp): original-ABI adapters in
 * objective_port.cpp. In the matching build 004592e0 and 00459360 are
 * objectrecall.c's functions (#749, finding 648):
 * object-like aliases before each name's only declaration, so the symbol
 * counter does not move. 00459360 returns an int that this unit ignores. */
#if defined(_MSC_VER) && _MSC_VER < 1200 && !defined(__cplusplus)
#define objective_osf_store_adapter object_recall_snapshot
#define objective_osf_restore_adapter object_recall_restore
#endif
void objective_osf_store_adapter(void);                           /* 004592e0 */
void objective_osf_restore_adapter(void);                         /* 00459360 */
/* GAME-037 Reward.c (reward.c). */
int reward_level_start(void);                                     /* 00460220 */
void reward_score_add(float value);                               /* 00462800 */
/* GAME-041 SFX.c (sfx.c). */
int sfx_get_type(const char* sfxName, int* sfxID);                /* 00464c70 */
int sfx_random_play_sound_normal(int sfxID, int loop);            /* 00464fa0 */
float sfx_random_get_sample_play_time(int sfxID);                 /* 00465160 */
int sfx_is_queue_mode(void);                                      /* 004652b0 */
void sfx_set_queue_mode_and_flush(int on);                        /* 004652c0 */
void sfx_set_queue_mode(int on, int flushQueued);                 /* 004652d0 */
/* GAME-044 Teleporter.c (teleporter.c). */
int teleporter_service_all(unsigned int objTypes);                /* 0046a4c0 */
void teleporter_start(unsigned int objTypes, unsigned int modeFlags,
                      unsigned int teleportFlags);                /* 0046a510 */
/* GAME-045 Text.c (text.c). */
void text_display_message(int textType, int changeTiming,
                          int setFlag4);                          /* 0046ac60 */
/* GAME-046 ToolTip.c (tooltip.c). */
void tooltip_activate(int toolTipType);                           /* 0046b770 */

/* gods98 Main.c (frameloop.cpp). */
void main_loop_frame_pump(int force);                             /* 00477f30 */
/* gods98 Sound3D.c (sound3d.c). */
int sound_stream_stop_and_release(int stream_index);              /* 0047ba20 */
#if defined(_MSC_VER) && _MSC_VER < 1200 && !defined(__cplusplus)
/* Images.c is images_unit.c in the matching build (finding 632): the
 * C callers' calls go to the unit (images_link_names.h). */
#define image_surface_release Image_Remove /* 0047d470 */
#define resource_record_add_ref Image_SetPenZeroTrans /* 0047dbc0 */
#define image_surface_blit_transformed Image_SetupTrans /* 0047dbf0 */
#define objective_image_get_screenshot_adapter Image_GetScreenshot /* 0047e300 */
#endif
/* gods98 Images.c (images_unit.cpp, images.h). */
void image_surface_release(void* record);                         /* 0047d470 */
void resource_record_add_ref(void* record);                       /* 0047dbc0 */
void image_surface_blit_transformed(struct TextureSurface* surface,
                                    float low_red, float low_green,
                                    float low_blue, float high_red,
                                    float high_green, float high_blue); /* 0047dbf0 */
/* Image_GetScreenshot (0047e300) has no port entry with the original's
 * contract: an unannotated adapter in objective_port.cpp. */
int objective_image_get_screenshot_adapter(void* image, unsigned int xsize,
                                           unsigned int ysize);  /* 0047e300 */
/* gods98 Input.c (input). */
void input_cursor_client_position_set(int x, int y);              /* 0047f0d0 */
/* gods98 TextWindow.c (textwindow.c), by the names its definitions take. */
void* text_buffer_create(uintptr_t font_handle, const uint32_t* header,
                         long size);                              /* 004850f0 */
void text_output_field82c_bit2_set_clear(void* record, int set_bit); /* 00485160 */
void text_buffer_destroy_target(void* record);                    /* 00485190 */
void float_pair_at_offsetc_set_from_uints(void* record, uint32_t x,
                                          uint32_t y);            /* 004851d0 */
void formatted_text_append_to_ring_buffer(void* record, int skip_bells,
                                          const char* format, ...); /* 00485210 */
void text_buffer_append_format(void* record, const char* format, ...); /* 00485230 */
int text_buffer_draw_wrapped(void* record, uint32_t start_line,
                             float line_scale, int* height_out);  /* 00485390 */
void text_buffer_reset_target(void* record);                      /* 004859b0 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_OBJECTIVE_UNIT_IMPORTS_H */
