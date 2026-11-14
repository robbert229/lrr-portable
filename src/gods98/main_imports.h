// SPDX-License-Identifier: LGPL-3.0-or-later
/* C boundary header (ADR 0005 step 6) for Main.c (G98-005, finding 641):
 * the functions and state of other modules that the unit uses and that have
 * no C unit header. Only main.c and src/game/test_c_boundaries.cpp include
 * it; the C++ declarations it pairs with have C linkage in place. */
#ifndef LEGORRR_GODS98_MAIN_IMPORTS_H
#define LEGORRR_GODS98_MAIN_IMPORTS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Input.c (G98-013, input_unit.cpp; input.hpp). */
int input_direct_initialize(void);                                  /* 0047ed90 */
void input_state_poll_retail(void);                                 /* 0047eef0 */
void input_direct_shutdown(void);                                   /* 0047efd0 */
void input_cursor_position_update(void);                            /* 0047f010 */

/* G98-028 (file_list_streams.cpp): the start-up streams. */
void ledger_stub_0048b260_StartupStreamStateReset(void);            /* 0048b260 */
void window_mode_flag_set(unsigned int mode);                       /* 0048b280 */
void ledger_stub_0048b290_StartupStreamsClose(void);                /* 0048b290 */
void startup_config_stream_load(void);                              /* 0048b2f0 */

/* 00484c90, the original's shared empty function (Flic.c's Flic_Nothing
 * in the matching build, flic_link_names.h; flic.hpp natively). */
void flic_opcode27_fallback_noop(void);                             /* 00484c90 */

/* Lego.c's Gods_Go (GAME-021, lego_unit.h). */
void lego_game_config_callback_registry_init(const char* gameName); /* 0041fa20 */

/* 004b8a14: Config.c's "", the literal the linker pooled for the whole
 * image (config.c; finding 624). */
extern const char s_config_angle_empty[4];

/* DAT_00505800 and DAT_00505804: Main.c's window and module instance
 * (platform/window.h, platform/winapi/frameloop.cpp). */
extern void* g_platform_cursor_window_handle;
extern void* g_main_instance_handle;
/* DAT_00505808 and DAT_0050580c: the active and exit flags
 * (frameloop.hpp). */
extern unsigned int s_window_application_active;
extern unsigned int s_window_close_requested;
/* DAT_00505918, DAT_0050591c and DAT_00505920: the retained-mode device,
 * its immediate-mode device and the fog method (viewports_port.cpp,
 * mesh_port.cpp, view.cpp). DAT_00505914, the IDirect3DRM3, is in main.h. */
extern void* s_main_device;
extern void* s_main_im_device;
extern unsigned int s_scene_fog_mode;
/* DAT_00505924 and DAT_00505928: the application's width and height
 * (crt.hpp). */
extern int s_display_w;
extern int s_display_h;
/* DAT_00505f84, DAT_00505f88 and DAT_0050608c: the flags, the programmer
 * level and the -flags word (crt.hpp, engine.h). */
extern unsigned int s_engine_feature_word;
extern int s_application_mode;
extern unsigned int s_render_feature_flags;
/* DAT_0076ab3c to DAT_0076ab58: Input.c's mouse button state, which the
 * window procedures write (crt.hpp). */
extern unsigned int g_renderer_word_c;
extern unsigned int s_window_mouse_word_40;
extern unsigned int s_window_mouse_word_4c;
extern unsigned int s_window_mouse_word_50;
extern unsigned int s_window_mouse_word_54;
extern unsigned int s_window_mouse_word_58;

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_MAIN_IMPORTS_H */
