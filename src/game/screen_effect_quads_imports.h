// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-009 imports: C declarations of the callees and globals the unit
 * (screen_effect_quads.c: the credits loop and the damage font) uses from
 * other modules (ADR 0005 step 6; finding 609). Only screen_effect_quads.c
 * and test_c_boundaries.cpp include it, so the other C units' symbol
 * numbering does not change (finding 568, "Unit-private imports header").
 * Each declaration names a definition with C linkage, by the name it has in
 * the matching build. */
#ifndef LEGORRR_GAME_SCREEN_EFFECT_QUADS_IMPORTS_H
#define LEGORRR_GAME_SCREEN_EFFECT_QUADS_IMPORTS_H

#include <stdint.h>

#include "gods98/avi_player.h"
#include "gods98/container.h"
#include "gods98/record.h"
#include "gods98/view.h"

#ifdef __cplusplus
extern "C" {
#endif

struct EntitySlot;
struct Lego_Object;
struct EngineTaskCallbackRecord;
struct MeshRecordPool;
struct TextureSurface;

/* gods98 Main.c (crt.cpp, crt_mouse.h, frameloop.cpp): appWidth
 * (00505924), appHeight (00505928), the left mouse button (0076ab3c) and
 * Main_LoopUpdate. */
extern int s_display_w;
extern int s_display_h;
extern uint32_t g_renderer_word_c;
void main_loop_frame_pump(int draw_projection_overlay);           /* 00477f30 */

/* gods98 Utils.c (utils.c): Util_Tokenise. */
int sound_variant_split(char* text, char** out, const char* separators); /* 00477440 */

/* gods98 Font.c (font.c): Font_GetHeight, by the name its definition takes
 * in the matching build (font_link_names.h), and Font_PrintF. Natively
 * Font_PrintF is bitmap_font_port.cpp's. */
uint32_t object_field_be4_dword_get(const void* record);          /* 0047a540 */
void font_print_f(const struct TextureSurface* font, int x, int y,
                  const char* msg, ...);                          /* 0047a180 */
/* Font.h's inline Font_GetStringWidth (advisor.c). */
uint32_t formatted_text_printf_width(const struct TextureSurface* font,
                                     const char* text, ...);      /* 00401c50 */

/* gods98 Input.c (input_unit.cpp): Input_AnyKeyPressed. */
int input_keyboard_down_key_count(void);                          /* 0047efb0 */

/* gods98 Files.c (files.c): File_LoadBinary. */
void* file_load_binary(const char* filename,
                       unsigned int* sizeptr);                    /* 004800a0 */

/* GAME-041 SFX.c (sfx.c): SFX_Update, by the port's name that its
 * definition takes in the matching build (sfx_link_names.h). */
void sound_subsystem_frame_tick(float dt);                        /* 004651a0 */

/* GAME-021 Lego.c: the current camera's container. */
uint32_t lego_context_selected_global_pointer_get(void);          /* 004295a0 */

/* GAME-023 LegoObject.c: LegoObject_GetActivityContainer. */
void* legoobject_type_component_get(struct Lego_Object* obj);      /* 0044a200 */

/* GAME-043 Stats.c (entity_stats.c), by the names its definitions take in
 * the matching build (entity_stats_link_names.h): StatsObject_GetCollHeight
 * and StatsObject_GetStatsFlags2. */
float entity_stats_float_at74(const struct EntitySlot* e);       /* 00469d50 */
uint32_t entity_state_flags_get(const struct EntitySlot* e);     /* 00469e80 */

/* gods98 Container.c (scene_pool.hpp, view.hpp, record.h, view.h). The
 * container's master frame is at +0x04. */
void* ddiscene_object_allocate(const void* parent);              /* 00472950 */
int ddiscene_object_position_get(void* object, void* alternate_frame,
                                 float position[3]);              /* 004754c0 */
int ddiscene_object_facing_pair_get(void* object, void* alternate_frame,
                                    float first[3], float second[3]); /* 00475500 */
void ddiscene_scene_object_attach_or_unlink(void* pair_record,
                                            void* parent);        /* 00475710 */
/* gods98 Viewports.c (viewports.c): Viewport_GetCamera, by its matching
 * build name (viewports_link_names.h). */
uint32_t scene_object_query_value_if_ready(void* object);         /* 00476f70 */

/* gods98 Mesh.c (mesh_unit.cpp; view.hpp, surface.hpp, record.hpp). */
struct EngineTaskCallbackRecord* engine_task_callback_record_allocate_and_initialize(
    void* owner, void* callback, uint32_t mode, void* context,
    uint32_t flag_value);                                         /* 00480870 */
int mesh_record_allocate_and_fill(struct MeshRecordPool* pool,
                                  uint32_t vertex_count,
                                  uint32_t index_rows,
                                  uint32_t index_columns,
                                  const uint16_t* source_indices); /* 00482220 */
void instance_quad_vertex_data_fill(void* record, int block,
                                    int first_vertex, int count,
                                    const float* positions,
                                    const float triple[3],
                                    const uint32_t* pairs);       /* 00482500 */
void indexed_record_flag6c_bit0_set(void* record, int index, int set); /* 004827a0 */
void animated_texture_frame_surface_set(void* texture, int frame_index,
                                        const void* frame_state); /* 004830c0 */
uint32_t render_object_color_vector_set(void* record, int block, float red,
                                        float green, float blue,
                                        int slot);                /* 00483270 */
/* 00482e70 Mesh_LoadTexture has no port entry, and 00483420
 * Mesh_SetGroupMaterialValues has C++ linkage only (the frozen
 * record_from_mesh_unit.hpp), so the existing unannotated adapters stand
 * in: sfx_port.cpp's (sfx.c) and lws_port.cpp's (lws.c). */
void* sfx_mesh_load_texture_adapter(const char* basePath, const char* name,
                                    unsigned int* width,
                                    unsigned int* height);        /* 00482e70 */
uint32_t mesh_group_material_values_set(void* mesh, int group, float value,
                                        int type);                /* 00483420 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_SCREEN_EFFECT_QUADS_IMPORTS_H */
