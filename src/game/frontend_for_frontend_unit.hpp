// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what frontend_unit uses from frontend.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GAME_FRONTEND_FOR_FRONTEND_UNIT_HPP
#define LRR_GAME_FRONTEND_FOR_FRONTEND_UNIT_HPP

#if defined(LEGORRR_BACKEND_WINAPI)
#define FE_FLOW_LEVEL_START_SLOT 11
#define FE_FLOW_MENU_OPTIONS_SLOT 12
#define FE_FLOW_QUIT_WORD       s_fe_flow_quit
#define FE_FLOW_SAVE_SLOT       s_fe_flow_save_slot
#define FE_FLOW_INGAME_SLOT     s_fe_flow_ingame
#define FE_FLOW_LEVEL_START     s_fe_flow_level_start
#define FE_FLOW_MENU_OPTIONS    s_fe_flow_menu_options
#else
#define FE_FLOW_LEVEL_START_SLOT 3
#define FE_FLOW_MENU_OPTIONS_SLOT 4
#define FE_FLOW_QUIT_WORD       s_fe_flow[0]
#define FE_FLOW_SAVE_SLOT       s_fe_flow[1]
#define FE_FLOW_INGAME_SLOT     s_fe_flow[2]
#define FE_FLOW_LEVEL_START     s_fe_flow[FE_FLOW_LEVEL_START_SLOT]
#define FE_FLOW_MENU_OPTIONS    s_fe_flow[FE_FLOW_MENU_OPTIONS_SLOT]
#endif

/* --- the active-object word selector (see frontend.hpp) --------------------
 * DAT_00556fd0 (in-game) / DAT_00556fe4 (out) are unowned: zero-init
 * state words in savegame.cpp, addressed through the in-game test. */
extern int s_active_object_ingame;

extern int s_active_object_outgame;

extern void* DAT_0055613c;
extern void* DAT_00556f50;
extern void* DAT_00556f54;
extern uint32_t s_menu_wipe_arm;
extern uint32_t s_menu_wipe;
extern uint32_t s_menu_wipe_stop;
extern float s_menu_wipe_clock;
extern float s_menu_plane_pan[2];
extern int s_main_menu_text_item_pending;
void fe_display_label(const char* label, char* out, size_t out_size);
int s_stristr(const char* haystack, const char* needle);
const char* fe_cfg_str(BlockFile* cfg, const char* level, const char* key, char* out, size_t out_sz);
int fe_cfg_int(BlockFile* cfg, const char* level, const char* key, int dflt);
void fe_options_write(FrontEnd* fe, FeItem* it);
void fe_overlay_close_all(FrontEnd* fe);
int fe_text_width(const char* s);
void fe_draw_text(void* px, int pitch, int w, int h, int x, int y, const char* s, int r, int g, int b);
void fe_blit_texture_crop(void* px, int pitch, int w, int h, const TextureSurface* image, int source_x, int source_width, int source_height, int x, int y);
int fe_picker_submenu(const FeSubmenu* sub);
extern void* s_current_selection_context;
extern uint32_t s_menu_clock_millis;
extern uint32_t s_menu_blit_record[16];
#if defined(LEGORRR_BACKEND_WINAPI)
extern int s_fe_flow_ingame;
#endif
#if !defined(LEGORRR_BACKEND_WINAPI)
extern int s_fe_flow[5];
#endif
#if defined(LEGORRR_BACKEND_WINAPI)
extern int s_fe_flow_save_slot;
#endif
#if defined(LEGORRR_BACKEND_WINAPI)
extern int s_fe_flow_level_start;
#endif
#if defined(LEGORRR_BACKEND_WINAPI)
extern int s_fe_flow_quit;
#endif
#if defined(LEGORRR_BACKEND_WINAPI)
extern int s_fe_flow_menu_options;
#endif

#endif /* LRR_GAME_FRONTEND_FOR_FRONTEND_UNIT_HPP */
