// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-021 Lego.c part 1 (issue #544): the C declarations of the functions
 * and globals of other modules that lego_unit.c calls or reads.
 *
 * Every entry has C linkage. Where the owning module has a C++ header, its
 * declaration there carries extern "C" (in place) and
 * src/game/test_c_boundaries.cpp includes both, so a mismatch fails to
 * compile. They are kept here, not in the owners' C boundary headers, so
 * the other C units' symbol numbering does not change (finding 538); they
 * move to the owners' headers when the owners are converted. Where a
 * callee's contract differs from the original's, the port's contract is
 * used and noted. */
#ifndef LEGORRR_GAME_LEGO_UNIT_IMPORTS_H
#define LEGORRR_GAME_LEGO_UNIT_IMPORTS_H

#include <stdint.h>

/* GAME-025 Loader.c (loader.c, finding 633), two GAME-014 Encyclopedia.c
 * functions (encyclopedia.c) and Advisor.c's Main_GetStartLevel
 * (advisor.c). The declarations below keep the port's names, which the
 * port defines for native callers (loader_port.cpp, resource.cpp,
 * lego_port.cpp). In the matching build they name the
 * original functions instead; a #define adds no declaration, so
 * lego_unit.c's symbol numbering does not move (finding 538). */
#if defined(_MSC_VER) && _MSC_VER < 1200 && !defined(__cplusplus)
#define loader_initialise_adapter loader_initialise           /* 0044db50 */
#define loader_profile_entries_release_adapter loader_shutdown /* 0044dd40 */
#define string_table_entry_set_current loader_display_loading_bar /* 0044dde0 */
#define global_resource_handle_release loader_display_shutdown /* 0044e0a0 */
#define main_get_start_level_adapter main_get_start_level     /* 00401c00 */
#define effects_update_original encyclopedia_update             /* 0040e7f0 */
#define screen_space_effect_dispatch_original encyclopedia_draw_select_box /* 0040e8d0 */
#endif

/* GAME-022 Camera (camera_control.c, finding 587). The declarations below
 * keep the port's names, which the port tail (camera_control_port.cpp)
 * defines for native callers. In the matching build they name the unit's
 * original functions instead; a #define adds no declaration, so
 * lego_unit.c's symbol numbering does not move (finding 538). */
#if defined(_MSC_VER) && _MSC_VER < 1200
#define scene_object_chain_record_create camera_create
#define entity_b8_bit3_set_or_clear camera_enable_free_movement
#define camera_view_context_close camera_free
#define camera_view_context_set camera_init_camera_movements
#define screen_effect_record_set_original camera_track_object
#define record_fields_4_and_14_set camera_set_fp_object
#define record_field14_get camera_get_fp_camera_frame
#define cursor_screen_ray_context_reset camera_stop_movement
#define view_grid_edge_get camera_get_mouse_scroll_indent
#define entity_fields_a0_a4_store_a8_clear_original camera_shake
#define view_tick_original camera_update
#define camera_tilt_bounds_set camera_set_tilt_range
#define camera_tilt_set camera_set_tilt
#define camera_pan_axis_update camera_add_tilt
#define camera_rotation_set camera_set_rotation
#define camera_tilt_axis_update camera_add_rotation
#define camera_distance_bounds_set camera_set_zoom_range
#define camera_distance_set camera_set_zoom
#define camera_zoom_update camera_add_zoom
#define selection_record_point_get camera_get_topdown_position
#define selection_record_point_set camera_set_topdown_position
#define camera_topdown_position_get_adapter camera_get_topdown_world_pos
#define camera_pointer_pan_update camera_move
/* G98-003 Viewports.c (gods98/viewports.c, finding 593). These four keep
 * the unit's names in the matching build, because C++ port code calls the
 * port's names with the port's contract (viewports_link_names.h). The
 * unit's other functions take the port's names there, so lego_unit.c
 * reaches them unchanged. */
#define scene_resource_slots_release_all viewport_shutdown
#define lego_viewport_size_get_adapter viewport_get_size
#define lego_viewport_field_get_adapter viewport_get_field
#define camera_project_point_to_screen viewport_world_to_screen
/* GAME-049 Water.c (game/water.c, finding 614). The port's definitions of
 * these four names are outside the unit's port tail (gods98/view.cpp,
 * game/terrain_grid.cpp and game/lego_port.cpp), so in the matching build
 * lego_unit.c calls the unit's names; water_link_names.h gives the unit the
 * port's names of the other two. */
#define global_entity_render_parameter_mode_set water_change_view_mode
#define terrain_cell_surface_quad_reset_current water_seal_block
#define simtick_fx_bank_tick_original water_update
#define world_spatial_record_find_at_grid_coordinate water_find_pool
/* GAME-044 Teleporter.c (game/teleporter.c, finding 621). The port's
 * definition of this name is an adapter in game/lego_port.cpp, so in the
 * matching build lego_unit.c calls Teleporter_Update by the unit's name;
 * teleporter_link_names.h gives the unit the port's names of the other
 * three Lego.c callees. */
#define camera_move_pipeline_original teleporter_update
/* GAME-038 Roof.c (game/roof.c, finding 619). The port's definitions of
 * these names are in gods98/resource.cpp, the port tail
 * terrain_overlay_quads_port.cpp and game/lego_port.cpp (the two no-op
 * adapters), so in the matching build lego_unit.c calls the unit's names.
 * Every call passes the original's arguments. */
#define indexed_surface_resource_allocate roof_initialise
#define animated_texture_replace roof_set_texture
#define graphics_overlay_resources_release roof_shutdown
#define lego_roof_block_vertices_set_adapter roof_set_block_roof_vertices
#define terrain_dirty_quad_normal_emit_pass_original roof_update
#define graphics_overlay_visibility_set roof_hide
#define indexed_surface_entry_hide_and_record roof_hide_block
#define indexed_surface_entries_restore roof_show_hidden_blocks
/* G98-015 Mesh.c (gods98/mesh_unit.c, finding 625). The port's definition
 * of this name is the port tail gods98/mesh_port.cpp's, so in the matching
 * build lego_unit.c calls Mesh_Initialise by the unit's name. */
#define shared_texture_root_register mesh_initialise
/* GAME-017 FrontEnd.c part 1 (game/frontend_unit.c, finding 623). The
 * port's definition of this name is an adapter in frontend_port.cpp, so
 * in the matching build lego_unit.c calls Front_IsIntrosEnabled by the
 * unit's name; it takes nothing and answers the same flag. */
#define movie_avis_enabled_original front_is_intros_enabled
/* GAME-017 FrontEnd.c part 2 (finding 628): the original-ABI adapters'
 * entries, by their matching-build names (frontend_unit_link_names.h). */
#define frontend_enabled_original frontend_enabled                  /* 00416d00 */
#define options_scroll_index_store_current options_scroll_index_store /* 00414f90 */
#define overlay_menu_gate_original front_options_update             /* 00415370 */
#define frontend_start_level_original front_load_selected_level     /* 00415220 */
#define options_apply_from_slot_original front_update_options_slider_levels /* 00415150 */
#define front_play_intro_splash_adapter front_play_intro_splash     /* 004157c0 */
#define front_play_intro_movie_adapter front_play_intro_movie       /* 00415910 */
#define movie_play_level_video_adapter front_play_level_movie       /* 00415990 */
#define front_load_menus_adapter front_load_menus                   /* 004161f0 */
#define front_run_screen_menu_type_adapter front_run_screen_menu_type /* 00416c80 */
#define front_get_selected_level_adapter front_get_selected_level   /* 00416d70 */
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ---- group M ---- */


/* legoGlobs fields the port models as globals of its own (each annotated
 * at the field's address in its file). */
extern struct LevelState* g_active_level_retail; /* terrain_grid.cpp, 005560d4 currLevel */
extern const char* s_engine_config_root;      /* engine.cpp, 005560c4 gameName */
extern void* s_zoom_step_context;             /* view.cpp, 005560e4 cameraMain */
extern float s_scroll_scale;                  /* engine.cpp, 0055615c gameSpeed */
extern uint32_t s_triple_first;               /* engine.cpp, 0055651c */
extern uint32_t s_triple_second;              /* engine.cpp, 00556520 */
extern uint32_t s_triple_third;               /* engine.cpp, 00556524 */
extern int32_t s_context_mode;                /* engine.cpp, 00556ec8 viewMode */
extern uint32_t s_panel_highlight_flags;      /* engine.cpp, 00556ed0 flags2 */
extern uint32_t s_global_low_word;            /* engine.cpp, 00556ed4 */
extern float s_global_float_00556f00;         /* crt.cpp, 00556f00 */
extern float s_score_text_origin_x;           /* engine.cpp, 00556f1c */
extern float s_score_text_origin_y;           /* engine.cpp, 00556f20 */
extern float s_task_text_origin_x;            /* engine.cpp, 00556f24 */
extern float s_task_text_origin_y;            /* engine.cpp, 00556f28 */
extern void* s_refresh_record;                /* engine.cpp, 005563b4 */
extern void* s_position_handle;               /* engine.cpp, 00556f3c */
/* Lego.c's own .bss that the port's lego.cpp still defines. */
extern int s_scroll_guard;                    /* 004de548, defined at the end */

/* GAME-032 Panel.c (panel_unit.h). */
void ui_overlay_animation_advance(int up, unsigned int count); /* 0045c580 */
/* GAME-011 (tile_variants.h): Detail_GetTexture, which takes the texture
 * as a byte. */
unsigned int packed_grid_cell_value_get(const void* grid,
                                        unsigned char packed_cell); /* 0040b850 */
/* G98 sound (sound.hpp, sound.h). */
int sound_batch_mode_get(void);                            /* 004652b0 */
void sound_batch_queue_drain(int batch_set, int replay);   /* 004652d0 */
int sound_alias_play(int alias, int loop);                 /* 00464fa0 */
/* GAME-033 pointer (media.hpp). */
void media_pointer_cursor_override_set(int slot, float value); /* 0045cc40 */
/* GAME-017 FrontEnd.c: FUN_00414f90 takes no argument; the port's entry
 * takes the game context. Thin adapter at the end of frontend_unit.cpp. */
void options_scroll_index_store_current(float speed);      /* 00414f90 */



/* ---- group A ---- */

/* The C boundary headers of the converted gods98 units whose records
 * Lego_Initialise uses: Config.c (BlockFile, BlockRec) and Images.c
 * (Image, Area2F, Point2F, and the original-ABI image adapters). */
#include "gods98/config.h"
#include "gods98/images.h"
#include "gods98/image_adapters.h"

struct TextureSurface;
struct SceneObjectChainRecord;
struct ScreenBoundedSurfaceRecord;

/* legoGlobs fields the port keeps as port globals. */
extern uint32_t s_engine_feature_word;        /* crt.cpp, 00505f84 mainGlobs.flags */
extern void* DAT_0055613c;                    /* frontend.cpp, 0055613c the RSFont */
extern void* DAT_00556f50;                    /* frontend.cpp, 00556f50 CreditsTextFile */
extern void* DAT_00556f54;                    /* frontend.cpp, 00556f54 CreditsBackAVI */
/* lego.cpp's stand-ins for the two Panel.c fonts and the radar zoom
 * (game/lego.h, finding 554), unannotated there. */
extern struct TextureSurface* s_lego_text_window_font;   /* 00556124 fontStandard */
extern struct TextureSurface* s_lego_crystal_count_font; /* 00556128 fontToolTip */
extern float s_lego_radar_zoom;                          /* 005563f4 */

/* G98 Keys.c (keys.h). */
void keyboard_key_names_init(void);                        /* 00485a20 */
/* G98 Viewports.c (scene_resources.hpp). */
void scene_resource_slots_initialize(void);                /* 00476d50 */
struct ScreenBoundedSurfaceRecord* render_target_viewport_scale_set(
    float x, float y, float width_scale, float height_scale,
    void* camera);                                         /* 00476dc0 */
void renderer_object_set_clip_distance(void* object, float distance); /* 00476fb0 */
/* GAME-022 LegoObject.c (ai.hpp, frontend.hpp, entity.hpp, level.hpp). */
void ai_activity_name_table_initialize(void);              /* 00436c20 */
void tooltip_icons_config_load(void* config);              /* 00437150 */
void entity_tool_name_strings_load(void* string_loader,
                                   void* config);          /* 00439950 */
void terrain_object_sfx_grid_config_load(struct BlockRec* config,
                                         const char* root_path); /* 0044acc0 */
/* G98 Images.c (surface.hpp). */
void renderer_record_table_initialize(void);               /* 0047d410 */
/* G98 Flic.c (flic.hpp): an empty function; the original's other empty
 * callee at this address (it takes two arguments) is an adapter below. */
void flic_opcode27_fallback_noop(void);                    /* 00484c90 */
/* GAME-03x map view (frontend.hpp). */
void screen_coord_normalize_init(void);                    /* 0045d8b0 */
/* GAME-032 Panel.c (panel_unit.h). */
void ui_panel_name_tables_init(void);                      /* 0045a030 */
void frontend_panel_rotation_control_config_load(struct BlockRec* config,
                                                 const char* gameName); /* 0045b9d0 */
void three_surface_resources_load(const char* smallCrystal,
                                  const char* usedCrystal,
                                  const char* noSmallCrystal); /* 0045bfb0 */
void panel_resource_layout_store(const char* juiceName, unsigned int juiceX,
                                 unsigned int juiceY,
                                 unsigned int juiceLength,
                                 const char* noAirName, unsigned int noAirX,
                                 unsigned int noAirY);     /* 0045c340 */
void panel_resource_handle_and_dimensions_store(const char* name,
                                                unsigned int x,
                                                unsigned int y,
                                                unsigned int length); /* 0045c520 */
void legacy_scroll_panel_controls_initialize(void);        /* 0045a240 */
void panel_item_flag16_set(int panelType, int button, int on); /* 0045aac0 */
/* GAME-002 AITask.c (cmd_from_aitask.hpp, cmd.hpp). */
void cmd_registry_init_target(void);                       /* 00401cf0 */
void engine_memory_free(void* ptr);                        /* 00402210 Mem_Free */
/* G98 resource (resource.hpp). */
void message_string_table_initialize(void);                /* 00451cd0 */
void string_table_entry_set_current(const char* name);     /* 0044dde0 */
void preload_numbered_bitmaps(const char* directory, const char* prefix); /* 0040a400 */
/* GAME sfx.cpp (crt.hpp, sound.hpp, sound.h). */
int scene_effect_registry_reset(void);                     /* 00465380 */
void sound_alias_registry_init(void);                      /* 00464740 */
int sound_alias_lookup(const char* name, int* out_index);  /* 00464c70 */
int sound_positional_sound_submit(int position, int alias, int loop,
                                  int positional, const float* xyz); /* 00465090 */
int sound_subsystem_enabled(void);                         /* 00465370 */
#if defined(_MSC_VER) && _MSC_VER < 1200
/* Matching build: the original itself, in this unit (00434a50) (finding 634). */
#define sound_samples_config_load lego_sound_samples_config_load
#endif
void sound_samples_config_load(struct BlockRec* config, int allow_marked); /* 00434a50 */
/* GAME pointer.cpp (ui_list.hpp, frontend.hpp, media.hpp). */
void ui_pointer_cursor_name_registry_init(void);           /* 0045c830 */
void pointer_cursor_config_load_target(struct BlockRec* first_entry); /* 0045ca70 */
void media_pointer_cursor_select(int slot);                /* 0045cc20 */
/* GAME-004 Bubbles.c (util.hpp, ai.hpp). */
void bubble_action_strings_initialize(void);               /* 004070b0 */
void bubble_config_texture_load(struct BlockRec* cfg);     /* 00407240 */
/* G98 Font.c (surface.hpp): Font_Load. */
void* font_atlas_record_create(const char* name);          /* 00479ee0 */
/* GAME Info.c (message.hpp, frontend.hpp). The original passes the font. */
#if defined(_MSC_VER) && _MSC_VER < 1200
/* Matching build: the original itself, in Info.c (info_unit.c) (finding 634). */
#define incident_message_catalog_init info_initialise
#endif
void incident_message_catalog_init(uint32_t owner);        /* 00419230 */
#if defined(_MSC_VER) && _MSC_VER < 1200
/* Matching build: the original itself, in Info.c (info_unit.c) (finding 634). */
#define interface_message_tab_info_text_load info_set_overflow_image_file
#endif
void interface_message_tab_info_text_load(const char* path); /* 00419430 */
/* GAME Interface.c (frontend.hpp, entity.hpp). */
/* G98 Container.c (crt.hpp, scene_pool.hpp). */
void scene_end_hook_register(void* hook, void* value);     /* 00472900 */
void shared_texture_directory_set(const char* path);       /* 004728c0 */
void ddiscene_object_pool_shutdown(void);                  /* 00472800 */
/* G98 Advisor.c (advisor.h). */
int display_width_get(void);                               /* 00401c30 */
int display_height_get(void);                              /* 00401c40 */
int engine_app_mode_get(void);                             /* 00401bf0 */
void advisor_scene_controller_initialize(const char* gameName,
                                         void* camera, void* viewMain); /* 00401000 */
void advisor_scene_visual_state_initialize(float distance); /* 00401210 */
void advisor_config_slots_load_target(struct BlockRec* config,
                                      const char* gameName); /* 00401270 */
void advisor_position_config_load(struct BlockRec* config,
                                  const char* gameName, int screenWidth,
                                  int screenHeight);       /* 00401530 */
/* GAME ToolTip.c (frontend.hpp). */
void tooltip_overlay_state_initialize(
    void* font, uint32_t geometry_a, uint32_t geometry_b,
    float duration_scale, uint32_t display_width, uint32_t display_height,
    uint32_t inset_word, float color_r, float color_g,
    float color_b);                                        /* 0046b1d0 */
/* G98 DirectDraw.c (directdraw.h). */
int directdraw_get_avail_texture_mem(unsigned int* total,
                                     unsigned int* avail);   /* 0047cdd0 */
void low_texture_flag_set(void);                           /* 00477bd0 */
/* G98 TextWindow.c (heap.hpp, textwindow.h). */
void* text_buffer_create(uintptr_t font_handle, const uint32_t* header,
                         long size);                       /* 004850f0 */
void text_buffer_append_format(void* record, const char* format, ...); /* 00485230 */
/* GAME Text.c (message.hpp). The original passes two screen positions as
 * the middle words. */
void ui_message_strings_initialize(void* primary_font, void* secondary_font,
                                   void* message_resource,
                                   void* subtitle_font,
                                   float line_delay);      /* 0046a7f0 */
/* GAME-017 FrontEnd.c (mission_for_reward.hpp, savegame.hpp, frontend.hpp,
 * options.hpp). */
char* mission_replace_char(char* text, char from, char to); /* 00413f00 */
void save_slot_selection_clear_target(void);               /* 00415cf0 */
int quit_menu_probe(void);                                 /* 00416dd0 */
void options_load_or_default_target(int load_options,
                                    int reset_frontend);   /* 00416940 */
/* G98 Utils.c (utils.h) and Sound.c (soundsys.h, sound.h). */
int sound_variant_split(char* text, char** out, const char* separators); /* 00477440 */
int media_cd_audio_playback_available(void);               /* 00488b90 */
int sound_alias_play(int alias, int loop);                 /* 00464fa0 */
/* G98 Camera / view (camera_control.cpp; view.hpp). */
struct SceneObjectChainRecord* scene_object_chain_record_create(void* parent,
                                                                int type); /* 00435b20 */
void camera_view_context_set(float cap, float follow_scale,
                             float pan_scale, int edge); /* 00435e0e */
void camera_distance_bounds_set(void* camera, float lo, float hi); /* 00436a18 */
void camera_tilt_bounds_set(void* camera, float lo, float hi); /* 00436831 */
/* GAME level.hpp. */
#if defined(_MSC_VER) && _MSC_VER < 1200
/* Matching build: the original itself, in this unit (004350a0) (finding 634). */
#define surface_type_descriptions_config_load lego_surface_type_descriptions_config_load
#endif
void surface_type_descriptions_config_load(struct BlockRec* config,
                                           const char* config_root); /* 004350a0 */
/* GAME Priorities.c (priorities.h). */
void priorities_load_positions(struct BlockRec* config, const char* game_name); /* 0045d060 */
void priorities_load_images(struct BlockRec* config, const char* game_name);    /* 0045cdc0 */
/* G98 Images.c / scene resources (scene_resources.hpp). */
int shared_texture_root_register(const char* root);        /* 004805b0 */
/* GAME message.hpp. */
void deferred_callback_set(uint32_t value);                /* 00453900 */
#if defined(_MSC_VER) && _MSC_VER < 1200
/* Matching build: the original itself, in this unit (00434c10) (finding 634). */
#define message_load_infomessages lego_message_load_infomessages
#endif
void message_load_infomessages(struct BlockRec* cfg);      /* 00434c10 */
/* GAME-021 parts 2-3, still in lego.cpp (entity_config_from_lego.hpp,
 * frontend.hpp). */
void lego_object_names_load(struct BlockRec* config);      /* 0042d600, called by 0041fb50 */
void lego_object_the_names_load(struct BlockRec* config);  /* 0042da20, called by 0041fb50 */
#if defined(_MSC_VER) && _MSC_VER < 1200
/* Matching build: the original itself, in this unit (00434da0) (finding 634). */
#define advisor_tooltip_and_sound_config_load lego_advisor_tooltip_and_sound_config_load
#endif
void advisor_tooltip_and_sound_config_load(struct BlockRec* config); /* 00434da0 */
#if defined(_MSC_VER) && _MSC_VER < 1200
/* Matching build: the original itself, in this unit (00434a00) (finding 634). */
#define advisor_tutorial_icon_image_load lego_advisor_tutorial_icon_image_load
#endif
void advisor_tutorial_icon_image_load(struct BlockRec* cfg); /* 00434a00 */
/* G98 Sound3D.c (sound3d.c, simtick.hpp, sound3d.h). The native port's
 * entries take a frame handle and the distance as an integer word. */
#if defined(LEGORRR_BACKEND_MINIWIN)
typedef int LegoA_Sound3DFrameArg;
void sound_tracked_buffers_set_max_distance(uint32_t value); /* 0047b4d0 */
#else
typedef struct Sound3DFrame* LegoA_Sound3DFrameArg;
void sound_tracked_buffers_set_max_distance(float dist);   /* 0047b4d0 */
#endif
void sound_tracked_buffers_set_min_distance(float value);  /* 0047b4a0 */
void simtick_directsound_listener_view_set(LegoA_Sound3DFrameArg frame); /* 0047ab80 */
float ledger_stub_0047c100_DirectSoundListenerRolloffSet(float rolloff); /* 0047c100 */
/* GAME placement_footprint.cpp (record.hpp). */
void surface_visibility_set(const void* record, uint32_t flag); /* 00464720 */
/* GAME HelpWindow.c (frontend.hpp). The original passes the font. */
void selection_mode_state_set(uint32_t value);             /* 00418190 */
/* GAME ObjInfo.c (frontend.hpp), LightEffects.c (effects.hpp). */
void frontend_render_three_pass_update(struct BlockRec* values,
                                       const char* config_root); /* 00459530 */
void light_effect_configuration_load(void* config,
                                     const char* config_root); /* 0044c7c0 */
void transient_effect_textures_preload(const char* filename_prefix,
                                       const char* filename_format,
                                       uint32_t texture_count); /* 004653a0 */
/* GAME Encyclopedia.c (frontend.hpp), Weapons.c (weapon.hpp). */
void encyclopedia_config_files_open(struct BlockRec* cfg,
                                    const char* root_path); /* 0040e490 */
int weapon_type_config_load(void* config, void* context);  /* 0046eb80 */
/* GAME mission.hpp. */
int reward_initialise(void);                               /* reward.c, 0045f030 */

/* Original-ABI adapters (ADR 0005 step 6), unannotated, where the port's
 * entry has another contract or there is none. */
/* Container_Initialise (00472710): no port entry (lego.cpp). */
void* container_initialise_original(const char* gameName);
/* LegoObject_TriggerFrameCallback (0043f130): no port entry (lego.cpp). */
#if defined(_MSC_VER) && _MSC_VER < 1200
/* Matching build: the original itself, in LegoObject.c (legoobject_unit.c) (finding 634). */
#define lego_object_trigger_frame_callback_adapter legoobject_trigger_frame_callback
#endif
void lego_object_trigger_frame_callback_adapter(void* cont, void* data);
/* DirectDraw_AdjustTextureUsage (0047cd50, directdraw.h). */
void directdraw_adjust_texture_usage(unsigned int* textureUsage);
/* TextWindow-backed message panel set-up (0046a950): no port entry
 * (lego.cpp). */
void text_message_panel_initialise_adapter(const char* imageFile,
                                           unsigned int x, unsigned int y,
                                           unsigned int openY,
                                           Area2F* messageWindow,
                                           Area2F* imageWindow,
                                           float timer);
/* Front_IsFrontEndEnabled (00416d00), Front_IsIntrosEnabled (00414ce0):
 * the port's entries read the config (frontend_unit.cpp). */
int frontend_enabled_original(void);
int movie_avis_enabled_original(void);
/* Front_PlayIntroMovie (00415910), Front_PlayIntroSplash (004157c0): no
 * port entries with this contract (lego.cpp). */
void front_play_intro_movie_adapter(const char* movieKey, int skippable);
void front_play_intro_splash_adapter(const char* imageKey, int skippable,
                                     const char* timeKey);
/* Loader_Initialise (0044db50): the port's entry is a key/value loader
 * (lego.cpp). */
void loader_initialise_adapter(const char* loadScreenName,
                               const char* shutdownScreenName,
                               struct TextureSurface* font,
                               const char* profileName, int progressDirection,
                               const char* progressBarName,
                               Area2F* progressWindow,
                               const char* loadingText);
/* Viewport_Create (00476e50): the port's entry is a ledger stub in a
 * namespace taking the size first (viewports.cpp). */
void* render_target_viewport_create_original(int x, int y, int width,
                                             int height, void* camera);
/* SFX/Lws_SetupSoundTriggers (004876c0): the port's entry takes its
 * callbacks in another order (lws.cpp). */
#ifdef __cplusplus
}
#endif
#include "gods98/lws.h" /* lws_initialise (004876c0), G98-020 */
#ifdef __cplusplus
extern "C" {
#endif
/* Lego_LoadLighting (00433770), Lego_LoadGraphicsSettings (004334f0),
 * Lego_LoadUpgradeTypes (0042d460): the port's entries take hook tables
 * or the config, or sit in a namespace (lego.cpp). */
#if defined(_MSC_VER) && _MSC_VER < 1200 && !defined(__cplusplus)
/* Matching build: the unit's own Lego_LoadLighting and
 * Lego_LoadGraphicsSettings (finding 639). */
#define scene_lights_build_from_config_original lego_scene_lights_build_from_config
#define render_quality_config_apply_original lego_render_quality_config_apply
#endif
int scene_lights_build_from_config_original(void);
int render_quality_config_apply_original(void);
/* SelectPlace_Create (00463f00): the port's entry takes a hook table
 * (placement_footprint.cpp). */
void* select_place_create_adapter(void* root, float size);
/* HelpWindow_Initialise (004181b0): the port's entry takes a resource-op
 * table (helpwindow.cpp). */
void help_window_initialise_adapter(struct BlockRec* config,
                                    const char* gameName);
/* Stats_Initialise (004667e0): entity_stats.c (GAME-043, finding 581). */
int stats_initialise(struct BlockRec* config, const char* gameName);
/* Dependencies_Initialise (0040ab70): the port's entry takes a level block
 * and the config (dependencies.cpp). */
void dependencies_initialise_adapter(struct BlockRec* config,
                                     const char* gameName);
/* The debug-key binder (00452010): the port's entry takes six words
 * (message_unit.cpp). */
void lego_key_callback_set_adapter(int key, int op, int a1, int a2, int a3);
/* An empty function the linker folded to 00484c90, called with the root
 * container and "animtex_dirt" (lego.cpp). */
void lego_noop_load_adapter(void* root, const char* name);
#if defined(_MSC_VER) && _MSC_VER < 1200
/* Matching build: the original calls the shared no-op (finding 634). */
#define lego_noop_load_adapter(root, name) \
    (((void (*)())flic_opcode27_fallback_noop)(root, name))
#endif
/* RockFall_Initialise (0040bfd0): the port's entry takes a style table
 * (rock_fall_styles.cpp). */
void rock_fall_initialise_adapter(struct BlockRec* config,
                                  const char* gameName, void* root);
/* Lego_LoadPanels (004345f0), Lego_LoadPanelButtons (00434710),
 * Lego_LoadTextMessages (00434af0): the port's entries are in a namespace
 * (lego.cpp). */
#if defined(_MSC_VER) && _MSC_VER < 1200
/* Matching build: the original itself, in this unit (004345f0) (finding 634). */
#define panels_config_entry_apply_original lego_panels_config_entry_apply
#endif
#if defined(_MSC_VER) && _MSC_VER < 1200
/* The unit's own declaration takes unsigned sizes. */
void panels_config_entry_apply_original(struct BlockRec* config,
                                        unsigned int width,
                                        unsigned int height);
#else
void panels_config_entry_apply_original(struct BlockRec* config, int width,
                                        int height);
#endif
#if defined(_MSC_VER) && _MSC_VER < 1200
/* Matching build: the original itself, in this unit (00434710) (finding 634). */
#define panel_button_layout_config_load_original lego_panel_button_layout_config_load
#endif
#if defined(_MSC_VER) && _MSC_VER < 1200
/* The unit's own declaration takes unsigned sizes. */
void panel_button_layout_config_load_original(struct BlockRec* config,
                                              unsigned int width,
                                              unsigned int height);
#else
void panel_button_layout_config_load_original(struct BlockRec* config,
                                              int width, int height);
#endif
#if defined(_MSC_VER) && _MSC_VER < 1200
/* Matching build: the original itself, in this unit (00434af0) (finding 634). */
#define text_messages_config_load_original lego_text_messages_config_load
#endif
void text_messages_config_load_original(struct BlockRec* config);
/* Interface_LoadImages (0041ada0), Interface_LoadSurroundImages
 * (0041afd0): the port's entries take the config only (interface.cpp). */
/* Reward_Initialise (0045f7f0): the port's entry takes the game context
 * (reward.cpp). */
int reward_load_graphics(struct BlockRec* config, const char* gameName); /* reward.c, 0045f7f0 */
/* Front_LoadMenus (004161f0): no port entry (lego.cpp). */
void front_load_menus_adapter(struct BlockRec* config);
/* Front_RunScreenMenuType (00416c80), Front_GetSelectedLevel (00416d70):
 * the port's entries take the front end (frontend_unit.cpp). */
void front_run_screen_menu_type_adapter(int menuType);
const char* front_get_selected_level_adapter(void);
/* Main_GetStartLevel (00401c00): Main.c's buffer is the Win32 entry
 * point's (lego.cpp). */
const char* main_get_start_level_adapter(void);



/* GAME-020 Interface.c, converted (interface_unit.h, finding 564). */
void interface_initialise(unsigned int x, unsigned int y,
                          struct TextureSurface* font);           /* 0041a300 */
void interface_shutdown(void);                                    /* 0041a850 */
void interface_init_object_items(void);                           /* 0041aa00 */
void interface_load_menu_items(BlockFile* config, const char* gameName); /* 0041ada0 */
void interface_load_item_panels(BlockFile* config, const char* gameName); /* 0041afd0 */
void interface_load_dependency_images(const char* plusName,
                                      const char* equalsName);    /* 0041b270 */
void interface_draw(void);                                        /* 0041b490 */
void interface_update(float elapsed);                             /* 0041b930 */
void interface_update_hover_sfx(float elapsed);                   /* 0041ba10 */

/* ---- group B ---- */

struct TextureSurface;
struct EntitySlot;
struct ViewContext;
struct TerrainMap;
struct TerrainMapVector3;
struct Image;
struct Area2F;
struct Point2F;
struct PanelGlobs;

/* Port globals at legoGlobs fields and gods98 globals (annotated in their
 * files). */
extern int32_t s_application_mode;            /* engine.cpp, 00505f88 mainGlobs.programmerLevel */
extern int s_display_w;                       /* crt.cpp, 00505924 appWidth */
extern uint32_t s_renderer_word_a;            /* crt.cpp, 0076ab24 mouse x */
extern uint32_t s_renderer_word_b;            /* crt.cpp, 0076ab28 mouse y */
extern float s_panel_highlight_timer;         /* engine.cpp, 00556efc */
extern struct PanelGlobs panelGlobs;          /* panel_unit.c, 005000e0 */
/* The two interface fonts, legoGlobs 00556124 and 00556128 (lego.cpp,
 * lego.h). */

/* gods98 Font.c, Input.c and the bitmap font (record.hpp, frontend.hpp,
 * input.hpp). */
uint32_t object_field_be4_dword_get(const void* record);   /* 0047a540 Font_GetHeight */
int bitmap_font_glyph_measure_width(void* font, unsigned char glyph); /* 0047a520 */
uint32_t input_key_context_check(int key);                 /* 00411f00 */
/* GAME-001 Advisor.c (advisor.h): the Font.h inline and Advisor_Update. */
uint32_t formatted_text_printf_width(const struct TextureSurface* font,
                                     const char* text, ...); /* 00401c50 */
void advisor_transition_worker(float elapsed);             /* 00401990 */
/* Font_PrintF (0047a180): font.c in the matching build; natively a thin
 * adapter in bitmap_font_port.cpp, whose renderer draws to a software
 * target. */
void font_print_f(const struct TextureSurface* font, int x, int y,
                  const char* msg, ...);
/* Message.c (select.hpp, message_unit.cpp). */
struct EntitySlot* selection_active_entity_get(void);      /* 004525c0 */
struct EntitySlot* const* selection_active_context_record_get(int* count_out); /* 00452580 */
void cmd_queue_tick(void);                                 /* 004520d0 */
/* LegoObject.c (entity.hpp, legoobject_port.cpp). */
const char* entity_display_name_lookup(const struct EntitySlot* entity); /* 004395e0 */
int grid_record_count_difference_get(int row, int column, int slot,
                                     int subtract_first_table); /* 00437460 */
void entity_type_two_event_dispatch(const struct EntitySlot* entity,
                                    uint32_t argument_a,
                                    uint32_t argument_b); /* 0043b820 */
void entity_type_two_positioned_callback_dispatch(
    const struct EntitySlot* entity, void* scene_object, float inner_range,
    float outer_range, int mode);                          /* 0043b850 */
/* Lego.c parts 2-3, still in lego.cpp (lego.h). */
struct LevelState* level_current_owner_get(void);          /* 00431560 */
void terrain_grid_entry_resources_release_all_current(void); /* 00434530 */
/* Single-unit ticks whose port entries keep the original ABI. */
void cmd_tick_global_delta(float delta);                   /* 00402220 aitask.cpp */
void screen_effects_tick(float delta);                     /* 004075a0 bubbles.cpp */
void lava_erosion_pending_cell_tick(float scale);          /* 0040eab0 */
uint32_t kind_six_random_sweep_dispatch(float delta);      /* 0040f0e0 */
int level_start_flag(void);                                /* 00416de0 frontend_unit.cpp */
int menu_options_flag(void);                               /* 00416df0 */
void help_window_update(void);                             /* 00418a00 helpwindow.c */
void message_dispatch(int id, const char* p2, uint32_t p3,
                      const void* p4);                     /* 00419b80 info.cpp */
#if defined(_MSC_VER) && _MSC_VER < 1200 && !defined(__cplusplus)
/* Matching build: Info.c's originals (info_unit.c), which take the same
 * frame time; natively the port's message queue keeps these names
 * (finding 639). */
#define info_message_queue_tick info_draw                   /* 00419f30 */
#define incident_message_text_consumer info_draw_panel      /* 0041a080 */
#define message_cooldown_tick info_update_timers            /* 0041a2c0 */
#endif
void info_message_queue_tick(float dt);                    /* 00419f30 */
void incident_message_text_consumer(float frame_delta);    /* 0041a080 */
void message_cooldown_tick(float dt);                      /* 0041a2c0 */
uint32_t sim_status_banner_word_get(void);                 /* 0041b9b0 */
uint32_t record_field14_get(const void* record);           /* 00435f0b camera_control.cpp */
void selection_record_point_set(struct ViewContext* v, float x,
                                float y);                  /* 00436b0b */
void motion_gameplay_tick(float delta);                    /* 0044cfa0 util.cpp */
void tutorial_script_command_dispatch(float delta);        /* 00453320 */
void tutorial_frame_timer_array_advance(float delta);      /* 00453bb0 */
void objective_announce_current(int who);                  /* 00458670 objective_unit.cpp */
void objective_tick_target(void* engine, struct LevelState* s,
                           float checker_dt, float frame_dt); /* 00458be0 */
void media_pointer_cursor_select(int slot);                /* 0045cc20 pointer.cpp */
void pointer_cursor_draw_target(uint32_t x, uint32_t y);   /* 0045cc70 */
void media_pointer_cursor_override_tick(float delta);      /* 0045cd90 */
void graphics_overlay_visibility_set(uint32_t flag);       /* 004634b0 */
void indexed_surface_entries_restore(void);                /* 00463540 */
void view_blit_rect_positions_apply(int view);             /* 00463d20 */
int sound_alias_lookup(const char* name, int* out_index);  /* 00464c70 */
int sound_variant_select(int alias_index);                 /* 00464e20 */
void sound_variant_property_set(int alias_index, int millibels); /* 00465010 */
void sound_handle_stop_forward(int buffer_handle);         /* 00465190 */
void sound_subsystem_frame_tick(float dt);                 /* 004651a0 */
void transient_effect_registry_enable_set(int enabled);    /* 00465a90 */
void transient_effect_instances_tick(float delta);         /* 00465b00 */
uint32_t building_upgrade_admission_check(int type, int row); /* 00469f40 */
void advisor_text_animation_update(int slot, int force, int selected); /* 0046ac60 */
void advisor_text_display_worker(float delta);             /* 0046ad00 */
void timed_effect_runtime_update(uint32_t render_x, uint32_t render_y,
                                 float delta);             /* 0046b7c0 */
/* GAME-026 terrain map (terrain_map.h). */
void terrain_map_update(struct TerrainMap* map, float elapsed); /* 0044eff0 */
void terrain_map_block_first_vertex_get(struct TerrainMap* map, int bx,
                                        int by,
                                        struct TerrainMapVector3* position); /* 00450060 */
void terrain_map_visible_clear(struct TerrainMap* map);    /* 00450780 */
/* GAME-032 Panel.c (panel_unit.h). */
void panel_print_f(int panel, const struct TextureSurface* font, int x,
                   int y, int center, const char* msg, ...); /* 0045a460 */
void simtick_manager_tick(int panel, float elapsed);       /* 0045a730 */
int request_record_ready_flag2_target(int panel);          /* 0045ab60 */
void global_value_0050080c_set(int on);                    /* 0045c4a0 */
void panel_side_bar_draw(void);                            /* 0045c5f0 */
/* GAME-034 Priorities.c (priorities.h). */
void priorities_draw(void);                                /* 0045d6d0 */
/* gods98 (record.hpp, view.hpp, viewports, surface, maths.h, sound3d.h,
 * text_window, soundsys.h, images.h). */
void scene_render_lock_toggle(void* scene_object, int enabled); /* 00473b40 */
int ddiscene_object_position_get(void* object, void* alternate_frame,
                                 float position[3]);        /* 004754c0 */
void render_object_rotate_axis(void* object, uint32_t mode, float x,
                               float y, float z, float angle); /* 00475580 */
void ledger_stub_00477010_DDiSceneDrawBegin(void* render_object,
                                            int begin);     /* 00477010 */
void ddiscene_draw_end(void* render_object, const void* commit_source,
                       float frame_delta);                  /* 00477150 */
void render_interface_transform_point(void* object, void* output,
                                      const void* input);   /* 004772b0 */
void renderer_begin_frame_once_retail(void);               /* 00477fa0 */
float random_float_range(float low, float high);           /* 004798b0 */
float ledger_stub_0047c100_DirectSoundListenerRolloffSet(float rolloff); /* 0047c100 */
int32_t constant_negative_10000_get(void);                 /* 0047c160 */
void image_display_scaled(struct Image* image, const struct Area2F* src,
                          const struct Point2F* dest_pos,
                          const struct Point2F* dest_size); /* 0047dcb0 */
void text_buffer_append_format(void* record, const char* format, ...); /* 00485230 */
void media_cd_audio_watchdog_tick(int cdtrack);            /* 00488c00 */

/* Original-ABI adapters (ADR 0005 step 6). The port's entries take its
 * engine or game context, other arguments, or do not exist; the adapters
 * are at the end of lego.cpp unless noted. */
int keep_running_original(void);                           /* 00435940 */
#if defined(_MSC_VER) && _MSC_VER < 1200 && !defined(__cplusplus)
/* Matching build: Lego_KeepRunning itself. lego_keep_running is already
 * declared through lego_keep_running_adapter's alias, so a function-like
 * macro after this declaration keeps every declaration, and with it the
 * symbol counter (finding 634, finding 639). */
#define keep_running_original() lego_keep_running()
#endif
void camera_move_pipeline_original(float elapsedWorld);    /* 0046a730 */
#if defined(_MSC_VER) && _MSC_VER < 1200 && !defined(__cplusplus)
/* Matching build: the originals themselves (finding 639): Lego.c's
 * Lego_CameraRegistryProbe and the tutorial icon pass in this unit,
 * Weapons_Update (weapons.c), ElectricFence_Update
 * (terrain_cell_objects.c) and NERPs' tutorial icon (tutorial_script.c,
 * whose third parameter is unused). */
#define camera_registry_probe_original lego_camera_registry_probe
#define simtick_sfx_bank_original weapons_update
#define simtick_post_cluster_original electric_fence_update
#define world_render_pass_a_original lego_tutorial_block_icons_update
#define world_render_pass_b_original nerps_draw_tutorial_icon
/* Sound3D_Update (sound3d.c); the matching build's DirectSound body does
 * not read the port's context parameter. */
#define simtick_sound_keepalive_original simtick_sound_keepalive
/* Panel_Crystals_Draw (panel_unit.c); like the original, it ignores the
 * frame time the caller passes. */
#define panel_crystals_draw_original panel_crystals_draw
#endif
void camera_registry_probe_original(void);                 /* 00435a50 */
void simtick_table_dispatch_original(float elapsed);       /* 0040aa10 */
void simtick_scene_pass_original(float elapsedWorld);      /* 0043b270 */
void simtick_sfx_bank_original(float elapsedWorld);        /* 0046f550 */
void view_tick_original(void* camera, Lego_Level* level, float elapsed,
                        float elapsedWorld);                /* 00435f5c */
void construction_update_all(float elapsedGame);           /* 004091e0 construction.c */
void simtick_render_step_original(void);                   /* 00449c00 */
void simtick_post_cluster_original(float elapsedWorld);    /* 0040d450 */
void simtick_fx_bank_tick_original(Lego_Level* level, float elapsedWorld); /* 0046e390 */
void simtick_sound_keepalive_original(void);               /* 0047aba0 */
void spiderweb_update(float elapsedGame);                  /* 004665c0 spiderweb.c */
void terrain_dirty_quad_normal_emit_pass_original(void);   /* 00463230 */
void screen_space_effect_dispatch_original(void* view);    /* 0040e8d0 */
void world_render_pass_a_original(Lego_Level* level, void* view,
                                  float elapsedWorld, float elapsed); /* 00435300 */
void world_render_pass_b_original(Lego_Level* level, void* view,
                                  float elapsedWorld, float elapsed); /* 00454600 */
void periodic_timer25_advance_original(float elapsedWorld, int renderTrack); /* 0043c2b0 */
void effects_update_original(float elapsed);               /* 0040e7f0 */
void panel_crystals_draw_original(unsigned int crystals,
                                  unsigned int usedCrystals,
                                  float elapsedWorld);      /* 0045c0d0 */
int objective_is_ended_original(void);                     /* 004589a0 */
int frontend_enabled_original(void);                       /* 00416d00 */
int overlay_menu_gate_original(float elapsed, int mode);   /* 00415370 */
int reward_level_start(void);                              /* reward.c, 00460220 */
void frontend_start_level_original(void);                  /* 00415220 */
/* At the end of sfx.cpp: the port's entry discards the volume. */
int sound_variant_property_read_original(int alias_index); /* 00465030 */



/* ---- group C ---- */

/* gods98 Input's mouse state (crt.cpp, input.cpp). The port types the
 * position and button words unsigned; the original reads them as
 * signed ints. */
extern uint32_t g_renderer_word_c;            /* crt.cpp, 0076ab3c mslb */
extern uint32_t s_window_mouse_word_40;       /* crt.cpp, 0076ab40 msrb */
extern int s_cursor_delta_x;                  /* input.cpp, 0076ab34 diffX */
extern int s_cursor_delta_y;                  /* input.cpp, 0076ab38 diffY */
#define legoInput_mouseX ((int)s_renderer_word_a)
#define legoInput_mouseY ((int)s_renderer_word_b)
#define legoInput_mslb g_renderer_word_c
#define legoInput_msrb s_window_mouse_word_40
extern int s_display_h;                       /* crt.cpp, 00505928 appHeight */
/* lego.cpp's stand-in for legoGlobs.radarZoom (005563f4, game/lego.h). */

struct EntitySlot;
struct TerrainMap;
struct TerrainMapVector3;
struct ViewContext;
struct ViewProjectionAdapter;

/* GAME-030 (entity_stats.cpp, entity.hpp). */
float entity_stats_float_at7c(const struct EntitySlot* e);       /* 00469db0 */
uint32_t entity_stats_word_get(const struct EntitySlot* e);      /* 00469e60 */
uint32_t entity_capability_flags_get(const struct EntitySlot* e); /* 00469ea0 */
/* Camera_TrackObject (00435ebb). The port's screen_effect_record_set takes
 * the float arguments as words; adapter at the end of camera_control.cpp. */
/* camera_control.cpp: Camera_StopMovement (00435f16),
 * Camera_GetTopdownPosition (00436b33; the port's entry takes the engine
 * context instead of the map: adapter at the end of camera_control.cpp)
 * and Camera_Move (00436b87). */
void cursor_screen_ray_context_reset(struct ViewContext* v);  /* 00435f16 */
int camera_topdown_position_get_adapter(void* camera, void* map,
                                        float out[3]);        /* 00436b33 */
void camera_pointer_pan_update(struct ViewContext* v, float vec[3],
                               float amount);                 /* 00436b87 */
/* legoobject_port.cpp (entity.hpp): LegoObject_GetPosition (00442260). */
void entity_transform_position_get_outputs(const struct EntitySlot* entity,
                                           float* out_x, float* out_y);
/* RadarMap_SetZoom (0045d8a0; the port's entry takes the float as a word):
 * adapter in level_id_remap.cpp. RadarMap_Draw (0045dbc0): radarmap.c's
 * definition takes this name in the matching build (radarmap_link_names.h,
 * finding 606); natively a no-op adapter at the end of lego_port.cpp. */
#if defined(_MSC_VER) && _MSC_VER < 1200
/* Matching build: the original itself, level_id_remap.cpp's 0045d8a0
 * (C linkage since finding 634), which stores the float. */
#define radar_map_zoom_set_adapter object_field14_set
#endif
void radar_map_zoom_set_adapter(void* radarMap, float zoom);
void radar_map_tv_position_set_adapter(void* radarMap, const float* pos);
/* NERPs' tutorial flags (00456240): the original passes the NERPs stack
 * argument; the port's system_state_flags_get takes none. Adapter at the
 * end of tutorial_script.cpp. */
/* record.hpp: SelectPlace_Hide (00464720). */
void surface_visibility_set(const void* record, uint32_t flag);
/* frontend.hpp: HelpWindow_HandleInput-style hit test (00418da0). */
int help_window_button_hit_test(uint32_t x, uint32_t y, int mode,
                                int transition_flags, int active,
                                float feedback_amount);
/* media.hpp: Pointer_SetCurrent_IfTimerFinished (0045cc20). */
void media_pointer_cursor_select(int slot);
/* crt.hpp: Pointer_GetCurrentType (0045cc60). */
uint32_t global_cursor_word_get(void);
/* GAME-026 Map3D.c (terrain_map.h). */
int terrain_map_screen_pick(struct TerrainMap* map, void* viewport,
                            unsigned int mx, unsigned int my,
                            unsigned int* bx, unsigned int* by,
                            struct TerrainMapVector3* intersection); /* 004502e0 */
/* view.hpp: Container_SetPosition (00475430). */
int ddiscene_object_position_set(void* object, void* alternate_frame,
                                 float x, float y, float z);
/* simtick.hpp (0044da20). */
void simtick_route_position_blocked_state_set(int blocked);
/* select.hpp / message_unit.cpp: Message_AnyUnitSelected (004525b0),
 * Message_GetPrimarySelectedUnit (004525c0), Message_IsUnitSelected
 * (00452610). */
int selection_active_has_any_get(void);
struct EntitySlot* selection_active_entity_get(void);
uint32_t selection_contains_entity_active_target(struct EntitySlot* entity,
                                                 uint32_t* index_out);
/* GAME-032 Panel.c (panel_unit.h): Panel_CheckCollision (0045adb0). */
int panel_cursor_interaction_update(float elapsed, int mouseX, int mouseY,
                                    int leftButton, int leftButtonLast,
                                    int* overPanel);
/* Priorities.c (priorities.h): Priorities_HandleInput (0045d110). */
int priorities_handle_cursor(unsigned int x, unsigned int y, int pressed,
                             int released, int apply);
/* Interface_HandleInput (0041baa0): the port's entry has another arity, in
 * a namespace; adapter at the end of interface.cpp. */
#if defined(_MSC_VER) && _MSC_VER < 1200
/* Matching build: the original itself, in Interface.c (interface_unit.c) (finding 634). */
#define interface_handle_mouse_adapter interface_do_mouse
#endif
int interface_handle_mouse_adapter(int x, int y, int leftButton,
                                   int leftButtonLast, int leftReleased,
                                   float elapsed);
/* Info_CheckCollision-style click (0041a1a0): the port's entry has another
 * arity, in a namespace; adapter at the end of info.cpp. */
#if defined(_MSC_VER) && _MSC_VER < 1200
/* Matching build: the original itself, in Info.c (info_unit.c) (finding 634). */
#define info_check_click_adapter ui_entry_hit_test_and_dispatch
#endif
int info_check_click_adapter(int x, int y, int leftReleased,
                             int rightReleased);
/* frontend.h: tooltip activation (0046b770). */
void timed_effect_update_request_set_corpus(int index);
/* objective.hpp (00453910) and tutorial_hud_ports.hpp (00453920). */
void objective_deferred_callback_invoke_if_set(void);
void tutorial_advisor_scroll_step_advance(void);
/* BuildingPlacement update (0044a910): the port's entry takes a hook
 * table; adapter at the end of lego.cpp. */
#if defined(_MSC_VER) && _MSC_VER < 1200
/* Matching build: the original itself, in LegoObject.c (legoobject_unit.c) (finding 634). */
#define building_placement_update_adapter legoobject_building_placement_preview_update
#endif
int building_placement_update_adapter(int type, int index, int leftReleased,
                                      int rightReleased,
                                      const float* pos,
                                      unsigned int bx, unsigned int by,
                                      int show, void* selectPlace);
/* entity.hpp: LegoObject at a screen point (0043b360), the selection
 * rectangle (0043b6c0), the selectable predicate (004385b0). */
int entity_task_at_grid_coordinate_find(uint32_t x, uint32_t y,
                                        struct EntitySlot** out_entity);
void entity_clipped_rectangle_callback_dispatch(void* payload,
                                                const float rect_a[2],
                                                const float rect_b[2]);
int entity_shared_eligibility_check(const struct EntitySlot* e,
                                    int relaxed);
/* media.hpp (004351a0). */
void media_action_code_to_sound_alias(int code);
/* GAME-021 part 2 (lego.cpp, cmd.hpp): 00431a30. */
int selected_entities_any_predicate_match(int cx, int cy,
                                          int require_stat_bit);
/* GAME-021 part 2 (lego.cpp, level.hpp): 00431da0. */
/* LegoObject_GetBlockPos (004424f0): the port's entry takes the level as
 * well; adapter at the end of lego.cpp. */
void entity_position_to_terrain_cell_adapter(const struct EntitySlot* entity,
                                             int* cell_x, int* cell_y);
/* AITask.c (cmd.hpp): AITask_DoClearTypeAction-style block command
 * (00402510; the port's entry takes x and y: adapter at the end of
 * aitask.cpp), AITask_Game_SetNoGather-style marker (00402000; the port's
 * payload is an int: adapter at the end of aitask.cpp), 004026c0 and
 * 00402960. */
void terrain_cell_command_create_adapter(const int* blockPos, int flag,
                                         int raise20);
void cmd_mark_matching_adapter(int type, struct EntitySlot* object,
                               const int* coords);
void entity_deferred_work_schedule(struct EntitySlot* e);
void entity_kind_command_issue(struct EntitySlot* e);
/* Message.c (message.h, cmd.hpp): Message_PostEvent (00452060). */
void cmd_produce(int op, const void* a1, int a2, const int* pair);
/* Interface.c (interface.h): 0041b2d0 and 0041e750. */
void command_context_post_to_hud(int kind, const uint32_t coord[2]);
void selection_clear_request_and_hud_reset(void);
/* frontend.hpp: 0041e990. */
int cell_action_permission_get(int column, int row);
/* level.hpp (encyclopedia.cpp): 0040e700. */
void terrain_choice_resource_select(const void* record);
/* view.hpp (viewports.cpp): Viewport_WorldToScreen (004772d0). */
void camera_project_point_to_screen(
    const struct ViewProjectionAdapter* adapter, float* output_xy,
    const float* input_xyz);



/* ---- group D ---- */

/* _strcmpi, malloc and free natively; vc5names.h has its own linkage. */
#ifdef __cplusplus
}
#endif
#include "crt/vc5names.h"
#ifdef __cplusplus
extern "C" {
#endif

struct BlockRec;
struct Lego_Object;
struct EntitySlot;
struct Lego_Level;

/* Globals the port annotates at the original's address. */
extern float g_camera_countdown;              /* camera.cpp, 00556ed8 */

/* G98-006 Config.c (gods98/config.h; BlockFile is struct BlockRec). */
/* Config.c's pooled "" and ":" (config.c, C linkage): the original's
 * literals resolve to these two addresses (findings 581, 626). */
extern const char s_config_rgb_separator[];                             /* 004a1cd4 ":" */
extern const char s_config_angle_empty[4];                              /* 004b8a14 "" */
const char* string_path_join(const char* root, ...);                    /* 00478f50 */
char* block_get_str_copy(struct BlockRec* root, const char* path);      /* 00479050 */
const char* block_get_strptr(struct BlockRec* root, const char* path);  /* 004790b0 */
int block_get_bool_path(struct BlockRec* root, const char* path);       /* 004790d0 */
int config_rgb_vector_read(struct BlockRec* root, const char* path,
                           float* r, float* g, float* b);              /* 00479170 */
/* G98 Utils.c (gods98/utils.h): Util_Tokenise. */
int sound_variant_split(char* text, char** out, const char* separators); /* 00477440 */
/* GAME-032 Panel.c (panel_unit.h) and GAME-034 Priorities.c (priorities.h). */
void mission_crystal_quota_setup(struct BlockRec* config, const char* gameName,
                                 const char* levelName);               /* 0045c040 */
int priorities_load(struct BlockRec* config, const char* game_name,
                    const char* level_name);                           /* 0045cf50 */
/* Message.c (message.h) and Interface.c (interface.h). */
void cmd_produce(int op, const void* a1, int a2, const int* pair);    /* 00452060 */
void command_context_post_to_hud(int kind, const uint32_t coord[2]);  /* 0041b2d0 */

/* Port entries with the original's contract (extern "C" in place). */
void level_no_gather_object_setup(int mode);           /* cmd.hpp, aitask.cpp 00402170 */
void level_state_stored_value_set(uint32_t v);         /* crt.hpp, 00409d40 */
int rock_fall_style_name_lookup(const char* name, uint32_t* index); /* resource.hpp, 0040bf70 */
void rockfall_style_table_base_set(uint32_t v);        /* crt.hpp, 0040bfc0 */
struct ElectricFenceLevel;
void electric_fence_restart(struct ElectricFenceLevel* level); /* terrain_cell_objects.h, 0040ce80 */
void erosion_timing_state_init(float value_a, float value_b,
                               float value_c);         /* level.hpp, 0040e930 */
void global_config_value_set(uint32_t v);              /* crt.hpp, 0040f5e0 */
void* sav_build_ctx_target(void);                      /* savegame.hpp, 00417e70 */
int save_slot_get_target(void);                        /* savegame.hpp, 00417e90 */
void mode_flag_pair_apply_if_bit2(int value);          /* frontend.hpp, 00418800 */
void incident_message_flag_set(int value);             /* message.hpp, 00419ae0 */
void render_config_six_field_copy_in_or_clear(const uint32_t in[6]); /* level.hpp, 004370d0 */
int entity_teardown(struct EntitySlot* entity);        /* entity.hpp, 00437540 */
void scatter_spawn_gate_set(int enabled);              /* spawn.hpp, 0044adc0 */
void string_table_entry_set_current(const char* name); /* resource.hpp, 0044dde0 */
int tutorial_script_file_load_target(const char* path); /* tutorial_hud_*.hpp (frozen; pre-declared), 00452df0 */
int objective_advisor_token_file_load(const char* path); /* objective.hpp, 00452e70 */
void tutorial_path_coordinate_index(struct LevelState* s); /* level.hpp, 00456b80 */
void global_value_004ffe48_set(uint32_t value);        /* crt.hpp, 00458660 */
void objective_announce_current(int who);              /* objective.hpp, 00458670 */
void objective_tick_target(void* engine, struct LevelState* s,
                           float checker_dt, float frame_dt); /* objective.hpp, 00458be0 */
void spiderweb_restart(Lego_Level* level);                /* spiderweb.c, 00466230 */
void global_linked_record_chain_clear(void);           /* crt.hpp, 0046a390 */

/* Original-ABI adapters (unannotated, ADR 0005 step 6) where the port's
 * contract differs, or where frozen headers declare the port's entry with
 * C++ linkage; each names the file that defines it. */
int entity_pool_find_by_callback_filtered_current_original(
    int (*callback)(struct EntitySlot*, void*), void* payload); /* lego.cpp, 004377b0 */
void* tile_model_entity_register_original(void* manager, const char* nameA,
                                          const char* nameB, float blockSize,
                                          void* textureSet); /* detail.c (matching build), 0040b5f0 */
void cell_visual_channel_table_dispose_original(void* table); /* detail.c (matching build), 0040b7d0 */
void options_apply_from_slot_original(void);           /* options.cpp, 00415150 */
void movie_play_level_video_adapter(const char* levelName, int flag); /* frontend_unit.cpp, 00415990 */
void* lego_object_create_adapter(void* objectData, int type, void* extra); /* lego.cpp, 00437d00 */
void simulation_update_counter_add_original(struct EntitySlot* object,
                                            int count); /* lego.cpp, 0043a8f0 */
void simulation_mode_counter_add_original(struct EntitySlot* object,
                                          int count); /* lego.cpp, 0043a910 */
void objective_load_original(struct BlockRec* config, const char* gameName,
                             const char* levelName, struct Lego_Level* level,
                             int screenWidth, int screenHeight); /* objective_unit.cpp, 00457d40 */
int osf_read_dirty_flag_get_original(void);            /* object_state_file.cpp, 004593d0 */
int osf_read_original(const char* path);               /* object_state_file.cpp, 00459470 */
int level_identifier_remap_table_load_original(const char* fileName,
                                               const char* gameName); /* level_id_remap.cpp, 0045d7e0 */
void renderer_fog_color_set_original(float r, float g, float b); /* lego.cpp, 00473e10 */
void renderer_fog_mode_set_original(unsigned int mode);          /* lego.cpp, 00473e70 */
void renderer_fog_parameters_set_original(float start, float end,
                                          float density);        /* lego.cpp, 00473ea0 */


/* ---- group E ---- */

struct EntitySlot;
struct LevelState;
struct BlockRec;
struct Point2F;
struct ViewProjectionAdapter;
struct TerrainMap;
struct ScenePairedAnimationState;
struct RenderResourceBinding;

/* legoGlobs fields and other modules' globals the port models. */
extern void* s_upgrade_records;               /* cmd.cpp, 005561e4 upgradeData */
extern uint32_t s_window_application_active;  /* frameloop.cpp, 00505808 */
/* legoGlobs' object-name tables and catalogue counts (0055618c,
 * 00556254-00556264, 0055627c): entity.cpp's globals, with C linkage (the
 * tables from the pre-declarations before entity_for_legoobject.hpp, the
 * counts from those before entity_for_lego.hpp). Deviation (scoring only):
 * the 15 declarations here and in group 2C cost the symbol counter 15
 * (finding 538, rule B); nine duplicate prototypes with 15 named
 * parameters were removed from this header to keep it unchanged. */
extern const char* s_object_name_special[9];
#define legoGlobs_objectNamesSpecial s_object_name_special
extern const char** s_object_name_kind1;
#define legoGlobs_objectNamesKind1 s_object_name_kind1
extern const char** s_object_name_kind2;
#define legoGlobs_objectNamesKind2 s_object_name_kind2
extern const char** s_object_name_kind3;
#define legoGlobs_objectNamesKind3 s_object_name_kind3
extern const char** s_object_name_kind4;
#define legoGlobs_objectNamesKind4 s_object_name_kind4
extern const char** s_object_name_kind10;
#define legoGlobs_objectNamesKind10 s_object_name_kind10
extern uint32_t s_catalog_counts[5];
#define legoGlobs_catalogCounts s_catalog_counts

/* GAME-030 Objective.c / stats (entity.hpp, objective.hpp). */
float entity_stats_float_atdc(const struct EntitySlot* e);      /* 00469e20 */
float entity_stats_float_at74(const struct EntitySlot* e);      /* 00469d50 */
float entity_stats_float_at78(const struct EntitySlot* e);      /* 00469d60 */
uint32_t entity_stats_word_at_c0_or_c4(const struct EntitySlot* e,
                                       int selector);           /* 00469f90 */
uint32_t entity_stats_word_atc8(const struct EntitySlot* e);    /* 00469fc0 */
int objective_voice_alias_raise_commit(struct LevelState* s);   /* 00454b70 */
int objective_voice_alias_raise_gate_bit27(struct LevelState* s); /* 00454b80 */
void reward_shutdown(void);                                     /* reward.c, 0045f750 */
/* Fallin / scene resources (resource.hpp). */
int scene_resource_availability_sweep(float delta, uint32_t** groups_out,
                                      uint32_t** slots_out);    /* 0040c3a0 */
void scene_cell_table_pair_read(int group, int slot, uint32_t* first,
                                uint32_t* second);              /* 0040c570 */
void rock_fall_style_scene_resources_release(void);             /* 0040c6c0 */
void global_resource_handle_release(void);                      /* 0044e0a0 */
void render_resource_track_and_retain(void* resource);          /* 00477220 */
void entity_payload_type1_release(
    struct ScenePairedAnimationState* state);                   /* 0046ce10 */
/* GAME-024 Message.c (message.h, select.hpp). */
void cmd_produce(int op, const void* a1, int a2, const int* pair); /* 00452060 */
struct EntitySlot* selection_active_entity_get(void);           /* 004525c0 */
struct EntitySlot* const* selection_active_records_get(void);   /* 004525e0 */
uint32_t selection_active_count_get(void);                      /* 004525f0 */
/* GAME-022 LegoObject.c (entity.hpp, ai.hpp). */
int entity_pool_find_by_callback_filtered_current(
    int (*callback)(struct EntitySlot*, void*), void* payload); /* 004377b0 */
uint32_t entity_type_operation_release(const struct EntitySlot* entity); /* 00449ff0 */
void entity_transform_position_get_outputs(const struct EntitySlot* entity,
                                           float* out_x,
                                           float* out_y);       /* 00442260 */
void* entity_type_component_get(const void* entity_record);     /* 0044a200 */
void ai_set_activity(struct EntitySlot* e, int id, int param);  /* 004409e0 */
int activity_register_bookkeeping(struct EntitySlot* e);        /* 00447c40 */
void ai_idle_chain_record_attach(struct EntitySlot* entity);    /* 00403f30 */
void pooled_record_pool_resources_release(void);                /* 00437050 */
void entity_payload_release_subslots(struct EntitySlot* e);     /* 00406ec0 */
void entity_payload_teardown(struct EntitySlot* e);             /* 00408940 */
void handle_array_release_and_reset(void);                      /* 004020c0 */
/* LegoObject_SetCustomName (00437880) and
 * LegoObject_MoveAnimation-style input (0044a310): adapters at the end of
 * lego.cpp (no port entry with C linkage). */
void lego_object_custom_name_set_adapter(struct EntitySlot* liveObj,
                                         const char* name);     /* 00437880 */
#if defined(_MSC_VER) && _MSC_VER < 1200
/* Matching build: the original itself (LegoObject.c's legoobject_rename),
 * through entity_custom_name_set_adapter's alias below. A function-like
 * macro keeps both declarations, so the symbol counter does not move
 * (finding 634). */
#define lego_object_custom_name_set_adapter(liveObj, name) \
    entity_custom_name_set_adapter(liveObj, name)
#endif
#if defined(_MSC_VER) && _MSC_VER < 1200
/* Matching build: the original itself, in LegoObject.c (legoobject_unit.c) (finding 634). */
#define lego_object_movement_input_adapter legoobject_movement_input_accumulate
#endif
void lego_object_movement_input_adapter(struct EntitySlot* liveObj,
                                        int forward, int strafe,
                                        float rotate);          /* 0044a310 */
/* GAME-033 Map3D (terrain_map.h). */
float terrain_map_world_z_get(struct TerrainMap* map, float x, float y); /* 0044f940 */
/* Lego.c parts 2-3 and others with no port equivalent of the original's
 * contract: adapters at the end of lego.cpp. */
#if defined(_MSC_VER) && _MSC_VER < 1200
/* Matching build: the original itself, in this unit (finding 634). */
#define lego_keep_running_adapter lego_keep_running
#endif
int lego_keep_running_adapter(void);                            /* 00435940 */
int lego_quit_poll_adapter(int a, int b, int* quit);            /* 004589c0 */
int lego_level_low_bits_enabled_adapter(void);                  /* 004589a0 */
/* Matching build: Objective.c's Objective_IsEnded (objective_unit.c,
 * finding 631), which takes the name objective_is_ended_original there. */
#if defined(_MSC_VER) && _MSC_VER < 1200
#define lego_level_low_bits_enabled_adapter objective_is_ended_original
#endif
void lego_fog_enable_adapter(int on);                           /* 00473dc0 */
void lego_fog_colour_set_adapter(float r, float g, float b);    /* 00473e10 */
#if defined(_MSC_VER) && _MSC_VER < 1200 && !defined(__cplusplus)
/* Matching build: lego_level_load's fog colour goes to Container.c's
 * Container_SetFogColour, which container.c serves under this name
 * (container_link_names.h). Function-like, after both declarations, so the
 * symbol counter does not move (finding 634). */
#define renderer_fog_color_set_original(r, g, b) lego_fog_colour_set_adapter(r, g, b)
#endif
void lego_light_penumbra_set_adapter(void* light, float value); /* 00473460 */
void lego_light_umbra_set_adapter(void* light, float value);    /* 00473480 */
/* GAME-002 / helpwindow (frontend.hpp). */
int main_sim_state_admission_predicate(void);                   /* 004191a0 */
void advisor_callback_schedule_gate(void);                      /* 0041c440 */
void state_flag_toggle_when_zero(void);                         /* 00407340 */
/* Camera (camera_control.h, view.hpp). */
void camera_pan_axis_update(void* camera, float delta);         /* 00436900 */
void camera_tilt_axis_update(void* camera, float delta);        /* 004369f4 */
void camera_zoom_update(void* camera, float delta);             /* 00436aa5 */
void camera_view_context_close(void* view);                     /* 00435dc8 */
void entity_b8_bit3_set_or_clear(void* record, int set);        /* 00435d91 */
/* Camera_Shake (00435f32): the port's entry takes the two floats as
 * words. Thin adapter at the end of camera_control.cpp. */
void entity_fields_a0_a4_store_a8_clear_original(void* camera, float a,
                                                 float b);      /* 00435f32 */
/* gods98 Viewports / Container / Draw / Font / TextWindow / Config. */
void camera_screen_coordinate_project(
    const struct ViewProjectionAdapter* adapter, float* output,
    const float* input);                                         /* 00477290 */
void camera_project_point_to_screen(
    const struct ViewProjectionAdapter* adapter, float* output_xy,
    const float* input_xyz);                                     /* 004772d0 */
int ddiscene_object_position_get(void* object, void* alternate_frame,
                                 float position[3]);             /* 004754c0 */
void ledger_stub_00472a40_DDiSceneObjectDestroy(void* scene_object); /* 00472a40 */
void ddiscene_object_pool_shutdown(void);                        /* 00472800 */
void scene_resource_slots_release_all(void);                     /* 00476d80 */
void scene_object_registry_teardown(void);                       /* 0047d430 */
void surface_visibility_set(const void* record, uint32_t flag);  /* 00464720 */
void surface_draw_polyline(struct Point2F* fromList,
                           struct Point2F* toList, uint32_t count,
                           float r, float g, float b, int effect); /* 00485ff0 */
uint32_t object_field_be4_dword_get(const void* record);         /* 0047a540 */
void font_atlas_record_release(void* record);                    /* 0047a550 */
void text_buffer_destroy_target(void* record);                   /* 00485190 */
void text_buffer_append_format(void* record, const char* format, ...); /* 00485230 */
/* DirectDraw_Clear (0047ce20, directdraw.h). */
void directdraw_clear(struct Area2F* window, unsigned int colour); /* 0047ce20 */
const char* string_path_join(const char* root, ...);             /* 00478f50 */
int block_get_bool_path(struct BlockRec* root, const char* path); /* 004790d0 */
void block_close(struct BlockRec* root);                         /* 00479240 */
void render_resource_binding_release(
    struct RenderResourceBinding* binding);                      /* 0046c0f0 */
void flic_opcode27_fallback_noop(void);                          /* 00484c90 */
/* Maths (advisor.h, view.hpp). */
float vec2_dist(const float* a, const float* b);                 /* 00401240 */
float* vec_cross(float* dst, const float* b, const float* c);    /* 00401470 */
float* vec_add(float* dst, const float* a, const float* b);      /* 00401500 */
float* vec_sub(float* dst, const float* a, const float* b);      /* 004016c0 */
float* vec_scale(float* dst, const float* src, float s);         /* 00401720 */
void vec_normalize(float* v);                                    /* 00401750 */
int display_width_get(void);                                     /* 00401c30 */
int display_height_get(void);                                    /* 00401c40 */
/* Sound (sound.hpp, sound3d.h, soundsys.h). */
int sound_submission_route_wrapper(const void* wrapped_position, int alias,
                                   int loop, int positional,
                                   const float* xyz);            /* 00465050 */
void audio_active_shutdown(void);                                /* 0047a800 */
int media_cd_audio_playback_available(void);                     /* 00488b90 */
/* Panel (panel_unit.h). */
void panel_item_flag16_set(int panel, int button, int on);       /* 0045aac0 */
/* LoaderProfile release (loading_progress.cpp): the original takes the
 * profile file name; the port's entry takes the parsed entries. Thin
 * adapter at the end of loading_progress.cpp. */
void loader_profile_entries_release_adapter(const char* profile); /* 0044dd40 */



/* ---- group F ---- */

/* legoGlobs fields the port models as globals of its own. */
/* GAME-028 NERPs (tutorial_script.cpp): NERPFunc__GetTutorialFlags. The
 * original takes the NERPs parameter stack; the port's entry takes none.
 * Thin adapter at the end of tutorial_script.cpp. */
uint32_t system_state_flags_get_original(const int* parameters); /* 00456240 */
/* GAME-021 part 3 (lego.cpp): Lego_GetLevel. */
struct LevelState* level_current_owner_get(void);         /* 00431560 */
/* LegoObject.c (legoobject_port.cpp, declared in camera.hpp). */
void* predug_spawn_coordinate_select_and_create(int source_object,
                                                int type); /* 0043ad50 */
/* LegoObject.c FUN_00438960: no port entry; the port's spawn probe hook.
 * Adapter at the end of lego.cpp. */
int spawn_registry_probe_adapter(int first, int second);  /* 00438960 */
/* G98 config.c (gods98/config.h). */
const char* string_path_join(const char* root, ...);       /* 00478f50 */
const char* block_get_strptr(struct BlockRec* root, const char* path); /* 004790b0 */
/* G98 Container_Load (gods98/entity.h): unannotated adapter in entity.cpp. */
void* container_load(void* parent, const char* filename,
                     const char* typeString, int looseLoad); /* 00472cd0 */
/* G98 Container_Hide (gods98/record.h, record.hpp). */
void scene_render_lock_toggle(void* scene_object, int enabled); /* 00473b40 */
/* GAME-008 (rock_fall_styles.cpp, resource.hpp): Effect_Load_Electric
 * FenceBeam. */
int indexed_resource_handle_set_create(void* scene, const char* path,
                                       int group);         /* 0040c1b0 */
/* GAME-008: the original takes the root container and the path; the
 * port's entry takes the path only. Adapter at the end of
 * rock_fall_styles.cpp. */
int global_resource_handle_load_and_enable_original(void* root,
                                                    const char* path); /* 0040c720 */
/* GAME-008: the original reads the MiscObjects block itself; the port's
 * entry takes parsed entries. Adapter at the end of rock_fall_styles.cpp. */
void misc_objects_effect_slots_bind_original(struct BlockRec* config,
                                             const char* gameName,
                                             void* root);   /* 0040c990 */
/* LegoObject.c (legoobject_port.cpp, entity.hpp): LegoObject_GetLangName and
 * Object_GetLangName. */
const char* entity_display_name_lookup(const struct EntitySlot* entity); /* 004395e0 */
const char* localized_object_name_get(uint32_t kind, int index); /* 00439860 */
/* LegoObject_SetCustomName: no port entry; adapter at the end of lego.cpp. */
void entity_custom_name_set_adapter(struct EntitySlot* object,
                                    const char* name);     /* 00437880 */
/* GAME-006 (screen_effect_quads.cpp, entity.hpp). */
int entity_quad_mesh_render_record_allowed(const struct EntitySlot* e); /* 0040aae0 */
/* LegoObject.c (legoobject_port.cpp, level.hpp): the object's tool tip SFX. */
int terrain_object_type_sfx_at_cell_get(const void* position_record); /* 0044ad80 */
/* Stats.c (entity_stats.cpp, ai.hpp): StatsObject_GetCapacity. */
int ai_activity_cap(const struct EntitySlot* e);          /* 0046a060 */
/* ToolTip.c (tooltip.cpp; frontend.h, cmd.hpp, the frozen frontend
 * headers): SetContent (unannotated adapter in frontend.cpp), AddIcon,
 * SetSFX, Activate. */
void tooltip_set_content(int index, const char* format, ...); /* 0046b520 */
void command_sample_stats_accumulate(int kind, const void* sample); /* 0046b660 */
void timed_effect_cue_request_set_corpus(int index, uint32_t value); /* 0046b700 */
void timed_effect_update_request_set_corpus(int index);   /* 0046b770 */
/* GAME-007 (placement.cpp, record.hpp): the resources delivered to a
 * building site. */
int count_matching_coordinates_in_list(const void* manager,
                                       int32_t coord_x, int32_t coord_y); /* 00408e10 */
/* Stats.c (entity_stats.cpp, entity.hpp): the building costs in crystals,
 * ore and studs. */
uint32_t entity_indexed_record_word_at_b0_get(int group, int index,
                                              int slot);   /* 00469ac0 */
uint32_t entity_indexed_record_word_at_b4_get(int group, int index,
                                              int slot);   /* 00469af0 */
uint32_t stats_table_word_at_b8(int type, int row, int stride_index); /* 00469b20 */
/* GAME-021 part 3 (lego.cpp, level.hpp): Lego_GetSurfaceTypeDescription
 * and Lego_GetSurfaceTypeSFX. */
const char* lego_terrain_surface_type_name_get(int surfaceType); /* 00435170 */
uint32_t lego_surface_type_configured_value_at_index(int surfaceType); /* 00435190 */
/* G98 Input (crt.cpp, crt.hpp): the mouse position and left button. */
/* GAME-033 Pointer.c (pointer.cpp, media.hpp): Pointer_SetCurrent_IfTimerFinished. */
void media_pointer_cursor_select(int slot);               /* 0045cc20 */
/* GAME-036 RadarMap.c (radarmap.c; map_view_port.cpp natively, view.hpp).
 * In the matching build radarmap.c defines these names
 * (radarmap_link_names.h, finding 606). Natively the port tail serves
 * them: ScreenToBlockPos's port entry takes two words, so its adapter at
 * the end of map_view_port.cpp stands in. TrySelectObject returns the
 * object as a word. */
struct TerrainPointPickRegion;
int rect_contains_point_half_open(const float* rect_record, int x, int y); /* 0045e660 */
int terrain_point_pick_resolve_original(const struct TerrainPointPickRegion* region,
                                        uint32_t screen_x, uint32_t screen_y,
                                        float out_world_xy[2],
                                        uint32_t* out_block_x,
                                        uint32_t* out_block_y); /* 0045e6d0 */
int viewport_point_callback_dispatch(const struct TerrainPointPickRegion* region,
                                     uint32_t screen_x, uint32_t screen_y,
                                     uint32_t* object_handle,
                                     float out_position_xy[2]); /* 0045e820 */
/* GAME-022 camera (camera_control.cpp, view.hpp). Camera_TrackObject takes
 * floats where the port's entry takes words: adapter at the end of
 * camera_control.cpp. */
struct ViewContext;
int view_grid_edge_get(void);                             /* 00435f28 */
void selection_record_point_get(struct ViewContext* v, float out[3]); /* 00436ae9 */
void selection_record_point_set(struct ViewContext* v, float x, float y); /* 00436b0b */
void screen_effect_record_set_original(void* camera, struct EntitySlot* object,
                                       float speed, float distance,
                                       float tilt, float rotation); /* 00435ebb */

/* The debug keys' callees (004288e0). */
struct LevelState;
/* GAME-005 (lava_erosion.cpp, level.hpp). */
void lava_erosion_neighbor_mutate(const int xy[2], unsigned int direction,
                                  int propagate);           /* 0040f330 */
/* LegoObject.c (legoobject_port.cpp, entity.hpp, spawn.hpp). */
void entity_enter_event21_state_once(struct EntitySlot* entity); /* 00444620 */
struct EntitySlot* creature_spawn_from_emergence(void* base_entity, int mode_token,
                                                 int owner_index, int cell_x,
                                                 int cell_y); /* 0043af30 */
void entity_clear_nested_state_8_fields(struct EntitySlot* e); /* 004487c0 */
void entity_action_sequence_reset(struct EntitySlot* entity,
                                  int preserve_mode);       /* 00441940 */
void object_list_field_assign_all(struct EntitySlot* entity,
                                  void* texture_resource);  /* 00440bf0 */
/* LegoObject.c entries the port keeps as namespaced ledger stubs, takes
 * with other contracts, or lacks: adapters at the end of lego.cpp. */
int nearest_open_cell_search_adapter(struct EntitySlot* entity, int* x, int* y,
                                     int requireDistance, int allowMaskC0,
                                     int allowFlag20);      /* 0044a5d0 */
struct EntitySlot* object_create_from_container_adapter(void* container,
                                                        int type, int id); /* 00437d00 */
#if defined(_MSC_VER) && _MSC_VER < 1200
/* Matching build: LegoObject_Create itself, by way of the alias of
 * lego_object_create_adapter (declared above). */
#define object_create_from_container_adapter(container, type, id) \
    ((struct EntitySlot*)lego_object_create_adapter((container), (type), (void*)(id)))
#endif
void entity_type_operation_dispatch_adapter(struct EntitySlot* entity,
                                            int operation); /* 00449f50 */
int entity_try_create_linked_at_position_original(struct EntitySlot* entity,
                                                  int x, int y,
                                                  struct EntitySlot* linked); /* 004471b0 */
int entity_pair_scalar_fields_set_original(struct EntitySlot* entity,
                                           const float* pair,
                                           float scalar);   /* 0044c150 */
void entity_damage_apply_original(struct EntitySlot* entity, float damage,
                                  int emitEffects, float elapsed); /* 0043f6a0 */
int level_debug_cell_retarget_adapter(struct EntitySlot* entity,
                                      struct EntitySlot* target); /* 004403d0 */
/* GAME-030 Message.c (message_unit.cpp, select.hpp). */
struct EntitySlot* const* selection_active_context_record_get(int* count_out); /* 00452580 */
int selection_active_has_any_get(void);                   /* 004525b0 */
struct EntitySlot* selection_active_entity_get(void);     /* 004525c0 */
struct EntitySlot* const* selection_active_records_get(void); /* 004525e0 */
uint32_t selection_active_count_get(void);                /* 004525f0 */
/* Stats.c (entity_stats.cpp, entity.hpp). */
void entity_record_flag400_toggle_and_refresh(struct EntitySlot* e); /* 0046a330 */
/* GAME-021 part 3 (lego.cpp, level.hpp). */
/* GAME-028 NERPs (tutorial_script.cpp, message.hpp): SetTutorialFlags. */
uint32_t system_state_flags_set_target(const uint32_t* value); /* 00456550 */
/* GAME-006 (screen_effect_quads.cpp, crt.hpp). */
void global_mode_zero_input_set(int value);               /* 0040ab10 */
/* G98 Container_SetPosition and Container_SetOrientation (view.hpp). */
int ddiscene_object_position_set(void* object, void* alternate_frame,
                                 float x, float y, float z); /* 00475430 */
int ddiscene_object_pose_method_dispatch(void* object, void* alternate_frame,
                                         float parameter_3, float parameter_4,
                                         float parameter_5, float parameter_6,
                                         float parameter_7,
                                         float parameter_8); /* 00475470 */
/* LightEffects.c (lighteffects.cpp; simtick.hpp, effects.hpp). */
void simtick_scene_position_reset_and_effect_motion_cancel(void); /* 0044cf70 */
void scene_transform_enable_set(int enabled);             /* 0044c790 */
/* Building.c and Vehicle.c upgrades (building_model.cpp, vehicle_model.cpp,
 * record.hpp, entity.hpp). Vehicle_SetUpgradeLevel's port entry takes the
 * record list and the channel state: adapter at the end of
 * vehicle_model.cpp. */
uint32_t task_flag_bit_query(const void* task, uint32_t bit, int suppress); /* 00408970 */
void entity_payload_state_set(struct EntitySlot* e, int state); /* 004089a0 */
uint32_t record_flag_bit_test(const void* record, uint32_t bit_index,
                              int reject_masked);           /* 0046cf40 */
void scene_record_indexed_entries_rebuild_original(void* vehicleModel,
                                                   int upgradeLevel); /* 0046cf90 */
/* AITask.c (aitask.cpp, cmd.hpp). The original 00402e40 takes the block
 * position by pointer: adapter at the end of aitask.cpp. */
void entity_command14_batch_issue(int count);             /* 00402800 */
void cell_command_type13_issue_original(const int* blockPos); /* 00402e40 */
/* ElectricFence.c (GAME-013, terrain_cell_objects.h; finding 566). */
struct ElectricFenceObject;
int electric_fence_spark_from(struct ElectricFenceObject* object, int bx, int by); /* 0040d4f0 */
int electric_fence_debug_place(int bx, int by);           /* 0040d170 */
int electric_fence_debug_remove(int bx, int by);          /* 0040d1f0 */
int electric_fence_block_is_fence(int bx, int by);        /* 0040e460 */
/* GAME-042 SpiderWeb.c (spiderweb.c). */
int spiderweb_spawn_at(int bx, int by);                   /* 00466250 */
/* Info.c (info.cpp, message.hpp). */
void message_dispatch(int id, const char* p2, uint32_t p3, const void* p4); /* 00419b80 */
/* Advisor.c and Panel.c (C units). */
void advisor_clear_active_state(void);                    /* 00401b20 */
int panel_advisor_animation_play(int panel, int button, int force); /* 0045c670 */



/* ---- group G ---- */

struct BlockRec;
struct EntitySlot;
struct PanelGlobs;
struct Point2F;
struct ResourceInterfaceWrapper;
struct TerrainMap;
struct ViewProjectionAdapter;

/* Globals the port models in other files. */
#if defined(_MSC_VER) && _MSC_VER < 1200
/* Text.c's textGlobs (text.c, 00503190; finding 607), as far as Lego.c
 * writes it: the NERPs message counter (00503610) and the message timer
 * (00503618). Natively the port's text_port.cpp word and lego_unit.c's own
 * word stand in for them. One struct and one extern replace the two
 * declarations of those words, so the unit's symbol numbering (finding 538)
 * does not change. */
struct Text_Globs {
    unsigned char reserved[0x480];
    uint32_t counter;                         /* +0x480 */
    char* currText;                           /* +0x484 */
    float timer;                              /* +0x488 */
};
extern struct Text_Globs textGlobs;
#define s_overlay_word_03610 textGlobs.counter
#define textGlobs_f_503618 textGlobs.timer
#else
extern uint32_t s_overlay_word_03610;         /* text_port.cpp, 00503610 */
#endif
#if defined(_MSC_VER) && _MSC_VER < 1200
/* Reward.c's rewardGlobs (reward.c, 00552980): the level-start pilot count
 * Lego.c stores is rewardGlobs.level.itemsArray[5].startCount (00555118).
 * Natively spawn.cpp's word stands in for it. One extern replaces the
 * other (finding 626). Deviation (scoring only): the extern views the
 * struct as words, because naming its tag would add one to the unit's
 * symbol numbering (finding 538). */
extern uint32_t rewardGlobs[];
#define s_start_level_pilot_grid_count (rewardGlobs[0x2798 / 4])
#else
/* spawn.cpp, 00555118 (namespace unported_ledger_stubs, C linkage). */
extern uint32_t s_start_level_pilot_grid_count;
#endif
/* GAME-032 Panel.c (panel_unit.h), 005000e0; panelTable[0].flags is at
 * +0x5c. */

/* G98 Main.c (frameloop.hpp). The triplet is Main_State's three callbacks. */
void window_title_set(const char* title);                       /* 004783d0 */
int window_runtime_callback_triplet_set(const size_t* triplet); /* 00477fd0 */
/* GAME-021 parts 2-3 and others still in lego.cpp / level.cpp. */
struct LevelState* level_current_owner_get(void);               /* 00431560 */
uint32_t entity_type_grid_record_count_get(const char* type_name,
                                           int slot);           /* 00455300 */
/* 00431570 takes no argument; the port's entry takes two ignored cells.
 * Adapter at the end of lego.cpp. */
void* level_current_terrain_get_original(void);                 /* 00431570 */
#if defined(_MSC_VER) && _MSC_VER < 1200
/* Matching build: Lego_GetMap itself. lego_unit.c's definition keeps the
 * port's two ignored cells, so a call without them is the original's. */
#define level_current_terrain_get_original() \
    (((void* (*)())level_current_terrain_get)())
#endif
/* 00433e80 (Lego.c part 3) has no port entry with the original's
 * contract. Adapter at the end of lego.cpp; in the matching build, the
 * unit's own function (part 3, below in lego_unit.c). */
#if defined(_MSC_VER) && _MSC_VER < 1200
#define terrain_projected_cell_state_refresh_original lego_terrain_projected_cell_state_refresh
#endif
void terrain_projected_cell_state_refresh_original(void* container,
    void* view, float distance, unsigned int count);            /* 00433e80 */
/* (entity.hpp, namespace unported_ledger_stubs, C linkage). */
void entity_attached_object_list_detach(struct EntitySlot* entity,
                                        int reset_mode);        /* 00439e70 */
/* 0046a510 takes (selector, mode, flags); the port's entry adds the game
 * context and a callback. Adapter at the end of record_chains.cpp. */
void scene_object_state_dispatch_original(uint32_t selector, uint32_t mode,
                                          uint32_t flags);      /* 0046a510 */
/* GAME Interface.c (cmd.hpp, interface.h). */
void selection_clear_request_and_hud_reset(void);               /* 0041e750 */
/* GAME-032 Panel.c (panel_unit.h). */
void panel_state_flags_toggle(int panelType);                   /* 0045ab00 */
/* G98 Sound3D.c (sound.hpp, sound3d.h). */
void sound_subsystem_stop_all(void);                            /* 0047b160 */
void sound_tracked_buffers_set_min_distance(float value);       /* 0047b4a0 */
/* GAME SFX.c (sound.hpp). */
void sound_subsystem_enable_set_and_stop(int enable);           /* 004652a0 */
void transient_effect_enable_set(void* instance, int enabled);  /* 00465ac0 */
/* Advisor/Text (engine.hpp, heap.hpp, textwindow.h). */
int engine_mode_set_target(const int* mode);                    /* 004565f0 */
void text_buffer_reset_target(void* record);                    /* 004859b0 */
/* G98 Config.c and Utils.c (config.h, utils.h). */
const char* string_path_join(const char* root, ...);            /* 00478f50 */
struct BlockRec* block_subgroup(struct BlockRec* root,
                                const char* path);              /* 00478ff0 */
struct BlockRec* block_next_equal_key(const struct BlockRec* start); /* 00479020 */
char* string_duplicate(const char* text);                       /* 00477550 */
/* G98 Container.c (view.hpp). */
int ddiscene_object_position_get(void* object, void* alternate_frame,
                                 float position[3]);            /* 004754c0 */
int ddiscene_object_facing_pair_get(void* object, void* alternate_frame,
                                    float first[3],
                                    float second[3]);           /* 00475500 */
void global_entity_render_parameter_mode_set(int mode);         /* 0046e1c0 */
/* G98 Viewports.c (view.hpp, scene_resources.hpp, resource.hpp). */
void resource_interface_call_slot48(void* wrapper, void* value); /* 00476f50 */
void renderer_object_set_clip_distance(void* object, float distance); /* 00476fb0 */
/* 00477240 takes the field of view as a float; the port's entry stores a
 * pointer. Adapter at the end of viewports.cpp. */
void scene_current_object_set_original(void* view, float field); /* 00477240 */
void resource_interface_call_slot40_and_clear_field14(
    struct ResourceInterfaceWrapper* wrapper, uint32_t value);  /* 00477250 */
void camera_screen_coordinate_project(
    const struct ViewProjectionAdapter* adapter, float* output,
    const float* input);                                        /* 00477290 */
void camera_project_point_to_screen(
    const struct ViewProjectionAdapter* adapter, float* output_xy,
    const float* input_xyz);                                    /* 004772d0 */
/* G98 Maths.c (maths.h). */
short rng_thunk_draw15(void);                                   /* 004798a0 */
int view_ray_intersection_test(float* endPoint, const float* rayOrigin,
                               const float* ray, const float* planePoint,
                               const float* planeNormal);       /* 004798e0 */
/* G98 Draw.c (draw.h). */
void surface_draw_polyline(struct Point2F* fromList, struct Point2F* toList,
                           uint32_t count, float r, float g, float b,
                           int effect);                         /* 00485ff0 */
/* G98 Sound.c CD audio (soundsys.h). */
void media_cd_audio_track_start(int track, int mode,
                                void (*callback)(void));        /* 00488bb0 */
void media_cd_audio_stop_and_close_if_active(void);             /* 00488bf0 */
/* GAME-024 camera (record.hpp). The original takes the tracked object as
 * a pointer; the port's entry types it as a word. */
void record_fields_4_and_14_set(void* record, uint32_t value_a,
                                uint32_t value_b);              /* 00435ef4 */
/* GAME Map3D.c (terrain_map.h). */
int terrain_map_world_to_block(struct TerrainMap* map, float x, float y,
                               int* bx, int* by);               /* 0044f6d0 */
void terrain_map_visible_radius_add(struct TerrainMap* map, int bx, int by,
                                    int radius,
                                    void (*callback)(int x, int y)); /* 00450600 */
float terrain_context_scale_get(const void* context);           /* 00450890 */
void terrain_map_wobble_emissive_set(struct TerrainMap* map, int on); /* 00450a80 */

/* ---- GAME-021 part 2 (issue #574) ---- */

/* ---- part 2 imports, group 2A (0042b510-0042c440): begin ---- */

struct UiRenderRecord;
struct TerrainCellResourceGrid;
/* GAME Map3D.c (terrain_map.h). */
struct TerrainMap* terrain_map_create(void* root, const char* filename,
                                      float blockSize, float roughLevel); /* 0044e0c0 */
void terrain_map_texture_no_fade_set(struct TerrainMap* map,
                                     unsigned char texture);  /* 0044e6b0 */
void terrain_map_vertex_normals_update_all(struct TerrainMap* map); /* 0044e880 */
void terrain_map_fade_pair_add(struct TerrainMap* map, unsigned char texA,
                               unsigned char texB);           /* 0044f1a0 */
void terrain_map_texture_set_set(struct TerrainMap* map, void* textureSet); /* 0044f220 */
void terrain_map_perspective_correction_set(struct TerrainMap* map, int on); /* 0044f490 */
/* RadarMap_Create (radarmap.c; map_view_port.cpp natively, frontend.hpp):
 * the zoom passes as the same stack word. */
struct UiRenderRecord* ui_render_record_create(void* source,
                                               const uint32_t* payload,
                                               uint32_t extra); /* 0045da90 */
/* Roof (resource.hpp). */
int indexed_surface_resource_allocate(void* owner, int width, int height); /* 00462da0 */
int animated_texture_replace(const char* name);               /* 00462e60 */
/* Detail_LoadTextureSet (detail.c in the matching build, terrain_grid.hpp). */
struct TerrainCellResourceGrid* terrain_cell_resource_grid_load(
    const char* bitmap_prefix, uint32_t width, uint32_t height); /* 0040b310 */
/* GAME-005 erosion (level.hpp). */
int lava_erosion_stage_register(const int xy[2], int stage); /* 0040ea10 */
/* SpiderWeb/predug coordinate pool (legoobject_port.cpp, level.hpp). */
void predug_coordinate_pool_append(const int cell_xy[2]);   /* 0044a4b0 */
/* Smoke/cell passes (cell_flood_passes.cpp, terrain_grid.hpp). */
void terrain_cell_quad_run_stage_level(void* scene,
                                       struct LevelState* level); /* 0046db90 */
/* Map files (files.cpp, lego.cpp, g98_024.c; level.hpp). */
int map_open2(const char* path, int32_t* size_out);         /* 00480170 */
void level_map_dimensions_get(uint32_t slot, uint32_t* width,
                              uint32_t* height);             /* 004334c0 */
void level_map_slot_release(uint32_t slot);                  /* 00489460 */

/* ---- part 2 imports, group 2A: end ---- */

/* ---- part 2 imports, group 2B (0042c480-0042d460): begin ---- */


/* GAME-021 part 3 still in lego.cpp, and gods98 Files.c (level.hpp,
 * extern "C" in place). */
/* LegoObject.c (spawn.hpp, ai.hpp; extern "C" in place). */
void type11_cell_coordinate_append(const int* coordinate);      /* 0044a390 */
void entity_activity_scene_state_update_retail(void);           /* 00439990 */
/* AITask.c (cmd.hpp, extern "C" in place). */
void coordinate_pair_callback_dispatch(const void* coord_pair); /* 00402a40 */
void coordinate_pair_event_queue(const int pair[2], void* payload); /* 004030b0 */
/* GAME-012 (level.hpp, extern "C" in place; placement.cpp). */
int terrain_cell_predicate_check_current(const uint32_t pair_key[2]); /* 004090a0 */
int lava_erosion_cell_eligible_and_mutate(const int xy[2], int hint); /* 0040f190 */
/* GAME-019 Info.c (info_unit.c, a C unit). */
struct InfoPoint2I;
void info_send(int type, char* text, void* object,
               struct InfoPoint2I* blockPos);                   /* 00419b80 */
#if defined(_MSC_VER) && _MSC_VER < 1200
/* Matching build: the port's message_dispatch (declared above) is Info_Send
 * itself, with the original's arguments. */
#define message_dispatch(type, text, object, blockPos) \
    info_send((type), (char*)(text), (void*)(object), (struct InfoPoint2I*)(blockPos))
#endif
/* Vehicle.c, Creature.c, Building.c and Upgrade.c (record.hpp, view.hpp,
 * type_config_ports.hpp; extern "C" in place). */
void scene_resource_visibility_set(void* record, uint32_t flag);   /* 0046d2c0 */
void animation_object_visibility_set(const void* anim, uint32_t flag); /* 00406c90 */
int entity_building_type_config_record_parse(void* destination,
                                             uint32_t row_index,
                                             void* scene_context,
                                             const char* asset_path,
                                             void* engine_config); /* 00407d60 */
void scene_object_pair_apply_mode(const void* record, uint32_t flag); /* 00408670 */
void render_resource_binding_initialize(struct RenderResourceBinding* binding,
                                        void* key, void* config_a,
                                        void* config_b);        /* 0046c030 */
/* Original-ABI adapter at the end of lego.cpp (group 2B). */
void lego_noop_block_adapter(const int* blockPos);              /* 00484c90 */
#if defined(_MSC_VER) && _MSC_VER < 1200
/* Matching build: the original passes the block to the shared no-op. */
#define lego_noop_block_adapter(blockPos) \
    (((void (*)())flic_opcode27_fallback_noop)(blockPos))
#endif
int lego_vehicle_load_adapter(void* vehicle, unsigned int objID, void* root,
                              const char* filename,
                              const char* gameName);            /* 0046c6f0 */
#if defined(_MSC_VER) && _MSC_VER < 1200
/* Matching build: the original itself, in animation_object.c; its sixth parameter is unused (finding 634). */
#define lego_creature_load_adapter entity_type_config_resource_row_load
#endif
int lego_creature_load_adapter(void* creature, unsigned int objID, void* root,
                               const char* filename,
                               const char* gameName);           /* 00406990 */

/* ---- part 2 imports, group 2B: end ---- */

/* ---- part 2 imports, group 2C (0042d600-0042dff0): begin ---- */

/* gods98 Util_StrCpy (00477590; utils.c). */
char* formatted_text_escape_decode_and_emit(const char* format, ...);
/* legoGlobs' "the" object-name tables (005561b0, 00556268-00556278):
 * entity.cpp's globals, with C linkage like the tables in group E. */
extern const char* s_object_the_name_special[9];
#define legoGlobs_objectTheNamesSpecial s_object_the_name_special
extern const char** s_object_the_name_kind1;
#define legoGlobs_objectTheNamesKind1 s_object_the_name_kind1
extern const char** s_object_the_name_kind2;
#define legoGlobs_objectTheNamesKind2 s_object_the_name_kind2
extern const char** s_object_the_name_kind3;
#define legoGlobs_objectTheNamesKind3 s_object_the_name_kind3
extern const char** s_object_the_name_kind4;
#define legoGlobs_objectTheNamesKind4 s_object_the_name_kind4
extern const char** s_object_the_name_kind10;
#define legoGlobs_objectTheNamesKind10 s_object_the_name_kind10
/* legoGlobs.recordObjects (005563b8, 10 entries) and recordObjectCount
 * (005563e0): entity.cpp's s_object_pointer_registry_words/_count, with
 * C linkage (the pre-declarations before entity_for_lego.hpp). */
extern uint32_t s_object_pointer_registry_words[10];
#define legoGlobs_recordObjects ((void**)s_object_pointer_registry_words)
extern uint32_t s_object_pointer_registry_count;
#define legoGlobs_recordObjectCount s_object_pointer_registry_count
/* Deviation (scoring only): in the original the count and the array are
 * members of one struct (legoGlobs), so a store into the array may alias
 * the count and MSVC reloads it after each store (0042dfc0). As separate
 * globals they cannot alias; a volatile read gives the same reload. */
#define legoGlobs_recordObjectCountReloaded \
    (*(volatile uint32_t*)&s_object_pointer_registry_count)

/* Map3D_BlockToWorldPos (0044f640; terrain_map.c). */
int terrain_map_block_to_world(struct TerrainMap* map, unsigned int bx,
                               unsigned int by, float* xOut, float* yOut);
/* ---- part 2 imports, group 2C: end ---- */

/* ---- part 2 imports, group 2D (0042e020-0042ef40): begin ---- */

/* legoGlobs.recordObjects/recordObjectCount (005563b8/005563e0): group
 * 2C's macros above. */

/* GAME Map3D.c (terrain_map.h). */
/* GAME-024 camera (view.hpp): Camera_SetTilt and Camera_SetRotation. */
void camera_tilt_set(void* camera, float value);                /* 00436860 */
void camera_rotation_set(void* camera, float value);            /* 00436953 */
/* GAME Building.c (building_model.h): Building_GetShapePoints. */
uint32_t task_field84_get_and_field88_out(const void* task,
                                          uint32_t* out88);     /* 00408360 */
/* GAME Construction.c (placement_footprint.h): the shape rotated to a
 * direction around a block. */
void* rotated_pairs_scratch_fill(const int* offset_pair, const int* source,
                                 int count, int rotation);      /* 00464110 */
/* GAME-022 LegoObject.c (entity.hpp, ai.hpp, cmd.hpp). */
void entity_kind_record_update_dispatch(struct EntitySlot* entity); /* 00438460 */
void entity_queue_for_rockfall_processing_if_eligible(struct EntitySlot* e); /* 0043a9e0 */
#if defined(_MSC_VER) && _MSC_VER < 1200
/* Matching build: the original itself, in LegoObject.c (legoobject_unit.c) (finding 634). */
#define entity_linked_activity_event_dispatch legoobject_linked_activity_event_dispatch
#endif
void entity_linked_activity_event_dispatch(struct EntitySlot* entity,
                                           uint32_t events);    /* 0043f5b0 */
void entity_world_position_set(struct EntitySlot* entity, float x, float y,
                               float heading, int update_heading); /* 004428a0 */
void cmd_create_task(struct EntitySlot* e, int32_t parameter);  /* 00402720 */
int entity_state_override_pop(struct EntitySlot* entity);       /* 00459240 */
/* G98 Container.c (view.hpp): Container_SetActivity and
 * Container_SetAnimationTime. */
uint32_t scene_action_name_set(void* scene_object, const char* action_name); /* 00473370 */
float ddiscene_frame_update_and_callbacks(void* scene_object, float time); /* 00475140 */
/* GAME-042 SpiderWeb.c (spiderweb.c): SpiderWeb_Add. */
void spiderweb_add(int bx, int by, struct Lego_Object* webObj); /* 00466380 */
/* GAME-013 ElectricFence.c (terrain_cell_objects.h): ElectricFence_Add. */
void electric_fence_add(struct ElectricFenceObject* object);   /* 0040cf50 */
/* Original-ABI adapters at the end of lego.cpp (group 2D), where the
 * port's entry takes another contract. */
#if defined(_MSC_VER) && _MSC_VER < 1200
/* Matching build: the original itself, in LegoObject.c (legoobject_unit.c) (finding 634). */
#define deferred_olist_entity_queue_add_original legoobject_deferred_olist_queue_add
#endif
void deferred_olist_entity_queue_add_original(void* objModel, int objType,
                                              unsigned int objID,
                                              const float* position,
                                              float heading, float health,
                                              const char* olName,
                                              const char* drivingName); /* 00437c20 */
void* construction_place_building(int objID, int* blockPos,
                                  unsigned int direction, int* shapePoints,
                                  unsigned int shapeCount,
                                  int teleportDown);           /* 00409a90 construction.c */
void building_upgrade_state_changed_dispatch(int type, int row, int column,
                                             int state); /* 00418830 helpwindow.c */
#if defined(_MSC_VER) && _MSC_VER < 1200
/* Matching build: the original itself, in LegoObject.c (legoobject_unit.c) (finding 634). */
#define entity_linked_route_flag_clear_original legoobject_linked_route_flag_clear
#endif
void entity_linked_route_flag_clear_original(struct EntitySlot* object,
                                             struct EntitySlot* driven); /* 0043f100 */


/* ---- part 2 imports, group 2D: end ---- */

/* ---- part 2 imports, group 2E (0042efc0-0042f6f0): begin ---- */

/* G98 Movie.c (gods98/movie_unit.h, G98-001). movie_load's return type
 * declares the G98CMovie tag; struct tagRECT is Movie_Update's RECT.
 * Deviation (scoring only): movie_load's parameter is unnamed. Named, it
 * moves MSVC 5.0's symbol counter so that 0x00431450 falls from 100% to
 * 83.78% and 0x004311d0 from 70.80% to 67.15% (finding 538 rule B). */
struct tagRECT;
struct G98CMovie* movie_load(const char*);               /* 00472560 */
void movie_free(struct G98CMovie* mov);                         /* 004726f0 */
void movie_get_size(struct G98CMovie* mov, unsigned int* width,
                    unsigned int* height);                      /* 00472670 */
int movie_update(struct G98CMovie* mov, float speed,
                 struct tagRECT* destRect);                     /* 004726d0 */
/* G98 Main.c (frameloop.hpp). */
void main_loop_frame_pump(int draw_projection_overlay);         /* 00477f30 */
/* DirectDraw_ReturnFrontBuffer (0047cc20, directdraw.h). */
void directdraw_return_front_buffer(void);                      /* 0047cc20 */

/* Level_Free's callees. */
extern uint32_t s_entity_state_flags;          /* legoobject_port.cpp, 004dee14 */
extern int s_cell_refresh_track_count;         /* level.cpp, 00556e8c */
extern int s_cell_previous_track_count;        /* level.cpp, 00556e90 */
/* 00435aa0 (Lego.c part 3): the port's entry also takes the
 * EngineContext, which it does not read. Adapter at the end of lego.cpp. */
#if defined(_MSC_VER) && _MSC_VER < 1200
/* Matching build: the original itself, in this unit (finding 634). */
#define lego_panel_selection_highlight_toggle_adapter lego_panel_selection_highlight_toggle
#endif
void lego_panel_selection_highlight_toggle_adapter(int on);     /* 00435aa0 */
void camera_distance_set(void* camera, float value);            /* view.hpp, 00436a4a */
void transient_effect_registry_release_all(void);               /* effects.hpp, 00465970 */
void global_state_low_byte_clear_and_refresh(void);             /* lego.cpp, 00435a20 */
void ui_message_overlay_state_initialize(void);                 /* message.hpp, 0046aa90 */
void selection_state_flag_clear_bit0(void);                     /* frontend.hpp, 004181a0 */
void grid_record_flag_mask_clear(void);                         /* level.hpp, 0040ab30 */
/* 00456c60 takes no argument; the port's entry takes the pool table.
 * Adapter at the end of lego.cpp. */
void lego_object_pool_lists_free_all_adapter(void);             /* 00456c60 */
void cmd_flush_all(int disable_mode);                           /* cmd.hpp, 00402100 */
void spawn_coordinate_pool_clear(void);                         /* level.hpp, 004372a0 */
void path_record_release_all(void);                             /* level.hpp, 004099d0 */
void ledger_stub_0040bdc0_SetTimeoutTo10000(void);              /* rock_fall_styles.cpp, 0040bdc0 */
void graphics_overlay_resources_release(void);                  /* resource.hpp, 00462ed0 */
void selection_active_count_reset(void);                        /* select.hpp, 00452600 */
int tutorial_display_resources_release(void);                   /* tutorial_hud_ports.hpp, 00453220 */
void ui_render_record_free(void* record);                       /* heap.hpp, 0045daf0 */
void terrain_map_remove(struct TerrainMap* map);                /* terrain_map.h, 0044e670 */
void terrain_cell_resource_grid_dispose(struct TerrainCellResourceGrid* grid); /* terrain_grid.hpp, 0040b410 */
void scene_resource_singleton_pool_release(void);               /* resource.hpp, 0040c690 */
void pending_record_release_all(void);                          /* info_unit.h, 00419f10 */
void interface_reset(void);                                     /* interface_unit.h, 0041a920 */
void legacy_panel_category_interaction_state_reset(void);       /* panel_unit.h, 0045a270 */
void advisor_animation_end_cleanup(void);                       /* advisor.h, 004011c0 */
void info_enable(int on);                                       /* info_unit.h, 00419ae0 */
#if defined(_MSC_VER) && _MSC_VER < 1200 && !defined(__cplusplus)
/* Matching build: Info_Enable itself (lego_level_load), after both
 * declarations so the symbol counter does not move. */
#define incident_message_flag_set(value) info_enable(value)
#endif
void screen_effect_flags_clear_and_release(void);               /* frontend.hpp, 0040a3d0 */

/* Level_BlockUpdateSurface's callees. */
void entity_task_counter_increment_by_branch(int nonzero);      /* entity.hpp, 004374d0 */
void terrain_cell_refresh_dispatch_current(const int cell_xy[2], int mode); /* level.hpp, 00402ae0 */
/* The empty function at 00484c90, called with the block position. */
void lego_noop_point_adapter(const int* blockPos);              /* 00484c90 */
#if defined(_MSC_VER) && _MSC_VER < 1200
/* Matching build: the original passes the block to the shared no-op
 * (finding 634). */
#define lego_noop_point_adapter(blockPos) \
    (((void (*)())flic_opcode27_fallback_noop)(blockPos))
#endif
void selection_mode7_cancel_at_point(const uint32_t coordinate[2]); /* interface_unit.h, 0041cf80 */
int electric_fence_stud_fits(struct ElectricFenceLevel* level, int bx, int by); /* terrain_cell_objects.h, 0040e1e0 */
void electric_fence_stud_set(struct ElectricFenceLevel* level, int bx, int by,
                             int on);                           /* terrain_cell_objects.h, 0040e350 */
void lava_erosion_pending_cell_add(const int xy[2], int add);   /* level.hpp, 0040ee50 */
void terrain_map_block_uv_wobble_set(struct TerrainMap* map, unsigned int bx,
                                     unsigned int by, int on);  /* terrain_map.h, 00450960 */
/* sfx.cpp's entry types the ninth argument (a float) as a word. */
void* transient_effect_emitter_create(void* scene_context, uint32_t position_x,
                                      uint32_t position_y, int* direction,
                                      float red, float green, float blue,
                                      float rate, int parameter_9, int mode,
                                      uint32_t seed, int auxiliary); /* effects.hpp, 00465430 */
void terrain_map_vertex_position_get(struct TerrainMap* map, unsigned int vx,
                                     unsigned int vy, float* x, float* y,
                                     float* z);                 /* terrain_map.h, 004507d0 */
void terrain_map_block_rotated_set(struct TerrainMap* map, unsigned int bx,
                                   unsigned int by, int on);    /* terrain_map.h, 0044edf0 */
void coordinate_callback_dispatch_current(const void* coord_pair); /* level.hpp, 00402b30 */
void terrain_map_block_highlight_clear(struct TerrainMap* map, int bx, int by); /* terrain_map.h, 0044f570 */
uint32_t render_feature_flags_get(void);                        /* crt.hpp, 00477f70 */
void terrain_map_block_texture_fade_set(struct TerrainMap* map, unsigned int bx,
                                        unsigned int by, unsigned char texture,
                                        unsigned char direction); /* terrain_map.h, 0044e6d0 */
void terrain_map_block_texture_set(struct TerrainMap* map, unsigned int bx,
                                   unsigned int by, unsigned char texture,
                                   unsigned char direction);    /* terrain_map.h, 0044f230 */
/* 00433380 (Lego.c part 3, lego.cpp); the port's LevelState is the
 * original's level record in the matching build. */
void terrain_cell_mutation_progress(struct LevelState* level, uint32_t x,
                                    uint32_t y);                /* level.hpp, 00433380 */

/* ---- part 2 imports, group 2E: end ---- */

/* ---- part 2 imports, group 2F (004302b0-00431570): begin ---- */

struct WorldSpatialRecordModel;
/* AITask.c (cmd.hpp, level.hpp). */
/* placement.cpp (level.hpp, port_loop_ports.hpp). */
void terrain_cell_refresh_eight_neighbors_current(const int cell_xy[2]); /* 00409940 */
/* GAME-021 part 3 (lego.cpp, level.hpp): the same contract as the
 * original's, with the port's level view. */
/* LegoObject.c (legoobject_port.cpp: ai.hpp, entity.hpp, level.hpp,
 * port_loop_ports.hpp). */
void deferred_olist_entity_create_and_link(const int cell_xy[2]);     /* 00437940 */
void ledger_stub_0044b140_SmallSpiderCellSwarmSpawn(int cell_x, int cell_y,
                                                    uint32_t variation); /* 0044b140 */
/* 00439370 and 004394b0 are declared only in legoobject_port.cpp's ledger
 * namespace. Adapters at the end of lego.cpp. */
#if defined(_MSC_VER) && _MSC_VER < 1200
/* Matching build: the original itself, in LegoObject.c (legoobject_unit.c) (finding 634). */
#define entity_build_placement_setup_original legoobject_build_placement_command_setup
#endif
void entity_build_placement_setup_original(const int cell_xy[2], int entity_id,
                                           const float world_xy[2],
                                           int update_state);   /* 00439370 */
#if defined(_MSC_VER) && _MSC_VER < 1200
/* Matching build: the original itself, in LegoObject.c (legoobject_unit.c) (finding 634). */
#define entity_build_placement_alt_setup_original legoobject_build_placement_alternate_command_setup
#endif
void entity_build_placement_alt_setup_original(const int cell_xy[2], int entity_id,
                                               const float world_xy[2],
                                               int update_state); /* 004394b0 */
/* GAME-026 Map3D.c (terrain_map.h). */
void terrain_map_block_vertex_modified(struct TerrainMap* map, unsigned int vx,
                                       unsigned int vy);        /* 0044efb0 */
void terrain_map_vertex_move(struct TerrainMap* map, unsigned int vx,
                             unsigned int vy, float zDist);     /* 0044f380 */
/* reward.c (reward.h). */
void global_float_at_00554adc_add_one(void);                          /* 00462830 */
void global_float_at_00554efc_add_one(void);                          /* 00462850 */
/* G98 (terrain_grid.hpp, view.hpp). */
void terrain_cell_surface_quad_height_fill_current(struct LevelState* level,
                                                   int x, int y); /* 0046e220 */
void terrain_cell_surface_quad_reset_current(int x, int y);           /* 0046e330 */
struct WorldSpatialRecordModel* world_spatial_record_find_at_grid_coordinate(
    int x, int y, uint32_t* point_index);                       /* 0046e8a0 */
/* G98 Container.c (view.hpp). */
void* entity_visual_resource_load(void* scene_object);                /* 00473690 */
float ddiscene_advance_by_time_target(void* scene_object, float delta); /* 00475120 */
/* GAME-019 Info.c (info_unit.h). */
/* GAME-020 Interface.c (interface_unit.h). */

/* ---- part 2 imports, group 2F: end ---- */

/* ---- part 2 imports, group 2G (00431580-00432490): begin ---- */

/* legoGlobs block-update lists the port models (level.cpp): 0055684c,
 * 00556e8c, 00556b6c, 00556e90, 00556528, 00556848. 100 pairs each. */
extern int32_t s_cell_refresh_track[100][2];
extern int32_t s_cell_previous_track[100][2];
extern int32_t s_cell_visited_track[100][2];
extern int s_cell_visited_track_count;
/* Lego.c .bss (lego.cpp): the neighbour-pick rotation, 004de588. */
extern uint32_t s_neighbor_pick_rotation;
/* G98 Map file images (g98_024.h): 004894a0; lego.cpp (level.hpp): 004334c0. */
unsigned char* level_map_slot_get(unsigned int slot);
/* GAME-021 part 3: 00432770 (shared in the matching build), 00432a20. */
int terrain_cell_walkability_query_current(const struct EntitySlot* entity,
                                           uint32_t x, uint32_t y,
                                           int require_stat_bit,
                                           int gated_mode);
int lego_terrain_cell_allows_placement(struct EntitySlot* liveObj,
                                       unsigned int bx, unsigned int by); /* 00432a20, part 3 */
/* placement.cpp (level.hpp): the block's path descriptor (004090a0). */
/* Stats.c (entity.hpp): 00469e80. */
uint32_t entity_state_flags_get(const struct EntitySlot* e);
/* LegoObject_GetBlockPos (004424f0) returns whether the position is on the
 * map; the port's entry returns nothing. Adapter at the end of lego.cpp. */
int entity_block_pos_get_adapter(const struct EntitySlot* entity,
                                 int* bx, int* by);
#if defined(_MSC_VER) && _MSC_VER < 1200
/* Matching build: LegoObject_GetBlockPos itself (legoobject_unit.c), which
 * returns the on-map flag. entity_position_to_terrain_cell_adapter's
 * declaration (above) names it in the matching build
 * (legoobject_port_aliases.h) with the port's void result, hence the cast.
 * Defined after the declaration, which keeps its own name. */
#define entity_block_pos_get_adapter(entity, bx, by) \
    (((int (*)())entity_position_to_terrain_cell_adapter)((entity), (bx), (by)))
#endif
/* GAME-026 Map3D.c (terrain_map.h): 0044f640, 004507d0. */
/* GAME-008 Effects (rock_fall_styles.cpp, resource.hpp): the rock fall,
 * 0040c230. */
int scene_slot_pose_scale_register(int group, uint32_t first, uint32_t second,
                                   float x, float y, float z,
                                   float heading, float pitch);

/* ---- part 2 imports, group 2G: end ---- */

/* ---- part 3 imports, group 3A (004324d0-00433330): begin ---- */

/* ---- part 3 imports, group 3A: end ---- */

/* ---- part 3 imports, group 3B (00433380-00434530): begin ---- */

/* ---- part 3 imports, group 3B: end ---- */

/* ---- part 3 imports, group 3C (00434570-00435aa0): begin ---- */

/* ---- part 3 imports, group 3C: end ---- */

#if defined(_MSC_VER) && _MSC_VER < 1200 && !defined(__cplusplus)
/* Images.c is images_unit.c in the matching build (finding 632): Lego.c's
 * own declaration of Image_SetPenZeroTrans goes to the unit. */
#define resource_record_add_ref Image_SetPenZeroTrans /* 0047dbc0 */
#endif

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_LEGO_UNIT_IMPORTS_H */
